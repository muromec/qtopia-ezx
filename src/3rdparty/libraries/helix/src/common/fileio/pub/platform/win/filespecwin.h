/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filespecwin.h,v 1.4 2005/03/14 19:36:32 bobclark Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef FILESPECWIN_H
#define FILESPECWIN_H

#include "hxstring.h"

class CHXFileSpecifier;
class CHXDirSpecifier;

// this utility class helps out the two classes below.  It breaks a path up into these components:
//
//  C:/dir1/dir2/dir3/file.ext
//  ^ ^              ^^   ^^
//
// Each arrow above points to the start of a component.  The components are:
//
// volume
// parent [path]
// separator [note: this may be more than one character, for instance:  c:/dir1////file1.txt, which is equivalent to c:/dir1/file1.txt]
// name [for a file, the filename, for a directory, the directory name
// extension separator [I think this is always a single '.']
// extension [note that even directories may have extensions].
//
// Here are some special cases that are handled:
//
// /dir1/dir2/  -- a path with no volume, and trailing slash.
//
//     volume: ""
//     parent: "/dir1"
//     separator: "/"
//     name: "dir2"
//     extension separator: ""
//     extension: ""
//
// \\hostname\drive_1\dev\file1.
//
//     volume: "\\hostname"
//     parent: "\drive_1\dev"
//     separator: "\"
//     name: "file1"
//     extension separator: "."
//     extension: ""
//
// C:bogus.txt -- relative path, no directory
//
//     volume: "C:"
//     parent: ""
//     separator: ""
//     name: "bogus"
//     extension separator: "."
//     extension: "txt"
//
// 


class CHXPathParser
{
    friend class CHXFileSpecifier;
    friend class CHXDirSpecifier;

private:
    CHXPathParser();

    CHXString GetPathName() const;
    HXBOOL IsSet() const;
	void UnSet();

    CHXPathParser &operator=(const CHXPathParser &other);
    HXBOOL operator==(const CHXPathParser &other);
    HXBOOL operator!=(const CHXPathParser &other);

    void ParsePath(const char *psz);

    CHXString GetSubstring(int nStart, int nLength) const;

    CHXString GetPersistentString() const;
    void SetFromPersistentString(const char *pBuffer);



    CHXString m_path;
    HXBOOL m_bEmpty;

    HXBOOL m_bIsValid;
    HXBOOL m_bCannotBeFile;
    HXBOOL m_bAbsolute;

    int m_nVolumeLength;
    int m_nParentLength;
    int m_nSeparatorLength;
    int m_nNameLength;
    int m_nExtensionSeparatorLength;
    int m_nExtensionLength;
};

#endif // FILESPECWIN_H
