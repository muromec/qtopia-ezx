/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxurlwrp.cpp,v 1.12 2007/07/06 21:58:07 jfinnecy Exp $
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

/*
 *
 * Abstraction:
 * Provides a "wrapper" for a URL. A UrlWrapper is a html file that causes
 * a refresh to the "wrapped" URL. Since there is a limit to the length of URLs
 * that can be passed to a browser via DDE or the command line, UrlWrapper
 * provides a work-around the length limit.
 *
 */

#include "hxurlwrp.h"
#include "hxstrutl.h"
#include "hxresult.h"
#include "chxdataf.h"
#include "hxpref.h"
#include "hxbuffer.h"
#include "pckunpck.h"

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/fcntl.h"

#include "hxheap.h"
#include "hxver.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// Create a temporarly html file that will immediately "refresh" to the original URL
// This works around the Win32 256 char limit sending URLs to browser via DDE or command line.
// WARNING: We are assuming the URL is safe to embed into a META refresh command. IE, the
// URL needs to be properly URL-escaped.

HX_RESULT CHXUrlWrapper::Wrap(const char* szOldUrl, CHXString* pNewUrl, IUnknown* pContext)
{
    HX_RESULT nResult = HXR_OK;

    CHXDataFile*    pDataFile = CHXDataFile::Construct(pContext);
    HX_ASSERT(pDataFile);

    if (pDataFile == NULL)
	return HXR_OUTOFMEMORY;

    // Go through a lot just to get a file called 'G2Play.htm' in a guaranteed
    // temp directory.
    char  szTempFull[_MAX_PATH] = ""; /* Flawfinder: ignore */
    
    pDataFile->GetTemporaryFileName("HX", szTempFull, _MAX_PATH);
    *pNewUrl = szTempFull;
    pDataFile->Delete(szTempFull);

#ifndef _MACINTOSH
    int nIndex = pNewUrl->ReverseFind('.');
    if(nIndex != -1)
	*pNewUrl = pNewUrl->Left(nIndex);
#endif
    *pNewUrl += ".htm";

    IHXPreferences* pSharedPrefs = new HXPreferences;
    if(pSharedPrefs)
    {
	pSharedPrefs->AddRef();
	((HXPreferences*)pSharedPrefs)->OpenShared(HXVER_COMMUNITY);

	IHXBuffer* pBuffer = NULL;
	if(pSharedPrefs->ReadPref("LastTempFile", pBuffer) == HXR_OK)
	{
	    pDataFile->Delete((const char*)pBuffer->GetBuffer());
	    HX_RELEASE(pBuffer);
	}

	if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (BYTE*)(const char*)*pNewUrl, 
					    pNewUrl->GetLength() + 1, pContext))
	{
	    pSharedPrefs->WritePref("LastTempFile", pBuffer);
	    HX_RELEASE(pBuffer);
	}

        HX_RELEASE(pSharedPrefs);
    }

    if (SUCCEEDED(nResult))
    {
	nResult = pDataFile->Open( (const char *)(*pNewUrl), O_CREAT|O_TRUNC|O_WRONLY, TRUE);
    }

    if (SUCCEEDED(nResult))
    {
#ifdef _MACINTOSH
    	char* pSlash = pNewUrl->GetBuffer(pNewUrl->GetLength());
    	while ((pSlash = strchr(pSlash, ':')) != 0)
    	    *pSlash = '/';
    	pNewUrl->ReleaseBuffer();
    	
#endif
    	CHXString tempData = "<HEAD>\n<META HTTP-EQUIV=\"refresh\" CONTENT=\"0;URL=";
    
    	nResult = pDataFile->Write(tempData, tempData.GetLength());
    	if (SUCCEEDED(nResult))
    	{
	    // XXXJL_SECURITY If the URL contains any of these reserved characters that attempt to end the CONTENT attribute
	    // or the META tag, then we truncate the URL at this character.  This makes sure that the CONTENT tag is only
	    // ended by this code so arbitrary html (mainly script) cannot be injected
	    const char pDisallowedChars[] = "<>\"";
	    const char* pFirstDisallowedChar = ::strpbrk(szOldUrl, pDisallowedChars);
	    UINT32 ulLength = ::strlen(szOldUrl);
	    if (pFirstDisallowedChar)
	    {
		ulLength = pFirstDisallowedChar - szOldUrl;
	    }
    	    pDataFile->Write(szOldUrl, ulLength);
    	}
    	if (SUCCEEDED(nResult))
    	{
    	    pDataFile->Write("\">\n", 3 );
    	    pDataFile->Write("</HEAD>\n", 8 );
    	}
    	
    	pDataFile->Close();
    }

    HX_DELETE(pDataFile);
    
    return (nResult);
}

