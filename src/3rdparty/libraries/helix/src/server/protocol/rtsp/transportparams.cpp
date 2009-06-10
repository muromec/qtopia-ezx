/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: transportparams.cpp,v 1.20 2009/02/07 06:28:15 jzeng Exp $
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

#include "hxtypes.h"

#include <stdio.h>
#include <stdlib.h>

#include "hxcom.h"
#include "sockio.h"

#include "hxbuffer.h"
#include "hxsbuffer.h"

#include "hxslist.h"
#include "hxmap.h"

#include "hxnet.h"

#include "hxerror.h"

#include "hxprefs.h"
#include "mimehead.h"
#include "mimescan.h"
#include "timerep.h"
#include "rtspmsg.h"
#include "rtsppars.h"

#include "rtspserv.h"
#include "rtspsession.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "servlist.h"

#include "chxrtsptransocket.h"
#include "chxrtcptransmapsocket.h"

#include "rdt_base.h"
#include "rdt_tcp.h"
#include "rdt_udp.h"
#include "rtp_base.h"
#include "rtp_tcp.h"
#include "rtp_udp.h"
#include "nulltran.h"

#include "bdst_stats.h"
#include "bcngtypes.h"
#include "bcngtran.h"

#include "transportparams.h"

#include "ihxlist.h"
#include "mime2.h"

#include "trmimemp.h"
#include "defslice.h"
#include "proc.h"
#include "qos_cfg_names.h"
#if 0
#include "proc.h"
#include "server_trace.h"
#else
#define STRACEX(x)
#define STRACE1(x)
#endif

/*
 * This defines the transports we support.  It is used in the session's
 * selectTransport() function.
 */
#define IS_SUPPORTED_TRANSPORT(t) \
    ((t) > RTSP_TR_NONE && (t) < RTSP_TR_LAST && \
    (t) != RTSP_TR_RTP_MCAST && (t) != RTSP_TR_RTCP)


RTSPTransportParams::RTSPTransportParams() :
        m_lTransportType(RTSP_TR_NONE),
        m_Mode(RTSP_TRMODE_NONE),
        m_sPort(0),
        m_streamNumber(0),
        m_ulBufferDepth(0),
        m_pDestAddr(NULL),
        m_ulStreamID(0),
        m_bAggregateTransport(0),
        m_ulPullSplitID(0),
        m_Protocol(ProtocolType::UDP_UNICAST),
        m_bResendSupported(FALSE),
        m_ucFECLevel(0),
        m_ucTTL(0),
        m_ucSecurityType(0),
        m_pAuthenticationDataField(NULL),
        m_ulRefCount(0)
{
#if 0
    printf("RTSPTransportParams(%p) created\n", this);
#endif

    m_pSockets[0] = m_pSockets[1] = NULL;
}

RTSPTransportParams::~RTSPTransportParams()
{
#if 0
    printf("RTSPTransportParams(%p) destroyed, type %s, Aggregate? %d\n", this,
               RTSPTransportMimeMapper::getTransportMimeType(m_lTransportType),
               m_bAggregateTransport);
#endif

    HX_RELEASE(m_pDestAddr);

    /** If the transport used was UDP we may have a socket pair. We Close()
      * each socket before releasing because at this point we know it will 
      * not be reused. This code is correct even if the transport is also 
      * calling Close() on the socket - redundant Close() calls are harmless */
    if ( m_pSockets[0])
    {
        m_pSockets[0]->Close();
        m_pSockets[0]->Release();
    }

    if ( m_pSockets[1])
    {
        m_pSockets[1]->Close();
        m_pSockets[1]->Release();
    }

    HX_RELEASE(m_pAuthenticationDataField);
}


STDMETHODIMP
RTSPTransportParams::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(UINT32)
RTSPTransportParams::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(UINT32)
RTSPTransportParams::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}



RTSPTransportInstantiator::RTSPTransportInstantiator(BOOL bAggregate)
                     : m_bAggregateTransport(bAggregate)
                     , m_pLocalAddr(NULL)
                     , m_bSelected(FALSE)
                     , m_pBaseProt(NULL)
                     , m_usFirstStream(0)
                     , m_ulRefCount(0)
                     , m_bPreferTCP(FALSE)
                     , m_bPreferRTP(FALSE)
{
#if 0
    printf("RTSPTransportInstantiator(%p) created\n", this);
#endif

    m_RTSPTransportPriorityTable[RTSP_TR_NONE] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_RDT_MCAST] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_RDT_UDP] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_RDT_TCP] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_TNG_UDP] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_TNG_TCP] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_TNG_MCAST] = 4;
#if defined(HELIX_FEATURE_SERVER_PREFER_RTP)
    m_RTSPTransportPriorityTable[RTSP_TR_RTP_UDP] = 3;
    m_RTSPTransportPriorityTable[RTSP_TR_RTP_MCAST] = 3;
    m_RTSPTransportPriorityTable[RTSP_TR_RTP_TCP] = 3;
#else
    m_RTSPTransportPriorityTable[RTSP_TR_RTP_UDP] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_RTP_MCAST] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_RTP_TCP] = 4;
#endif
    m_RTSPTransportPriorityTable[RTSP_TR_RTCP] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_NULLSET] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_BCNG_UDP] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_BCNG_MCAST] = 4;
    m_RTSPTransportPriorityTable[RTSP_TR_BCNG_TCP] = 4;
}

RTSPTransportInstantiator::~RTSPTransportInstantiator()
{
#if 0
    printf("RTSPTransportInstantiator(%p) destroyed\n", this);
#endif

    clearTransportParamsList();
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pBaseProt);
    HX_RELEASE(m_pLocalAddr);
}

void
RTSPTransportInstantiator::clearTransportParamsList(void)
{
    RTSPTransportParams* pParams = NULL;

#if 0
    printf("RTSPTransportInstantiator(%p)::clearTransportParamsList\n", this);
#endif

    while(m_transportParamsList.IsEmpty() == FALSE)
    {
        pParams = (RTSPTransportParams*)m_transportParamsList.RemoveHead();
        HX_RELEASE(pParams);
    }
}

void
RTSPTransportInstantiator::AdjustTransportPriorities()
{
    // "subtract 1" each time a transport is "preferred". 
    // Among equal priority transports, preference is determined 
    // by the client via advertising order

    // In the code below, assignment is used instead of decrementing
    // the original value, because we cannot ensure that
    // AdjustTransportPriorities() is called only once

    // Prefer RTP as transport only for non Real media content
    if (m_bPreferRTP)
    {
        //Reset the transport priorities to prefer RTP
        m_RTSPTransportPriorityTable [RTSP_TR_RTP_TCP] = 3;
        m_RTSPTransportPriorityTable [RTSP_TR_RTP_UDP] = 3;
        m_RTSPTransportPriorityTable [RTSP_TR_RTP_MCAST] = 3;
    }

    if (m_bPreferTCP)
    {
        //Reset the transport priorities to prefer TCP
        m_RTSPTransportPriorityTable [RTSP_TR_RDT_TCP] = 3;
        m_RTSPTransportPriorityTable [RTSP_TR_TNG_TCP] = 3;
        m_RTSPTransportPriorityTable [RTSP_TR_RTP_TCP] = 3;
    }

    if (m_bPreferTCP && m_bPreferRTP)
    {
        m_RTSPTransportPriorityTable [RTSP_TR_RTP_TCP] = 2;
    }
}

/**
 * \brief Init : initialize the RTSP session with its context
 *
 * \param pContext [in]
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPTransportInstantiator::Init(IUnknown* pContext, 
                                const char* pSessionID,
                                IHXQoSSignalBus* pSignalBus,
                                CRTSPBaseProtocol* pServerProtocol)
{
    HX_RESULT hresult = HXR_OK;
	STRACE1(this);
    m_pContext = pContext;
    m_pContext->AddRef();

    m_pBaseProt = pServerProtocol;
    m_pBaseProt->AddRef();

    IHXQoSProfileConfigurator* pProfileConfigurator = NULL;
    if(pSignalBus && SUCCEEDED(pSignalBus->QueryInterface(
                               IID_IHXQoSProfileConfigurator, (void**)&pProfileConfigurator)))
    {
        INT32 lTemp = 0;
        if (SUCCEEDED(pProfileConfigurator->GetConfigInt(QOS_CFG_PREFER_RTP, lTemp))
            && !m_pBaseProt->GetSession(pSessionID)->m_bIsRealDataType)
        {
            m_bPreferRTP = lTemp != 0 ? TRUE : FALSE;
        }
    }

    IHXRegistry* pRegistry = NULL;
    hresult = m_pContext->QueryInterface(IID_IHXRegistry,
                                         (void**)&pRegistry);
    HX_VERIFY(HXR_OK == hresult);
    if (hresult == HXR_OK)
    {
        IHXBuffer* pBuf = NULL;
        INT32 lEnabled = 0;
        INT32 nTmp = 0;
        
        pRegistry->GetIntByName(REGISTRY_TCP_PREF_ENABLED, lEnabled);
        nTmp = 0;
        if ((lEnabled || LICENSE_TCP_PREF_ENABLED) && pProfileConfigurator &&
              SUCCEEDED(pProfileConfigurator->GetConfigInt(QOS_CFG_PREFER_TCP, nTmp)))
        {
            m_bPreferTCP = nTmp != 0 ? TRUE : FALSE;
        }
    }

    HX_RELEASE(pProfileConfigurator);
    HX_RELEASE(pRegistry);

    AdjustTransportPriorities();
    return HXR_OK;
}


STDMETHODIMP
RTSPTransportInstantiator::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(UINT32)
RTSPTransportInstantiator::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(UINT32)
RTSPTransportInstantiator::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    delete this;
    return 0;
}



RTSPTransportParams*
RTSPTransportInstantiator::parseTransportField(IHXMIMEField* pField,
    UINT32 ulStrmCtrlID)
{
    STRACE1(this);
    IHXBuffer* pBufVal = NULL;
    IHXBuffer* pBufAttr = NULL;
    IHXList* pListParam = NULL;
    IHXListIterator* pIterParam = NULL;
    IUnknown* pUnkParam = NULL;
    IHXMIMEParameter* pParam = NULL;
    RTSPTransportParams* pParams = NULL;

    if (pField)        // assume transport type is value
    {
        pParams = new RTSPTransportParams;
        pParams->AddRef();

        pParams->m_Mode = RTSP_TRMODE_PLAY;
        pParams->m_streamNumber = 0xFFFF;
        pParams->m_ulStreamID = ulStrmCtrlID;
        pParams->m_ulBufferDepth = TRANSPORT_BUF_DURATION_UNDEF;
        pParams->m_bAggregateTransport = m_bAggregateTransport;

        pField->GetParamListConst(pListParam);
        pIterParam = pListParam->Begin();
        HX_RELEASE(pListParam);

        // assume first parameter is transport type
        char szTran[80];
        UINT32 nTranLen = 0;
        BOOL bIsBCNG = FALSE;
        szTran[0] = '\0';

        if (pIterParam->HasItem())
        {
            pBufAttr = 0;
            pBufVal = 0;
            pUnkParam = pIterParam->GetItem();
            pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                (void **)&pParam);
            HX_RELEASE(pUnkParam);
            pParam->Get(pBufAttr, pBufVal);
            HX_RELEASE(pParam);
            if (pBufAttr->GetSize() < sizeof(szTran))
            {
                memcpy(szTran, pBufAttr->GetBuffer(), pBufAttr->GetSize());
                nTranLen += pBufAttr->GetSize();
                szTran[nTranLen] = '\0';

                /** we can't do a final determination of the transport
                  * type yet because we need the unicast/multicast parameter
                  * but we can determine if we are BCNG now! */
                RTSPTransportTypeEnum lTransportType = RTSP_TR_NONE;
                lTransportType =
                         RTSPTransportMimeMapper::getTransportType(szTran);

                if ((lTransportType == RTSP_TR_BCNG_UDP) ||
                    (lTransportType == RTSP_TR_BCNG_TCP) ||
                    (lTransportType == RTSP_TR_BCNG_MCAST))
                {
                    bIsBCNG = TRUE;
                }
            }

            HX_RELEASE(pBufAttr);
            HX_RELEASE(pBufVal);
            pIterParam->MoveNext();
        }

        while (pIterParam->HasItem())
        {
            pBufAttr = 0;
            pBufVal = 0;
            pUnkParam = pIterParam->GetItem();
            pUnkParam->QueryInterface(IID_IHXMIMEParameter,
                (void **)&pParam);
            HX_RELEASE(pUnkParam);
            pParam->Get(pBufAttr, pBufVal);
            HX_RELEASE(pParam);

            const char* pAttrName = (const char*)pBufAttr->GetBuffer();
            UINT32 uAttrLen = pBufAttr->GetSize();

            if (pBufVal != NULL && uAttrLen == 11 &&
                     strncasecmp(pAttrName, "client_port", 11) == 0)
            {
                const char* pPort = (const char*)pBufVal->GetBuffer();

                // This should probably be a stricter parse
                pParams->m_sPort = (UINT16)strtol(pPort, NULL, 10);

                // Note we don't even look for a dash, it's just assumed
                // to be there for RTP.  This will change for RTSP 1.1.
            }
            else if (pBufVal != NULL && uAttrLen == 11 &&
                     strncasecmp(pAttrName, "destination", 11) == 0)
            {
                UINT32 uValLen = pBufVal->GetSize();
                if (uValLen < MAX_HOST_LEN)
                {
                    HX_RELEASE(pParams->m_pDestAddr);
                    m_pBaseProt->m_pSock->CreateSockAddr(&pParams->m_pDestAddr);
                    pParams->m_pDestAddr->SetAddr(pBufVal);
                }
            }
            else if (pBufVal != NULL && uAttrLen == 12 &&
                     strncasecmp(pAttrName, "buffer_depth", 12) == 0)
            {
                // Value is terminated by a non-digit (';' or CR/LF) -- TDM
                pParams->m_ulBufferDepth =
                                       atoi((const char*)pBufVal->GetBuffer());
            }
            else if (uAttrLen == 7 &&
                     strncasecmp(pAttrName, "unicast", 7) == 0)
            {
                if (nTranLen + 8 < sizeof(szTran))
                {
                    memcpy(szTran+nTranLen, ";unicast", 8);
                    nTranLen += 8;
                    szTran[nTranLen] = '\0';
                }
            }
            else if (uAttrLen == 9 &&
                     strncasecmp(pAttrName, "multicast", 9) == 0)
            {
                if (nTranLen + 10 < sizeof(szTran))
                {
                    memcpy(szTran+nTranLen, ";multicast", 10);
                    nTranLen += 10;
                    szTran[nTranLen] = '\0';
                }
            }
            else if (pBufVal != NULL && uAttrLen == 4 &&
                     strncasecmp(pAttrName, "mode", 4) == 0)
            {
                UINT32 uValLen = pBufVal->GetSize();
                const char* pVal = (const char*)pBufVal->GetBuffer();

                // Strip quotes
                if (uValLen > 2 && pVal[0] == '"' && pVal[uValLen-1] == '"')
                {
                    uValLen -= 2;
                    pVal++;
                }

                if (uValLen == 4 && strncasecmp(pVal, "play", 4) == 0)
                {
                    pParams->m_Mode = RTSP_TRMODE_PLAY;
                }
                else if (uValLen == 6 && strncasecmp(pVal, "record", 6) == 0)
                {
                    pParams->m_Mode = RTSP_TRMODE_RECORD;
                }
                else
                {
                    pParams->m_Mode = RTSP_TRMODE_OTHER;
                }
            }
            else
            {
                if (bIsBCNG)
                {
#if 0 /// Not used!
                    if (pBufVal != NULL && uAttrLen == 7 &&
                             strncasecmp(pAttrName, "address", 7) == 0)
                    {
                        ulAddress = atol((const char*)pBufVal->GetBuffer());
                    }
#endif
                    if (pBufVal != NULL && uAttrLen == 8 &&
                             strncasecmp(pAttrName, "protocol", 8) == 0)
                    {
                        if (!strncasecmp(
                                (const char *)pBufVal->GetBuffer(), "udp",
                                pBufVal->GetSize()))
                        {
                            pParams->m_Protocol = ProtocolType::UDP_UNICAST;
                        }
                        else if (!strncasecmp(
                                (const char *)pBufVal->GetBuffer(), "tcp",
                                pBufVal->GetSize()))
                        {
                            pParams->m_Protocol = ProtocolType::TCP;
                        }
                        else if (!strncasecmp(
                                (const char *)pBufVal->GetBuffer(), "mcast",
                                pBufVal->GetSize()))
                        {
                            pParams->m_Protocol = ProtocolType::UDP_MCAST;
                        }
                    }
                    else if (pBufVal != NULL && uAttrLen == 16 &&
                             strncasecmp(pAttrName, "resend_supported", 16) == 0)
                    {
                        pParams->m_bResendSupported
                            = atoi((const char*)pBufVal->GetBuffer());
                    }
                    else if (pBufVal != NULL && uAttrLen == 9 &&
                             strncasecmp(pAttrName, "fec_level", 9) == 0)
                    {
                        pParams->m_ucFECLevel
                            = atoi((const char*)pBufVal->GetBuffer());
                    }
                    else if (pBufVal != NULL && uAttrLen == 3 &&
                             strncasecmp(pAttrName, "ttl", 3) == 0)
                    {
                        pParams->m_ucTTL = atoi((const char*)pBufVal->GetBuffer());
                    }
                    else if (pBufVal != NULL && uAttrLen == 8 &&
                             strncasecmp(pAttrName, "sec_type", 8) == 0)
                    {
                        if (!strncasecmp(
                            (const char *)pBufVal->GetBuffer(), "proxypull",
                            pBufVal->GetSize()))
                        {
                            pParams->m_ucSecurityType = BroadcastSecurity::PROXYPULL;
                        }
                        else if (!strncasecmp(
                            (const char *)pBufVal->GetBuffer(), "basic",
                            pBufVal->GetSize()))
                        {
                            pParams->m_ucSecurityType = BroadcastSecurity::BASIC;
                        }
                        else if (!strncasecmp(
                            (const char *)pBufVal->GetBuffer(), "none",
                            pBufVal->GetSize()))
                        {
                            pParams->m_ucSecurityType = BroadcastSecurity::NONE;
                        }
                    }
                    else if (pBufVal != NULL && uAttrLen == 8 &&
                             strncasecmp(pAttrName, "sec_data", 8) == 0)
                    {
                        pParams->m_pAuthenticationDataField = pBufVal;
                        pBufVal->AddRef();
                    }
                    /* no need for sec_len -- TDM */
                    else if (pBufVal != NULL && uAttrLen == 7 &&
                             strncasecmp(pAttrName, "pull_id", 7) == 0)
                    {
                        pParams->m_ulPullSplitID
                            = atol((const char*)pBufVal->GetBuffer());
                    }

                }
            }
            HX_RELEASE(pBufAttr);
            HX_RELEASE(pBufVal);
            pIterParam->MoveNext();
        }
        HX_RELEASE(pIterParam);

        pParams->m_lTransportType =
                          RTSPTransportMimeMapper::getTransportType(szTran);

        // if setup is the first request, we don't know m_bMulticastOK is true or
        // false at this point.  We will add it to the list and decide it later in
        // Session::selectTransport().
        if (pParams->m_lTransportType == RTSP_TR_NONE)
        {
            /// release it and NULL it out (this gets returned to caller)
            HX_RELEASE(pParams);
        }
    }

    return pParams;
}



/**
 * \brief parseTransportHeader - parse the fields in a Transport header
 *
 * parseTransportHeader parses one field from a Transport header and saves the
 * field info in the m_transportParamsList which is a prioritized list of the
 * transports the client supports.
 *
 * \param pField [in]
 * \param ulStrmCtrlID [in]
 * \param bChallengeMet [in] : not used!
 *
 * \return HXR_OK if successful 
 */
HX_RESULT
RTSPTransportInstantiator::parseTransportHeader(IHXMIMEHeader* pHeader,
    UINT32 ulStrmCtrlID)
{
    HX_RESULT rc = HXR_FAIL;
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;

    STRACE1(this);
#if 0
    printf("RTSPTransportInstantiator(%p)::parseTransportHdr, control ID %d\n",
                                                        this, ulStrmCtrlID);
#endif

    /// Process Transport info and store for later use

    /// \todo should clear sessions transport params list here
    /// or ignore or reject if not empty???

    pHeader->GetFieldListConst(pListField);
    pIterField = pListField->Begin();
    HX_RELEASE(pListField);

    while (pIterField->HasItem())
    {
        pUnkField = pIterField->GetItem();
        pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
        HX_RELEASE(pUnkField);

        RTSPTransportParams* pParams;

        pParams = parseTransportField(pField, ulStrmCtrlID);

        if (pParams)
        {
            /// return success if we find any viable transports!
            rc = HXR_OK;

            if (m_transportParamsList.IsEmpty())
            {
                m_transportParamsList.AddTail(pParams);
            }
            else
            {
                /* Perform the prioritization of transport selection */
                LISTPOSITION cur = m_transportParamsList.GetHeadPosition();
                RTSPTransportParams* pCurParams =
                    (RTSPTransportParams*)m_transportParamsList.GetHead();

                UINT16 myPrio = (UINT16)m_RTSPTransportPriorityTable [pParams->m_lTransportType];

                while(cur && pCurParams)
                {
                    UINT16 curPrio = (UINT16)m_RTSPTransportPriorityTable [pCurParams->m_lTransportType];

                    if (myPrio < curPrio)
                    {
                        break;
                    }

                    pCurParams = (RTSPTransportParams*)m_transportParamsList.GetAtNext(cur);
                }

                if (cur == NULL)
                {
                    m_transportParamsList.AddTail(pParams);
                }
                else
                {
                    m_transportParamsList.InsertBefore(cur, pParams);
                }
            }
        }

        HX_RELEASE(pField);
        pIterField->MoveNext();
    }
    HX_RELEASE(pIterField);

    return rc;
}



/**
 * \brief matchSelected - try to match a field in a header to our transport
 *
 * matchSelected -  try to match a field in a "Tranport" or 
 * "Aggregate-Transport"  header to our selected transport. This is 
 * used as a sanity check when we already have a transport (as in 
 * the aggregate case)
 *
 * \param pHeader [in] : the fields in a transport header
 *
 * \return TRUE if we were successful in finding a matching transport
 */
BOOL
RTSPTransportInstantiator::matchSelected(IHXMIMEHeader* pHeader)
{
    IHXList* pListField = NULL;
    IHXListIterator* pIterField = NULL;
    IUnknown* pUnkField = NULL;
    IHXMIMEField* pField = NULL;
    BOOL bMatched = FALSE;

    SPRTSPTransportParams spSelectedParams;
    RTSPTransportParams* pSelectedParams;
    GetTransportParams(spSelectedParams.AsInOutParam());
    
    if (!spSelectedParams.IsValid())
    {
        HX_ASSERT(FALSE);
        return FALSE;
    }

    pSelectedParams = spSelectedParams.Ptr();

    pHeader->GetFieldListConst(pListField);
    pIterField = pListField->Begin();
    HX_RELEASE(pListField);

    while (!bMatched && pIterField->HasItem())
    {
        pUnkField = pIterField->GetItem();
        pUnkField->QueryInterface(IID_IHXMIMEField, (void **)&pField);
        HX_RELEASE(pUnkField);

        RTSPTransportParams* pParams;

        pParams = parseTransportField(pField);

        if (pParams && (*pSelectedParams == *pParams))
        {
            bMatched = TRUE;
#if 0
            printf("RTSPTransportInstantiator(%p)::matchSelected, %p matches!\n", this, pSelectedParams);
#endif
        }

        HX_RELEASE(pParams);

        HX_RELEASE(pField);
        pIterField->MoveNext();
    }

    HX_RELEASE(pIterField);

#if 0
    printf("RTSPTransportInstantiator(%p)::matchSelected, bMatch %d\n", 
       this, bMatched);
#endif
    return bMatched;
}


BOOL
RTSPTransportInstantiator::CanUseTransport(RTSPTransportParams* pParams,
                                       BOOL bAllowRARTP,
                                       BOOL bForceRTP, 
                                       BOOL bAllowDest, 
                                       BOOL bIsRealDataType,
                                       BOOL bMulticastOK,
                                       BOOL bRequireMulticast,
                                       BOOL bIsMidBox,
                                       BOOL bIsFirstSetup)
{
    STRACE1(this);

    if (!IS_SUPPORTED_TRANSPORT(pParams->m_lTransportType))
    {
        return FALSE;
    }

    if (bIsRealDataType)
    {
        // Real Datatypes must use RDT or do the RARTP challenge
        if (IS_CLIENT_TRANSPORT(pParams->m_lTransportType) &&
            !IS_RDT_TRANSPORT(pParams->m_lTransportType))
        {
            if (!bAllowRARTP)
            {
                return FALSE;
            }
        }
    }

    if (!bMulticastOK)
    {
        if (IS_CLIENT_TRANSPORT(pParams->m_lTransportType) &&
            IS_MCAST_TRANSPORT(pParams->m_lTransportType))
        {
            return FALSE;
        }
    }

    if (bForceRTP)
    {
        // Make sure it's either an RTP transport or a server-to-server
        // wrapper such as BCNG.
        if (IS_CLIENT_TRANSPORT(pParams->m_lTransportType) &&
            !IS_RTP_TRANSPORT(pParams->m_lTransportType))
        {
            return FALSE;
        }
    }

    if (pParams->m_sPort == 0 || pParams->m_sPort == 65535)
    {
        // The port is invalid, this must be either a server-to-server
        // or a tcp transport.
        if (IS_CLIENT_TRANSPORT(pParams->m_lTransportType) &&
            !IS_TCP_TRANSPORT(pParams->m_lTransportType))
        {
            return FALSE;
        }
    }

    if (!bAllowDest)
    {
        if (pParams->m_pDestAddr != NULL)
        {
            return FALSE;
        }
    }

    if (bIsFirstSetup && bRequireMulticast)
    {
        if (pParams->m_lTransportType != RTSP_TR_RDT_MCAST &&
            pParams->m_lTransportType != RTSP_TR_NULLSET)
        {
            return FALSE;
        }
    }

    if (!IS_CLIENT_TRANSPORT(pParams->m_lTransportType) &&
        pParams->m_lTransportType != RTSP_TR_NULLSET &&
        !bIsMidBox)
    {
        // A client is not allowed to setup a server-to-server transport
        return FALSE;
    }

    // Looks like it's valid
    return TRUE;
}

/**
 * \brief selectTransport - select a transport to use for data transfer.
 *
 * selectTransport - select a transport to use from the list of transports 
 * the client provided. The rules the server uses to prioritize transports
 * are embedded in this code
 *
 * \param streamNumber [in]
 * \param bAllowRARTP [in] : if TRUE we can use RTP xport with a real datatype 
 * \param bForceRTP [in]
 * \param bAllowDest [in]
 *
 * \return TRUE if we were successful in finding a transport
 */
HX_RESULT
RTSPTransportInstantiator::selectTransport(
             BOOL bAllowRARTP,
             BOOL bForceRTP, 
             BOOL bAllowDest, 
             BOOL bIsRealDataType,
             BOOL bMulticastOK,
             BOOL bRequireMulticast,
             BOOL bIsMidBox,
             BOOL bIsFirstSetup, 
             RTSPTransportParams* pExistingAggregateParams,
             UINT32 ulControlID,
             UINT16 uStreamNumber)
{
#if 0
    printf("RTSPTransportInstantiator(%p)::selectTransport\n" \
                       "pExistingAggregateParams %p\n" \
                       "\tcontrol id             %d\n" \
                       "\tstream number          %d\n" \
                       "\tbAllowRARTP            %d\n" \
                       "\tbForceRTP              %d\n" \
                       "\tbAllowDest             %d\n" \
                       "\tbIsRealDataType        %d\n" \
                       "\tbMulticastOK           %d\n" \
                       "\tbRequireMulticast      %d\n" \
                       "\tbIsMidBox              %d\n" \
                       "\tbIsFirstSetup          %d\n",
            this, pExistingAggregateParams, (int)ulControlID, 
            (int)uStreamNumber,
            bAllowRARTP, bForceRTP, bAllowDest, bIsRealDataType,
            bMulticastOK, bRequireMulticast, bIsMidBox, bIsFirstSetup);
#endif

    STRACE1(this);
    LISTPOSITION pos;
    RTSPTransportParams* pParams;
    RTSPTransportParams* pSelectedParams = NULL;

    if (pExistingAggregateParams)
    {
        BOOL bMatch = FALSE;

        if (CanUseTransport(pExistingAggregateParams, bAllowRARTP, 
                     bForceRTP, bAllowDest, bIsRealDataType, bMulticastOK,
                     bRequireMulticast, bIsMidBox, bIsFirstSetup))
        {
            pos = m_transportParamsList.GetHeadPosition();
            while (pos)
            {
                pParams = (RTSPTransportParams*)m_transportParamsList.GetNext(pos);

                if (*pParams == *pExistingAggregateParams)
                {
                    bMatch = TRUE;
                }
            }

            /** if we have a match we want to remove all the entries 
                in our list and replace them with the aggregate 
                (which already has a socket - so we are ready to go) */
            if (bMatch)
            {
                clearTransportParamsList();
        
                m_transportParamsList.AddTail(pExistingAggregateParams);
                pExistingAggregateParams->AddRef();
        
                m_bSelected = TRUE;
#if 0
                printf("RTSPTransportInstantiator(%p)::selectTransport, " \
                       " existing agg %p matches!\n", 
                       this, pExistingAggregateParams);
#endif
                return HXR_OK;
            }
#if 0
            else
            {
                printf("RTSPTransportInstantiator(%p)::selectTransport," \
                       "no match for existing aggregate %p\n", 
                       this, pExistingAggregateParams);
            }
#endif
        }
    }

    if (bIsFirstSetup)
    {
        m_usFirstStream = uStreamNumber;
    }

    /* Find the first valid transport for the given stream */
    pos = m_transportParamsList.GetHeadPosition();
    while (pos)
    {
        pParams = (RTSPTransportParams*)m_transportParamsList.GetNext(pos);

        if (!m_bAggregateTransport)
        {
            if (pParams->m_ulStreamID != ulControlID)
            {
                continue;
            }
        }

        pParams->m_streamNumber = uStreamNumber;

        if (!CanUseTransport(pParams,
                             bAllowRARTP,
                             bForceRTP,
                             bAllowDest,
                             bIsRealDataType,
                             bMulticastOK,
                             bRequireMulticast,
                             bIsMidBox,
                             bIsFirstSetup))
        {
            continue;
        }

        // Looks like it's valid
        pSelectedParams = pParams;
        break;
    }

    /* Remove unused transports for the given stream */
    pos = m_transportParamsList.GetHeadPosition();
    while (pos)
    {
        LISTPOSITION curpos = pos;
        pParams = (RTSPTransportParams*)m_transportParamsList.GetNext(pos);

        if (m_bAggregateTransport || (pParams->m_ulStreamID == ulControlID))
        {
            if (pParams != pSelectedParams)
            {
                m_transportParamsList.RemoveAt(curpos);
                HX_RELEASE(pParams);
            }
        }
    }

    /** At this point if there is a selected transport the list will
      * either have a single set of transport params in it (if its
      * an aggregate) or one set of params per stream that has been 
      * setup so far */

    if (pSelectedParams)
    {
        /** We have selected a transport, now we need to know whether
          * we need to create a socket or share one.
          *
          * We need to create a socket if :
          *        the transport type is one of the UDP transports
          *        AND the transport is RTP OR its RDT and its the 
          *        first setup (later setups share the same channel 
          *        with RDT)
          *
          * Note that if the transport is an aggregate createUDPSockets()
          * will only actually create the socket the first time through.
          * If its BCNG the underlying transport creates the socket, we
          * never create here.
          */
        HX_RESULT rc = HXR_OK;
        BOOL bIsRDTUDPClient = 
                 (IS_CLIENT_TRANSPORT(pSelectedParams->m_lTransportType) &&
                  !IS_TCP_TRANSPORT(pSelectedParams->m_lTransportType) &&
                  IS_RDT_TRANSPORT(pSelectedParams->m_lTransportType));

        m_bSelected = TRUE;

        if (!m_bAggregateTransport && bIsRDTUDPClient && !bIsFirstSetup)
        {
            /// share the first UDP socket created if RDTvUDP
            SPRTSPTransportParams spFirstParams;

            GetTransportParams(spFirstParams.AsInOutParam(), m_usFirstStream);

            HX_ASSERT(IS_RDT_TRANSPORT(spFirstParams->m_lTransportType));

            pSelectedParams->m_pSockets[0] = spFirstParams->m_pSockets[0];
            pSelectedParams->m_pSockets[0]->AddRef();
        }
        else
        {
            /// we don't need a socket if this is BCNG - special case
            if (IS_UDP_TRANSPORT(pSelectedParams->m_lTransportType) &&
                !IS_BCNG_TRANSPORT(pSelectedParams->m_lTransportType))
            {
                rc = createUDPSockets(pSelectedParams);
            }
        }

        if (!SUCCEEDED(rc))
        {
            pSelectedParams = NULL;
            m_bSelected = FALSE;
        }
    }

#if 0
    printf("RTSPTransportInstantiator(%p)::selectTransport, selected %p, %s\n", 
               this, pSelectedParams, pSelectedParams ?
               RTSPTransportMimeMapper::getTransportMimeType(
                            pSelectedParams->m_lTransportType) : "NULL");
#endif

    return (pSelectedParams != NULL ? HXR_OK : HXR_FAIL);
}


/**
 * \brief GetTransportParams - return the transport information for a given stream
 *
 * \param ppParams [in] RTSPTransportParams** holds return result
 * \param streamNumber [in]
 *
 * \return HXR_OK if found, HXR_FAIL otherwise (and pointer returned is 
 *         NULL)
 */

HX_RESULT
RTSPTransportInstantiator::GetTransportParams(RTSPTransportParams** ppParams,
                                          UINT16 streamNumber)
{
    STRACE1(this);

    /*
     * Note this function must only be called _after_ selectTransport()
     * or m_bSelected will be false and we will return NULL.
     */
    LISTPOSITION pos;
    RTSPTransportParams* pParams;
    *ppParams = NULL;

    if (!m_bSelected)
    {
        return HXR_FAIL;
    }

    pos = m_transportParamsList.GetHeadPosition();
    while (pos)
    {
        pParams = (RTSPTransportParams*)m_transportParamsList.GetNext(pos);

        if (m_bAggregateTransport || 
                (pParams->m_streamNumber == streamNumber))
        {
            pParams->AddRef();
            *ppParams = pParams;
            return HXR_OK;
        }
    }

    return HXR_FAIL;
}


HX_RESULT
RTSPTransportInstantiator::SetBCNGParameters(UINT16 uStreamNumber,
                                         UINT32 ulSessionID, UINT32 ulStartTime,  
                                         UINT16 usBCNGPort, UINT8 ucTCPInterleave)
{
    SPRTSPTransportParams spParams;
    GetTransportParams(spParams.AsInOutParam(), uStreamNumber);
            
    if (!spParams.IsValid())
    {   
        return HXR_FAIL;
    }

    spParams->m_ulBCNGSessionID = ulSessionID;
    spParams->m_ulBCNGStartTime = ulStartTime;
    spParams->m_ucBCNGTCPInterleave = ucTCPInterleave;
    spParams->m_usBCNGPort = usBCNGPort;

    return HXR_OK;
}



/**
 * \brief MakeTransportHeader - make the "Transport" header for the SETUP response
 *
 * \param pSession [in] : the RTSP session that holds the transport information
 * \param pParams [in] : the transport information for the stream we are handling
 *
 * \return pointer to the new header
 */
IHXMIMEHeader*
RTSPTransportInstantiator::MakeTransportHeader (BOOL bAggregate, UINT16 uStreamNumber)
{
    STRACE1(this);

    SPRTSPTransportParams pParams;
    GetTransportParams(pParams.AsInOutParam(), uStreamNumber);

    if (!pParams.IsValid())
    {
        return NULL;
    }

    IHXCommonClassFactory* pCCF;
    IHXBuffer* pBuf = 0;

    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**) &pCCF);

    if (HXR_OK != pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf))
    {
        HX_RELEASE(pCCF);
        return NULL;
    }

    if (bAggregate)
    {
        pBuf->Set((BYTE *)"Aggregate-Transport", 19);
    }
    else
    {
        pBuf->Set((BYTE *)"Transport", 9);
    }

    IHXMIMEHeader* pHeader = new CMIMEHeader(NULL);
    pHeader->SetKey(pBuf);
    HX_RELEASE(pBuf);
    const char* pTransportType = RTSPTransportMimeMapper::getTransportMimeType(
            pParams->m_lTransportType);

    char value[512];
    char* p = value;
    p += sprintf(value, "%s", pTransportType);

    IHXBuffer* pSourceAddrBuf = NULL;
    UINT16 nSourcePort;
    UINT16 nDestPort;
    m_pBaseProt->GetSourceAddr(pSourceAddrBuf);

    IHXSockAddr* pLocalAddr = NULL;
    switch(pParams->m_lTransportType)
    {
        case RTSP_TR_RTP_UDP:
            HX_ASSERT(pParams->m_pSockets[0]);
            pParams->m_pSockets[0]->GetLocalAddr(&pLocalAddr);
            nSourcePort = pLocalAddr->GetPort();
            nDestPort = pParams->m_sPort;
            p += sprintf(p, ";client_port=%hu-%hu", nDestPort, nDestPort+1);
            p += sprintf(p, ";server_port=%hu-%hu", nSourcePort, nSourcePort+1);

            if (pSourceAddrBuf != NULL)
            {
                p += sprintf(p, ";source=%s", (const char*)pSourceAddrBuf->GetBuffer());
            }
            HX_RELEASE(pLocalAddr);
            break;

        case RTSP_TR_TNG_TCP:
        case RTSP_TR_RDT_TCP:
            p += sprintf(p, ";interleaved=%d",
                m_pBaseProt->m_tcpInterleave);
            m_pBaseProt->m_tcpInterleave++; // for next stream
            break;

        case RTSP_TR_RTP_TCP:
            // we need to include RTCP channel...
            p += sprintf(p, ";interleaved=%d-%d",
                m_pBaseProt->m_tcpInterleave,
                m_pBaseProt->m_tcpInterleave+1);
            m_pBaseProt->m_tcpInterleave += 2; // for next stream
            break;

        case RTSP_TR_TNG_UDP:
        case RTSP_TR_RDT_UDP:
        case RTSP_TR_RDT_MCAST:
            HX_ASSERT(pParams->m_pSockets[0]);

            pParams->m_pSockets[0]->GetLocalAddr(&pLocalAddr);
            nSourcePort = pLocalAddr->GetPort();
            nDestPort = pParams->m_sPort;
            p += sprintf(p, ";client_port=%hu", nDestPort);
            p += sprintf(p, ";server_port=%hu", nSourcePort);

            if (pSourceAddrBuf != NULL)
            {
                p += sprintf(p, ";source=%s", (const char*)pSourceAddrBuf->GetBuffer());
            }

            HX_RELEASE(pLocalAddr);
            break;

        case RTSP_TR_BCNG_UDP:
        case RTSP_TR_BCNG_TCP:
        case RTSP_TR_BCNG_MCAST:
            p += sprintf(p, ";session_id=%lu", pParams->m_ulBCNGSessionID);
            p += sprintf(p, ";start_time=%lu", pParams->m_ulBCNGStartTime);
            p += sprintf(p, ";interleaved=%d", pParams->m_ucBCNGTCPInterleave);
            p += sprintf(p, ";low_port=%hu", pParams->m_usBCNGPort);
            p += sprintf(p, ";high_port=%hu", pParams->m_usBCNGPort);

            break;
        default:
            /// \todo proc.h is included for DPRINTF, be nice to replace
            /// with a better trace mechanism
            DPRINTF(D_INFO, ("Unrecognized transport type %d\n",
                    pParams->m_lTransportType));
            break;
    }
    if (HXR_OK != pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf))
    {
        HX_RELEASE(pCCF);
        return 0;
    }

#if 0
    printf("RTSPTransportInstantiator(%p)::MakeTransportHeader %s\n", this, value);
#endif

    pBuf->Set((BYTE *)value, p-value);
    pHeader->SetValueFromBuffer(pBuf);
    HX_RELEASE(pBuf);
    HX_RELEASE(pCCF);
    return pHeader;
}


/**
 * \brief GetUdpPortRange - returns the min and max UDP ports that the server can use
 *
 * \param nMin [out] : the min port from the preferences (or MIN_UDP_PORT)
 * \param nMax [out] : the max port from the preferences (or MAX_UDP_PORT)
 *
 * \return HXR_OK
 */
void
RTSPTransportInstantiator::GetUdpPortRange(UINT16& nMin, UINT16& nMax)
{
    STRACE1(this);
    IHXPreferences* pPreferences = NULL;
    IHXBuffer* pBuffer = NULL;
    nMin = MIN_UDP_PORT;
    nMax = MAX_UDP_PORT;
    if (HXR_OK != m_pContext->QueryInterface(IID_IHXPreferences,
                                              (void**)&pPreferences))
    {
        return;
    }

    /// If the MaxUDPPort Preference is set, use that instead of our defined limit
    if (pPreferences->ReadPref("MinUDPPort", pBuffer) == HXR_OK)
    {
        nMin = atoi((const char*)pBuffer->GetBuffer());
        nMin = (nMin & 0xfffe);
        pBuffer->Release();

        if (nMin < MIN_UDP_PORT)
        {
            nMin = MIN_UDP_PORT;
        }
    }
    if (pPreferences->ReadPref("MaxUDPPort", pBuffer) == HXR_OK)
    {
        nMax = atoi((const char*) pBuffer->GetBuffer());
        nMax = 1 + (nMax & 0xfffe);
        pBuffer->Release();

        if (nMax <= MIN_UDP_PORT)
        {
            nMax = MIN_UDP_PORT+1;
        }
    }

    pPreferences->Release();
}



/**
 * \brief createUDPSockets : create and bind UDP socket(s) for data transfer
 *
 * createUDPSockets opens the UDP socket (RDT) or pair of sockets (RTP) that
 * will be used by a UDP transport for data packets.
 *
 * \param nSockets [in] : 1 for RDT, 2 for RTP
 * \param ppSockets [out] : an array for the created sockets to be returned in
 * \param pLocalAddr [in] : local addr to bind to : NULL lets socket select
 *
 * \return HXR_OK if successful
 */
HX_RESULT
RTSPTransportInstantiator::createUDPSockets(RTSPTransportParams* pParams)
{
    STRACE1(this);
    HX_RESULT rc = HXR_OK;
    UINT16 nMinUDPPort = MIN_UDP_PORT;
    UINT16 nMaxUDPPort = MAX_UDP_PORT;
    IHXNetServices* pNetSvc = NULL;
    UINT n;
    UINT16 nStart, nPort;
    IHXSockAddr* pBindAddr = NULL;
    UINT32 nSockets;
    IHXSocket** ppSockets;
    HX_RESULT hxr;

    if (!pParams)
    {
        return HXR_FAIL;
    }

    /** If we already created (aggregate transports can come in here more
     *  than once) or if this is a TCP transport, we just return success */
    if (pParams->m_pSockets[0])
    {
        return HXR_OK;
    }

    if (IS_TCP_TRANSPORT(pParams->m_lTransportType))
    {
        return HXR_OK;
    }

    if (IS_RDT_TRANSPORT(pParams->m_lTransportType))
    {
        nSockets = 1;
    }
    else if (IS_RTP_TRANSPORT(pParams->m_lTransportType))
    {
        nSockets = 2;
    }
    else
    {
        return HXR_UNEXPECTED;
    }
    
    ppSockets = pParams->m_pSockets;

    if (HXR_OK != m_pContext->QueryInterface(IID_IHXNetServices,
                                        (void**)&pNetSvc))
    {
        return HXR_OUTOFMEMORY;
    }

    GetUdpPortRange(nMinUDPPort, nMaxUDPPort);

    /// we need to make sure the refcount of the address is exactly ONE,
    /// because we need to call SetPort.
    hxr = m_pLocalAddr->Clone(&pBindAddr);
    if (FAILED(hxr))
    {
        HX_RELEASE(pNetSvc);
        return hxr;
    }

    nStart = nMinUDPPort + (rand() % (nMaxUDPPort - nMinUDPPort));
    nStart &= 0xfffe;
    nPort = nStart;

    pBindAddr->SetPort(nPort);

    IHXSockAddr* pPeerAddr = getPeerAddress(pParams);
    if (!pPeerAddr)
    {
        HX_RELEASE(pNetSvc);
        HX_RELEASE(pBindAddr);
        return HXR_FAIL;
    }

    IHXSockAddr* pSingleRefPeerAddr = NULL;

    hxr = pPeerAddr->Clone(&pSingleRefPeerAddr);
    if (FAILED(hxr))
    {
        HX_RELEASE(pPeerAddr);
        HX_RELEASE(pNetSvc);
        HX_RELEASE(pBindAddr);
        return hxr;
    }

    HX_RELEASE(pPeerAddr);

    pSingleRefPeerAddr->SetPort(pParams->m_sPort);

    BOOL bSuccess;
    do
    {
        bSuccess = TRUE;
        for (n = 0; n < nSockets; n++)
        {
            pNetSvc->CreateSocket(&ppSockets[n]);
        }
        for (n = 0; n < nSockets; n++)
        {
            rc = ppSockets[n]->Init(pBindAddr->GetFamily(),
                    HX_SOCK_TYPE_UDP,
                    HX_SOCK_PROTO_ANY);
	    if (rc != HXR_OK)
            {
                bSuccess = FALSE;
                break;
            }
            rc = ppSockets[n]->Bind(pBindAddr);
            if (rc != HXR_OK)
            {
                bSuccess = FALSE;
                break;
            }

            /** \todo undocumented behavior has been to "Connect" RDT UDP
             *  sockets but not RTP. Is this really the intended behavior? */
            if ((nSockets == 1) && (pSingleRefPeerAddr != NULL))
            {
                rc = ppSockets[n]->ConnectToOne(pSingleRefPeerAddr);
		if (rc != HXR_OK)
                {
                    bSuccess = FALSE;
                    break;
                }
            }

            pBindAddr->SetPort(pBindAddr->GetPort()+1);
        }

        if (!bSuccess)
        {
            for (n = 0; n < nSockets; n++)
            {
                ppSockets[n]->Close();
                HX_RELEASE(ppSockets[n]);
            }

            nPort += 2;

            if (nPort >= nMaxUDPPort)
            {
                nPort = nMinUDPPort;
            }

            pBindAddr->SetPort(nPort);
        }
    }
    while (!bSuccess && nPort != nStart);

    HX_RELEASE(pNetSvc);
    HX_RELEASE(pBindAddr);
    HX_RELEASE(pSingleRefPeerAddr);

    if (bSuccess)
    {
	rc = HXR_OK;
    }
    else
    {
	rc = HXR_BAD_TRANSPORT;
    }
    return rc;
}


HX_RESULT
RTSPTransportInstantiator::setLocalAddress(IHXSockAddr* pLocalAddr)
{
    HX_RELEASE(m_pLocalAddr);
    m_pLocalAddr = pLocalAddr;

    if (m_pLocalAddr != NULL)
    {
        m_pLocalAddr->AddRef();
    }
    else
    {
        return HXR_UNEXPECTED;
    }

    return HXR_OK;
}

BOOL
RTSPTransportInstantiator::DataCapableTransportExists(void)
{
    LISTPOSITION pos;
    RTSPTransportParams* pParams;

    pos = m_transportParamsList.GetHeadPosition();
    while (pos)
    {
        pParams = (RTSPTransportParams*)m_transportParamsList.GetNext(pos);

        if (pParams->m_lTransportType != RTSP_TR_NULLSET)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief getPeerAddress - get socket address we will be sending to
 *
 * _FinishPlaynow is called to complete processing of the PLAYNOW request, including
 * populating the Aggregate-Transport header, and send the response to the client.
 *
 * \param pTransParams [in] : contains destination address transport param 
 *                            if it was present in transport header
 *
 * \return pointer to IHXSockAddr with peer address
 */
IHXSockAddr*
RTSPTransportInstantiator::getPeerAddress(RTSPTransportParams* pTransParams)
{
    HX_ASSERT(m_pBaseProt);
    HX_ASSERT(m_pBaseProt->m_pSock);
    IHXSockAddr* pPeerAddr = NULL;

    if (m_pBaseProt->m_pProxyPeerAddr!= NULL)
    {
        pPeerAddr = m_pBaseProt->m_pProxyPeerAddr;
        pPeerAddr->AddRef();
    }
    else if (pTransParams && (pTransParams->m_pDestAddr != NULL))
    {
        IHXRegistry* pRegistry = NULL;

        m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry);

        /** Note that m_pDestAddr for the selected transport must be NULL
         *  unless "config.SupportRTSPDestinationAddress" was set */
        pPeerAddr = pTransParams->m_pDestAddr;
        pPeerAddr->AddRef();

        INT32 lDestLogging = 0;
        if ((pRegistry->GetIntByName("config.RTSPDestinationLogging",
                                       lDestLogging) == HXR_OK) &&
            lDestLogging != 0)
        {
            IHXSockAddr* pRtspPeerAddr = NULL;
            IHXBuffer* pRtspPeerAddrBuf = NULL;
            IHXBuffer* pDestPeerAddrBuf = NULL;
            m_pBaseProt->m_pSock->GetPeerAddr(&pRtspPeerAddr);
            pRtspPeerAddr->GetAddr(&pRtspPeerAddrBuf);
            HX_RELEASE(pRtspPeerAddr);
            pPeerAddr->GetAddr(&pDestPeerAddrBuf);

            char logMsg[64+2*HX_ADDRSTRLEN];
            snprintf(logMsg, sizeof(logMsg), "client IP %s redirecting to %s\n",
                    (const char*)pRtspPeerAddrBuf->GetBuffer(),
                    (const char*)pDestPeerAddrBuf->GetBuffer());
            m_pBaseProt->m_pErrorMessages->Report(HXLOG_INFO, 0, 0, logMsg, NULL);
            HX_RELEASE(pDestPeerAddrBuf);
            HX_RELEASE(pRtspPeerAddrBuf);
        }
    }
    else
    {
        m_pBaseProt->m_pSock->GetPeerAddr(&pPeerAddr);
    }

    HX_ASSERT(pPeerAddr);

    return pPeerAddr;
}

/**
 * \brief SetupTransportTNGUdpRDTUdpMcast - find or create an RDT UDP transport for this stream
 *
 * SetupTransportTNGUdpRDTUdpMcast will create an RDT UDP transport object for
 * this session, or re-use the existing one if this is not the first stream.
 * The UDP socket pair is created when the transport is so that we know that
 * we can use the ports in the SETUP response.
 *
 *
 * \return HXR_OK unless we could not get a UDP socket
 */
HX_RESULT
RTSPTransportInstantiator::SetupTransportTNGUdpRDTUdpMcast(
    RTSPServerSession* pSession,
    RTSPStreamInfo* pStreamInfo,
    RTSPTransportParams* pTransParams,
    UINT16 usStreamNumber)
{
    HX_RESULT rc = HXR_OK;

    if(pSession->m_sSetupCount == 1)
    {
        /* FeatureLevel 2+ implies Packet Aggregation Support */
        RDTUDPTransport* pTransport = new RDTUDPTransport(TRUE, !m_pBaseProt->m_bSetupRecord,
                                         pTransParams->m_lTransportType,
                                         pSession->m_ulRDTFeatureLevel,
                                         m_pBaseProt->m_bDisableResend);
        pTransport->AddRef();

        pSession->m_pTransportList->AddTail(pTransport);

        pTransport->addStreamInfo(pStreamInfo,
                                  pTransParams->m_ulBufferDepth);

        IHXSocket* pUDPSocket = NULL;

        /** UDP sockets are created in the RTSPTransportInstantiator class when
          * the correct transport is chosen. */
        HX_ASSERT(pTransParams->m_pSockets[0]);

        if (pTransParams->m_pSockets[0])
        {
            pUDPSocket = pTransParams->m_pSockets[0];
            pUDPSocket->AddRef();
        }
        else
        {
            HX_ASSERT(FALSE);
            HX_RELEASE(pTransport);
            return HXR_BAD_TRANSPORT;
        }

        rc = pTransport->init(m_pContext, pUDPSocket, m_pBaseProt);
        pTransport->IncrProtocolCount();

        /** If this is an aggregate transport we need to tell the transport
          * because it will close the socket when it is torn down otherwise.
          * This is only relevant with UDP.
          */
        if (pTransParams->m_bAggregateTransport)
        {
            pTransport->SetAsAggregate();
        }

        IHXSockAddr* pPeerAddr = getPeerAddress(pTransParams);
        if (!pPeerAddr)
        {
            HX_ASSERT(FALSE);
            HX_RELEASE(pTransport);
            return HXR_BAD_TRANSPORT;
        }

        IHXSockAddr* pSingleRefPeerAddr;
        pPeerAddr->Clone(&pSingleRefPeerAddr);
        HX_RELEASE(pPeerAddr);

        pSingleRefPeerAddr->SetPort(pTransParams->m_sPort);

        pTransport->setPeerAddr(pSingleRefPeerAddr);
        HX_RELEASE(pSingleRefPeerAddr);

        pTransport->setSessionID(pSession->m_sessionID);
        pSession->mapTransportStream(pTransport, usStreamNumber);
        pSession->m_bIsTNG = TRUE;
        m_pBaseProt->AddTransport(pTransport, pSession->m_sessionID,
                              usStreamNumber, 0);
        HX_RELEASE(pUDPSocket);
    }
    else
    {
        Transport* pTransport = pSession->getFirstTransportSetup();

        if(pTransport)
        {
            pSession->mapTransportStream(pTransport,
                                         usStreamNumber);
            pTransport->addStreamInfo(pStreamInfo,
                                      pTransParams->m_ulBufferDepth);
            m_pBaseProt->AddTransport(pTransport,
                                  pSession->m_sessionID,
                                  usStreamNumber, 0);
        }
    }

    return rc;
}

/**
 * \brief SetupTransportTNGTcpRDTTcp - find or create an RDT TCP transport for this stream
 *
 * SetupTransportTNGTcpRDTTcp will create an RDT TCP transport object for
 * this session and map the stream to it, or re-use the existing transport
 * if this is not the first stream.
 *
 * \return HXR_OK unless we fail to initialize the transport
 */
HX_RESULT
RTSPTransportInstantiator::SetupTransportTNGTcpRDTTcp(
    RTSPServerSession* pSession,
    RTSPStreamInfo* pStreamInfo,
    UINT16 usStreamNumber)
{
    HX_RESULT rc = HXR_OK;

    if (!m_pBaseProt->m_pSock)
    {
        return HXR_FAIL;
    }

    if (pSession->m_sSetupCount == 1)
    {
        IHXSocket* pSock = new CHXRTSPTranSocket(m_pBaseProt->m_pSock);

        /*
         * Parameter !m_bSetupRecord means that we are not handling
         * a record request, ie we are a source
         * Parameter !m_bSendLostPackets means that we want this
         * TCPTransport to
         * not forward on lost packets because it will be talking
         * to a player and players don't like them.
         */
        RDTTCPTransport* pTransport = new RDTTCPTransport(!m_pBaseProt->m_bSetupRecord,
                                         !m_pBaseProt->m_bSendLostPackets,
                                         pSession->m_ulRDTFeatureLevel);

        pTransport->AddRef();

        pSession->m_pTransportList->AddTail(pTransport);
        pTransport->addStreamInfo(pStreamInfo);
        pSession->mapTransportStream(pTransport, usStreamNumber);
        pSession->mapTransportChannel(pTransport,
                                     m_pBaseProt->m_tcpInterleave);

        m_pBaseProt->AddTransport(pTransport,
                              pSession->m_sessionID,
                              usStreamNumber,
                              0);

        rc = pTransport->init(m_pContext,
                         pSock, m_pBaseProt->m_tcpInterleave,
                         m_pBaseProt);

        pTransport->IncrProtocolCount();
		IUnknown* punkItem = NULL;
        pSock->QueryInterface(IID_IUnknown, (void**)&punkItem);
        (*(m_pBaseProt->m_pWriteNotifyMap))[pTransport] = punkItem;

        pTransport->setSessionID(pSession->m_sessionID);

        (*(m_pBaseProt->m_pSessionIDList))[m_pBaseProt->m_tcpInterleave] =
          new_string(pSession->m_sessionID);
    }
    else
    {
        Transport* pTransport = pSession->getFirstTransportSetup();
        if (pTransport)
        {
            pSession->mapTransportStream(pTransport,
                                         usStreamNumber);
            pTransport->addStreamInfo(pStreamInfo);
            pSession->mapTransportChannel(
                pTransport, m_pBaseProt->m_tcpInterleave);

            m_pBaseProt->AddTransport(pTransport, pSession->m_sessionID,
                                  usStreamNumber, 0);
        }


        (*(m_pBaseProt->m_pSessionIDList))[m_pBaseProt->m_tcpInterleave] =
          new_string(pSession->m_sessionID);
    }
    return rc;
}

/**
 * \brief SetupTransportRTPUdp - create an RDT UDP transport for this stream
 *
 * SetupTransportRTPUdp creates an RTP UDP transport object for this stream
 * in this session. The UDP socket pair is created when the transport is so
 * that we know that we can use the ports in the SETUP response.
 *
 *
 * \return HXR_OK unless we could not get a UDP socket
 */
HX_RESULT
RTSPTransportInstantiator::SetupTransportRTPUdp(
    RTSPServerSession* pSession,
    RTSPStreamInfo* pStreamInfo,
    RTSPTransportParams* pTransParams, UINT16 usStreamNumber, HXBOOL bOldRTPTS)
{
    HX_RESULT rc = HXR_OK;

    ServerRTPUDPTransport* pTransport = new ServerRTPUDPTransport(!m_pBaseProt->m_bSetupRecord, bOldRTPTS);
    pTransport->AddRef();
    pSession->m_pTransportList->AddTail(pTransport);
    pSession->mapTransportStream(pTransport, usStreamNumber);
    pTransport->setSessionID(pSession->m_sessionID);
    //Add StreamInfo to RTP Transport.
    pTransport->addStreamInfo(pStreamInfo);

    /*
     * XXXMC
     * Special-case handling for PV clients behind a NAT/firewall.
     */
    if (pSession->m_bEmulatePVSession)
    {
        pTransport->setPVEmulationMode(TRUE);
        DPRINTF(D_INFO, ("PV Emulation enabled in RTP transport\n"));
    }

    if (m_pBaseProt->m_bRTPLiveLegacyMode)
    {
        pTransport->setLegacyRTPLive();
    }

    const UINT32 SOCKET_COUNT = 2;
    IHXSocket* pSockets[SOCKET_COUNT];

    /** UDP sockets are created in the RTSPTransportInstantiator class when
      * the correct transport is chosen. */
    if (pTransParams->m_pSockets[0])
    {
        pSockets[0] = pTransParams->m_pSockets[0];
        pSockets[0]->AddRef();
        pSockets[1] = pTransParams->m_pSockets[1];
        pSockets[1]->AddRef();
    }
    else
    {
        HX_ASSERT(FALSE);
        return HXR_BAD_TRANSPORT;
    }

    rc = pTransport->init(m_pContext, pSockets[0], m_pBaseProt);
    pTransport->IncrProtocolCount();

    /** If this is an aggregate transport we need to tell the transport
      * because it will close the socket when it is torn down otherwise.
      * This is only relevant with UDP.
      */
    if (pTransParams->m_bAggregateTransport)
    {
        pTransport->SetAsAggregate();
    }

    m_pBaseProt->AddTransport(pTransport, pSession->m_sessionID,
                          usStreamNumber, 0);

    IHXSockAddr* pPeerAddr = getPeerAddress(pTransParams);
    IHXSockAddr* pSingleRefPeerAddr;
    pPeerAddr->Clone(&pSingleRefPeerAddr);

    pSingleRefPeerAddr->SetPort(pTransParams->m_sPort);

    pTransport->setPeerAddr(pSingleRefPeerAddr);
    HX_RELEASE(pSingleRefPeerAddr);

    if(!m_pBaseProt->m_bSetupRecord)
    {
        UINT16 fport = pTransParams->m_sPort + 1;

        // create an RTCP transport for this stream
        ServerRTCPUDPTransport* pRTCPTran;

        pPeerAddr->Clone(&pSingleRefPeerAddr);
        pSingleRefPeerAddr->SetPort(fport);

        pRTCPTran = new ServerRTCPUDPTransport(!m_pBaseProt->m_bSetupRecord);
        pRTCPTran->AddRef();
        pSession->m_pTransportList->AddTail(pRTCPTran);

        
        if (pSession->m_bEmulatePVSession && m_pBaseProt->m_bRTCPRRWorkAround)
        {
            //PV client sends RTCP packets for both the streams to the same server socket.

            //Following is the sequence the RTCP sockets are connected for each stream.
            //pSockets[1] <--(1)--> pSockResponse <--(2)--> pTransResponse <--(3)--> pRTCPTran

            CHXRTCPTransMapSocket* pSockResponse;
            //handle (1)
            pSockResponse = new CHXRTCPTransMapSocket();
            pSockResponse->AddRef();
            pSockResponse->SetSocketPort(pSockets[1],fport);
            pSockets[1]->SetResponse((IHXSocketResponse*)pSockResponse);
            pSockResponse->SetIsSockResponse(TRUE);
            pSockResponse->SetPeerAddr(pSingleRefPeerAddr);

            //handle (2) this link could change depending upon the foreign port the packet is received from.
            CHXRTCPTransMapSocket* pTransResponse;
            pTransResponse = new CHXRTCPTransMapSocket();
            pTransResponse->AddRef();
            pTransResponse->SetSocketPort(pSockResponse,fport); 
            //this effectively maps the foreign port and transport.
            pSession->mapFportTransResponse(pTransResponse,fport);
            pSockResponse->SetMap(pSession->m_pFportTransResponseMap);
            pTransResponse->SetIsSockResponse(FALSE);
            pTransResponse->SetPeerAddr(pSingleRefPeerAddr);
            pSockResponse->SetResponse(pTransResponse);  
            
            //handle (3)
            pRTCPTran->init(m_pContext, pTransResponse, pTransport,
                        m_pBaseProt, usStreamNumber);
            
            HX_RELEASE(pSockResponse);
            HX_RELEASE(pTransResponse);
        }
        else
        {   
            pRTCPTran->init(m_pContext, pSockets[1], pTransport, m_pBaseProt,
                            usStreamNumber);
        }

        /** If this is an aggregate transport we need to tell the transport
          * because it will close the socket when it is torn down otherwise.
          * This is only relevant with UDP.
          */
        if (pTransParams->m_bAggregateTransport)
        {
            pRTCPTran->SetAsAggregate();
        }

        pRTCPTran->setSessionID(pSession->m_sessionID);
        pTransport->setRTCPTransport(pRTCPTran);

        //Add StreamInfo to RTCP transport.
        pRTCPTran->addStreamInfo(pStreamInfo);

        pRTCPTran->setPeerAddr(pSingleRefPeerAddr);

	HX_RELEASE(pSingleRefPeerAddr);
    }

    HX_RELEASE(pPeerAddr);

    for (UINT32 i = 0; i < SOCKET_COUNT; i++)
    {
        HX_RELEASE(pSockets[i]);
    }
    return rc;
}

/**
 * \brief ServerRTPTCPTransport - create an RTP TCP transport for this stream
 *
 * SetupTransportRTPTcp will create an RTP TCP transport object for this 
 * stream in this session
 *
 * \return HXR_OK unless we fail to initialize the transport
 */
HX_RESULT
RTSPTransportInstantiator::SetupTransportRTPTcp(RTSPServerSession* pSession,
    RTSPStreamInfo* pStreamInfo,
    UINT16 usStreamNumber, HXBOOL bOldRTPTS)
{
    HX_RESULT rc = HXR_OK;

    if (!m_pBaseProt->m_pSock)
    {
        return HXR_FAIL;
    }

    /*
     *  Don't reuse a tranport obj
     */
    IHXSocket* pSock = new CHXRTSPTranSocket(m_pBaseProt->m_pSock);

    ServerRTPTCPTransport* pTransport = new ServerRTPTCPTransport(!m_pBaseProt->m_bSetupRecord, bOldRTPTS);
    pTransport->AddRef();

    pTransport->setSessionID(pSession->m_sessionID);
    pSession->m_pTransportList->AddTail(pTransport);
    pTransport->addStreamInfo(pStreamInfo);
    pSession->mapTransportStream(pTransport, usStreamNumber);
    m_pBaseProt->AddTransport(pTransport, pSession->m_sessionID,
                          usStreamNumber, 0);

    rc = pTransport->init(m_pContext, pSock, m_pBaseProt);

    pTransport->IncrProtocolCount();
    IUnknown* punkItem = NULL;
    pSock->QueryInterface(IID_IUnknown, (void**)&punkItem);
    (*(m_pBaseProt->m_pWriteNotifyMap))[pTransport] = punkItem;

    pTransport->setInterleaveChannel(m_pBaseProt->m_tcpInterleave);

    /* so handleTCPData can do the right thing */
    pSession->mapTransportChannel(pTransport,
                                  m_pBaseProt->m_tcpInterleave);

    if (m_pBaseProt->m_bRTPLiveLegacyMode)
    {
        pTransport->setLegacyRTPLive();
    }

    // create an RTCP transport for this stream
    ServerRTCPTCPTransport* pRTCPTran =
        new ServerRTCPTCPTransport(!m_pBaseProt->m_bSetupRecord);
    pRTCPTran->AddRef();
    pSession->m_pTransportList->AddTail(pRTCPTran);
//XXXTDM: bad code?    pSession->mapTransportPort(pRTCPTran, foreignPort + 1);

    pRTCPTran->init(m_pContext,
                    m_pBaseProt->m_pSock,  // RTCP does not go through CHXRTSPTranSocket
                    pTransport,
                    m_pBaseProt,
                    usStreamNumber);

    pRTCPTran->setInterleaveChannel(
        m_pBaseProt->m_tcpInterleave + 1);

    pSession->mapTransportChannel(pRTCPTran,
                                 m_pBaseProt->m_tcpInterleave + 1);

    pRTCPTran->setSessionID(pSession->m_sessionID);
    pTransport->setRTCPTransport(pRTCPTran);

    //Add StreamInfo to RTCP transport.
    pRTCPTran->addStreamInfo(pStreamInfo);

    (*(m_pBaseProt->m_pSessionIDList))[m_pBaseProt->m_tcpInterleave] =
        new_string(pSession->m_sessionID);
    (*(m_pBaseProt->m_pSessionIDList))[m_pBaseProt->m_tcpInterleave + 1] =
        new_string(pSession->m_sessionID);

    return rc;
}

/**
 * \brief SetupTransportNULLSet - create a "place holder" transport
 *
 * SetupTransportNULLSet will create a "place holder" transport object for
 * this stream in this session - or just re-use the transport created
 * for an earlier stream. The "NULL Setup" is created for RTSP sessions
 * from an RTSP proxy that are only used by the server for accounting purposes.
 *  These transports don't need to allocate any resources for data transfer.
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPTransportInstantiator::SetupTransportNULLSet(
    RTSPServerSession* pSession,
    RTSPStreamInfo* pStreamInfo, UINT16 usStreamNumber)
{
    if (pSession->m_sSetupCount == 1)
    {
        NullSetupTransport* pTransport = new NullSetupTransport();
        pTransport->AddRef();

        pSession->m_pTransportList->AddTail(pTransport);

        pTransport->setSessionID(pSession->m_sessionID);
        pTransport->addStreamInfo(pStreamInfo);

        pSession->mapTransportStream(pTransport, usStreamNumber);
        m_pBaseProt->AddTransport(pTransport, pSession->m_sessionID,
                              usStreamNumber, 0);
    }
    else
    {
        Transport* pTransport = pSession->getFirstTransportSetup();
        if (pTransport)
        {
            pSession->mapTransportStream(pTransport,
                                         usStreamNumber);
            pTransport->addStreamInfo(pStreamInfo);
            m_pBaseProt->AddTransport(pTransport,
                                  pSession->m_sessionID,
                                  usStreamNumber, 0);
        }
    }
    return HXR_OK;
}

/**
 * \brief SetupTransportBCNGUdpTcpMcast - create a "BCNG" transport
 *
 * SetupTransportBCNGUdpTcpMcast will create a "BCNG" transport object for
 * this stream in this session - or just re-use the transport created
 * for an earlier stream. The "BCNG" transport is created for RTSP sessions
 * from a Helix splitter that wants to acquire a "Broadcast NG" feed. 
 *
 * This transport was created at a time when there was no way to pass a 
 * connection to the main RTSP port on the server to the broadcast 
 * distribution plugin. It is a design that needs to be revisited.
 *
 * \return HXR_OK
 */
HX_RESULT
RTSPTransportInstantiator::SetupTransportBCNGUdpTcpMcast(
    RTSPServerSession* pSession,
    RTSPStreamInfo* pStreamInfo,
    RTSPTransportParams* pTransParams,
    UINT16 usStreamNumber,
    BOOL bIsPre12Dot1Proxy)
{
#ifndef HELIX_FEATURE_SERVER_BCNG
    return HXR_FAIL;
#else
    IHXPreferences* pPreferences = NULL;
    IHXBuffer*      pBuffer = NULL;
    UINT16 nMaxUDPPort = MAX_UDP_PORT;

    /* get udp port range */
    if (HXR_OK != m_pContext->QueryInterface(IID_IHXPreferences,
                                              (void**)&pPreferences))
    {
        return HXR_OUTOFMEMORY;
    }

    // If the MaxUDPPort Preference is set, use that instead of our defined limit
    if (pPreferences && pPreferences->ReadPref("MaxUDPPort", pBuffer) == HXR_OK)
    {
        nMaxUDPPort = atoi((const char*) pBuffer->GetBuffer());
        pBuffer->Release();

        if (nMaxUDPPort < MIN_UDP_PORT)
        {
            nMaxUDPPort = MAX_UDP_PORT;
        }
    }

    pPreferences->Release();
    pPreferences = 0;

    if (nMaxUDPPort % 2)
    {
        nMaxUDPPort--;
    }

    UINT16 usPortNum = (rand() % (nMaxUDPPort - MIN_UDP_PORT)) + MIN_UDP_PORT;

    if (pSession->m_sSetupCount == 1)
    {
        buffer* pAuthenticationData = NULL;

        IHXSockAddr* pPeerAddr = getPeerAddress(NULL);
        IHXSockAddr* pSingleRefPeerAddr = NULL;
        pPeerAddr->Clone(&pSingleRefPeerAddr);
        HX_RELEASE(pPeerAddr);

        BCNGTransport* pTransport = new BCNGTransport(TRUE, m_pContext);
        pTransport->AddRef();
        if(bIsPre12Dot1Proxy)
        {
            pTransport->SetPre12Dot1Proxy();
        }

        pAuthenticationData = new buffer;
        pAuthenticationData->len  = pTransParams->m_pAuthenticationDataField->GetSize();
        pAuthenticationData->data = new INT8 [pAuthenticationData->len];
        memcpy((void*)(pAuthenticationData->data), 
                    pTransParams->m_pAuthenticationDataField->GetBuffer(),
                    sizeof(UINT8) * pAuthenticationData->len);

        if (HXR_OK != pTransport->init(m_pBaseProt,
            pSingleRefPeerAddr, usPortNum, pTransParams->m_Protocol, 
            pTransParams->m_bResendSupported,
            pTransParams->m_ucFECLevel, 
            pTransParams->m_ucTTL, 
            pTransParams->m_ucSecurityType, 
            pAuthenticationData,
            pTransParams->m_ulPullSplitID,
            /*tcp only */ m_pBaseProt->m_pSock,
            m_pBaseProt->m_tcpInterleave + 1))
        {
            return HXR_BAD_TRANSPORT;
        }
        pTransport->IncrProtocolCount();
        HX_RELEASE(pSingleRefPeerAddr);

        pSession->m_pTransportList->AddTail(pTransport);

        pTransport->setSessionID(pSession->m_sessionID);
        pTransport->addStreamInfo(pStreamInfo, 0);

        pSession->mapTransportStream(pTransport, usStreamNumber);
        m_pBaseProt->AddTransport(pTransport, pSession->m_sessionID,
                              usStreamNumber, 0);

        pSession->mapTransportChannel(pTransport,
                                     pSession->m_pBaseProt->m_tcpInterleave + 1);

        CRTSPBaseProtocol* pProt = pSession->m_pBaseProt;
        (*pProt->m_pSessionIDList)[pProt->m_tcpInterleave + 1] =
            new_string(pSession->m_sessionID);
    }
    else
    {
        Transport* pTransport = pSession->getFirstTransportSetup();
        if (pTransport)
        {
            pSession->mapTransportStream(pTransport,
                                         usStreamNumber);
            pTransport->addStreamInfo(pStreamInfo);
            m_pBaseProt->AddTransport(pTransport,
                                  pSession->m_sessionID,
                                  usStreamNumber, 0);
        }
    }
    return HXR_OK;
#endif  // HELIX_FEATURE_SERVER_BCNG
}



STDMETHODIMP
RTSPTransportInstantiator::CreateTransport (THIS_ HXBOOL /*IN*/ bIsAggregate, void** /*OUT*/ ppTransport)
{
    return HXR_NOTIMPL;
}

/**
 * \brief selectTransport - help the RTSPServerProtocol select a transport
 *
 * Marshall the information in the file/stream headers, the config file 
 * and the session object to enable the RTSPTransportInstantiator to select 
 * a transport. If the session has an existing aggregate transport the 
 * instantiator will give that priority, otherwise it will select from the
 * list of transports that we received in the request. 
 *
 * \param pSession [in] : the RTSP session 
 * \param usStreamNumber [in] : the stream to select a transport for 
 *                              (ignored if its an aggregate transport)
 *
 * \return HXR_OK if we successfully selected a transport.
 */

HX_RESULT
RTSPTransportInstantiator::selectTransport(RTSPServerSession* pSession,
                                    UINT16 usStreamNumber)
{
    HX_ASSERT(pSession != NULL);

    BOOL bAllowRARTP = FALSE;
    BOOL bAllowDest = FALSE;
    UINT32 ulControlID;

    /** With an aggregate transport it is implied that all the streams
      * use the same underlying transport. So we use the "ForceRTP" from
      * the first stream to apply across all of them. This is why the 
      * stream number defaults to 0 in this case. */
    BOOL bForceRTP = FALSE;

    if (pSession->m_ppStreamInfo != NULL && pSession->m_uStreamCount)
    {
        if (!pSession->m_ppStreamInfo[usStreamNumber])
        {
            HX_ASSERT(FALSE);
            return HXR_UNEXPECTED;
        }
    }

    bForceRTP = pSession->m_ppStreamInfo[usStreamNumber]->m_bForceRTP;
    ulControlID = pSession->m_ppStreamInfo[usStreamNumber]->m_ulControlID;

    /** \The idea here is to only allow realAudio via RTP if we have either
      * already completed the RARTP challenge OR we have not started 
      * the challenge process yet.
      */
    if (m_pBaseProt->m_bRARTPChallengeMet || m_pBaseProt->m_ulChallengeInitMethod == RTSP_VERB_NONE)
    {
        if (pSession->m_bSupportsRARTPChallenge)
        {
            bAllowRARTP = TRUE;
        }
    }

    INT32 lDestAddrSupported = 0;
    if (m_pBaseProt->m_pRegistry->GetIntByName("config.SupportRTSPDestinationAddress", lDestAddrSupported) == HXR_OK)
    {
        if (lDestAddrSupported)
        {
            bAllowDest = TRUE;
        }
    }

    /** Technically this code can't currently be reached in the proxy case,
      * so the m_pProxyLocalAddr check is for possible future reference */
    IHXSockAddr* pLocalAddr = m_pBaseProt->m_pProxyLocalAddr;

    if (pLocalAddr)
    {
        pLocalAddr->AddRef();
    }
    else
    {
        if (FAILED(m_pBaseProt->m_pSock->GetLocalAddr(&pLocalAddr)))
        {
            HX_ASSERT(FALSE);
            return HXR_FAIL;
        }
    }

    setLocalAddress(pLocalAddr);
    HX_RELEASE(pLocalAddr);

    if (IsAggregateTransport())
    {
        /** If pSession->m_pAggregateTransportParams is non-NULL, it
          * points to a pre-existing aggregate that we can try to 
          * reuse. Otherwise, we select a suitable transport from 
          * the list we populated when we got the "Aggregate-Transport" 
          * header
          **/
        if (!SUCCEEDED(selectTransport(
                                       bAllowRARTP,
                                       bForceRTP,
                                       bAllowDest,
                                       pSession->m_bIsRealDataType,
                                       pSession->m_bMulticastOK,
                                       pSession->m_bRequireMulticast,
                                       pSession->m_bIsMidBox,
                                       FALSE,
                                       pSession->m_pAggregateTransportParams)))
        {
            return HXR_INVALID_PARAMETER;
        }
    }
    else
    {
        if (!SUCCEEDED(selectTransport(
                                       bAllowRARTP,
                                       bForceRTP,
                                       bAllowDest,
                                       pSession->m_bIsRealDataType,
                                       pSession->m_bMulticastOK,
                                       pSession->m_bRequireMulticast,
                                       pSession->m_bIsMidBox,
                                       (pSession->m_sSetupCount == 1),
                                       NULL,
                                       ulControlID,
                                       usStreamNumber)))
        {
            return HXR_INVALID_PARAMETER;
        }
    }

    /** At this point we should have chosen an appropriate
     *  set of transport params. */

    return HXR_OK;
}

