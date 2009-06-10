/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: corshare.cpp,v 1.16 2008/07/30 20:17:56 amsaleem Exp $
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

#include "hxcom.h"
#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif

#include "prefdefs.h"
#include "plprefk.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxpref.h"
#include "hxausvc.h"
#include "hxmon.h"
#include "hxclreg.h"
#include "hxgroup.h"
#include "hxsmbw.h"

#include "chxeven.h"
#include "chxelst.h"
#include "hxmap.h"
#include "hxrquest.h"
#include "hxmangle.h"

#include "hxstrutl.h"
#include "strminfo.h"
#include "timeval.h"
#include "smiltype.h"
#include "hxbsrc.h"
#include "hxsrc.h"
#include "srcinfo.h"
#include "pckunpck.h"

#include "clntcore.ver"
#include "hxver.h"
#include "hxwinver.h"

// will be taken out once flags are defined in a separate file
#include "rmfftype.h"	
#include "hxplay.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

HX_RESULT 
SetRequest(const char*		pURL, 
	   HXBOOL			bAltURL, 
	   IHXPreferences*	pPreferences,
	   IHXRegistry*	pRegistry,
	   IHXValues*		pValuesInRequest,
           IHXCommonClassFactory* pCCF,
	   REF(IHXRequest*)	pRequest)
{
    HX_RESULT		hr = HXR_OK;
    HXBOOL		bSetRequestHeader = FALSE;
    CHXString           szBandwidth;
    IHXBuffer*		pRegionData = NULL;
    IHXBuffer*		pLanguage = NULL;
    IHXValues*		pHeaders = NULL;
    IHXBuffer*		pClientID = NULL;
    IHXBuffer*		pGUID = NULL;
    IHXBuffer*		pMaxASMBW = NULL;
    IHXBuffer*		pValue = NULL;
    HXBOOL		bCanSendGuid = FALSE;

    if (!pCCF)
    {
        return HXR_FAIL;
    }

    if (!pRequest)
    {
	// create our own if it doesn't exist
        pCCF->CreateInstance(CLSID_IHXRequest, (void**) &pRequest);
        if(!pRequest)
        {
            hr = HXR_OUTOFMEMORY;
            goto exit;
        }
    }

    pRequest->SetURL(pURL);    

    if(pPreferences)
    {
        ReadPrefCSTRING(pPreferences, "Bandwidth", szBandwidth);
        ReadPrefBOOL(pPreferences, "AllowAuthID", bCanSendGuid);
    }

    if (pRegistry)
    {
	CHXString strTemp;
	strTemp = HXREGISTRY_PREFPROPNAME;
	strTemp += '.';
	strTemp += CLIENT_ID_REGNAME;
	pRegistry->GetStrByName(strTemp, pClientID);

	strTemp = HXREGISTRY_PREFPROPNAME;
	strTemp += ".RegionData";
	pRegistry->GetStrByName(strTemp, pRegionData);

	strTemp = HXREGISTRY_PREFPROPNAME;
	strTemp += ".Language";
	pRegistry->GetStrByName(strTemp, pLanguage);
    }
    if (pClientID == NULL)
    {
        // Encode the client ID with the pieces of interest.
        HXVERSIONINFO verInfo;
        HXGetWinVer(&verInfo);
        const char* pszClientID = HXGetVerEncodedName(&verInfo,
                                                      PRODUCT_ID,
                                                      TARVER_STRING_VERSION,
                                                      LANGUAGE_CODE,
                                                      "RN01");
        // Set clientID
        CreateStringBufferCCF(pClientID, pszClientID, pCCF);
    }
 
    if(bCanSendGuid &&
       pPreferences && 
       pPreferences->ReadPref(CLIENT_GUID_REGNAME, pGUID) == HXR_OK)
    {
	char* pszGUID = DeCipher((char*)pGUID->GetBuffer());
	HX_RELEASE(pGUID);
        pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pGUID);
        if(pGUID)
        {
	    pGUID->Set((UCHAR*) pszGUID, strlen(pszGUID) + 1);
        }
        else
        {
            hr = HXR_OUTOFMEMORY;
            goto exit;
        }
	delete[] pszGUID;
    }
    else
    {
        pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pGUID);
        if(pGUID)
        {
	    pGUID->Set((UCHAR*) CLIENT_ZERO_GUID, sizeof(CLIENT_ZERO_GUID));
        }
        else
        {
            hr = HXR_OUTOFMEMORY;
            goto exit;
        }
    }

    // set attributes to the request header
    if (HXR_OK != pRequest->GetRequestHeaders(pHeaders) || !pHeaders)
    {		
        pCCF->CreateInstance(CLSID_IHXValues, (void**) &pHeaders);

        if(!pHeaders)
        {
            hr = HXR_OUTOFMEMORY;
            goto exit;
        }

	bSetRequestHeader = TRUE;
    }

    pHeaders->SetPropertyULONG32("IsAltURL", bAltURL);

    if (HXR_OK != pHeaders->GetPropertyCString("Bandwidth", pValue) &&
        !szBandwidth.IsEmpty())
    {
        SetCStringPropertyCCF(pHeaders, "Bandwidth", (const char*)szBandwidth, pCCF);
    }
    HX_RELEASE(pValue);

    if (HXR_OK != pHeaders->GetPropertyCString("Language", pValue) &&
	pLanguage && pLanguage->GetSize())
    {
	pHeaders->SetPropertyCString("Language", pLanguage);
    }
    HX_RELEASE(pValue);

    if (HXR_OK != pHeaders->GetPropertyCString("RegionData", pValue) &&
	pRegionData && pRegionData->GetSize())
    {
	pHeaders->SetPropertyCString("RegionData", pRegionData);
    }
    HX_RELEASE(pValue);

    if (HXR_OK != pHeaders->GetPropertyCString("ClientID", pValue) &&
	pClientID && pClientID->GetSize())
    {
	pHeaders->SetPropertyCString("ClientID", pClientID);
    }
    HX_RELEASE(pValue);

    if (HXR_OK != pHeaders->GetPropertyCString("GUID", pValue) &&
	pGUID && pGUID->GetSize())
    {
	pHeaders->SetPropertyCString("GUID", pGUID);
    }
    HX_RELEASE(pValue);

    if (HXR_OK != pHeaders->GetPropertyCString ("SupportsMaximumASMBandwidth", pValue))
    {
        pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pMaxASMBW);
        if(!pMaxASMBW)
        {
            hr = HXR_OUTOFMEMORY;
            goto exit;
        }
	pMaxASMBW->Set((UCHAR*)"1",2);
	pHeaders->SetPropertyCString ("SupportsMaximumASMBandwidth",pMaxASMBW);
    }
    HX_RELEASE(pValue);

    if (pValuesInRequest)
    {
	CHXHeader::mergeHeaders(pHeaders, pValuesInRequest);
    }

    if (bSetRequestHeader)
    {
	pRequest->SetRequestHeaders(pHeaders);
    }
exit:
    HX_RELEASE(pMaxASMBW);
    HX_RELEASE(pLanguage);
    HX_RELEASE(pRegionData);
    HX_RELEASE(pClientID);
    HX_RELEASE(pGUID);

    HX_RELEASE(pHeaders);

    return hr;
}

