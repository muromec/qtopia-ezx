/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspprotocol.cpp,v 1.81 2007/02/27 06:20:50 gbajaj Exp $
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

#include "hxcom.h"
#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hxver.h"

#include "hxtypes.h"
#include "hxresult.h"
#include "hxstrutl.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxasm.h"
#include "hxsdesc.h"
#include "chxpckts.h"
#include "hxcore.h"
#include "hxprefs.h" 
#include "hxpref.h"
#include "hxsdesc.h"
#include "hxpends.h"
#include "hxrsdbf.h"
#include "hxengin.h"

#include "hxstring.h"
#include "hxslist.h"
#include "hxtick.h"
#include "chxeven.h"

#include "hxcleng.h"
#include "rtspclnt.h"
#include "rtspclntext.h"
#include "rtsputil.h"
#include "mimehead.h"
#include "rtspmsg.h"
#include "rtsppars.h"
#include "rtspmdsc.h"

#include "statinfo.h"
#include "hxntsrc.h"
#include "hxprotocol.h"

#include "hxauthn.h"
#include "hxplgns.h"

#include "hxauth.h"
#include "hxdate.h"
#include "hxurl.h"
#include "platform.h"
#include "clntcore.ver"

#include "hxplugn.h"
#include "dtrvtcon.h"
#include "rmfftype.h"
#include "rtspprotocol.h"
#include "verutil.h"

#include "hxcomsp.h"
#include "hxpktsp.h"
#include "hxplnsp.h"
#include "miscsp.h"
#include "hxxrsmg.h"
#include "hxresmgr.h"
#include "dcoreres.h"
#ifdef _MACINTOSH
#include "hxmm.h"
#endif

#include "pckunpck.h"
#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


#define PRAGMA  "initiate-session"
#define MINIMUM_STATS_INTERVAL      15000 // 15 seconds
#define MAX_DEFAULT_STATS_SIZE      512
#define MAX_TRANSPORT               10

RTSPProtocol::RTSPProtocol(HXNetSource* owner, ULONG32 ulPlatformSpecific)
: HXProtocol (owner, ulPlatformSpecific)
, m_lRefCount(0)
, m_uSecurityKey(0)
, m_uStreamCount(0)
, m_uCurrentStreamCount(0)
, m_pPendingStatus(0)
, m_pStatistics(0)
, m_pRequest(NULL)
, m_bPlaying(FALSE)
, m_bIsASMSource(FALSE)
, m_bUseRTP(FALSE)
, m_bFirstAuthAttempt(TRUE)
, m_bPendingSeek(FALSE)
, m_bHandleWWWAuthentication(FALSE)
, mReceivedControl(FALSE)
, m_bReceivedData(FALSE)
, m_bMulticastOnly(FALSE)
, m_pIDInfo(NULL)
, m_ulSeekPos1(0)
, m_ulSeekPos2(0)
, m_ulLastPacketReceivedTime(0)
, m_ulSessionInitedTime(0)
, m_WWWResult(HXR_OK)
, m_pWWWValues(NULL)
, m_idleState(NULL_STATE)
#if defined(HELIX_FEATURE_REVERTER)
, m_pDataRevert(0)
#endif /* HELIX_FEATURE_REVERTER */
, m_bSocketsInited(FALSE)
{
    m_pStreamInfoList = new CHXMapLongToObj;
    
    if (owner)
    {
        IHXPlayer* pPlayer = NULL;
        IUnknown*   pUnknown = NULL;

        owner->GetPlayer(pPlayer);

        if (pPlayer)
        {
            pUnknown = (IUnknown*)pPlayer;
        }
        // auto. config doesn't have the player object
        // use IHXClientEngine instead
        else
        {
            owner->GetContext(pUnknown);
        }

#if defined(HELIX_FEATURE_REVERTER)
        m_pDataRevert = new DataRevertController(pUnknown);
        m_pDataRevert->AddRef();
        m_pDataRevert->SetControlResponse(this);
#endif /* HELIX_FEATURE_REVERTER */

        HX_RELEASE(pUnknown);
    }

    /* Always allowed in RTSP - till we add some code to file format
     * plugins to set this value in file header - XXXRA
     */
    m_bPerfectPlayAllowed = TRUE;
    //
    // get proxy info for RTSP protocol
    //
    initialize_members();

    ReadPrefBOOL(m_pPreferences, "NonRS", m_bUseRTP);    
    if (!m_bUseRTP)
    {
        ReadPrefBOOL(m_pPreferences, "UseRTP", m_bUseRTP);
    }
}

RTSPProtocol::~RTSPProtocol ()
{
    destroyProtocolLib();

    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pPendingStatus);
    HX_RELEASE(m_pStatistics);
    HX_RELEASE(m_pIDInfo);
    HX_RELEASE(m_pWWWValues);
#if defined(HELIX_FEATURE_REVERTER)
    HX_RELEASE(m_pDataRevert);
#endif /* HELIX_FEATURE_REVERTER */
    
    HX_DELETE(m_pStreamInfoList);

}

/* IUnknown methods */

STDMETHODIMP
RTSPProtocol::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPendingStatus), (IHXPendingStatus*)this },
            { GET_IIDHANDLE(IID_IHXStatistics), (IHXStatistics*)this },
            { GET_IIDHANDLE(IID_IHXBackChannel), (IHXBackChannel*)this },
            { GET_IIDHANDLE(IID_IHXAtomicRuleChange), (IHXAtomicRuleChange*)this },
            { GET_IIDHANDLE(IID_IUnknown), this }
        };
    
    if (HXR_OK == ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj))
    {
        return HXR_OK;
    }    
    else if (IsEqualIID(riid, IID_IHXASMSource) && m_bIsASMSource)
    {
        AddRef();
        *ppvObj = (IHXASMSource*)this;
        return HXR_OK;
    }
    // mOwner(HXNetSource) implements IHXPreferredTransportSink
    else if (IsEqualIID(riid, IID_IHXPreferredTransportSink) && mOwner)
    {
        return mOwner->QueryInterface(riid, ppvObj);
    }
    else if (m_spProtocolLib &&
            (HXR_OK == m_spProtocolLib->QueryInterface(riid, ppvObj)))
    {
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) RTSPProtocol::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) RTSPProtocol::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/* IHXRTSPClientProtocolResponse methods */

STDMETHODIMP
RTSPProtocol::InitDone
(
    HX_RESULT status
)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::InitDone(): status = 0x%08x", this, status);

    if (HXR_OK == status)
    {
        IHXAutoBWDetection* pABD = NULL;
        if (HXR_OK == m_spProtocolLib->QueryInterface(IID_IHXAutoBWDetection, 
                                                     (void**)&pABD))
        {
            mOwner->InitABD(pABD);
        }
        HX_RELEASE(pABD);
    }
    else
    {
        /*
         * XXX...Need to get proper errors from protocol library
         */

        mOwner->ReportError(status); //ConvertHResultToHXError(status));
    }

    if (m_spProtocolLib->HttpOnly())
    {
        mCurrentTransport = TCPMode;
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleWWWAuthentication
(
    HX_RESULT HX_RESULTStatus,
    IHXValues* pIHXValuesHeaders
)
{
#if defined(HELIX_FEATURE_AUTHENTICATION)
#ifdef _MACINTOSH
    /*
     * We load a plugin for authetication. Can't do this at interrupt
     * time on Mac
     */
    if (HXMM_ATINTERRUPT())
    {
        m_bHandleWWWAuthentication  = TRUE;
        m_WWWResult                 = HX_RESULTStatus;

        HX_RELEASE(m_pWWWValues);

        m_pWWWValues                = pIHXValuesHeaders;
        if (m_pWWWValues)
        {
            m_pWWWValues->AddRef();
        }

        if (mOwner)
        {
            mOwner->ScheduleProcessCallback();
        }

        return HXR_OK;
    }
#endif

    return (handlePendingWWWAuthentication(HX_RESULTStatus, pIHXValuesHeaders));
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

#if defined(HELIX_FEATURE_AUTHENTICATION)
// IHXClientAuthResponse
STDMETHODIMP
RTSPProtocol::ResponseReady
(
    HX_RESULT   HX_RESULTStatus,
    IHXRequest* pIHXRequestResponse
)
{
    HX_RESULT HX_RESULTErr;
    IHXValues* pIHXValuesRequestHeaders = NULL;

    if(SUCCEEDED(HX_RESULTStatus))
    {
        if (!m_spProtocolLib)
        {
            return HXR_OUTOFMEMORY;
        }

        pIHXRequestResponse->GetRequestHeaders
        (
            pIHXValuesRequestHeaders
        );

        if(!m_uStreamCount)
        {
            // We haven't received a successful Describe..
            //
            CHXHeader::mergeHeaders
            (
                pIHXValuesRequestHeaders,
                m_spIHXValuesStoredHeaders
            );

            HX_RESULTErr = m_spProtocolLib->SendStreamDescriptionRequest
            (
                mPath,
                pIHXValuesRequestHeaders
            );
        }
        else if (m_uCurrentStreamCount == m_uStreamCount)
        {
            // We haven't received a successful Setup..
            //
            HX_RESULTErr = m_spProtocolLib->SendSetupRequest
            (
                NULL,
                0,
                pIHXValuesRequestHeaders
            );
        }

        HX_RELEASE(pIHXValuesRequestHeaders);
    }
    else
    {
        if (HXR_FAIL == HX_RESULTStatus)
        {
            HX_RESULTStatus = HXR_NOT_AUTHORIZED;
        }
        mOwner->ReportError(HX_RESULTStatus);
    }

    return HXR_OK;
}

HX_RESULT
RTSPProtocol::handlePendingWWWAuthentication
(
    HX_RESULT HX_RESULTStatus,
    IHXValues* pIHXValuesHeaders
)
{
    HX_RESULT HX_RESULTRet = HXR_FAIL;

    if(HXR_NOT_AUTHORIZED == HX_RESULTStatus)
    {
        IUnknown*   pIUnknownContext = NULL;
        IHXPlayer* pIHXPlayerPlayer = NULL;
        IHXAuthenticationManager* pIHXAuthenticationManager = NULL;

        // Don't transport switch while we are waiting for authentication
        mOwner->StopDataWait();

        if
        (
            m_spIHXClientAuthConversationAuthenticator.IsValid()
            &&
            m_spIHXClientAuthConversationAuthenticator->IsDone()
        )
        {
            // Well we tried to authenticate already,
            // so it must have failed

            m_spIHXClientAuthConversationAuthenticator->Authenticated(FALSE);

            // Cleanup so that we can re-auth
            m_spIHXClientAuthConversationAuthenticator.Release();
        }

        mOwner->GetPlayer(pIHXPlayerPlayer);

        if (NULL == pIHXPlayerPlayer)
        {
            // in case of the Auto. Config
            mOwner->GetContext(pIUnknownContext);
        }
        else
        {
            pIUnknownContext = (IUnknown*)pIHXPlayerPlayer;
        }

        if (!m_spIHXClientAuthConversationAuthenticator.IsValid())
        {
            DECLARE_SMART_POINTER_UNKNOWN spIUnknownAuthenticator;
            DECLARE_SMART_POINTER
            (
                IHXObjectConfiguration
            ) spIHXObjectConfigurationAuthenticator;
            DECLARE_SMART_POINTER
            (
                IHXCommonClassFactory
            ) spIHXCommonClassFactoryHXCore;

            spIHXCommonClassFactoryHXCore = pIUnknownContext;

            // Starting conversation
            HX_RESULTRet = spIHXCommonClassFactoryHXCore->CreateInstance
            (
                CLSID_CHXClientAuthenticator,
                (void**)&spIUnknownAuthenticator
            );

            if
            (
                SUCCEEDED(HX_RESULTRet)
                &&
                spIUnknownAuthenticator.IsValid()
            )
            {
                spIHXObjectConfigurationAuthenticator =
                (
                    spIUnknownAuthenticator
                );

                spIHXObjectConfigurationAuthenticator->SetContext
                (
                    pIUnknownContext
                );

                m_spIHXClientAuthConversationAuthenticator =
                (
                    spIUnknownAuthenticator
                );
            }
        }

        if
        (
            m_spIHXClientAuthConversationAuthenticator.IsValid()
            &&
            !m_spIHXClientAuthConversationAuthenticator->IsDone()
        )
        {
            HX_ASSERT(m_pRequest);
            if (m_pRequest)
            {
                m_pRequest->SetResponseHeaders
                (
                    pIHXValuesHeaders
                );

                HX_RESULTRet =
                (
                    m_spIHXClientAuthConversationAuthenticator->MakeResponse
                    (
                        this,
                        m_pRequest
                    )
                );
            }
            else
            {
                // Auth Failed!
                m_spIHXClientAuthConversationAuthenticator->Authenticated(FALSE);
                mOwner->ReportError(HXR_NOT_AUTHORIZED);
            }

            // Flow continues in ResponseReady()
        }
        else
        {
            // Auth Failed!
            if (m_spIHXClientAuthConversationAuthenticator.IsValid())
            {
                m_spIHXClientAuthConversationAuthenticator->Authenticated(FALSE);
            }
            mOwner->ReportError(HXR_NOT_AUTHORIZED);
        }

        HX_RELEASE(pIUnknownContext);

        return HXR_OK;
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::AuthenticationRequestDone(HX_RESULT result,
                                        const char* user,
                                        const char* password)
{
    return HXR_NOTIMPL;
}
#endif /* HELIX_FEATURE_AUTHENTICATION */

// XXXGo - IPTV_HACK
void
RTSPProtocol::hackCookie(IHXBuffer* pCookie)
{
    HX_ASSERT(m_bUseRTP);
    // XXXGo - Interop Hack
    // IP/TV send a spec incomplient cookie...take care of it for now while
    // we get them to fix it.

    // we will assume Set-Cookie has just NAME=VALUE paris.

    IHXBuffer* pBuffer = NULL;
    IHXBuffer* pBuf = NULL;

    CreateAndSetBufferCCF(pBuffer, (BYTE*)pCookie->GetBuffer(),
			  strlen((const char*)pCookie->GetBuffer()) +1, m_pContext);

    char* pcOneCookie = (char*)pBuffer->GetBuffer();
    char* pc = pcOneCookie;
    // go thorough each ';'
    for (;;)
    {
        pc = strchr(pc, ';');
        if (pc)
        {
	    *pc = NULL;
	    if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)pcOneCookie, 
						strlen(pcOneCookie) + 1, m_pContext))
	    {
		mOwner->SetCookie(pBuf);
		HX_RELEASE(pBuf);
	    }
            pcOneCookie = ++pc;
        }
        else
        {
            // just set it
	    if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)pcOneCookie, 
						strlen(pcOneCookie) + 1, m_pContext))
	    {
		mOwner->SetCookie(pBuf);
		HX_RELEASE(pBuf);
	    }
            break;
        }
    }
}

HX_RESULT
RTSPProtocol::SwitchToUnicast(void)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::SwitchToUnicast()", this);
    HX_RESULT   rc = HXR_FAILED;
    IHXBuffer* pBuffer = NULL;

    if (mOwner && mOwner->m_pFileHeader)
    {
        // get a URL from the file header.  No URL, no Unicast!
        if (HXR_OK == mOwner->m_pFileHeader->GetPropertyCString("UnicastURL", pBuffer) &&
            pBuffer)
        {
            rc = HandleRedirectRequest((const char*)pBuffer->GetBuffer(), 0);
        }
        HX_RELEASE(pBuffer);
    }

    return rc;
}

HX_RESULT
RTSPProtocol::InitSockets()
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::InitSockets()", this);

    HX_RESULT           rc = HXR_OK;
    IUnknown*           pContext = NULL;
    IHXInterruptState*  pInterruptState = NULL;

    m_bSocketsInited = FALSE;

    if (UDPMode == mCurrentTransport ||
        MulticastMode == mCurrentTransport)
    {
        mOwner->GetContext(pContext);
        if (pContext)
        {
            pContext->QueryInterface(IID_IHXInterruptState, (void**) &pInterruptState);
        }

        /* We want to initialize sockets ONLY at system time */
        if (!pInterruptState ||
            !pInterruptState->AtInterruptTime())
        {
            rc = m_spProtocolLib->InitSockets();
        }
        // set the state and try to init socket in the next
        // process idle
        else
        {
            m_idleState = SEND_SETUP_REQUEST_STATE;
            rc = HXR_WOULD_BLOCK;
        }
    }

    if (HXR_WOULD_BLOCK != rc)
    {
        m_bSocketsInited = TRUE;
    }

    HX_RELEASE(pInterruptState);
    HX_RELEASE(pContext);

    return rc;
}

STDMETHODIMP
RTSPProtocol::HandleOptionsResponse(HX_RESULT status,
    IHXValues* pHeader)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleOptionsResponse(): status = 0x%08x", this, status);
    if (FAILED(status))
    {
        mOwner->ReportError(status);
        return status;
    }
    else if (HXR_REDIRECTION == status)
    {
        // expect a redirection message soon!
        m_idleState = NULL_STATE;

        HXBOOL bHackCookie = FALSE;
        IHXBuffer* pAgent = NULL;
        if (pHeader->GetPropertyCString("User-Agent", pAgent) == HXR_OK)
        {
            if (strncasecmp((const char*)pAgent->GetBuffer(), "Cisco IP/TV",
                11) == 0)
            {
                bHackCookie = TRUE;
            }
        }
        HX_RELEASE(pAgent);

        // retrieve cookies from the response header
        IHXKeyValueList* pKeyValueList = NULL;

        if (HXR_OK == pHeader->QueryInterface(IID_IHXKeyValueList,
                                        (void**)&pKeyValueList))
        {
            IHXKeyValueListIterOneKey* pListIter = NULL;
            IHXBuffer* pCookie = NULL;

            pKeyValueList->GetIterOneKey("Set-Cookie",pListIter);

            while (pListIter->GetNextString(pCookie) == HXR_OK)
            {
                // XXXGo - IPTV_HACK
                if (bHackCookie)
                {
                    hackCookie(pCookie);
                }
                else
                {
                    mOwner->SetCookie(pCookie);
                }
                HX_RELEASE(pCookie);
            }
            HX_RELEASE(pListIter);
        }
        HX_RELEASE(pKeyValueList);

        return HXR_OK;
    }
    else if (pHeader)
    {
        IHXBuffer* pBuffer = NULL;

        if (HXR_OK == pHeader->GetPropertyCString("Server", pBuffer))
        {
            HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleOptionsResponse(): server = %s", this, pBuffer->GetBuffer());
            ::GetVersionFromString((char*)pBuffer->GetBuffer(), m_ulServerVersion);
        }
        HX_RELEASE(pBuffer);

        if (HXR_OK == pHeader->GetPropertyCString("StatsMask", pBuffer))
        {
            mSendStatsMask = atoi((const char*)pBuffer->GetBuffer());
            mOwner->SetOption(HX_STATS_MASK, &mSendStatsMask);
        }
        HX_RELEASE(pBuffer);

        if (HXR_OK == pHeader->GetPropertyCString("StatsInterval", pBuffer))
        {
            UINT32 ulStatsInterval = 1000* atoi((const char*)pBuffer->GetBuffer());
            // ulStatsInterval = 0 to disable the stats
            if (ulStatsInterval && ulStatsInterval < MINIMUM_STATS_INTERVAL)
            {
                ulStatsInterval = MINIMUM_STATS_INTERVAL;
            }

            mOwner->SetOption(HX_STATS_INTERVAL, &ulStatsInterval);
        }
        HX_RELEASE(pBuffer);
    }

    m_bConnectDone = TRUE;

    HX_RESULT result = HXR_OK;

    result = m_spProtocolLib->SendStreamDescriptionRequest
    (
        mPath,
        m_spIHXValuesStoredHeaders
    );

    return result;
}

HXBOOL 
RTSPProtocol::IsRateAdaptationUsed()
{
    return m_spProtocolLib2->IsRateAdaptationUsed();
}

STDMETHODIMP
RTSPProtocol::HandleStreamDescriptionResponse(HX_RESULT      status,
                                                IHXValues*    pFileHeader,
                                                CHXSimpleList* pHeaderList,
                                                IHXValues*    pResponseHeaders)
{

    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleStreamDescriptionResponse(): status = 0x%08x", this, status);
    if (FAILED(status))
    {
        if (!pFileHeader || !pHeaderList)
        {
            /*
             * XXX...Need to get proper errors from protocol library
             */

            mOwner->ReportError(status);//ConvertHResultToHXError(status));

            return (status != (UINT32)HXR_OK) ? status : (UINT32)HXR_FAIL;
        }
    }
    else if (!pFileHeader || !pHeaderList)
    {
        // Not a severe failure, need to try again.
        return m_spProtocolLib->SendStreamDescriptionRequest(mPath,
                                                m_spIHXValuesStoredHeaders);
    }

#if defined(HELIX_FEATURE_REVERTER)
    // DESCRIBE Succeeded!
    //
    m_pDataRevert->RevertHeaders(pFileHeader, pHeaderList, pResponseHeaders);
#else
    RevertHeadersDone(pFileHeader,
                      pHeaderList,
                      pResponseHeaders,
                      FALSE);
#endif /* HELIX_FEATURE_REVERTER */

    return HXR_OK;
}

void
RTSPProtocol::RevertHeadersDone(IHXValues* pFileHeader,
                                CHXSimpleList* pHeaderList,
                                IHXValues* pResponseHeaders,
                                HXBOOL bUseReverter)
{
    ULONG32 tempLiveStream = 0;
    pFileHeader->GetPropertyULONG32("LiveStream", tempLiveStream);
    mLiveStream = (tempLiveStream > 0)?TRUE:FALSE;

#if defined(HELIX_FEATURE_REVERTER)
    if (bUseReverter && m_pDataRevert)
    {
        m_spProtocolLib->InitPacketFilter(m_pDataRevert);
    }
#endif /* HELIX_FEATURE_REVERTER */

    ULONG32 ulFlags = 0;
    pFileHeader->GetPropertyULONG32("Flags", ulFlags);
    mSaveAsAllowed = ulFlags & HX_SAVE_ENABLED ? TRUE : FALSE;

    IHXBuffer* pBuffer = NULL;
    if (HXR_OK == pResponseHeaders->GetPropertyCString("MaxBandwidth", pBuffer))
    {
        UINT32 ulMaxBandwidth = atoi((const char*)pBuffer->GetBuffer());
        // ulMaxBandwidth = 0 to disable the faststart
        mOwner->SetOption(HX_MAX_BANDWIDTH, &ulMaxBandwidth);
    }
    HX_RELEASE(pBuffer);

    if (HXR_OK == pResponseHeaders->GetPropertyCString("TurboPlay", pBuffer))
    {
        HXBOOL bTurboPlay = (HXBOOL)atoi((const char*)pBuffer->GetBuffer());
        mOwner->SetOption(HX_TURBO_PLAY, &bTurboPlay);
    }
    HX_RELEASE(pBuffer);

    // UseRTP was set from RTSPClientProtocol::handleDescribeResponse when
    // it detected we were not playing a stream from a RealServer.
    ULONG32 ulTemp = 0;
    if (HXR_OK == pResponseHeaders->GetPropertyULONG32("UseRTP", ulTemp))
    {
        m_bUseRTP = ulTemp;
    }

    // retrieve cookies from the response header
    IHXKeyValueList* pKeyValueList = NULL;

    if (HXR_OK == pResponseHeaders->QueryInterface(IID_IHXKeyValueList,
                                                    (void**)&pKeyValueList))
    {
        IHXKeyValueListIterOneKey* pListIter = NULL;
        IHXBuffer* pCookie = NULL;

        pKeyValueList->GetIterOneKey("Set-Cookie",pListIter);

        while (pListIter->GetNextString(pCookie) == HXR_OK)
        {
            mOwner->SetCookie(pCookie);
            HX_RELEASE(pCookie);
        }
        HX_RELEASE(pListIter);
    }
    HX_RELEASE(pKeyValueList);

    m_pRequest->SetResponseHeaders(pResponseHeaders);

    // notify the source file header is ready
    mOwner->FileHeaderReady(pFileHeader);

    UINT32 ulNumHeaders = (UINT32) pHeaderList->GetCount();

    HX_ASSERT(m_pStreamInfoList->IsEmpty() == TRUE);
    if (m_pStreamInfoList->IsEmpty() && ulNumHeaders > 0 &&
        ulNumHeaders < m_pStreamInfoList->GetHashTableSize())
    {
        m_pStreamInfoList->InitHashTable(ulNumHeaders);
    }

    CHXSimpleList::Iterator i;

    for (i = pHeaderList->Begin(); i != pHeaderList->End(); ++i)
    {
        IHXValues* pHeader = (IHXValues*)(*i);
        mOwner->HeaderCallback(pHeader);

        if(!m_bIsASMSource)
        {
            // see if there's an ASMRuleBook in the stream header and set
            //flag for QI. only need to get one rulebook to make this TRUE
            IHXBuffer* pRuleBook = NULL;
            pHeader->GetPropertyCString("ASMRuleBook", pRuleBook);
            if(pRuleBook)
            {
                m_bIsASMSource = TRUE;
                pRuleBook->Release();
            }
        }

        /*
         * Need to maintain status for the streams
         */

        UINT32 ulStreamNumber = 0;
        UINT32 ulClipBandwidth = 0;
        IHXBuffer* pMimeType = NULL;

        HX_RESULT result = pHeader->GetPropertyULONG32("StreamNumber", ulStreamNumber);
        if (result != HXR_OK)
        {
            return;
        }

        pHeader->GetPropertyCString("MimeType", pMimeType);
        pHeader->GetPropertyULONG32("AvgBitRate", ulClipBandwidth);

        HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::RevertHeadersDone(): creating stream info for stream %u", this, ulStreamNumber);

        RTSP_STREAM_INFO* pStreamInfo = new RTSP_STREAM_INFO;
        pStreamInfo->m_uStreamNumber = (UINT16)ulStreamNumber;
        pStreamInfo->m_ulClipBandwidth = ulClipBandwidth;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        pStreamInfo->m_pStreamStats = create_statistics((UINT16)ulStreamNumber);
        if (pStreamInfo->m_pStreamStats)
        {
            pStreamInfo->m_pStreamStats->m_pClipBandwidth->SetInt(0);
            if (pMimeType)
            {
                pStreamInfo->m_pStreamStats->m_pMimeType->SetStr((char*)pMimeType->GetBuffer());
            }
        }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

        HX_RELEASE(pMimeType);

        (*m_pStreamInfoList)[ulStreamNumber] = pStreamInfo;
        m_uStreamCount++;
    }

    mReceivedControl        = TRUE;
    m_uCurrentStreamCount   = m_uStreamCount;

    // try to init the UDP socket
    HX_RESULT status = InitSockets();
    if (HXR_OK == status)
    {        
        if (!mOwner->m_bContinueWithHeaders)
        {
            status = send_setup_request();
            m_idleState = NULL_STATE;
        }
        else
        {
            m_idleState = SEND_SETUP_REQUEST_STATE;
        }
    }

    // remember the error and report it later
    if (HXR_OK != status && HXR_WOULD_BLOCK != status)
    {
        m_LastError = status;
    }
}

void
RTSPProtocol::SendControlBuffer(IHXBuffer* pBuffer)
{
    char* p64Buf = new char[pBuffer->GetSize() * 2 + 4];
    BinTo64((const unsigned char*)pBuffer->GetBuffer(),
            pBuffer->GetSize(), p64Buf);
    m_spProtocolLib->SendSetParameterRequest("DataConvertBuffer", "1",
            "base64", p64Buf);
    delete[] p64Buf;
}

UINT16
RTSPProtocol::GetRDTFeatureLevel(void)
{
#if defined(HELIX_FEATURE_RDT)
    if (m_spProtocolLib2)
    {
        return m_spProtocolLib2->GetRDTFeatureLevel();
    }
    else
#endif /* HELIX_FEATURE_RDT */
    {
        return 0;
    }
}

void
RTSPProtocol::LeavePrefetch(void)
{
    m_bPrefetch = FALSE;

    if (m_spProtocolLib2)
    {
        m_spProtocolLib2->LeavePrefetch();
    }

    return;
}

void
RTSPProtocol::EnterFastStart(void)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::EnterFastStart()", this);
    m_bFastStart = TRUE;

    if (m_spProtocolLib2)
    {
        m_spProtocolLib2->EnterFastStart();
    }

    return;
}

void
RTSPProtocol::LeaveFastStart(void)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::LeaveFastStart()", this);
    m_bFastStart = FALSE;

    if (m_spProtocolLib2)
    {
        m_spProtocolLib2->LeaveFastStart();
    }

    return;
}

/*
 *  If this is multicast, make sure to send subscribe msg
 */
HX_RESULT
RTSPProtocol::handle_multicast(void)
{
#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    HX_ASSERT(mCurrentTransport == MulticastMode);

    STREAM_INFO* pStreamInfo = NULL;
    IHXBuffer* pRuleBook = NULL;

    for (UINT16 uStrmNum = 0; uStrmNum < m_uStreamCount; uStrmNum++)
    {
        pStreamInfo = NULL;

        if (FAILED(mOwner->GetStreamInfo(uStrmNum, pStreamInfo)))
        {
            return HXR_OK;
        }
        HX_ASSERT(pStreamInfo);

        // If there is an ASMRuleBook, subscription will be done eventually
        // through RTSPClientProtocol::RuleChange()...
        pRuleBook = NULL;
        if (FAILED(pStreamInfo->m_pHeader->GetPropertyCString("ASMRuleBook", pRuleBook)))
        {
            HX_ASSERT(!pRuleBook);
            Subscribe(uStrmNum, 0);
        }

        HX_RELEASE(pRuleBook);
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
}

HX_RESULT
RTSPProtocol::send_setup_request()
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::send_setup_request()", this);
    HX_RESULT   rc = HXR_OK;

    RTSPTransportType* pTransport = new RTSPTransportType[MAX_TRANSPORT];
    int nTransports = 0;

    switch (mCurrentTransport)
    {
        /*
         * Port is omitted here, the transport layer will pick it.
         */
#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
        case MulticastMode:
            if (!m_bUseRTP)
            {
                pTransport[nTransports].m_lTransportType = RTSP_TR_RDT_MCAST;
                pTransport[nTransports++].m_sPort = 0;
                pTransport[nTransports].m_lTransportType = RTSP_TR_RDT_UDP;
                pTransport[nTransports++].m_sPort = 0;
                pTransport[nTransports].m_lTransportType = RTSP_TR_TNG_UDP;
                pTransport[nTransports++].m_sPort = 0;
                pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_UDP;
                pTransport[nTransports++].m_sPort = 0;
                if (m_ulTransportPrefMask & ATTEMPT_TCP)
                {
                    pTransport[nTransports].m_lTransportType = RTSP_TR_TNG_TCP;
                    pTransport[nTransports++].m_sPort = 0;
                    pTransport[nTransports].m_lTransportType = RTSP_TR_RDT_TCP;
                    pTransport[nTransports++].m_sPort = 0;
                    pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_TCP;
                    pTransport[nTransports++].m_sPort = 0;
                }
            }
            else
            {
                pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_UDP;
                pTransport[nTransports++].m_sPort = 0;
                if (m_ulTransportPrefMask & ATTEMPT_TCP)
                {
                    pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_TCP;
                    pTransport[nTransports++].m_sPort = 0;
                }
            }

            // tell the owner that we are using multicast...
            // this is needed to display it on the stats dialog...
            mOwner->TransportStarted(MulticastMode);
            break;
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
        case UDPMode:
            if(!m_bUseRTP)
            {
                pTransport[nTransports].m_lTransportType = RTSP_TR_RDT_UDP;
                pTransport[nTransports++].m_sPort = 0;
                pTransport[nTransports].m_lTransportType = RTSP_TR_TNG_UDP;
                pTransport[nTransports++].m_sPort = 0;
                pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_UDP;
                pTransport[nTransports++].m_sPort = 0;
                if (m_ulTransportPrefMask & ATTEMPT_TCP)
                {
                    pTransport[nTransports].m_lTransportType = RTSP_TR_TNG_TCP;
                    pTransport[nTransports++].m_sPort = 0;
                    pTransport[nTransports].m_lTransportType = RTSP_TR_RDT_TCP;
                    pTransport[nTransports++].m_sPort = 0;
                    pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_TCP;
                    pTransport[nTransports++].m_sPort = 0;
                }
            }
            else
            {
                pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_UDP;
                pTransport[nTransports++].m_sPort = 0;
                if (m_ulTransportPrefMask & ATTEMPT_TCP)
                {
                    pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_TCP;
                    pTransport[nTransports++].m_sPort = 0;
                }
            }

            // tell the owner that we are using UDP...
            // this is needed to display it on the stats dialog...
            mOwner->TransportStarted(UDPMode);
            break;

        case TCPMode:
            if(!m_bUseRTP)
            {
                pTransport[nTransports].m_lTransportType = RTSP_TR_TNG_TCP;
                pTransport[nTransports++].m_sPort = 0;
                pTransport[nTransports].m_lTransportType = RTSP_TR_RDT_TCP;
                pTransport[nTransports++].m_sPort = 0;
                pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_TCP;
                pTransport[nTransports++].m_sPort = 0;
            }
            else
            {
                pTransport[nTransports].m_lTransportType = RTSP_TR_RTP_TCP;
                pTransport[nTransports++].m_sPort = 0;
            }

            // tell the owner that we are using TCPt...
            // this is needed to display it on the stats dialog...
            if (m_bHTTPOnly)
            {
                mOwner->TransportStarted(HTTPCloakMode);
            }
            else
            {
                mOwner->TransportStarted(TCPMode);
            }
            break;

        default:
	    HXLOGL1(HXLOG_RTSP, "RTSPProtocol[%p]::send_setup_request(): invalid transport %d", this, mCurrentTransport);
            HX_VECTOR_DELETE(pTransport);            
            return HXR_FAIL;
    }

    HX_ASSERT(nTransports <= MAX_TRANSPORT);

    // look for a cookie...
    // If we use pRequsetHeaders straight from m_pRequest, it will contain a
    // whole bunch of stuff that we don't need.  SO, just extract the "Cookie"
    // and create a new header right here.  If you need other headers, we might
    // need to come up with a better loginc.
    IHXValues* pRequestHeaders = NULL;
    IHXBuffer* pCookie = NULL;
    if (SUCCEEDED(m_pRequest->GetRequestHeaders(pRequestHeaders)))
    {
        pRequestHeaders->GetPropertyCString("Cookie", pCookie);
        HX_RELEASE(pRequestHeaders);
        if (pCookie)
        {
	    CreateValuesCCF(pRequestHeaders, m_pContext);
            pRequestHeaders->SetPropertyCString("Cookie", pCookie);
            HX_RELEASE(pCookie);
        }
    }

    rc = m_spProtocolLib->SendSetupRequest(pTransport, nTransports, pRequestHeaders);
    HX_RELEASE(pRequestHeaders);

    HX_VECTOR_DELETE(pTransport);

    return rc;
}

STDMETHODIMP
RTSPProtocol::HandleStreamRecordDescriptionResponse
(
    HX_RESULT   status,
    IHXValues* pResponseHeaders
)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleSetupResponse
(
    HX_RESULT status
)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleSetupResponse(): status = 0x%08x", this, status);
    // HXR_WOULD_BLOCK is not an error condition, it indicates the
    // RTSP messages needs to be pipelined
    if (status != HXR_OK && status != HXR_WOULD_BLOCK)
    {
        /*
         * XXX...Need to get proper errors from protocol library
         */

        mOwner->ReportError(status);

        return status;
    }

    // some operations need to wait till status == HXR_OK which means
    // all the streams have been setup successfully
    if (status == HXR_OK)
    {
        switch(m_spProtocolLib->GetProtocolType())
        {
            case 1:
                HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleSetupResponse(): mode = MULTICAST", this);
                mOwner->TransportStarted(MulticastMode);
#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
                // OK, this is multicast for sure...Make sure to send subscription.
                handle_multicast();
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
                break;
            case 2:
                HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleSetupResponse(): mode = UDP", this);
                mOwner->TransportStarted(UDPMode);
                break;
            case 3:
                HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleSetupResponse(): mode = TCP", this);
                // notify that the server has selected TCP over UDP
                // when the player supports both
                if (!m_bHTTPOnly                        &&
                    mCurrentTransport != TCPMode        &&
                    HXR_OK == mOwner->TransportStarted(TCPMode))
                {
                    mOwner->TransportSucceeded(TCPMode, 0);
                }
                /* No need to report TCP success
                 * We may be in TCP/HTTP Cloaking mode
                 */
                break;
            default:
                HX_ASSERT(FALSE);
                break;
        }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        /*
         * Must wait until transport is established before initializing
         * statistics
         */
        CHXMapLongToObj::Iterator i;
        for (i = m_pStreamInfoList->Begin(); i != m_pStreamInfoList->End(); ++i)
        {
            RTSP_STREAM_INFO* pStreamInfo = (RTSP_STREAM_INFO*)(*i);

            if (m_spProtocolLib2)
            {
                m_spProtocolLib2->SetStatistics(pStreamInfo->m_uStreamNumber,
                                                pStreamInfo->m_pStreamStats);
            }
        }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
    }

    // mOwner prevents itself from being initialized multiple times
    mOwner->Initialize();

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandlePlayResponse
(
    HX_RESULT status
)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandlePlayResponse(): status = 0x%08x", this, status);
    if (status != HXR_OK)
    {
        /*
         * XXX...Need to get proper errors from protocol library
         */

        mOwner->ReportError(status); //ConvertHResultToHXError(status));

        return status;
    }

    /* Mark the current state as Playing ONLY if we have called resume */
    if (!m_bIsFirstResume)
    {
        m_bPlaying = TRUE;
        // Start Waiting for data..
        mOwner->StartDataWait();
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleRecordResponse
(
    HX_RESULT status
)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleTeardownResponse
(
    HX_RESULT status
)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleTeardownResponse(): status = 0x%08x (destroying proto lib)", this, status);
    AddRef();
    destroyProtocolLib();
    Release();

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleSetParameterRequest
(
    UINT32 lParamType,
    const char* pParamName,
    IHXBuffer* pParamValue
)
{
    IHXPlayer* pPlayer;
    IHXBandwidthManager * pBWM;
    if(!strcmp(pParamName,"MaximumASMBandwidth") &&
       (HXR_OK == mOwner->GetPlayer(pPlayer)) &&
       (HXR_OK == pPlayer->QueryInterface(IID_IHXBandwidthManager,
                                                  (void **)&pBWM)))
    {
        pBWM->ChangeBW((UINT32)atoi((const char *)pParamValue->GetBuffer()),
                mOwner);
        return HXR_OK;
    }
#if defined(HELIX_FEATURE_REVERTER)
    else if (!strcmp(pParamName, "DataConvertBuffer"))
    {
        m_pDataRevert->ControlBufferReady(pParamValue);
        return HXR_OK;
    }
#endif /* HELIX_FEATURE_REVERTER */
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleSetParameterRequest(const char* pParamName,
        const char* pParamValue, const char* pContent)
{
#if defined(HELIX_FEATURE_REVERTER)
    if (!strcmp(pParamName, "DataConvertBuffer"))
    {
        IHXBuffer* pBuffer = NULL;
	if (HXR_OK == CreateBufferCCF(pBuffer, m_pContext))
	{
	    int contentLen = strlen(pContent);
	    pBuffer->SetSize(contentLen);
	    int offset = BinFrom64(pContent, contentLen,
				  (unsigned char*)pBuffer->GetBuffer());
	    pBuffer->SetSize(offset);
	    m_pDataRevert->ControlBufferReady(pBuffer);
	    HX_RELEASE(pBuffer);
	    return HXR_OK;
	}
	else
	{
	    return HXR_FAILED;
	}
    }
#endif /* HELIX_FEATURE_REVERTER */

    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleSetParameterResponse
(
    HX_RESULT status
)
{
    return HXR_OK; // XXXSMP Just return OK for now.
}


STDMETHODIMP
RTSPProtocol::HandleSetParameterResponseWithValues
(
    HX_RESULT status,
    IHXValues* pValues
)
{
    if (status == HXR_OK && pValues)
    {
        UINT32 ulStatsInterval = 0;
        UINT32 ulValue = 0;
        if (pValues->GetPropertyULONG32("UpdateStatsInterval", ulStatsInterval) == HXR_OK)
        {
            ulStatsInterval = ulStatsInterval * 1000;
            // ulStatsInterval = 0 to disable the stats
            if (ulStatsInterval && ulStatsInterval < MINIMUM_STATS_INTERVAL)
            {
                ulStatsInterval = MINIMUM_STATS_INTERVAL;
            }

            mOwner->SetOption(HX_STATS_INTERVAL, &ulStatsInterval);
        }

        if (pValues->GetPropertyULONG32("Reconnect", ulValue) == HXR_OK)
        {
            mOwner->SetReconnectInfo(pValues);
        }
    }

    return HandleSetParameterResponse(status);
}

STDMETHODIMP
RTSPProtocol::HandleGetParameterRequest
(
    UINT32 lParamType,
    const char* pParamName,
    IHXBuffer** pParamValue
)
{
    /*
     * XXX...Need to implement
     */

    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleGetParameterResponse
(
    HX_RESULT status,
    IHXBuffer* pParamValue
)
{
    /*
     * XXX...Need to implement
     */

    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleAlertRequest
(
    HX_RESULT status,
    INT32 lAlertNumber,
    const char* pAlertText
)
{
    HX_RESULT   theErr = HXR_OK;

    m_idleState = ALERT_STATE;
    m_ulLastAlert = (UINT32)lAlertNumber;

    HX_VECTOR_DELETE(m_pTextBuf);

    if (pAlertText)
    {
        m_pTextBuf = new char[::strlen(pAlertText) + 1];

        if(!m_pTextBuf)
        {
            theErr = HXR_OUTOFMEMORY;
            goto cleanup;
        }

        strcpy(m_pTextBuf, pAlertText); /* Flawfinder: ignore */
    }

cleanup:

    // Clear the authentication cache
    if (m_pRegistry)
    {
        m_pRegistry->DeleteByName("CredCache");
    }

    return theErr;
}

STDMETHODIMP
RTSPProtocol::HandleUseProxyRequest
(
    const char* pProxyURL
)
{
    HX_RESULT rc = HXR_OK;

    if(!pProxyURL)
    {
        mOwner->ReportError(HXR_DNR);
    }
    else
    {
        char* pProxyHost = NULL;
        UINT32 ulProxyPort = 0;

        // parse host name, port
        IUnknown* pContext = NULL;
        mOwner->GetContext(pContext);
        CHXURL proxyURL(pProxyURL, pContext);

        HX_RELEASE(pContext);

        IHXValues* pProxyURLProps = proxyURL.GetProperties();

        IHXBuffer* pBuffer = NULL;
        if(HXR_OK == pProxyURLProps->GetPropertyBuffer(PROPERTY_HOST, pBuffer))
        {
            pProxyHost = new char[pBuffer->GetSize()+1];
            strcpy(pProxyHost, (char*)pBuffer->GetBuffer()); /* Flawfinder: ignore */
            HX_RELEASE(pBuffer);
        }
        pProxyURLProps->GetPropertyULONG32(PROPERTY_PORT, ulProxyPort);
        HX_RELEASE(pProxyURLProps);

        if(pProxyHost)
        {
            initialize_members();

            set_proxy(pProxyHost, (UINT16)ulProxyPort);

            // copy host/path so setup doesn't override
            char* pHost = new_string(mHost);
            char* pPath = new_string(mPath);
            rc = setup(pHost, pPath, mServerPort, mLossCorrection,
                       m_bHTTPOnly, m_bSDPInitiated, mCloakPort);
            delete[] pHost;
            delete[] pPath;
        }
        delete[] pProxyHost;
    }

    return rc;
}

STDMETHODIMP
RTSPProtocol::HandleRedirectRequest
(
    const char* pURL,
    UINT32 msFromNow
)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleRedirectRequest(): url = %s", this, pURL);

    HX_RESULT rc = HXR_FAIL;
    if(pURL)
    {
        IUnknown* pContext = NULL;
        mOwner->GetContext(pContext);
        CHXURL urlObj(pURL, pContext);
        HX_RELEASE(pContext);

        IHXValues* pHeader = urlObj.GetProperties();

        IHXBuffer* pHost = NULL;
        rc = pHeader->GetPropertyBuffer(PROPERTY_HOST, pHost);
        if (HXR_OK == rc)
        {
            IHXBuffer* pResource = NULL;
            rc = pHeader->GetPropertyBuffer(PROPERTY_RESOURCE, pResource);
            if (HXR_OK == rc)
            {
                UINT32 ulPort = 0;
                pHeader->GetPropertyULONG32(PROPERTY_PORT, ulPort);

                mOwner->SetRedirectURL((const char*)pHost->GetBuffer(), (UINT16)ulPort, (const char*)pResource->GetBuffer(), &urlObj);
                m_LastError = HXR_REDIRECTION;

                HX_RELEASE(pResource);
            }
            HX_RELEASE(pHost);
        }
        HX_RELEASE(pHeader);
    }
    
    if (FAILED(rc))
    {
        // HXR_DNR ???
        mOwner->ReportError(HXR_DNR);
    }

    return rc;
}

STDMETHODIMP
RTSPProtocol::HandlePacket
(
    HX_RESULT   status,
    const char* pSessionID,
    IHXPacket* pPacket
)
{
    if (HXR_OK == status)
    {
        /*
         * These are flushed pre-seek packets
         */
        if (pPacket)
        {
            CHXEvent* pEvent = new CHXEvent(pPacket);
            pEvent->SetPreSeekEvent();
            mOwner->EventReady(pEvent);
        }
        // HandlePacket is called with pPacket=NULL to indicate
        // the liveness of TCP/UDP data connection in order to
        // detect the server timeout
        else
        {
            m_ulLastPacketReceivedTime = HX_GET_TICKCOUNT();
        }
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleProtocolError
(
    HX_RESULT status
)
{
    /*
     * XXX...Need to get proper errors from protocol library
     */

    mOwner->ReportError(status);//ConvertHResultToHXError(status));

    m_bPlaying = FALSE;

    return status;
}

STDMETHODIMP
RTSPProtocol::HandleRTTResponse
(
    HX_RESULT status,
    const char* pSessionID,
    UINT32    ulSecs,
    UINT32    ulUSecs
)
{
    // we handle RTT response in order to detect
    // whether the UDP channel is still alive during PAUSE/RESUME
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleCongestion
(
    HX_RESULT status,
    const char* pSessionID,
    INT32     xmitMultiplier,
    INT32     recvMultiplier
)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPProtocol::HandleSourceDone(void)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleSourceDone()", this);
    mSourceEnd = TRUE;
    if (mOwner)
    {
        mOwner->SetEndOfClip();
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandlePrerollChange(THIS_ 
				  RTSPPrerollTypeEnum prerollType,
				  UINT32 ulPreroll)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandlePrerollChange()", this);
    if (RTSP_PREROLL_PREDECBUFPERIOD == prerollType)
    {
	// Handle 3GPP 26.234 Annex G x-initpredecbufperiod field
	// This field only updates the preroll for video streams
        // x-initpredecbufperiod is given in resolution of 90KHz and needs to be converted to milliseconds
        ulPreroll = (ulPreroll / 90);

	// Look through all the streams
	for (UINT16 uStrmNum = 0; uStrmNum < m_uStreamCount; uStrmNum++)
	{
	    STREAM_INFO* pStreamInfo = NULL;
	    IHXBuffer* pMimeType = NULL;

	    // Look for video streams
	    if ((HXR_OK == mOwner->GetStreamInfo(uStrmNum, pStreamInfo)) &&
		(pStreamInfo->m_pHeader) &&
		(HXR_OK == pStreamInfo->m_pHeader->GetPropertyCString("Mimetype",
								      pMimeType)) &&
		(!strncasecmp("video/", (char*)pMimeType->GetBuffer(), 6)))
	    {
		// Update the preroll value
		pStreamInfo->UpdatePreroll(ulPreroll);
	    }

	    HX_RELEASE(pMimeType);
	}
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::HandleStreamDone
(
    HX_RESULT status,
    UINT16    uStreamNumber
)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleStreamDone(): status = %08x; stream = %u", this, status, uStreamNumber);
    STREAM_INFO* pStreamInfo = NULL;

    if (HXR_OK != mOwner->GetStreamInfo(uStreamNumber, pStreamInfo))
    {
        return HXR_FAIL;
    }

    HX_ASSERT(!pStreamInfo->m_bSrcStreamDone);

    if (!pStreamInfo->m_bSrcStreamDone)
    {
        mOwner->HandleStreamDone(status, uStreamNumber);
        pStreamInfo->m_streamEndReasonCode = status;
        pStreamInfo->m_bSrcStreamDone = TRUE;

        m_uCurrentStreamCount--;

        HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::HandleStreamDone(): current stream count now %u",this,  m_uCurrentStreamCount);
        if (0 == m_uCurrentStreamCount)
        {
            mOwner->SetEndOfClip();
        }
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPProtocol::GetStatus
(
    REF(UINT16) uStatusCode,
    REF(IHXBuffer*) pStatusDesc,
    REF(UINT16) ulPercentDone
)
{
// XXX HP
// we no longer care about the buffering at the transport layer
// playback starts as soon as we get enough data at BufferManager
#if 0
    /*
     * XXX...This needs to be properly implemented
     */

    IHXBuffer*          pStatus = NULL;
    IUnknown*           pContext = NULL;
    IHXClientEngine*    pEngine = NULL;

    uStatusCode = HX_STATUS_READY;
    ulPercentDone = 100;
    pStatusDesc = NULL;

    if (!m_bConnectDone)
    {
        uStatusCode     = HX_STATUS_CONTACTING;
        ulPercentDone   = 0;

        mOwner->GetContext(pContext);

#if defined(HELIX_FEATURE_RESOURCEMGR)
        if (pContext &&
            HXR_OK == pContext->QueryInterface(IID_IHXClientEngine, (void**)&pEngine))
        {
            pStatus = ((HXClientEngine *)pEngine)->GetResMgr()->GetMiscString(IDS_STATUS_CONTACTING);
        }

	CreateBufferCCF(pStatusDesc, m_pContext);
        if (!pStatusDesc)
        {
            return HXR_OUTOFMEMORY;
        }

        CHXString statusDesc = "";
        if (pStatus)
        {
            statusDesc += (const char*) pStatus->GetBuffer();
            statusDesc += " ";
        }

        statusDesc += mHost;
        statusDesc += "...";
        pStatusDesc->Set((UCHAR*)(const char*) statusDesc,
                         strlen((const char*)statusDesc)+1);

        HX_RELEASE(pStatus);
        HX_RELEASE(pEngine);
#endif /* HELIX_FEATURE_RESOURCEMGR */

        HX_RELEASE(pContext);

        return HXR_OK;
    }
    else if (m_pPendingStatus)
    {
        return m_pPendingStatus->GetStatus(uStatusCode,
                                           pStatusDesc,
                                           ulPercentDone);
    }
    else
    {
        uStatusCode = HX_STATUS_BUFFERING;
        ulPercentDone = 0;
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif
}

#if defined(HELIX_FEATURE_STATS)
/************************************************************************
 *      Method:
 *          IHXStatistics::InitializeStatistics
 *      Purpose:
 *          Pass registry ID to the caller
 */
STDMETHODIMP
RTSPProtocol::InitializeStatistics
(
    UINT32      /*IN*/ ulRegistryID
)
{
    m_ulRegistryID = ulRegistryID;

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXStatistics::UpdateStatistics
 *      Purpose:
 *          Notify the user to updates its statistics registry
 */
STDMETHODIMP
RTSPProtocol::UpdateStatistics()
{
    if (!m_pStatistics)
    {
        return HXR_FAIL;
    }

    return m_pStatistics->UpdateStatistics();
}
#endif /* HELIX_FEATURE_STATS */

HX_RESULT
RTSPProtocol::GetStreamStatistics
(
    ULONG32 ulStreamNumber,
    STREAM_STATS** ppStreamStats
)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    RTSP_STREAM_INFO* pStreamInfo;

    if (!m_pStreamInfoList->Lookup(ulStreamNumber, (void*&)pStreamInfo))
    {
        *ppStreamStats = NULL;
        return HXR_FAIL;
    }

    *ppStreamStats = pStreamInfo->m_pStreamStats;

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

HX_RESULT
RTSPProtocol::UpdateRegistry(UINT32 ulStreamNumber,
                             UINT32 ulRegistryID)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    HX_RESULT result = HXR_OK;

    if (!m_pRegistry)
    {
        return HXR_FAIL;
    }

    CHXMapLongToObj::Iterator i;
    for (i = m_pStreamInfoList->Begin(); i != m_pStreamInfoList->End(); ++i)
    {
        RTSP_STREAM_INFO* pStreamInfo = (RTSP_STREAM_INFO*)(*i);

        if (pStreamInfo->m_uStreamNumber == (UINT16)ulStreamNumber)
        {
            STREAM_STATS* pTmpStreamStats = new STREAM_STATS(m_pContext, ulRegistryID);
            *pTmpStreamStats = *pStreamInfo->m_pStreamStats;

            HX_DELETE(pStreamInfo->m_pStreamStats);
            pStreamInfo->m_pStreamStats = pTmpStreamStats;

            if (m_spProtocolLib2)
            {
                m_spProtocolLib2->SetStatistics(pStreamInfo->m_uStreamNumber,
                                                pStreamInfo->m_pStreamStats);
            }
            break;
        }
    }

    return result;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

STDMETHODIMP
RTSPProtocol::RuleChange(REF(CHXSimpleList) pList)
{
    return m_spProtocolLib->RuleChange(&pList);
}

/*
 * IHXASMSource methods
 */

/************************************************************************
 *      Method:
 *          IHXASMSource::Subscribe
 *      Purpose:
 *          Subscribe to a stream
 */

STDMETHODIMP
RTSPProtocol::Subscribe
(
    UINT16 streamNumber,
    UINT16 ruleNumber
)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::Subscribe(): stream %u -> rule %u", this, streamNumber, ruleNumber);

    RTSPSubscription sub;
    sub.m_streamNumber = streamNumber;
    sub.m_ruleNumber = ruleNumber;
    sub.m_bIsSubscribe = TRUE;
    CHXSimpleList subList;
    subList.AddTail(&sub);
    return m_spProtocolLib->Subscribe(&subList);
}

/************************************************************************
 *      Method:
 *          IHXASMSource::Unsubscribe
 *      Purpose:
 *          Unsubscribe from a stream
 */

STDMETHODIMP
RTSPProtocol::Unsubscribe
(
    UINT16 streamNumber,
    UINT16 ruleNumber
)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::Unsubscribe(): stream %u -> rule %u", this, streamNumber, ruleNumber);
    RTSPSubscription sub;
    sub.m_streamNumber = streamNumber;
    sub.m_ruleNumber = ruleNumber;
    sub.m_bIsSubscribe = FALSE;
    CHXSimpleList subList;
    subList.AddTail(&sub);
    return m_spProtocolLib->Unsubscribe(&subList);
}

/*
 * IHXBackChannel methods
 */

/************************************************************************
 *      Method:
 *          IHXBackChannel::PacketReady
 *      Purpose:
 *          Send a packet from renderer back to file format
 */

STDMETHODIMP
RTSPProtocol::PacketReady
(
    IHXPacket* pPacket
)
{
    /*
     * XXXSMP - We'll use the transport when available eventually, but
     * Control messages will do for now.
     */
    if(m_spProtocolLib)
    {
        return m_spProtocolLib->BackChannelPacketReady(pPacket);
    }
    return HXR_FAIL;
}

/* HXProtocol methods */

HX_RESULT
RTSPProtocol::server_hello(void)
{
    HX_RESULT   theErr = HXR_OK;
    IUnknown*   pContext = NULL;
    IHXValues*  pInfo = NULL;

    pContext = (IUnknown*)(IHXStreamSource*)mOwner;
    pContext->AddRef();

    theErr = CreateValuesCCF(pInfo, pContext);
    
    if (HXR_OK == theErr)
    {
        const char* pPropName = "path";

        if (m_bSDPInitiated)
        {
            pPropName = "helix-sdp";
        }

        theErr = SetCStringPropertyCCF(pInfo, pPropName, mPath, 
                                       pContext, FALSE);
    }
    

    m_ulSessionInitedTime = HX_GET_TICKCOUNT();
    
    theErr = m_spProtocolLib->Init((IUnknown*)pContext,
                                  mHost,
                                  mServerPort,
                                  (IHXRTSPClientProtocolResponse*)this,
                                  mUseProxy?RTSP_INIT_HXPRIVATE_AUTHORIZATION:0,
                                  m_pIDInfo,
                                  pInfo,
                                  m_bHTTPOnly,
                                  mCloakPort,
                                  FALSE);

    if (pInfo)
    {
        UINT32  ulMulticastOnly = 0;
	IHXBuffer*      pBuffer = NULL;

        m_spProtocolLib2->SetUseRTPFlag(m_bUseRTP);

        // "MulticastOnly" is set by the RTSPClientProtocol when it's scalable
        // multicast via SDP file.
        //
        // If "MulticastOnly" is set and the current transport is not multicast then
        // we will returns error immediately and won't waste time in transport switching
        pInfo->GetPropertyULONG32("MulticastOnly", ulMulticastOnly);
        m_bMulticastOnly = (ulMulticastOnly > 0)?TRUE:FALSE;
        if (m_bMulticastOnly && mCurrentTransport != MulticastMode)
        {
           // Retrieve "UnicastURL" from pInfo,
           // if it exists, then call HandleRedirectRequest("UnicastURL")
           // else report the following error

           if (HXR_OK == pInfo->GetPropertyCString("UnicastURL", pBuffer) && pBuffer)
           {
               theErr = HandleRedirectRequest((char*)pBuffer->GetBuffer(), 0);
               HX_RELEASE(pBuffer);
           }
           else
           {
	       theErr = HXR_SE_MULTICAST_DELIVERY_ONLY;
	   }
	}
    }

    HX_RELEASE(pInfo);
    HX_RELEASE(pContext);

    return theErr;
}

HX_RESULT
RTSPProtocol::proxy_hello(void)
{
    m_spProtocolLib->SetProxy(mProxy, mProxyPort);

    return server_hello();
}

HX_RESULT
RTSPProtocol::process(void)
{
    HX_RESULT theErr = HXR_OK;

    if (m_LastError != HXR_OK)
    {
        return m_LastError;
    }

#ifdef _MACINTOSH
#if defined(HELIX_FEATURE_AUTHENTICATION)
    if (m_bHandleWWWAuthentication)
    {
        /* Need to wait for system time */
        if (HXMM_ATINTERRUPT())
        {
            return HXR_OK;
        }

        m_bHandleWWWAuthentication = FALSE;
        handlePendingWWWAuthentication(m_WWWResult, m_pWWWValues);
        HX_RELEASE(m_pWWWValues);
    }
#endif
#endif

    switch(m_idleState)
    {
        case SEND_SETUP_REQUEST_STATE:
        {
            if (!m_bSocketsInited)
            {
                theErr = InitSockets();
            }

            if (HXR_OK == theErr)
            {
                // Make sure we are finished with the file header
                if (!mOwner->m_bContinueWithHeaders)
                {
                    theErr = send_setup_request();
                    m_idleState = NULL_STATE;
                }
            }
        }
        break;

        case ALERT_STATE:
        {
            // the new localizable server alert support is excluded for server
            // version < 10.1.1.322 since the alert number returned from the
            // server is not guaranteed to be correct
            UINT32 ulVersionTarget = HX_ENCODE_PROD_VERSION(10L, 1L, 1L, 322L);
            if (m_ulServerVersion < ulVersionTarget)
            {
                theErr = HXR_SERVER_ALERT;
            }
            else
            {
                // Set theErr to the correct ServerAlert HXR_ code.
                theErr = MAKE_SA(m_ulLastAlert);
                if (!IS_SERVER_ALERT(theErr))
                {
                    theErr = HXR_SERVER_ALERT;
                }
            }
            m_idleState = NULL_STATE;
        }
        break;

        default:
        break;
    }

    // check whether the connection has timed out
    if (!theErr && !m_bPaused)
    {
        ULONG32 ulNow = HX_GET_TICKCOUNT();

        // apply transport timeout logic if we haven't successfully established
        // the connection(control) OR we haven't received any data from the server
        // after receiving PLAY response with "200 OK"
        if (!mReceivedControl || (!m_bReceivedData && m_bPlaying))
        {
            if (mOwner->CheckTransportTimeout(ulNow))
            {
                /* Make sure that we have really not receioved any data.
                * If it is a sparse stream, transport may hold onto initial
                * data for around 2 seconds before releasing it to the next
                * layer.
                */
                if (!mReceivedControl)
                {
                    theErr = HXR_NET_CONNECT;
                }
                else
                {
                    if (m_spProtocolLib && m_spProtocolLib->IsDataReceived())
                    {
                        /* We haver received the data.. Transport will
                        * eventually give it to use (hopefully!)
                        */
                        m_bReceivedData = TRUE;
                    }
                    else
                    {
                        HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::process(): no data received detected (curr trans = %ld)", this, mCurrentTransport);

                        switch (mCurrentTransport)
                        {
                        case MulticastMode:
                            theErr = HXR_MULTICAST_UDP;
                            break;
                        case UDPMode:
                            theErr = HXR_NET_UDP;
                            break;
                        case TCPMode:
                            if (!m_bHTTPOnly)
                            {
                                theErr = HXR_NET_TCP;
                            }
                            else
                            {
                                theErr = HXR_DNR;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
        // apply server timeout logic
        else if (!mSourceEnd && m_ulLastPacketReceivedTime)
        {
            ULONG32 ulEllapsed = CALCULATE_ELAPSED_TICKS(m_ulLastPacketReceivedTime, ulNow); 

            if (mServerTimeout > 0 &&
                ulEllapsed > (mServerTimeout*MILLISECS_PER_SECOND))
	    {
                HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::process(): HXR_SERVER_TIMEOUT", this);
	        theErr = HXR_SERVER_TIMEOUT;
	    }		
        }

        // fallback to unicast if scalable multicast fails
        if (m_bSDPInitiated && HXR_MULTICAST_UDP == theErr && mLiveStream)
        {
            HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::process(): scalalable mcast fails; switching to unicast", this);
            if (HXR_OK == SwitchToUnicast())
            {
                theErr = HXR_OK;
            }
            else
            {
                theErr = HXR_MULTICAST_JOIN;
            }
        }
    }

    // remember the last error so we won't proceed to the next
    // RTSP state in subsequent process() calls
    if (HXR_OK == m_LastError && HXR_OK != theErr)
    {
        m_LastError = theErr;
    }

    return theErr;
}

HX_RESULT
RTSPProtocol::abort(void)
{
    HX_RESULT theErr = HXR_OK;

//XXX...Currently unimplemented

    return theErr;
}

HX_RESULT
RTSPProtocol::GetEvent
(
    UINT16 uStreamNumber,
    CHXEvent*& pEvent
)
{

    HX_RESULT result;
    IHXPacket* pPacket = NULL;

    pEvent = NULL;

    result = m_spProtocolLib->GetPacket(uStreamNumber, pPacket);

#ifdef _DEBUG
    if (result == HXR_AT_END)
    {
        STREAM_INFO* pStreamInfo;

        if (HXR_OK != mOwner->GetStreamInfo(uStreamNumber, pStreamInfo))
        {
            return HXR_FAIL;
        }

        HX_ASSERT(pStreamInfo->m_bSrcStreamDone);
    }
#endif

    if (pPacket)
    {
        // signal the raw data is received
        m_bReceivedData = TRUE;

        pEvent = new CHXEvent(pPacket);
        pPacket->Release();
    }

    return result;
}

HX_RESULT
RTSPProtocol::setup(const char* host, const char* path, UINT16 port,
                    HXBOOL LossCorrection, HXBOOL bHTTPCloak, 
                    HXBOOL bSDPInitiated, UINT16 cloakPort)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::setup()", this);

    HX_RESULT   theErr  = HXR_OK;
    IHXValues*  pRequestHeaders = NULL;
    IHXValues*  pCloakValues = NULL;
    IHXBuffer*  pRegionData = NULL;
    CHXString	strUserAgent;

    m_bSDPInitiated = bSDPInitiated;

    mOwner->GetRequest(m_pRequest);

    // XXXkshoop store origional headers for use
    // in replaying Describe's.  This supports
    // authentication.
    HX_ASSERT(m_pRequest);
    if (m_pRequest)
    {
        m_spIHXValuesStoredHeaders.Release();
        theErr = m_pRequest->GetRequestHeaders(
            m_spIHXValuesStoredHeaders.ptr_reference());

        HX_VERIFY(SUCCEEDED(theErr) && m_spIHXValuesStoredHeaders.IsValid());
    }

    // setup base class members
    theErr = HXProtocol::setup(host, path, port, LossCorrection, bHTTPCloak,
                               m_bSDPInitiated, cloakPort);

    if (theErr)
    {
        return theErr;
    }

    if
    (
        m_pRequest
        &&
        SUCCEEDED(m_pRequest->GetRequestHeaders(pRequestHeaders))
        &&
        pRequestHeaders
    )
    {
        pRequestHeaders->GetPropertyCString("RegionData", pRegionData);
    }

    HX_RELEASE(pRequestHeaders);

    // construct the IHXValues
    CreateValuesCCF(m_pIDInfo, m_pContext);

    IHXBuffer*  pGUID = NULL;
    IHXBuffer*  pClientID = NULL;
    IHXBuffer*  pPragma = NULL;

    CreateAndSetBufferCCF(pGUID, (UCHAR*)(const char*)m_guid, 
			  m_guid.GetLength()+1, m_pContext);
    CreateAndSetBufferCCF(pClientID, (UCHAR*)(const char*)m_clientID, 
			  m_clientID.GetLength()+1, m_pContext);
    CreateAndSetBufferCCF(pPragma, (UCHAR*)PRAGMA, 
			  strlen(PRAGMA)+1, m_pContext);

    m_pIDInfo->SetPropertyCString("GUID", pGUID);
    m_pIDInfo->SetPropertyCString("ClientID", pClientID);

    // server doesn't like this in SDP initiated playback
    if (!m_bSDPInitiated)
    {
        m_pIDInfo->SetPropertyCString("Pragma", pPragma);
    }

#if 0
    IHXBuffer* pPasswordBuffer;
    if (m_pPreferences && m_pPreferences->ReadPref("LoadTestPassword",
       pPasswordBuffer) == HXR_OK)
    {
        char szLoadTestPasswordKey[HX_COMPANY_ID_KEY_SIZE] = {0}; /* Flawfinder: ignore */
        // create the encrypted key
        CalcCompanyIDKey((const char*)pPasswordBuffer->GetBuffer(),
                        (const char*)szStarttime,
                        (const char*)"LoadTestID",
                        (const char*)pRCMagic1,
                        (const char*)pMagic2,
                        (UCHAR*) &szLoadTestPasswordKey[0]);

        char szEncodedLTP[HX_COMPANY_ID_KEY_SIZE * 2]; // probably overkill /* Flawfinder: ignore */
        BinTo64((const BYTE*)szLoadTestPasswordKey, HX_COMPANY_ID_KEY_SIZE,
            szEncodedLTP);

        IHXBuffer* pEncodedLTPBuffer = NULL;
	if (HXR_OK == CreateAndSetBufferCCF(pEncodedLTPBuffer, (UCHAR*)szEncodedLTP,
					    strlen(szEncodedLTP)+1), m_pContext))
	{
	    m_pIDInfo->SetPropertyCString("LoadTestPassword", pEncodedLTPBuffer);
	    HX_RELEASE(pEncodedLTPBuffer);
	}
        HX_RELEASE(pPasswordBuffer);

        /*
         * XXXSMP Very poor way to set a requirement.  This won't work
         * as soon rmartsp needs to send other requirements.  Comment
         * in rmartsp about this.
         */
        const UINT8 pRequireStr[] = "com.real.load-test-password-enabled";
        IHXBuffer* pReq = NULL;
	if (HXR_OK == CreateAndSetBufferCCF(pReq, pRequireStr, sizeof(pRequireStr) + 1, m_pContext))
	{
	    m_pIDInfo->SetPropertyCString("Require", pReq);
	    HX_RELEASE(pReq);
	}
    }
#endif

    if (pRegionData)
    {
        m_pIDInfo->SetPropertyCString("RegionData", pRegionData);
    }

    HX_RELEASE(pGUID);
    HX_RELEASE(pClientID);
    HX_RELEASE(pRegionData);
    HX_RELEASE(pPragma);

    /*
     * XXX...This had better get fixed under full IRMA (should
     *       be done with QueryInterface)
     */


    IHXCommonClassFactory* pCCF = NULL;
    if (HXR_OK != m_pContext->QueryInterface(IID_IHXCommonClassFactory,
					     (void**)&pCCF))
    {
	theErr = HXR_FAILED;
	goto exit;
    }

    m_spProtocolLib = SPIHXRTSPClientProtocol(pCCF, CLSID_IHXRTSPClientProtocol, &theErr);
    if (!m_spProtocolLib )
    {
	goto exit;
    }

    m_spProtocolLib2 = m_spProtocolLib.Ptr();

    if (m_bHTTPOnly)
    {
        IHXBuffer* pBuffer = NULL;
        const char* pszURL = NULL;

	if (HXR_OK == CreateValuesCCF(pCloakValues, m_pContext))
	{
	    if (HXR_OK == m_pIDInfo->GetPropertyCString("ClientID", pBuffer))
	    {
		pCloakValues->SetPropertyCString("ClientID", pBuffer);
	    }
	    HX_RELEASE(pBuffer);

	    if (m_pRequest)
	    {
		if (HXR_OK == m_pRequest->GetURL(pszURL))
		{
		    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (UCHAR*)pszURL, 
							strlen(pszURL)+1, m_pContext))
		    {
			pCloakValues->SetPropertyCString("url", pBuffer);
			HX_RELEASE(pBuffer);
		    }
		}

		if (HXR_OK == m_pRequest->GetRequestHeaders(pRequestHeaders))
		{
		    if (HXR_OK == pRequestHeaders->GetPropertyCString("Cookie", pBuffer))
		    {
			pCloakValues->SetPropertyCString("Cookie", pBuffer);
		    }
		    HX_RELEASE(pBuffer);
		}
		HX_RELEASE(pRequestHeaders);
	    }

	    if (m_spProtocolLib2)
	    {
		m_spProtocolLib2->InitCloak(m_pCloakPorts, m_nCloakPorts, pCloakValues);
	    }
	    HX_RELEASE(pCloakValues);
	}
    }

    if (m_bPrefetch && m_spProtocolLib2)
    {
        m_spProtocolLib2->EnterPrefetch();
    }

    if (m_spProtocolLib)
    {
	SPIHXStatistics spStats = m_spProtocolLib.Ptr();
	if (spStats)
	{
	    spStats->InitializeStatistics(m_ulRegistryID);
	}
    }

#ifdef HELIX_FEATURE_USER_AGENT_PREF
    ReadPrefCSTRING(m_pPreferences, "UserAgent", strUserAgent);
    if (strUserAgent.GetLength() > 0)
    {
        m_strUserAgent = strUserAgent;
    }
    else
#endif //HELIX_FEATURE_USER_AGENT_PREF
    {
        // USER_AGENT_PREFIX, USER_AGENT_STRING and USER_AGENT_POSTFIX are defined 
        // in common/include/hxver.h
        // TARVER_STRING_VERSION is defined in client/core/clntcore.ver
        // TARVER_STR_PLATFORM is defined in common/include/platform.h
        m_strUserAgent = USER_AGENT_PREFIX
                         USER_AGENT_STRING
                         USER_AGENT_POSTFIX "/"
                         TARVER_STRING_VERSION " ("
                         TARVER_STR_PLATFORM ")";
    }                               
    
#ifdef HELIX_FEATURE_USER_AGENT_EXTN
    CHXString strUserAgentExt;
    ReadPrefCSTRING(m_pPreferences, "UserAgentExtn", strUserAgentExt);
    if (strUserAgentExt.GetLength() > 0)
    {
        m_strUserAgent += " ";
        m_strUserAgent += strUserAgentExt;
    }
#endif

    m_spProtocolLib->SetBuildVersion((const char*)m_strUserAgent);

    m_pPendingStatus = m_spProtocolLib->GetPendingStatus();
    m_pStatistics = m_spProtocolLib->GetStatistics();

    // start communication with server
    if(mUseProxy)
    { 
        theErr = proxy_hello();
    }
    else
    {
        theErr = server_hello();
    }

    // Start Waiting for control channel..
    mOwner->StartDataWait(TRUE);

exit:

    HX_RELEASE(pCCF);

    return theErr;
}

// these get initialized every time we make a new connection with
// the server
void
RTSPProtocol::initialize_members(void)
{
    // intialize base class members
    HXProtocol::initialize_members();
    m_bPerfectPlayAllowed   = TRUE;


    // release any allocated local resources
    HX_RELEASE(m_pPendingStatus);
    HX_RELEASE(m_pStatistics);
    HX_RELEASE(m_pIDInfo);

    destroyProtocolLib();

    CHXMapLongToObj::Iterator i;
    for (i = m_pStreamInfoList->Begin(); i != m_pStreamInfoList->End(); ++i)
    {
        RTSP_STREAM_INFO* pStreamInfo = (RTSP_STREAM_INFO*)(*i);

        HX_DELETE(pStreamInfo);
    }
    m_pStreamInfoList->RemoveAll();
}

HX_RESULT
RTSPProtocol::seek
(
    ULONG32 posArg,
    ULONG32 posArg2,
    UINT16 seekFrom
)
{
    HX_RESULT theErr = m_spProtocolLib->SeekFlush();

    if (IsLive())
    {
        return theErr;
    }

    m_uCurrentStreamCount = m_uStreamCount;

    mSourceEnd = FALSE;

    m_bPendingSeek = TRUE;
    m_ulSeekPos1 = posArg;
    m_ulSeekPos2 = posArg2;

    return theErr;
}


HX_RESULT
RTSPProtocol::resume(UINT32 ulEndTime)
{
    HX_RESULT res = HXR_OK;
    
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::resume()", this);
    m_ulLastPacketReceivedTime = HX_GET_TICKCOUNT();
    if (m_bPendingSeek)
    {
        m_bPendingSeek = FALSE;
        m_bIsFirstResume = FALSE;

        if( !m_bPaused )
        {
            //According to the RTSP spec, you can send PLAY requests while
            //playing and servers should queue those PLAY requests up. Helix and
            //Real server don't like this and currently throw 501 errors. Either
            //way, however, this is not what we want doing quick seeks (frame
            //scrubbing). We want the servers to stop sending packets and then
            //send us packets as soon as they can for the new seek point. So, if
            //we are not paused and we are going to send a play request we send
            //a pause first. This is only for m_bSeekPending==TRUE.
            res = m_spProtocolLib->SendPauseRequest();

            //Not sure what the proper thing to do here is on error.  I guess,
            //just hope it works.
            HX_ASSERT(SUCCEEDED(res));
            
        }
        
        m_bPaused = FALSE;

        if (mCurrentTransport == UDPMode)
        {
            m_bAreResuming = TRUE;
            mOwner->StartDataWait();
        }

        return m_spProtocolLib->SendPlayRequest(m_ulSeekPos1, (!mLiveStream && ulEndTime > 0) ? ulEndTime : m_ulSeekPos2, 0);
    }
    if (m_bIsFirstResume)
    {
        m_bIsFirstResume = FALSE;
        m_bPaused = FALSE;

        res = m_spProtocolLib->SendPlayRequest(0, (!mLiveStream && ulEndTime > 0) ? ulEndTime : RTSP_PLAY_RANGE_BLANK, 0);

#if !defined(HELIX_FEATURE_ASM)
        if (HXR_OK == res)
        {
            IHXThinnableSource* pThinning = NULL;

            UINT32 ulDeliveryDate = 0;
            UINT32 ulAvgBitrate = 0;
            CHXMapLongToObj::Iterator i;
            for (i = m_pStreamInfoList->Begin(); i != m_pStreamInfoList->End(); ++i)
            {
                RTSP_STREAM_INFO* pStreamInfo = (RTSP_STREAM_INFO*)(*i);
                ulAvgBitrate += pStreamInfo->m_ulClipBandwidth;        
            }

#if defined(HELIX_FEATURE_TURBOPLAY)
            ulDeliveryDate = ulAvgBitrate * 10;
#else
            ulDeliveryDate = ulAvgBitrate * 1.5;
#endif /* HELIX_FEATURE_TURBOPLAY */

            if (HXR_OK == m_spProtocolLib->QueryInterface(IID_IHXThinnableSource, (void**)&pThinning))
            {
                res = pThinning->SetDeliveryBandwidth(ulDeliveryDate, 0);
            }
            HX_RELEASE(pThinning);
        }
#endif /* !HELIX_FEATURE_ASM */
        return res;            
    }
    else if (mCurrentTransport == UDPMode)
    {
        m_bAreResuming = TRUE;
        mOwner->StartDataWait();
    }

    if (!m_bPaused)
    {
        return HXR_OK;
    }

    m_bPaused = FALSE;

    return m_spProtocolLib->SendResumeRequest();
}


HX_RESULT
RTSPProtocol::stop(void)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::stop()", this);
    if (m_spProtocolLib)
    {
        m_spProtocolLib->SendTeardownRequest();
        destroyProtocolLib();
    }

    HXProtocol::stop();

#if defined(HELIX_FEATURE_REVERTER)
    if (m_pDataRevert)
    {
        m_pDataRevert->Done();
        HX_RELEASE(m_pDataRevert);
    }
#endif /* HELIX_FEATURE_REVERTER */

    CHXMapLongToObj::Iterator i;
    for (i = m_pStreamInfoList->Begin(); i != m_pStreamInfoList->End(); ++i)
    {
        RTSP_STREAM_INFO* pStreamInfo = (RTSP_STREAM_INFO*)(*i);

        HX_DELETE(pStreamInfo);
    }
    m_pStreamInfoList->RemoveAll();

    m_bReceivedData = FALSE;
    m_bPlaying = FALSE;
    m_uStreamCount = 0;

    return HXR_OK;
}

HX_RESULT
RTSPProtocol::pause(void)
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::pause()", this);
    HX_RESULT   rc = HXR_OK;

    // properly set the pause state for server timeout
    // calculation
    if (m_bIsFirstResume)
    {
        m_bPaused = TRUE;
    }
    else if (!m_bPaused)
    {
        m_bPaused = TRUE;
        rc = m_spProtocolLib->SendPauseRequest();
    }

    return rc;
}

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
void
RTSPProtocol::send_statistics
(
    UINT32      ulStatsMask
)
{
    UINT32  ulStats123 = 0;
    UINT32  ulStats4 = 0;
    char*   pszStats123 = NULL;
    char*   pszStats4 = NULL;
    char*   pszStats = NULL;

    prepare_statistics(ulStatsMask, pszStats123);
    if (pszStats123)
    {
        ulStats123 = strlen(pszStats123);
    }

    if (ulStatsMask & 8UL)
    {
        prepare_stats4(pszStats4, ulStats4);
    }

    if (pszStats123 && pszStats4)
    {
        pszStats = new char[ulStats123 + ulStats4 + 1];
        strcpy(pszStats, pszStats123); /* Flawfinder: ignore */
        strcat(pszStats, pszStats4); /* Flawfinder: ignore */

        HX_VECTOR_DELETE(pszStats123);
        HX_VECTOR_DELETE(pszStats4);
    }
    else if (pszStats123 && !pszStats4)
    {
        pszStats = pszStats123;
    }
    else if (!pszStats123 && pszStats4)
    {
        pszStats = pszStats4;
    }

    if (pszStats && m_spProtocolLib)
    {
        m_spProtocolLib->SendPlayerStats(pszStats);
    }

    HX_VECTOR_DELETE(pszStats);
    return;
}

HX_RESULT
RTSPProtocol::prepare_stats4(char*& pszStats, UINT32& ulStats)
{
    HX_RESULT           rc = HXR_OK;
    UINT32              i = 0;
    UINT32              ulAllocatedBufferSize = MAX_DEFAULT_STATS_SIZE;
    UINT32              ulValue = 0;
    char                szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    char*               pszValue = NULL;
    char*               pszTempStats = NULL;
    RTSP_STREAM_INFO*   pStreamInfo = NULL;
    STREAM_STATS*       pStreamStats = NULL;
    IHXBuffer*          pParentName = NULL;
    IHXBuffer*          pValue = NULL;
    CHXMapLongToObj::Iterator j;

    pszStats = new char[ulAllocatedBufferSize];
    ulStats = 0;

    SafeStrCpy(pszStats, "[Stat4:", MAX_DISPLAY_NAME);
    ulStats += 7;

    statistics_cat_ext(pszStats, ulAllocatedBufferSize, m_uStreamCount, " ", ulStats);

    // per stream stats
    if (m_pStreamInfoList->GetCount())
    {
        for (j = m_pStreamInfoList->Begin(); j != m_pStreamInfoList->End(); ++j)
        {
            pStreamInfo = (RTSP_STREAM_INFO*)(*j);

            pStreamStats = pStreamInfo->m_pStreamStats;
            if (!pStreamStats || !pStreamStats->m_bInitialized)
            {
                continue;
            }

            // mime type
            if (pStreamStats->m_pMimeType &&
                (pszValue = pStreamStats->m_pMimeType->GetStr()))
            {
                ulStats += strlen(pszValue);
                if (ulStats > (ulAllocatedBufferSize - MAX_DISPLAY_NAME))
                {
                    ulAllocatedBufferSize += MAX_DEFAULT_STATS_SIZE;
                    pszTempStats = new char[ulAllocatedBufferSize];

                    SafeStrCpy(pszTempStats, pszStats, ulAllocatedBufferSize);
                    HX_VECTOR_DELETE(pszStats);

                    pszStats = pszTempStats;
                }
                SafeStrCat(pszStats, pszValue, ulAllocatedBufferSize);
                HX_VECTOR_DELETE(pszValue);
            }
            else
            {
                SafeStrCat(pszStats, "N/A", ulAllocatedBufferSize);
                ulStats += 3;
            }
            SafeStrCat(pszStats, "|", ulAllocatedBufferSize);
            ulStats++;

            // codec
            if (HXR_OK == m_pRegistry->GetPropName(pStreamStats->m_pRenderer->m_ulRegistryID, pParentName))
            {
                SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Codec", pParentName->GetBuffer());
                if (HXR_OK == m_pRegistry->GetStrByName(szRegKeyName, pValue) && pValue)
                {
                    ulValue = pValue->GetSize();

                    pszValue = new char[ulValue+1];
                    strcpy(pszValue, (const char*)pValue->GetBuffer()); /* Flawfinder: ignore */

                    // replace space with underscore
                    for (i = 0; i < ulValue; i++)
                    {
                        if (pszValue[i] == ' ')
                        {
                            pszValue[i] = '_';
                        }
                    }
                }
                HX_RELEASE(pValue);
            }
            HX_RELEASE(pParentName);

            if (pszValue)
            {
                ulStats += strlen(pszValue);
                if (ulStats > (ulAllocatedBufferSize - MAX_DISPLAY_NAME))
                {
                    ulAllocatedBufferSize += MAX_DEFAULT_STATS_SIZE;
                    pszTempStats = new char[ulAllocatedBufferSize];

                    SafeStrCpy(pszTempStats, pszStats, ulAllocatedBufferSize);
                    HX_VECTOR_DELETE(pszStats);

                    pszStats = pszTempStats;
                }
                SafeStrCat(pszStats, pszValue, ulAllocatedBufferSize);
                HX_VECTOR_DELETE(pszValue);
            }
            else
            {
                SafeStrCat(pszStats, "N/A", ulAllocatedBufferSize);
                ulStats += 3;
            }
            SafeStrCat(pszStats, "|", ulAllocatedBufferSize);
            ulStats++;

            // bandwidth stats
            statistics_cat_ext(pszStats, ulAllocatedBufferSize, pStreamStats->m_pReceived->GetInt(), "|", ulStats);
            statistics_cat_ext(pszStats, ulAllocatedBufferSize, pStreamStats->m_pLost->GetInt(), "|", ulStats);
            statistics_cat_ext(pszStats, ulAllocatedBufferSize, pStreamStats->m_pReceived->GetInt() - pStreamStats->m_pNormal->GetInt(), "|", ulStats);
            statistics_cat_ext(pszStats, ulAllocatedBufferSize, pStreamStats->m_pAvgBandwidth->GetInt(), "|", ulStats);
            statistics_cat_ext(pszStats, ulAllocatedBufferSize, pStreamStats->m_pCurBandwidth->GetInt(), "|", ulStats);

            SafeStrCat(pszStats, ";", ulAllocatedBufferSize);
            ulStats++;
        }
    }
    else
    {
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, 0, " ", ulStats);
    }
    pszStats[ulStats-1] = ' ';

    // Transport
    statistics_cat_ext(pszStats, ulAllocatedBufferSize, mCurrentTransport, " ", ulStats);

    // TurboPlay
    statistics_cat_ext(pszStats, ulAllocatedBufferSize, m_bFastStart, "|", ulStats);
    if (m_bFastStart)
    {
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, mOwner->m_turboPlayStats.ulAcceleratedBW, "|", ulStats);
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, mOwner->m_turboPlayStats.ulBufferedTime, "|", ulStats);
    }
    else
    {
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, mOwner->m_turboPlayStats.tpOffReason, "|", ulStats);
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, 0, "|", ulStats);
    }
    SafeStrCat(pszStats, " ", ulAllocatedBufferSize);
    ulStats++;

    // Startup latency, first data packet arrives!
    statistics_cat_ext(pszStats, ulAllocatedBufferSize, mOwner->GetFirstDataArriveTime(), " ", ulStats);

    // Reason of clip end
    statistics_cat_ext(pszStats, ulAllocatedBufferSize, mOwner->m_srcEndCode, " ", ulStats);
    ulStats++;

    // Startup time
    ulValue = 0;
    if (m_ulSessionInitedTime != 0 && mOwner->m_ulPlaybackStartedTix != 0)
    {
    	ulValue = CALCULATE_ELAPSED_TICKS(m_ulSessionInitedTime, mOwner->m_ulPlaybackStartedTix);
        if (ulValue > mOwner->m_ulDelay)
        {
            ulValue -= mOwner->m_ulDelay;
        }
        else
        {
            ulValue = 0;
        }
    }
    statistics_cat_ext(pszStats, ulAllocatedBufferSize, ulValue, " ", ulStats);
    ulStats++;
    
    // Play time
    ulValue = mOwner->GetCurrentPlayTime();
    if (ulValue > mOwner->m_ulDelay)
    {
        ulValue -= mOwner->m_ulDelay;
    }
    else
    {
        ulValue = 0;
    }
    statistics_cat_ext(pszStats, ulAllocatedBufferSize, ulValue, " ", ulStats);
    ulStats++;
    
    // Rebuffering time
    statistics_cat_ext(pszStats, ulAllocatedBufferSize, mOwner->m_ulRebufferingCumulativeTime, " ", ulStats);
    ulStats++;

    // Latency statistics
    if (mOwner->IsLive())
    {
    	UINT32 ulAvg, ulMin, ulMax;
    	mOwner->GetLatencyStats(ulAvg, ulMin, ulMax);
    	mOwner->ResetLatencyStats();
        // avg latency
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, ulAvg, "|", ulStats);
        // min latency
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, ulMin, "|", ulStats);
        // max latency
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, ulMax, NULL, ulStats);
    }
    else
    {
        // avg latency
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, 0, "|", ulStats);
        // min latency
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, 0, "|", ulStats);
        // max latency
        statistics_cat_ext(pszStats, ulAllocatedBufferSize, 0, NULL, ulStats);
    }
    ulStats++;

    SafeStrCat(pszStats, "]", ulAllocatedBufferSize);

    return rc;
}

STREAM_STATS*
RTSPProtocol::create_statistics(UINT16 uStreamNumber)
{
    HX_RESULT       result = HXR_OK;
    STREAM_STATS*   pStats = NULL;
    IHXBuffer*      pParentName = NULL;
    CHAR            RegKeyName[MAX_DISPLAY_NAME] = {0};

    if (!m_pRegistry)
    {
        goto cleanup;
    }

    if (HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pParentName) && pParentName)
    {
        SafeSprintf(RegKeyName, MAX_DISPLAY_NAME, "%s.Stream%d", pParentName->GetBuffer(), uStreamNumber);

        /*
         * This the location where the first STREAM_STATS for the stream
         * is instantiated. The protocol library may make another to fill
         * the fields in the registry, but it must use the keys created
         * here
         */

        //ASSERT(!m_pRegistry->GetId(RegKeyName));
        UINT32 uStreamId = 0;
        if (!m_pRegistry->GetId(RegKeyName))
        {
            uStreamId = m_pRegistry->AddComp(RegKeyName);
        }
        else
        {
            uStreamId = m_pRegistry->GetId(RegKeyName);
        }

        pStats = new STREAM_STATS(m_pContext, uStreamId);
    }
    HX_RELEASE(pParentName);

cleanup:

    return pStats;
}
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

HXBOOL
RTSPProtocol::IsSourceDone(void)
{
    return mSourceEnd;
}

HX_RESULT
RTSPProtocol::GetCurrentBuffering(UINT16  uStreamNumber,
                                  UINT32& ulLowestTimestamp,
                                  UINT32& ulHighestTimestamp,
                                  UINT32& ulNumBytes,
                                  HXBOOL& bDone)
{
    HX_RESULT   theErr  = HXR_OK;

    ulLowestTimestamp   = 0;
    ulHighestTimestamp  = 0;
    ulNumBytes          = 0;
    bDone               = FALSE;

    if (m_spProtocolLib)
    {
        theErr = m_spProtocolLib->GetCurrentBuffering(uStreamNumber,
                                                      ulLowestTimestamp,
                                                      ulHighestTimestamp,
                                                      ulNumBytes,
                                                      bDone);
        if (!theErr && ulNumBytes > 0)
        {
            m_bReceivedData = TRUE;
        }
    }

    return theErr;
}

void RTSPProtocol::destroyProtocolLib()
{
    HXLOGL3(HXLOG_RTSP, "RTSPProtocol[%p]::destroyProtocolLib()", this);
    
    if (m_spProtocolLib2)
    {
	m_spProtocolLib2.Clear();
    }

    if (m_spProtocolLib)
    {
        IHXAutoBWDetection* pABD = NULL;
        if (HXR_OK == m_spProtocolLib->QueryInterface(IID_IHXAutoBWDetection, 
                                                     (void**)&pABD))
        {
            mOwner->ShutdownABD(pABD);
        }
        HX_RELEASE(pABD);

        m_spProtocolLib->Done();
	m_spProtocolLib.Clear();
    }
}
