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

#include "hxtypes.h"

#include "hxcom.h"
#include "hxcomm.h"
#include "hxerror.h"
#include "hxstrutl.h"
#include "hxcore.h"
#include "hxassert.h"
#include "hxbuffer.h"

#ifdef __TCS__
#include "hxresult.h"
#endif

#if !defined(HELIX_CONFIG_MINIMIZE_SIZE)
#include "HXErrorCodeStrings.h"
#endif

#include "exerror.h"

#include <stdio.h>
#include "print.h"

#include "globals.h"
struct _stGlobals*& GetGlobal(); //in main.cpp

#ifdef __TCS__
#if defined(__cplusplus)
extern "C" {
#endif

void hookRealAudio_ReportError(int err, long errVal);

#ifdef __cplusplus
}
#endif
#endif

ExampleErrorSink::ExampleErrorSink(IUnknown* pUnknown) 
    : m_lRefCount(0),
      m_pPlayer(NULL)
{
    IHXClientEngine* pEngine = NULL;
    pUnknown->QueryInterface(IID_IHXClientEngine, (void**)&pEngine );
    if( pEngine )
    {
        IUnknown* pTmp = NULL;
        pEngine->GetPlayer(0, pTmp);
        m_pPlayer = (IHXPlayer*)pTmp;
    }
    
    HX_RELEASE( pEngine );
    HX_ASSERT(m_pPlayer);
}

ExampleErrorSink::~ExampleErrorSink()
{
    HX_RELEASE(m_pPlayer);
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::QueryInterface
//  Purpose:
//  Implement this to export the interfaces supported by your 
//  object.
//
STDMETHODIMP ExampleErrorSink::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXErrorSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXErrorSink))
    {
        AddRef();
        *ppvObj = (IHXErrorSink*) this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::AddRef
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32) ExampleErrorSink::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//  IUnknown::Release
//  Purpose:
//  Everyone usually implements this the same... feel free to use
//  this implementation.
//
STDMETHODIMP_(ULONG32) ExampleErrorSink::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXErrorSink methods
 */

STDMETHODIMP 
ExampleErrorSink::ErrorOccurred(const UINT8 unSeverity,  
                                const ULONG32   ulHXCode,
                                const ULONG32   ulUserCode,
                                const char* pUserString,
                                const char* pMoreInfoURL
                                )
{
    char HXDefine[256]; /* Flawfinder: ignore */

    // Store the code, so we can return it from main()
    GetGlobal()->g_Error = ulHXCode;

    ConvertErrorToString(ulHXCode, HXDefine, 256);

#ifdef __TCS__
    hookRealAudio_ReportError(ulHXCode,ulUserCode);
#else
    STDOUT("Report(%d, %ld, \"%s\", %ld, \"%s\", \"%s\")\n",
           unSeverity,
           ulHXCode,
           (pUserString && *pUserString) ? pUserString : "(NULL)",
           ulUserCode,
           (pMoreInfoURL && *pMoreInfoURL) ? pMoreInfoURL : "(NULL)",
           HXDefine);
#endif

    return HXR_OK;
}

void
ExampleErrorSink::ConvertErrorToString(const ULONG32 ulHXCode, char* pszBuffer, UINT32 ulBufLen)
{
    IHXErrorMessages* pErrMsg = NULL;

    if( !pszBuffer)
        return;
    
    pszBuffer[0]='\0';

    HX_ASSERT(m_pPlayer);
    if( m_pPlayer)
    {
        m_pPlayer->QueryInterface(IID_IHXErrorMessages, (void**)&pErrMsg);
        if( pErrMsg )
        {
            IHXBuffer* pMessage = pErrMsg->GetErrorText(ulHXCode);
            if( pMessage )
            {
                SafeStrCpy( pszBuffer, (const char*)pMessage->GetBuffer(), ulBufLen);
                pMessage->Release();
            }
        }
        HX_RELEASE(pErrMsg);
    }
 
    if( strlen(pszBuffer)==0 )
    {
#if !defined(HELIX_CONFIG_MINIMIZE_SIZE)        
	const char* pszHXCodeString = HXErrorCodeToString(ulHXCode);
        if (pszHXCodeString)
        {   
            SafeSprintf( pszBuffer, ulBufLen, "%s", pszHXCodeString);
        }
        else
#endif        
        {
            HX_ASSERT(FALSE);
            SafeSprintf( pszBuffer, ulBufLen, "Can't convert error code %p - make sure it's defined in common/util/HXErrorCodeStrings.c", ulHXCode );
        }
    }
}

