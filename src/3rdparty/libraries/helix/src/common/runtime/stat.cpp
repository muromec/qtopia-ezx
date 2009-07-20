/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: stat.cpp,v 1.9 2008/01/18 09:17:26 vkathuria Exp $
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

#include "hlxclib/sys/stat.h"
#include "hlxclib/windows.h"

#if defined(WIN32_PLATFORM_PSPC)

int __helix_stat(const char* pFilename, struct stat *buf)
{
    int ret = -1;
    
    HANDLE hFile = CreateFile(OS_STRING(pFilename), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
			      FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile != INVALID_HANDLE_VALUE)
    {
	buf->st_mode = 0;
	buf->st_size = GetFileSize(hFile, 0);

	FILETIME createTime;
	FILETIME accessTime;
	FILETIME modifyTime;
	if (GetFileTime(hFile, &createTime, &accessTime, &modifyTime))
	{
	    buf->st_atime = accessTime.dwLowDateTime;
	    buf->st_ctime = createTime.dwLowDateTime;
	    buf->st_mtime = modifyTime.dwLowDateTime;

	    ret = 0;
	}

	CloseHandle(hFile);
    }

    return ret;
}

int __helix_fstat(int filedes, struct stat *buffer)
{
        /// not supported by current V7
	int ret = 1;
	return ret;
}

#elif defined(_OPENWAVE)

int __helix_stat(const char *path, struct stat *buffer)
{
	int ret = 0;
	OpFsStatStruct opstat;
	if (OpFsStat(path, &opstat) == kOpFsErrAny)
	{
		ret = -1;
	}
	buffer->st_mode = (mode_t)opstat.attr;
	buffer->st_size = opstat.size;
	return ret;
}


int __helix_fstat(int filedes, struct stat *buffer)
{
        /// not supported by current V7
	int ret = 1;
	return ret;
}

#elif defined(_BREW)

int __helix_stat(const char *path, struct stat *buffer)
{
    HX_ASSERT(0);
    return 0;
}

int __helix_fstat(int filedes, struct stat *buffer)
{
    HX_ASSERT(0);
    return 0;
}

#endif /* defined(WIN32_PLATFORM_PSPC) */

