/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: multicast_mgr.cpp,v 1.16 2007/03/08 00:17:42 tknox Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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
#include <stdlib.h>


#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "ihxpckts.h"
#include "hxasm.h"
#include "hxdtcvt.h"
#include "fio.h"
#include "sio.h"
#include "bufio.h"
#include "fsio.h"
#include "udpio.h"
#include "source.h"
#include "multicast_mgr.h"
#include "streamer_container.h"
#include "hxstrutl.h"
#include "timeval.h"
#include "hxmap.h"
#include "hxpcktflwctrl.h"
#include "pcktflowwrap.h"
#include "hxprot.h"
#include "config.h"
#include "hxstrutl.h"
#include "hxassert.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "transport.h"
#include "rdt_base.h"
#include "rdt_tcp.h"
#include "rdt_udp.h"
#include "rtspmcast.h"
#include "defslice.h"
#include "servbuffer.h"

#include "addrpool.h"       // IHXMulticastAddressPool
#include "mcstaddr.h"       // MulticastAddressAllocator
#include "base_errmsg.h"
#include "sapmgr.h"         // IHXSapManager
#include "server_context.h"
#include "hxsdesc.h"
#include "hxspriv.h"
#include "chxpckts.h"
#include "hxtick.h"
#include "hxdtcvt.h"
#include "hxreg.h"
#include "server_version.h"

#include "url.h"        //      For URL definition

#if defined _WIN32 && !defined snprintf
#define snprintf _snprintf
#endif // _WIN32

MulticastSession::MulticastSession
(
    IUnknown* pContext,
    const char* pFileName,
    enum MMProtocols eProtocol,
    MulticastManager* pMgr
)
    : m_lRefCount(0)
    , m_lCount(0)
    , m_pContext(pContext)
    , m_pMgr(pMgr)
    , m_eProtocol(eProtocol)
    , m_pPacketFlowWrap(0)
    , m_pTransport(0)
    , m_pSourceControl(0)
    , m_ulAddress(0)
    , m_pUDP(0)
    , m_pSessionControl(NULL)
    , m_pTransportDataConvert(NULL)
    , m_ulStreamCount(0)
    , m_ulStreamHeadersReceived(0)
    , m_ppStreamHeaders(NULL)
    , m_bSAPStarted(FALSE)
{
    m_pContext->AddRef();
    m_pFileName = new_string(pFileName);
}

MulticastSession::~MulticastSession()
{
    // the address should have been released
    HX_ASSERT(0 == m_ulAddress);
    HX_RELEASE( m_pContext );
}

/*
 * Called by RTSP after MulticastSessionSetup().
 */
HX_RESULT
MulticastSession::GetMulticastInfo
(
    UINT16  ulRuleNumber, //XXXSMPNOW Remove me
    UINT32& ulAddress,
    UINT16& unPort,
    UINT32& ulFECAddress,
    UINT16& unFECPort
)
{
    /* Note: RTSP/RDT Doesn't manage it's addresses from this class */

    return HXR_OK;
}


/*
 * RegisterSource() and RegisterStream() will be called on the
 * PacketFlowWrapper by the creator.  We now own the PacketFlowWrapper.
 */
void
MulticastSession::MulticastSessionSetup(PacketFlowWrapper* pPacketFlowWrap,
                                        IHXPSourceControl* pSourceControl,
                                        IHXSessionStats* pSessionStats,
                                        UINT16 unNumStreams,
                                        REF(IHXPacketFlowControl*) pSessionControl,
                                        IHXDataConvertResponse* pDataConvert)
{
    HX_ASSERT(!m_pSourceControl);
    HX_ASSERT(!m_pSessionControl);

    HX_ASSERT(pPacketFlowWrap);
    HX_ASSERT(pSourceControl);

    m_pPacketFlowWrap = pPacketFlowWrap;

    m_pSourceControl = pSourceControl;
    m_pSourceControl->AddRef();
    m_pSourceControl->Init(this);

    if (pDataConvert)
    {
        m_pTransportDataConvert = pDataConvert;
        m_pTransportDataConvert->AddRef();
    }

    // XXX:TDK The cast to a DataConvertShim is REALLY evil, but cleaning it
    // up right would involve changing PPM and MDP, and testing therein. Ugh!
    // So punt for now.
    m_pPacketFlowWrap->RegisterSource(m_pSourceControl, pSessionControl, pSessionStats,
                                     unNumStreams,
                                     FALSE,       /*bUseMDP*/
                                     TRUE,        /*bIsLive*/
                                     FALSE,       /*bIsMCast*/
                                     (DataConvertShim*) pDataConvert);

    m_pSessionControl = pSessionControl;
    m_pSessionControl->AddRef();
}

/*
 * We now own the transport.
 */
Transport*
MulticastSession::GetRTSPTransport(UINT32 ulStreamCount, Transport* pTransport)
{
    if (m_pTransport)
    {
        return m_pTransport;
    }

    switch (m_eProtocol)
    {
        case MMP_RTSP_RDT:
        {
            m_pTransport = new RTSPServerMulticastTransport(m_pContext,
                pTransport, m_pMgr->m_ucMulticastTTL, m_ulAddress, m_pMgr);
            break;
        }
        default:
            PANIC(("Internal Error mm/131"));
    };

    m_pTransport->AddRef();

    return m_pTransport;
}

STDMETHODIMP
MulticastSession::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPSinkControl))
    {
        AddRef();
        *ppvObj = (IHXPSinkControl*) this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP
MulticastSession::InitDone(HX_RESULT               ulStatus)
{
    ASSERT(ulStatus == HXR_OK);

    m_pSourceControl->GetFileHeader(this);

    return HXR_OK;
}

STDMETHODIMP
MulticastSession::FileHeaderReady(
                                HX_RESULT               ulStatus,
                                IHXValues*             pHeader)
{
    UINT32 i = 0;
    HX_RESULT hr = HXR_OK;

    HX_ASSERT(m_pSourceControl);

    if (FAILED(hr = pHeader->GetPropertyULONG32("StreamCount", m_ulStreamCount)))
    {
        return hr;
    }

    if (m_ulStreamCount > 0)
    {
        m_ppStreamHeaders = new IHXValues*[m_ulStreamCount];
        memset((void*)m_ppStreamHeaders, 0, m_ulStreamCount * sizeof(IHXValues*));

        for (i = 0; i < m_ulStreamCount; i++)
        {
            m_pSourceControl->GetStreamHeader(this, i);
        }
    }

    return HXR_OK;
}

STDMETHODIMP
MulticastSession::StreamHeaderReady(
                                HX_RESULT               ulStatus,
                                IHXValues*             pHeader)
{
    UINT32 ulStreamNumber = 0;

    HX_RESULT hr = HXR_OK;

    ASSERT(m_ulStreamHeadersReceived < m_ulStreamCount);


    if (SUCCEEDED(hr = pHeader->GetPropertyULONG32("StreamNumber", ulStreamNumber)))
    {
        m_ppStreamHeaders[ulStreamNumber] = pHeader;
        (m_ppStreamHeaders[ulStreamNumber])->AddRef();
    }

    m_ulStreamHeadersReceived++;

    return hr;
}

STDMETHODIMP
MulticastSession::StreamDone(UINT16                  unStreamNumber)
{
    if (m_pSessionControl)
        m_pSessionControl->StreamDone(unStreamNumber);

    return HXR_OK;
}

STDMETHODIMP
MulticastSession::SeekDone(HX_RESULT               ulStatus)
{
    ASSERT(0);

    return HXR_OK;
}

HX_RESULT
MulticastSession::SubscriptionDone(BYTE* bRuleOn,
                                   REF(UINT32) ulSourcePort,
                                   REF(UINT32) ulPort,
                                   REF(UINT32)ulAddr,
                                   REF(TransportStreamHandler*)pHandler)
{
    HX_RESULT theErr = m_pTransport->SubscriptionDone(bRuleOn,
                                                      ulSourcePort,
                                                      ulPort,
                                                      ulAddr,
                                                      pHandler);
    // save addr so we can release it later
    m_ulAddress = ulAddr;

    if (!m_bSAPStarted)
    {
        HX_ASSERT(m_ulStreamHeadersReceived == m_ulStreamCount);

        if (m_pMgr)
        {
            m_pMgr->StartSap(m_ulAddress, m_pMgr->m_ulRTSPPort, m_ppStreamHeaders, m_ulStreamCount);
            m_bSAPStarted = TRUE;
        }
    }

    return theErr;
}

STDMETHODIMP_(ULONG32)
MulticastSession::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
MulticastSession::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    delete this;
    return 0;
}

void
MulticastSession::AddCount()
{
    m_lCount++;
}

void
MulticastSession::ReleaseCount()
{
    m_lCount--;
    if (m_lCount == 0)
    {
        m_pMgr->UnRegisterMulticast(m_pFileName, m_eProtocol);

        if (m_ulAddress)
        {
            m_pMgr->ReleaseAddress(m_ulAddress);
            m_ulAddress = 0;
        }

        HX_VECTOR_DELETE(m_pFileName);

        if (m_pSourceControl)
        {
            m_pSourceControl->Done();
            m_pSourceControl->Release();
        }

        HX_RELEASE(m_pSessionControl);
        HX_RELEASE(m_pTransport);
        HX_DELETE(m_pPacketFlowWrap);

        if (m_pUDP)
        {
            IHXDescriptorRegistration* pDescReg = NULL;
            m_pContext->QueryInterface(IID_IHXDescriptorRegistration, (void**)&pDescReg);
            if (pDescReg)
            {
                pDescReg->UnRegisterSockets(1);
            }
            HX_RELEASE( pDescReg );
            HX_DELETE(m_pUDP);
        }

        if (m_pTransportDataConvert)
        {
            IHXDataConvert *pConverter = NULL;
            m_pTransportDataConvert->QueryInterface(IID_IHXDataConvert, (void**)pConverter);
            if (pConverter)
            {
                pConverter->Done();
            }
            HX_RELEASE( pConverter );
            HX_RELEASE( m_pTransportDataConvert );
        }

        for (UINT32 i = 0; i < m_ulStreamCount; i++)
        {
            HX_RELEASE(m_ppStreamHeaders[i]);
        }

        HX_VECTOR_DELETE(m_ppStreamHeaders);

        Release();
    }
}

HX_RESULT
MulticastSession::GetFileName(REF(char*) pFileName)
{
    if (m_pFileName)
    {
        pFileName = m_pFileName;
        return HXR_OK;
    }
    else
    {
        pFileName = NULL;
        return HXR_OK;
    }
}

MulticastManager::MulticastManager(IUnknown* pContext)
    : m_ucMulticastTTL(16)
    , m_bSetupOK(FALSE)
    , m_ulRTSPPort(554)
    , m_bMulticastOnly(FALSE)
    , m_bMulticastResend(TRUE)
    , m_pAddrAllocator(NULL)
    , m_pSapMgr(NULL)
    , m_pContext(pContext)
    , m_pStreamDesc(NULL)
#ifdef _DEBUG
    , m_bInitDone(FALSE)
#endif
    ,m_bLicenseCheckDone(FALSE)

{
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);

    m_pRDTMap         = new CHXMapStringToOb;
    m_pSapHandleMap   = new CHXMapLongToObj;

    m_pRDTMutex       = HXMutexCreate();
    m_pSapHandleMutex = HXMutexCreate();

    HXMutexInit(m_pRDTMutex);
    HXMutexInit(m_pSapHandleMutex);

    // multicast address handlers...
    IHXMulticastAddressPool* pAddrPool = NULL;
    HX_RESULT theErr = m_pContext->QueryInterface(
        IID_IHXMulticastAddressPool, (void**)&pAddrPool);
    if (SUCCEEDED(theErr))
    {
        m_pAddrAllocator = new MulticastAddressAllocator();
        m_pAddrAllocator->Init(pAddrPool);
        HX_RELEASE(pAddrPool);
    }
    else
    {
        /* error */
        m_bSetupOK = FALSE;
        return;
    }

    BOOL bEnabled = TRUE;
    INT32 lTemp = 0;
    if (SUCCEEDED(m_pRegistry->GetIntByName("config.Multicast.Enabled", lTemp)))
    {
        bEnabled = (BOOL)lTemp;
    }
    if (SUCCEEDED(m_pRegistry->GetIntByName("config.Multicast.TTL", lTemp)))
    {
        m_ucMulticastTTL = (UINT8)lTemp;
    }
    if (SUCCEEDED(m_pRegistry->GetIntByName("config.Multicast.DeliveryOnly", lTemp)))
    {
        m_bMulticastOnly = (BOOL)lTemp;
    }
    if (SUCCEEDED(m_pRegistry->GetIntByName("config.Multicast.Resend", lTemp)))
    {
        m_bMulticastResend = (BOOL)lTemp;
    }
    if (SUCCEEDED(m_pRegistry->GetIntByName("config.Multicast.RTSPPort", lTemp)))
    {
        m_ulRTSPPort = (UINT32)lTemp;
    }


    IHXBuffer* pBuf = NULL;
    if (SUCCEEDED(m_pRegistry->GetStrByName("config.Multicast.AddressRange", pBuf)))
    {
        char* pMyAddress = new_string((const char *)pBuf->GetBuffer());
        char* pLow = pMyAddress;
        char* pHigh = strstr(pMyAddress, "-");

        if (!pHigh)
        {
            return;
        }

        *(pHigh++) = 0;         // Zero the dash and advance the pointer.

        UINT32 ulLowAddr = ntohl(inet_addr(pLow));
        UINT32 ulHighAddr = ntohl(inet_addr(pHigh));

        // one addr is OK!
        m_bSetupOK = (ulLowAddr &&
                      ulHighAddr &&
                      (ulHighAddr >= ulLowAddr));

        if (m_bSetupOK)
        {
            m_pAddrAllocator->SetAddressRange(ulLowAddr, ulHighAddr);
        }

        delete[] pMyAddress;
    }

    if (!bEnabled)
    {
        m_bSetupOK = FALSE;
    }
}

MulticastManager::~MulticastManager()
{
    HX_ASSERT(m_pRDTMap->IsEmpty());

    HX_DELETE(m_pRDTMap);
    HX_DELETE(m_pSapHandleMap);
    HXMutexDestroy(m_pRDTMutex);
    HXMutexDestroy(m_pSapHandleMutex);

    HX_DELETE(m_pAddrAllocator);

    HX_RELEASE( m_pSapMgr );
    HX_RELEASE( m_pRegistry );
    HX_RELEASE( m_pContext );
}

void
MulticastManager::Init()
{
#ifdef _DEBUG
    m_bInitDone = TRUE;
#endif

    HX_ASSERT(m_pContext);

    if (HXR_OK != m_pContext->QueryInterface(IID_IHXSapManager, (void**)&m_pSapMgr))
    {
        // no announcement...well, it's still OK
        return;
    }

    HX_ASSERT(m_pSapMgr);
    if (!m_pSapMgr->IsAnnouncerEnabled())
    {
        HX_RELEASE(m_pSapMgr);
        return;
    }

    BOOL  bAnnounce = TRUE;    // default TRUE
    INT32 lAnnounceSap = 1;

    if (HXR_OK == m_pRegistry->GetIntByName("config.Multicast.AnnounceSap",
                                                     lAnnounceSap))
    {
        bAnnounce = (BOOL)lAnnounceSap;
    }

    if (!bAnnounce)
    {
        HX_RELEASE(m_pSapMgr);
        return;
    }

    // get sdpplin
    IHXPluginEnumerator*   pEnum;
    IUnknown*               pPlugin;
    INT32                   iPlugin;
    IHXPlugin*              pHXPlugin;

    if (HXR_OK == m_pContext->QueryInterface(IID_IHXPluginEnumerator,
                                                            (void**)&pEnum))
    {
        for (iPlugin = pEnum->GetNumOfPlugins() - 1;
            iPlugin >= 0 && m_pStreamDesc == NULL;
            iPlugin--)
        {
            pEnum->GetPlugin(iPlugin, pPlugin);

            pPlugin->QueryInterface(IID_IHXStreamDescription,
                (void**)&m_pStreamDesc);
            if (m_pStreamDesc)
            {
                // QI IHXPlugin
                pPlugin->QueryInterface(IID_IHXPlugin,
                                    (void**)&pHXPlugin);
                if (pHXPlugin)
                {
                    pHXPlugin->InitPlugin(m_pContext);
                    HX_RELEASE(pHXPlugin);
                }
                else
                {
                    HX_RELEASE(m_pStreamDesc);
                }
            }

            pPlugin->Release();
        }
        pEnum->Release();
    }

    if (!m_pStreamDesc)
    {
        HX_RELEASE(m_pSapMgr);
    }
}

BOOL MulticastManager::SetupOK()
{
    if ( !m_bLicenseCheckDone )
    {
	//This check should only be done after licensing plugin is initialized.
	//It is only done once to match previous behavior.

	INT32 nLicensed = 0;
	// Find out if Events are licensed to be served
	if (HXR_OK == m_pRegistry->GetIntByName(
		REGISTRY_GENERAL_MULTICAST, nLicensed))
	{
	    m_bSetupOK = m_bSetupOK && nLicensed;
	}
	else
	{
	    m_bSetupOK = m_bSetupOK && LICENSE_GENERAL_MULTICAST;
	}
	
	m_bLicenseCheckDone = TRUE;
    }

    return m_bSetupOK;
}

BOOL
MulticastManager::RegisterMulticast
(
    const char* pFileName,
    enum MMProtocols eProtocol,
    MulticastSession*& pMSession,
    IUnknown* pContext
)
{
#ifdef _DEBUG
    HX_ASSERT(m_bInitDone);
#endif
    BOOL  bRetVal = FALSE;
    CHXMapStringToOb* pMap = 0;
    HX_MUTEX pMutex = 0;
    pMSession = 0;

    switch (eProtocol)
    {
        case MMP_RTSP_RDT:
            pMap = m_pRDTMap;
            pMutex = m_pRDTMutex;
            break;
        case MMP_PNA_AUDIO:
        case MMP_PNA_RAW:
            //depreciated
            break;
        default:
            PANIC(("Internal Error mm/90"));
    };

    if (!pMap || !pMutex)
    {
        return FALSE;
    }

    URL* pURL = new URL(pFileName, strlen(pFileName));
    void* pVoid = 0;
    HXMutexLock(pMutex);
    pMap->Lookup(pURL->name, pVoid);
    pMSession = (MulticastSession*) pVoid;

    if (pMSession)
    {
        pMSession->AddCount();
        bRetVal = FALSE;
    }
    else
    {
        // we want a streamer not core
        pMSession = new MulticastSession(pContext, pFileName, eProtocol, this);
        pMSession->AddRef();
        pMSession->AddCount();
        pMap->SetAt(pURL->name, (void *)pMSession);
        bRetVal = TRUE;
    }

    HXMutexUnlock(pMutex);
    HX_DELETE(pURL);
    return bRetVal;
}

void
MulticastManager::UnRegisterMulticast
(
    const char* pFileName,
    enum MMProtocols eProtocol
)
{
#ifdef _DEBUG
    HX_ASSERT(m_bInitDone);
#endif

    HX_MUTEX pMutex = 0;
    MulticastSession* pMSession = 0;
    CHXMapStringToOb* pMap = 0;

    switch (eProtocol)
    {
        case MMP_RTSP_RDT:
            pMap = m_pRDTMap;
            pMutex = m_pRDTMutex;
            break;
        case MMP_PNA_AUDIO:
        case MMP_PNA_RAW:
            //depreciated
            break;
        default:
            PANIC(("Internal Error mm/90"));
    };

    if (!pMap || !pMutex)
    {
        return;
    }

    URL* pURL = new URL(pFileName, strlen(pFileName));
    HXMutexLock(pMutex);
    HX_VERIFY(pMap->RemoveKey(pURL->name));
    HXMutexUnlock(pMutex);
    HX_DELETE(pURL);
}

void
MulticastManager::StartSap(UINT32 ulAddr, UINT32 ulPort, IHXValues** ppStreamHeaders, UINT32 ulStreamCount)
{
#ifdef _DEBUG
    HX_ASSERT(m_bInitDone);
#endif

// SAP disabled.
    if (!m_pSapMgr)
    {
        return;
    }

    HX_ASSERT(m_pStreamDesc);
    // create sdp
    // sap it
    // save handle
    char    szAddr[32];
    char    szTitle[256];
    int     cHeaders = ulStreamCount + 2;

    snprintf (szTitle, 256, "%s address announcement", ServerVersion::ProductName());
    szTitle[255] = '\0';
    memset(szAddr, 0, sizeof(char) * 32);

    IHXBuffer* pSDP = NULL;
    IHXValues** ppHeaders = new IHXValues*[cHeaders];

    int i;
    for (i = 0; i < cHeaders; i++)
    {
        ppHeaders[i] = new CHXHeader();
        ppHeaders[i]->AddRef();
    }

    /***** File Header *****/
    IHXBuffer* pBuf = new ServerBuffer(TRUE);
    pBuf->Set((BYTE*)szTitle, sizeof(szTitle));
    ppHeaders[0]->SetPropertyCString("Title", pBuf);
    HX_RELEASE(pBuf);

    sprintf(szAddr, "%d.%d.%d.%d",
            (ulAddr >> 24) & 0xFF,
            (ulAddr >> 16) & 0xFF,
            (ulAddr >>  8) & 0xFF,
            (ulAddr      ) & 0xFF);

    pBuf = new ServerBuffer(TRUE);
    pBuf->Set((unsigned char*)szAddr, strlen(szAddr) + 1);
    ppHeaders[0]->SetPropertyCString("MulticastAddress", pBuf);
    HX_RELEASE(pBuf);

    ppHeaders[0]->SetPropertyULONG32("MulticastTTL", m_ucMulticastTTL);

    /***** Option *****/
    ppHeaders[1]->SetPropertyULONG32("LastModified", HX_GET_TICKCOUNT());

    for (i = 0; i < ulStreamCount; i++)
    {
        IHXBuffer* pMimeType = NULL;

        if (!ppStreamHeaders[i])
        {
            continue;
        }

        if (SUCCEEDED(ppStreamHeaders[i]->GetPropertyCString("MimeType", pMimeType)))
        {
            (ppHeaders[i + 2])->SetPropertyCString("MimeType", pMimeType);
        }
        HX_RELEASE(pMimeType);
    }


    if (SUCCEEDED(m_pStreamDesc->GetDescription(cHeaders, ppHeaders, pSDP)))
    {
        // sdpplin CSDPStreamDescription::GetDescription() is adding extra byte
        // in this buffer for NULL.  We don't want that since sdr is picky about this.
        // so subtract 1 from it in here....

        pSDP->SetSize(pSDP->GetSize() - 1);

        SapHandle* phSapHandle = new SapHandle;
        *phSapHandle = m_pSapMgr->StartAnnouncement(pSDP, m_ucMulticastTTL);

        m_pSapHandleMap->SetAt(ulAddr, phSapHandle);
        HX_RELEASE(pSDP);
    }

    for (i = 0; i < cHeaders; i++)
    {
        ppHeaders[i]->Release();
    }
    HX_VECTOR_DELETE(ppHeaders);


}

void
MulticastManager::StopSap(UINT32 ulAddr)
{
#ifdef _DEBUG
    HX_ASSERT(m_bInitDone);
#endif

    SapHandle* phSapHandle = NULL;
    if (m_pSapHandleMap->Lookup(ulAddr, reinterpret_cast<void*&>(phSapHandle)))
    {
        m_pSapHandleMap->RemoveKey(ulAddr);

        m_pSapMgr->StopAnnouncement(*phSapHandle);

        delete phSapHandle;
    }
#ifdef _DEBUG
    else
    {
        HX_ASSERT(!"SAP handle is not in the map");
    }
#endif

}

HX_RESULT
MulticastManager::GetNextAddress(REF(UINT32)ulAddr, UINT32 ulPort)
{
#ifdef _DEBUG
    HX_ASSERT(m_bInitDone);
#endif

    HX_RESULT theErr = HXR_OK;
    HX_ASSERT(m_pAddrAllocator);

    theErr = m_pAddrAllocator->GetNextAddress(ulAddr);

    IHXErrorMessages* pErrorHandler = NULL;
    if (FAILED(theErr) && SUCCEEDED(m_pContext->QueryInterface(IID_IHXErrorMessages,
                (void**)&pErrorHandler)))
    {
        // log...
        ERRMSG(pErrorHandler,
            "Error in creating Back-channel multicast session.  Please increase "
            "the AddressRange configuration variable.");

    }
    HX_RELEASE( pErrorHandler );
    return theErr;
}

void
MulticastManager::ReleaseAddress(UINT32 ulAddr)
{
#ifdef _DEBUG
    HX_ASSERT(m_bInitDone);
#endif

    HX_ASSERT(m_pAddrAllocator);

    if (m_pSapMgr)
    {
        StopSap(ulAddr);
    }

    m_pAddrAllocator->ReleaseAddress(ulAddr);
}

