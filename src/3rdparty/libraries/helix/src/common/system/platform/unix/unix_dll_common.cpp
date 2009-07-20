/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unix_dll_common.cpp,v 1.7 2007/07/06 20:41:57 jfinnecy Exp $
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

#include "dllacces.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "hlxclib/sys/stat.h"
#include "hxassert.h"
#include "hxstrutl.h"

#include "platform/unix/unix_dll_common.h"

void HandleHXStopOnLoad(const char* libName)
{
    const char *dlls;
    if (dlls = getenv ("HX_STOPONLOAD"))
    {
	if (strstr(dlls, libName))
	{
	    HXDebugBreak();
	}
	else if (!strcmp("all", dlls))
	{
	    HXDebugBreak();
	}
    }
}

char* UnixFindDLLVersion(const char* libName)
{
    char* pRet = 0;

    int rc = 0;
    char tmpPaths[MAXPATHLEN+1]; /* Flawfinder: ignore */
    tmpPaths[0] = '\0';

    // get list of semi-separated directories to search for library
    char* libSearchPaths = getenv("LD_LIBRARY_PATH");
    if(libSearchPaths)
    {
	SafeStrCpy(tmpPaths, libSearchPaths, MAXPATHLEN+1);
	SafeStrCpy(tmpPaths, ";", MAXPATHLEN+1);
    }
    SafeStrCat(tmpPaths, "/usr/lib;/lib", MAXPATHLEN+1);
    char* dirName = strtok(tmpPaths, ";");
    while(dirName)
    {
        struct stat buf;
        char pathName[MAXPATHLEN+1];  // current path /* Flawfinder: ignore */
        char realPath[MAXPATHLEN+1];  // resolved path /* Flawfinder: ignore */

        // lstat dirName/libName to get final inode
        SafeStrCpy(pathName, dirName, MAXPATHLEN+1);
        SafeStrCat(pathName, "/", MAXPATHLEN+1);
        SafeStrCat(pathName, libName, MAXPATHLEN+1);

        while(lstat(pathName, &buf) == 0)
        {
            if(S_ISLNK(buf.st_mode))    // need to get symlink
            {
                char path[MAXPATHLEN+1]; /* Flawfinder: ignore */
                int linklen = readlink(pathName, path, sizeof(path)-1);
                if(linklen < 0) // pathName is not a link
                {
                    if(path[0] == '.')  // relative path
                    {
                        SafeStrCpy(realPath, dirName, MAXPATHLEN+1);
                        SafeStrCat(realPath, "/", MAXPATHLEN+1);
                        SafeStrCat(realPath, path, MAXPATHLEN+1);
                    }
                    else
                        SafeStrCpy(realPath, path, MAXPATHLEN+1);
                    break;
                }
                // next dir
                path[linklen] = '\0';
                SafeStrCpy(pathName, dirName, MAXPATHLEN+1);
                SafeStrCat(pathName, "/", MAXPATHLEN+1);
                SafeStrCat(pathName, path, MAXPATHLEN+1);
            }
            else	// this is the actual filename
            {
		char path[MAXPATHLEN+1]; /* Flawfinder: ignore */
                SafeStrCpy(realPath, pathName, MAXPATHLEN+1);
                if(realpath(realPath, path))
		{
		    // now, finally, get version from actual file name
		    char version[20]; /* Flawfinder: ignore */
		    char tmp[20]; /* Flawfinder: ignore */
		    // walk backwards through the string until a letter is
		    // found ('o', for instance)
		    char* tPtr = tmp;
		    char* vPtr = &path[strlen(path)-1];
		    while(vPtr >= path && tPtr < version + 20 && !isalpha(*vPtr)) // XXX -- this is bogus
			*tPtr++ = *vPtr--;
		    *tPtr = '\0';
		    int len = strlen(tmp);
		    if(len > 0)
		    {
			// copy back to version, skipping over the initial '.'
			vPtr = &version[strlen(tmp)-1];
			*vPtr = '\0';
			vPtr--;
			tPtr = tmp;
			for(int i=0;i<len;i++)
			    *vPtr-- = *tPtr++;

			UINT32 bufSize = strlen(version) + 1;
			pRet = new char[bufSize];
			SafeStrCpy(pRet, version, bufSize);
		    }
		    else
		    {
			pRet = new char[1];
			
			HX_ASSERT(pRet);
			if (pRet)
			    *pRet = '\0';
		    }

                    rc = 1;
		}
                break;
            }
        }
        if(rc)
            break;
        dirName = strtok(NULL, ";");
    }

    return pRet;
}

void
UnixCreateName(const char* short_name, const char* long_name, 
		      char* out_buf, UINT32& out_buf_len, 
		      UINT32 nMajor, UINT32 nMinor)
{
    UINT32  long_name_len;

    out_buf[0] = 0;

    long_name_len = strlen(long_name);

    if (long_name_len + DLLAccess::EXTRA_BUF_LEN > out_buf_len)
    {
	HX_ASSERT(0);
	out_buf_len = 0;
	return;
    }
    out_buf_len = sprintf(out_buf, "%s.so", long_name); /* Flawfinder: ignore */
}
