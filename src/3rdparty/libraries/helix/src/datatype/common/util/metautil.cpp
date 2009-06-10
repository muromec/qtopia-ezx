/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: metautil.cpp,v 1.7 2007/07/06 22:00:23 jfinnecy Exp $
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

#include "hlxclib/string.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxccf.h"

HX_RESULT CheckMetaInfoRequest(IHXRequest*            pRequest,
                               IHXCommonClassFactory* pFactory,
                               REF(HXBOOL)              rbAcceptMetaInfoPresent,
                               REF(HXBOOL)              rbAllMetaInfoRequested,
                               REF(IHXValues*)        rpRequestedMetaInfo)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pRequest && pFactory)
    {
        IHXValues* pReqHdrs = NULL;
        retVal = pRequest->GetRequestHeaders(pReqHdrs);
        if (SUCCEEDED(retVal) && pReqHdrs)
        {
            IHXBuffer* pBuffer = NULL;
            pReqHdrs->GetPropertyCString("AcceptMetaInfo", pBuffer);
            if (pBuffer)
            {
                // "AcceptMetaInfo" is present
                rbAcceptMetaInfoPresent = TRUE;
                // Get the string to examine
                const char* pszStr = (const char*) pBuffer->GetBuffer();
                // Check if it's "*"
                if (!strcmp(pszStr, "*"))
                {
                    // All meta info is requested
                    rbAllMetaInfoRequested = TRUE;
                    rpRequestedMetaInfo    = NULL;
                }
                else
                {
                    // We must parse the string
                    IHXValues* pInfo = NULL;
                    // We're going to use strtok() which is
                    // destructive, so copy the string first
                    char* pszCopy = new char [strlen(pszStr) + 1];
                    if (pszCopy)
                    {
                        const char* pszSep = " \t\r\n,;";
                        char* pszTok = strtok(pszCopy, pszSep);
                        while (pszTok)
                        {
                            if (!pInfo)
                            {
                                pFactory->CreateInstance(CLSID_IHXValues,
                                                         (void**) &pInfo);
                            }
                            if (pInfo)
                            {
                                pInfo->SetPropertyULONG32(pszTok, 1);
                            }
                            pszTok = strtok(NULL, pszSep);
                        }
                    }
                    HX_VECTOR_DELETE(pszCopy);
                    // Did we get any valid parsed strings?
                    if (pInfo)
                    {
                        rbAllMetaInfoRequested = FALSE;
                        HX_RELEASE(rpRequestedMetaInfo);
                        rpRequestedMetaInfo = pInfo;
                        rpRequestedMetaInfo->AddRef();
                    }
                    else
                    {
                        // AcceptMetaInfo was present, but there
                        // weren't any valid strings in it, so we fail
                        retVal = HXR_FAIL;
                    }
                    // Release the values
                    HX_RELEASE(pInfo);
                }
            }
            else
            {
                // "AcceptMetaInfo" is not present in
                // the request headers
                rbAcceptMetaInfoPresent = FALSE;
            }
            HX_RELEASE(pBuffer);
        }
        else
        {
            // No request headers
            rbAcceptMetaInfoPresent = FALSE;
        }
        HX_RELEASE(pReqHdrs);
    }

    return retVal;
}

