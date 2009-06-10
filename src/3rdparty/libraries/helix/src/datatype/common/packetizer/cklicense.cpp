/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "hxcom.h"
#include "hxassert.h"
#include "hxcore.h"
#include "hxbrdcst.h"
#include "hxmon.h"
#include "hxerror.h"

#include "cklicense.h"


HX_RESULT
CheckLicense(IUnknown* pContext, 
	     const char* pLicenseName, 
	     const INT32 ulDefault,
	     const char* pStrErrMsg)
{
    HX_ASSERT(pContext && pLicenseName && pStrErrMsg);

    HX_RESULT theErr = HXR_NOT_LICENSED;    
    IHXClientEngine* pPlayer = NULL;
    IHXRemoteBroadcastServices* pRBServices = NULL;

    theErr = pContext->QueryInterface(IID_IHXClientEngine, (void**)&pPlayer);
    if (HXR_OK == theErr)
    {
	pPlayer->Release();
	return HXR_OK;
    }

    theErr = pContext->QueryInterface(IID_IHXRemoteBroadcastServices, (void**)&pRBServices);
    if (HXR_OK == theErr)
    {
	pRBServices->Release();
	return HXR_OK;
    }

    HX_ASSERT(!pPlayer && !pRBServices);
    
    // On the Server, check the license section of the registry
    IHXRegistry* pRegistry = NULL;        
    INT32 nLicensed = 0;
    
    theErr = pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    if (HXR_OK != theErr)
    {
	return HXR_UNEXPECTED;
    }
    
    theErr = pRegistry->GetIntByName(pLicenseName, nLicensed);
    if (HXR_OK != theErr)
    {
	nLicensed = ulDefault;
    }

    if (nLicensed)
    {
	theErr = HXR_OK;
    }
    else
    {    
	theErr = HXR_NOT_LICENSED;
	
	IHXErrorMessages* pErrMsg = NULL;	
	if (HXR_OK == pContext->QueryInterface(IID_IHXErrorMessages,
	    (void **)&pErrMsg))
	{
	    pErrMsg->Report(HXLOG_ALERT, HXR_NOT_LICENSED, 0, pStrErrMsg, NULL);
	}
	HX_RELEASE(pErrMsg);
    }

    
    HX_RELEASE(pRegistry); 
    return theErr;    
}

HX_RESULT
GetConfigDefaultMaxPacketSize(IUnknown* pContext, 
			      const char* pConfigName,  // NULL is ok
			      REF(UINT32)ulMaxPktSize)
{
    HX_ASSERT(pContext);
    HX_RESULT theErr;
    IHXRegistry* pRegistry = NULL;        
    INT32 i = 0;
    
    theErr = pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);
    if (HXR_OK != theErr)
    {
	return HXR_UNEXPECTED;
    }

    theErr = pRegistry->GetIntByName(pConfigName, i);
    if (HXR_OK != theErr)
    {
	// this is the uber default
	theErr = pRegistry->GetIntByName("config.Datatypes.DefaultMaxPacketSize",
					 i);
    }

    ulMaxPktSize = (UINT32)i;
    
    pRegistry->Release();
    return theErr;    
}
