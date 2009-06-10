/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pacutil.cpp,v 1.7 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxslist.h"
#include "hxbuffer.h"
#include "hxdir.h"
#include "pacutil.h"
#include "hxstrutl.h"
#include "pckunpck.h"

HX_RESULT 
ParsePACInfo(char* pszPACInfo, CHXSimpleList*& pPACInfoList)
{
    HX_RESULT	rc = HXR_OK;
    INT32	i = 0;
    char*	pToken = NULL;
    PACInfo*	pPACInfo = NULL;

    HX_ASSERT(!pPACInfoList || pPACInfoList->GetCount() == 0);

    if (pszPACInfo)
    {
	pToken = strtok(pszPACInfo, ";");
	
	// At least one entry should present
	HX_ASSERT(pToken);
    
        while (pToken)
	{
	    pPACInfo = NULL;

	    CHXString entry = pToken;

	    entry.TrimLeft();
	    entry.TrimRight();

	    i = entry.Find(' ');
	    // DIRECT - no host::port info
	    if (-1 == i)
	    {		
		pPACInfo = new PACInfo;
		pPACInfo->type = PAC_DIRECT;
	    }
	    else
	    {
		CHXString type;
		CHXString proxyinfo;
		CHXString host;
		CHXString port;

		type = entry.NthField(' ', 1);
		proxyinfo = entry.NthField(' ', 2);
		
		i = proxyinfo.Find(':');
		if (-1 == i)
		{
		    host = proxyinfo;
		}
		else
		{
		    host = proxyinfo.NthField(':', 1);
		    port = proxyinfo.NthField(':', 2);
		}

		// we treat SOCKS the same as PROXY
		pPACInfo = new PACInfo;
		pPACInfo->type = PAC_PROXY;

		pPACInfo->pszHost = new char[host.GetLength() + 1];
		strcpy(pPACInfo->pszHost, (const char*)host); /* Flawfinder: ignore */

		if (!port.IsEmpty())
		{
		    pPACInfo->ulPort = atoi((const char*)port);
		}
	    }

	    if (pPACInfo)
	    {
		if (!pPACInfoList)
		{
		    pPACInfoList = new CHXSimpleList();
		}

		pPACInfoList->AddTail(pPACInfo);
	    }
	    
	    pToken = strtok(NULL, ";");
	}
    }

    return rc;
}

// used by HXPreferredTransportManager and HXPACPlugin to
// manage persistent config files
// <filename0>,<expiration0>;<filename1>,<expiration1>;<filename2>,<expiration2>
HX_RESULT 
AddFileToFileListWithCap(const char* pszNewFile, 
			 UINT32 ulExpiration, 
			 const char* pszPath, 
			 IHXBuffer*& pBuffer,
			 IUnknown* pContext)
{
    HX_RESULT	    rc = HXR_OK;
    int		    nFields = 0;
    int		    nFiles = 0;
    int		    i = 0;
    char	    buffer[20] = {0}; /* Flawfinder: ignore */
    char*	    pszFile = NULL;
    CHXString	    filesIn;
    CHXString	    filesOut;
    CHXString	    fileInfo;
    CHXString	    fileName;
    CHXDirectory    Dir;

    filesOut = pszNewFile;
    filesOut +=",";
    filesOut += itoa(ulExpiration, buffer, 10);
    nFiles++;

    if (pBuffer)
    {
	filesIn = (const char*)pBuffer->GetBuffer();
    
	nFields = filesIn.CountFields(';');
	for (i = 1; i <= nFields; i++)
	{
	    fileInfo = filesIn.NthField(';', i);
	    fileName = fileInfo.NthField(',', 1);

	    if (fileName.CompareNoCase(pszNewFile) != 0)
	    {
		if (nFiles >= MAX_CFG_FILES)
		{
		    pszFile = new char[strlen(pszPath) + fileName.GetLength() + 10];
		    ::strcpy(pszFile, pszPath); /* Flawfinder: ignore */
		    if (pszFile[::strlen(pszFile)-1] != OS_SEPARATOR_CHAR)
		    {
			strcat(pszFile, OS_SEPARATOR_STRING); /* Flawfinder: ignore */
		    }

		    strcat(pszFile, (const char*)fileName); /* Flawfinder: ignore */

		    CHXDirectory Dir;
		    Dir.DeleteFile(pszFile);

		    HX_VECTOR_DELETE(pszFile);
		}
		else
		{
		    filesOut += ";";
		    filesOut += fileInfo;
		    nFiles++;
		}
	    }
	}
    }
    else
    {
	CreateBufferCCF(pBuffer, pContext);
    }

    pBuffer->Set((const UCHAR*)(const char*)filesOut, filesOut.GetLength() + 1);

    return rc;
}

// used by HXPreferredTransportManager and HXPACPlugin to
// manage persistent config files
// <filename0>,<expiration0>;<filename1>,<expiration1>;<filename2>,<expiration2>
HX_RESULT 
GetFileFromFileListWithCap(const char* pszNewFile, UINT32& ulExpiration, IHXBuffer* pBuffer, IUnknown* pContext)
{
    HX_RESULT	    rc = HXR_FAILED;
    int		    nFields = 0;    
    int		    i = 0;
    CHXString	    filesIn;
    CHXString	    fileInfo;
    CHXString	    fileName;
    CHXString	    fileExpiration;

    if (pBuffer)
    {
	filesIn = (const char*)pBuffer->GetBuffer();
    
	nFields = filesIn.CountFields(';');
	for (i = 1; i <= nFields; i++)
	{
	    fileInfo = filesIn.NthField(';', i);

	    fileName = fileInfo.NthField(',', 1);
	    fileExpiration = fileInfo.NthField(',', 2);

	    if (fileName.CompareNoCase(pszNewFile) == 0)
	    {
		ulExpiration = atoi((const char*)fileExpiration);
		rc = HXR_OK;
		break;
	    }
	}
    }

    return rc;
}


