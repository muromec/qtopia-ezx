/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pathutil.cpp,v 1.7 2007/07/06 20:39:16 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#include "hxstring.h"
#include "hxslist.h"
#include "hxstringutil.h"
#include "pathutil.h"


// return joined path using path delimiter
CHXString
HXPathUtil::CombinePath(const char* base, const char* tail, char pathDelim)
{
    CHXString strPath = base;
    if(!strPath.IsEmpty())
    {
        INT32 idxLast = strPath.GetLength() - 1;
        if(strPath[idxLast] != pathDelim)
        {
            strPath += pathDelim;
        }
    }
    strPath += tail;
    return strPath;
}


// appends 'right' to parent directory that contains 'left'
//
// assumes input is net-path format
//
CHXString HXPathUtil::GetNetPathRelativePath(const CHXString& left, 
                                      const CHXString& right, 
                                      bool hardRoot)
{
    CHXString combined;

    // add left part only if right is not absolute
    if(right.GetLength() == 0 || right[0] != '/')
    {
        combined = left;
        if( !combined.IsEmpty() )
        {
            // redundant slashes are handled in NormalizeNetPath()
            combined += "/";
        }
        combined += "../";
    }
    combined += right;

    return NormalizeNetPath(combined, hardRoot);
}

bool HXPathUtil::HasNetPathFolderSuffix(const CHXString& path)
{
    bool hasSuffix = false;
    INT32 cch = path.GetLength();
    if(cch > 0)
    {
        hasSuffix = (path[cch -1] == '/');
    }
    return hasSuffix;
}

void HXPathUtil::EnsureNetPathFolderSuffix(CHXString& path /*in out*/)
{
    if(!HasNetPathFolderSuffix(path))
    {
        path += "/";
    }
}


// Assumes path is normalized. Returns directory part of path.
// 
// foo/bar.rm   => "foo/"
// foo/bar      => "foo/"
// foo/bar/     => "foo/bar/"
// foo          => ""
// ./           => "./"
//
CHXString HXPathUtil::GetNetPathDirectory(const CHXString& path)
{
    CHXString folder;
    INT32 idxLast = path.ReverseFind('/');
    if (-1 != idxLast)
    {
        folder = path.Left(idxLast + 1); // include /
    }
    else
    {
        // no folder
        folder = "";
    }
    return folder;
}

bool HXPathUtil::HasNetPathFolderPrefix(const CHXString& path)
{
    bool hasPrefix = false;
    INT32 cch = path.GetLength();
    if(cch > 0)
    {
        hasPrefix = (path[0] == '/');
    }
    return hasPrefix;
}


// Converts os- or net-path path to normalized net path
// 
// hardRoot determines how following is handled:
//
// "/foo/../../../bar"
//
// if (hardRoot) result is "/bar"
// otherwise result is "/../../bar" 
//
//

CHXString HXPathUtil::NormalizeNetPath(const CHXString& path, bool hardRoot)
{
    // convert all os-path delimiters to forward-slashes
    CHXString fixedPath = path;
    fixedPath.FindAndReplace(OS_SEPARATOR_STRING, "/", TRUE);

    // split into components
    CHXStringList old;
    HXStringUtil::Split(fixedPath, "/", old);

    bool isRooted = HasNetPathFolderPrefix(fixedPath);
    bool hasTrailSlash = false;
   
    CHXStringList normalized;
    CHXSimpleList::Iterator begin = old.Begin();
    CHXSimpleList::Iterator end = old.End();

    for(/**/ ;begin != end; ++begin)
    {
        CHXString* pElement = (CHXString*)*begin;
        HX_ASSERT(pElement);
        if(pElement)
        {
            if(pElement->IsEmpty() || *pElement == ".")
            {
                hasTrailSlash = true;
            }
            else if (*pElement == "..")
            {
                if( normalized.IsEmpty())
                {
                    if( !isRooted || !hardRoot)
                    {
                        normalized.AddTailString(*pElement);
                        hasTrailSlash = true;
                    }
                }
                else
                {
                    CHXString* pString = (CHXString*&)normalized.GetTail();
                    HX_ASSERT(pString);

                    if(*pString == "..")
                    {
                        // add '..'
                        normalized.AddTailString(*pElement);
                    }
                    else 
                    {
                        // pop last ('cd out')
                        normalized.RemoveTailString();
                    }
                    hasTrailSlash = true;
                }

                
            }
            else
            {
                normalized.AddTailString(*pElement);
                hasTrailSlash = false;
            } 
        }

    } // for

    // form final path with normalized path components
    CHXString final;
    if( isRooted )
    {
        final = "/";
    }
    
    final += HXStringUtil::Join(normalized, "/");

    if( final.IsEmpty() )
    {
        final = ".";
        hasTrailSlash = true;
    }

    if(hasTrailSlash)
    {
        EnsureNetPathFolderSuffix(final);
    }

    return final;
 
}


