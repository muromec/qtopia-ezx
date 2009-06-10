/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: buffutil.cpp,v 1.11 2007/10/10 00:50:08 qluo Exp $
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

// system
#include "hlxclib/string.h"
#include "safestring.h"
// include
#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxcomm.h"
#include "hxfiles.h"
// pncont
#include "hxstring.h"
#include "chxpckts.h"
#include "pckunpck.h"
// pnmisc
#include "hxurl.h"
// pxcomlib
#include "buffutil.h"


HX_RESULT PXUtilities::CreateSizedBuffer(UINT32 ulSize, IUnknown* pContext,
                                         REF(IHXBuffer*) rpBuffer)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pContext && ulSize)
    {
        IHXCommonClassFactory* pFactory = NULL;
        retVal                           = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                                                    (void**) &pFactory);
        if (SUCCEEDED(retVal))
        {
            IHXBuffer* pBuffer = NULL;
            retVal              = pFactory->CreateInstance(CLSID_IHXBuffer,
                                                           (void**) &pBuffer);
            if (SUCCEEDED(retVal))
            {
                retVal = pBuffer->SetSize(ulSize);
                if (SUCCEEDED(retVal))
                {
                    HX_RELEASE(rpBuffer);
                    rpBuffer = pBuffer;
                    rpBuffer->AddRef();
                }
            }
            HX_RELEASE(pBuffer);
        }
        HX_RELEASE(pFactory);
    }

    return retVal;
}

HX_RESULT PXUtilities::GetURLParam(IHXRequest*     pRequest,
                                   HXBOOL             bAddBase,
                                   IUnknown*        pContext,
                                   const char*      pszParamName,
                                   REF(IHXBuffer*) rpParamValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pRequest && pContext && pszParamName)
    {
        // First try and get the parameter
        // as an URL-encoded parameter
        const char* pszURL = NULL;
        retVal = pRequest->GetURL(pszURL);
        if (SUCCEEDED(retVal))
        {
            // Create an URL string
            CHXString cDummy;
            if (bAddBase)
            {
                cDummy = "rtsp://chxurl-sucks.com/";
            }
            cDummy += pszURL;
            // Set up a CHXURL object
            CHXURL cURL(cDummy, pContext);
            retVal = cURL.GetLastError();
            if (SUCCEEDED(retVal))
            {
                // Get the URL-encoded parameters from the url object
                IHXValues* pOptions = NULL;
                pOptions = cURL.GetOptions();
                if (pOptions)
                {
                    // If the param is a string, it will be a "buffer" property
                    // coming from CHXURL.
                    HX_RELEASE(rpParamValue);
                    pOptions->GetPropertyBuffer(pszParamName, rpParamValue);
                    if (!rpParamValue)
                    {
                        // If the param is purely digits, then CHXURL will
                        // pass it as a "ULONG32" property
                        UINT32 ulValue = 0;
                        retVal = pOptions->GetPropertyULONG32(pszParamName, ulValue);
                        if (SUCCEEDED(retVal))
                        {
                            // It WAS a ULONG32 property, so we need to
                            // convert it back to a buffer
                            //
                            // UINT32 can't be any more than 10 characters, but
                            // we'll be safe
                            char szTmp[16]; /* Flawfinder: ignore */
                            SafeSprintf(szTmp, sizeof(szTmp), "%lu", ulValue);
                            // Now convert to string
                            retVal = CreateStringBufferCCF(rpParamValue,
							   (const char*) szTmp,
                                                           pContext);
                        }
                    }
                }
                else
                {
                    retVal = HXR_FAIL;
                }
                HX_RELEASE(pOptions);
            }
        }
    }

    return retVal;
}

HX_RESULT PXUtilities::GetRequestParam(IHXRequest*     pRequest,
                                       IUnknown*        pContext,
                                       const char*      pszParamName,
                                       REF(IHXBuffer*) rpParamValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pRequest && pContext && pszParamName)
    {
        // Obtain the request headers from the IHXRequest
        IHXValues* pHeaders = NULL;
        pRequest->GetRequestHeaders(pHeaders);
        if (pHeaders)
        {
            // All properties coming out of the request
            // headers will be CString properties
            HX_RELEASE(rpParamValue);
            retVal = pHeaders->GetPropertyCString(pszParamName, rpParamValue);
        }
        HX_RELEASE(pHeaders);
    }

    return retVal;
}

HX_RESULT PXUtilities::GetURLOrRequestParam(IHXRequest*     pRequest,
                                            HXBOOL             bAddBase,
                                            IUnknown*        pContext,
                                            const char*      pszParamName,
                                            REF(IHXBuffer*) rpParamValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pRequest && pContext && pszParamName)
    {
        // Try first to get it as a URL-encoded parameter
        retVal = PXUtilities::GetURLParam(pRequest, bAddBase, pContext,
                                          pszParamName, rpParamValue);
        if (FAILED(retVal))
        {
            // Didn't get it as a URL-encoded parameter, so
            // try getting it from the request headers
            retVal = PXUtilities::GetRequestParam(pRequest, pContext,
                                                  pszParamName, rpParamValue);
        }
    }

    return retVal;
}

HX_RESULT PXUtilities::GetNullTerminatedBuffer(IUnknown* pContext, IHXBuffer* pInBuffer, REF(IHXBuffer*) pOutBuffer)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pInBuffer && pContext)
    {
	if ( *((char*)(pInBuffer->GetBuffer() + pInBuffer->GetSize() - 1)) == 0)
	{
	    pOutBuffer = pInBuffer;
	    pOutBuffer->AddRef();
	    retVal = HXR_OK;
	}
	else
	{
	    IHXBuffer* pBuffer = NULL;
	    IHXCommonClassFactory* pFactory = NULL;
	    retVal = pContext->QueryInterface(IID_IHXCommonClassFactory, (void**) &pFactory);
	    if (SUCCEEDED(retVal))
	    {
		retVal = pFactory->CreateInstance(CLSID_IHXBuffer, (void**) &pBuffer);
		if (SUCCEEDED(retVal))
		{
		    // add termination
		    pBuffer->Set(pInBuffer->GetBuffer(), pInBuffer->GetSize() + 1);
		    *((UCHAR*)(pBuffer->GetBuffer() + pInBuffer->GetSize())) = 0;
		    pOutBuffer = pBuffer;
		    pOutBuffer->AddRef();
		    HX_RELEASE(pBuffer);
		    retVal = HXR_OK;
		}
		HX_RELEASE(pFactory);
	    }
	}
    }

    return retVal;
}

