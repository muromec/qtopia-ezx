/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxntsrc.cpp,v 1.165 2009/02/12 04:23:44 sgarg Exp $
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

#if defined (_UNIX) || defined (_WIN16)
#include <stdlib.h>
#endif

#include "hxcomm.h"
#include "hxplugn.h"

#include "ihxpckts.h"
#include "hxpends.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxpref.h"
#include "hxausvc.h"
#include "hxasm.h"
#include "hxauthn.h"
#include "hxgroup.h"
#include "hxsmbw.h"
#include "hxrsdbf.h"
#include "hxmime.h"
#include "hxurlutil.h"
#include "hxslist.h"
#include "hxstring.h"
#include "portaddr.h"

#include "chxeven.h"
#include "chxelst.h"
#include "strminfo.h"

#include "hxbsrc.h"
#include "hxsrc.h"
#include "hxntsrc.h"
#include "hxstrm.h"
#include "hxplay.h"
#include "rtspif.h"
#include "hxprotocol.h"
#include "dtrvtcon.h"
#include "rtspprotocol.h"
#include "latency_mode_hlpr.h"

#if defined(HELIX_FEATURE_PNA)
#include "hxpnapro.h"
#endif /* HELIX_FEATURE_PNA */

#include "rmfftype.h"
#include "prefdefs.h"
#include "plprefk.h"
#include "hxtick.h"
#include "netbyte.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxmap.h"
#include "hxmangle.h"
#include "srcinfo.h"
#include "srcerrs.h"
#include "ihxcookies.h"
#include "pacutil.h"

#if defined(HELIX_FEATURE_NETCHECK)
#include "netchck.h"
#endif /* defined(HELIX_FEATURE_NETCHECK) */

#include "hxrasyn.h"
#include "hxaudstr.h"
#include "hxaudses.h"
#include "verutil.h"
#include "hxrendr.h"
#include "hxcleng.h"
#include "hxtlogutil.h"
#include "hxescapeutil.h"
#include "wmbufctl.h"
#include "fbbufctl.h"

#include "clntcore.ver"
#include "hxver.h"
#include "hxwinver.h"

#if defined(HELIX_FEATURE_DRM)
#include "hxdrmcore.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


// maximum level of statistics that we wanna send back to the server
#define MAX_STATS_MASK                            15    // value from server is a bit field
#define MAX_LOGINFO_LENGTH                      2048    // maximum of 2K bytes of statistics
#define DEFAULT_RECORD_SOURCE_OVERRUN_PROTECTION_TIME 5000    // how close to write point we can get


/* RTSP Setup may take a lot ot time */
#define NEW_DEF_SETTINGS_TIMEOUT    3000

#define MINIMUM_TIMEOUT             5

const short int MAX_CLOAK_PORTS = 4;
const UINT16 g_uCloakPorts[MAX_CLOAK_PORTS] = {80, 8080, 554, 7070};
#define CLOAKPORT_URL_OPTION    "cloakport"

// HXLOG helper
#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
#define STATE_ENTRY(x) case x: return #x
static
const char* GetStateDesc(NetSourceState state)
{
    switch (state)
    {
    STATE_ENTRY(NETSRC_READY);
    STATE_ENTRY(NETSRC_PACREADY);
    STATE_ENTRY(NETSRC_PACPENDING);
    STATE_ENTRY(NETSRC_TRANSPORTREADY);
    STATE_ENTRY(NETSRC_TRANSPORTPENDING);
    STATE_ENTRY(NETSRC_RECONNECTSTARTED);
    STATE_ENTRY(NETSRC_RECONNECTPENDING);
    STATE_ENTRY(NETSRC_RECONNECTFORCED);
    STATE_ENTRY(NETSRC_REDIRECTSTARTED);
    STATE_ENTRY(NETSRC_REDIRECTPENDING);
    STATE_ENTRY(NETSRC_REDIRECTFAILED);
    STATE_ENTRY(NETSRC_ENDED);
    STATE_ENTRY(NETSRC_ENDPENDING);
    }
    HX_ASSERT(false);
    return "bad NetSourceState";
}

#else
inline const char* GetStateDesc(NetSourceState state) { return ""; }
#endif

HXNetSource::HXNetSource() :
      mServerSelRecordSupport(FALSE)
    , mInterframeControlSupport(FALSE)
    , mServerHasBandwidthReport(FALSE)
    , mServerHasFrameControl(FALSE)
    , m_pHost (0)
    , m_pPath (NULL)
    , m_pResource (0)
    , m_uPort (0)
    , m_pProxy (0)
    , m_uProxyPort (0)
    , m_bUseProxy (FALSE)
    , m_pPostRedirectHeaderList(NULL)
    , m_pProto (0)
    , m_ulStartBuffering (0)
    , m_ulServerTimeOut (90)
    , m_ulConnectionTimeout(DEFAULT_CONN_TIMEOUT)
    , m_fReBufferPercent ((float)0.)
    , m_pLogInfoList(NULL)
    , m_ulLogInfoLength(0)
    , m_bAttemptReconnect(TRUE)
    , m_pszReconnectServer(NULL)
    , m_pszReconnectProxy(NULL)
    , m_pszReconnectURL(NULL)
    , m_ulReconnectProxyPort(0)
    , m_ulReconnectServerPort(0)
    , m_pszRedirectServer(NULL)
    , m_pszRedirectResource(NULL)
    , m_ulRedirectServerPort(0)
    , m_bRedirectInSMIL(FALSE)
    , m_pProtocolStatus (0)
    , m_pCookies(NULL)
    , m_pCookies2(NULL)
    , m_ulUDPTimeout (NEW_DEF_SETTINGS_TIMEOUT)
    , m_lPacketTimeOffSet(0)
    , m_ulMulticastTimeout (NEW_DEF_SETTINGS_TIMEOUT)
    , m_ulTCPTimeout(NEW_DEF_SETTINGS_TIMEOUT)
    , m_ulSendStatsMask(0)
    , m_ulStatsInterval(0)
    , m_ulSeekPendingTime(0)
    , m_ulTransportPrefMask(0)
    , m_uProtocolType(unknownProtocol)
    , m_PreferredTransport(UnknownMode)
    , m_CurrentTransport(UnknownMode)
    , m_bLossCorrection (FALSE)
    , m_bAltURL(FALSE)
    , m_bRTSPProtocol (FALSE)
    , m_bDataWaitStarted(FALSE)
    , m_bConnectionWait(TRUE)
    , m_bSendStatistics (TRUE)
    , m_bUseUDPPort (FALSE)
    , m_bResendAuthenticationInfo(FALSE)
    , m_bTimeBased(FALSE)
    , m_bUserHasCalledResume(FALSE)
    , m_bUserHasCalledStartInit(FALSE)
    , m_bAtInterrupt (FALSE)
    , m_bBruteForceReconnected(FALSE)
    , m_bBruteForceConnectToBeDone(FALSE)
    , m_bPerfectPlayPreferenceRead(FALSE)
    , m_bPerfectPlayErrorChecked(FALSE)
    , m_bServerHasPerfectPlay(FALSE)
    , m_bInRetryMode(FALSE)
    , m_bPushDownSet(FALSE)
    , m_bForcePerfectPlay (FALSE)
    , m_bServerHasTransportSwitching (FALSE)
    , m_bSeekPending(FALSE)
    , m_pCloakPortList(NULL)
    , m_nNumberOfCloakPorts(0)
    , m_nCurrPortIdx (0)
    , m_uCurrCloakedPort(0)
    , m_bProtocolPaused(FALSE)
    , m_prefTransportState(PTS_UNKNOWN)
    , m_pPreferredTransport(NULL)
    , m_pPreferredTransportManager(NULL)
    , m_pConnBWInfo(NULL)
    , m_pReconnectCallback(NULL)
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    , m_pStatsCallback(NULL)
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
    , m_pPAC(NULL)
    , m_pPACInfoList(NULL)
    , m_state(NETSRC_READY)
    , m_ulRecordSourceOverrunProtectionTime(DEFAULT_RECORD_SOURCE_OVERRUN_PROTECTION_TIME)
    , m_bDisableRecordSourceOverrunProtection(FALSE)
    , m_pBufferCtl(NULL)
    , m_pWMBufferCtl(NULL)
    , m_bTransportPrefFromURLAttempted(FALSE)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::HXNetSource");

    memset(m_szUserName, 0, MAX_DISPLAY_NAME);
    memset(m_szPassword, 0, MAX_DISPLAY_NAME);
}

HXNetSource::~HXNetSource()
{
    DoCleanup();

    HX_DELETE(m_pPACInfoList);

    HX_VECTOR_DELETE(m_pHost);
    HX_VECTOR_DELETE(m_pPath);
    HX_VECTOR_DELETE(m_pResource);
    HX_VECTOR_DELETE(m_pCloakPortList);
    HX_VECTOR_DELETE(m_pszReconnectServer);
    HX_VECTOR_DELETE(m_pszReconnectProxy);
    HX_VECTOR_DELETE(m_pszReconnectURL);
    HX_VECTOR_DELETE(m_pszRedirectServer);
    HX_VECTOR_DELETE(m_pszRedirectResource);

    HX_RELEASE(m_pProtocolStatus);
    HX_RELEASE(m_pCookies);
    HX_RELEASE(m_pCookies2);    
    HX_RELEASE(m_pPreferredTransport);
    HX_RELEASE(m_pPreferredTransportManager);
    HX_RELEASE(m_pConnBWInfo);
    HX_RELEASE(m_pPAC);

    HXLOGL4(HXLOG_NSRC, "HXNetSource::~HXNetSource");
}

HX_RESULT
HXNetSource::DoCleanup(EndCode endCode)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::DoCleanup");

    if (m_pPostRedirectHeaderList)
    {
        CHXSimpleList::Iterator i = m_pPostRedirectHeaderList->Begin();
        for (; i != m_pPostRedirectHeaderList->End(); ++i)
        {
            IHXValues* pHeader = (IHXValues*) (*i);
            HX_RELEASE(pHeader);
        }
        m_pPostRedirectHeaderList->RemoveAll();
    }
    HX_DELETE(m_pPostRedirectHeaderList);

    if (m_pBufferCtl)
    {
	m_pBufferCtl->Close();
    }
    HX_RELEASE(m_pBufferCtl);
    HX_RELEASE(m_pWMBufferCtl);

    m_latencyModeHlpr.Close();

    m_srcEndCode = endCode;

#if defined(HELIX_FEATURE_PAC)
    if (NETSRC_PACPENDING == m_state)
    {
        m_state = NETSRC_READY;
        m_pPAC->AbortProxyInfo(this);
    }
#endif /* HELIX_FEATURE_PAC */

#if defined(HELIX_FEATURE_SMARTERNETWORK)
    // notify the failure of transport detection
    if (m_pPreferredTransport)
    {
        if (m_pPlayer && m_pHost && m_prefTransportState != PTS_READY)
        {
            m_pPreferredTransport->RemoveTransport();
        }
        m_pPreferredTransport->RemoveTransportSink(this);
        HX_RELEASE(m_pPreferredTransport);
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    // sends the stats
    if (m_pProto)
    {
        // log stop action
        LogInformation("STOP", NULL);

        if (m_bSendStatistics)
        {
            m_pProto->send_statistics(m_ulSendStatsMask);
        }
    }

    HXSource::DoCleanup(endCode);

    cleanup_proxy();

    /* UnRegister any previously registered source */
    if (m_pSourceInfo)
    {
        m_pSourceInfo->UnRegister();
    }

    //  Close down the net connection
    if (m_pProto)
    {
        m_pProto->stop();
        HX_RELEASE (m_pProto);
    }

    // cleanup the log list
    if (m_pLogInfoList)
    {
        while (m_pLogInfoList->GetCount() > 0)
        {
            char* pszInfo = (char*) m_pLogInfoList->RemoveHead();
            delete [] pszInfo;
        }

        HX_DELETE(m_pLogInfoList);
    }

    if (m_pReconnectCallback)
    {
        m_pReconnectCallback->CancelCallback();
        HX_RELEASE(m_pReconnectCallback);
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (m_pStatsCallback)
    {
        m_pStatsCallback->CancelCallback();
        HX_RELEASE(m_pStatsCallback);
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

#if defined(HELIX_FEATURE_PAC)
    while (m_pPACInfoList && m_pPACInfoList->GetCount())
    {
        PACInfo* pPACInfo = (PACInfo*)m_pPACInfoList->RemoveHead();
        HX_DELETE(pPACInfo);
    }
#endif /* HELIX_FEATURE_PAC */

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP
HXNetSource::QueryInterface(REFIID riid, void** ppvObj)
{
    if (HXSource::QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }

    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPreferredTransportSink), (IHXPreferredTransportSink*)this },
            { GET_IIDHANDLE(IID_IHXProxyAutoConfigCallback), (IHXProxyAutoConfigCallback*)this }
        };

    HX_RESULT res = ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    
    // if successful, return immediately...
    if (SUCCEEDED(res))
    {
        return res;
    }
    // ... otherwise proceed onward

    if (m_pBufferCtl &&
	     m_pBufferCtl->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
    else if (m_pProto &&
             m_pProto->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pPlayer &&
             m_pPlayer->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    // we don't have m_pPlayer during AutoConfig
    else if (m_pEngine &&
             m_pEngine->QueryInterface(riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32)
HXNetSource::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32)
HXNetSource::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
HXNetSource::Setup(const char*  host,
                    const char* resource,
                    UINT16      port,
                    HXBOOL        LossCorrection,
                    const CHXURL* pURL,
                    HXBOOL        bAltURL)
{
    HXLOGL3(HXLOG_NSRC, "HXNetSource::Setup");

    HX_RESULT       theErr = HXR_OK;
    IHXValues*      pValues = NULL;
    IHXValues*      pRequestHeaders = NULL;
    IHXBuffer*      pBuffer = NULL;
    IHXBuffer*      pPlayerCookies = NULL;

    if (!m_bBruteForceReconnected &&
        !m_bReSetup)
    {
        theErr = SetupRegistry();
        if (HXR_OK != theErr)
        {
            goto cleanup;
        }
    }

    if (!m_bReSetup)
    {
        m_ulOriginalDelay = m_ulDelay;
    }

    // Remember whether or not we ask for loss correction!
    m_bLossCorrection = LossCorrection;

    // delete protocol object if one exists
    HX_RELEASE (m_pProto);

    m_uProtocolType = pURL->GetProtocol();
    if (rtspProtocol == m_uProtocolType || helixSDPProtocol == m_uProtocolType)
    {
        m_bRTSPProtocol = TRUE;
        m_ulSendStatsMask = MAX_STATS_MASK;
        /* Perfect Play Always supported iin RTSP,
         * till we add some preference to disable it.
         */
        HXBOOL bSupported = TRUE;
        SetOption(HX_PERFECTPLAY_SUPPORTED, &bSupported);
    }

    if (helixSDPProtocol != m_uProtocolType)
    {
        //  get out immediately if we have bogus parameters
        if(!host || !*host)
        {
            theErr = HXR_INVALID_URL_HOST;
            goto cleanup;
        }

        if(!theErr && (!resource || !*resource))
        {
            theErr = HXR_INVALID_URL_PATH;
            goto cleanup;
        }

        if  (m_pHost != host)
        {
            HX_VECTOR_DELETE(m_pHost);
            m_pHost = new char[::strlen(host) + 1];
            if (!m_pHost)
            {
                theErr = HXR_OUTOFMEMORY;
                goto cleanup;
            }
            ::strcpy(m_pHost, host); /* Flawfinder: ignore */
        }

        if(m_pResource != resource)
        {
            HX_VECTOR_DELETE(m_pResource);
            m_pResource = new char[::strlen(resource) + 1];
            if (!m_pResource)
            {
                theErr = HXR_OUTOFMEMORY;
                goto cleanup;
            }
            ::strcpy(m_pResource, resource); /* Flawfinder: ignore */
        }
    }

    if (m_pURL != pURL)
    {
        HX_DELETE(m_pURL);
        m_pURL = new CHXURL(*pURL);
        if (!m_pURL)
        {
            theErr = HXR_OUTOFMEMORY;
            goto cleanup;
        }

        HX_VECTOR_DELETE(m_pszURL);
        m_pszURL = new char[::strlen(m_pURL->GetURL()) + 1];
        if (!m_pszURL)
        {
            theErr = HXR_OUTOFMEMORY;
            goto cleanup;
        }
        ::strcpy(m_pszURL, m_pURL->GetURL()); /* Flawfinder: ignore */
    }

    m_uPort = port;

    HX_VECTOR_DELETE(m_pPath);

    pValues = m_pURL->GetProperties();

    if (pValues &&
        HXR_OK == pValues->GetPropertyBuffer(PROPERTY_PATH, pBuffer) && pBuffer)
    {
        StrAllocCopy(m_pPath, (char*)pBuffer->GetBuffer());
    }

    HX_RELEASE(pBuffer);
    HX_RELEASE(pValues);

    // save parameters for use with different protocols.
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    // save URL to the registry
    if (m_pStats)
    {
        m_pStats->m_pSourceName->SetStr((char*)m_pszURL);
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    // set request
    m_bAltURL = bAltURL;
    SetRequest(m_pURL, bAltURL);

    if (!m_pCookies && HXR_OK != m_pEngine->QueryInterface(IID_IHXCookies, (void**)&m_pCookies))
    {
        m_pCookies = NULL;
    }

    if (!m_pCookies2 && HXR_OK != m_pEngine->QueryInterface(IID_IHXCookies2, (void**)&m_pCookies2))
    {
        m_pCookies2 = NULL;
    }

    if (m_pCookies || m_pCookies2)
    {
        if (HXR_OK == m_pRequest->GetRequestHeaders(pRequestHeaders) && pRequestHeaders)
        {
            HX_RESULT res = HXR_FAIL;
            if(m_pCookies2)
                res = m_pCookies2->GetCookies(m_pHost, m_pPath, pBuffer, pPlayerCookies);
            else if(m_pCookies)
                res = m_pCookies->GetCookies(m_pHost, m_pPath, pBuffer);

            if (HXR_OK == res && pBuffer)
            {
                pRequestHeaders->SetPropertyCString("Cookie", pBuffer);
                if(pPlayerCookies)
                {
                    pRequestHeaders->SetPropertyCString("PlayerCookie", pPlayerCookies);
                }
            }
            HX_RELEASE(pBuffer);
            HX_RELEASE(pPlayerCookies);
        }
    }
    HX_RELEASE(pRequestHeaders);

#if defined(HELIX_FEATURE_SMARTERNETWORK)
    if (!m_pPreferredTransportManager &&
        HXR_OK != m_pEngine->QueryInterface(IID_IHXPreferredTransportManager,
                                            (void**)&m_pPreferredTransportManager))
    {
        theErr = HXR_FAILED;
        goto cleanup;
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    if (!m_pConnBWInfo && m_pEngine)
    {
        m_pEngine->QueryInterface(IID_IHXConnectionBWInfo, 
                                  (void**)&m_pConnBWInfo);
    }

    // Read all relevant preferences...
    theErr = ReadPreferences();

    // either we will wait for the first source to negotiate the transport
    //        then connect the remaining sources
    // or we are initiating PAC(ProxyAutoConfig)
    if (HXR_WOULD_BLOCK == theErr)
    {
        theErr = HXR_OK;
    }
    else if (HXR_OK == theErr)
    {
        theErr = FinishSetup();
    }

cleanup:

    return theErr;
}

HX_RESULT
HXNetSource::FinishSetup()
{
    HX_RESULT rc = HXR_OK;
    HXBOOL bSDPInitiated = FALSE;
    IHXSourceBufferingStats3* pSrcBufStats = NULL;
    IUnknown* pUnk = NULL;

    // since we've reached FinishSetup, we know we have no more
    // transport detection to do...

    if (m_pPreferredTransport)
    {
        m_pPreferredTransport->RemoveTransportSink(this);
    }

    // start off with the preferred protocol
    rc = CreateProtocol();
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    rc = InitializeProtocol();
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    if (helixSDPProtocol == m_uProtocolType)
    {
        bSDPInitiated = TRUE;

        CHXString url = HXEscapeUtil::UnEscape(m_pURL->GetURL());
            
        HX_VECTOR_DELETE(m_pResource);
        m_pResource = new char[url.GetLength() - 9]; 
        ::strcpy(m_pResource, (const char*)url + 10); // "helix-sdp:"
    }
                
    rc = m_pProto->setup(m_pHost,
                         m_pResource,
                         m_uPort,
                         m_bLossCorrection,
                         (HTTPCloakMode == m_CurrentTransport)?TRUE:FALSE,
                         bSDPInitiated,
                         m_uCurrCloakedPort);

    // Mask this error
    // Let the _ProcessIdle() determine whether ReportError() or
    // TransportSwitching for HXR_BAD_TRANSPORT
    if(rc == HXR_BLOCK_CANCELED || rc == HXR_BAD_TRANSPORT)
    {
        rc = HXR_OK;
    }

    if (HXR_OK != rc)
    {
        mLastError = rc;
        goto cleanup;
    }

    // Setup source buffer stats 
    if (HXR_OK == m_pProto->QueryInterface(IID_IHXSourceBufferingStats3,
                                           (void**)&pSrcBufStats))
    {
        SetSrcBufStats(pSrcBufStats);
        HX_RELEASE(pSrcBufStats);
    }

    if (HXR_OK == QueryInterface(IID_IUnknown, (void**)&pUnk))
    {
        m_latencyModeHlpr.Init(pUnk);
        HX_RELEASE(pUnk);
    }

    // create log info list
    m_pLogInfoList = new CHXSimpleList;
    m_ulLogInfoLength = 0;

    // start time of this source
    m_ulSourceStartTime = HX_GET_TICKCOUNT();

    if (m_pBufferCtl)
    {
	m_pBufferCtl->Close();
    }
    HX_RELEASE(m_pBufferCtl);
    HX_RELEASE(m_pWMBufferCtl);
    
#ifdef HELIX_FEATURE_FEEDBACK_BUFFER_CONTROL
    m_pBufferCtl = new HXFeedbackBufferControl();
#else
    m_pBufferCtl = new HXWatermarkBufferControl();
#endif /* HELIX_FEATURE_FEEDBACK_BUFFER_CONTROL */

    if (m_pBufferCtl)
    {
	m_pBufferCtl->AddRef();
	m_pBufferCtl->QueryInterface(IID_IHXWatermarkBufferControl,
				     (void**)&m_pWMBufferCtl);

	m_pBufferCtl->Init((IUnknown*)(IHXStreamSource*)this);

	if (m_pWMBufferCtl)
	{
	    m_pWMBufferCtl->SetSource(this);
	}
    }

cleanup:

    return rc;
}

HX_RESULT
HXNetSource::DoSeek(ULONG32 seekTime)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::DoSeek");

    m_bSeekInsideRecordControl = FALSE;

    if ((!m_pProto && m_state != NETSRC_RECONNECTPENDING) || 
        (mLiveStream && !m_bPlayFromRecordControl && m_bSourceEnd))
    {
        return HXR_OK;
    }

    // log seek action
    LogInformation("SEEK", NULL);
    m_bSeekedOrPaused = TRUE;

    if (seekTime >= m_ulDelay)
    {
        seekTime -= m_ulDelay;
        m_bDelayed = FALSE;
    }
    else
    {
        seekTime = 0;

        /* This source has not been even started yet...
         * Do not attempt to seek it if the start time = 0
         */
        if (m_bDelayed && m_ulStartTime == 0 && !m_bSourceEnd)
        {
            // will start pre-fetch again in TryResume()
            if (!m_bIsPreBufferingDone)
            {
                m_bIsPreBufferingStarted = FALSE;

                // will be registered again in DoResume() or TryResume()
                if (m_pSourceInfo)
                {
                    m_pSourceInfo->UnRegister();
                }
            }

            return HXR_OK;
        }

        m_bDelayed = TRUE;
    }

    /* Add any start time to seek time */
    seekTime    += m_ulStartTime;

    //  Skip over any stream data packets that occur between delay and current
    // play time.  If delay is less than current play time, then seek past the
    // difference to avoid streaming all those unneeded (too-early) packets.
    // Note: this condition can occur if pfs:displayWhile property is used in
    // a SMIL2 element and it becomes "displayable" some time after the
    // presentation has already started; in that case its begin delay is zero
    // (relative to its syncbase time so it may be non-zero if its parent or
    // ancestor starts after 0 in the presentation), and the time of its
    // AddTrack() occurs after zero (relative to syncbase):
    if (m_pSourceInfo->m_bSeekOnLateBegin)
    {
        UINT32 ulCurPlayTime = m_pPlayer->GetInternalCurrentPlayTime();
        if (m_bFirstResume  &&  (ulCurPlayTime > m_ulDelay)  &&
                (ulCurPlayTime - m_ulDelay >
                m_pSourceInfo->m_ulSeekOnLateBeginTolerance) )
        {
            seekTime += ulCurPlayTime - m_ulDelay;
            HXLOGL4(HXLOG_CORE, "[%p]HXNetSource::DoSeek()\tseekTime(%lu) "
                    "increased for late begin by %lu (== ulCurPlayTime(%lu) -"
                    " m_ulDelay(%lu)\n", this, seekTime,
                    ulCurPlayTime - m_ulDelay, ulCurPlayTime, m_ulDelay);
        }
    }

    /* Are we seeking past the last expected packet time?
     * If so, don't bother... and mark this source as done
     */
    HX_ASSERT(m_llLastExpectedPacketTime <= MAX_UINT32);
    // XXX HP make sure the source has been initialized otherwise
    // m_llLastExpectedPacketTime could be 0 and we could falsely
    // mark the source ended
    if (m_bInitialized && !mLiveStream && seekTime >= m_llLastExpectedPacketTime)
    {
        if (m_pSourceInfo && m_pSourceInfo->m_bSeekToLastFrame)
        {
            seekTime = INT64_TO_ULONG32(m_llLastExpectedPacketTime);
        }
        else
        {
            m_bSourceEnd = TRUE;
            m_bForcedSourceEnd = TRUE;
            AdjustClipBandwidthStats(FALSE);

#if defined(HELIX_FEATURE_RECORDCONTROL)
            if (m_pRecordControl)
            {
                m_pRecordControl->Seek(seekTime);
            }
#endif /* HELIX_FEATURE_RECORDCONTROL*/
            goto cleanup;
        }
    }

    // workaround for b#42118 for older servers(<= RealServer8.0 Gold)
    // server sends no more streamdone when reaching the endtime if seek
    // is occurred after streamdone has been sent. fix has been made in the latest server
    if (m_bSourceEnd && m_bCustomEndTime && m_pProto && m_pProto->GetRDTFeatureLevel() < 2)
    {
        m_bRTSPRuleFlagWorkAround = TRUE;
    }

    if (mLiveStream)
    {
        seekTime += m_ulFirstPacketTime;
    }

#if defined(HELIX_FEATURE_RECORDCONTROL)
    if (m_pRecordControl && m_pRecordControl->Seek(seekTime) == HXR_OK &&
        m_bPlayFromRecordControl)
    {
        m_bSeekInsideRecordControl = TRUE;
    }
    else
#endif /* HELIX_FEATURE_RECORDCONTROL */
    {
        m_ulSeekPendingTime = seekTime;
        m_bSeekPending = TRUE;

	if(m_bPlayFromRecordControl && !m_bProtocolPaused && m_pProto)
	{
	    m_pProto->pause();
	    m_bProtocolPaused = TRUE;
	}

        if(m_pProto)
            m_pProto->seek(seekTime, seekTime, FALSE);

        if (mLiveStream)
        {
            seekTime = 0;
        }

        m_bSourceEnd        = FALSE;
        m_bForcedSourceEnd  = FALSE;

        STREAM_INFO*    pStreamInfo = NULL;

        CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
        for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
        {
            pStreamInfo = (STREAM_INFO*) (*lStreamIterator);
            pStreamInfo->ResetPreReconnectEventList();
        }
    }

    // initiating reconnect NOW ...
    if (NETSRC_RECONNECTPENDING == m_state)
    {
        m_state = NETSRC_RECONNECTFORCED;
        if (m_pReconnectCallback)
        {
            m_pReconnectCallback->CancelCallback();
        }
        StartReconnect();
    }

    m_uLastBuffering                = 0;
    m_bInitialBuffering             = TRUE;
    m_bIsPreBufferingStarted        = FALSE;
    m_bIsPreBufferingDone           = FALSE;
    m_pushDownStatus                = PUSHDOWN_NONE;
    m_state                         = NETSRC_READY;

    if (m_pBufferCtl)
    {
	m_pBufferCtl->OnSeek();
    }

    // Buffer manager needs to know whether we seek inside the RecordControl
    // to not reset variables related to stream completeness.
    m_pBufferManager->DoSeek(seekTime, m_bSeekInsideRecordControl);

cleanup:

    return HXR_OK;
}

HX_RESULT
HXNetSource::DoPause(void)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::DoPause");

    if (!m_pProto || m_bPaused)
    {
        return HXR_OK;
    }

    // log pause action
    LogInformation("PAUSE", NULL);
    m_bSeekedOrPaused = TRUE;

    m_pBufferManager->DoPause();

    if (m_pBufferCtl)
    {
	m_pBufferCtl->OnPause();
    }

    m_bPaused = TRUE;

    if (m_bPlayFromRecordControl && !m_bDelayed && !m_bPartOfNextGroup)
    {
        return HXR_OK;
    }

    /* Do not call pause if the source has ended. In RTSP, if the transport
     * is paused, it does not give us any packets. We would never issue a
     * resume since the source has ended and this would result in starting
     * the playback without giving any packets to the renderer if the preroll
     * of the renderer happens to be zero.(like RealText).
     */

    /* Call Pause even after source is done. This is to keep track
     * of Accounting information on the server side for PPV content
     * The only time we do not issue a pause is if we have internally
     * stopped the stream because of a end/dur tag.
     *
     * RTSP transport has been fixed so that it returns packets even
     * in a paused state. So the bug reported above should not occur.
     * Moreover, we now call protocol Resume even if the source is done.
     * So the transport should never be really in a Paused state when
     * we need to read packets.
     */

    if (!m_bForcedSourceEnd)
    {
        m_pProto->pause();
        m_bProtocolPaused = TRUE;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        if (m_pStatsCallback)
        {
            m_pStatsCallback->PauseCallback();
        }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
    }

    return HXR_OK;
}


HX_RESULT
HXNetSource::StartInitialization(void)
{
    if (m_pProto)
    {
        m_bUserHasCalledStartInit = TRUE;
        m_pBufferManager->DoResume();
        if (m_pSourceInfo)
        {
            m_pSourceInfo->Resumed();

            /* This is temporary. The source MUST be registered by this point.
             * Fix it after B1.
             * For now, added a work around
             * - Rahul
             */
            if (!m_pSourceInfo->IsRegistered() && m_pSourceInfo->IsActive())
            {
                m_pSourceInfo->Register();
                if (m_pPlayer)
                {
                    m_pPlayer->RegisterSourcesDone();
                }
            }
        }

        setPacketDelays();

        HX_ASSERT(m_llLastExpectedPacketTime <= MAX_UINT32);

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        if (m_bSendStatistics && m_ulStatsInterval > 0)
        {
            if (!m_pStatsCallback)
            {
                m_pStatsCallback = new ReconnectCallback(this, TRUE);
                m_pStatsCallback->AddRef();
            }

            if (m_pStatsCallback->IsPaused())
            {
                m_pStatsCallback->ResumeCallback();
            }
            else
            {
                m_pStatsCallback->ScheduleCallback(m_ulStatsInterval);
            }
        }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

        // RTSP PAUSE request is not issued in DoPause() if it's played from the record control
        // since we want to keep receiving data at the background even though the playback is paused
        // at the foreground.
        // 
        // BUT, if the pause is issued because of SEEKING, then we still need to send PAUSE
        // per RFC2326 before sending PLAY. Otherwise, RFC says this is not a seek but instead 
        // a playlist queue(fixed b#112030)
        if (m_bSeekPending && (!m_bProtocolPaused || (m_bPlayFromRecordControl && !m_bDelayed && m_bPaused)))
        {
            m_pProto->pause();
        }

        m_bProtocolPaused = FALSE;

        return m_pProto->resume(INT64_TO_UINT32(m_llLastExpectedPacketTime));
    }

    return HXR_OK;
}


HX_RESULT
HXNetSource::StopInitialization(void)
{
	  HXLOGL4(HXLOG_NSRC, "HXNetSource::StopInitialization");
	  
	  if (m_bPlayFromRecordControl && !m_bDelayed && !m_bPartOfNextGroup)
    {
        return HXR_OK;
    }

    m_pBufferManager->DoPause();

    if (!m_bForcedSourceEnd)
    {
        m_pProto->pause();
        m_bProtocolPaused = TRUE;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        if (m_pStatsCallback)
        {
            m_pStatsCallback->PauseCallback();
        }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
    }

    return HXR_OK;
}


HX_RESULT
HXNetSource::DoResume(UINT32 ulLoopEntryTime,
		      UINT32 ulProcessingTimeAllowance)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::DoResume");

    HX_RESULT theErr = HXR_OK;

    /* This may happen if a new source is added from SMIL renderer during
     * initialization of exisitng sources. We will eventually call Resume
     * on this source once it is initialized in SourceInfo::ProcessIdle
     */
    if (!m_bInitialized)
    {
        return HXR_OK;
    }

    m_bUserHasCalledResume = TRUE;
    ChangeRebufferStatus(REBUFFER_NONE);

    // log resume action
    LogInformation("Resume", NULL);

    if(m_pBufferManager && !m_bSourceEnd && CanBeResumed())
    {
        m_pBufferManager->DoResume();
    }

    /* Call Resume even after source is done. This is to keep track
     * of Accounting information on the server side for PPV content
     * Only if we have forced a source end because of end/dur tag
     * or because of seeking past source's duration in SMIL,
     * we do not send Pause/Resume to the server
     */
    if (!m_pProto || (!m_bPaused && !m_bFirstResume))
    {
        return HXR_OK;
    }

    if(m_bPlayFromRecordControl && !m_bProtocolPaused && !m_bFirstResume && !m_bResumePending)
    {
        m_bPaused = FALSE;
        return HXR_OK;
    }

    if (m_bSourceEnd || CanBeResumed())
    {
        m_bResumePending    = FALSE;

        HX_ASSERT(m_pSourceInfo);

        if (!m_bSourceEnd)
        {
            /* This is temporary. This assert is currently tripping...
             * Ideally, it should not. Fix it after B1.
             * For now, added a work around
             * - Rahul
             */
            //  HX_ASSERT(!m_pSourceInfo || m_pSourceInfo->IsRegistered() || !m_pSourceInfo->IsActive());
            if (m_pSourceInfo && !m_pSourceInfo->IsRegistered() && m_pSourceInfo->IsActive())
            {
                m_pSourceInfo->Register();
                if (m_pPlayer)
                {
                    m_pPlayer->RegisterSourcesDone();
                }
            }

            setPacketDelays();

            if (m_bBruteForceReconnected && m_bSeekPending)
            {
                theErr = m_pProto->seek(m_ulSeekPendingTime);
            }

            HX_ASSERT(INT64_TO_ULONG32(m_llLastExpectedPacketTime) <= MAX_UINT32);

            m_bProtocolPaused   = FALSE;

            theErr              = m_pProto->resume(INT64_TO_UINT32(m_llLastExpectedPacketTime));

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
            if (m_bSendStatistics && m_ulStatsInterval > 0)
            {
                if (!m_pStatsCallback)
                {
                    m_pStatsCallback = new ReconnectCallback(this, TRUE);
                    m_pStatsCallback->AddRef();
                }

                if (m_pStatsCallback->IsPaused())
                {
                    m_pStatsCallback->ResumeCallback();
                }
                else
                {
                    m_pStatsCallback->ScheduleCallback(m_ulStatsInterval);
                }
            }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
        }

        // resume the audio streams if the source is added
        // while the player is in play
        // CAUTION: this will cause rewind in audio service
        if (m_bFirstResume                          &&
            IsPlaying()                             &&
            (m_state == NETSRC_READY)               &&
            m_pPlayer && m_ulDelay <= m_pPlayer->GetInternalCurrentPlayTime())
        {
            ResumeAudioStreams();
        }

        m_bFirstResume      = FALSE;
        m_bPaused           = FALSE;

        if (m_pSourceInfo)
        {
            m_pSourceInfo->Resumed();
        }
	
	if (m_pBufferCtl)
	{
	    m_pBufferCtl->OnResume();
	}
    }

    if (!m_bIsActive && !m_bDelayed && m_pPlayer &&
        m_pPlayer->GetInternalCurrentPlayTime() >= m_ulDelay)
    {
        AdjustClipBandwidthStats(TRUE);
    }

    return theErr;
}


/************************************************************************
 *      Method:
 *          IHXPendingStatus::GetStatus
 *      Purpose:
 *          Called by the user to get the current pending status from an object
 */
STDMETHODIMP
HXNetSource::GetStatus
(
    REF(UINT16) uStatusCode,
    REF(IHXBuffer*) pStatusDesc,
    REF(UINT16) ulPercentDone
)
{
    HX_RESULT   rc = HXR_OK;

    /* Default values*/
    uStatusCode     = HX_STATUS_READY;
    pStatusDesc     = NULL;
    ulPercentDone   = 100;

    if (m_bDelayed)
    {
        goto cleanup;
    }

    if (FastStartPrerequisitesFullfilled(uStatusCode, ulPercentDone))
    {
        goto cleanup;
    }

    if (m_bSourceEnd && 
        (!IsPlayingFromRecordControl() ||
	 !m_pRecordControl ||
	 m_pRecordControl->IsFinishedReading()))
    {
        if (!IsRebufferDone())
        {
            uStatusCode = HX_STATUS_BUFFERING;
            ulPercentDone = 99;
        }
        else
        {
            if (m_bInitialBuffering)
            {
                InitialBufferingDone();
            }

            m_uLastBuffering = 100;
            uStatusCode = HX_STATUS_READY;
    	    if (m_rebufferStatus == REBUFFER_REQUIRED)
            {
                ChangeRebufferStatus(REBUFFER_NONE);
            }
        }
        goto cleanup;
    }

    if (m_bInitialized)
    {
        if (m_pBufferManager)
        {
            m_pBufferManager->GetStatus(uStatusCode, pStatusDesc, ulPercentDone);
        }
    }
    else
    {
        uStatusCode         = HX_STATUS_INITIALIZING;
        pStatusDesc         = NULL;
        ulPercentDone       = 0;

        if (m_pProto)
        {
            m_pProto->GetStatus(uStatusCode, pStatusDesc, ulPercentDone);
        }
    }

    HXLOGL4(HXLOG_BUFF, "GetStatus: BfrMgr-PercentDone= %u", ulPercentDone);

    if (ulPercentDone > 100)
    {
        ulPercentDone = 100;
    }

    /* If we have not yet received a single packet, do not claim to
     * be done with buffering. This is needed to correctyl playback sparse
     * datatypes with no preroll.
     */
    if (!m_bReceivedData && (ulPercentDone == 100))
    {
        ulPercentDone = 99;
    }

    /* We only aggregate buffering from a lower level if we are in a buffering mode.
     * Reason: Once the initial bufering is done, we go in buffering more ONLY IF the
     * renderer tells us that it is in a panic state and we run out of packets.
     */
    if (ulPercentDone == 100 && !m_bInitialBuffering)
    {
        // Rebuffer requested by the Renderer might not be
        // done yet
        if (!IsRebufferDone())
        {
            uStatusCode = HX_STATUS_BUFFERING;
            ulPercentDone = 99;
        }
        else
        {
            uStatusCode = HX_STATUS_READY;
        }
        goto cleanup;
    }

    if (HX_STATUS_READY == uStatusCode)
    {
        ulPercentDone = 100;
    }
    else if (HX_STATUS_CONTACTING == uStatusCode)
    {
        ulPercentDone = 0;
    }
    else if (HX_STATUS_INITIALIZING == uStatusCode)
    {
        ulPercentDone = 0;
    }

    if (uStatusCode == HX_STATUS_BUFFERING && m_uLastBuffering < 100)
    {
        if (ulPercentDone < m_uLastBuffering)
        {
            ulPercentDone = m_uLastBuffering;
        }
        else
        {
            m_uLastBuffering = ulPercentDone;
        }
    }

    if (m_bInitialBuffering && HX_STATUS_READY == uStatusCode)
    {
        InitialBufferingDone();
        m_uLastBuffering = 100;
    }

cleanup:

    if (m_pBufferCtl)
    {
        if ((HX_STATUS_READY == m_uLastStatusCode) &&
            (HX_STATUS_BUFFERING == uStatusCode))
        {
            UINT32 ulRemainToBufferInMs = 0;
            UINT32 ulRemainToBuffer = 0;

            if (m_pBufferManager)
            {
                m_pBufferManager->GetRemainToBuffer(ulRemainToBufferInMs, 
                                                    ulRemainToBuffer);
            }
            
            m_pBufferCtl->OnBuffering(ulRemainToBufferInMs, ulRemainToBuffer);
        }
        else if ((HX_STATUS_BUFFERING == m_uLastStatusCode) &&
                 (HX_STATUS_READY == uStatusCode))
        {
            m_pBufferCtl->OnBufferingDone();
        }
    }

    m_uLastStatusCode = uStatusCode;

    if (m_uLastStatusCode == HX_STATUS_READY && m_rebufferStatus == REBUFFER_REQUIRED)
    {
         ChangeRebufferStatus(REBUFFER_NONE);
    }
    FastStartUpdateInfo();

    if(m_bSeekInsideRecordControl && IsPlaying())
    {
        m_bSeekInsideRecordControl = FALSE;
    }

    return rc;
}


UINT16
HXNetSource::GetNumStreams(void)
{
    HX_ASSERT(m_bInitialized);

    return m_uNumStreams;
}

HX_RESULT
HXNetSource::GetStreamInfo(ULONG32          ulStreamNumber,
                            STREAM_INFO*&   theStreamInfo)
{
    HX_RESULT       theErr = HXR_OK;
    STREAM_INFO*    pStreamInfo = NULL;

    if (!mStreamInfoTable->Lookup((LONG32)ulStreamNumber, (void *&)pStreamInfo))
    {
        theErr = HXR_INVALID_PARAMETER;
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    STREAM_STATS*   pStreamStats = NULL;
    if (m_pProto && HXR_OK == m_pProto->GetStreamStatistics(ulStreamNumber, &pStreamStats))
    {
        pStreamInfo->m_pStats = pStreamStats;

        if (!pStreamInfo->m_pStats)
        {
            return HXR_UNEXPECTED;
        }
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    theStreamInfo = pStreamInfo;

    return theErr;
}

HX_RESULT
HXNetSource::GetEvent(UINT16 usStreamNumber, 
		      CHXEvent*& pEvent, 
		      UINT32 ulLoopEntryTime, 
		      UINT32 ulProcessingTimeAllowance)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::GetEvent");

    HX_RESULT       nResult = HXR_OK;
    STREAM_INFO*    pStreamInfo = NULL;

    if (!mStreamInfoTable->Lookup((LONG32) usStreamNumber, (void *&) pStreamInfo))
    {
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

    if (!m_bPlayFromRecordControl)
    {
        nResult = GetEventFromProtocol(usStreamNumber, pStreamInfo, pEvent);

#if defined(HELIX_FEATURE_RECORDCONTROL)
        // Case of record-only RecordControl (no playback support).
        if (pEvent && m_pRecordControl)
        {
            m_pRecordControl->OnPacket(pEvent->GetPacket(), pEvent->GetTimeOffset());
        }
#endif /* HELIX_FEATURE_RECORDCONTROL */
    }
    else
    {
        pEvent = NULL;

        if (pStreamInfo->m_bReconnectToBeDone)
        {
            CHXEventList*  pEventList = &pStreamInfo->m_EventList;
            if (pEventList->GetNumEvents())
                pEvent = pEventList->RemoveHead();

            nResult = pEvent ? HXR_OK : HXR_NO_DATA;
        }
        else
        {
            nResult = GetEventFromRecordControl(usStreamNumber, 
						pStreamInfo, 
						pEvent,
						FALSE,	// do not force
						ulLoopEntryTime, 
						ulProcessingTimeAllowance);
        }
    }

    if (HXR_AT_END == nResult && 
        HXR_OK != pStreamInfo->m_streamEndReasonCode)
    {
        ActualReportError(pStreamInfo->m_streamEndReasonCode);
    }

    return nResult;
}

HX_RESULT
HXNetSource::GetEventFromRecordControl(UINT16 usStreamNumber, 
				       STREAM_INFO* pStreamInfo, 
				       CHXEvent*& pEvent,
				       HXBOOL bForce,
				       UINT32 ulLoopEntryTime,
				       UINT32 ulProcessingTimeAllowance)
{
#if defined(HELIX_FEATURE_RECORDCONTROL)
    if(!m_bPlayFromRecordControl)
        return HXR_UNEXPECTED;

    if (!pStreamInfo)
    {
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

    HX_ASSERT(m_pRecordControl);
    HX_ASSERT(m_pPlayer);

    if (!bForce)
    {                                                             
	UINT32 ulMaxRendererDispatchTime = m_pPlayer->ComputeFillEndTime(
		    m_pPlayer->GetInternalCurrentPlayTime(),
		    m_pPlayer->GetGranularity(),
		    m_ulMaxPreRoll + MAX_INTERSTREAM_TIMESTAMP_JITTER);

        // take into consideration of startime offset(?start=xxx)
        ulMaxRendererDispatchTime += m_ulStartTime;

        // take into consideration of m_ulFirstPacketTime for live source
        ulMaxRendererDispatchTime += m_ulFirstPacketTime;

	// We supply packets from the record control only when needed
	// since sparse streams may result in excessive buffering of packets due to
	// interleaving with non-sparse packets. 
	if (m_pRecordControl->IsRead() &&
	    (((LONG32) (ulMaxRendererDispatchTime - m_pRecordControl->GetLatestTimestampRead())) < 0))
	{
	    return HXR_WOULD_BLOCK;
	}
    }
        
    if ((ulLoopEntryTime != 0) && (ulProcessingTimeAllowance == 0) && m_pPlayer)
    {
	// Use the most generous - non-interrupt type allowance.
	ulProcessingTimeAllowance = m_pPlayer->GetPlayerProcessingInterval(FALSE);
    }

    pEvent = NULL;
    IHXPacket* pPacket = NULL;

    HX_RESULT nResult;

    do
    {
	nResult = m_pRecordControl->GetPacket(usStreamNumber, pPacket);
	
	if (nResult == HXR_OK)
	{
	    UINT32 ulEventTime = 0;
	    
	    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::::HXNetSource[%p]::GetEvent(Strm=%hu) Got Packet from Rec.Ctrl Time=%lu",
		    m_pPlayer,
		    this,
		    usStreamNumber,
		    pPacket->GetTime());
	    
	    if (pStreamInfo)
	    {
		if (!CanSendToDataCallback(pPacket))
		{
		    UINT32 ulLastPkt = 
		    pStreamInfo->BufferingState().LastPacketTimestamp();
		    
		    // Use timestamp from the last packet
		    ulEventTime = CalcEventTime(pStreamInfo, ulLastPkt, TRUE);
		}
		else
		{
		    ulEventTime = CalcEventTime(pStreamInfo, pPacket->GetTime(), 
						TRUE);
		    
		    /* Update buffering info and stats */
		    DataCallback(pPacket);
		}
	    }
	    
	    pEvent = new CHXEvent(pPacket, 0);
	    HX_RELEASE(pPacket);
	    
	    if(!pEvent)
		return HXR_FAILED;
	    
	    pEvent->SetTimeStartPos(ulEventTime);
	    pEvent->SetTimeOffset(m_ulStartTime - m_ulDelay);
	}
	else
	{
	    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::::HXNetSource[%p]::GetEvent(Strm=%hu) No Packet from Rec.Ctrl Err=%lu SourceEnd=%c StreamDone=%c",
		    m_pPlayer,
		    this,
		    usStreamNumber,
		    nResult,
		    m_bSourceEnd ? 'T' :'F',
		    pStreamInfo->m_bSrcStreamDone ? 'T' : 'F');
			
	    if (nResult == HXR_RETRY)
	    {
		if ((ulLoopEntryTime != 0) &&
		    ((HX_GET_BETTERTICKCOUNT() - ulLoopEntryTime) > ulProcessingTimeAllowance))
		{
		    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::HXNetSource[%p]::GetEvent(Strm=%hu) GetPacket from Rec.Ctrl CPU use timeout: Time=%lu Allowed=%lu", 
			    m_pPlayer, 
			    this,
			    usStreamNumber,
			    HX_GET_BETTERTICKCOUNT() - ulLoopEntryTime,
			    ulProcessingTimeAllowance);
		    break;
		}
	    }
	    else if (nResult == HXR_NO_DATA)
	    {
		if (m_bSourceEnd || pStreamInfo->m_bSrcStreamDone)
		{
		    if (m_pRecordControl->IsFinishedReading())
		    {
			if (m_pBufferManager)
			{
			    m_pBufferManager->Stop();
			}
		    }
		    
		    nResult = HXR_AT_END;
		}
		
		if (nResult == HXR_NO_DATA)
		{
		    nResult = HandleOutOfPackets(pStreamInfo);
		}
	    }
	}
    } while (nResult == HXR_RETRY);

    return nResult;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_RECORDCONTROL */
}

HX_RESULT
HXNetSource::GetEventFromProtocol(UINT16 usStreamNumber, STREAM_INFO* pStreamInfo, CHXEvent*& pEvent)
{
    HX_RESULT       theErr = HXR_OK;
    UINT32          ulEventTime = 0;
    UINT32          ulRemainToBufferInMs = 0;
    UINT32          ulRemainToBuffer = 0;
    IHXPacket*      pPacket = NULL;
    CHXEvent*       pTempEvent = NULL;
    CHXEventList*  pEventList = NULL;

    pEvent = NULL;

    if (!m_bInitialized && m_state == NETSRC_READY)
    {
        return HXR_NO_DATA;
    }

    if (m_bPaused && m_bDelayed)
    {
        if (TryResume() && m_pPlayer)
        {
            m_pPlayer->RegisterSourcesDone();
            DoResume();
        }
        else
        {
            return HXR_NO_DATA;
        }
    }

    /* give some time to the net object...*/
    theErr = _ProcessIdle();

    if (theErr)
    {
        return theErr;
    }

    if (!pStreamInfo)
    {
        HX_ASSERT(FALSE);
        return HXR_INVALID_PARAMETER;
    }

    if (pStreamInfo->m_bReconnectToBeDone)
    {
        pEventList = &pStreamInfo->m_EventList;
        if (pEventList->GetNumEvents() && !m_bPlayFromRecordControl)
        {
            pEvent = pEventList->RemoveHead();
            HXLOGL3(HXLOG_RECO, "GetEventFromPreReconnect\t%lu\t%lu", usStreamNumber, pEvent->GetPacket()->GetTime());
        }

        if (m_pProto)
        {
            while (TRUE)
            {
                theErr = m_pProto->GetEvent(usStreamNumber, pTempEvent);
                if (theErr)
                {
                    // Mask off any non-crucial errors
                    switch (theErr)
                    {
                        case HXR_AT_END:
                        case HXR_NO_DATA:
                        case HXR_BUFFERING:
                            theErr = HXR_OK;
                            break;
                        default:
                            break;
                    }

                    // if there is still an error, it needs to be reported
                    if (theErr)
                    {
                        return theErr;
                    }
                    break;
                }

                HX_ASSERT(pTempEvent);

                if (!pStreamInfo->m_pPosReconnectEventList)
                {
                    pStreamInfo->m_pPosReconnectEventList = new CHXEventList;
                }
                HXLOGL3(HXLOG_RECO, "AddEventToPosReconnect\t%lu\t%lu", usStreamNumber, pTempEvent->GetPacket()->GetTime());
                pStreamInfo->m_pPosReconnectEventList->InsertEvent(pTempEvent);
            }

            ProcessReconnect(pStreamInfo);
        }
    }
    else
    {
        pEventList = pStreamInfo->m_pPosReconnectEventList;
        if (pEventList && pEventList->GetNumEvents())
        {
            pEvent = pEventList->RemoveHead();
            HXLOGL3(HXLOG_RECO,"GetEventFromPosReconnect\t%lu\t%lu", usStreamNumber, pEvent->GetPacket()->GetTime());
        }

        if (!pEvent && m_pProto)
        {
            while (TRUE)
            {
                theErr = m_pProto->GetEvent(usStreamNumber, pEvent);
                if (theErr)
                {
                    // Mask off any non-crucial errors
                    switch (theErr)
                    {
                        case HXR_AT_END:
                        case HXR_NO_DATA:
                        case HXR_BUFFERING:
                            theErr = HXR_OK;
                            break;
                        default:
                            break;
                    }

                    // if there is still an error, it needs to be reported
                    if (theErr)
                    {
                        return theErr;
                    }
                    break;
                }

                HX_ASSERT(pEvent);

                if (!pStreamInfo->m_ulReconnectOverlappedPackets)
                {
                    break;
                }

                HXLOGL3(HXLOG_RECO, "DeleteEvent(overlapped)\t%lu\t%lu", usStreamNumber, pEvent->GetPacket()->GetTime());

                HX_DELETE(pEvent);
                pStreamInfo->m_ulReconnectOverlappedPackets--;
            }
            if (pEvent)
            {
                theErr = UpdateEvent(usStreamNumber, pEvent);
                if (HXR_OK != theErr)
                {
                    return theErr;
                }
                HXLOGL3(HXLOG_NSRC, "GetEventFromTransport\t%lu\t%lu", usStreamNumber, pEvent->GetPacket()->GetTime());
            }
        }

        if (pEvent)
        {
            AddToPreReconnectEventList(pStreamInfo, pEvent);
        }
    }

    if (pEvent)
    {
        pPacket = pEvent->GetPacket();
        if (pPacket)
        {
            if (!m_bPlayFromRecordControl &&
		CanSendToDataCallback(pPacket)&& !pEvent->IsPreSeekEvent())
            {
                /* Update buffering info and stats */
                DataCallback(pPacket);
            }

            if (pPacket->IsLost())
            {
		UINT32 ulLastPkt = 
		    pStreamInfo->BufferingState().LastPacketTimestamp();
                // Use timestamp from the last packet
                ulEventTime = CalcEventTime(pStreamInfo, ulLastPkt, TRUE);
            }
            else
            {
                // If this is the first time we received data,
                // then remember the time so we can report it...
                if (!m_bReceivedData)
                {
                    HXLOGL2(HXLOG_CORE, "%p\tHXNetSource::FirstDataReceived", this);

                    m_bReceivedData = TRUE;
                    m_bSeekPending = FALSE;

                    FirstDataArrives();

                    if (!m_ulFirstPacketTime && (mLiveStream || m_bRestrictedLiveStream))
                    {
                        // m_lPacketTimeOffSet != 0 when redirect has occured where
                        // we don't want to reset the first packet time.
                        if (m_lPacketTimeOffSet == 0)
                        {
                            m_ulFirstPacketTime = pPacket->GetTime();

                            if (m_bRestrictedLiveStream)
                            {
                                m_llLastExpectedPacketTime = CAST_TO_INT64 pPacket->GetTime() + CAST_TO_INT64 m_ulEndTime;
                            }
                        }
                    }
                }

                ulEventTime = CalcEventTime(pStreamInfo, 
                                            pPacket->GetTime() + m_lPacketTimeOffSet,
                                            TRUE);
            }
            pEvent->SetTimeStartPos(ulEventTime);
        }

	pEvent->SetTimeOffset(m_ulStartTime - m_ulDelay - m_lPacketTimeOffSet);

	/***
	    // This creates much logging noise - enable only for deep analysis of hard event queue bugs
	    HXLOGL4(HXLOG_NSRC, "HXNetSource(%p)::GetEventFromProtocol() EventTime=%lu Offset=%d PacketTime=%lu Delay=%lu StartTime=%lu PacketTimeOffset=%lu pPacket=%p",
		this,
	        pEvent->GetTimeStartPos(),
		pEvent->GetTimeOffset(),
		(pPacket ? pPacket->GetTime() : 0),
		m_ulDelay,
		m_ulStartTime,
		m_lPacketTimeOffSet,
		pPacket);
	***/
    }
    else if (m_bSourceEnd || pStreamInfo->m_bSrcStreamDone)
    {
        theErr = HXR_AT_END;
    }
    else
    {
        if(m_bPlayFromRecordControl)
        {
            theErr = HXR_NO_DATA;
        }
        else
        {
            theErr = HandleOutOfPackets(pStreamInfo);
        }
    }

    if (m_pBufferCtl)
    {
	if (theErr == HXR_BUFFERING)
	{
	    /* Make sure we have not paused the server when we are in a
	     * buffering state
	     */
	    m_pBufferCtl->OnBuffering(ulRemainToBufferInMs, ulRemainToBuffer);
	}
	else if (theErr == HXR_AT_END)
	{
	    m_pBufferCtl->OnClipEnd();
	}
    }

#ifdef LOSS_HACK
    if (m_ulLossHack > 0 && ((UINT32) (rand() % 100) < m_ulLossHack) &&
        !theErr && pEvent && !(pEvent->GetPacket())->IsLost())
    {
        GenerateFakeLostPacket(pEvent);
        /* Update the stats */
        pStreamInfo->m_ulLost++;
    }
#endif /* LOSS_HACK */

    return theErr;
}

HX_RESULT       
HXNetSource::HandleOutOfPackets(STREAM_INFO* pStreamInfo)
{
    HX_RESULT res = HXR_NO_DATA;

    if (!pStreamInfo)
    {
        res = HXR_UNEXPECTED;
    }
    else if (m_pBufferManager)
    {
        UINT32 ulRemainToBufferInMs = 0;
        UINT32 ulRemainToBuffer = 0;

        m_pBufferManager->GetRemainToBuffer(ulRemainToBufferInMs, ulRemainToBuffer);
    
        if (ulRemainToBufferInMs || ulRemainToBuffer)
        {
            res = HXR_BUFFERING;
        }
        else if (ShouldRebufferOnOOP(pStreamInfo))
        {
            DoRebuffer();
            res = HXR_BUFFERING;
        }
    }

    return res;
}

HX_RESULT
HXNetSource::UpdateEvent(UINT16 usStreamNumber, CHXEvent*& theEvent)
{
    HX_RESULT   rc = HXR_OK;
    UINT32      ulCurrentTime = 0;
    IHXPacket*  pPacket = NULL;

    if (!m_bRedirectInSMIL)
    {
        if (NETSRC_REDIRECTSTARTED == m_state)
            m_state = NETSRC_READY;
        goto cleanup;
    }

    if (!theEvent || !(pPacket = theEvent->GetPacket()))
    {
        rc = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (NETSRC_REDIRECTFAILED == m_state ||
        NETSRC_ENDED == m_state)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }
    else if (NETSRC_REDIRECTSTARTED == m_state)
    {
        if (m_bInRetryMode && !IsRedirectedOK())
        {
            m_state = NETSRC_REDIRECTFAILED;
            rc = HXR_FAILED;
            goto cleanup;
        }

        m_state = NETSRC_READY;
        ulCurrentTime = GetCurrentPlayTime();

        if (mLiveStream || m_bRestrictedLiveStream)
        {
            m_lPacketTimeOffSet = m_ulFirstPacketTime - pPacket->GetTime() + 
                                  ulCurrentTime;
        }

        // reset renderer's packet queue as if this were seeking
        CHXMapLongToObj::Iterator i = m_pSourceInfo->m_pRendererMap->Begin();
        for (; i != m_pSourceInfo->m_pRendererMap->End(); ++i)
        {
	    RendererInfo* pRendInfo = (RendererInfo*)(*i);

	    if (pRendInfo->m_pRenderer)
	    {
                pRendInfo->m_pRenderer->OnPreSeek(ulCurrentTime, ulCurrentTime);
#if defined(HELIX_FEATURE_DRM)
            //flush the DRM pending packets
            if (IsHelixDRMProtected() && m_pDRM)
            {
                m_pDRM->FlushPackets(TRUE);
            }
#endif /* HELIX_FEATURE_DRM */
                pRendInfo->m_pRenderer->OnPostSeek(ulCurrentTime, ulCurrentTime);
	    }
        }
        HXLOGL3(HXLOG_RECO, "UpdateEvent\t%lu\t%lu\t%lu\t%lu", ulCurrentTime, m_ulFirstPacketTime, pPacket->GetTime(), m_lPacketTimeOffSet);
    }


cleanup:

    return rc;
}

void
HXNetSource::ReBuffer(UINT32 ulLoopEntryTime,
		      UINT32 ulProcessingTimeAllowance)
{
    UINT32  ulRemainToBufferInMs = 0;
    UINT32  ulRemainToBuffer = 0;

    m_pBufferManager->GetRemainToBuffer(ulRemainToBufferInMs,
                                        ulRemainToBuffer);

//    HXLOGL2(HXLOG_NSRC, "Rebuffer %p CurrentTime: %lu RemainMs: %lu RemainBytes: %lu", this,
//                  HX_GET_TICKCOUNT(), ulRemainToBufferInMs, ulRemainToBuffer);

    if (ulRemainToBufferInMs == 0 &&
        ulRemainToBuffer == 0)
    {
        m_uLastBuffering = 0;
        m_pBufferManager->ReBuffer(
            m_latencyModeHlpr.RebufferPrerollIncrement());
    }
}

HXBOOL
HXNetSource::ShouldDisableFastStart()
{
    HXBOOL bShouldDisable = FALSE;

    // no faststart on non-RTSP protocol
    if (!m_bRTSPProtocol)
    {
        HXLOGL2(HXLOG_NSRC, "(%p)Not RTSP - TurboPlay Off", this);
        m_turboPlayStats.tpOffReason = TP_OFF_BY_NONRTSP;
        bShouldDisable = TRUE;
    }

    // no faststart on live source served from >= 9.0 server which get
    // rid of the ring buffer by default
    if (mLiveStream && m_pProto && (HX_GET_MAJOR_VERSION(m_pProto->GetServerVersion()) >= 9) &&
        (TURBO_PLAY_ON != m_serverTurboPlay))
    {
        HXLOGL2(HXLOG_NSRC, "(%p)Live From Server(>=9) - TurboPlay Off", this);
        m_turboPlayStats.tpOffReason = TP_OFF_BY_LIVE;
        bShouldDisable = TRUE;
    }

    return bShouldDisable;
}

HX_RESULT
HXNetSource::UpdateRegistry(UINT32 ulRegistryID)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    HX_RESULT       theErr = HXR_OK;
    UINT32          ulRepeatedRegistryID = 0;
    UINT32          ulRegId = 0;
    char            szRegName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    IHXBuffer*      pParentName = NULL;
    IHXBuffer*      pRepeatRegName = NULL;
    STREAM_INFO*    pStreamInfo = NULL;
    SOURCE_STATS*   pNewStats = NULL;
    CHXMapLongToObj::Iterator   ndxStream;

    m_ulRegistryID = ulRegistryID;

    if (!m_pStats)
    {
        // XXX HP, Hummmmmmmm ....
        HX_ASSERT(FALSE);
        SetupRegistry();
    }
    else if (m_ulRegistryID != m_pStats->m_ulRegistryID)
    {
#if defined(HELIX_FEATURE_SMIL_REPEAT)
        // repeated source
        if (!m_pSourceInfo->m_bLeadingSource ||
             m_pSourceInfo->m_pRepeatList)
        {
            if (m_pStatsManager)
            {
                m_pStatsManager->UpdateRegistry(m_ulRegistryID);
            }
            else if (m_pRegistry &&
                     HXR_OK == m_pRegistry->GetPropName(m_pPlayer->m_ulRepeatedRegistryID, pRepeatRegName))
            {
                SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.%ld%ld%ld",
                        pRepeatRegName->GetBuffer(),
                        m_pSourceInfo->m_uGroupID,
                        m_pSourceInfo->m_uTrackID,
                        (int)m_pSourceInfo->m_bLeadingSource);

                ulRepeatedRegistryID = m_pRegistry->GetId(szRegName);
                if (!ulRepeatedRegistryID)
                {
                    ulRepeatedRegistryID = m_pRegistry->AddComp(szRegName);
                }

                m_pStatsManager = new StatsManager(m_pRegistry, m_ulRegistryID, ulRepeatedRegistryID);
                m_pStatsManager->AddRef();

                pNewStats = new SOURCE_STATS((IUnknown*)(IHXPlayer*)m_pPlayer, ulRepeatedRegistryID);
            }
            else
            {
                // why stats' creation failed??
                HX_ASSERT(FALSE);
            }
            HX_RELEASE(pRepeatRegName);
        }
        // normal source
        else
#endif /* HELIX_FEATURE_SMIL_REPEAT */
        {
            pNewStats = new SOURCE_STATS((IUnknown*)(IHXPlayer*)m_pPlayer, m_ulRegistryID);
        }

        if (pNewStats && m_pPlayer)
        {
            *pNewStats = *m_pStats;

            ndxStream = mStreamInfoTable->Begin();
            for(; ndxStream != mStreamInfoTable->End(); ++ndxStream)
            {
                pStreamInfo = (STREAM_INFO*) (*ndxStream);

                if (m_pRegistry     &&
                    pNewStats       &&
                    HXR_OK == m_pRegistry->GetPropName(pNewStats->m_ulRegistryID, pParentName))
                {
                    SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.Stream%ld", pParentName->GetBuffer(),
                            pStreamInfo->m_uStreamNumber);

                    ulRegId = m_pRegistry->GetId(szRegName);
                    if (!ulRegId)
                    {
                        ulRegId = m_pRegistry->AddComp(szRegName);
                    }

                    if(m_pProto)
                        m_pProto->UpdateRegistry(pStreamInfo->m_uStreamNumber, ulRegId);
                }
                HX_RELEASE(pParentName);
            }

            HX_DELETE(m_pStats);
            m_pStats = pNewStats;
        }
    }

    return theErr;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

void
HXNetSource::LeavePrefetch(void)
{
#if defined(HELIX_FEATURE_PREFETCH)
    m_bPrefetch = FALSE;

    if (m_pProto)
    {
        m_pProto->LeavePrefetch();
    }

    // send prefetch notification so that SMIL
    // renderer can resolve the duration on this prefetch track
    if (m_pSourceInfo)
    {
        m_pPlayer->PrefetchTrackDone(m_pSourceInfo->m_uGroupID,
                                     m_pSourceInfo->m_uTrackID,
                                     HXR_OK);
    }
#endif /* HELIX_FEATURE_PREFETCH */
    return;
}

HXBOOL HXNetSource::IsRateAdaptationUsed(void)
{
    HXBOOL bRateAdaptationUsed = FALSE;
    if (m_pProto)
    {
        bRateAdaptationUsed = m_pProto->IsRateAdaptationUsed();
    }
    return bRateAdaptationUsed ;
}

void
HXNetSource::EnterFastStart(void)
{
#if defined(HELIX_FEATURE_TURBOPLAY)
    HXLOGL2(HXLOG_NSRC, "(%p)Enter TurboPlay", this);

    m_bFastStart = TRUE;

    if (m_pProto)
    {
        m_pProto->EnterFastStart();
    }
#endif /* HELIX_FEATURE_TURBOPLAY */
    return;
}

void
HXNetSource::LeaveFastStart(TurboPlayOffReason leftReason)
{
#if defined(HELIX_FEATURE_TURBOPLAY)
    HXLOGL2(HXLOG_NSRC, "(%p)Leave TurboPlay", this);

    m_turboPlayStats.tpOffReason = leftReason;
    m_bFastStart = FALSE;

    if (m_pProto)
    {
        m_pProto->LeaveFastStart();
    }
#endif /* HELIX_FEATURE_TURBOPLAY */
    return;
}

HXBOOL
HXNetSource::IsPrefetchEnded(void)
{
#if defined(HELIX_FEATURE_PREFETCH)
    HXBOOL  bResult = FALSE;
    UINT16  uStreamDone = 0;
    UINT32  ulNumBytes = 0;
    HXBOOL  bTSNotSetYet = TRUE;
    UINT32  ulLowestTimestamp;
    UINT32  ulHighestTimestamp;

    CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
    for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
    {
        STREAM_INFO*    pStreamInfo = (STREAM_INFO*) (*lStreamIterator);
        UINT16          uStreamNumber = pStreamInfo->m_uStreamNumber;

        UINT32 ulStreamLowestTimestamp = 0;
        UINT32 ulStreamHighestTimestamp = 0;
        UINT32 ulStreamNumBytes = 0;
        HXBOOL bStreamDone = FALSE;

        GetCurrentBuffering(uStreamNumber,
                            ulStreamLowestTimestamp,
                            ulStreamHighestTimestamp,
                            ulStreamNumBytes,
                            bStreamDone);

	if (bTSNotSetYet)
	{
	    bTSNotSetYet = FALSE;
	    ulLowestTimestamp = ulStreamLowestTimestamp;
	    ulHighestTimestamp = ulStreamHighestTimestamp;
	}
	else
	{
	    if (((LONG32) (ulLowestTimestamp - ulStreamLowestTimestamp)) > 0)
	    {
		ulLowestTimestamp = ulStreamLowestTimestamp;
	    }

	    if (((LONG32) (ulHighestTimestamp - ulStreamHighestTimestamp)) < 0)
	    {
		ulHighestTimestamp = ulStreamHighestTimestamp;
	    }
	}

        ulNumBytes += ulStreamNumBytes;

        if (bStreamDone)
        {
            uStreamDone++;
        }
    }

    // prefetch done once we have cached all the data
    if (uStreamDone == mStreamInfoTable->GetCount())
    {
        bResult = TRUE;
    }
    // verify the status of prefetch based on prefetch type/value
    else
    {
        switch (m_prefetchType)
        {
        case PrefetchTime:
            if ((ulHighestTimestamp - ulLowestTimestamp) >= m_ulPrefetchValue)
            {
                bResult = TRUE;
            }
            break;
        case PrefetchTimePercent:
            // convert to PrefetchTime
            m_prefetchType = PrefetchTime;
            m_ulPrefetchValue = (UINT32)(m_ulOriginalDuration * m_ulPrefetchValue / 100.0);
            break;
        case PrefetchBytes:
            if (ulNumBytes >= m_ulPrefetchValue)
            {
                bResult = TRUE;
            }
            break;
        case PrefetchBytesPercent:
            // convert to PrefetchBytes
            m_prefetchType = PrefetchBytes;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
            m_ulPrefetchValue = (UINT32)(m_pStats->m_pClipBandwidth->GetInt() * m_ulOriginalDuration * m_ulPrefetchValue / 800.0);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
            break;
        case PrefetchBandwidth:
            // no bandwidth control for PNA
            if (!m_bRTSPProtocol)
            {
                bResult = TRUE;
            }
            else
            {
                IHXThinnableSource* pThin = NULL;
                if (m_pProto &&
                    HXR_OK == m_pProto->QueryInterface(IID_IHXThinnableSource, (void **)&pThin))
                {
                    pThin->SetDeliveryBandwidth(m_ulPrefetchValue, 0);
                }
                HX_RELEASE(pThin);

                // change the prefetch type so we only SetDeliveryBandwidth once
                m_prefetchType = PrefetchUnknown;
                m_ulPrefetchValue = 0;
            }
            break;
        case PrefetchBandwidthPercent:
            {
                // convert to PrefetchBandwidth
                m_prefetchType = PrefetchBandwidth;

                // get total bandwidth from the preferences
                UINT32 ulBandwidth = 0;
                if (HXR_OK == ReadPrefUINT32(m_pPreferences, "Bandwidth", ulBandwidth))
                {
                    m_ulPrefetchValue = (UINT32)(ulBandwidth * m_ulPrefetchValue / 100.0);
                }
            }
            break;
        case PrefetchMaxAllowedPlus1:
            // what's this???
            break;
        default:
            break;
        }
    }

    return bResult;
#else
    return TRUE;
#endif /* HELIX_FEATURE_PREFETCH */
}

HX_RESULT
HXNetSource::UpdateStatistics(void)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    ULONG32     ulSourceTotal = 0;
    ULONG32     ulSourceReceived = 0;
    ULONG32     ulSourceNormal = 0;
    UINT32      ulSourceTotal30 = 0;
    UINT32      ulSourceLost30 = 0;
    ULONG32     ulSourceRecovered = 0;
    ULONG32     ulSourceDuplicate = 0;
    ULONG32     ulSourceOutOfOrder = 0;
    ULONG32     ulSourceFilledBufferSize = 0;
    ULONG32     ulSourceLost = 0;
    ULONG32     ulSourceLate = 0;
    ULONG32     ulSourceResendRequested = 0;
    ULONG32     ulSourceResendReceived = 0;

    ULONG32     ulSourceBandwidth = 0;
    ULONG32     ulSourceCurBandwidth = 0;
    ULONG32     ulSourceAvgBandwidth = 0;

    INT32       lAvgLatency = 0;
    INT32       lHighLatency = 0;
    INT32       lLowLatency = 0xFFFF;

    IHXStatistics* pStatistics = NULL;

    if (!m_bInitialized)
    {
        return HXR_OK;
    }

    if (m_pProto && HXR_OK == m_pProto->QueryInterface(IID_IHXStatistics, (void**) &pStatistics))
    {
        pStatistics->UpdateStatistics();

        pStatistics->Release();
        pStatistics = NULL;
    }

    CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
    for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
    {
        ULONG32         ulStreamNumber = 0;
        STREAM_INFO*    pStreamInfo    = (STREAM_INFO*) (*lStreamIterator);
        STREAM_STATS*   pStreamStats   = NULL;

        if (m_pProto && HXR_OK == m_pProto->GetStreamStatistics((ULONG32) pStreamInfo->m_uStreamNumber, &pStreamStats))
        {
            if (!pStreamStats || !pStreamStats->m_bInitialized)
            {
                continue;
            }

            ulSourceTotal           += pStreamStats->m_pTotal->GetInt();
            ulSourceReceived        += pStreamStats->m_pReceived->GetInt();
            ulSourceNormal          += pStreamStats->m_pNormal->GetInt();
            ulSourceRecovered       += pStreamStats->m_pRecovered->GetInt();
            ulSourceDuplicate       += pStreamStats->m_pDuplicate->GetInt();
            ulSourceOutOfOrder      += pStreamStats->m_pOutOfOrder->GetInt();
            ulSourceFilledBufferSize += pStreamStats->m_pFilledBufferSize->GetInt();
            ulSourceLost            += pStreamStats->m_pLost->GetInt();
            ulSourceLate            += pStreamStats->m_pLate->GetInt();
            ulSourceResendRequested += pStreamStats->m_pResendRequested->GetInt();
            ulSourceResendReceived  += pStreamStats->m_pResendReceived->GetInt();

            if (m_ulLossHack > 0 && pStreamInfo->m_ulLost > 0)
            {
                ulSourceReceived    -= pStreamInfo->m_ulLost;
                ulSourceNormal      -= pStreamInfo->m_ulLost;
                ulSourceLost        += pStreamInfo->m_ulLost;

                pStreamStats->m_pReceived->SetInt((INT32) ulSourceReceived);
                pStreamStats->m_pNormal->SetInt((INT32) ulSourceNormal);
                pStreamStats->m_pLost->SetInt((INT32) ulSourceLost);
            }

            ulSourceTotal30         += pStreamStats->m_pTotal30->GetInt();
            ulSourceLost30          += pStreamStats->m_pLost30->GetInt();

            ulSourceAvgBandwidth    += pStreamStats->m_pAvgBandwidth->GetInt();
            ulSourceCurBandwidth    += pStreamStats->m_pCurBandwidth->GetInt();
            ulSourceBandwidth       += pStreamStats->m_pClipBandwidth->GetInt();

            lAvgLatency             += pStreamStats->m_pAvgLatency->GetInt();

            if (lHighLatency < pStreamStats->m_pHighLatency->GetInt())
            {
                lHighLatency = pStreamStats->m_pHighLatency->GetInt();
            }

            if (lLowLatency > pStreamStats->m_pLowLatency->GetInt())
            {
                lLowLatency = pStreamStats->m_pLowLatency->GetInt();
            }
        }
    }

    if (m_bSourceEnd)
    {
        ulSourceCurBandwidth = 0;
        ulSourceAvgBandwidth = 0;
    }

    if (m_pStats->m_pNormal)            m_pStats->m_pNormal->SetInt((INT32)ulSourceNormal);
    if (m_pStats->m_pRecovered)         m_pStats->m_pRecovered->SetInt((INT32)ulSourceRecovered);
    if (m_pStats->m_pDuplicate)         m_pStats->m_pDuplicate->SetInt((INT32)ulSourceDuplicate);
    if (m_pStats->m_pOutOfOrder)        m_pStats->m_pOutOfOrder->SetInt((INT32)ulSourceOutOfOrder);
    if (m_pStats->m_pFilledBufferSize)  m_pStats->m_pFilledBufferSize->SetInt((INT32)ulSourceFilledBufferSize);
    if (m_pStats->m_pReceived)          m_pStats->m_pReceived->SetInt((INT32)ulSourceReceived);
    if (m_pStats->m_pLost)              m_pStats->m_pLost->SetInt((INT32)ulSourceLost);
    if (m_pStats->m_pLate)              m_pStats->m_pLate->SetInt((INT32)ulSourceLate);
    if (m_pStats->m_pTotal)             m_pStats->m_pTotal->SetInt((INT32)ulSourceTotal);
    if (m_pStats->m_pTotal30)           m_pStats->m_pTotal30->SetInt((INT32)ulSourceTotal30);
    if (m_pStats->m_pLost30)            m_pStats->m_pLost30->SetInt((INT32)ulSourceLost30);
    if (m_pStats->m_pResendRequested)   m_pStats->m_pResendRequested->SetInt((INT32)ulSourceResendRequested);
    if (m_pStats->m_pResendReceived)    m_pStats->m_pResendReceived->SetInt((INT32)ulSourceResendReceived);
    if (m_pStats->m_pClipBandwidth)     m_pStats->m_pClipBandwidth->SetInt((INT32)ulSourceBandwidth);
    if (m_pStats->m_pCurBandwidth)      m_pStats->m_pCurBandwidth->SetInt((INT32)ulSourceCurBandwidth);
    if (m_pStats->m_pAvgBandwidth)      m_pStats->m_pAvgBandwidth->SetInt((INT32)ulSourceAvgBandwidth);
    if (m_pStats->m_pAvgLatency)        m_pStats->m_pAvgLatency->SetInt((INT32)lAvgLatency);
    if (m_pStats->m_pHighLatency)       m_pStats->m_pHighLatency->SetInt((INT32)lHighLatency);
    if (m_pStats->m_pLowLatency)        m_pStats->m_pLowLatency->SetInt((INT32)lLowLatency);

    // XXXHP: we should only update these info. once
    // update transport mode
    switch (m_CurrentTransport)
    {
    case UnknownMode:
        m_pStats->m_pTransportMode->SetStr("Unknown");
        break;
    case MulticastMode:
        if (helixSDPProtocol == m_uProtocolType)
        {
            m_pStats->m_pTransportMode->SetStr("Scalable Multicast");
        }
        else
        {
            m_pStats->m_pTransportMode->SetStr("Multicast");
        }
        break;
    case UDPMode:
        m_pStats->m_pTransportMode->SetStr("UDP");
        break;
    case TCPMode:
        m_pStats->m_pTransportMode->SetStr("TCP");
        break;
    case HTTPCloakMode:
        if (m_bRTSPProtocol)
        {
            m_pStats->m_pTransportMode->SetStr("RTSPvHTTP");
        }
        else
        {
            m_pStats->m_pTransportMode->SetStr("PNAvHTTP");
        }
        break;
    default:
        break;
    }

    // update buffering mode(local machine)
    if (m_pStats->m_pBufferingMode)
    {
        if (m_bForcePerfectPlay && m_bPerfectPlay)
        {
            if (m_bCannotBufferEntireClip)
            {
                m_pStats->m_pBufferingMode->SetInt(BUFFERED_PLAY_NOT_ENTIRE_CLIP);
            }
            else
            {
                m_pStats->m_pBufferingMode->SetInt(BUFFERED_PLAY);
            }
        }
        else if (m_bPerfectPlay)
        {
            if (m_bCannotBufferEntireClip)
            {
                m_pStats->m_pBufferingMode->SetInt(PERFECT_PLAY_NOT_ENTIRE_CLIP);
            }
            else
            {
                m_pStats->m_pBufferingMode->SetInt(PERFECT_PLAY);
            }
        }
        else if (m_bBufferedPlay)
        {
            m_pStats->m_pBufferingMode->SetInt(BUFFERED_PLAY);
        }
        else
        {
            m_pStats->m_pBufferingMode->SetInt(NORMAL_PLAY);
        }
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return HXR_OK;
}

HX_RESULT
HXNetSource::_ProcessIdle(HXBOOL atInterrupt,
			  UINT32 ulLoopEntryTime, 
			  UINT32 ulProcessingTimeAllowance)
{
    HX_RESULT   theErr = HXR_OK;
    UINT32      ulAlert = 0;
    const char* pszAlert = NULL;

    if (m_bLocked)
    {
        return HXR_OK;
    }

    if (HandleAudioRebuffer())
    {
        return HXR_OK;
    }

    theErr = _ProcessIdleExt(atInterrupt);
    if (HXR_ABORT == theErr)
    {
        return HXR_OK;
    }

    m_bLocked   = TRUE;

    // set m_bDelayed to FALSE as soon as it's due to begin playback
    // even it's still in pre-fetch mode or if the source has ended
    if (!m_bPaused && m_bDelayed)
    {
	if (IsPassedResumeTime(NETWORK_FUDGE_FACTOR))
	{
	    m_bDelayed = FALSE;
	}
    }

    if (m_bRedirectPending && !m_bPartOfNextGroup)
    {
        m_bRedirectPending = FALSE;
	theErr = m_pSourceInfo->HandleRedirectRequest();
        goto exit;
    }

    switch (m_state)
    {
    case NETSRC_PACREADY:
    case NETSRC_TRANSPORTREADY:
        m_state = NETSRC_READY;
        theErr = FinishSetup();
        goto exit;
        break;
    case NETSRC_PACPENDING:
    case NETSRC_TRANSPORTPENDING:
        goto exit;
        break;
    case NETSRC_REDIRECTFAILED:
        m_state = NETSRC_ENDED;
        // internal redirect is failed, try to handle redirect at the player level        
        HX_ASSERT(m_pRedirectURL);
        //theErr = m_pSourceInfo->HandleRedirectRequest((char*)m_pRedirectURL->GetURL());
        theErr = m_pSourceInfo->HandleRedirectRequest();
        goto exit;
        break;
    case NETSRC_ENDED:
        goto exit;
        break;
    default:
        break;
    }

    if (!m_pProto)
    {
        goto exit;
    }

#ifdef _MACINTOSH
    if (m_bBruteForceConnectToBeDone)
    {
        HXBOOL bAtInterrupt = FALSE;
        IHXInterruptState* pInterruptState = NULL;
        if (m_pEngine &&
            m_pEngine->QueryInterface(IID_IHXInterruptState, (void**) &pInterruptState) == HXR_OK)
        {
            bAtInterrupt = pInterruptState->AtInterruptTime();
            HX_RELEASE(pInterruptState);
        }

        if (!bAtInterrupt)
        {
            m_bBruteForceConnectToBeDone = FALSE;
            theErr = handleTransportSwitch();
        }

        goto exit;
    }
#endif

    if (m_bBruteForceReconnected && m_bInitialized)
    {
        /* If user has not called DoResume yet, do not call it here */
        if (m_bUserHasCalledResume)
        {
            if (CanBeResumed())
            {
                if (m_pSourceInfo)
                {
                    m_pSourceInfo->Register();
                }

                if (m_pPlayer)
                {
                    m_pPlayer->RegisterSourcesDone();
                }
                DoResume();
            }
        }
        else if (m_bUserHasCalledStartInit)
        {
            StartInitialization();
        }

        m_bBruteForceReconnected = FALSE;
    }

    if (atInterrupt)
    {
        m_bAtInterrupt = TRUE;
        theErr = m_pProto->process_idle(TRUE);
        m_bAtInterrupt = FALSE;
    }
    else
    {
        theErr = m_pProto->process_idle(FALSE);

        if (!theErr && mLastError)
        {
            theErr = mLastError;
        }

        if (m_bInitialized &&
            m_bPerfectPlay &&
            !m_bPerfectPlayErrorChecked)
        {
            m_bPerfectPlayErrorChecked = TRUE;

            if(!m_bServerHasPerfectPlay)
            {
                theErr = HXR_PERFECTPLAY_NOT_SUPPORTED;
            }
            else
            {
                // Check for attempt to use perfect play on live stream...
                if (mLiveStream)
                {
                    theErr = HXR_NO_LIVE_PERFECTPLAY;
                }

                // we will not allow perfect play if perfect play bit is NOT set.
                if(!m_bPerfectPlayAllowed)
                {
                    theErr = HXR_PERFECTPLAY_NOT_ALLOWED;
                }
            }
        }

        if (theErr == HXR_UNEXPECTED_STREAM_END)
        {
            theErr = AttemptReconnect();
            mLastError = theErr;
        }

        // handle HXR_REDIRECTION explicitly
        if (theErr == HXR_REDIRECTION)
        {
            mLastError = theErr = AttemptRedirect();
        }
        else if (!m_bReceivedData && theErr)
        {
            // NOTE: Handle automatic transport switching here...
            switch(theErr)
            {
            case HXR_NET_CONNECT:
            case HXR_SERVER_DISCONNECTED:
            case HXR_SOCK_CONNRESET:
                if (HTTPCloakMode == m_CurrentTransport)
                {
                    HX_RESULT   tempErr = handleProxySwitch(theErr);
                    if(tempErr!=HXR_NOTIMPL)
                    {
                        theErr = tempErr;
                    }
                }

                if (HXR_OK != theErr)
                {
                    theErr = switch_to_next_transport(theErr);
                }
                break;

            case HXR_MULTICAST_UDP:
            case HXR_NET_UDP:
            case HXR_NET_TCP:
            case HXR_BAD_TRANSPORT:
                theErr = switch_to_next_transport(theErr);
                break;

            case HXR_PERFECTPLAY_NOT_SUPPORTED:
            case HXR_NO_LIVE_PERFECTPLAY:
            case HXR_PERFECTPLAY_NOT_ALLOWED:
                theErr = switch_out_of_perfectplay();
                break;

            case HXR_FORCE_PERFECTPLAY:
                // if we have connected to an old server and we wanted PerfectPlay, we
                // we need to reconnect to the server using the Protocol 8 PerfectPlay
                // mechanism to ensure that we get the data via UDP
                m_bPerfectPlay      = TRUE;

                /* If the server is forcing us into PerfectPlay, it means
                 * it does allow it AND the clip is perfectPlay-able.
                 */
                m_bPerfectPlayAllowed   = TRUE;
                m_bServerHasPerfectPlay = TRUE;

                WritePerfectPlayToRegistry();

                theErr = switch_to_next_transport(HXR_OK);
                break;

            default:
                break;
            };

            if (!m_bRTSPProtocol                            &&
                HTTPCloakMode == m_CurrentTransport         &&
                m_pCloakPortList                            &&
                m_nCurrPortIdx < m_nNumberOfCloakPorts-1    &&
                (HXR_NET_CONNECT == theErr                  ||
                 HXR_DNR == theErr                          ||
                 HXR_SERVER_DISCONNECTED == theErr          ||
                 HXR_DOC_MISSING == theErr                  ||
                 HXR_BAD_SERVER == theErr                   ||
                 HXR_PROXY_NET_CONNECT == theErr))
            {
                m_PreferredTransport = HTTPCloakMode;
                m_uCurrCloakedPort = m_pCloakPortList[++m_nCurrPortIdx];
                theErr = handleTransportSwitch();
                mLastError = theErr;
            }
        }
    }

    if (theErr)
    {
        if (theErr == HXR_NET_CONNECT && m_bUseProxy)
        {
            mLastError = HXR_PROXY_NET_CONNECT;
        }
        else if (theErr == HXR_NET_CONNECT && HTTPCloakMode == m_CurrentTransport)
        {
            // we mask to HTTP connection error only if we are in HTTP only mode
            if (m_ulTransportPrefMask == ATTEMPT_HTTPCLOAK)
            {
                mLastError = HXR_HTTP_CONNECT;
            }
            else
            {
                mLastError = theErr;
            }
        }
        else if (theErr == HXR_DNR && m_bUseProxy)
        {
            mLastError = HXR_PROXY_DNR;
        }
        else
        {
            mLastError = theErr;
        }
    }

    if (m_bReceivedData && HXR_OK != mLastError)
    {
        if (m_state == NETSRC_RECONNECTSTARTED)
        {
            HXLOGL2(HXLOG_RECO, "(%p)Reconnect Failed", this);
        }
        // we are reconnecting now
        else if (m_state == NETSRC_RECONNECTPENDING)
        {
            mLastError = theErr = HXR_OK;
        }
        // issue reconnect on the following network errors
        else
        {
            if (HXR_SOCK_CONNRESET == mLastError        ||
                HXR_SERVER_DISCONNECTED == mLastError   ||
                HXR_NET_SOCKET_INVALID == mLastError    ||
                HXR_GENERAL_NONET == mLastError         ||
                IS_SERVER_ALERT(mLastError))
            {
                HX_ASSERT(m_state == NETSRC_READY);

                if (m_bSourceEnd)
                {
                    mLastError = theErr = HXR_OK;
                }
                else if (IsNetworkAvailable() && m_bAttemptReconnect)
                {
                    if (IS_SERVER_ALERT(mLastError))
                    {
                        pszAlert = m_pProto->GetLastAlertInfo(ulAlert);
                        if (PE_PROXY_ORIGIN_DISCONNECTED == ulAlert)
                        {
                            theErr = AttemptReconnect();
                            mLastError = theErr;
                        }
                    }
                    else
                    {
                        theErr = AttemptReconnect();
                        mLastError = theErr;
                    }
                }
            }
        }
    }

    if (NETSRC_ENDPENDING == m_state)
    {
        handleEndOfSource();
    }

    /* tell the player about the error...
     * This is crucial...
     */
    if (m_pPlayer && mLastError)
    {
        ActualReportError(mLastError);
    }

    if (!theErr && m_bInitialized)
    {
        // Check to see if we have gone over our latency threshold
        // and take action if necessary.
        //
        if (EnforceLiveLowLatency())
        {
            enforceLatencyThreshold();
        }

        UINT32 ulCurrentTime = m_pPlayer->GetInternalCurrentPlayTime();

	if (m_bPaused && m_bDelayed)
	{
	    if (TryResume() && m_pPlayer)
	    {
		m_pPlayer->RegisterSourcesDone();
		DoResume();
	    }
	}

#if defined(HELIX_FEATURE_PREFETCH)
        // verify the status of prefetch
        if (m_bPrefetch)
        {
            HXBOOL bPrefetchDone = IsPrefetchEnded();
            // stop prefetch when either the prefetch has been done or
            // the active source itself started playback
            if (bPrefetchDone ||
                (!m_bDelayed && !m_bPartOfPrefetchGroup))
            {
                LeavePrefetch();

                // pause delayed source
                if (bPrefetchDone && m_bDelayed)
                {
                    DoPause();
                }
            }
        }
#endif /* HELIX_FEATURE_PREFETCH */

        if  (!m_bSourceEnd)
        {
            /* Get Current buffering status every 1 sec. */
            UINT32 ulCurrentSystemTime = HX_GET_TICKCOUNT();
            if (CALCULATE_ELAPSED_TICKS(m_ulLastBufferingCalcTime,
                                        ulCurrentSystemTime) > 1000)
            {
                m_ulLastBufferingCalcTime = ulCurrentSystemTime;
                CalculateCurrentBuffering();
            }
        }

        CheckForInitialPrerollRebuffer(ulCurrentTime);
    }

    // If we are playing from record control and we are
    // forward accelerated, then it's possible for accelerated
    // playback to overtake the write point. Here we detect
    // if we are about to overtake the write point. If we are,
    // then go back to normal velocity.
    if (m_bPlayFromRecordControl && m_pRecordControl &&
        m_pPlayer->GetVelocity() > HX_PLAYBACK_VELOCITY_NORMAL &&
        !m_pRecordControl->IsFinishedWriting() &&
        !m_bDisableRecordSourceOverrunProtection)
    {
        UINT32 ulPlayTime  = m_pPlayer->GetCurrentPlayTime();
        UINT32 ulWriteTime = m_pRecordControl->GetLatestTimestampWritten();
        if (ulWriteTime > m_ulRecordSourceOverrunProtectionTime &&
            ulPlayTime  > (ulWriteTime - m_ulRecordSourceOverrunProtectionTime))
        {
            HXLOGL2(HXLOG_TRIK, "Playback time (%lu) getting close to record "
                    "control write time (%lu), resetting velocity",
                    ulPlayTime, ulWriteTime);
            // We are getting close to overrun, so reset velocity
            m_pPlayer->SetVelocity(100, FALSE, FALSE);
        }
    }
exit:

    if (!theErr && !m_bIsActive && !m_bDelayed && m_pPlayer &&
        m_bInitialized && m_pPlayer->GetInternalCurrentPlayTime() >= m_ulDelay)
    {
        AdjustClipBandwidthStats(TRUE);
    }

    m_bLocked = FALSE;
    return(theErr);
}

HX_RESULT
HXNetSource::_ProcessIdleExt(HXBOOL atInterrupt)
{
    return HXR_OK;
}

// set the various parameters that are used by all the protocols
HX_RESULT
HXNetSource::InitializeProtocol()
{
    HXLOGL3(HXLOG_NSRC, "HXNetSource::InitializeProtocol");

    HX_RESULT       theErr = HXR_OK;

    // set the client ID
    theErr = m_pProto->set_client_id( mClientID );

    if (HXR_OK != theErr)
    {
        goto cleanup;
    }

    // If we reach to an unknown mode during transport switching, then
    // we are all out of possibilities, and we can consider the server
    // connection timed out...
    if (m_PreferredTransport == UnknownMode)
    {
        theErr = HXR_SERVER_TIMEOUT;
        goto cleanup;
    }
    else
    {
        set_transport(m_PreferredTransport);
    }

    // If we were asked to use a specific UDP Port. Then
    // tell the protocol object about the request...
    if (m_bUseUDPPort)
    {
        m_pProto->set_UDP_port();
    }

    if (m_bUseProxy)
    {
        theErr = m_pProto->set_proxy(m_pProxy,m_uProxyPort);

        if (HXR_OK != theErr)
        {
            goto cleanup;
        }
    }

    if (HTTPCloakMode == m_CurrentTransport &&
        PTS_READY != m_prefTransportState)
    {
        CreateCloakedPortList();
        m_pProto->SetCloakPortAttempted(m_pCloakPortList, m_nNumberOfCloakPorts);
    }

#if defined(HELIX_FEATURE_PREFETCH)
    if (m_bPrefetch)
    {
        m_pProto->EnterPrefetch();
    }
#endif /* HELIX_FEATURE_PREFETCH */

    m_pProto->set_server_timeout(m_ulServerTimeOut);
    m_pProto->set_perfect_play(m_bPerfectPlay);

cleanup:

    return theErr;
}

void
HXNetSource::set_transport(TransportMode mode)
{
    HXLOGL2(HXLOG_NSRC, "(%p)set_transport %lu", this, mode);
    m_CurrentTransport = mode;

    // Tell the protocol object, (in case it doesn't remember <g>).
    if(m_pProto)
        m_pProto->set_transport(m_CurrentTransport, m_ulTransportPrefMask);
}

// sets up HXSource to use a RealMedia splitter
HX_RESULT
HXNetSource::set_proxy(const char* proxy, UINT16 port)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::set_proxy");

    HX_RESULT           theErr = HXR_OK;
    IHXProxyManager*    pProxyManager = NULL;

    if(proxy == 0 || *proxy == 0)
    {
        theErr = HXR_OK;
        goto cleanup;
    }

    if(m_pProxy)
    {
        delete [] m_pProxy;
        m_pProxy = NULL;
    }

    m_pProxy = new char[::strlen(proxy) + 1];
    if(m_pProxy == NULL)
    {
        theErr = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    ::strcpy(m_pProxy, proxy); /* Flawfinder: ignore */
    m_uProxyPort = port;
    m_bUseProxy = TRUE;

    if (m_pPlayer &&
        HXR_OK == m_pPlayer->QueryInterface(IID_IHXProxyManager, (void**)&pProxyManager) &&
        pProxyManager)
    {
        m_bUseProxy = !(pProxyManager->IsExemptionHost(m_pHost));
    }
    HX_RELEASE(pProxyManager);

cleanup:

    return(theErr);
}

HX_RESULT
HXNetSource::cleanup_proxy(void)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::cleanup_proxy");
    HX_RESULT       theErr = HXR_OK;

    m_uProxyPort = 0;
    HX_VECTOR_DELETE(m_pProxy);

    m_bUseProxy = FALSE;

    return(theErr);
}

HX_RESULT
HXNetSource::ReadPreferences()
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::ReadPreferences");

    HX_RESULT       theErr = HXR_OK;
    UINT32          un16Temp = 0;
    IHXValues*      pRequestHeaders = NULL;
    IHXBuffer*      pValue = NULL;

    theErr = HXSource::ReadPreferences();
    if (theErr)
    {
            return theErr;
    }

    // Read Client ID
    IHXBuffer* pProxyHost = NULL;
    IHXBuffer* pProxyPort = NULL;

    /* We only want to read PerfectPlay preference once.
     * This is because we may disable PerfectPlay if the server
     * does not allow it OR if the content is not "PerfectPlay"able.
     * In each of these cases, we call brute_force_reconnect()
     * and we do not want to over-write m_bPerfectPlay
     */
    if (!m_bPerfectPlayPreferenceRead)
    {
        m_bPerfectPlayPreferenceRead = TRUE;
        ReadPrefBOOL(m_pPreferences, "PerfectPlay", m_bPerfectPlay);         
    }

    if (m_pRequest &&
        SUCCEEDED(m_pRequest->GetRequestHeaders(pRequestHeaders)))
    {
        pRequestHeaders->GetPropertyCString("ClientID", pValue);
    }

    if (pValue)
    {
        const char* pcszClientID = (const char*)pValue->GetBuffer();

        if (pcszClientID)
        {
            UINT32 ulSizeof_mClientID = sizeof(mClientID);
            UINT32 ulSafeStrlenClientId = HX_SAFESIZE_T(strlen(pcszClientID));

            HX_ASSERT(ulSafeStrlenClientId < ulSizeof_mClientID);

            if (ulSafeStrlenClientId >= ulSizeof_mClientID)
            {
                // /Limiting too-large string fixes PR 86928 (but whatever
                // writes to props|prefs needs to be sure not to overflow
                // the max size):
                ulSafeStrlenClientId = ulSizeof_mClientID - 1;
            }
            
            memcpy(mClientID, pcszClientID, ulSafeStrlenClientId); /* Flawfinder: ignore */
            mClientID[ulSafeStrlenClientId] = '\0';
        }
    }
    else
    {
        // ClientID is expected to be set by ::SetRequest() if it's not
        // overwritten by the application
        HX_ASSERT(FALSE);
    }

    HX_RELEASE(pValue);
    HX_RELEASE(pRequestHeaders);

    ReadPrefUINT32(m_pPreferences, "ServerTimeOut", m_ulServerTimeOut);
    if (m_ulServerTimeOut < MINIMUM_TIMEOUT)
    {
        m_ulServerTimeOut = MINIMUM_TIMEOUT;
    }

    ReadPrefUINT32(m_pPreferences, "ConnectionTimeOut", m_ulConnectionTimeout);
    if (m_ulConnectionTimeout < MINIMUM_TIMEOUT)
    {
        m_ulConnectionTimeout = MINIMUM_TIMEOUT;
    }

    ReadPrefUINT32(m_pPreferences, "MulticastTimeout", m_ulMulticastTimeout);
    ReadPrefUINT32(m_pPreferences, "UDPTimeout", m_ulUDPTimeout);
    ReadPrefUINT32(m_pPreferences, "TCPTimeout", m_ulTCPTimeout);
    
    ReadPrefBOOL(m_pPreferences, "SendStatistics", m_bSendStatistics);
    
    /////////////////////////////////////////////////////////////
    //
    // Handle Specific UDP Port Preferences here....
    //
    ReadPrefBOOL(m_pPreferences, "UseUDPPort", m_bUseUDPPort);

#if defined(HELIX_FEATURE_SMARTERNETWORK)
    if (!m_pPreferredTransport)
    {
        m_pPreferredTransportManager->GetTransportPreference(m_bRTSPProtocol?PTP_RTSP:PTP_PNM, m_ulTransportPrefMask);
        HX_ASSERT(m_ulTransportPrefMask);

        m_pPreferredTransportManager->GetPrefTransport(m_pHost,
                                                       m_bRTSPProtocol?PTP_RTSP:PTP_PNM,
                                                       m_pPreferredTransport);
        HX_ASSERT(m_pPreferredTransport);
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    if (!m_bTransportPrefFromURLAttempted)
    {
        HX_ASSERT(UnknownMode == m_PreferredTransport);
        m_bTransportPrefFromURLAttempted = TRUE;
        
        m_PreferredTransport = GetTransportFromURL(m_pszURL);
    }

    if (UnknownMode == m_PreferredTransport)
    {
#if defined(HELIX_FEATURE_SMARTERNETWORK)
        m_prefTransportState = m_pPreferredTransport->GetState();

        if (m_prefTransportState == PTS_READY ||
            m_prefTransportState == PTS_CREATE)
        {
            m_pPreferredTransport->GetTransport(m_PreferredTransport,
                                                m_uCurrCloakedPort);

#if !defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
            if (MulticastMode == m_PreferredTransport)
            {
                m_PreferredTransport = UDPMode;
            }
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
        }
        else if (m_prefTransportState == PTS_PENDING)
        {
            m_state = NETSRC_TRANSPORTPENDING;
            m_pPreferredTransport->AddTransportSink(this);
            theErr = HXR_WOULD_BLOCK;
        }
#else

#if defined(HELIX_FEATURE_TCP_OVER_UDP)
        // default to TCP
        m_PreferredTransport = TCPMode;
#else
        // default to UDP
        m_PreferredTransport = UDPMode;
#endif /* HELIX_FEATURE_TCP_OVER_UDP */


#endif /* HELIX_FEATURE_SMARTERNETWORK */
    }

    // if the "Use Proxy" is explicitly set, we will use proxy settings accordingly
    // regardless what server type(internal vs external) it is.
    //if (m_pPreferredTransport->GetClass() == PTC_EXTERNAL)
    {
        // handle proxies
        if (HTTPCloakMode == m_PreferredTransport)
        {
            un16Temp = 0;

#if defined(HELIX_FEATURE_PAC)
            if (HXR_OK != ReadPrefUINT32(m_pPreferences, "HTTPProxyAutoConfig", un16Temp))
            {
                // previously released Enterprise player may use "ProxyAutoConfig" for
                // HTTP proxy auto config
                ReadPrefUINT32(m_pPreferences, "ProxyAutoConfig", un16Temp);
            }

            // HTTP Proxy Auto Config
            if (un16Temp)
            {
                if (!m_pPAC)
                {
                    m_pEngine->QueryInterface(IID_IHXProxyAutoConfig, (void**)&m_pPAC);
                }

                if (m_pPAC &&
                    (!m_pPACInfoList || 0 == m_pPACInfoList->GetCount()))
                {
                    theErr = m_pPAC->GetHTTPProxyInfo((IHXProxyAutoConfigCallback*)this,
                                                      m_pszURL,
                                                      m_pHost);
                }
                // attempt the next proxy info from m_pPACInfoList
                else if (m_pPACInfoList && m_PACInfoPosition)
                {
                    PACInfo* pPACInfo = (PACInfo*)m_pPACInfoList->GetNext(m_PACInfoPosition);
                    if (pPACInfo && pPACInfo->type != PAC_DIRECT)
                    {
                        HXLOGL2(HXLOG_NSRC, "(%p)PAC: %s %lu", this, pPACInfo->pszHost, pPACInfo->ulPort);
                        set_proxy(pPACInfo->pszHost, (UINT16)pPACInfo->ulPort);
                    }
                    else if (pPACInfo)
                    {
                        HXLOGL2(HXLOG_NSRC, "(%p)PAC: DIRECT", this);
                    }
                }

                if (HXR_WOULD_BLOCK == theErr)
                {
                    m_state = NETSRC_PACPENDING;
                }
            }
            else
#endif /* HELIX_FEATURE_PAC */
            {
                if (HXR_OK == ReadPrefUINT32(m_pPreferences, "HTTPProxySupport", un16Temp))
                {
                    if (un16Temp)
                    {
                        if (m_pszReconnectProxy)
                        {
                            set_proxy(m_pszReconnectProxy, (UINT16)m_ulReconnectProxyPort);
                        }
                        else if (m_pPreferences && m_pPreferences->ReadPref("HTTPProxyHost", pProxyHost) == HXR_OK &&
                                 m_pPreferences->ReadPref("HTTPProxyPort", pProxyPort) == HXR_OK)
                        {
                            set_proxy((const char*)pProxyHost->GetBuffer(), atoi((const char*)pProxyPort->GetBuffer()));
                        }
                        HX_RELEASE(pProxyHost);
                        HX_RELEASE(pProxyPort);
                    }
                }
            }
        }
        else
        {
#if defined(HELIX_FEATURE_PAC)
            // RTSP/PNM Proxy Auto Config
            if (HXR_OK == ReadPrefUINT32(m_pPreferences, "RTSPPNMProxyAutoConfig", un16Temp) && un16Temp)
            {
                if (!m_pPAC)
                {
                    m_pEngine->QueryInterface(IID_IHXProxyAutoConfig, (void**)&m_pPAC);
                }

                if (m_pPAC &&
                    (!m_pPACInfoList || 0 == m_pPACInfoList->GetCount()))
                {
                    theErr = m_pPAC->GetRTSPPNMProxyInfo((IHXProxyAutoConfigCallback*)this,
                                                         m_pszURL,
                                                         m_pHost);
                }
                // attempt the next proxy info from m_pPACInfoList
                else if (m_pPACInfoList && m_PACInfoPosition)
                {
                    PACInfo* pPACInfo = (PACInfo*)m_pPACInfoList->GetNext(m_PACInfoPosition);
                    if (pPACInfo && pPACInfo->type != PAC_DIRECT)
                    {
                        HXLOGL2(HXLOG_NSRC, "(%p)PAC: %s %lu", this, pPACInfo->pszHost, pPACInfo->ulPort);
                        set_proxy(pPACInfo->pszHost, (UINT16)pPACInfo->ulPort);
                    }
                    else if (pPACInfo)
                    {
                        HXLOGL2(HXLOG_NSRC, "(%p)PAC: DIRECT", this);
                    }
                }

                if (HXR_WOULD_BLOCK == theErr)
                {
                    m_state = NETSRC_PACPENDING;
                }
            }
            else
#endif /* HELIX_FEATURE_PAC */
            {
                // RTSP proxy
                if (m_bRTSPProtocol)
                {
                    if (HXR_OK == ReadPrefUINT32(m_pPreferences, "RTSPProxySupport", un16Temp) && un16Temp)
                    {
                        if (m_pszReconnectProxy)
                        {
                            set_proxy(m_pszReconnectProxy, (UINT16)m_ulReconnectProxyPort);
                        }
                        else if (m_pPreferences && m_pPreferences->ReadPref("RTSPProxyHost", pProxyHost) == HXR_OK &&
                                 m_pPreferences->ReadPref("RTSPProxyPort", pProxyPort) == HXR_OK)
                        {
                            set_proxy((const char*)pProxyHost->GetBuffer(), atoi((const char*)pProxyPort->GetBuffer()));
                        }
                        HX_RELEASE(pProxyHost);
                        HX_RELEASE(pProxyPort);
                    }
                }
#if defined(HELIX_FEATURE_PNA)
                // PNA proxy
                else
                {
                    if (HXR_OK == ReadPrefUINT32(m_pPreferences, "PNAProxySupport", un16Temp) && un16Temp)
                    {
                        if (m_pszReconnectProxy)
                        {
                            set_proxy(m_pszReconnectProxy, (UINT16) m_ulReconnectProxyPort);
                        }
                        else if (m_pPreferences && m_pPreferences->ReadPref("PNAProxyHost", pProxyHost) == HXR_OK &&
                                 m_pPreferences->ReadPref("PNAProxyPort", pProxyPort) == HXR_OK)
                        {
                            set_proxy((const char*)pProxyHost->GetBuffer(), atoi((const char*)pProxyPort->GetBuffer()));
                        }
                        HX_RELEASE(pProxyHost);
                        HX_RELEASE(pProxyPort);
                    }
                }
#endif /* defined(HELIX_FEATURE_PNA)*/
            }
        }
    }

    // Read the acceleration overrun guard time
    ReadPrefUINT32(m_pPreferences,
                   "PlaybackVelocity\\RecordSourceOverrunProtectionTime",
                   m_ulRecordSourceOverrunProtectionTime);
    // Read the overrun protection flag
    ReadPrefBOOL(m_pPreferences,
                 "PlaybackVelocity\\DisableRecordSourceOverrunProtection",
                 m_bDisableRecordSourceOverrunProtection);

    return theErr;
}

HX_RESULT
HXNetSource::CreateProtocol()
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::CreateProtocol");

    HX_RESULT theErr = HXR_OK;

    // delete the earlier protocol if one exists
    HX_RELEASE(m_pProto);

    if (m_bRTSPProtocol)
    {
        m_pProto = new RTSPProtocol(this, 0);
    }
    else
    {
#if defined(HELIX_FEATURE_PNA)
        m_pProto = new PNAProtocol(this, 0);
#else
        theErr = HXR_INVALID_PROTOCOL;
#endif /* HELIX_FEATIRE_PNA */
    }

    if (HXR_OK == theErr)
    {
        if (m_pProto == NULL)
        {
            theErr = HXR_OUTOFMEMORY;
        }
        else
        {
            HX_ADDREF(m_pProto);
        }
    }

    // set the proxy
    if (!theErr && m_bUseProxy)
    {
        theErr = m_pProto->set_proxy(m_pProxy, m_uProxyPort);
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    IHXStatistics*      pStatistics = NULL;
    if (!theErr && HXR_OK == m_pProto->QueryInterface(IID_IHXStatistics, (void**) &pStatistics))
    {
        if (m_pStats)
        {
            pStatistics->InitializeStatistics(m_pStats->m_ulRegistryID);
        }

        HX_RELEASE(pStatistics);
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return theErr;
}

HXBOOL
HXNetSource::CheckTransportTimeout(ULONG32 ulTime)
{
    HXBOOL    bTimedOut = FALSE;
    ULONG32 ulTimeOut = 0;

    if (!m_bDataWaitStarted)
    {
        return FALSE;
    }

    ULONG32 elapsed = CALCULATE_ELAPSED_TICKS(m_ulStartDataWait,ulTime);

    if (m_bConnectionWait)
    {
        ulTimeOut = m_ulConnectionTimeout * MILLISECS_PER_SECOND;
    }
    else
    {
        switch (m_CurrentTransport)
        {
        case MulticastMode:
            ulTimeOut = m_ulMulticastTimeout;
            break;
        case UDPMode:
            ulTimeOut = m_ulUDPTimeout;
            break;
        case TCPMode:
            ulTimeOut = m_ulTCPTimeout;
            break;
        default:
            ulTimeOut = m_ulServerTimeOut * MILLISECS_PER_SECOND;
            break;
        }
    }

    if (elapsed > ulTimeOut)
    {
        bTimedOut = TRUE;
    }

    return bTimedOut;
}

HX_RESULT
HXNetSource::HandleRetry(const char* pszHost, UINT16 ulPort)
{
    HX_RESULT theErr = HXR_OK;

    if (!pszHost)
    {
        theErr = HXR_FAILED;
        goto cleanup;
    }

    HX_VECTOR_DELETE(m_pHost);

    m_pHost = new char[strlen(pszHost) + 1];
    strcpy(m_pHost, pszHost); /* Flawfinder: ignore */

    m_uPort = ulPort;

cleanup:

    return theErr;
}

HX_RESULT
HXNetSource::SetRedirectURL(const char* pHost, UINT16 nPort, const char* pPath, CHXURL* pURL)
{
    HXLOGL3(HXLOG_NSRC, "HXNetSource[%p]::SetRedirectURL(): %s:%u", this, pHost, nPort);
    HX_RESULT   theErr = HXR_OK;

    if (!pHost || !pPath || !pURL)
    {
        theErr = HXR_FAILED;
        goto cleanup;
    }

    HX_VECTOR_DELETE(m_pszRedirectServer);
    HX_VECTOR_DELETE(m_pszRedirectResource);
    HX_DELETE(m_pRedirectURL);

    m_pszRedirectServer = new char[strlen(pHost) + 1];
    strcpy(m_pszRedirectServer, pHost); /* Flawfinder: ignore */

    m_pszRedirectResource = new char[strlen(pPath) + 1];
    strcpy(m_pszRedirectResource, pPath); /* Flawfinder: ignore */

    m_ulRedirectServerPort = nPort;

    m_pRedirectURL = new CHXURL(*pURL);

    m_state = NETSRC_REDIRECTPENDING;

cleanup:

    return theErr;
}

HX_RESULT
HXNetSource::SetReconnectInfo(IHXValues* pValues)
{
    HX_RESULT   rc = HXR_OK;
    UINT32      ulValue = 0;
    IHXBuffer*  pServer = NULL;

    if (HXR_OK == pValues->GetPropertyULONG32("Reconnect", ulValue) &&
        0 == ulValue)
    {
        HX_VECTOR_DELETE(m_pszReconnectServer);
        HX_VECTOR_DELETE(m_pszReconnectProxy);
        HX_VECTOR_DELETE(m_pszReconnectURL);

        m_bAttemptReconnect = FALSE;
        goto cleanup;
    }

    m_bAttemptReconnect = TRUE;

    if (HXR_OK == pValues->GetPropertyCString("Alternate-Server", pServer))
    {
        HX_VECTOR_DELETE(m_pszReconnectServer);
        HX_VECTOR_DELETE(m_pszReconnectURL);

        m_pszReconnectServer = new char[pServer->GetSize() + 1];
        ::strcpy(m_pszReconnectServer, (const char*)pServer->GetBuffer()); /* Flawfinder: ignore */

        pValues->GetPropertyULONG32("Alternate-ServerPort", m_ulReconnectServerPort);

        m_pszReconnectURL = new char[pServer->GetSize() + strlen(m_pResource) + 32];
        if (m_bRTSPProtocol)
        {
            SafeSprintf(m_pszReconnectURL,pServer->GetSize() + strlen(m_pResource) + 32, "rtsp://%s:%u/%s", pServer->GetBuffer(), m_ulReconnectServerPort, m_pResource); /* Flawfinder: ignore */
        }
        else
        {
            SafeSprintf(m_pszReconnectURL, pServer->GetSize() + strlen(m_pResource) + 32, "pnm://%s:%u/%s", pServer->GetBuffer(), m_ulReconnectServerPort, m_pResource); /* Flawfinder: ignore */
        }
    }
    HX_RELEASE(pServer);

    if (HXR_OK == pValues->GetPropertyCString("Alternate-Proxy", pServer))
    {
        HX_VECTOR_DELETE(m_pszReconnectProxy);

        m_pszReconnectProxy = new char[pServer->GetSize() + 1];
        ::strcpy(m_pszReconnectProxy, (const char*)pServer->GetBuffer()); /* Flawfinder: ignore */

        pValues->GetPropertyULONG32("Alternate-ProxyPort", m_ulReconnectProxyPort);
    }
    HX_RELEASE(pServer);

cleanup:

    return rc;
}

void
HXNetSource::StartDataWait(HXBOOL bConnectionWait)
{
    m_ulStartDataWait   = HX_GET_TICKCOUNT();
    m_bDataWaitStarted  = TRUE;
    m_bConnectionWait   = bConnectionWait;
    m_turboPlayStats.bBufferDone = FALSE;
}

void
HXNetSource::StopDataWait()
{
    m_ulStartDataWait   = 0;
    m_bDataWaitStarted  = FALSE;
}

HX_RESULT
HXNetSource::FileHeaderReady(IHXValues* pHeader)
{
    HX_RESULT retVal;
    HXBOOL bSkipProcessingHeader = FALSE;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (!pHeader || !m_pStats || m_bInRetryMode)
#else
    if (!pHeader || m_bInRetryMode)
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
    {
	bSkipProcessingHeader = TRUE;
    }

    HX_RELEASE(m_pFileHeader);
    m_pFileHeader = pHeader;
    m_pFileHeader->AddRef();

    m_latencyModeHlpr.OnFileHeader(pHeader);

#if defined(HELIX_FEATURE_DRM)
    if (!bSkipProcessingHeader)
    {
	m_bIsProtected = IsHelixDRMProtected(pHeader);
	if (IsHelixDRMProtected())
	{
	    retVal = InitHelixDRM(pHeader);

	    if (SUCCEEDED(retVal) && m_pDRM)
	    {
		m_pDRM->FileHeaderHook(pHeader);
	    }
	}
    }
#endif /*HELIX_FEATURE_DRM*/

#if defined(HELIX_FEATURE_RECORDCONTROL)
    if (!bSkipProcessingHeader)
    {
	SendHeaderToRecordControl(TRUE, pHeader);
    }
#endif /* HELIX_FEATURE_RECORDCONTROL */

    if (bSkipProcessingHeader)
    {
        return HXR_OK;
    }

    retVal = PreFileHeaderReadyExt(pHeader);
    if (retVal == HXR_REQUEST_UPGRADE)
    {
        mLastError = retVal;
        return HXR_OK;
    }
    else if (retVal == HXR_WOULD_BLOCK)
    {
        return HXR_OK;
    }

    m_bContinueWithHeaders = FALSE;

#if defined(HELIX_FEATURE_DRM)
    if (IsHelixDRMProtected() && m_pDRM)
    {
        //DRM OnFileHeader callback will call ProcessFileHeader later
	return m_pDRM->ProcessFileHeader(pHeader);
    }
#endif /*HELIX_FEATURE_DRM*/

    return ProcessFileHeader();
}

HX_RESULT
HXNetSource::ProcessFileHeader()
{
    HXSource::ProcessFileHeader();

    PostFileHeaderReadyExt();

    return HXR_OK;
}

HX_RESULT
HXNetSource::PreFileHeaderReadyExt(IHXValues* pHeader)
{
    return HXR_OK;
}

HX_RESULT
HXNetSource::PostFileHeaderReadyExt(void)
{
    return HXR_OK;
}

HX_RESULT
HXNetSource::TACInfoFromHeader(IHXValues* pValues)
{
    // remove this method
    return HXR_OK;
}

HX_RESULT
HXNetSource::HeaderCallback(IHXValues* theHeader)
{
    HX_RESULT           theErr = HXR_OK;
    STREAM_INFO*        pStreamInfo = NULL;

#if defined(HELIX_FEATURE_DRM)
    if (SUCCEEDED(theErr) && IsHelixDRMProtected() && m_pDRM)
    {
        theErr = m_pDRM->StreamHeaderHook(theHeader);
    }
#endif /*HELIX_FEATURE_DRM*/

#if defined(HELIX_FEATURE_RECORDCONTROL)
    SendHeaderToRecordControl(FALSE, theHeader);
#endif /* HELIX_FEATURE_RECORDCONTROL */

    if (m_bInRetryMode)
    {
        if (NETSRC_REDIRECTSTARTED == m_state && theHeader)
        {
            if (!m_pPostRedirectHeaderList)
            {
                m_pPostRedirectHeaderList = new CHXSimpleList();
            }

            if (m_pPostRedirectHeaderList)
            {
                theHeader->AddRef();
                m_pPostRedirectHeaderList->AddTail(theHeader);
            }
        }

        return HXR_OK;
    }

    theErr = PreHeaderCallbackExt(theHeader);
    if (HXR_ABORT == theErr)
    {
        return HXR_OK;
    }

    // we do not support receving headers once we have started getting data.
    if (m_bReceivedData)
    {
        return HXR_FAILED; // define some more appropriate error code.
    }

    if (!theHeader)
    {
        return HX_INVALID_HEADER;
    }

    PostHeaderCallbackExt(theHeader);

#if defined(HELIX_FEATURE_DRM)
    if (IsHelixDRMProtected() && m_pDRM)
    {
        //DRM OnStreamHeader callback will call ProcessStreamHeaders later
        return m_pDRM->ProcessStreamHeader(theHeader);
    }
#endif /*HELIX_FEATURE_DRM*/

    return ProcessStreamHeaders(theHeader, pStreamInfo);
}

HX_RESULT
HXNetSource::ProcessStreamHeaders(IHXValues* pHeader, STREAM_INFO*& pStreamInfo)
{
    HX_RESULT theErr = HXSource::ProcessStreamHeaders(pHeader, pStreamInfo);
    if (HXR_OK == theErr)
    {
        HX_ASSERT(pStreamInfo);

        m_ulStreamIndex++;
        m_uNumStreams++;
    }

    return theErr;
}

HX_RESULT
HXNetSource::PreHeaderCallbackExt(IHXValues* theHeader)
{
    return HXR_OK;
}

HX_RESULT
HXNetSource::PostHeaderCallbackExt(IHXValues* theHeader)
{
    return HXR_OK;
}

HXBOOL HXNetSource::CanSendToDataCallback(IHXPacket* packet) const
{
    HXBOOL bRet = FALSE;
    
    // We send normal data packets and dropped packets to
    // DataCallback()
    if (packet && 
	(!packet->IsLost() || (packet->GetASMFlags() & HX_ASM_DROPPED_PKT)))
    {
	bRet = TRUE;
    }

    return bRet;
}

HX_RESULT
HXNetSource::DataCallback(IHXPacket* pPacket)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::DataCallback");

    HX_RESULT       theErr      = HXR_OK;

    if (!m_bInitialized)
    {
        return HXR_NOT_INITIALIZED;
    }

    // make sure data is valid...
    if (!pPacket)
    {
        return HXR_INVALID_PARAMETER;   // HX_INVALID_DATA;
    }

    m_pBufferManager->UpdateCounters(pPacket, m_lPacketTimeOffSet);

    return theErr;
}

void
HXNetSource::Initialize()
{
    if (!m_bInitialized)
    {
        HXLOGL3(HXLOG_NSRC, "HXNetSource::Initialize()");
        _Initialize();
    }
}

ULONG32
HXNetSource::GetCurrentPlayTime(void)
{
    if (m_pPlayer)
    {
        return m_pPlayer->GetInternalCurrentPlayTime();
    }
    else
    {
        return 0;
    }
}

void
HXNetSource::FirstDataArrives()
{

    m_bAltURL = FALSE;
    /* calculate elapsed time taking into consideration overflow...*/
    m_ulFirstDataArrives = CALCULATE_ELAPSED_TICKS(m_ulStartDataWait,
                                                  HX_GET_TICKCOUNT());
}

HX_RESULT
HXNetSource::SetCookie(IHXBuffer* pCookie)
{
    HX_RESULT hr = HXR_OK;

    if (!m_pCookies)
    {
        hr = HXR_FAILED;
        goto cleanup;
    }

    hr = m_pCookies->SetCookies(m_pHost, m_pPath, pCookie);

cleanup:

    return hr;
}

HX_RESULT
HXNetSource::SetOption(UINT16 option, void* data)
{
    HXLOGL4(HXLOG_NSRC, "HXNetSource::SetOption");

    HX_RESULT   theErr = HXR_OK;
    UINT32      ulMaxBandwidth = 0;
    HXBOOL        bTurboPlay = FALSE;

    switch (option)
    {
        case HX_PERFECTPLAY_SUPPORTED:
            {
                HXBOOL bServerHasPerfectPlay = TRUE;
                ::memcpy(&bServerHasPerfectPlay,data,sizeof(HXBOOL)); /* Flawfinder: ignore */
                m_bServerHasPerfectPlay |= bServerHasPerfectPlay;
            }
            break;
        case HX_RESEND_SUPPORTED:
//          ::memcpy(&m_bServerHasResend,data,sizeof(HXBOOL));
            break;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        case HX_STATS_MASK:
            ::memcpy(&m_ulSendStatsMask,data,sizeof(ULONG32)); /* Flawfinder: ignore */
            if (m_ulSendStatsMask > MAX_STATS_MASK)
                m_ulSendStatsMask = MAX_STATS_MASK;
            break;

        case HX_STATS_INTERVAL:
            {
                UINT32 ulStatsInterval = 0;
                ::memcpy(&ulStatsInterval,data,sizeof(ULONG32)); /* Flawfinder: ignore */

                // did it change. if so, re-schedule
                if (ulStatsInterval != m_ulStatsInterval)
                {
                    m_ulStatsInterval = ulStatsInterval;

                    if (m_pStatsCallback)
                    {
                        // disable Stats report if m_ulStatsInterval is set to 0
                        if (m_ulStatsInterval == 0)
                        {
                            m_pStatsCallback->CancelCallback();
                        }
                        else if (!m_pStatsCallback->IsPaused())
                        {
                            m_pStatsCallback->ScheduleCallback(m_ulStatsInterval);
                        }
                    }
                }
            }
            break;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
        case HX_TRANSPORTSWITCHING_SUPPORTED:
            ::memcpy(&m_bServerHasTransportSwitching,data,sizeof(HXBOOL)); /* Flawfinder: ignore */
            break;
        case HX_FORCE_PERFECT_PLAY:
            // the server is forcing us into perfect play mode
            ::memcpy(&m_bForcePerfectPlay,data,sizeof(HXBOOL)); /* Flawfinder: ignore */
            if(m_bForcePerfectPlay)
            {
                m_bPerfectPlay = TRUE;

                /* If the server is forcing us into PerfectPlay, it means
                 * it does allow it AND the clip is perfectPlay-able.
                 */
                m_bPerfectPlayAllowed   = TRUE;
                m_bServerHasPerfectPlay = TRUE;

                WritePerfectPlayToRegistry();

                if(m_pProto)
                {
                    m_pProto->set_transport(TCPMode, m_ulTransportPrefMask);
                    m_pProto->set_perfect_play(m_bPerfectPlay);
                }
            }
            break;
        case HX_SELECTIVE_RECORD_SUPPORTED:
            ::memcpy(&mServerSelRecordSupport,data,sizeof(HXBOOL)); /* Flawfinder: ignore */
            break;
        case HX_GENERIC_MESSAGE_SUPPORT:
            /* RV client/server does not use it curently */
            break;
        case HX_INTERFRAME_CONTROL_SUPPORT:
            ::memcpy(&mInterframeControlSupport,data,sizeof(HXBOOL)); /* Flawfinder: ignore */
            break;
        case HX_BANDWIDTH_REPORT_SUPPORT:
            ::memcpy(&mServerHasBandwidthReport,data,sizeof(HXBOOL)); /* Flawfinder: ignore */
            break;
        case HX_FRAME_CONTROL_SUPPORT:
            ::memcpy(&mServerHasFrameControl,data,sizeof(HXBOOL)); /* Flawfinder: ignore */
            break;
        case HX_MAX_BANDWIDTH:
            ::memcpy(&ulMaxBandwidth,data,sizeof(UINT32)); /* Flawfinder: ignore */
            if (ulMaxBandwidth == 0)
            {
                m_serverTurboPlay = TURBO_PLAY_OFF;
            }
            else
            {
                m_ulMaxBandwidth = ulMaxBandwidth;
            }
            break;
        case HX_TURBO_PLAY:
            ::memcpy(&bTurboPlay,data,sizeof(HXBOOL)); /* Flawfinder: ignore */
            if (bTurboPlay)
            {
                m_serverTurboPlay = TURBO_PLAY_ON;
            }
            else
            {
                m_serverTurboPlay = TURBO_PLAY_OFF;
            }
            break;
        default:
            theErr = HXR_INVALID_PARAMETER;             //HX_INVALID_OPTION;
            break;
    }

    return theErr;
}

HX_RESULT
HXNetSource::TransportStarted(TransportMode mode)
{
    HX_RESULT rc = HXR_OK;

    if (mode != GetTransportFromURL(m_pszURL))
    {    
#if defined(HELIX_FEATURE_SMARTERNETWORK)
        if (m_pPreferredTransport && !m_pPreferredTransport->ValidateTransport(mode))
        {
            rc = HXR_BAD_TRANSPORT;
        }
#endif /* HELIX_FEATURE_SMARTERNETWORK */
    }

    if (SUCCEEDED(rc))
    {
        set_transport(mode);
    }
    else
    {
        ReportError(rc);
    }

    return rc;
}

HX_RESULT
HXNetSource::switch_out_of_perfectplay()
{
    // Turn off perfect play mode.
    m_bPerfectPlay = FALSE;

    // Reset "transport" selection...
    m_CurrentTransport = UnknownMode;

    // Reconnect...
    return handleTransportSwitch();
}

HX_RESULT
HXNetSource::handleTransportSwitch()
{
    HX_RESULT rc = HXR_OK;

#ifdef _MACINTOSH
    HXBOOL bAtInterrupt = FALSE;
    IHXInterruptState* pInterruptState = NULL;
    if (m_pEngine &&
        m_pEngine->QueryInterface(IID_IHXInterruptState, (void**) &pInterruptState) == HXR_OK)
    {
        bAtInterrupt = pInterruptState->AtInterruptTime();
        HX_RELEASE(pInterruptState);
    }

    if (bAtInterrupt)
    {
        m_bBruteForceConnectToBeDone = TRUE;

        HX_ASSERT(m_pSourceInfo);
        if (m_pSourceInfo)
        {
            m_pSourceInfo->ScheduleProcessCallback();
        }

        return HXR_OK;
    }
#endif

    Reset();
    HX_VECTOR_DELETE(m_pszRedirectServer);
    HX_VECTOR_DELETE(m_pszRedirectResource);
    HX_VECTOR_DELETE(m_pszReconnectServer);
    HX_VECTOR_DELETE(m_pszReconnectURL);

    // have we already received the headers
    if (mStreamInfoTable->GetCount() > 0)
    {
        m_bInRetryMode = TRUE;
    }

    m_bBruteForceReconnected = TRUE;

    HXLOGL2(HXLOG_NSRC, "(%p)TransportSwitch %s", this, m_pszURL);

    rc = Setup(m_pHost, m_pResource, m_uPort,
               m_bLossCorrection, m_pURL, m_bAltURL);

    return rc;
}

HX_RESULT
HXNetSource::handleProxySwitch(HX_RESULT inError)
{
#if defined(HELIX_FEATURE_PAC)
    HX_RESULT   rc = HXR_FAILED;

    if (m_pPACInfoList && m_pPACInfoList->GetCount() && m_PACInfoPosition)
    {
        Reset();

        m_bUseProxy = FALSE;
        HX_VECTOR_DELETE(m_pProxy);

        m_bBruteForceReconnected = TRUE;

        rc = Setup(m_pHost, m_pResource, m_uPort, m_bLossCorrection, m_pURL, m_bAltURL);
    }

    if (HXR_OK != rc)
    {
        rc = inError;
    }

    return rc;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_PAC */
}

HX_RESULT
HXNetSource::handleRedirect()
{
    HX_RESULT   rc = HXR_OK;

    if (!m_pRedirectURL)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    Reset();
    m_ulDuration = 0;
    m_ulPreRollInMs = 0;
    m_ulPreRoll = 0;
    m_ulAvgBandwidth = 0;
    m_ulLastBufferingCalcTime = 0;
    m_bSeekedOrPaused = FALSE;

    // remove obsolete reconnect information
    HX_VECTOR_DELETE(m_pszReconnectServer);
    HX_VECTOR_DELETE(m_pszReconnectProxy);
    HX_VECTOR_DELETE(m_pszReconnectURL);

    // have we already received the headers
    if (mStreamInfoTable->GetCount() > 0)
    {
        m_bInRetryMode = TRUE;
    }

    m_bBruteForceReconnected = TRUE;

    if (m_pHost && strcasecmp(m_pszRedirectServer, m_pHost) != 0)
    {
        while (m_pPACInfoList && m_pPACInfoList->GetCount())
        {
            PACInfo* pPACInfo = (PACInfo*)m_pPACInfoList->RemoveHead();
            HX_DELETE(pPACInfo);
        }

#if defined(HELIX_FEATURE_SMARTERNETWORK)
        if (m_pPreferredTransport)
        {
            if (PTS_READY != m_prefTransportState)
            {
                m_pPreferredTransport->AbortTransport();
            }

            m_pPreferredTransport->RemoveTransportSink(this);
            HX_RELEASE(m_pPreferredTransport);
        }
#endif /* HELIX_FEATURE_SMARTERNETWORK */
    }

    HXLOGL2(HXLOG_NSRC, "(%p)Redirect %s", this, m_pRedirectURL->GetURL());

    rc = Setup(m_pszRedirectServer, m_pszRedirectResource, (UINT16)m_ulRedirectServerPort,
               m_bLossCorrection, m_pRedirectURL, m_bAltURL);

    m_state = NETSRC_REDIRECTSTARTED;

cleanup:

    HX_VECTOR_DELETE(m_pszRedirectServer);
    HX_VECTOR_DELETE(m_pszRedirectResource);

    return rc;
}

HX_RESULT
HXNetSource::handleReconnect()
{
    HX_RESULT   rc = HXR_OK;

    Reset();
    HX_VECTOR_DELETE(m_pszRedirectServer);
    HX_VECTOR_DELETE(m_pszRedirectResource);

    // have we already received the headers
    if (mStreamInfoTable->GetCount() > 0)
    {
        m_bInRetryMode = TRUE;
    }

    m_bBruteForceReconnected = TRUE;

    if (m_pszReconnectURL)
    {
        while (m_pPACInfoList && m_pPACInfoList->GetCount())
        {
            PACInfo* pPACInfo = (PACInfo*)m_pPACInfoList->RemoveHead();
            HX_DELETE(pPACInfo);
        }

#if defined(HELIX_FEATURE_SMARTERNETWORK)
        if (m_pPreferredTransport && strcasecmp(m_pszReconnectServer, m_pHost) != 0)
        {
            if (PTS_READY != m_prefTransportState)
            {
                m_pPreferredTransport->RemoveTransport();
            }
            m_pPreferredTransport->RemoveTransportSink(this);
            HX_RELEASE(m_pPreferredTransport);
        }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

        HXLOGL2(HXLOG_RECO, "(%p)Reconnect %s", this, m_pszReconnectURL);

        CHXURL obj(m_pszReconnectURL, (IHXClientEngine*)m_pEngine);

        rc = Setup(m_pszReconnectServer, m_pResource, (UINT16)m_ulReconnectServerPort,
                   m_bLossCorrection, &obj, m_bAltURL);

    }
    else
    {
        HXLOGL2(HXLOG_RECO, "(%p)Reconnect %s", this, m_pszURL);
        rc = Setup(m_pHost, m_pResource, m_uPort,
                   m_bLossCorrection, m_pURL, m_bAltURL);
    }

    HX_VECTOR_DELETE(m_pszReconnectServer);
    HX_VECTOR_DELETE(m_pszReconnectProxy);
    HX_VECTOR_DELETE(m_pszReconnectURL);

    return rc;
}

HX_RESULT
HXNetSource::handleEndOfSource(void)
{
    HXLOGL3(HXLOG_NSRC, "HXNetSource::handleEndOfSource(); m_state = %s (setting NETSRC_ENDED); m_bSourceEnd = %u", GetStateDesc(m_state), m_bSourceEnd);
    HX_RESULT   rc = HXR_OK;

    m_state = NETSRC_ENDED;

    if (!m_bSourceEnd)
    {
        /* Pause the source, if it is not done yet to free up the bandwidth.
         * This will happen when there is end /dur times on the url.
         */
        if (m_bForcedSourceEnd && !m_bPaused)
        {
            if(m_pProto)
                m_pProto->pause();
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
            if (m_pStatsCallback)
            {
                m_pStatsCallback->PauseCallback();
            }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
        }

        m_bSourceEnd = TRUE;
	
        // If we are playing from the record control,
	// all buffering cvontrol is tied to record control output
	// and not the actual source output since record control is
	// time-shifting the source.
	if (!IsPlayingFromRecordControl())
	{
	    m_pBufferManager->Stop();
	}
	
	if (m_pWMBufferCtl)
	{
	    m_pWMBufferCtl->ClearChillState();
	}

        /* UnRegister any previously registered source */
        if (m_pSourceInfo)
            m_pSourceInfo->UnRegister();

        if (m_pPlayer)
        {
            m_pPlayer->EndOfSource(this);
        }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        if (m_pStatsCallback)
        {
            m_pStatsCallback->CancelCallback();
        }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

	// End of source indicates reception of the last packet.
	// This, however, does not constitute that record control
	// has received its last packet since there may still be
	// a number of packets remaining in the transport layer.
	// Thus, it would be incorrect to signal OnEndOfPackets
	// to record control in this method.
    }

    return rc;
}

HXBOOL
HXNetSource::IsNetworkAvailable()
{
#if defined(HELIX_FEATURE_NETCHECK)
    CHXNetCheck* pNetCheck = new CHXNetCheck(4000);
    HXBOOL bIsNetAvailable = FALSE;

    if(pNetCheck)
    {
        pNetCheck->AddRef();

        if (HXR_OK == pNetCheck->Init((IUnknown*)(IHXPlayer*)m_pPlayer))
        {
            bIsNetAvailable = pNetCheck->FInternetAvailable(FALSE, m_bUseProxy);
        }
    }

    HX_RELEASE(pNetCheck);

    HXLOGL2(HXLOG_NSRC, "(%p)IsNetworkAvailable %lu", this, bIsNetAvailable);

    return bIsNetAvailable;
#else
    return FALSE;
#endif /* HELIX_FEATURE_NETCHECK */
}

HX_RESULT
HXNetSource::AttemptRedirect()
{
    HX_RESULT   rc = HXR_OK;

    if (!m_pRedirectURL)
    {
        return HXR_UNEXPECTED;
    }

    if (NETSRC_REDIRECTPENDING == m_state)
    {
        m_bRedirectInSMIL = FALSE;

        // determine whether it's too late to do redirect within the same
        // source - any redirect received before SetupResponse received is OK
        // since we haven't initialized all the renderers yet
        //
        // we used to decide upon m_bReceivedHeader which is set within
        // HandleDescribeResponse, we now enlarge the window a bit by
        // deciding upon m_bInitialized which is set within HandleSetupResponse
        if (!m_bInitialized)
        {
            if (m_pRedirectURL->IsNetworkProtocol())
            {
                // redirect at source level
                // we don't need to tear down the source since we
                // haven't receive any headers yet
                m_CurrentTransport = UnknownMode;
                mLastError = HXR_OK;

                DeleteStreamTable();
                rc = handleRedirect();
            }
            else
            {
                m_state = NETSRC_READY;
                rc = m_pSourceInfo->HandleRedirectRequest();           
            }
        }
        else if (m_pPlayer && m_pSourceInfo)
        {
            if (m_bPartOfNextGroup)
            {
                m_state = NETSRC_READY;
                m_bRedirectPending = TRUE;
            }
            else if (m_bRARVSource && IsRedirectInSMIL())
            {
                m_bRedirectInSMIL = TRUE;
                m_ulSeekPendingTime = m_pPlayer->GetCurrentPlayTime();
                m_bSeekPending = TRUE;

                rc = handleRedirect();
            }
            else
            {
                // too late to do the Redirect at source level
                // let the player to handle it
                m_state = NETSRC_READY;
                rc = m_pSourceInfo->HandleRedirectRequest();
            }
        }
    }

    return rc;
}

HXBOOL
HXNetSource::IsRedirectInSMIL(void)
{
    HXBOOL                    bResult = FALSE;
    HXPersistentComponent* pPersistentComponent = NULL;

#if defined(HELIX_FEATURE_NESTEDMETA)
    if (m_pPlayer && 
        m_pPlayer->m_pPersistentComponentManager &&
        m_pSourceInfo)
    {
        m_pPlayer->m_pPersistentComponentManager->GetPersistentComponent(m_pSourceInfo->m_ulPersistentComponentID,
                                                                         (IHXPersistentComponent*&)pPersistentComponent);

        if (pPersistentComponent && 
            PersistentSMIL == pPersistentComponent->m_ulPersistentType)
        {
            bResult = TRUE;
        }
        HX_RELEASE(pPersistentComponent);
    }
#endif /* HELIX_FEATURE_NESTEDMETA */

    return bResult;
}

HXBOOL
HXNetSource::IsRedirectedOK(void)
{
    HXBOOL                    bResult = FALSE;
    UINT32                  ulStreamNumber = 0;
    IHXValues*              pHeader = NULL;
    IHXUpdateProperties2*   pUpdateProperties = NULL;
    RendererInfo*           pRendererInfo = NULL;
    CHXSimpleList::Iterator i;

    HX_ASSERT(m_pPostRedirectHeaderList &&
              m_pPostRedirectHeaderList->GetCount());

    // stream count has to match
    if (m_pPostRedirectHeaderList->GetCount() != 
        mStreamInfoTable->GetCount())
    {
        goto cleanup;
    }

    // stream has to be RA/RV and acceptable by the renderer
    i = m_pPostRedirectHeaderList->Begin();
    for (; i != m_pPostRedirectHeaderList->End(); ++i)
    {
        pHeader = (IHXValues*) (*i);
        if (!IsRARVStream(pHeader))
        {
            goto cleanup;
        }        
               
        if (HXR_OK != pHeader->GetPropertyULONG32("StreamNumber", ulStreamNumber))
        {
            goto cleanup;
        }

        if (!m_pSourceInfo->m_pRendererMap->Lookup(ulStreamNumber, (void*&)pRendererInfo))
        {
            goto cleanup;
        }

	// If we do not have the renderer - we'll give the redirect a chance
	// When no renderes are present - we are likely running a pure RTSP agent
	// that does not renderer the stream.  We'll leave it to use entity using
	// the agent to handle any incompatibility issues.
	if (pRendererInfo->m_pRenderer)
	{
	    if (HXR_OK != pRendererInfo->m_pRenderer->QueryInterface(IID_IHXUpdateProperties2,
								     (void**)&pUpdateProperties))
	    {
		goto cleanup;
	    }

	    if (HXR_OK != pUpdateProperties->UpdateHeader(pHeader))
	    {
		goto cleanup;
	    }
	}
        HX_RELEASE(pUpdateProperties);
    }

    bResult = TRUE;

cleanup:

    HX_RELEASE(pUpdateProperties);

    i = m_pPostRedirectHeaderList->Begin();
    for (; i != m_pPostRedirectHeaderList->End(); ++i)
    {
        pHeader = (IHXValues*) (*i);
        HX_RELEASE(pHeader);
    }
    m_pPostRedirectHeaderList->RemoveAll();

    return bResult;   
}

HX_RESULT
HXNetSource::AttemptReconnect()
{
    HXBOOL          bDoneAtTransport = FALSE;
    UINT32          ulCurrentPlayPos = 0;
    UINT32          ulPacketsAheadInTime = 0;
    UINT32          ulReconnectStartTime = 0;
    UINT32          ulLastPacketTime = 0;
    UINT32          ulLastEventTime = 0;
    HXBOOL	    bLowestNotYetSet = TRUE;
    UINT32          ulLowestLastEventTime = MAX_UINT32;
    UINT32          ulLowestTimestampAtTransport = 0;
    UINT32          ulHighestTimestampAtTransport = 0;
    UINT32          ulNumBytesAtTransport = 0;
    STREAM_INFO*    pStreamInfo = NULL;

    m_state = NETSRC_RECONNECTPENDING;

    // caculate how far ahead the packets received at transport
    CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
    for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
    {
        pStreamInfo = (STREAM_INFO*) (*lStreamIterator);

        GetCurrentBuffering(pStreamInfo->m_uStreamNumber,
                            ulLowestTimestampAtTransport,
                            ulHighestTimestampAtTransport,
                            ulNumBytesAtTransport,
                            bDoneAtTransport);

        // in case we have no more packets at transport
        // get the last packet time from the saved queue
        if (ulNumBytesAtTransport == 0 &&
            pStreamInfo->m_pPreReconnectEventList)
        {
            ulLastPacketTime = *((UINT32*) pStreamInfo->m_pPreReconnectEventList->GetTail());
        }
        else
        {
            // XXX HP need to handle timestamp rollover
            ulLastPacketTime = ulHighestTimestampAtTransport;
        }

        ulLastEventTime = CalcEventTime(pStreamInfo, ulLastPacketTime, TRUE);
        if (bLowestNotYetSet ||
	    (((LONG32) (ulLastEventTime - ulLowestLastEventTime)) < 0))
        {
            ulLowestLastEventTime = ulLastEventTime;
	    bLowestNotYetSet = FALSE;
        }
    }

    // if we have received all the packets, we simply set the reconnect pending state without
    // schedule/start reconnect since it's not needed if the user let the playback continues
    // to the end.
    // reconnect will occur if the seek happens afterwards.
    if (bDoneAtTransport)
    {
        return HXR_OK;
    }

    if(!mLiveStream)
    {
        // retrieve all the packets at transport layer to the source layer
#if defined(HELIX_FEATURE_RECORDCONTROL)
        if(m_bPlayFromRecordControl)
        {
            m_state = NETSRC_READY;
            FillRecordControl();
            m_state = NETSRC_RECONNECTPENDING;
        }
        else
#endif /* HELIX_FEATURE_RECORDCONTROL */
        {
            CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
            for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
            {
                CHXEvent* pEvent = NULL;
                pStreamInfo = (STREAM_INFO*) (*lStreamIterator);
                UINT16 uStreamNumber = pStreamInfo->m_uStreamNumber;
                CHXEventList* pEventList = &pStreamInfo->m_EventList;

                // in case we reconnect again right after reconnect
                // get all the packets from PosReconnectEventList first since they are ahead of
                // the packets from protocol
                while (pStreamInfo->m_pPosReconnectEventList &&
                       pStreamInfo->m_pPosReconnectEventList->GetNumEvents())
                {
                    pEvent = (CHXEvent*)pStreamInfo->m_pPosReconnectEventList->RemoveHead();
                    pEventList->InsertEvent(pEvent);
                }

                // get all the packets from the protocol
                if(m_pProto)
                {
                    while (HXR_OK == m_pProto->GetEvent(uStreamNumber, pEvent))
                    {
                        pEventList->InsertEvent(pEvent);
                    }
                }
            }
        }

        Reset();
    }

    ulCurrentPlayPos = m_pPlayer->GetInternalCurrentPlayTime();

    if (((LONG32) (ulCurrentPlayPos - ulLowestLastEventTime)) <= 0)
    {
        ulPacketsAheadInTime = ulLowestLastEventTime - ulCurrentPlayPos;
        if (ulPacketsAheadInTime >= NEW_DEF_SETTINGS_TIMEOUT)
        {
            ulReconnectStartTime = ulPacketsAheadInTime - NEW_DEF_SETTINGS_TIMEOUT;

            if (!m_pReconnectCallback)
            {
                m_pReconnectCallback = new ReconnectCallback(this);
                m_pReconnectCallback->AddRef();
            }

            HXLOGL2(HXLOG_RECO, "(%p)AttemptReconnect in %lu ms", this, ulReconnectStartTime);
            m_pReconnectCallback->ScheduleCallback(ulReconnectStartTime);

            return HXR_OK;
        }
    }

    HXLOGL2(HXLOG_RECO, "(%p)AttemptReconnect now", this);
    return StartReconnect();
}

HX_RESULT
HXNetSource::StartReconnect()
{
    HX_RESULT       rc = HXR_OK;
    HXBOOL            bFirstEvent = TRUE;
    UINT16          uStreamNumber = 0;
    UINT32          ulPrevPacketTime = 0;
    UINT32          ulLargestGapInPacketTime = 0;
    UINT32          ulLastPacketTime = 0;
    UINT32          ulLowestLastPacketTime = MAX_UINT32;
    STREAM_INFO*    pStreamInfo = NULL;
    CHXEvent*       pEvent = NULL;
    CHXEventList*  pEventList = NULL;
    CHXSimpleList::Iterator lIter;

    HXLOGL2(HXLOG_RECO, "(%p)StartReconnect", this);

    // for live source, nothing we can do to archieve seamless user experience like
    // the static content does except tearn down the source and start all over with
    // new source as if it were a redirect
    if (mLiveStream)
    {
        HX_DELETE(m_pRedirectURL);

        if (m_pszReconnectURL)
        {
            m_pRedirectURL = new CHXURL(m_pszReconnectURL,
                                        (IHXClientEngine*)m_pEngine);
        }
        else
        {
            m_pRedirectURL = new CHXURL(m_pszURL,
                                        (IHXClientEngine*)m_pEngine);
        }

        rc = m_pSourceInfo->HandleRedirectRequest();
    }
    else if (NETSRC_RECONNECTPENDING == m_state)
    {
        // retrieve all the packets at transport layer to the source layer
        CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
        for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
        {
            pStreamInfo = (STREAM_INFO*) (*lStreamIterator);
            uStreamNumber = pStreamInfo->m_uStreamNumber;
            pEventList = &pStreamInfo->m_EventList;

            pStreamInfo->m_bReconnectToBeDone = TRUE;
            pStreamInfo->m_ulReconnectOverlappedPackets = 0;

            bFirstEvent = TRUE;

            if (m_bPlayFromRecordControl)
            {
		HX_RESULT nRes;
		
                // Now we can get all available events out of RecordControl
                // in order to use them for reconnection.
                while (TRUE)
                {
                    nRes = GetEventFromRecordControl(uStreamNumber, 
						     pStreamInfo, 
						     pEvent, 
						     TRUE); // Force events out

                    if(nRes == HXR_OK)
                    {
                        pEventList->InsertEvent(pEvent);
                    }

		    // We better not block if we are forcing the events out
		    HX_ASSERT(nRes != HXR_WOULD_BLOCK);
		    
                    if ((nRes != HXR_OK) && (nRes != HXR_RETRY))
		    {
                        break;
		    }
                }

                HX_ASSERT(pEventList->GetCount());
            }

            // save the prereconnect packet time so that we can
            // find the adjointing point after reconnect
            lIter = pEventList->Begin();
            for (; lIter != pEventList->End(); ++lIter)
            {
                pEvent = (CHXEvent*) (*lIter);
                AddToPreReconnectEventList(pStreamInfo, pEvent);
            }

            CHXSimpleList* pPreReconnectEventList = pStreamInfo->m_pPreReconnectEventList;
            HX_ASSERT(pPreReconnectEventList);

            // determine the seek position after reconnect
            // enough overlapped packets to ensure searching of adjointing point
            // successfully after the reconnect
            if (pPreReconnectEventList)
            {
                CHXSimpleList::Iterator lIterator = pPreReconnectEventList->Begin();
                for (; lIterator != pPreReconnectEventList->End(); ++lIterator)
                {
                    UINT32* pPreReconnectEventTime = (UINT32*)(*lIterator);

                    UpdateReconnectInfo(*pPreReconnectEventTime,
                                        bFirstEvent,
                                        ulPrevPacketTime,
                                        ulLargestGapInPacketTime,
                                        ulLastPacketTime);
                }
            }

            if (ulLastPacketTime < ulLowestLastPacketTime)
            {
                ulLowestLastPacketTime = ulLastPacketTime;
            }
        }

        if (ulLowestLastPacketTime > ulLargestGapInPacketTime)
        {
            m_ulSeekPendingTime = ulLowestLastPacketTime - ulLargestGapInPacketTime;
        }
        else
        {
            m_ulSeekPendingTime = 0;
        }
        m_bSeekPending = TRUE;

        m_state = NETSRC_RECONNECTSTARTED;
        rc = handleReconnect();
    }
    else if (NETSRC_RECONNECTFORCED == m_state)
    {
        // we don't need to retrieve the left-over packets if the reconnect
        // is forced(i.e. seek)
        m_state = NETSRC_RECONNECTSTARTED;
        rc = handleReconnect();
    }

    return rc;
}

HX_RESULT
HXNetSource::ProcessReconnect(STREAM_INFO* pStreamInfo)
{
    HX_RESULT       rc = HXR_OK;
    HXBOOL            bAdjointFound = FALSE;
    HXBOOL            bOverlapped = FALSE;
    UINT32          ulAdjointTimeFrom = 0;
    UINT32          ulAdjointTimeTo = 0;
    IHXPacket*      pPacket = NULL;
    IHXPacket*      pNextPacket = NULL;
    UINT32*         pPreReconnectEventTime = NULL;
    UINT32*         pPreReconnectNextEventTime = NULL;
    CHXEvent*       pPosReconnectEvent = NULL;
    CHXEvent*       pPosReconnectNextEvent = NULL;
    CHXSimpleList*  pPreReconnectEventList = NULL;
    CHXEventList*  pPosReconnectEventList = NULL;
    LISTPOSITION    preReconnectPos = 0;
    LISTPOSITION    posReconnectPos = 0;

    //HXLOGL2(HXLOG_RECO, "EnterProcessReconnect %p %lu", this, pStreamInfo->m_uStreamNumber);

    if (!pStreamInfo || !pStreamInfo->m_pPosReconnectEventList)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    if(!pStreamInfo->m_pPreReconnectEventList || !pStreamInfo->m_pPreReconnectEventList->GetCount())
    {
        // We need to use all the packets in pStreamInfo->m_pPosReconnectEventList.
        pStreamInfo->m_bReconnectToBeDone = FALSE;
    }

    pPosReconnectEventList = pStreamInfo->m_pPosReconnectEventList;
    posReconnectPos = pPosReconnectEventList->GetHeadPosition();
    if (posReconnectPos)
    {
        pPosReconnectEvent = (CHXEvent*) pPosReconnectEventList->GetAt(posReconnectPos);
        while (pPosReconnectEvent && pStreamInfo->m_bReconnectToBeDone)
        {
            pPosReconnectEventList->GetNext(posReconnectPos);
            if (!posReconnectPos)
            {
                break;
            }

            pPacket = pPosReconnectEvent->GetPacket();

            pPosReconnectNextEvent = (CHXEvent*) pPosReconnectEventList->GetAt(posReconnectPos);
            pNextPacket = pPosReconnectNextEvent->GetPacket();

            if (pPacket->GetTime() != pNextPacket->GetTime())
            {
                bAdjointFound = TRUE;
                ulAdjointTimeFrom = pPacket->GetTime();
                ulAdjointTimeTo = pNextPacket->GetTime();
            }

            if (bAdjointFound && pStreamInfo->m_pPreReconnectEventList)
            {
                pPreReconnectEventList = pStreamInfo->m_pPreReconnectEventList;
                preReconnectPos = pPreReconnectEventList->GetHeadPosition();

                pPreReconnectEventTime = (UINT32*) pPreReconnectEventList->GetAt(preReconnectPos);
                while (pPreReconnectEventTime)
                {
                    pPreReconnectEventList->GetNext(preReconnectPos);
                    if (!preReconnectPos)
                    {
                        break;
                    }

                    pPreReconnectNextEventTime = (UINT32*) pPreReconnectEventList->GetAt(preReconnectPos);

                    if (*pPreReconnectEventTime == ulAdjointTimeFrom &&
                        *pPreReconnectNextEventTime == ulAdjointTimeTo)
                    {
                        HXLOGL3(HXLOG_RECO, "ProcessReconnect(succeeded)\t%lu\t%lu\t%lu", pStreamInfo->m_uStreamNumber, ulAdjointTimeFrom, ulAdjointTimeTo);
                        pStreamInfo->m_bReconnectToBeDone = FALSE;
                        break;
                    }

                    pPreReconnectEventTime = pPreReconnectNextEventTime;
                }
            }

            pPosReconnectEvent = pPosReconnectNextEvent;
        }
    }

    if (!pStreamInfo->m_bReconnectToBeDone)
    {
        if (pPreReconnectEventList)
        {
            pPreReconnectEventList->GetNext(preReconnectPos);
            while (preReconnectPos)
            {
                pStreamInfo->m_ulReconnectOverlappedPackets++;
                pPreReconnectEventList->GetNext(preReconnectPos);
            }
        }

        HXLOGL3(HXLOG_RECO, "EventsOverlapped\t%lu\t%lu", pStreamInfo->m_uStreamNumber, pStreamInfo->m_ulReconnectOverlappedPackets);

        while (pPosReconnectEventList->GetNumEvents() &&
               pStreamInfo->m_ulReconnectOverlappedPackets)
        {
            pPosReconnectEvent = pPosReconnectEventList->RemoveHead();

            if (bOverlapped)
            {
                pStreamInfo->m_ulReconnectOverlappedPackets--;
            }

            if (pPosReconnectEvent == pPosReconnectNextEvent)
            {
                bOverlapped = TRUE;
            }

            HXLOGL3(HXLOG_RECO, "DeleteEvent(overlapped)\t%lu\t%lu", pStreamInfo->m_uStreamNumber, pPosReconnectEvent->GetPacket()->GetTime());
            HX_DELETE(pPosReconnectEvent);
        }

        EndReconnect();
    }

cleanup:

    HXLOGL3(HXLOG_TRAN, "LeaveProcessReconnect %p %lu %lu", this, pStreamInfo->m_uStreamNumber, pStreamInfo->m_bReconnectToBeDone);

    return rc;
}

HX_RESULT
HXNetSource::EndReconnect()
{
    HX_RESULT       rc = HXR_OK;
    HXBOOL            bReconnectToBeDone = FALSE;
    STREAM_INFO*    pStreamInfo = NULL;

    CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
    for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
    {
        pStreamInfo = (STREAM_INFO*) (*lStreamIterator);

        if (pStreamInfo->m_bReconnectToBeDone)
        {
            bReconnectToBeDone = TRUE;
            break;
        }
    }

    if (!bReconnectToBeDone)
    {
        HXLOGL2(HXLOG_RECO, "(%p)EndReconnect", this);
        m_state = NETSRC_READY;
    }

    return rc;
}

HX_RESULT
HXNetSource::AddToPreReconnectEventList(STREAM_INFO* pStreamInfo, CHXEvent* pEvent)
{
    HX_RESULT       rc = HXR_OK;
    UINT32*         pPacketTime = NULL;
    CHXSimpleList*  pPreReconnectEventList = NULL;

    if (!pStreamInfo->m_pPreReconnectEventList)
    {
        pStreamInfo->m_pPreReconnectEventList = new CHXSimpleList;
    }

    pPreReconnectEventList = pStreamInfo->m_pPreReconnectEventList;
    if (pPreReconnectEventList->GetCount() == 30)
    {
        pPacketTime = (UINT32*)pPreReconnectEventList->RemoveHead();
        HX_DELETE(pPacketTime);
    }

    if (!pPacketTime)
    {
        pPacketTime = new UINT32;
    }

    *pPacketTime = pEvent->GetPacket()->GetTime();
    pPreReconnectEventList->AddTail(pPacketTime);

    HX_ASSERT(pPreReconnectEventList->GetCount() <= 30);

    return rc;
}

void
HXNetSource::UpdateReconnectInfo(UINT32         ulPacketTime,
                                  HXBOOL&         bFirstEvent,
                                  UINT32&       ulPrevPacketTime,
                                  UINT32&       ulLargestGapInPacketTime,
                                  UINT32&       ulLastPacketTime)
{
    UINT32          ulGapInPacketTime = 0;

    if (bFirstEvent)
    {
        bFirstEvent = FALSE;
        ulPrevPacketTime = 0;
        ulLargestGapInPacketTime = 0;
        ulLastPacketTime = 0;
    }
    else
    {
        HX_ASSERT(ulPacketTime >= ulPrevPacketTime);

        ulGapInPacketTime = ulPacketTime - ulPrevPacketTime;
        if (ulLargestGapInPacketTime < ulGapInPacketTime)
        {
            ulLargestGapInPacketTime = ulGapInPacketTime;
        }
    }
    ulPrevPacketTime = ulPacketTime;

    if (ulLastPacketTime < ulPacketTime)
    {
        ulLastPacketTime = ulPacketTime;
    }

    return;
}

void
HXNetSource::Reset()
{
    HXLOGL3(HXLOG_NSRC, "HXNetSource::Reset()");
    // Consider the net play object uninitialized.
    m_bInitialized              = FALSE;
    m_bPerfectPlayErrorChecked  = FALSE;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (m_pStatsCallback)
    {
        m_pStatsCallback->CancelCallback();
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    // Stop current connection. This will totally disconnect
    // from the server, and shut down the communication ports.
    // DoCleanup();

    //  Close down the net connection
    HX_RELEASE(m_pProtocolStatus);

    /* UnRegister any previously registered source */
    if (m_pSourceInfo)
        m_pSourceInfo->UnRegister();

    if (m_pProto)
    {
        m_pProto->stop();
        HX_RELEASE (m_pProto);
    }

    cleanup_proxy();

    // cleanup the log list
    if (m_pLogInfoList)
    {
        while (m_pLogInfoList->GetCount() > 0)
        {
            char* pszInfo = (char*) m_pLogInfoList->RemoveHead();
            delete [] pszInfo;
        }

        HX_DELETE(m_pLogInfoList);
    }

    m_ulMaxBandwidth = 4000;
    m_bPushDownSet = FALSE;
    m_pushDownStatus = PUSHDOWN_NONE;
    m_bReceivedData = FALSE;
    m_bDataWaitStarted  = FALSE;
    m_bFirstResume = TRUE;
    m_bResendAuthenticationInfo = TRUE;
    m_bRTSPRuleFlagWorkAround = FALSE;
    m_bIsActive = FALSE;

    return;
}

void
HXNetSource::ReSetup()
{
    HX_RESULT theErr = HXR_OK;

    Reset();

    mServerSelRecordSupport = FALSE;
    mInterframeControlSupport = FALSE;
    mServerHasBandwidthReport = FALSE;
    mServerHasFrameControl = FALSE;
    m_bConnectionWait = FALSE;
    m_bUseUDPPort = FALSE;
    m_bTimeBased = FALSE;
    m_bAtInterrupt = FALSE;
    m_bInRetryMode = FALSE;
    m_bBruteForceReconnected = FALSE;
    m_bBruteForceConnectToBeDone = FALSE;
    m_bUserHasCalledResume = FALSE;
    m_bUserHasCalledStartInit = FALSE;
    m_CurrentTransport = UnknownMode;
    m_ulSeekPendingTime = 0;
    m_bSeekPending = FALSE;
    m_ulStatsInterval = 0;
    m_ulSendStatsMask = 0;
    m_ulMaxPreRoll = 0;

#if defined(HELIX_FEATURE_SMIL_REPEAT)
    if (m_pSourceInfo)
    {
        CHXSimpleList* pRepeatList = m_pSourceInfo->m_bLeadingSource?m_pSourceInfo->m_pRepeatList:
                                                                     m_pSourceInfo->m_pPeerSourceInfo->m_pRepeatList;

        if (pRepeatList)
        {
            RepeatInfo* pRepeatInfo = (RepeatInfo*)pRepeatList->GetAt(m_pSourceInfo->m_curPosition);
            m_ulDelay = m_pSourceInfo->m_ulRepeatDelayTimeOffset + pRepeatInfo->ulDelay;

            if (m_pSourceInfo->m_bRepeatIndefinite      &&
                m_pSourceInfo->m_ulMaxDuration  &&
                m_ulDelay + pRepeatInfo->ulDuration > m_ulOriginalDelay + m_pSourceInfo->m_ulMaxDuration)
            {
                m_ulRestrictedDuration = m_ulOriginalDelay + m_pSourceInfo->m_ulMaxDuration - m_ulDelay;
            }
            else
            {
                m_ulRestrictedDuration = pRepeatInfo->ulDuration;
            }
        }
    }
#endif /* HELIX_FEATURE_SMIL_REPEAT */

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    m_pStats->Reset();
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    m_bReSetup = TRUE;
    theErr = Setup(m_pHost, m_pResource, m_uPort, m_bLossCorrection,
                   m_pURL, m_bAltURL);

    return;
}

HX_RESULT
HXNetSource::switch_to_next_transport(HX_RESULT incomingError)
{
    HXLOGL3(HXLOG_NSRC, "HXNetSource::switch_to_next_transport(): error = %08x", incomingError);
    HX_RESULT theErr = HXR_OK;

    /* Use TCP for PercectPlay for PNA protocol.
     * for RTSP, we always use UDP..
     * m_bTimeBased is set to TRUE for RTSP protocol
     */
    if ((m_bPerfectPlay && !m_bTimeBased) || !m_pPreferredTransport)
    {
        if (UDPMode == m_CurrentTransport)
        {
            m_PreferredTransport = TCPMode;
        }
#if defined(HELIX_FEATURE_HTTPCLOAK)
        else if (TCPMode == m_CurrentTransport)
        {
            m_PreferredTransport = HTTPCloakMode;
        }
#endif
        else
        {
            m_PreferredTransport = UnknownMode;
        }
    }
#if defined(HELIX_FEATURE_SMARTERNETWORK)
    else
    {
        theErr = m_pPreferredTransport->SwitchTransport(incomingError, m_PreferredTransport);
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    // If there is no new transport, then we are done!
    if (HXR_OK != theErr || UnknownMode == m_PreferredTransport)
    {
        theErr = incomingError;
        goto exit;
    }

    // reset transport state to PTS_CREATE so the PreferredTransport object
    // will be notified on the success/failure of this transport switch
    m_prefTransportState = PTS_CREATE;

    // Actually switch here!
    if (m_pProto && m_pProto->can_switch_transport())
    {
        theErr = m_pProto->switch_transport(m_PreferredTransport);
    }
    else
    {
        // Switch transport via brute force, non-optimal manner.
        theErr = handleTransportSwitch();
        mLastError = theErr;
    }

exit:

    return theErr;
}

HX_RESULT
HXNetSource::_Initialize(void)
{
    HX_RESULT theErr = HXR_OK;

    ResetASMSource();

    if (m_bInRetryMode)
    {
        if (m_bBruteForceReconnected)
        {
            // reset ASMSource
            CHXSimpleList::Iterator lIter = m_HXStreamList.Begin();
            for (; lIter != m_HXStreamList.End(); ++lIter)
            {
                HXStream* pStream = (HXStream*) (*lIter);

                pStream->ResetASMSource((IHXASMSource*)m_pProto);
            }

            if (m_pSourceInfo)
            {
                m_pSourceInfo->ReInitializeStats();
            }

            if (m_bFastStart)
            {
                EnterFastStart();
            }
        }

        if (m_bClipTimeAdjusted)
        {
            m_bInitialized = TRUE;

            /* Initial seek if a start time is specified */
            if (m_ulStartTime > 0)
            {
                DoSeek(0);
            }
        }
    }

    /* get all the flags */
    if (!theErr && m_pProto)
    {
        m_bPerfectPlayAllowed |= m_pProto->IsPerfectPlayAllowed();
        mSaveAsAllowed        |= m_pProto->IsSaveAllowed();

        WritePerfectPlayToRegistry();

        mLiveStream = m_pProto->IsLive();

        if (mLiveStream && m_bCustomEndTime)
        {
            m_bRestrictedLiveStream = TRUE;
        }
    }

    if (m_bInitialized)
    {
        return HXR_OK;
    }

    if (!theErr)
    {
        m_bInitialized = TRUE;

        m_bRARVSource = IsRARVSource();

        m_ulOriginalDuration = m_ulDuration;
        theErr = AdjustClipTime();
    }

    m_pBufferManager->Init();

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    /*
     * put the protocol and protocol version into the stats so rarenderer
     * and any other part of the system that needs to work with old servers
     * can know what the version and protocol is.
     */
    if (m_pStats != NULL && m_pProto != NULL)
    {
        m_pStats->m_pProtocolVersion->SetInt(m_pProto->get_protocol_version());
        m_pStats->m_pProtocol->SetStr((char*)m_pProto->get_protocol_name());
    }

    IHXBuffer* pBuffer = NULL;
    IHXValues* pResponseHeaders = NULL;

    if (HXR_OK == m_pRequest->GetResponseHeaders(pResponseHeaders) &&
        pResponseHeaders)
    {
        if (HXR_OK == pResponseHeaders->GetPropertyCString("Server", pBuffer))
        {
            if (m_pStats->m_pServerInfo)
            {
                m_pStats->m_pServerInfo->SetStr((char*)pBuffer->GetBuffer());
            }
        }
        HX_RELEASE(pBuffer);
    }
    HX_RELEASE(pResponseHeaders);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return theErr;
}

/*
 *      IHXRegistryID methods
 */

/************************************************************************
 *      Method:
 *          IHXRegistryID::GetID
 *      Purpose:
 *          Get registry ID(hash_key) of the objects(player, source and stream)
 *
 */
STDMETHODIMP
HXNetSource::GetID(REF(UINT32) /*OUT*/ ulRegistryID)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    (m_pStats)?(ulRegistryID = m_pStats->m_ulRegistryID):(ulRegistryID = 0);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

/************************************************************************
 *      Method:
 *          IHXInfoLogger::LogInformation
 *      Purpose:
 *          Logs any user defined information in form of action and
 *          associated data.
 */
STDMETHODIMP
HXNetSource::LogInformation(const char* /*IN*/ pAction,
                            const char* /*IN*/ pData)
{
    HX_RESULT   hr = HXR_OK;
    UINT32      ulTimeSinceStart = 0;
    UINT32      ulCurrentPlayPos = 0;
    UINT32      ulInfoLen = 0;

    // action Name is a must
    if (!pAction)
    {
        return HXR_FAILED;
    }

    if (!m_pLogInfoList)
    {
        return HXR_UNEXPECTED;
    }

    // If we've already reached our limit, don't add any more info.
    if (m_ulLogInfoLength > MAX_LOGINFO_LENGTH)
    {
        return HXR_OK;
    }

    ulTimeSinceStart = CALCULATE_ELAPSED_TICKS(m_ulSourceStartTime, HX_GET_TICKCOUNT());
    // XXXCP: during auto-config there is no m_pPlayer
    if (m_pPlayer)
        ulCurrentPlayPos = m_pPlayer->GetInternalCurrentPlayTime();


    // 10 chars for max UINT32, 1 for |, 1 for last \0
    // additional 1 for \0
    ulInfoLen = 10*2 + 1*3 + 1;

    ulInfoLen += strlen(pAction);
    ulInfoLen += pData ? (strlen(pData) + 2) : 0; // +2 for ()

    char*   pszInfo = new char[ulInfoLen];
    memset(pszInfo, 0, ulInfoLen);

    SafeSprintf(pszInfo, ulInfoLen, "%lu|%lu|%s|", ulTimeSinceStart, ulCurrentPlayPos, pAction); /* Flawfinder: ignore */

    if (pData)
    {
        SafeStrCat(pszInfo, "(", ulInfoLen);
        SafeStrCat(pszInfo, pData, ulInfoLen);
        SafeStrCat(pszInfo, ")", ulInfoLen);
    }

    SafeStrCat(pszInfo, ";", ulInfoLen);

    m_ulLogInfoLength += strlen(pszInfo);

    // append to the list
    m_pLogInfoList->AddTail((void*) pszInfo);

    // If we just reached our size limit, add "..." to the end
    if (m_ulLogInfoLength > MAX_LOGINFO_LENGTH)
    {
        pszInfo = new char[4];
        strcpy(pszInfo, "..."); /* Flawfinder: ignore */
        m_pLogInfoList->AddTail((void*) pszInfo);
        m_ulLogInfoLength += 4;
    }

    return hr;
}

/*
 * IHXPreferredTransportSink methods
 */
STDMETHODIMP
HXNetSource::TransportSucceeded(TransportMode   /* IN */   prefTransportType,
                                 UINT16         /* IN */   uCloakPort)
{
    HXLOGL3(HXLOG_NSRC, "HXNetSource::TransportSucceeded(): type = %lu", prefTransportType);
#if defined(HELIX_FEATURE_SMARTERNETWORK)
    if (m_pPreferredTransport)
    {
        if (m_prefTransportState == PTS_CREATE)
        {
            m_prefTransportState = PTS_READY;
            m_pPreferredTransport->SetTransport(prefTransportType, uCloakPort);
        }
        else if (m_prefTransportState == PTS_PENDING)
        {
            m_prefTransportState = PTS_READY;
            HX_ASSERT(m_prefTransportState == m_pPreferredTransport->GetState());

            m_pPreferredTransport->GetTransport(m_PreferredTransport,
                                                m_uCurrCloakedPort);

            m_state = NETSRC_TRANSPORTREADY;
        }
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    return HXR_OK;
}

STDMETHODIMP
HXNetSource::TransportFailed()
{
    return HXR_OK;
}

STDMETHODIMP
HXNetSource::TransportAborted()
{
#if defined(HELIX_FEATURE_SMARTERNETWORK)
    HX_ASSERT(m_pPreferredTransport);

    if ((m_state == NETSRC_REDIRECTSTARTED)
        || (m_state == NETSRC_REDIRECTPENDING))
    {
        // if we're already in the middle of a redirect
        // we don't care that somebody else got a redirect
        // (and thus aborted).
        return HXR_OK;
    }

    if (m_pPreferredTransport)
    {
        m_prefTransportState = m_pPreferredTransport->GetState();

        HX_ASSERT((m_prefTransportState == PTS_CREATE)
            || (m_prefTransportState == PTS_PENDING));

        if (m_prefTransportState == PTS_CREATE)
        {
            HX_ASSERT(m_prefTransportState == m_pPreferredTransport->GetState());
            // m_pPreferredTransport->RemoveTransportSink(this) will be called in
            // FinishSetup.
            m_pPreferredTransport->GetTransport(m_PreferredTransport,
                                                m_uCurrCloakedPort);

            m_state = NETSRC_TRANSPORTREADY;
        }
        else if (m_prefTransportState == PTS_PENDING)
        {
            HX_ASSERT(m_state == NETSRC_TRANSPORTPENDING);
        }
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    return HXR_OK;
}

void
HXNetSource::ReportError(HX_RESULT theErr)
{
    if (!mLastError && theErr)
        mLastError = theErr;
}

void
HXNetSource::ActualReportError(HX_RESULT theErr)
{
#if defined(HELIX_FEATURE_SMARTERNETWORK)
    if (m_pPreferredTransport)
    {
        m_prefTransportState = PTS_UNKNOWN;
        m_pPreferredTransport->RemoveTransport();
        m_pPreferredTransport->RemoveTransportSink(this);
        HX_RELEASE(m_pPreferredTransport);
    }
#endif /* HELIX_FEATURE_SMARTERNETWORK */

    CHXString alert;
    if (IS_SERVER_ALERT(theErr))
    {
        UINT32  ulAlert = 0;
        const char* pszAlert = m_pProto->GetLastAlertInfo(ulAlert);
        if(pszAlert)
        {
            // prefix alert info
            alert = pszAlert;
            alert += "\r\n";
        }
    }

    if (helixSDPProtocol != m_uProtocolType)
    {
        alert += m_pszURL;
    }

    m_pPlayer->ReportError(this, theErr, alert);
   
    return;
}

void
HXNetSource::CalculateCurrentBuffering(void)
{
    UINT32      ulRemainToBufferInMs    = 0;
    UINT32      ulRemainToBuffer        = 0;
    UINT32      ulExcessBufferInMs      = 0;
    UINT32      ulExcessBuffer          = 0;
    HXBOOL        bValidInfo              = FALSE;
    UINT32      ulActualExcessBufferInMs= 0;
    UINT32      ulActualExcessBuffer    = 0;

    m_pBufferManager->GetExcessBufferInfo(ulRemainToBufferInMs,
                                          ulRemainToBuffer,
                                          ulExcessBufferInMs,
                                          ulExcessBuffer,
                                          bValidInfo,
                                          ulActualExcessBufferInMs,
                                          ulActualExcessBuffer);

    if (bValidInfo)
    {
        /* Be conservative in marking pre-buffering done */
        if (!m_bIsPreBufferingDone &&
            ulRemainToBufferInMs == 0 && ulRemainToBuffer == 0 &&
            (ulExcessBuffer > m_ulAvgBandwidth/8 ||
             ulExcessBufferInMs > m_ulPreRollInMs))
        {
            if (m_bDelayed && m_pPlayer)
            {
                // pause the src if it's not time to play but it's done
                // with the pre-fetch
                if (m_pSourceInfo)
                {
                    m_pSourceInfo->UnRegister();
                }

                DoPause();
            }

            m_bIsPreBufferingDone = TRUE;
        }

	if (m_pWMBufferCtl)
	{
	    m_pWMBufferCtl->OnBufferReport(ulActualExcessBufferInMs,
					   ulActualExcessBuffer);
	}
    }
}

void
HXNetSource::SetEndOfClip(HXBOOL bForcedEndofClip)
{
    HXLOGL3(HXLOG_NSRC, "HXNetSource::SetEndOfClip: forced = %u; m_state = %s", bForcedEndofClip, GetStateDesc(m_state));
    // ignore the end of source if we already in redirecting
    if (m_state == NETSRC_REDIRECTPENDING || 
        m_state == NETSRC_REDIRECTSTARTED)
    {
        return;
    }

    m_bForcedSourceEnd = bForcedEndofClip;
    // we only set the state, we need to make sure there is
    // no pending RTSP request(i.e. redirect) in the queue since
    // there could be a race condition between EOS packet(UDP) and
    // RTSP redirect(TCP)
    //
    // if the state isn't changed in the next ProcessIdle(), then
    // handleEndOfSource() will be called
    m_state = NETSRC_ENDPENDING;
}

HXBOOL
HXNetSource::IsSourceDone(void)
{
    return m_bSourceEnd;
}

void
HXNetSource::EnterBufferedPlay(void)
{
#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
    if (!m_bBufferedPlay)
    {
        HXLOGL2(HXLOG_NSRC, "(%p)Enter BufferedPlay", this);
        m_bBufferedPlay = TRUE;
        if (m_pBufferManager)
        {
            m_pBufferManager->EnterBufferedPlay();
        }
    }
#endif // (HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
}

void
HXNetSource::LeaveBufferedPlay(void)
{
#if !defined(HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
    if (m_bBufferedPlay)
    {
        HXLOGL2(HXLOG_NSRC, "(%p)Leave BufferedPlay", this);

        m_bBufferedPlay = FALSE;
        if (m_pBufferManager)
        {
            m_pBufferManager->LeaveBufferedPlay();
        }
    }
#endif // (HELIX_CONFIG_MIN_PCM_PUSHDOWN_BYTES)
}

HX_RESULT
HXNetSource::GetCurrentBuffering (UINT16 uStreamNumber,
                                  UINT32 &ulLowestTimestamp,
                                  UINT32 &ulHighestTimestamp,
                                  UINT32 &ulNumBytes,
                                  HXBOOL &bDone)

{
    ulLowestTimestamp   = 0;
    ulHighestTimestamp  = 0;
    ulNumBytes          = 0;
    bDone               = FALSE;

    return m_pProto ? m_pProto->GetCurrentBuffering(uStreamNumber,
                                                    ulLowestTimestamp,
                                                    ulHighestTimestamp,
                                                    ulNumBytes,
                                                    bDone) : HXR_OK;
}

STDMETHODIMP
HXNetSource::GetProxyInfoDone(HX_RESULT    status,
                               char*        pszProxyInfo)
{
    HX_RESULT   rc = HXR_OK;

#if defined(HELIX_FEATURE_PAC)
    PACInfo*    pPACInfo = NULL;
    HXLOGL2(HXLOG_NSRC, "(%p)GetProxyInfoDone: %s %lu", this, m_pszURL, status);

    if (HXR_OK == status && pszProxyInfo)
    {
        ::ParsePACInfo(pszProxyInfo, m_pPACInfoList);
        // at least one PAC entry
        HX_ASSERT(m_pPACInfoList && m_pPACInfoList->GetCount());

        m_PACInfoPosition = m_pPACInfoList->GetHeadPosition();
        pPACInfo = (PACInfo*)m_pPACInfoList->GetNext(m_PACInfoPosition);

        if (pPACInfo && pPACInfo->type != PAC_DIRECT)
        {
            HXLOGL2(HXLOG_NSRC, "(%p)PAC: %s %lu", this, pPACInfo->pszHost, pPACInfo->ulPort);
            HX_ASSERT(pPACInfo->type == PAC_PROXY);
            set_proxy(pPACInfo->pszHost, (UINT16)pPACInfo->ulPort);
        }
        else if (pPACInfo)
        {
            HXLOGL2(HXLOG_NSRC, "(%p)PAC: DIRECT", this);
        }
    }
#endif /* HELIX_FEATURE_PAC */

    if (m_state == NETSRC_PACPENDING)
    {
        m_state = NETSRC_PACREADY;
    }

    return rc;
}

void
HXNetSource::AdjustClipBandwidthStats(HXBOOL bActivate /* = FALSE */)
{
    if (!m_pProto)
    {
        return;
    }

    m_bIsActive = bActivate;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    CHXMapLongToObj::Iterator lStreamIterator = mStreamInfoTable->Begin();
    for (; lStreamIterator != mStreamInfoTable->End(); ++lStreamIterator)
    {
        ULONG32         ulStreamNumber = 0;
        STREAM_INFO*    pStreamInfo    = (STREAM_INFO*) (*lStreamIterator);
        STREAM_STATS*   pStreamStats   = NULL;

        if (m_pProto &&
            HXR_OK == m_pProto->GetStreamStatistics((ULONG32) pStreamInfo->m_uStreamNumber, &pStreamStats))
        {
            HX_ASSERT(pStreamStats && pStreamStats->m_bInitialized);

            if (!pStreamStats || !pStreamStats->m_bInitialized)
            {
                continue;
            }

//          HXLOGL2(HXLOG_NSRC, "AdjustClipBandwidthStats: Source: %p Stream=%d On: %d, Bw=%d",
//              this, pStreamInfo->m_uStreamNumber, (int) bActivate, (bActivate? pStreamInfo->m_ulAvgBandwidth : 0));
            if (bActivate)
            {
                pStreamStats->m_pClipBandwidth->SetInt((INT32)pStreamInfo->BufferingState().AvgBandwidth());
            }
            else
            {
                pStreamStats->m_pClipBandwidth->SetInt(0);
            }
        }
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

HXBOOL
HXNetSource::CanBeResumed()
{
    HXBOOL    bResult = TRUE;

    if (!m_bInitialized             ||
        !m_pProto                   ||
        !m_pPlayer->IsInitialized() ||
        m_bSourceEnd                ||
        (m_pSourceInfo && !m_pSourceInfo->AreStreamsSetup()))
    {
        bResult = FALSE;
    }
#if defined(HELIX_FEATURE_PREFETCH)
    else if (m_bPrefetch)
    {
		UINT32  ulCurrentTime = 0;
        HX_ASSERT(m_pSourceInfo?(m_pPlayer->GetCurrentGroupID() == m_pSourceInfo->m_uGroupID):TRUE);

        // m_bDelayed could be turned off by pre-buffering in TryResume()
        if (m_bDelayed)
        {
            ulCurrentTime = m_pPlayer->GetInternalCurrentPlayTime();
            if (ulCurrentTime < m_ulPrefetchDelay)
            {
                bResult = FALSE;
            }
        }
    }
#endif /* HELIX_FEATURE_PREFETCH */
    else if ((!m_bIsPreBufferingStarted && m_bDelayed) ||
             (m_bIsPreBufferingDone && ((!m_bPaused && !m_bFirstResume) || m_bDelayed)) ||
             // if the source is in the current group, don't resume till its renderers
             // have been initialized
             (m_pSourceInfo && !m_pSourceInfo->IsInitialized() &&
             m_pPlayer->GetCurrentGroupID() == m_pSourceInfo->m_uGroupID))
    {
        bResult = FALSE;
    }

    return bResult;
}

void
HXNetSource::ResetASMSource(void)
{
    HX_RELEASE(m_pBackChannel);
    HX_RELEASE(m_pASMSource);

    HX_ASSERT(m_pProto);

    m_pProto->QueryInterface(IID_IHXBackChannel, (void**) &m_pBackChannel);
    m_pProto->QueryInterface(IID_IHXASMSource, (void**) &m_pASMSource);
}

HXBOOL
HXNetSource::IsInCloakPortList(UINT16 uPort)
{
    HXBOOL bResult = FALSE;
    int  i = 0;

    if (m_pCloakPortList && m_nNumberOfCloakPorts)
    {
        for (i = 0; i < m_nNumberOfCloakPorts; i++)
        {
            if (m_pCloakPortList[i] == uPort)
            {
                bResult = TRUE;
                break;
            }
        }
    }

    return bResult;
}

void
HXNetSource::WritePerfectPlayToRegistry()
{
    // JEBXXX: write out the perfect play registry item
    IHXRegistry* pRegistry = NULL;
    if (m_pEngine->QueryInterface(IID_IHXRegistry, (void**)&pRegistry) == HXR_OK)
    {
        UINT32 ulRegID = 0;
        if (HXR_OK == GetID(ulRegID) && ulRegID != 0)
        {
            IHXBuffer* pBuffer = NULL;
            char szRegistryEntry[256];  /* Flawfinder: ignore */
            if (HXR_OK == pRegistry->GetPropName(ulRegID, pBuffer))
            {
                // now build key name
                SafeSprintf (szRegistryEntry, 256, "%s.PerfectPlayAllowed", pBuffer->GetBuffer());

                // if AddInt fails, set existing value instead
                if (HXR_OK != pRegistry->AddInt(szRegistryEntry, (m_bPerfectPlayAllowed ?1 :0)))
                {
                    INT32 val = (m_bPerfectPlayAllowed ?1 :0);
                    pRegistry->SetIntByName(szRegistryEntry, val);
                }

                HX_RELEASE(pBuffer);
            }
        }

        HX_RELEASE(pRegistry);
    }
}

void
HXNetSource::CreateCloakedPortList()
{
    // time to create cloak port list!
    if (!m_pCloakPortList)
    {
        m_pCloakPortList = new UINT16[MAX_CLOAK_PORTS];
        IHXValues* pOptions = NULL;
        IHXBuffer* pCloakPorts = NULL;
        UINT32      ulCloakPort = 0;
        HXBOOL    bShouldAddDefaultPort = TRUE;
        if (m_pURL)
        {
            pOptions = m_pURL->GetOptions();
        }

        if (pOptions)
        {
            // try the string and then the number!
            if (HXR_OK != pOptions->GetPropertyBuffer(CLOAKPORT_URL_OPTION, pCloakPorts))
            {
                pOptions->GetPropertyULONG32(CLOAKPORT_URL_OPTION, ulCloakPort);
            }
        }

        if (pCloakPorts)
        {
            char* pPortString = ::new char[pCloakPorts->GetSize() + 1];
            ::strcpy(pPortString, (const char*) pCloakPorts->GetBuffer()); /* Flawfinder: ignore */
            char* pTok = ::strtok(pPortString, ", ");
            while (pTok && m_nNumberOfCloakPorts < MAX_CLOAK_PORTS)
            {
                UINT16 uTmpCloakPort = ::atoi(pTok);
                m_pCloakPortList[m_nNumberOfCloakPorts++] = uTmpCloakPort;
                if (uTmpCloakPort == m_uPort)
                {
                    bShouldAddDefaultPort = FALSE;
                }
                pTok = ::strtok(NULL, ", ");
            }

            HX_VECTOR_DELETE(pPortString);
        }
        else if (ulCloakPort > 0)
        {
            m_pCloakPortList[m_nNumberOfCloakPorts++] = (UINT16) ulCloakPort;
            if (ulCloakPort == m_uPort)
            {
                bShouldAddDefaultPort = FALSE;
            }
        }

        if (pCloakPorts || (ulCloakPort > 0))
        {
            if (bShouldAddDefaultPort && m_nNumberOfCloakPorts < MAX_CLOAK_PORTS)
            {
                m_pCloakPortList[m_nNumberOfCloakPorts++] = m_uPort;
            }
        }
        else
        {
            for (UINT16 i = 0; i < MAX_CLOAK_PORTS; i++)
            {
                // use the port specified in the URL instead of using the
                // standard ports!
                if ((m_bRTSPProtocol && g_uCloakPorts[i] == 554) ||
                    (!m_bRTSPProtocol && g_uCloakPorts[i] == 7070))
                {
                    m_pCloakPortList[i] = m_uPort;
                }
                else
                {
                    m_pCloakPortList[i] = g_uCloakPorts[i];
                }
            }

            m_nNumberOfCloakPorts = MAX_CLOAK_PORTS;
        }

        HX_RELEASE(pCloakPorts);
        HX_RELEASE(pOptions);

        HX_ASSERT(m_pCloakPortList && m_nNumberOfCloakPorts > 0);
        m_nCurrPortIdx = 0;
        m_uCurrCloakedPort = m_pCloakPortList[m_nCurrPortIdx];
    }
}

HX_RESULT
HXNetSource::ReportStats()
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    HX_ASSERT(m_bSendStatistics && m_pProto);

    if (!m_bSendStatistics || !m_pProto)
    {
        return HXR_OK;
    }

    m_pProto->send_statistics(m_ulSendStatsMask);

    if (m_pStatsCallback && m_ulStatsInterval > 0)
    {
        m_pStatsCallback->ScheduleCallback(m_ulStatsInterval);
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

HX_RESULT
HXNetSource::FillRecordControl(UINT32 ulLoopEntryTime)
{
#if defined(HELIX_FEATURE_RECORDCONTROL)
    if(m_state == NETSRC_RECONNECTPENDING)
        return HXR_OK;

    if(!m_bPlayFromRecordControl || m_bForcedSourceEnd || mLastError)
        return HXR_FAILED;

    if(!m_pRecordControl->CanAcceptPackets())
        return HXR_OK;

    // Try to time interleave packets of different stream for better playback performance.
    // Using 1 sec as pace for time-interleaving packets of different streams.
    HXBOOL bFillTimeSet = FALSE;
    UINT32 nFillTime = 0;
    HXBOOL bHasPackets = TRUE;

    // To work around problem wirh PNA protocol, which stops giving packets when we have 
    // less then certain ammount of bytes on transport level, we have to alternate initial 
    // stream number for packet retrieval. The best interleaving happens when we start
    // from a stream that is most behind in time. (PR# 112748)
    UINT16 nInitialStream = 0;
    UINT32 nHighestTime = 0xFFFFFFFF;

    UINT16 nStream;
    for(nStream = 0; nStream < m_uNumStreams; nStream++)
    {
        STREAM_INFO*    pStreamInfo = NULL;
        mStreamInfoTable->Lookup((LONG32) nStream, (void *&) pStreamInfo);

        if (pStreamInfo)
	{
	    if (pStreamInfo->BufferingState().LastPacketTimestamp() < nHighestTime)
	    {
		nHighestTime = pStreamInfo->BufferingState().LastPacketTimestamp();
		nInitialStream = nStream;
	    }
	    
	    if (pStreamInfo->BufferingState().GetMinBufferingInMs() > m_ulMaxPreRoll)
	    {
		m_ulMaxPreRoll = pStreamInfo->BufferingState().GetMinBufferingInMs();
	    }
	}
    }

    while(bHasPackets)
    {
        bHasPackets = FALSE;

        if(bFillTimeSet)
            nFillTime += 1000;

        for(UINT16 nIndex = 0; nIndex < m_uNumStreams; nIndex++)
        {
            nStream = (nInitialStream + nIndex) % m_uNumStreams;

            STREAM_INFO* pStreamInfo = NULL;
            if (!mStreamInfoTable->Lookup((LONG32) nStream, (void *&) pStreamInfo))
            {
                HX_ASSERT(FALSE);
                return HXR_INVALID_PARAMETER;
            }

            CHXEvent* pProtocolEvent = NULL;
            while(GetEventFromProtocol(nStream, pStreamInfo, pProtocolEvent) == HXR_OK &&

                  pProtocolEvent)
            {
                UINT32 nPacketTime = 0;
                IHXPacket* pPacket = pProtocolEvent->GetPacket();
                if (pPacket)
                {
                    bHasPackets = TRUE;
                    m_pRecordControl->OnPacket(pPacket, pProtocolEvent->GetTimeOffset());

                    nPacketTime = pPacket->GetTime();
                }

                HX_DELETE(pProtocolEvent);

                if(!bFillTimeSet)
                {
                    bFillTimeSet = TRUE;
                    nFillTime = nPacketTime + 1000;
                }

                if(nPacketTime >= nFillTime && nPacketTime - nFillTime < MAX_TIMESTAMP_GAP ||
                   nPacketTime < nFillTime && nFillTime - nPacketTime > MAX_TIMESTAMP_GAP)
                    break;
            }
        }

        if(!bHasPackets) // No need to go any further.
            break;
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_RECORDCONTROL */
}

HX_RESULT 
HXNetSource::OnTimeSync(ULONG32 ulCurrentTime)
{
    return HXSource::OnTimeSync(ulCurrentTime + m_lPacketTimeOffSet);
}

HX_RESULT HXNetSource::HandleStreamDone(HX_RESULT status, UINT16 usStreamNumber)
{
#if defined(HELIX_FEATURE_RECORDCONTROL)
    if (m_pRecordControl)
    {
        m_pRecordControl->HandleStreamDone(status, usStreamNumber);
    }
#endif /* HELIX_FEATURE_RECORDCONTROL*/

    return HXR_OK;
}

void HXNetSource::enforceLatencyThreshold()
{
    // Check to see if our latency has gone over the threshold
    if (m_ulLastLatencyCalculation > m_ulMaxLatencyThreshold)
    {
        // We are taking action to restore the latency to an acceptable
        // value. The current implementation will seek the audio player 
        // forward enough so that the latency will be equal to the 
        // preroll value. Seeking the audio player has 2 effects. It 
        // causes the timeline to jump forward which tends to cause 
        // video frames to be blitted quicker or even dropped. It also 
        // causes streamed audio writes to fail because the audio 
        // session's concept of the next write time is farther into the
        // future than what the renderers thinks it should be. Note that
        // renderers are not explicitly notified of this seek forward.
        // They become aware of it through OnTimeSync() calls and writes 
        // to their audio stream objects. From the renderer's point of 
        // view it just looks like it is behind in decoding. This seek 
        // usually triggers the renderer logic that tries to "catch up" 
        // to where it should be.
        //
        // m_ulLastLatencyCalculation is set in HXSource::NewPacketTimeStamp(),
        // but we can't make calls on the audio player in that function 
        // because it will cause a deadlock

        // Compute a seek time that restores the latency
        // to the preroll value

        ULONG32 ulSeekTime = 
            m_ulLastReportedPlayTime + m_ulLastLatencyCalculation - m_ulPreRollInMs;
        
        if (m_pPlayer && m_pPlayer->m_pAudioPlayer)
        {
            if (E_PLAYING == m_pPlayer->m_pAudioPlayer->GetState())
            {
                HXLOGL4(HXLOG_LLLX, "Latency of %u triggered seek %u -> %u", 
                        m_ulLastLatencyCalculation,
                        m_ulLastReportedPlayTime, ulSeekTime);
                
                m_pPlayer->m_pAudioPlayer->Pause();
                m_pPlayer->m_pAudioPlayer->Seek(ulSeekTime);
                m_pPlayer->m_pAudioPlayer->Resume();
            }
            else
            {
                HXLOGL4(HXLOG_LLLX, "Skipping latency triggered seek");
            }
        }
    
        // Clear the calculation so we don't keep getting called
        // when we aren't getting packets
        m_ulLastLatencyCalculation = 0;
    }
}

void HXNetSource::setPacketDelays()
{
    m_latencyModeHlpr.SetTransportDelays();
    
    if (m_pBufferManager)
    {
        m_pBufferManager->SetMaxAdditionalBuffering(
            m_latencyModeHlpr.MaxPrerollIncrement());
    }
}

ULONG32 
HXNetSource::ComputeMaxLatencyThreshold(ULONG32 ulPrerollInMs, 
                                        ULONG32 ulPostDecodeDelay)
{
    ULONG32 ulBaseThreshold = 
        HXSource::ComputeMaxLatencyThreshold(ulPrerollInMs,
                                             ulPostDecodeDelay);

    // Compute a latency jitter value that includes the resend buffer
    // delay and additional buffering because of rebuffers
    ULONG32 ulLatencyJitter = 
        (m_latencyModeHlpr.MaxResendBufferDelay() + 
         m_latencyModeHlpr.MaxPrerollIncrement());

    // Compute a threshold based on the latency jitter
    ULONG32 ulNewThreshold = ulPrerollInMs + ulLatencyJitter;

    // Pick the maximum of the two thresholds
    return HX_MAX(ulBaseThreshold, ulNewThreshold);
}

TransportMode 
HXNetSource::GetTransportFromURL(char* pszURL)
{
    TransportMode transportMode = UnknownMode;
    
    if (pszURL)
    {
        HXURLRep urlRep = HXURLRep(pszURL);
        HXURLUtil::ProtocolInfo urlProtocolInfo = HXURLUtil::GetProtocolInfo(urlRep.Scheme());

        if (HXURLUtil::ProtocolInfo::SCHEME_RTSPU == urlProtocolInfo.type)
        {
            transportMode = UDPMode;
        }
    }

    return transportMode;
}

void HXNetSource::InitABD(IHXAutoBWDetection* pABD)
{
    if (pABD && m_pConnBWInfo)
    {
        m_pConnBWInfo->AddABDInfo(pABD, m_pPreferredTransport);
    }
}

void HXNetSource::ShutdownABD(IHXAutoBWDetection* pABD)
{
    if (pABD && m_pConnBWInfo)
    {
        m_pConnBWInfo->RemoveABDInfo(pABD);
    }
}

ReconnectCallback::ReconnectCallback(HXNetSource*       pSource,
                                     HXBOOL bIsStatsCallback /*= FALSE*/) :
     m_pSource (pSource)
    ,m_PendingHandle (0)
    ,m_bIsStatsReportingCallback(bIsStatsCallback)
    ,m_pScheduler(0)
    ,m_ulScheduleTime(0)
    ,m_ulTimeout(0)
    ,m_bPaused(FALSE)
    ,m_lRefCount (0)
{
    m_pSource->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
}

ReconnectCallback::~ReconnectCallback()
{
    HX_RELEASE(m_pScheduler);
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your
//              object.
//
STDMETHODIMP ReconnectCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCallback*)this }
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) ReconnectCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) ReconnectCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *      IHXPlayerCallback methods
 */
STDMETHODIMP ReconnectCallback::Func(void)
{
    m_PendingHandle         = 0;

    if (m_pSource)
    {
        if (m_bIsStatsReportingCallback)
        {
            return m_pSource->ReportStats();
        }
        else
        {
            return m_pSource->StartReconnect();
        }
    }

    return HXR_OK;
}

HX_RESULT ReconnectCallback::ScheduleCallback(UINT32 ulTimeout)
{
    CancelCallback();

    m_bPaused = FALSE;
    m_ulScheduleTime = HX_GET_TICKCOUNT();
    m_ulTimeout      = ulTimeout;
    m_PendingHandle = m_pScheduler->RelativeEnter(this, m_ulTimeout);

    return HXR_OK;
}

HX_RESULT ReconnectCallback::PauseCallback()
{
    CancelCallback();
    UINT32 ulElapsed = CALCULATE_ELAPSED_TICKS(m_ulScheduleTime, HX_GET_TICKCOUNT());
    if (m_ulTimeout > ulElapsed)
    {
        m_ulTimeout -= ulElapsed;
    }
    else
    {
        // must be really close to sending stats, next time we will schedule for 0 timeout
        m_ulTimeout = 0;
    }
    m_bPaused = TRUE;
    return HXR_OK;
}

HX_RESULT ReconnectCallback::ResumeCallback()
{
    ScheduleCallback(m_ulTimeout);
    return HXR_OK;
}

HX_RESULT ReconnectCallback::CancelCallback()
{
    m_bPaused = FALSE;
    if (m_PendingHandle)
    {
        m_pScheduler->Remove(m_PendingHandle);
        m_PendingHandle = 0;
    }

    return HXR_OK;
}

