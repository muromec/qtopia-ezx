/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: filetestutil.cpp,v 1.4 2004/07/09 18:20:32 hubbe Exp $
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

#include <stdio.h>

#include "filetestutil.h"

#include "hlxclib/string.h"


UINT16 CFileTestUtil::ParseModeString(const char* pModeString)
{
    UINT16 uMode = 0;
    
    while (*pModeString)
    {
	switch (*pModeString)
	{
	case 'r':
	    uMode |= HX_FILEFLAG_READ;
	    break;
	case 'w':
	    uMode |= HX_FILEFLAG_WRITE;
	    break;
	case 'b':
	    uMode |= HX_FILEFLAG_BINARY;
	    break;
	case 'n':
	    uMode |= HX_FILEFLAG_NOTRUNC;
	    break;
	default:
	    break;
	}
	
	pModeString++;
    }

    return uMode;
}

bool CFileTestUtil::ParseResultString(const char* pModeString)
{
    bool bResult = true;
    
    if (strcmp(pModeString, "FAIL") == 0)
    {
	bResult = false;
    }

    return bResult;
}

bool CFileTestUtil::ParseBoolString(const char* pModeString)
{
    bool bResult = true;
    
    if (strcmp(pModeString, "FALSE") == 0)
    {
	bResult = false;
    }

    return bResult;
}

bool CFileTestUtil::CheckBuffer(const char* pString, IHXBuffer* pHXBuffer)
{
    return CheckStorage(pString, pHXBuffer, 0);
}

bool CFileTestUtil::CheckString(const char* pString, IHXBuffer* pHXBuffer)
{
    return CheckStorage(pString, pHXBuffer, 1);
}

bool CFileTestUtil::CheckStorage(const char* pString, IHXBuffer* pHXBuffer, INT32 lHXBufEndOffset)
{
    bool bRetVal = false;

    if (pHXBuffer && pString && (strlen(pString) == (pHXBuffer->GetSize() - lHXBufEndOffset)))
    {
	if (memcmp(pString, 
		   pHXBuffer->GetBuffer(),
		   pHXBuffer->GetSize() - lHXBufEndOffset) == 0)
	{
	    bRetVal = true;
	}
    }

    return bRetVal;
}

UINT16 CFileTestUtil::ParseSeekModeString(const char* pSeekRefString)
{
    UINT16 uSeekMode = SEEK_SET;

    if (strcmp(pSeekRefString, "CUR") == 0)
    {
	uSeekMode = SEEK_CUR;
    }
    else if (strcmp(pSeekRefString, "END") == 0)
    {
	uSeekMode = SEEK_END;
    }

    return uSeekMode;
}

XHXDirectory::FSOBJ CFileTestUtil::ParseFSOBJString(const char* pFSOBJString,
						    XHXDirectory::FSOBJ okObj)
{
    XHXDirectory::FSOBJ retObj = XHXDirectory::FSOBJ_NOTVALID;

    if (strcmp(pFSOBJString, "FILE") == 0)
    {
	retObj = XHXDirectory::FSOBJ_FILE;
    }
    else if (strcmp(pFSOBJString, "DIR") == 0)
    {
	retObj = XHXDirectory::FSOBJ_DIRECTORY;
    }
    else if (strcmp(pFSOBJString, "OK") == 0)
    {
	retObj = okObj;
    }

    return retObj;
}
