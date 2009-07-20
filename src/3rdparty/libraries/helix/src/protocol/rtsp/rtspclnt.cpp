/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspclnt.cpp,v 1.219 2007/04/17 15:10:44 jwei Exp $
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

#if defined _UNIX
#if defined _SOLARIS || defined _IRIX || defined _FREEBSD || defined _OPENBSD || defined _NETBSD
#include <sys/types.h>
#endif
#endif
#include "hlxclib/sys/socket.h"
#if defined _WINDOWS
#include "hlxclib/windows.h"
#endif
#include "hxcom.h"

#include <stdlib.h>
#include "hxtypes.h"
#include "timeval.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxmarsh.h"
#include "hxtick.h"
#include "netbyte.h"
#include "hxengin.h"
#include "hxnet.h"
#include "hxcore.h"
#include "hxpnets.h"
#include "ihxpckts.h"
#include "hxcomm.h"
#include "hxprefs.h"
#include "hxpref.h"
#include "hxplugn.h"
#include "hxencod.h"
#include "hxrsdbf.h"
#include "plghand2.h"
#ifdef HELIX_FEATURE_SERVER
#include "plgnhand.h"
#endif
#include "verutil.h"
#include "filespecutils.h"
#include "filespec.h"
#include "hxplugn.h"
#include "hxsdesc.h"
#include "netbyte.h"
#include "chxpckts.h"
#include "asmrulep.h"
#include "growingq.h"
#include "mimehead.h"
#include "mimescan.h"
#include "timerep.h"
#include "hxthread.h"
#if defined (HELIX_FEATURE_HTTPCLOAK)
#include "hxcloakedsocket.h"
//#include "hxcloaksockv2.h"
#endif
#include "rtspmsg.h"
#include "rtsppars.h"
#include "rtspmdsc.h"
#include "basepkt.h"
#include "servrsnd.h"
#include "portaddr.h"
#include "chxkeepalive.h"
#include "rtspclnt.h"
#include "rtsputil.h"
#include "sdptools.h"
#include "hxurl.h"
#include "pckunpck.h"
#include "hxstrutl.h"
#include "trmimemp.h"
#include "hxescapeutil.h"
#include "rtptypes.h"
#include "stream_desc_hlpr.h"
#include "hxfiles.h"
#include "pipelinedesc.h"
#include "errdbg.h"
#include "hxsockutil.h"
#include "httppost.h"
#include "rateadaptinfo.h"
#include "ntsrcbufstats.h"
#include "pckunpck.h"
#include "sessionlingertimeout.h"

#include "altgrputil.h"
#include "hxtlogutil.h"
#include "debug.h"
#include "hxheap.h"
#include "hxresult.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define RN_COMMON_MIME_TYPE_FRAGMENT    "x-pn-"

#ifndef BUFFER_DEPTH_UNDEFINED
#define BUFFER_DEPTH_UNDEFINED 0xffffffff
#endif

// HXLOG helper XXXLCM re-org this and similar log helpers
#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
static void GetLogDesc(IHXSockAddr* pAddr, CHXString& strOut)
{
    UINT16 port = pAddr->GetPort();
    IHXBuffer* pBuff = NULL;
    pAddr->GetAddr(&pBuff);

    strOut.Format("addr [%p]: [%s]:%u", pAddr, pBuff->GetBuffer(), port);
    HX_RELEASE(pBuff);
}
#endif

// Multicast: 224.0.0.0/4 or 224.0.0.0-239.255.255.255
#define IS_MULTICAST(n) (((UINT32)(n)>>28) == 0xe)

#define DEFAULT_SERVER_TIMEOUT          90          // in seconds
#define MINIMUM_TIMEOUT                 5           // in seconds

#if defined(HELIX_FEATURE_MIN_HEAP)
#define DEFAULT_TRANSPORT_BYTE_LIMIT	1024000	    // 1MByte
#else	// HELIX_FEATURE_MIN_HEAP
#define DEFAULT_TRANSPORT_BYTE_LIMIT	104875600   // 100MBytes
#endif	// HELIX_FEATURE_MIN_HEAP

#define AUTOBWDETECTION_TIMEOUT         1500        // in ms

const RTSPTableEntry RTSPClientProtocol::zm_pRTSPTable[RTSP_TABLE_SIZE] =
{
    { "SETUP",          SETUP       },
    { "REDIRECT",       REDIRECT    },
    { "PLAY",           PLAY        },
    { "PAUSE",          PAUSE       },
    { "SET_PARAMETER",  SET_PARAM   },
    { "GET_PARAMETER",  GET_PARAM   },
    { "OPTIONS",        OPTIONS     },
    { "DESCRIBE",       DESCRIBE    },
    { "TEARDOWN",       TEARDOWN    },
    { "RECORD",         RECORD      },
    { "ANNOUNCE",       ANNOUNCE    }
};


static struct _RTSPErrorsMap
{
    UINT32      m_ulErrorCode;
    HX_RESULT   m_ulErrorTag;
} const RTSPErrorsMap[] = {
    {400, HXR_PE_BAD_REQUEST},
    {401, HXR_PE_UNAUTHORIZED},
    {402, HXR_PE_PAYMENT_REQUIRED},
    {403, HXR_PE_FORBIDDEN},
    {404, HXR_PE_NOT_FOUND},
    {405, HXR_PE_METHOD_NOT_ALLOWED},
    {406, HXR_PE_NOT_ACCEPTABLE},
    {407, HXR_PE_PROXY_AUTHENTICATION_REQUIRED},
    {408, HXR_PE_REQUEST_TIMEOUT},
    {410, HXR_PE_GONE},
    {411, HXR_PE_LENGTH_REQUIRED},
    {412, HXR_PE_PRECONDITION_FAILED},
    {413, HXR_PE_REQUEST_ENTITY_TOO_LARGE},
    {414, HXR_PE_REQUESTURI_TOO_LARGE},
    {415, HXR_PE_UNSUPPORTED_MEDIA_TYPE},
    {451, HXR_PE_PARAMETER_NOT_UNDERSTOOD},
    {452, HXR_PE_CONFERENCE_NOT_FOUND},
    {453, HXR_PE_NOT_ENOUGH_BANDWIDTH},
    {454, HXR_PE_SESSION_NOT_FOUND},
    {455, HXR_PE_METHOD_NOT_VALID_IN_THIS_STATE},
    {456, HXR_PE_HEADER_FIELD_NOT_VALID_FOR_RESOURCE},
    {457, HXR_PE_INVALID_RANGE},
    {458, HXR_PE_PARAMETER_IS_READONLY},
    {459, HXR_PE_AGGREGATE_OPERATION_NOT_ALLOWED},
    {460, HXR_PE_ONLY_AGGREGATE_OPERATION_ALLOWED},
    {461, HXR_PE_UNSUPPORTED_TRANSPORT},
    {462, HXR_PE_DESTINATION_UNREACHABLE},
    {500, HXR_PE_INTERNAL_SERVER_ERROR},
    {501, HXR_PE_NOT_IMPLEMENTED},
    {502, HXR_PE_BAD_GATEWAY},
    {503, HXR_PE_SERVICE_UNAVAILABLE},
    {504, HXR_PE_GATEWAY_TIMEOUT},
    {505, HXR_PE_RTSP_VERSION_NOT_SUPPORTED},
    {551, HXR_PE_OPTION_NOT_SUPPORTED},
    {0, 0}
};

HX_RESULT
getHXRErrorCode(UINT32 ulErrorCode)
{
    for (int i = 0; RTSPErrorsMap[i].m_ulErrorCode != 0; i++)
        if (RTSPErrorsMap[i].m_ulErrorCode == ulErrorCode)
            return RTSPErrorsMap[i].m_ulErrorTag;

    return HXR_FAIL;
}

/*
 * RTSPTransportInfo methods
 */

RTSPTransportInfo::RTSPTransportInfo():
    m_pTransport(NULL),
    m_pRTCPTransport(NULL),
    m_sPort(0),
    m_sResendPort(0)
{
}

RTSPTransportInfo::~RTSPTransportInfo()
{
    if(m_pTransport)
    {
        m_pTransport->Done();
    }

    if(m_pRTCPTransport)
    {
        m_pRTCPTransport->Done();
    }

    HX_RELEASE(m_pTransport);
    HX_RELEASE(m_pRTCPTransport);
}

void
RTSPTransportInfo::addStreamNumber(UINT16 streamNumber)
{
    HXLOGL3(HXLOG_RTSP, "RTSPTransportInfo[%p]::addStreamNumber(); stream %u", this, streamNumber);

    //XXXTDM: warning: cast to pointer from integer of different size
    //        we should be using a vector or map here
    m_streamNumberList.AddTail((void*)streamNumber);
}

HXBOOL
RTSPTransportInfo::containsStreamNumber(UINT16 streamNumber)
{
    CHXSimpleList::Iterator i;
    for(i=m_streamNumberList.Begin();i!=m_streamNumberList.End();++i)
    {
        UINT16 sNumber = (UINT16)(PTR_INT)(*i);
        if(sNumber == streamNumber)
        {
            return TRUE;
        }
    }
    return FALSE;
}


/*
 * RTSPTransportRequest methods
 */

RTSPTransportRequest::RTSPTransportRequest(RTSPTransportTypeEnum tType,
                                           UINT16 sPort)
    :
    m_lTransportType(tType),
    m_sPort(sPort),
    m_sResendPort(0),
    m_tcpInterleave((INT8)0xFF),
    m_bDelete(FALSE),
    m_lListPos(0)
{
    HXLOGL3(HXLOG_RTSP, "RTSPTransportRequest[%p]::RTSPTransportRequest(): type = %ld; port = %u",this,  tType, m_sPort);
}


RTSPTransportRequest::~RTSPTransportRequest()
{
    CHXSimpleList::Iterator i;
    for(i=m_transportInfoList.Begin();i!=m_transportInfoList.End();++i)
    {
        RTSPTransportInfo* pInfo = (RTSPTransportInfo*)(*i);
        delete pInfo;
    }
}

RTSPTransportInfo*
RTSPTransportRequest::getTransportInfo(UINT16 streamNumber)
{
    CHXSimpleList::Iterator i;
    for(i=m_transportInfoList.Begin();i!=m_transportInfoList.End();++i)
    {
        RTSPTransportInfo* pInfo = (RTSPTransportInfo*)(*i);
        if(pInfo->containsStreamNumber(streamNumber))
        {
            return pInfo;
        }
    }
    return 0;
}

RTSPTransportInfo*
RTSPTransportRequest::getFirstTransportInfo()
{
    m_lListPos = m_transportInfoList.GetHeadPosition();
    if(m_lListPos)
    {
        return (RTSPTransportInfo*)m_transportInfoList.GetNext(m_lListPos);
    }
    return 0;
}

RTSPTransportInfo*
RTSPTransportRequest::getNextTransportInfo()
{
    if(m_lListPos)
    {
        return (RTSPTransportInfo*)m_transportInfoList.GetNext(m_lListPos);
    }
    return 0;
}

HX_RESULT
RTSPTransportRequest::addTransportInfo(RTSPTransport* pTrans,
                                       RTCPBaseTransport* pRTCPTrans, UINT16 streamNumber)
{
    HXLOGL3(HXLOG_RTSP, "RTSPTransportRequest[%p]::addTransportInfo(); stream %u (setting port 0)", this, streamNumber);

    HX_RESULT retVal = HXR_OK;
    RTSPTransportInfo* pTransInfo = new RTSPTransportInfo;
    if(pTransInfo)
    {
        pTransInfo->m_pTransport = pTrans;      // already AddRef'd
        pTransInfo->m_pRTCPTransport = pRTCPTrans;
        pTransInfo->addStreamNumber(streamNumber);
        pTransInfo->m_sPort = 0;

        LISTPOSITION listRet = m_transportInfoList.AddTail(pTransInfo);
        if( listRet == NULL )
        {
            HX_DELETE(pTransInfo);
            retVal = HXR_OUTOFMEMORY;
        }
    }
    else
    {
        retVal = HXR_OUTOFMEMORY;
    }
    return retVal;
}

HX_RESULT
RTSPTransportRequest::addTransportInfo(RTSPTransport* pTrans,
                                       RTCPBaseTransport* pRTCPTrans, UINT16 streamNumber, IHXSockAddr* pAddr)
{
    HX_RESULT retVal = HXR_OK;

    UINT16 uPort = pAddr->GetPort();

    HXLOGL3(HXLOG_RTSP, "RTSPTransportRequest[%p]::addTransportInfo(); stream %u (addr port = %u)", this, streamNumber, uPort);

    RTSPTransportInfo* pTransInfo = new RTSPTransportInfo;
    if(pTransInfo)
    {
        pTransInfo->m_pTransport = pTrans;      // already AddRef'd
        pTransInfo->m_pRTCPTransport = pRTCPTrans;
        pTransInfo->addStreamNumber(streamNumber);
        pTransInfo->m_sPort = uPort;

        LISTPOSITION listRet = m_transportInfoList.AddTail(pTransInfo);
        if( listRet == NULL )
        {
            HX_DELETE(pTransInfo);
            retVal = HXR_OUTOFMEMORY;
        }
    }
    else
    {
        retVal = HXR_OUTOFMEMORY;
    }
    return retVal;
}

#define QUEUE_START_SIZE    512

/*
 * RTSPClientSession methods
 */

RTSPClientSession::RTSPClientSession() :
    m_bIgnoreSession(FALSE),
    m_hostPort(0),
    m_pConnectAddr(0),
    m_bUseProxy(FALSE),
    m_bHTTPCloak(FALSE),
    m_pContext(NULL),
    m_bReopenSocket(FALSE),
    m_bChallengeDone(FALSE),
    m_bChallengeMet(FALSE),
    m_bIsValidChallengeT(FALSE),
    m_autoBWDetectionState(ABD_STATE_INIT),
    m_lRefCount(0),
    m_pNetServices(NULL),
    m_pSocket(NULL),
    m_ulLastSeqNo(0),
    m_pInQueue(NULL),
    m_bSessionDone(FALSE),
    m_bSetSessionCalled(FALSE),
    m_pScheduler(NULL),
    m_pConnBWInfo(NULL),
    m_pPreferences(NULL),
    m_abdDispatchItr(0),
    m_pAutoBWDetectionCallback(NULL),
    m_ulAutoBWDetectionCallbackHandle(0),
    m_pAutoBWDetectionSinkList(NULL),
    m_pConnectingProt(0),
    m_pMutex(NULL),
#if defined(HELIX_FEATURE_HTTPCLOAK_VERSION_2)
    m_nCloakVerMajor(2),
    m_nCloakVerMinor(0),
#else
    m_nCloakVerMajor(1),
    m_nCloakVerMinor(1),
#endif
    m_emptySessionLingerTimeout(0),
    m_connectionState(CONN_INIT)
{
    m_pInQueue = new CByteGrowingQueue(QUEUE_START_SIZE); //XXXLCM move to init
    m_pInQueue->SetMaxSize(MAX_QUEUE_SIZE);
    m_pParser = new RTSPParser;
}

RTSPClientSession::~RTSPClientSession()
{
    if (m_pConnBWInfo)
    {
        m_pConnBWInfo->RemoveSink(this);
    }
    HX_RELEASE(m_pConnBWInfo);

    if (m_pAutoBWDetectionSinkList)
    {
        CHXSimpleList::Iterator i;
        for(i=m_pAutoBWDetectionSinkList->Begin();i!=m_pAutoBWDetectionSinkList->End();++i)
        {
            IHXAutoBWDetectionAdviseSink* pSink = (IHXAutoBWDetectionAdviseSink*)(*i);
            HX_RELEASE(pSink);
        }
        HX_DELETE(m_pAutoBWDetectionSinkList);
    }

    CHXSimpleList::Iterator i;
    for(i=m_protList.Begin();i!=m_protList.End();++i)
    {
        RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
        delete pInfo;
    }

    HX_DELETE(m_pInQueue);
    HX_DELETE(m_pParser);

    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pConnectAddr);
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pSocket);
    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pConnectingProt);
}


HX_RESULT
RTSPClientSession::Init(IUnknown* pContext,
                        RTSPClientProtocol* pProt,
                        IHXSockAddr* pConnectAddr,
                        const char* pHostName,
                        UINT16 hostPort,
                        HXBOOL bUseProxy,
                        HXBOOL bHTTPCloak)
{

#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
    CHXString strAddr;
    GetLogDesc(pConnectAddr, strAddr);
    HXLOGL3(HXLOG_RTSP, "RTSPClientSession[%p]::Init(): proto [%p]; connect %s; host %s; port %u",
            this, pProt, (const char*)strAddr, pHostName, hostPort);
#endif

    HX_RESULT hr = HXR_OK;

    const UINT32 MAX_EMPTY_SESS_LINGER = 10 * 1000; // 10 secs

    HX_ASSERT(pConnectAddr); // XXXLCM TODO: pass in and use address collection
    HX_ASSERT(!m_pConnectAddr);
    pConnectAddr->Clone(&m_pConnectAddr); // XXLCM oom check

    HX_ASSERT(pContext);
    m_pContext = pContext;
    if (m_pContext != NULL)
    {
        m_pContext->AddRef();
    }

    HX_ENABLE_LOGGING(m_pContext);

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);

    // host name and port from request URL
    m_hostName  = pHostName;
    m_hostPort  = hostPort;
    m_bUseProxy = bUseProxy;
    m_bHTTPCloak = bHTTPCloak;
    addProtocol(pProt);

    pContext->QueryInterface(IID_IHXPreferences,(void**)&m_pPreferences);
    pContext->QueryInterface(IID_IHXConnectionBWInfo, (void**)&m_pConnBWInfo);
    pContext->QueryInterface(IID_IHXNetServices,(void**)&m_pNetServices);
    if (!m_pNetServices)
    {
        hr = HXR_FAIL;
        goto cleanup;
    }

    // specifies how long to keep an empty session around (connected) after
    // last protocol associated with the session is closed
    //
    ReadPrefUINT32(m_pPreferences, "EmptySessionLingerTimeout", m_emptySessionLingerTimeout);
    if (m_emptySessionLingerTimeout > MAX_EMPTY_SESS_LINGER)
    {
        m_emptySessionLingerTimeout = MAX_EMPTY_SESS_LINGER;
    }

    if (m_bHTTPCloak)
    {
#if defined (HELIX_FEATURE_HTTPCLOAK)
        HX_ASSERT(pProt);
        if (pProt)
        {
            m_pConnectingProt = pProt;
            m_pConnectingProt->AddRef();
        }

        hr = StartCloakingSession(m_pContext);

#else
        hr = HXR_FAIL;
#endif //HELIX_FEATURE_HTTPCLOAK

    }
    else
    {
        hr = CreateAndConnectSessionSocket(pProt); // XXXLCM if this fails and proto goes away we crash in ConnectDone
    }


  cleanup:
    if(FAILED(hr))
    {
        HXLOGL3(HXLOG_RTSP, "RTSPClientSession[%p]::Init(): hr = %08x", this, hr);
        ConnectDone(hr);
    }

    return hr;
}


HX_RESULT RTSPClientSession::StartCloakingSession(IUnknown* pContext)
{
    HX_RESULT  hr              = HXR_FAIL;

#if defined(HELIX_FEATURE_HTTPCLOAK)
    IHXSocket* pCloakedSocket  = NULL;
//    HXCloakedV2TCPSocket* pV2  = NULL;
    HXCloakedSocket*      pV1  = NULL;

    //We can create several sockets now, so clean up each one
    //before proceeding.
    if( m_pSocket )
    {
        m_pSocket->Close();
        HX_RELEASE(m_pSocket);
        m_connectionState = CONN_CLOSED;
    }

    switch( m_nCloakVerMajor )
    {
       case 0:
           //First call, start with the latest version.

//XXXgfw Removed until next release.
           HX_ASSERT("Should not be using new cloaking yet"==NULL);
//            pV2 = (HXCloakedV2TCPSocket*)new HXCloakedV2TCPSocket(pContext);
//            if( pV2 )
//            {
//                pV2->QueryInterface( IID_IHXSocket, (void**)&pCloakedSocket );
//                m_nCloakVerMajor = 2;
//                m_nCloakVerMinor = 0;
//            }
           break;
       case 1:
           switch( m_nCloakVerMinor )
           {
              case 0:
                  //Our original version of cloaking didn't work
                  //either.  Nothing left to try.
                  hr = HXR_FAIL;
                  break;
              case 1:
                  //Version 1.1 (no special URL string) didn't
                  //work. Try the original version of cloaking.
                  pV1  = (HXCloakedSocket*)new HXCloakedSocket(pContext, FALSE);
                  if( pV1 )
                  {
                      pV1->QueryInterface( IID_IHXSocket, (void**)&pCloakedSocket );
                      m_nCloakVerMajor = 1;
                      m_nCloakVerMinor = 0;
                  }
                  break;
              default:
                  HX_ASSERT("should not reach here"==NULL);
           }
           break;
       case 2:
           //Version 2 didn't work, start falling back to older
           //versions.
           pV1  = (HXCloakedSocket*)new HXCloakedSocket(pContext, TRUE);
           if( pV1 )
           {
               pV1->QueryInterface( IID_IHXSocket, (void**)&pCloakedSocket );
               m_nCloakVerMajor = 1;
               m_nCloakVerMinor = 1;
           }
           break;
       default:
           HX_ASSERT("should not reach here"==NULL);
    };

    //Grab the interfaces we need.
    IHXHTTPProxy*        pProxy   = NULL;
    IHXCloakedTCPSocket* pCloaked = NULL;

    if( !pCloakedSocket )
    {
        hr = HXR_OUTOFMEMORY;
    }
    else
    {
        hr = HXR_FAIL;

        pCloakedSocket->QueryInterface( IID_IHXHTTPProxy, (void**)&pProxy );
        pCloakedSocket->QueryInterface( IID_IHXCloakedTCPSocket, (void**)&pCloaked );

        HX_ASSERT( pProxy );
        HX_ASSERT( pCloaked );

        if( pCloaked )
        {
            IHXValues* pValues = NULL;
            if (m_pConnectingProt->m_bUseHTTPProxy)
            {
                //  make a copy
                if (pContext)
                {
                    CreateValuesCCF(pValues, pContext);
                    if (pValues)
                    {
                        if (m_pConnectingProt->m_pCloakValues)
                        {
                            CHXHeader::mergeHeaders(pValues, m_pConnectingProt->m_pCloakValues);
                        }
                        
                        IHXBuffer* pBuffer = NULL;
                        CreateStringBufferCCF(pBuffer, (const char*)m_pConnectingProt->m_hostName, pContext);
                        if (pBuffer)
                        {
                            pValues->SetPropertyCString("ServerAddress", pBuffer);
                            HX_RELEASE(pBuffer);
                            pValues->SetPropertyULONG32("ServerPort", (ULONG32) m_pConnectAddr->GetPort());
                        }
                    }
                }
            }
            else
            {
                pValues = m_pConnectingProt->m_pCloakValues;
                if (pValues)
                {
                    pValues->AddRef();
                }
            }
            
            hr = pCloaked->InitCloak(pValues, pContext);
            HX_RELEASE(pValues);
        }
        }

        if( SUCCEEDED(hr) )
        {
            if (m_pConnectingProt->m_bUseHTTPProxy)
            {
                pProxy->SetProxy(m_pConnectingProt->m_proxyName,
                                 m_pConnectingProt->m_proxyPort);
            }

            m_hostName     = m_pConnectingProt->m_hostName;
            HX_ASSERT(!m_pSocket);
            m_pSocket      = pCloakedSocket;
            pCloakedSocket = NULL;

            hr = m_pSocket->Init(m_pConnectAddr->GetFamily() , HX_SOCK_TYPE_TCP, HX_SOCK_PROTO_ANY);
            if( SUCCEEDED(hr) )
            {
                m_pSocket->SetResponse(this);
                m_pSocket->SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);
                m_pSocket->SelectEvents(HX_SOCK_EVENT_CONNECT|
                                        HX_SOCK_EVENT_READ|
                                        HX_SOCK_EVENT_CLOSE
                                        );

                m_connectionState = CONN_PENDING;
                hr = m_pSocket->ConnectToOne(m_pConnectAddr);
            }
        }

        HX_RELEASE( pProxy );
        HX_RELEASE( pCloaked );

        if( !SUCCEEDED(hr) )
        {
            CHXSimpleList::Iterator i;
            for(i=m_protList.Begin();i!=m_protList.End();++i)
            {
                RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
                if (pInfo && pInfo->m_pProt)
                {
                    pInfo->m_pProt->SessionFailed(this, m_pSocket);
                }
            }
            closeSocket();
        }
#endif
    return hr;
}

STDMETHODIMP
RTSPClientSession::EventPending(UINT32 uEvent, HX_RESULT status)
{
    switch (uEvent)
    {
       case HX_SOCK_EVENT_CONNECT:
           //If we have failed here and we started out with RTSPvHTTP2.0
           //then try the original http cloaking.
           if( SUCCEEDED(status) )
           {
               ConnectDone(status);
           }
           else
           {
#if defined(HELIX_FEATURE_HTTPCLOAK)
               if( m_bHTTPCloak )
               {
                   //Try the next version of HTTP cloaking.
                   status = StartCloakingSession(m_pContext);
               }
#endif

               if( !SUCCEEDED(status) )
               {
                   //We tried all the versions and none of them worked.
                   ConnectDone( status );
               }
           }
           break;
       case HX_SOCK_EVENT_READ:
       {
           if(SUCCEEDED(status))
           {
               IHXBuffer* pBuf = NULL;
               HX_RESULT hxr = m_pSocket->Read(&pBuf);
               if (hxr != HXR_SOCK_WOULDBLOCK)
               {
                   ReadDone(hxr, pBuf);
               }
               HX_RELEASE(pBuf);
           }
           else
           {
               ReadDone(status, NULL);
           }
       }
       break;
       case HX_SOCK_EVENT_CLOSE:
           //If we close with a DOC_MISSING error with any of the
           //new cloaking modes it means that the server does not
           //support them, so we need to keep trying with the
           //original.
           if( HXR_DOC_MISSING==status)
           {
               if( 2==m_nCloakVerMajor || 1==m_nCloakVerMinor )
               {
                   StartCloakingSession(m_pContext);
               }
           }
           else
           {
               // Send error to all valid prots in list:
               CHXSimpleList::Iterator i;
               for(i=m_protList.Begin();i!=m_protList.End();++i)
               {
                   RTSPClientProtocolInfo* pInfo =
                       (RTSPClientProtocolInfo*)(*i);
                   if (pInfo  &&  pInfo->m_pProt)
                   {
                       HX_RESULT hxrslt = status;
                       if (HXR_OUTOFMEMORY != hxrslt)
                       {
                           // Call OnProtocolError with this value so
                           // reconnect will be attempted (fixes PR 135700):
                           hxrslt = HXR_NET_SOCKET_INVALID;
                       }

                       pInfo->m_pProt->OnProtocolError(hxrslt);
                   }
               }
           }

           break;
       default:
           HX_ASSERT(false); // unexpected (didn't select this event)
           break;
    }
    return HXR_OK;
}

HX_RESULT
RTSPClientSession::Done()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSession[%p]::Done()", this);

    m_pMutex->Lock();
    m_bSessionDone = TRUE;
    closeSocket();
    m_pMutex->Unlock();
    return HXR_OK;
}

HX_RESULT
RTSPClientSession::handleInput(IHXBuffer* pBuffer)
{
    INT32   ret = HXR_OK;
    BYTE*   pData = NULL;
    UINT32  dataLen = 0;

    if (!pBuffer)
    {
        return HXR_INVALID_PARAMETER;
    }

    pData = pBuffer->GetBuffer();
    dataLen = pBuffer->GetSize();

    if (dataLen)
    {
        ret = m_pInQueue->EnQueue(pData, (UINT16)dataLen);
        if( ret == 0 )
        {
            // Why ABORT? Why not OUTOFMEMORY?
            return HXR_ABORT;
        }
    }

    UINT16 bytesAvail = m_pInQueue->GetQueuedItemCount();
    UINT32 bytesUsed = 0;

    if(bytesAvail == 0)
    {
        return HXR_OK;
    }

    BYTE* pBuf = new BYTE[bytesAvail];
    if(!pBuf)
    {
        return HXR_OUTOFMEMORY;
    }

    for(;;)
    {
        RTSPClientProtocol* pProt = NULL;

        if (!bytesAvail || m_bSessionDone)
        {
            break;
        }
        m_pInQueue->DeQueue(pBuf, bytesAvail);

        if(pBuf[0] == '$')
        {
            bytesUsed = 0;
            HXBOOL bGotData = FALSE;
            if(bytesAvail >= 4)
            {
                // handle TCP data
                INT8 interleave = (INT8)pBuf[1];
                UINT16 tcpDataLen = (UINT16)getshort(&pBuf[2]);
                UINT32 currentDataLen = (UINT32)(bytesAvail - 4);
                if(currentDataLen >= tcpDataLen)
                {
                    pProt = findProtocolFromInterleave(interleave);
                    if(pProt)
                    {
                        IHXTimeStampedBuffer* pTSBuffer = NULL;
                        if ((pProt == m_pConnectingProt)                  &&
                            (ABD_STATE_REQUEST == m_autoBWDetectionState) &&
                            (HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer, (void**)&pTSBuffer)))
                        {
                            ret = pProt->handleTCPData(&pBuf[4], tcpDataLen, interleave, pTSBuffer->GetTimeStamp());
                        }
                        else
                        {
                            ret = pProt->handleTCPData(&pBuf[4], tcpDataLen, interleave);
                        }
                        HX_RELEASE(pTSBuffer);
                    }
                    bytesUsed = (UINT32)(tcpDataLen + 4);
                    bytesAvail -= (UINT16)bytesUsed;
                    bGotData = TRUE;
                }
            }
            m_pInQueue->EnQueue(&pBuf[bytesUsed], (UINT16)bytesAvail);
            if(!bGotData)
            {
                break;
            }
        }
        else
        {
            bytesUsed = bytesAvail;
            RTSPMessage* pMsg = m_pParser->parse((const char*)pBuf,
                                                 bytesUsed);
            bytesAvail -= (UINT16)bytesUsed;
            m_pInQueue->EnQueue(&pBuf[bytesUsed], (UINT16)bytesAvail);
            if (!pMsg)
            {
                break;
            }

            // first always find protocol based on its sessionID
            CHXString pszSessionID = "";

            getSessionID(pMsg, &pszSessionID);
            if (!pszSessionID.IsEmpty())
            {
                pProt = findProtocolFromSessionID(&pszSessionID);
            }

            // then, based on the seq No.
            if (!pProt)
            {
                pProt = findProtocolFromSeqNo(pMsg->seqNo());
            }

            // then, we just grab the head from our protocol list
            if (!pProt && !m_protList.IsEmpty())
            {
                RTSPClientProtocolInfo* pInfo =
                    (RTSPClientProtocolInfo*)m_protList.GetHead();
                pProt = pInfo ? pInfo->m_pProt:NULL;
            }

            if(pProt)
            {
                AddRef();
                pProt->AddRef();
                ret = pProt->handleMessage(pMsg);
                if(ret == HXR_OK)
                {
                    removeProtocolSeqNo(pProt, pMsg->seqNo());
                }
                pProt->Release();
                Release();
            }
            delete pMsg;
        }
    }
    delete[] pBuf;
    return ret;
}

UINT32
RTSPClientSession::getNextSeqNo(RTSPClientProtocol* pProt)
{
    m_pMutex->Lock();
    UINT32 seqNo = ++m_ulLastSeqNo;
    setProtocolSeqNo(pProt, seqNo);
    m_pMutex->Unlock();
    return seqNo;
}

RTSPClientProtocol*
RTSPClientSession::findProtocolFromInterleave(INT8 interleave)
{
    CHXSimpleList::Iterator i;
    for(i=m_protList.Begin();i!=m_protList.End();++i)
    {
        RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
        void* pDummy;
        if(pInfo->m_interleaveMap.Lookup(interleave, pDummy))
        {
            return pInfo->m_pProt;
        }
    }
    return 0;
}

RTSPClientProtocol*
RTSPClientSession::findProtocolFromSeqNo(UINT32 seqNo)
{
    CHXSimpleList::Iterator i;
    for(i=m_protList.Begin();i!=m_protList.End();++i)
    {
        RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
        LISTPOSITION pos = pInfo->m_seqNoList.Find((void*)seqNo);
        if(pos)
        {
            return pInfo->m_pProt;
        }
    }
    return 0;
}

RTSPClientProtocol*
RTSPClientSession::findProtocolFromSessionID(CHXString* pszSessionID)
{
    CHXSimpleList::Iterator i;
    for(i=m_protList.Begin();i!=m_protList.End();++i)
    {
        RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
        if (pInfo->m_pProt &&
            !pInfo->m_pProt->m_sessionID.IsEmpty())
        {
            if(pszSessionID->CompareNoCase(pInfo->m_pProt->m_sessionID) == 0)
            {
                return pInfo->m_pProt;
            }
        }
    }

    return NULL;
}

void
RTSPClientSession::getSessionID(RTSPMessage* pMsg, CHXString* pszSessionID)
{
    MIMEHeader* pSessionID = pMsg->getHeader("Session");
    if(pSessionID)
    {
        MIMEHeaderValue* pHeaderValue = pSessionID->getFirstHeaderValue();
        if(pHeaderValue)
        {
            MIMEParameter* pParam = pHeaderValue->getFirstParameter();
            *pszSessionID = (const char*)pParam->m_attribute;
        }
    }

    return;
}

HX_RESULT
RTSPClientSession::addProtocol(RTSPClientProtocol* pProt)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSession[%p]::addProtocol(): %p", this, pProt);
    HX_RESULT rc = HXR_OK;
    m_pMutex->Lock();
    RTSPClientProtocolInfo* pInfo = new RTSPClientProtocolInfo;
    if(pInfo)
    {
        pInfo->m_pProt = pProt;
        m_protList.AddTail(pInfo);

        if (CONN_READY == m_connectionState)
        {
            pProt->SessionSucceeded(this, m_pSocket);
        }
    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }
    m_pMutex->Unlock();
    return rc;
}

HX_RESULT
RTSPClientSession::removeProtocol(RTSPClientProtocol* pProt)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSession[%p]::removeProtocol(): %p", this, pProt);
    m_pMutex->Lock();
    HX_RESULT hr = HXR_FAIL;
    LISTPOSITION pos = m_protList.GetHeadPosition();
    while(pos)
    {
        RTSPClientProtocolInfo* pInfo =
            (RTSPClientProtocolInfo*)m_protList.GetAt(pos);
        if(pInfo->m_pProt == pProt)
        {
            delete pInfo;
            m_protList.RemoveAt(pos);
            hr = HXR_OK;
            goto exit;
        }
        m_protList.GetNext(pos);
    }

  exit:
    m_pMutex->Unlock();
    return hr;
}

HXBOOL
RTSPClientSession::isEmpty()
{
    return (m_protList.GetCount() == 0);
}

HXBOOL
RTSPClientSession::HttpOnly()
{
    return m_bHTTPCloak;
}

void
RTSPClientSession::setABDState(AutoBWDetectionState state)
{
    m_pMutex->Lock();

    m_autoBWDetectionState = state;

    if (ABD_STATE_REQUEST == m_autoBWDetectionState)
    {
        if (!m_pScheduler)
        {
            m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
        }

        if (m_pScheduler)
        {
            if (!m_pAutoBWDetectionCallback)
            {
                m_pAutoBWDetectionCallback = new RTSPClientSession::TimeoutCallback(this, RTSPCLIENTSESSION_TIMEOUT_AUTOBWDETECTION);
                m_pAutoBWDetectionCallback->AddRef();
            }

            // no AutoBWDetection callback should be previously scheduled
            HX_ASSERT(m_ulAutoBWDetectionCallbackHandle == 0);
            m_ulAutoBWDetectionCallbackHandle = m_pScheduler->RelativeEnter(m_pAutoBWDetectionCallback,
                                                                            AUTOBWDETECTION_TIMEOUT);
        }
        else
        {
            HX_ASSERT(m_pScheduler);
        }
    }

    m_pMutex->Unlock();
}

UINT32
RTSPClientSession::getConnectionBW(void)
{
    HX_RESULT   rc = HXR_OK;
    UINT32      uRet = 0;

    m_pMutex->Lock();

    if (m_pConnBWInfo)
    {
        UINT32 uConnBW = 0;

        rc = m_pConnBWInfo->GetConnectionBW(uConnBW, TRUE);
        if (HXR_OK == rc || HXR_INCOMPLETE == rc)
        {
            HX_ASSERT(uConnBW);
            uRet = uConnBW;
        }
        else
        {
            HX_ASSERT(FALSE);
        }
    }
    else if (m_pPreferences)
    {
        /* Get initial bandwidth guess from Prefs */
        ReadPrefUINT32(m_pPreferences, "Bandwidth", uRet);
    }

    m_pMutex->Unlock();

    return uRet;
}

void
RTSPClientSession::updateABDState(HX_RESULT status)
{
    HX_RESULT rc = HXR_OK;
    UINT32    ulBW = 0;

    if (m_pConnBWInfo && HXR_TIMEOUT != status)
    {
        rc = m_pConnBWInfo->GetConnectionBW(ulBW, TRUE);
        // HXR_INCOMPLETE is considered ABD done
        if (HXR_OK == rc || HXR_INCOMPLETE == rc)
        {
            // We have a connection bandwidth so we are
            // actually done.
            m_autoBWDetectionState = ABD_STATE_DONE;
        }
        else if (HXR_WOULD_BLOCK == rc)
        {
            HX_ASSERT(ABD_STATE_WAITING != m_autoBWDetectionState);

            m_autoBWDetectionState = ABD_STATE_WAITING;
            // Bandwidth calibration is initiated inside GetConnectionBW()
            // Register ourselves to be notified when it's
            m_pConnBWInfo->AddSink(this);
        }
        else
        {
            HX_ASSERT(FALSE);
        }
    }
    else
    {
        m_autoBWDetectionState = ABD_STATE_DONE;
    }

    if (ABD_STATE_DONE == m_autoBWDetectionState)
    {
        // notify the rest protocols about the completion of Auto BW Detection
        // so they can proceed the rest of RTSP requests
        LISTPOSITION pos = m_protList.GetHeadPosition();
        while (pos)
        {
            RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)m_protList.GetNext(pos);
            RTSPClientProtocol*     pProt = pInfo?pInfo->m_pProt:NULL;

            if (pProt)
            {
                pProt->AutoBWDetectionDone(status, ulBW);
            }
        }
    }
}

HX_RESULT
RTSPClientSession::setProtocolInterleave(
    RTSPClientProtocol* pProt, INT8 interleave)
{
    CHXSimpleList::Iterator i;
    for(i=m_protList.Begin();i!=m_protList.End();++i)
    {
        RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
        if(pInfo->m_pProt == pProt)
        {
            pInfo->m_interleaveMap[(LONG32)interleave] = pProt;
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

HX_RESULT
RTSPClientSession::setProtocolSeqNo(RTSPClientProtocol* pProt,
                                    UINT32 seqNo)
{
    CHXSimpleList::Iterator i;
    for(i=m_protList.Begin();i!=m_protList.End();++i)
    {
        RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
        if(pInfo->m_pProt == pProt)
        {
            pInfo->m_seqNoList.AddTail((void*)seqNo);
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

HX_RESULT
RTSPClientSession::removeProtocolSeqNo(RTSPClientProtocol* pProt,
                                       UINT32 seqNo)
{
    CHXSimpleList::Iterator i;
    for(i=m_protList.Begin();i!=m_protList.End();++i)
    {
        RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
        if(pInfo->m_pProt == pProt)
        {
            LISTPOSITION pos = pInfo->m_seqNoList.Find((void*)seqNo);
            if(pos)
            {
                pInfo->m_seqNoList.RemoveAt(pos);
                return HXR_OK;
            }
        }
    }
    return HXR_FAIL;
}

IHXSocket*
RTSPClientSession::getSocket()
{
    return m_pSocket;
}

HX_RESULT
RTSPClientSession::closeSocket()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSession[%p]::closeSocket()", this);
    m_pMutex->Lock();

    if (m_pSocket != NULL)
    {
        m_pSocket->Close();
        HX_RELEASE(m_pSocket);
    }

    m_connectionState = CONN_CLOSED;

    m_pMutex->Unlock();
    return HXR_OK;
}

HX_RESULT
RTSPClientSession::CreateAndConnectSessionSocket(RTSPClientProtocol* pProt)
{
    HX_ASSERT(m_pConnectAddr);
    HX_ASSERT(!m_pSocket);
    HX_ASSERT(!m_pConnectingProt);


    HX_RESULT hr = HXSockUtil::CreateSocket(m_pNetServices, this,
                                            m_pConnectAddr->GetFamily(),
                                            HX_SOCK_TYPE_TCP,
                                            HX_SOCK_PROTO_ANY, m_pSocket);
    HXLOGL1(HXLOG_RTSP, "CreateAndConnectSessionSocket Created Socket %08x", hr);

    if(SUCCEEDED(hr))
    {
        m_pSocket->SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);
        m_pSocket->SelectEvents(HX_SOCK_EVENT_CONNECT|HX_SOCK_EVENT_READ|
                                HX_SOCK_EVENT_CLOSE);

        HX_ASSERT(pProt);
        if( pProt )
        {
            HX_RELEASE(m_pConnectingProt);
            m_pConnectingProt = pProt;
            m_pConnectingProt->AddRef();
        }


        // wait for HX_SOCK_EVENT_CONNECT in EventPending()

        //XXXLCM TODO: use ConnectToAny()
        //hr = m_pSocket->ConnectToAny(nVecLen, ppAddrVec);
        m_connectionState = CONN_PENDING;
        hr = m_pSocket->ConnectToOne(m_pConnectAddr);
        if (HXR_OK != hr)
        {
            CHXSimpleList::Iterator i;
            for(i=m_protList.Begin();i!=m_protList.End();++i)
            {
                RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
                if (pInfo && pInfo->m_pProt)
                {
                    pInfo->m_pProt->SessionFailed(this, m_pSocket);
                }
            }
        }
    }
    return hr;
}


HX_RESULT
RTSPClientSession::reopenSocket(RTSPClientProtocol* pProt)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSession[%p]::reopenSocket(): prot = %p", this, pProt);
    m_pMutex->Lock();

    /*
     * Not checking for HTTP because connectionless control channel is not
     * currently slated to work with HTTP cloaking
     */

    HX_RESULT rc = CreateAndConnectSessionSocket(pProt);
    m_pMutex->Unlock();
    return rc;
}

/*
 * IUnknown methods
 */

STDMETHODIMP
RTSPClientSession::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), this },
            { GET_IIDHANDLE(IID_IHXTCPResponse), (IHXTCPResponse*) this },
            { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*) this },
            { GET_IIDHANDLE(IID_IHXAutoBWDetection), (IHXAutoBWDetection*) this },
            { GET_IIDHANDLE(IID_IHXAutoBWDetectionAdviseSink), (IHXAutoBWDetectionAdviseSink*) this },
            { GET_IIDHANDLE(IID_IHXConnectionBWAdviseSink), (IHXConnectionBWAdviseSink*) this }
        };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(UINT32)
    RTSPClientSession::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
    RTSPClientSession::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (UINT32)m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 * IHXTCPResponse methods
 */

STDMETHODIMP
RTSPClientSession::ConnectDone(HX_RESULT status)
{
    HX_RESULT   rc = HXR_OK;

    HX_RESULT	rcTemp = HXR_OK;
    HXLOGL1(HXLOG_RTSP, "RTSPClientSession[%p]::ConnectDone(): %08x", this, status);
    HX_ASSERT(m_pConnectingProt); // reset called while connecting?
    if (!m_pConnectingProt)
    {
        return HXR_FAIL;
    }

    m_pMutex->Lock();

    if(HXR_OK == status)
    {
        m_connectionState = CONN_READY;

        if (m_bReopenSocket)
        {
            m_pConnectingProt->ReopenSocketDone(HXR_OK);
        }
        else
        {
	    // Even though the connection is established, we can't validate
	    // the RTSP session when we are in HTTPCloaking or connecting to
	    // Proxy. For both cases, we have to postpone the session validation
	    // until we actually receive response from the server in ReadDone()
	    // 
	    // Reason being:
	    // For HTTPCloaking, we perform cloaking ports scanning, there might
	    // be multiple successful connections, but only one of them actually 
	    // responses.
	    //
	    // For Proxy, we establish the connection to the Proxy instead of the
	    // server, so a successful connection to the proxy doesn't guarantee 
	    // a successful RTSP session to the actual server.
            if (!m_bHTTPCloak && !m_bUseProxy)
            {
                m_bSetSessionCalled = TRUE;

                CHXSimpleList::Iterator i;
                for(i=m_protList.Begin();i!=m_protList.End();++i)
                {
                    RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
                    if (pInfo && pInfo->m_pProt)
                    {
                        pInfo->m_pProt->SessionSucceeded(this, m_pSocket);
                    }
                }
            }
            else
            {
                rcTemp = m_pConnectingProt->sendInitialMessage(this, m_pSocket);
		// bailout if there is error
		if (HXR_OK != rcTemp)
		{
		    m_pConnectingProt->InitDone(rcTemp);
		}
            }
        }

        m_pMutex->Unlock();
        return rc;
    }

    rc = HXR_FAILED;
    m_bSetSessionCalled = TRUE;

    if (m_bReopenSocket)
    {
        m_pConnectingProt->ReopenSocketDone(HXR_NET_CONNECT);
    }
    else
    {
        CHXSimpleList::Iterator i;
        for(i=m_protList.Begin();i!=m_protList.End();++i)
        {
            RTSPClientProtocolInfo* pInfo = (RTSPClientProtocolInfo*)(*i);
            if (pInfo && pInfo->m_pProt)
            {
                pInfo->m_pProt->SessionFailed(this, m_pSocket);
            }
        }
    }

    HX_RELEASE(m_pConnectingProt);

    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientSession::ReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT hresult = HXR_OK;

    if (m_bIgnoreSession)
    {
        return HXR_OK;
    }

    if(status == HXR_OK)
    {
        AddRef();
        m_pMutex->Lock();

        if (!m_bSetSessionCalled)
        {
            m_bSetSessionCalled = TRUE;

            LISTPOSITION pos = m_protList.GetHeadPosition();
            while (pos)
            {
                RTSPClientProtocolInfo* pInfo =
                    (RTSPClientProtocolInfo*)m_protList.GetNext(pos);

                if (pInfo->m_pProt->IsSessionSucceeded())
                {
                    m_bIgnoreSession = TRUE;
                    goto ignoreexit;
                }

                pInfo->m_pProt->SessionSucceeded(this, m_pSocket);
            }
        }

        hresult = handleInput(pBuffer);
        if (hresult == HXR_OUTOFMEMORY)
        {
            m_pMutex->Unlock();
            Release();
            return hresult;
        }

      ignoreexit:
        m_pMutex->Unlock();
        Release();
    }
    else
    {
        AddRef();
        m_pMutex->Lock();

        LISTPOSITION pos = m_protList.GetHeadPosition();
        while (pos)
        {
            RTSPClientProtocolInfo* pInfo =
                (RTSPClientProtocolInfo*)m_protList.GetNext(pos);
            if (!m_bSetSessionCalled)
            {
                pInfo->m_pProt->SessionFailed(this, m_pSocket);
            }
            hresult = pInfo->m_pProt->OnProtocolError(status);
        }

        m_bSetSessionCalled = TRUE;

        m_pMutex->Unlock();
        Release();
    }

    if( hresult == HXR_OUTOFMEMORY )
    {
        ReportError( hresult );
    }
    // We have handled OOM errors, and it is not the responsibility of the
    // caller to handle our other errors, so we return HXR_OK.
    return HXR_OK;
}

/*
 *      IHXAutoBWDetection methods
 */
STDMETHODIMP
RTSPClientSession::InitAutoBWDetection(HXBOOL bEnabled)
{
    if (!bEnabled)
    {
        m_autoBWDetectionState = ABD_STATE_DISABLED;
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPClientSession::AddAutoBWDetectionSink(IHXAutoBWDetectionAdviseSink* pSink)
{
    if (!pSink)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (!m_pAutoBWDetectionSinkList)
    {
        m_pAutoBWDetectionSinkList = new CHXSimpleList();
        m_pAutoBWDetectionSinkList->AddTail(pSink);
        pSink->AddRef();
    }
    else
    {
        LISTPOSITION pos = m_pAutoBWDetectionSinkList->Find(pSink);
        if (!pos)
        {
            m_pAutoBWDetectionSinkList->AddTail(pSink);
            pSink->AddRef();
        }
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPClientSession::RemoveAutoBWDetectionSink(IHXAutoBWDetectionAdviseSink* pSink)
{
    if (m_pAutoBWDetectionSinkList)
    {
        LISTPOSITION pos = m_pAutoBWDetectionSinkList->Find(pSink);
        if (pos)
        {
            // Check to see if we are removing the next
            // element to be dispatched to
            if (m_abdDispatchItr == pos)
            {
                // Set the iterator to the next element
                // so that it is in the proper position
                // after this element is removed
                (void)m_pAutoBWDetectionSinkList->GetNext(m_abdDispatchItr);
            }

            m_pAutoBWDetectionSinkList->RemoveAt(pos);
            HX_RELEASE(pSink);
        }
    }

    return HXR_OK;
}

/*
 *      IHXAutoBWDetectionAdviseSink methods
 */
STDMETHODIMP
RTSPClientSession::AutoBWDetectionDone(HX_RESULT  status,
                                       UINT32     ulBW)
{
    HX_RESULT   rc = HXR_OK;

    m_pMutex->Lock();

    // remove the callback and socket hooks of the RTSPClientProtocol
    // which performs the Auto BW Detection
    if (m_ulAutoBWDetectionCallbackHandle)
    {
        m_pScheduler->Remove(m_ulAutoBWDetectionCallbackHandle);
        m_ulAutoBWDetectionCallbackHandle = 0;
    }
    HX_RELEASE(m_pAutoBWDetectionCallback);

    dispatchABDDoneCalls(status, ulBW);

    updateABDState(status);

    m_pMutex->Unlock();

    return HXR_OK;
}

/*
 * IHXConnectionBWAdviseSink methods
 */
STDMETHODIMP
RTSPClientSession::NewConnectionBW(THIS_ UINT32 uConnectionBW)
{
    m_pMutex->Lock();

    if (m_pConnBWInfo)
    {
        m_pConnBWInfo->RemoveSink(this);
    }

    updateABDState(HXR_OK);

    m_pMutex->Unlock();

    return HXR_OK;
}

void
RTSPClientSession::dispatchABDDoneCalls(HX_RESULT status, UINT32 ulBW)
{
    if (m_pAutoBWDetectionSinkList)
    {
        LISTPOSITION pos = m_pAutoBWDetectionSinkList->GetHeadPosition();

        while(pos)
        {
            IHXAutoBWDetectionAdviseSink* pSink =
                (IHXAutoBWDetectionAdviseSink*)m_pAutoBWDetectionSinkList->GetNext(pos);

            // If this fires it means that 2 pieces of code
            // are trying to dispatch at the same time. That
            // shouldn't ever happen.
            HX_ASSERT(m_abdDispatchItr == 0);

            m_abdDispatchItr = pos;

            pSink->AddRef();
            pSink->AutoBWDetectionDone(status, ulBW);
            HX_RELEASE(pSink);

            // NOTE: m_abdDispatchItr can change inside the call above if
            //       pSink was removed from the list
            pos = m_abdDispatchItr;
            m_abdDispatchItr = 0;
        }
    }

    return;
}

RTSPClientSession::TimeoutCallback::TimeoutCallback
(
    RTSPClientSession* pOwner,
    RTSPClientSessionTimeoutType type
    ) : m_lRefCount(0)
      , m_pOwner(pOwner)
      , m_type(type)
{
    HX_ADDREF(m_pOwner);
}

RTSPClientSession::TimeoutCallback::~TimeoutCallback()
{
    HX_RELEASE(m_pOwner);
}

STDMETHODIMP
RTSPClientSession::TimeoutCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), this },
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*) this },
        };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32)
    RTSPClientSession::TimeoutCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
    RTSPClientSession::TimeoutCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (ULONG32)m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
RTSPClientSession::TimeoutCallback::Func()
{
    if (m_pOwner)
    {
        m_pOwner->AddRef();

        switch(m_type)
        {
           case RTSPCLIENTSESSION_TIMEOUT_AUTOBWDETECTION:
               m_pOwner->AutoBWDetectionDone(HXR_TIMEOUT, 0);
               break;
           default:
               break;
        }

        m_pOwner->Release();
    }

    return HXR_OK;
}

// static initializations
RTSPClientSessionManagerType RTSPClientSessionManager::zm_pSessionManager = 0;

RTSPClientSessionManager::RTSPClientSessionManager(IUnknown* pContext) :
    m_pMutex(NULL),
    m_pLingerTimeout(NULL),
    m_pContext(pContext),
    m_lRefCount(0)
{
    HX_ADDREF(m_pContext);
    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);
}

RTSPClientSessionManager::~RTSPClientSessionManager(void)
{
    Close();
    HX_RELEASE(m_pMutex);
    HX_RELEASE(m_pContext);
}

void RTSPClientSessionManager::Close()
{
    CHXSimpleList::Iterator i;
    for (i = m_sessionList.Begin(); i != m_sessionList.End(); ++i)
    {
        RTSPClientSession* pSession = (RTSPClientSession*)(*i);
        pSession->Release();
    }
    m_sessionList.RemoveAll();

    if (m_pLingerTimeout)
    {
        m_pLingerTimeout->Cancel();
        HX_RELEASE(m_pLingerTimeout);
    }
}

/*
 * IUnknown methods
 */

STDMETHODIMP
RTSPClientSessionManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
    RTSPClientSessionManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
    RTSPClientSessionManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (UINT32)m_lRefCount;
    }

    delete this;
    SessionManGlobal() = 0;

    return 0;
}

/*
 *  RTSPClientSessionManager methods
 */

RTSPClientSessionManager*& RTSPClientSessionManager::SessionManGlobal()
{
#if defined(HELIX_CONFIG_NOSTATICS)
    GlobalID globalID = (GlobalID)RTSPClientSessionManager::zm_pSessionManager;
    return (RTSPClientSessionManager*&)HXGlobalPtr::Get(globalID);
#else
    return RTSPClientSessionManager::zm_pSessionManager;
#endif
}

RTSPClientSessionManager*
RTSPClientSessionManager::instance(IUnknown* pContext)
{
    RTSPClientSessionManager*& pSessionManager = SessionManGlobal();

    if(!pSessionManager)
    {
        pSessionManager = new RTSPClientSessionManager(pContext);
    }
    pSessionManager->AddRef();

    return pSessionManager;
}

HX_RESULT
RTSPClientSessionManager::CreateSessionInstance(RTSPClientSession*& pSession)
{
    HX_RESULT res = HXR_OK;
    pSession = new RTSPClientSession;
    if (pSession)
    {
        pSession->AddRef();
    }
    else
    {
        res = HXR_OUTOFMEMORY;
    }
    return res;

}

//
// The connect addr will contain addr:port to connect to. This may
// be a proxy or have a cloak port. Host name and port are from
// the requested url.
//
HX_RESULT
RTSPClientSessionManager::NewSession(IUnknown* pContext,
                                     RTSPClientProtocol* pProt,
                                     IHXSockAddr* pConnectAddr,
                                     const char* pHostName,
                                     UINT16 hostPort,
                                     HXBOOL bUseProxy,
                                     HXBOOL bHTTPCloak)
{
#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
    CHXString strAddr;
    GetLogDesc(pConnectAddr, strAddr);
    HXLOGL3(HXLOG_RTSP, "RTSPClientSessionManager[%p]::NewSession(): proto [%p]; connect %s; host = %s; port %u",
            this, pProt, (const char*)strAddr, pHostName, hostPort);
#endif

    m_pMutex->Lock();

    RTSPClientSession* pSession = NULL;
    HX_RESULT hr = CreateSessionInstance(pSession);
    if (SUCCEEDED(hr))
    {
        hr = pSession->Init(pContext, pProt,
                            pConnectAddr,
                            pHostName, hostPort,
                            bUseProxy, bHTTPCloak);
        if (SUCCEEDED(hr))
        {
            m_sessionList.AddTail(pSession);
            if (pProt)
            {
                pProt->SessionCreated(pSession);
            }
        }
        else
        {
            HX_RELEASE(pSession);
        }
    }

    m_pMutex->Unlock();
    return hr;
}

int
RTSPClientSessionManager::GetSessionCount()
{
    return m_sessionList.GetCount();
}

HXBOOL
RTSPClientSessionManager::MatchPlayerContext(IUnknown* pNewContext,
                                             IUnknown* pKnownContext)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSessionManager[%p]::MatchPlayerContext(): new [%p]; known [%p]", this, pNewContext, pKnownContext);
    HXBOOL        bResult = FALSE;
    IHXPlayer*  pNewHXPlayer = NULL;
    IHXPlayer* pKnownHXPlayer = NULL;

    if (!pNewContext || !pKnownContext)
    {
        goto cleanup;
    }

    if (HXR_OK == pNewContext->QueryInterface(IID_IHXPlayer, (void**)&pNewHXPlayer) &&
        HXR_OK == pKnownContext->QueryInterface(IID_IHXPlayer, (void**)&pKnownHXPlayer))
    {
        if (pNewHXPlayer == pKnownHXPlayer)
        {
            bResult = TRUE;
        }
    }

  cleanup:

    HX_RELEASE(pNewHXPlayer);
    HX_RELEASE(pKnownHXPlayer);

    return bResult;
}



// Remove the protocol from the session. If the session becomes empty
// the session is removed or added to the linger timeout.
//
HX_RESULT
RTSPClientSessionManager::RemoveFromSession(RTSPClientProtocol* pProt,
                                            RTSPClientSession* pSessionRemoved)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSessionManager[%p]::RemoveFromSession(): prot [%p]; sess [%p]", this, pProt, pSessionRemoved);
    HX_RESULT hr = HXR_OK;

    LISTPOSITION pos = m_sessionList.GetHeadPosition();
    while(pos)
    {
        RTSPClientSession* pSession =
            (RTSPClientSession*)m_sessionList.GetAt(pos);

        if(pSession == pSessionRemoved &&
           HXR_OK == pSession->removeProtocol(pProt))
        {
            if(pSession->isEmpty())
            {
                UINT32 msLinger = pSession->GetEmptySessionLingerTimeout();

                HXLOGL3(HXLOG_RTSP, "RTSPClientSessionManager[%p]::RemoveFromSession(): sess [%p] empty; ConnectionState = %i; linger = %lu ms",
                        this, pSession, pSession->m_connectionState, msLinger);

                // Only linger a session if it is actually connected.
                if (CONN_CLOSED != pSession->m_connectionState && msLinger > 0)
                {
                    // XXXLCM Current logic only allows us to linger one session at a
                    //        time. A little more work is needed to support mutiple
                    //        lingering sessions.
                    //
                    if (!m_pLingerTimeout)
                    {
                        hr = SessionLingerTimeout::Create(m_pContext, this, m_pLingerTimeout);
                        if (FAILED(hr))
                        {
                            HX_ASSERT(FALSE);
                            DoRemoveEmptySession(pSession, pos);
                        }
                    }

                    if (m_pLingerTimeout)
                    {
                        // This will keep session around a bit in case a new
                        // connect request to the same server:port comes in
                        // shortly after this.
                        if (m_pLingerTimeout->IsPending())
                        {
                            m_pLingerTimeout->Cancel();
                        }
                        HX_ASSERT(!m_pLingerTimeout->IsPending());
                        m_pLingerTimeout->Start(pSession, msLinger);
                    }

                }
                else
                {
                    DoRemoveEmptySession(pSession, pos);
                }
            }
            break;
        }
        m_sessionList.GetNext(pos);
    }

    return hr;
}

void
RTSPClientSessionManager::CheckLingerTimeout()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSessionManager[%p]::CheckLingerTimeout()", this);
    if (m_pLingerTimeout)
    {
        m_pLingerTimeout->Cancel();
    }
}

void
RTSPClientSessionManager::OnLingerTimeout(RTSPClientSession* pSession)
{
    DoRemoveEmptySession(pSession);
}

void
RTSPClientSessionManager::DoRemoveEmptySession(RTSPClientSession* pSession,
                                               LISTPOSITION pos)
{
    HX_ASSERT(pSession);
    HX_ASSERT(pSession->isEmpty());

    if (!pos)
    {
        pos = m_sessionList.Find(pSession);
        HX_ASSERT(pos);
    }

    HXLOGL3(HXLOG_RTSP, "RTSPClientSessionManager[%p]::DoRemoveEmptySession(): removing sess [%p]: ConnectionState = %i",
            this, pSession, pSession->m_connectionState);

    pSession->Done();
    pSession->Release();
    m_sessionList.RemoveAt(pos);

    if (m_sessionList.IsEmpty())
    {
        HX_RELEASE(m_pLingerTimeout);
    }
}



//
// The connect addr will contain addr:port to connect to. This may
// be a proxy or have a cloak port. Host name and port are from
// the requested url.
//
RTSPClientSession*
RTSPClientSessionManager::FindSession(IHXSockAddr* pConnectAddr,
                                      const char* pHostName,
                                      UINT16 hostPort,
                                      HXBOOL bUseProxy,
                                      IUnknown* pContext)
{
#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
    CHXString strAddr;
    GetLogDesc(pConnectAddr, strAddr);
    HXLOGL3(HXLOG_RTSP, "RTSPClientSessionManager[%p]::FindSession(): connect %s; host = %s; port %u",
            this, (const char*)strAddr, pHostName, hostPort);
#endif

    m_pMutex->Lock();
    RTSPClientSession* pSession = NULL;
    CHXSimpleList::Iterator i;
    for(i=m_sessionList.Begin();i!=m_sessionList.End();++i)
    {
        RTSPClientSession* pThis = (RTSPClientSession*)(*i);

        if( pThis->m_pConnectAddr->IsEqual(pConnectAddr) &&

            /* Either the context passed in is NULL OR it matches
             * the current session context
             */
            (!pContext || MatchPlayerContext(pContext, pThis->m_pContext)))
        {
            if (bUseProxy && pHostName)
            {
                // ensure origin host name and port match as well
                if (strcasecmp(pHostName, pThis->m_hostName) != 0 ||
                    hostPort != pThis->m_hostPort)
                {
                    continue;
                }
            }

            HXLOGL3(HXLOG_RTSP, "RTSPClientSessionManager[%p]::FindSession(): found sess [%p]; ConnectionState = %i",
                    this, pThis, pThis->m_connectionState);

            if (CONN_PENDING == pThis->m_connectionState ||
                CONN_READY == pThis->m_connectionState)
            {
                pSession = pThis;
            }
            else
            {
                HX_ASSERT(FALSE);
            }
            break;
        }
    }

    m_pMutex->Unlock();
    return pSession;
}

/*
 * RTSPClientProtocol methods
 */

/*
 * IUnknown methods
 */

STDMETHODIMP
RTSPClientProtocol::_QueryInterface(REFIID riid, void** ppvObj)
{
    RTSPTransport* pTrans = NULL;
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), this },
            { GET_IIDHANDLE(IID_IHXPendingStatus), (IHXPendingStatus*) this },
            { GET_IIDHANDLE(IID_IHXStatistics), (IHXStatistics*) this },
            { GET_IIDHANDLE(IID_IHXResolverResponse), (IHXResolverResponse*) this },
            { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*) this },
            { GET_IIDHANDLE(IID_IHXResendBufferControl), (IHXResendBufferControl*) this },
            { GET_IIDHANDLE(IID_IHXResendBufferControl2), (IHXResendBufferControl2*) this },
            { GET_IIDHANDLE(IID_IHXThinnableSource), (IHXThinnableSource*) this },
            { GET_IIDHANDLE(IID_IHXTransportSyncServer), (IHXTransportSyncServer*) this },
            { GET_IIDHANDLE(IID_IHXTransportBufferLimit), (IHXTransportBufferLimit*) this },
	    { GET_IIDHANDLE(IID_IHXRTSPClientProtocol), (IHXRTSPClientProtocol*) this },
	    { GET_IIDHANDLE(IID_IHXRTSPClientProtocol2), (IHXRTSPClientProtocol2*) this }
        };
    if(QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }
    else if (m_pSrcBufStats &&
             HXR_OK == m_pSrcBufStats->QueryInterface(riid,ppvObj))
    {
        return HXR_OK;
    }
    //XXXLCM why transport for stream idx = 0?
    else if ((pTrans = GetTransport(0)) && HXR_OK == pTrans->QueryInterface(riid, ppvObj))
    {
        return HXR_OK;
    }
    // Only the actual RTSPClientProtocol which does the dynamic ABD needs to expose
    // the following ABD interfaces when QIed from the caller
    //
    // e.g. multiple RTSPClientProtocol share the same RTSPClientSession, there will be
    //      only one of them does the dynamic ABD
    else if (IsEqualIID(riid, IID_IHXAutoBWDetection) ||
             IsEqualIID(riid, IID_IHXAutoBWDetectionAdviseSink))
    {
        if (m_pSession && m_pSession->amIDoingABD(this))
        {
            return m_pSession->QueryInterface(riid, ppvObj);
        }
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/*
 * RTSPClientProtocol methods
 */

IMPLEMENT_MANAGED_COMPONENT(RTSPClientProtocol)

BEGIN_COMPONENT_INTERFACE_LIST(RTSPClientProtocol)
INTERFACE_LIST_ENTRY_DELEGATE_BLIND(_QueryInterface)
END_INTERFACE_LIST

RTSPClientProtocol::RTSPClientProtocol():
    m_bPipelineRTSP(TRUE),
    m_bTrimTrailingSlashes(FALSE),
    m_pPeerAddr(NULL),
    m_pConnectAddr(NULL),
    m_setupResponseCount(0),
    m_pInterruptState(NULL),
    m_pResp(NULL),
    m_pSessionManager(NULL),
    m_pSession(NULL),
    m_state(RTSPClientProtocol::INIT),
    m_pScheduler(NULL),
    m_pSessionHeaders(NULL),
    m_pCloakValues(NULL),
    m_pResponseHeaders(NULL),
    m_pRegistry(NULL),
    m_pErrMsg(NULL),
    m_ulServerVersion(0),
    m_proxyPort(0),
    m_hostPort(0),
    m_FWPortToBeClosed(0),
    m_pCloakPorts(NULL),
    m_nCloakPorts(0),
    m_pFileHeader(NULL),
    m_pTransportStreamMap(NULL),
    m_pTransportPortMap(NULL),
    m_pTransportMPortMap(NULL),
    m_pTransportChannelMap(NULL),
    m_pUDPSocketStreamMap(NULL),
    m_pRTCPSocketStreamMap(NULL),
    m_pControlToStreamNoMap(NULL),
    m_pPreSetupResponsePacketQueueMap(NULL),
    m_pUDPPortToStreamNumMap(NULL),
    m_bSeqValueReceived(FALSE),
    m_bSetupRecord(FALSE),
    m_bClientDone(FALSE),
    m_bUseProxy(FALSE),
    m_bUseHTTPProxy(FALSE),
    m_bHTTPCloak(FALSE),
    m_bNoReuseConnection(FALSE),
    m_bLoadTest(FALSE),
    m_bPrefetch(FALSE),
    m_bFastStart(FALSE),
    m_bPaused(FALSE),
    m_bSessionSucceeded(FALSE),
    m_bMulticastStats(FALSE),
    m_bHandleWMServers(FALSE),
    m_pPipelinedDescLogic(NULL),
    m_pConnectionlessControl(NULL),
    m_bConnectionlessControl(FALSE),
    m_pConnectionCheckCallback(NULL),
    m_uConnectionCheckCallbackHandle(0),
    m_bConnectionAlive(FALSE),
    m_uConnectionTimeout(DEFAULT_CONN_TIMEOUT),
    m_bEntityRequired(TRUE),
    m_cloakPort(0),
    m_pMutex(NULL),
    m_uProtocolType(0),
    m_currentTransport(UnknownMode),
    m_ulBufferDepth(BUFFER_DEPTH_UNDEFINED),
    m_bSplitterConsumer(FALSE),
    m_pPacketFilter(NULL),
    m_bHasSyncMasterStream(FALSE),
    m_pFWCtlMgr(NULL),
    m_pNetSvc(NULL),
    m_pResolver(NULL),
    m_pPreferences(NULL),
    m_pUAProfURI(NULL),
    m_pUAProfDiff(NULL),
    // workaround...
    m_bNonRSRTP(FALSE),
    m_pSetupRequestHeader(NULL),
    m_bPlayJustSent(FALSE),
    m_bIPTV(FALSE),
    m_bColumbia(FALSE),
    m_bNoKeepAlive(FALSE),
    m_bForceUCaseTransportMimeType(FALSE),
    m_bReportedSuccessfulTransport(FALSE),
    m_bInitMsgSent(FALSE),
    m_bSDPInitiated(FALSE),
    m_bMulticast(FALSE),
    m_bIsLive(FALSE),
    m_bInitDone(FALSE),
    m_pSDPFileHeader(NULL),
    m_pSDPRequestHeader(NULL),
    m_pSDPStreamHeaders(NULL),
    m_pSessionTimeout(NULL),
    m_pKeepAliveCallback(NULL),
    m_bUseLegacyTimeOutMsg(TRUE),
    m_ulServerTimeOut(DEFAULT_SERVER_TIMEOUT),
    m_ulCurrentTimeOut(0),
    m_pRateAdaptInfo(NULL),
    m_pSrcBufStats(NULL),
    m_ulRegistryID(0),
    m_ulLastBWSent(MAX_UINT32),
    m_bHaveSentRemainingSetupRequests(FALSE)
#if defined(_MACINTOSH)
    , m_pCallback(NULL)
#endif /* _MACINTOSH */
{
    /*
     * Byte queue must be as large as possible because messages may be
     * bigger than MAX_RTSP_MSG
     */

    // all methods supported...
    memset(m_pIsMethodSupported, 1, sizeof(HXBOOL) * RTSP_TABLE_SIZE);
}

RTSPClientProtocol::~RTSPClientProtocol()
{
    DEBUG_OUT(m_pErrMsg, DOL_RTSP, (s,"(%u, %p) ~RTSPClnt",
                                    HX_GET_TICKCOUNT(), this));
    reset();
}

/*
 * IHXRTSPClientProtocol methods
 */

void
RTSPClientProtocol::SetSplitterConsumer(HXBOOL b)
{
    m_bSplitterConsumer = b;
}

STDMETHODIMP
RTSPClientProtocol::Init(IUnknown* pContext,
                         const char* pHostName, // from request url
                         UINT16 hostPort,       // from request url
                         IHXRTSPClientProtocolResponse* pClient,
                         UINT32 initializationType,
                         IHXValues* pSessionHeaders,
                         IHXValues* pInfo,
                         HXBOOL bHTTPCloak,
                         UINT16 cloakPort, // selected by parent (RTSPProtocol)
                         HXBOOL bNoReuseConnection)
{
    m_pSessionManager = RTSPClientSessionManager::instance(pContext);

    return InitExt(pContext,
                   pHostName,
                   hostPort,
                   pClient,
                   initializationType,
                   pSessionHeaders,
                   pInfo,
                   bHTTPCloak,
                   cloakPort,
                   bNoReuseConnection);
}

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
HX_RESULT
RTSPClientProtocol::FinishSDPInit()
{
    HX_RESULT rc = HXR_FAIL;
    if (m_bSDPInitiated && m_bMulticast)
    {
        HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::FinishSDPInit()", this);
        m_pResp->InitDone(HXR_OK);

        char szStatsMask[MAX_DISPLAY_NAME];
        UINT32 ulStatsMask = 0;
        UINT32 ulStatsStyle = 0;
        IHXValues* pHeader = NULL;
        IHXBuffer* pBuffer = NULL;


        // if any one of them doesn't exist, no stats!
        if (HXR_OK == m_pSDPFileHeader->GetPropertyULONG32("StatsStyle", ulStatsStyle)  &&
            HXR_OK == m_pSDPFileHeader->GetPropertyULONG32("StatsMask", ulStatsMask)    &&
            HXR_OK == m_pSDPFileHeader->GetPropertyCString("StatsURL", pBuffer))
        {
            m_bMulticastStats = TRUE;

            // relay the stats info to the caller
            if (HXR_OK == CreateValuesCCF(pHeader, m_pContext))
            {
                SafeSprintf(szStatsMask, MAX_DISPLAY_NAME, "%lu", ulStatsMask);
                SetCStringPropertyCCF(pHeader, "StatsMask", (const char*)szStatsMask, m_pContext);
                rc = m_pResp->HandleOptionsResponse(HXR_OK, pHeader);
            }
        }
        else
        {
            rc = m_pResp->HandleOptionsResponse(HXR_OK, NULL);
        }
        HX_RELEASE(pBuffer);
        HX_RELEASE(pHeader);
    }
    return rc;
}
#endif // HELIX_FEATURE_TRANSPORT_MULTICAST


STDMETHODIMP
RTSPClientProtocol::GetAddrInfoDone(HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::GetAddrInfoDone(): status = %08x", this, status);
    HX_RESULT   rc = HXR_OK;

    HX_ASSERT(m_pResp); //XXXLCM handle this case
    if (!m_pResp)
    {
        goto exit;
    }

    if (status == HXR_OK)
    {
        HX_RELEASE(m_pConnectAddr);
        HX_ASSERT(nVecLen > 0);
        status = ppAddrVec[0]->Clone(&m_pConnectAddr);
        if (HXR_OK == status)
        {
            // we have the resolved address; now determine
            // the port that we will connect to
            //
            UINT16 connectPort = 0;
            if(m_bUseProxy)
            {
                connectPort = m_proxyPort;
            }
            else if (m_bHTTPCloak)
            {
                // default cloak port; cloak port scanning case handled below
                connectPort = m_cloakPort;
            }
            else
            {
                connectPort = m_hostPort;
            }

            m_pConnectAddr->SetPort(connectPort);
#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
            CHXString strAddr;
            GetLogDesc(m_pConnectAddr, strAddr);
            HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::GetAddrInfoDone(): using %s", this, (const char*)strAddr);
#endif

        }
    }
    else
    {
        //Due to change in teh error scope (becoming more global)
        //convert general resolver failure to general DNR failure,
        //all others pass up.
        //Up to and including Cayenne version, all errors were
        //converted to HXR_DNR
        if(status == HXR_FAIL)
        {
            status = HXR_DNR;
        }
    }

    if (FAILED(status))
    {
        m_pResp->InitDone(status);
        goto exit;
    }

    HX_ASSERT(m_pSessionManager->isValid());

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    if (m_bSDPInitiated && m_bMulticast)
    {
        rc = FinishSDPInit();
        goto exit;
    }
#endif

    if (!m_pSession && !m_pSocket)
    {
        RTSPClientSession* pSession = NULL;
        IUnknown* pContextToMatch = NULL;

        /* Do not reuse connection for connections coming from the
         * same player. Check for context (IHXPlayer) equality test
         * Context check is needed to simulate real-world scenario
         * for SMIL load testing
         */
        if (m_bNoReuseConnection && m_bLoadTest)
        {
            pContextToMatch = m_pContext;
        }

        /* We share established connections if
         * 1. m_bNoReuseConnection is FALSE OR
         * 2. m_bLoadTest is set to TRUE (in which case we share
         *    connections ONLY for the same context (see above)
         * AND
         * 3. m_bHTTPCloak is FALSE OR
         * 4. m_pCloakPorts == NULL which means we *not* gonna
         *    attempt cloakport scanning
         *
         * NOTE: Splitter Plugin uses m_bNoReuseConnection = TRUE
         */


        if ((!m_bHTTPCloak || !m_pCloakPorts) &&
            (!m_bNoReuseConnection || m_bLoadTest))
        {
            pSession = m_pSessionManager->FindSession(
                m_pConnectAddr,
                m_hostName,
                m_hostPort,
                m_bUseProxy,
                pContextToMatch);
        }

        if(pSession)
        {
            // session will call back via either SessionSucceeded() or
            // SessionFailed()
            pSession->addProtocol(this);
        }
        else
        {
            // XXX HP:      the new session is created based on
            //              the actual(foreign) host and port to
            //              fix SMIL containing diff. servers
            //              the better fix will be send a new
            //              Challenge but still sharing the same
            //              session
            if (m_pCloakPorts)
            {
                HX_ASSERT(m_bHTTPCloak);

                // initiating cloakport scanning
                for (int i = 0; i < m_nCloakPorts; i++)
                {
                    IHXSockAddr* pCloakAddr = NULL;

                    m_pConnectAddr->Clone(&pCloakAddr);
                    pCloakAddr->SetPort(m_pCloakPorts[i]);

                    m_pSessionManager->NewSession( m_pContext,
                                                   this, pCloakAddr,
                                                   m_hostName, m_hostPort,
                                                   m_bUseProxy,
                                                   m_bHTTPCloak);
                    HX_RELEASE(pCloakAddr);
                }
            }
            else
            {
                // not cloaking or just one cloak port
                m_pSessionManager->NewSession(
                    m_pContext,
                    this, m_pConnectAddr,
                    m_hostName, m_hostPort,
                    m_bUseProxy,
                    m_bHTTPCloak);
            }
        }
    }
    else
    {
        // being re-inited..
        //sendInitialMessage();
        m_pResp->InitDone(HXR_OK);
    }

    m_pSessionManager->CheckLingerTimeout();

  exit:

    if( m_pResolver )
    {
        m_pResolver->Close();
        HX_RELEASE(m_pResolver);
    }

    return rc;
}

STDMETHODIMP
RTSPClientProtocol::GetNameInfoDone(HX_RESULT status, const char* pNode, const char* pService)
{
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

// InitExt() helper
//
// Initializing with sdp data (sdp file playback)
//
HX_RESULT
RTSPClientProtocol::InitExtInitSDP(IHXValues* pInfo, CHXString& strHostOut, UINT16& hostPortOut)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitExtInitSDP()", this);
    HX_ASSERT(pInfo);

    IHXBuffer* pSdpData = NULL;
    HX_RESULT res = pInfo->GetPropertyCString("helix-sdp", pSdpData);
    if (HXR_OK == res)
    {
        m_bSDPInitiated = TRUE;
        res = ParseSDP("application/sdp", pSdpData);
        HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitExtInitSDP(): got sdp data [%p]", this, pSdpData);
        if (HXR_OK == res)
        {
            if (m_bMulticast)
            {

                // To simplify logic we always do GetAddrInfo() even though we may
                // already know the multicast addr at this point (see ParseSDP).
                HX_ASSERT(!m_sdpMulticastAddr.IsEmpty());
                strHostOut = m_sdpMulticastAddr;
                pInfo->SetPropertyULONG32("MulticastOnly", 1);

                // Retrieve "UnicastURL" from m_pSDPFileHeader and set it in pInfo
                IHXBuffer* pBuffer = NULL;
                res = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                            (void**)&pBuffer);
                m_pSDPFileHeader->GetPropertyCString("UnicastURL", pBuffer);
                pInfo->SetPropertyCString("UnicastURL", pBuffer);
                HX_RELEASE(pBuffer);
            }
            else
            {
                CHXURL url(m_headerControl, m_pContext);

                IHXValues* pProps = url.GetProperties();
                if (pProps)
                {
                    UINT32 port = 0;
                    pProps->GetPropertyULONG32(PROPERTY_PORT, port);
                    hostPortOut = UINT16(port);
                    IHXBuffer* pHost = NULL;
                    pProps->GetPropertyBuffer(PROPERTY_HOST, pHost);
                    strHostOut = (const char*)pHost->GetBuffer();
                    HX_RELEASE(pHost);
                    HX_RELEASE(pProps);
                    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitExtInitSDP(): unicast:  [%s]:%u", this, (const char*)strHostOut, hostPortOut);
                }
            }

            HX_RELEASE(pSdpData);
        }
    }

    return res;
}

// InitExt() helper
//
// Do init based on settings in prefs.
//
HX_RESULT
RTSPClientProtocol::InitExtInitPrefs()
{
    HXBOOL bFWIntegration = TRUE;
    ReadPrefBOOL(m_pPreferences, "FWIntegration", bFWIntegration);
    if (bFWIntegration)
    {
        m_pContext->QueryInterface(IID_IHXFirewallControlManager, (void**)&m_pFWCtlMgr);

        if (m_pFWCtlMgr && m_pFWCtlMgr->IsFirewallOn())
        {
            // if the app itself is already on the firewall exemption list, then
            // no need to open the UDP port
            CHXFileSpecifier thisApp = CHXFileSpecUtils::GetCurrentApplication();
            if (m_pFWCtlMgr->IsAppExempted((const char*)thisApp.GetPathName()))
            {
                HX_RELEASE(m_pFWCtlMgr);
            }
        }
        else
        {
            HX_RELEASE(m_pFWCtlMgr);
        }
    }

    ReadPrefUINT32(m_pPreferences, "ConnectionTimeOut", m_uConnectionTimeout);

    ReadPrefUINT32(m_pPreferences, "ServerTimeOut", m_ulServerTimeOut);
    if (m_ulServerTimeOut < MINIMUM_TIMEOUT)
    {
        m_ulServerTimeOut = MINIMUM_TIMEOUT;
    }
    m_ulServerTimeOut *= MILLISECS_PER_SECOND;

    ReadPrefBOOL(m_pPreferences, "RTSPMessageDebug", m_bMessageDebug);
    if (m_bMessageDebug)
    {
        IHXBuffer* pBuf = NULL;
        if (HXR_OK == m_pPreferences->ReadPref("RTSPMessageDebugFile", pBuf))
        {
            if (pBuf->GetSize() <= 0)
            {
                // no file name, no log
                m_bMessageDebug = FALSE;
            }
            else
            {
                m_messageDebugFileName = (const char*)pBuf->GetBuffer();
            }
        }
        HX_RELEASE(pBuf);
    }

    // XXXGo - Interop workaround
    ReadPrefBOOL(m_pPreferences, "NonRS", m_bNonRSRTP);
    if (m_bNonRSRTP)
    {
        m_pIsMethodSupported[SET_PARAM] = FALSE;
        m_pIsMethodSupported[GET_PARAM] = FALSE;
        m_pIsMethodSupported[RECORD] = FALSE;
        m_pIsMethodSupported[ANNOUNCE] = FALSE;
    }

    ReadPrefBOOL(m_pPreferences, "RTSPNoReuseConnection", m_bNoReuseConnection);
    ReadPrefBOOL(m_pPreferences, "LoadTest", m_bLoadTest);

    // check for UAProf headers
    HX_RELEASE(m_pUAProfURI);
    HX_RELEASE(m_pUAProfDiff);
    ReadPrefStringBuffer(m_pPreferences, "XWapProfileURI", m_pUAProfURI);
    ReadPrefStringBuffer(m_pPreferences, "XWapProfileDiff", m_pUAProfDiff);

    // Determine if we should handle Windows Media Servers. The default
    // is to error out if we enounter a Windows Media Server. If the
    // "UseHXRTSPForWMServers" pref is set, then we will 
    // attempt to use this RTSP client to talk to Windows Media servers.
    ReadPrefBOOL(m_pPreferences, "UseHXRTSPForWMServers", m_bHandleWMServers);

    return HXR_OK;

}

HX_RESULT
RTSPClientProtocol::InitExt(IUnknown*       pContext,
                            const char*     pHostName, // from url (null if info indicates sdp)
                            UINT16          hostPort,  // from url
                            IHXRTSPClientProtocolResponse* pClient,
                            UINT32          initializationType,
                            IHXValues*      pSessionHeaders,
                            IHXValues*      pInfo,
                            HXBOOL          bHTTPCloak,  // true when current transport http-cloaking
                            UINT16          cloakPort, // picked by higher-level code goes through list
                            HXBOOL          bNoReuseConnection)
{
    HX_RESULT       hresult = HXR_OK;
    IUnknown*       pUnknown = NULL;
    CHXString       connectHost;


    m_hostName              = pHostName;
    m_hostPort              = hostPort;
    m_bHTTPCloak            = bHTTPCloak;
    m_cloakPort             = cloakPort;
    m_bNoReuseConnection    = bNoReuseConnection;
    if (m_bHTTPCloak)
    {
        m_currentTransport = HTTPCloakMode;
        // m_bUseProxy is set via SetProxy()
        if (m_bUseProxy)
        {
            m_bUseHTTPProxy = TRUE;
        }
    }
    if (pSessionHeaders && !m_pSessionHeaders)
    {
        m_pSessionHeaders = pSessionHeaders;
        m_pSessionHeaders->AddRef();
    }

    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    m_pContext->AddRef();

    CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, m_pContext);

    HX_RELEASE(m_pResp);
    m_pResp = pClient;
    m_pResp->AddRef();

    if (!m_pContext || !m_pResp)
    {
        // required interfaces
        HX_ASSERT(FALSE);
        hresult = HXR_FAIL;
        goto cleanup;
    }


    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pErrMsg);
    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pScheduler);

#if defined(HELIX_FEATURE_REGISTRY)
    if(!m_pRegistry)
    {
        hresult = m_pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);
        if (FAILED(hresult))
        {
            goto cleanup;
        }
    }
#endif /* HELIX_FEATURE_REGISTRY */

    m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrMsg);
    m_pContext->QueryInterface(IID_IHXCommonClassFactory,(void**) &m_pCommonClassFactory);
    m_pContext->QueryInterface(IID_IHXInterruptState,(void**) &m_pInterruptState);
    m_pContext->QueryInterface(IID_IHXNetServices, (void**)&m_pNetSvc);
    m_pContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences);
    m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    if (!m_pInterruptState || !m_pNetSvc || !m_pCommonClassFactory || !m_pScheduler)
    {
        // required interfaces
        HX_ASSERT(FALSE);
        hresult = HXR_FAIL;
        goto cleanup;
    }


    DEBUG_OUT(m_pErrMsg, DOL_RTSP, (s,"(%u, %p) RTSPClnt",
                                    HX_GET_TICKCOUNT(), this));

    InitExtInitPrefs();

    // m_pSrcBufStats MUST be initialized before m_pRateAdaptInfo
    // because it provides interfaces that m_pRateAdaptInfo needs
    // for initialization.

    HX_RELEASE(m_pSrcBufStats);
    m_pSrcBufStats = new HXNetSourceBufStats(this);
    if (!m_pSrcBufStats)
    {
        hresult = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    m_pSrcBufStats->AddRef();
    if (HXR_OK != m_pSrcBufStats->Init(m_pContext))
    {
        HX_RELEASE(m_pSrcBufStats);
    }

#if defined(HELIX_FEATURE_RATE_ADAPTATION)
    m_pRateAdaptInfo = new CHXRateAdaptationInfo();
    if (!m_pRateAdaptInfo)
    {
        hresult = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    if (HXR_OK != m_pRateAdaptInfo->Init(pContext))
    {
        // This is not a critical error. It just means
        // that we can't negotiate server rate adaptation.
        HX_DELETE(m_pRateAdaptInfo);
    }
#endif /* HELIX_FEATURE_RATE_ADAPTATION */

    // handle possible sdp if host name is not set
    //
    if (m_hostName.IsEmpty() && pInfo)
    {
        // has to be sdp...
        hresult = InitExtInitSDP(pInfo, m_hostName, m_hostPort);
        if (FAILED(hresult))
        {
            goto cleanup;
        }
    }
#ifdef HELIX_FEATURE_PIPELINED_DESCRIBE
    else if (!m_pPipelinedDescLogic)
    {
        HXBOOL bDoPipelining = FALSE;
        ReadPrefBOOL( m_pPreferences, "PipelineRTSPDescribe", bDoPipelining );
        if( bDoPipelining )
        { 
            m_pPipelinedDescLogic = new CHXPipelinedDescribeLogic();
            if (!m_pPipelinedDescLogic)
            {
                hresult = HXR_OUTOFMEMORY;
                goto cleanup;
            }
            m_pPipelinedDescLogic->OnInit(m_pContext, pInfo, this);
        }
    }
#endif

    if (!m_pTransportStreamMap)
    {
        m_pTransportStreamMap = new CHXMapLongToObj;
        if(!m_pTransportStreamMap)
        {
            hresult = HXR_OUTOFMEMORY;
            goto cleanup;
        }
    }

    if (!m_pTransportPortMap)
    {
        m_pTransportPortMap = new CHXMapLongToObj;
        if(!m_pTransportPortMap)
        {
            hresult = HXR_OUTOFMEMORY;
            goto cleanup;
        }
    }

    if (!m_pTransportMPortMap)
    {
        m_pTransportMPortMap = new CHXMapLongToObj;
        if(!m_pTransportMPortMap)
        {
            hresult = HXR_OUTOFMEMORY;
            goto cleanup;
        }
    }

    HX_RELEASE(m_pResponseHeaders);

    if (HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXKeyValueList, (void**)&pUnknown))
    {
        hresult = HXR_FAILED;
        goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXKeyValueList, (void**)&m_pResponseHeaders))
    {
        hresult = HXR_FAILED;
        goto cleanup;
    }

    // close existing resolver (just in case) and re-create
    if( m_pResolver )
    {
        m_pResolver->Close();
        HX_RELEASE(m_pResolver);
    }

    m_pNetSvc->CreateResolver(&m_pResolver);
    if(!m_pResolver)
    {
        hresult = HXR_OUTOFMEMORY;
        goto cleanup;
    }
    m_pResolver->Init(this);


    if (m_bUseProxy)
    {
        // media proxy or web proxy (cloaking)
        connectHost = m_proxyName;
    }
    else
    {
        connectHost = m_hostName;
    }
    // response via GetAddrInfoDone()...
    HXLOGL2(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitExt(): resolving '%s'", this, (const char*)connectHost);
    hresult = m_pResolver->GetAddrInfo(connectHost, NULL, NULL);


#if defined(_MACINTOSH)
    if (m_pCallback &&
        m_pCallback->m_bIsCallbackPending &&
        m_pScheduler)
    {
        m_pCallback->m_bIsCallbackPending = FALSE;
        m_pScheduler->Remove(m_pCallback->m_Handle);
    }
#endif /* _MACINTOSH */

  cleanup:

    HX_RELEASE(pUnknown);
    return hresult;
}

STDMETHODIMP
RTSPClientProtocol::SetProxy(const char* pProxyHost, UINT16 uProxyPort)
{
    m_bUseProxy = TRUE;
    m_proxyName = pProxyHost;
    m_proxyPort = uProxyPort;
    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::SetResponse(IHXRTSPClientProtocolResponse* pResp)
{
    HX_RELEASE(m_pResp);

    m_pResp = pResp;
    m_pResp->AddRef();

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::SetBuildVersion(const char* pVersionString)
{
    m_versionString = pVersionString;
    return HXR_OK;
}

// normally called by owning protocol (RTSPProtocol) after
// teardown.
STDMETHODIMP
RTSPClientProtocol::Done()
{
    if (m_bClientDone)
    {
	return HXR_OK;
    }

    HX_LOCK(m_pMutex);

    m_bClientDone = TRUE;

#if defined(_MACINTOSH)
    if (m_pCallback &&
        m_pCallback->m_bIsCallbackPending &&
        m_pScheduler)
    {
        m_pCallback->m_bIsCallbackPending = FALSE;
        m_pScheduler->Remove(m_pCallback->m_Handle);
    }
    HX_RELEASE(m_pCallback);
#endif /* _MACINTOSH */

    while (!m_sessionList.IsEmpty())
    {
        RTSPClientSession* pTempSession = (RTSPClientSession*)m_sessionList.RemoveHead();
        m_pSessionManager->RemoveFromSession(this, pTempSession);
    }

    HX_ASSERT(m_pSessionManager && m_pSessionManager->isValid());
    if (m_pSession)
    {
        m_pSessionManager->RemoveFromSession(this, m_pSession);
        m_pSession = NULL;
    }
    HX_RELEASE(m_pSessionManager);

    reset();

    HX_UNLOCK(m_pMutex);
    HX_RELEASE(m_pMutex);

    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
RTSPClientProtocol::IsRateAdaptationUsed()
{
    HXBOOL bRateAdaptationUsed = FALSE;
    if (m_pRateAdaptInfo)
    {
        bRateAdaptationUsed = m_pRateAdaptInfo->IsRateAdaptationInternal();
    }
    return bRateAdaptationUsed;
}

STDMETHODIMP
RTSPClientProtocol::SendStreamDescriptionRequest(const char* pURL,
                                                 IHXValues* pRequestHeaders)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendStreamDescriptionRequest()", this);

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    HX_RESULT   rc = HXR_OK;
    if (m_bSDPInitiated && m_bMulticast)
    {
        m_pSDPRequestHeader = pRequestHeaders;
        HX_ADDREF(m_pSDPRequestHeader);

        IHXValues* pResponseHeaders = NULL;
        if (HXR_OK == m_pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseHeaders))
        {
            rc = m_pResp->HandleStreamDescriptionResponse
                (
                    HXR_OK,
                    m_pSDPFileHeader,
                    m_pSDPStreamHeaders,
                    pResponseHeaders
                    );
        }
        HX_RELEASE(pResponseHeaders);
        return rc;
    }
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */

    if (m_pPipelinedDescLogic &&
        (HXR_OK == m_pPipelinedDescLogic->OnSendStreamDescribeRequest()))
    {
        return HXR_OK;
    }

#ifdef _MACINTOSH
    if (m_pInterruptState && m_pInterruptState->AtInterruptTime())
    {
        /*
         * We cannot load DLLs (Stream Desc Plugins) at deferred time.
         * Also, encodeURL calls HXIsLeadByte->CharType->...internally
         * calls GetPort. Mac does not like that either at deferred time.
         * Schedule a callback and do this operation at system time
         */

        if (!m_pCallback)
        {
            m_pCallback = new RTSPClientProtocolCallback(this);
            if(!m_pCallback)
            {
                return HXR_OUTOFMEMORY;
            }
            m_pCallback->AddRef();
        }

        HX_ASSERT(m_pScheduler);

        if (!m_pCallback->m_bIsCallbackPending)
        {
            m_pCallback->m_bIsCallbackPending = TRUE;
            m_pCallback->m_Handle =
                m_pScheduler->RelativeEnter(m_pCallback, 0);
        }

        /*
         * If we receive a SendStreamDescriptionRequest when
         * one is already pending, we do not queue them up.
         * The last one wins
         */
        m_pCallback->m_PendingDescURL = pURL;
        HX_RELEASE(m_pCallback->m_pPendingRequestHeaders);
        m_pCallback->m_pPendingRequestHeaders = pRequestHeaders;
        m_pCallback->m_pPendingRequestHeaders->AddRef();

        return HXR_OK;
    }

    HX_ASSERT(m_pScheduler);
    /* remove any previously scheduled callback */
    if (m_pCallback &&
        m_pCallback->m_bIsCallbackPending)
    {
        m_pCallback->m_bIsCallbackPending = FALSE;
        m_pScheduler->Remove(m_pCallback->m_Handle);
    }
#endif

    return sendPendingStreamDescription(pURL, pRequestHeaders);
}

HX_RESULT
RTSPClientProtocol::sendPendingStreamDescription(const char* pURL,
                                                 IHXValues* pRequestHeaders, HXBOOL bLockMutex)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendPendingStreamDescription()", this);
    if (bLockMutex && m_pMutex)
    {
        m_pMutex->Lock();
    }

    HX_RESULT rc = HXR_OK;

    if (HXR_OUTOFMEMORY == extractExistingAuthorizationInformation(pRequestHeaders))
    {
        if (bLockMutex && m_pMutex)
        {
            m_pMutex->Unlock();
        }

        return HXR_OUTOFMEMORY;
    }

    RTSPDescribeMessage* pMsg = new RTSPDescribeMessage;
    if(!pMsg)
    {
        if (bLockMutex && m_pMutex)
        {
            m_pMutex->Unlock();
        }

        return HXR_OUTOFMEMORY;
    }

    // pURL is expected to be a relative URL


    HX_ASSERT (HXURLRep::TYPE_RELPATH == HXURLRep(pURL).GetType());

    HXURLRep originalURL(pURL);
    HXEscapeUtil::EnsureEscapedURL(originalURL);
    HXURLRep url(HXURLRep::TYPE_NETPATH, "rtsp", "", /* no user info */
                 m_hostName, m_hostPort,
                 originalURL.Path(),
                 originalURL.Query(),
                 originalURL.Fragment());

    HX_ASSERT(url.IsValid());
    m_url = url.String();

    pMsg->setURL(m_url);

    IHXValues* pValuesRequestHeaders = NULL;
    CreateValuesCCF(pValuesRequestHeaders, m_pContext);
    if(!pValuesRequestHeaders)
    {
        HX_DELETE(pMsg);

        if (bLockMutex && m_pMutex)
        {
            m_pMutex->Unlock();
        }
        return HXR_OUTOFMEMORY;
    }

    if (m_bEntityRequired)
    {
        CHXString CHXStringRequireValue
            (
                RTSPClientProtocol::RTSPRequireOptionsTable
                [
                    RTSPClientProtocol::RTSP_REQUIRE_ENTITY
                    ].pcharOption
                );

        IHXBuffer* pBufferRequireValue = NULL;

        CHXBuffer::FromCharArray
            (
                CHXStringRequireValue.GetBuffer(0),
                &pBufferRequireValue
                );

        // Set require header
        pValuesRequestHeaders->SetPropertyCString
            (
                "Require",
                pBufferRequireValue
                );

        HX_RELEASE(pBufferRequireValue);
    }

    addUAProfHeaders(pValuesRequestHeaders);

    CHXHeader::mergeHeaders
        (
            pValuesRequestHeaders,
            pRequestHeaders
            );

    // get all StreamDescription plugin mime types
    CHXString mimeTypes;
    IHXCommonClassFactory*      pClassFactory       = NULL;
    IHXPluginGroupEnumerator*   pPluginGroupEnum    = NULL;

    // ok we are going to get an IRMA PluginGroupEnumerator from the core.
    if
        (
            SUCCEEDED
            (
                m_pContext->QueryInterface
                (
                    IID_IHXCommonClassFactory,
                    (void**) &pClassFactory
                    )
                )
            )
    {
        pClassFactory->CreateInstance
            (
                CLSID_IHXPluginGroupEnumerator,
                (void**)&pPluginGroupEnum
                );

        HX_RELEASE(pClassFactory);
    }

    if(pPluginGroupEnum &&
       (HXR_OK == pPluginGroupEnum->Init(IID_IHXStreamDescription)))
    {
        IUnknown* pUnknown = NULL;
        ULONG32 ulNumPlugins = pPluginGroupEnum->GetNumOfPlugins();
        for(ULONG32 i=0;i<ulNumPlugins;++i)
        {
            if (SUCCEEDED(pPluginGroupEnum->GetPlugin(i, pUnknown)))
            {
                GetStreamDescriptionInfo(pUnknown, mimeTypes);
                HX_RELEASE(pUnknown);
            }
        }
    }
    else
    {
        // if we don't have a CCF or we don't have a pluginGroupEnumerator
        // then we will have to use a plugin Enumerator.
        IHXPluginEnumerator* pPluginEnumerator = NULL;
        m_pContext->QueryInterface
            (
                IID_IHXPluginEnumerator,
                (void**)&pPluginEnumerator
                );

        if(pPluginEnumerator)
        {
            IUnknown* pUnknown = NULL;
            IHXContextUser* pContextUser = NULL;
            ULONG32 ulNumPlugins = pPluginEnumerator->GetNumOfPlugins();
            for(ULONG32 i=0;i<ulNumPlugins;++i)
            {
                if(SUCCEEDED(pPluginEnumerator->GetPlugin(i, pUnknown)))
                {
                    // XXX Ping, this is an expenstive iteration, we should use
                    // IHXPluginGroupEnumerator instead after we add it to the BaseHandler 
                    // which is used in Static & Consolidated build.
                    // 
                    // plugin implements IHXContextUser expects RegisterContext() to be called
                    // in order to initialize properly
                    if (SUCCEEDED(pUnknown->QueryInterface(IID_IHXContextUser, (void**)&pContextUser)))
                    {
                        pContextUser->RegisterContext(m_pContext);
                        HX_RELEASE(pContextUser);
                    }
                    GetStreamDescriptionInfo(pUnknown, mimeTypes);
                    HX_RELEASE(pUnknown);
                }
            }
            pPluginEnumerator->Release();
        }
    }

    HX_RELEASE(pPluginGroupEnum);

    pMsg->addHeader("Accept", (const char*)mimeTypes);
    AddCommonHeaderToMsg(pMsg);

    addRFC822Headers(pMsg, pValuesRequestHeaders);

    appendAuthorizationHeaders(pMsg);

    HX_RELEASE(pValuesRequestHeaders);

    UINT32 seqNo = m_pSession->getNextSeqNo(this);
    rc = sendRequest(pMsg, seqNo);

    if ((HXR_OK == rc) && m_pPipelinedDescLogic)
    {
        m_pPipelinedDescLogic->OnDescribeSent(pMsg);
    }

    if (bLockMutex && m_pMutex)
    {
        m_pMutex->Unlock();
    }

    return rc;
}


STDMETHODIMP
RTSPClientProtocol::SendStreamRecordDescriptionRequest(
    const char* pURL,
    IHXValues* pFileHeader,
    CHXSimpleList* pStreams,
    IHXValues* pRequestHeaders)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendStreamRecordDescriptionRequest()", this);
    HX_RESULT rc = HXR_OK;
    IHXBuffer* pDescription = 0;

    if (!m_pIsMethodSupported[ANNOUNCE])
    {
        return HXR_OK;
    }

    // Let's make sure we have enough mem before we do the mutex lock.
    RTSPAnnounceMessage* pMsg = new RTSPAnnounceMessage;
    if(!pMsg)
    {
        return HXR_OUTOFMEMORY;
    }
    m_pMutex->Lock();
    pMsg->setURL(pURL);
    m_url = pURL;
    const char* pDesc = 0;

    addRFC822Headers(pMsg, pRequestHeaders);

    clearStreamInfoList();

    // use the first stream description plugin found...
    char* pMimeType = 0;
    if(HXR_OK == getStreamDescriptionMimeType(pMimeType))
    {
        IHXStreamDescription* pSD =
            getStreamDescriptionInstance(pMimeType);

        if(pSD)
        {
            UINT32 streamNumber;
            UINT32 needReliable;
            UINT32 rtpPayloadType;
            UINT32 ulIsLive;
            IHXBuffer* pControlString;
            UINT32 ulIsSessionLive = 0;

            // if we are talking to a realserver, we will make an old sdpfile
            // for now...We need to check version and send the spec complient
            // sdp!
            IHXValues* pResponseHeaders = NULL;
            if (HXR_OK ==
                m_pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseHeaders))
            {
                pFileHeader->SetPropertyULONG32("SdpFileType",
                                                GetSdpFileTypeWeNeed(pResponseHeaders));
            }
            HX_RELEASE(pResponseHeaders);

            UINT16 nStreams = (UINT16)pStreams->GetCount();
            IHXValues** ppValues = new IHXValues*[nStreams+2];
            if(!ppValues)
            {
                rc = HXR_OUTOFMEMORY;
                HX_DELETE(pMsg);
                goto overandout;
            }
            ppValues[0] = pFileHeader;
            ppValues[1] = 0;    // no options

            pFileHeader->GetPropertyULONG32("LiveStream", ulIsSessionLive);

            CHXSimpleList::Iterator i;
            INT16 j=2;
            for(i=pStreams->Begin();i!=pStreams->End();++i,++j)
            {
                // reset...
                streamNumber    = 0;
                needReliable    = 0;
                rtpPayloadType  = (UINT32)-1;
                ulIsLive        = ulIsSessionLive;
                pControlString  = 0;

                ppValues[j] = (IHXValues*)(*i);

                // build stream info list
                RTSPStreamInfo* pInfo = new RTSPStreamInfo;
                if(!pInfo)
                {
                    rc = HXR_OUTOFMEMORY;
                    HX_DELETE(pMsg);
                    HX_VECTOR_DELETE(ppValues);
                    goto overandout;
                }
                ppValues[j]->GetPropertyULONG32("StreamNumber",
                                                streamNumber);
                ppValues[j]->GetPropertyULONG32("NeedReliablePackets",
                                                needReliable);
                ppValues[j]->GetPropertyULONG32("RTPPayloadType",
                                                rtpPayloadType);
                ppValues[j]->GetPropertyCString("Control",
                                                pControlString);
                ppValues[j]->GetPropertyULONG32("LiveStream", ulIsLive);

                pInfo->m_streamNumber = (UINT16)streamNumber;
                pInfo->m_bNeedReliablePackets = needReliable? TRUE: FALSE;
                pInfo->m_rtpPayloadType = (INT16)rtpPayloadType;
                pInfo->m_bIsLive = ulIsLive ? TRUE : FALSE;
                pInfo->m_sPort = 0;
                if(pControlString)
                {
                    pInfo->m_streamControl = pControlString->GetBuffer();

                    // done with the buffer
                    pControlString->Release();
                    pControlString = NULL;
                }
                else
                {
                    char tmp[32];
                    SafeSprintf(tmp,32, "streamid=%u", (UINT16)streamNumber);
                    pInfo->m_streamControl = tmp;
                }
                m_streamInfoList.AddTail(pInfo);
            }
            pSD->GetDescription((UINT16)(nStreams + 2), ppValues, pDescription);
            pDesc = (const char*)pDescription->GetBuffer();
            pSD->Release();
            delete[] ppValues;
        }
    }
    if(pDesc)
    {
        m_bSetupRecord = TRUE;
#ifdef _MACINTOSH
// someday, someone in core should look at why m_pSession is NULL when
// an invalid port is used, and yet m_pSession is assumed to be valid, and subsequently
// crashes macs HARD. I would attempt to investigate this further, however
// the core deferred task keeps crashing the debugger while I am attempting
// to trace through why m_pSession never gets assigned. since Mac Producer goes
// beta in 2 weeks, I'm putting this 'fix' in here now.. rlovejoy 2/16/00

        if (m_pSession == NULL) {
            rc = HXR_PORT_IN_USE;
        } else
#endif
        {
            UINT32 seqNo = m_pSession->getNextSeqNo(this);
            rc = sendRequest(pMsg, pDesc, pMimeType, seqNo);
        }
        // done with the description, we need to clean it up
        pDescription->Release();
    }
    else
    {
        rc = HXR_FAIL;
    }
  overandout:
    delete[] pMimeType;
    m_pMutex->Unlock();
    return rc;
}


STDMETHODIMP
RTSPClientProtocol::SendSetupRequest(RTSPTransportType* pTransType,
                                     UINT16 nTransTypes, IHXValues* pRequestHeaders)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendSetupRequest()", this);
    m_pMutex->Lock();

    HX_RESULT           rc = HXR_OK;
    RTPUDPTransport*    pTrans = NULL;
    RTCPUDPTransport*   pRTCPTrans = NULL;

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    CHXSimpleList::Iterator i;
    IHXSockAddr* pAddrOne = 0;
    IHXSockAddr* pAddrTwo = 0;
    IHXSocket*          pUDPSocket = NULL;
    IHXSocket*          pRTCPUDPSocket = NULL;
    RTSPStreamInfo*     pStreamInfo = NULL;
    if (m_bSDPInitiated && m_bMulticast)
    {
        RTSPTransportRequest* pRequest = new RTSPTransportRequest(RTSP_TR_RTP_MCAST, 0);
        if(pRequest)
        {
            HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendSetupRequest(): multicast: adding RTSP trans request [%p]", this, pRequest);
            m_transportRequestList.AddTail(pRequest);
        }
        else
        {
            rc = HXR_OUTOFMEMORY;
            goto cleanup;
        }

        for (i=m_streamInfoList.Begin();i!=m_streamInfoList.End();++i)
        {
            pStreamInfo = (RTSPStreamInfo*)(*i);

            pUDPSocket = (IHXSocket*)(*m_pUDPSocketStreamMap)[pStreamInfo->m_streamNumber];
            pRTCPUDPSocket = (IHXSocket*)(*m_pRTCPSocketStreamMap)[pStreamInfo->m_streamNumber];

            // create a new transport for each setup
            pTrans = new RTPUDPTransport(m_bSetupRecord);
            if(!pTrans)
            {
                rc = HXR_OUTOFMEMORY;
                goto cleanup;
            }
            HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendSetupRequest():  RTP UDP transport [%p]", this, pTrans);

            pTrans->AddRef();

            if (HXR_OK != pTrans->init(m_pContext, pUDPSocket, this))
            {
                rc = HXR_BAD_TRANSPORT;
                goto cleanup;
            }
            pTrans->notifyRTPInfoProcessed();

            // create an RTCP transport for this stream
            pRTCPTrans = new RTCPUDPTransport(m_bSetupRecord);
            if (!pRTCPTrans)
            {
                rc = HXR_OUTOFMEMORY;
                goto cleanup;
            }
            HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendSetupRequest(): RTCP UDP transport [%p]", this, pRTCPTrans);

            pRTCPTrans->AddRef();

            if (HXR_OK != pRTCPTrans->init(m_pContext, pRTCPUDPSocket, pTrans,
                                           this, pStreamInfo->m_streamNumber))
            {
                rc = HXR_BAD_TRANSPORT;
                goto cleanup;
            }
            pRTCPTrans->notifyRTPInfoProcessed();

            pTrans->setRTCPTransport(pRTCPTrans);

            HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendSetupRequest(): updating stream,port map stream %u; port = %u", this, pStreamInfo->m_streamNumber, pStreamInfo->m_sPort);
            if ((!m_bHasSyncMasterStream) &&
                (pStreamInfo->m_eMediaType == RTSPMEDIA_TYPE_AUDIO))
            {
                pStreamInfo->m_bIsSyncMaster = TRUE;
                m_bHasSyncMasterStream = TRUE;
            }
            pTrans->addStreamInfo(pStreamInfo);
            HX_ASSERT(0 == m_pTransportStreamMap->Lookup(pStreamInfo->m_streamNumber));
            (*m_pTransportStreamMap)[pStreamInfo->m_streamNumber] = pTrans;

            if (!m_activeTransportList.Find(pTrans))
            {
                m_activeTransportList.AddTail(pTrans);
            }

            HX_ASSERT(pStreamInfo->m_pMulticastAddr);
            if(FAILED(pStreamInfo->m_pMulticastAddr->Clone(&pAddrOne)) ||
               FAILED(pStreamInfo->m_pMulticastAddr->Clone(&pAddrTwo)))
            {
                rc = HXR_OUTOFMEMORY;
                goto cleanup;
            }

            pAddrOne->SetPort(pStreamInfo->m_sPort);
            pAddrTwo->SetPort(pStreamInfo->m_sPort + 1);

            // timeout logic handles case where join fails
            pTrans->JoinMulticast(pAddrOne);
            pRTCPTrans->JoinMulticast(pAddrTwo);


            (*m_pTransportMPortMap)[pStreamInfo->m_sPort] = pTrans;
            (*m_pTransportMPortMap)[pStreamInfo->m_sPort+1] = pRTCPTrans;



            mapControlToStreamNo(pStreamInfo->m_streamControl, pStreamInfo->m_streamNumber);

            rc = pRequest->addTransportInfo(pTrans, (RTCPBaseTransport*)pRTCPTrans, pStreamInfo->m_streamNumber);
            if (rc == HXR_OUTOFMEMORY)
            {
                goto cleanup;
            }

            pUDPSocket->SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);
            pRTCPUDPSocket->SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);

            pUDPSocket->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
            pRTCPUDPSocket->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
        }

        m_uProtocolType = 1;
        m_pResp->HandleSetupResponse(HXR_OK);

        goto cleanup;
    }
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */

#ifdef HELIX_FEATURE_AUTHENTICATION
    rc = extractExistingAuthorizationInformation(pRequestHeaders);
    if (rc == HXR_OUTOFMEMORY) // XXXLCM what about other errors?
    {
        goto cleanup;
    }
#endif /* HELIX_FEATURE_AUTHENTICATION */

    if(pTransType)
    {
        for (int i = 0; i < nTransTypes; ++i)
        {
            RTSPTransportRequest* pRequest =
                new RTSPTransportRequest(pTransType[i].m_lTransportType,
                                         pTransType[i].m_sPort);
            if(pRequest)
            {
                m_transportRequestList.AddTail(pRequest);
            }
            else
            {
                rc = HXR_OUTOFMEMORY;
                goto cleanup;
                break;
            }
        }
    }

    if (m_bIPTV && rc == HXR_OK)
    {
        // we are going to keep this around for now to send cookie in all SETUP.
        m_pSetupRequestHeader = pRequestHeaders;
        m_pSetupRequestHeader->AddRef();
    }


    if (rc == HXR_OK)
    {
        rc =  sendFirstSetupRequest(pRequestHeaders);
    }

  cleanup:

    if (HXR_OK != rc)
    {
        HX_RELEASE(pRTCPTrans);
        HX_RELEASE(pTrans);
    }

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    HX_RELEASE(pAddrOne);
    HX_RELEASE(pAddrTwo);
#endif

    m_pMutex->Unlock();

    return rc;
}

HX_RESULT
RTSPClientProtocol::sendFirstSetupRequest(IHXValues* pRequestHeaders)
{
    HX_RESULT   rc = HXR_FAILED;

    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendFirstSetupRequest()", this);

    HX_ASSERT(m_activeTransportList.IsEmpty());
    m_activeTransportList.RemoveAll();

    m_setupResponseCount = 0;
    if (!m_streamInfoList.IsEmpty())
    {
        RTSPStreamInfo* pStreamInfo = (RTSPStreamInfo*)m_streamInfoList.GetHead();
        if(pStreamInfo)
        {
            rc = sendSetupRequestMessage(pStreamInfo, pRequestHeaders, TRUE);
        }
    }

    return rc;
}

HX_RESULT
RTSPClientProtocol::sendRemainingSetupRequests()
{
    HX_RESULT status = HXR_OK;
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendRemainingSetupRequests()", this);
    
    // we should enter this code only once per protocol object.
    if( !m_bHaveSentRemainingSetupRequests )
    {
        CHXSimpleList::Iterator i;
        HXBOOL bFirst = TRUE;
        for(i=m_streamInfoList.Begin();
            status == HXR_OK && (i!=m_streamInfoList.End());++i)
        {
            if(bFirst)
            {
                bFirst = FALSE;
            }
            else
            {
                RTSPStreamInfo* pStreamInfo = (RTSPStreamInfo*)(*i);
                status = sendSetupRequestMessage(pStreamInfo, NULL, FALSE);
            }
        }
        m_bHaveSentRemainingSetupRequests = TRUE;
    }
    return status;
}

HX_RESULT
RTSPClientProtocol::sendSetupRequestMessage(RTSPStreamInfo* pStreamInfo,
                                            IHXValues* pIHXValuesRequestHeaders,
                                            HXBOOL bFirstSetup)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendSetupRequestMessage(): stream %u", this, pStreamInfo->m_streamNumber);
    m_pMutex->Lock();
    RTSPSetupMessage* pMsg = new RTSPSetupMessage;
    if(!pMsg)
    {
        m_pMutex->Unlock();
        return HXR_OUTOFMEMORY;
    }

    HX_RESULT status = HXR_OK;
    status = sendSetupRequestMessageExt(pStreamInfo,
                                        pIHXValuesRequestHeaders,
                                        bFirstSetup,
                                        pMsg);

    pMsg->addHeader("User-Agent", m_versionString);

    if (bFirstSetup && !m_sessionID.IsEmpty())
    {
        pMsg->addHeader("If-Match", m_sessionID);
    }
    else if (!m_sessionID.IsEmpty())
    {
        pMsg->addHeader("Session", m_sessionID);
    }

    // append stream control string to request
    pMsg->setURL(getSetupRequestURL(pStreamInfo));

    if (m_pRateAdaptInfo)
    {
        addRateAdaptationHeaders(pMsg, pStreamInfo);
    }

    CHXString linkChar = createStreamLinkCharHdr(pStreamInfo);
    if (linkChar.GetLength())
    {
        pMsg->addHeader("3GPP-Link-Char", (const char*)linkChar);
    }

    if (pIHXValuesRequestHeaders)
    {
        addUAProfHeaders(pIHXValuesRequestHeaders);
        addRFC822Headers(pMsg, pIHXValuesRequestHeaders);
    }

    UINT32 seqNo;
    seqNo = m_pSession->getNextSeqNo(this);

    /* Why are we not checking for any error code from above ??? */
    status = sendRequest(pMsg, seqNo);

    m_pMutex->Unlock();
    return status;
}

HX_RESULT
RTSPClientProtocol::sendSetupRequestMessageExt(RTSPStreamInfo* pStreamInfo,
                                               IHXValues*& pIHXValuesRequestHeaders,
                                               HXBOOL bFirstSetup,
                                               RTSPSetupMessage*& pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendSetupRequestMessageExt(): stream %u", this, pStreamInfo->m_streamNumber);
    MIMEHeader* pHeader = new MIMEHeader("Transport");
    if(!pHeader)
    {
        return HXR_OUTOFMEMORY;
    }

    HX_RESULT status = HXR_OK;
    CHXSimpleList::Iterator i;
    IHXSockAddr* pRtpAddr = NULL;
    IHXSockAddr* pRtcpAddr = NULL;

    for(i=m_transportRequestList.Begin();i!=m_transportRequestList.End();++i)
    {
        RTSPTransportRequest* pRequest = (RTSPTransportRequest*)(*i);
        UINT16 streamNumber = pStreamInfo->m_streamNumber;

#if defined(HELIX_FEATURE_RTP)
        switch(pRequest->m_lTransportType)
        {
           case RTSP_TR_RTP_UDP:
           {
               // create a new transport for each setup
               RTPUDPTransport* pTrans =
                   new RTPUDPTransport(m_bSetupRecord);
               if(!pTrans)
               {
                   HX_DELETE(pHeader);
                   return HXR_OUTOFMEMORY;
               }
               pTrans->AddRef();
               HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendSetupRequestMessageExt(): created RTP UDP trans [%p]", this, pTrans);

               if (m_bPrefetch)
               {
                   pTrans->EnterPrefetch();
               }

               IHXSocket* pRtpSocket = (IHXSocket*)
                   (*m_pUDPSocketStreamMap)[streamNumber];
               IHXSocket* pRtcpSocket = (IHXSocket*)
                   (*m_pRTCPSocketStreamMap)[streamNumber];

               // We should not get here if we fail to initialize the UDP sockets in
               // InitSockets().
               // We will either switch to TCP or error out if TCP is not available.
               HX_ASSERT(pRtpSocket && pRtcpSocket);

               pRtpSocket->GetLocalAddr(&pRtpAddr);
               pRtcpSocket->GetLocalAddr(&pRtcpAddr);

               if (HXR_OK != pTrans->init(m_pContext, pRtpSocket, this))
               {
                   pTrans->Release();
                   return HXR_BAD_TRANSPORT;
               }

               // create an RTCP transport for this stream
               RTCPUDPTransport* pRTCPTran = new RTCPUDPTransport(m_bSetupRecord);
               if(!pRTCPTran)
               {
                   HX_DELETE(pHeader);
                   HX_RELEASE(pTrans);
                   return HXR_OUTOFMEMORY;
               }
               pRTCPTran->AddRef();
               pRTCPTran->init(m_pContext, pRtcpSocket, pTrans, this, streamNumber);

               pTrans->setRTCPTransport(pRTCPTran);

               status = pRequest->addTransportInfo(pTrans, pRTCPTran, streamNumber, pRtpAddr);

               AddPortToStreamMapping(pRtpAddr->GetPort(), streamNumber);

               if (m_bIPTV && m_pSetupRequestHeader && status != HXR_OUTOFMEMORY)
               {
                   addRFC822Headers(pMsg, m_pSetupRequestHeader);
                   // don't add it twice...
                   pIHXValuesRequestHeaders = NULL;
               }
           }
           break;

           case RTSP_TR_RTP_TCP:
           {
               RTPTCPTransport* pTrans = new RTPTCPTransport(m_bSetupRecord);
               if(!pTrans)
               {
                   HX_DELETE(pHeader);
                   return HXR_OUTOFMEMORY;
               }
               pTrans->AddRef();
               HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendSetupRequestMessageExt(): created RTP TCP trans [%p]", this, pTrans);

               if (m_bPrefetch)
               {
                   pTrans->EnterPrefetch();
               }

               if (HXR_OK != pTrans->init(m_pContext, m_pSocket, this))
               {
                   status = HXR_BAD_TRANSPORT;
               }

               // create an RTCP transport for this stream
               RTCPTCPTransport* pRTCPTran = new RTCPTCPTransport(m_bSetupRecord);
               if(!pRTCPTran)
               {
                   HX_DELETE(pHeader);
                   HX_DELETE(pTrans);
                   return HXR_OUTOFMEMORY;
               }
               pRTCPTran->AddRef();
               pRTCPTran->init(m_pContext, m_pSocket, pTrans, this, streamNumber);

               pTrans->setRTCPTransport(pRTCPTran);

               status = pRequest->addTransportInfo(pTrans, pRTCPTran, streamNumber);

               if (m_bIPTV && m_pSetupRequestHeader)
               {
                   addRFC822Headers(pMsg, m_pSetupRequestHeader);
                   // don't add it twice...
                   pIHXValuesRequestHeaders = NULL;
               }
           }
           break;

           default:
           {
           }
           break;
        }
#endif /* HELIX_FEATURE_RTP */

        char* pModifiedMimeType = NULL;
        const char* pMimeType =
            RTSPTransportMimeMapper::getTransportMimeType(pRequest->m_lTransportType);

        // Accomodate incompliant servers that understand only upper case
        // transport mime-types
        if (m_bForceUCaseTransportMimeType)
        {
            ULONG32 ulMimeTypeLength = strlen(pMimeType);

            if (ulMimeTypeLength != 0)
            {
                pModifiedMimeType = new char [ulMimeTypeLength + 1];
                if(!pModifiedMimeType)
                {
                    HX_DELETE(pHeader);
                    return HXR_OUTOFMEMORY;
                }
            }

            if (pModifiedMimeType)
            {
                strcpy(pModifiedMimeType, pMimeType); /* Flawfinder: ignore */
                StrToUpper(pModifiedMimeType);
                pMimeType = pModifiedMimeType;
            }
        }

#if defined(HELIX_FEATURE_RTP)
        switch(pRequest->m_lTransportType)
        {
           case RTSP_TR_RTP_UDP:
           case RTSP_TR_RTP_TCP:
           {
               MIMEHeaderValue* pHeaderValue = new MIMEHeaderValue(pMimeType);

               if(!pHeaderValue)
               {
                   HX_DELETE(pHeader);
                   HX_DELETE(pModifiedMimeType);
                   return HXR_OUTOFMEMORY;
               }

               if (RTSP_TR_RTP_UDP == pRequest->m_lTransportType)
               {
                   char portValue[32]; /* Flawfinder: ignore */

                   // This is not very pretty... maka an IHXSockAddr::GetPort?
                   UINT16 nRtpPort = pRtpAddr ? pRtpAddr->GetPort() : 0;
                   UINT16 nRtcpPort = pRtcpAddr ? pRtcpAddr->GetPort() : 0;
                   SafeSprintf(portValue, 32, "%hu-%hu", nRtpPort, nRtcpPort);
                   pHeaderValue->addParameter("client_port", (const char*)portValue);
               }

               if(m_bSetupRecord)
               {
                   pHeaderValue->addParameter("mode", "record");
               }
               else
               {
                   pHeaderValue->addParameter("mode", "play");
               }
               pHeader->addHeaderValue(pHeaderValue);
           }
           break;

           case RTSP_TR_RTP_MCAST:
           default:
           {
           }
           break;
        }
#endif /* HELIX_FEATURE_RTP */

        HX_VECTOR_DELETE(pModifiedMimeType);
    }
    pMsg->addHeader(pHeader);

    HX_RELEASE(pRtcpAddr);
    HX_RELEASE(pRtpAddr);

    return status;
}

STDMETHODIMP
RTSPClientProtocol::SendPlayRequest(UINT32 lFrom, UINT32 lTo,
                                    CHXSimpleList* pASMRules)
{

    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendPlayRequest(): %lu to %lu", this, lFrom, lTo);

    /*
     * Flush the data packets out of the transport buffers
     */

    m_pMutex->Lock();

    m_bPaused = FALSE;

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    if (m_bSDPInitiated && m_bMulticast)
    {
        m_pMutex->Unlock();
        return m_pResp->HandlePlayResponse(HXR_OK);
    }
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */

    /*
     * XXXGH...I believe we should be iterating through m_transportRequestList
     *         here and for SendPauseRequest, SendResumeRequest, etc.
     */

    // only used when m_bNonRSRTP is TRUE
    m_bPlayJustSent = TRUE;

    if (!m_transportRequestList.IsEmpty())
    {
        RTSPTransportRequest* pRequest =
            (RTSPTransportRequest*)m_transportRequestList.GetHead();
        RTSPTransportInfo* pTransInfo = pRequest->getFirstTransportInfo();
        while(pTransInfo)
        {
            pTransInfo->m_pTransport->playReset();
            // set the range in transport...only for RTP
            pTransInfo->m_pTransport->setPlayRange(lFrom, lTo);
            pTransInfo->m_pTransport->SetPlayRequestSent(TRUE);
            pTransInfo->m_pTransport->resumeBuffers();
            pTransInfo = pRequest->getNextTransportInfo();
        }
    }

    HX_RESULT rc = HXR_OK;
    RTSPPlayMessage* pMsg = new RTSPPlayMessage;
    if(pMsg)
    {
        RTSPRange range(lFrom, lTo, RTSPRange::TR_NPT);

        pMsg->setURL(getAggControlURL());
        AddCommonHeaderToMsg(pMsg);

        pMsg->addHeader("Range", (const char*)range.asString());

        UINT32 uConnectionBW = m_pSession->getConnectionBW();
        if (uConnectionBW)
        {
            CHXString buffer;
            buffer.AppendULONG(uConnectionBW);
            pMsg->addHeader("Bandwidth", buffer);
        }

        CHXString linkChar = createSessionLinkCharHdr();
        if (linkChar.GetLength())
        {
            pMsg->addHeader("3GPP-Link-Char", (const char*)linkChar);
        }

        if (m_pRateAdaptInfo)
        {
            RTSPStreamInfo* pStreamInfo = NULL;
            for(CHXSimpleList::Iterator i=m_streamInfoList.Begin();i!=m_streamInfoList.End();++i)
            {
                pStreamInfo = (RTSPStreamInfo*)(*i);
                addRateAdaptationHeaders(pMsg, pStreamInfo);
            }
        }

        UINT32 seqNo = m_pSession->getNextSeqNo(this);
        rc = sendRequest(pMsg, seqNo);

	if (SUCCEEDED(rc))
	{
	    rc = augmentRequestPendingReplyList(m_PlayRequestPendingReplyList, 
						seqNo);
	}
    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }
    m_pMutex->Unlock();
    return rc;
}


STDMETHODIMP
RTSPClientProtocol::SendRecordRequest()
{
    if (!m_pIsMethodSupported[RECORD] || !m_pSession)
    {
        return HXR_OK;
    }

    HX_RESULT rc = HXR_OK;
    m_pMutex->Lock();
    // Declaring these here so I can use a goto below!
    CHXString streamSequenceNumbers;
    HXBOOL bIsFirst = TRUE;
    CHXMapLongToObj::Iterator i;

    RTSPRecordMessage* pMsg = new RTSPRecordMessage;
    if(!pMsg)
    {
        rc =  HXR_OUTOFMEMORY;
        goto overandout;
    }

    pMsg->setURL(getAggControlURL());
    AddCommonHeaderToMsg(pMsg);

    /*
     * Add header for sequence numbers
     */

    for(i=m_pTransportStreamMap->Begin(); i!=m_pTransportStreamMap->End(); ++i)
    {

        int lenTmpBuf = (int)(100 + strlen(m_url));
        char* tmpBuf = new char[lenTmpBuf];
        if(!tmpBuf)
        {
            HX_DELETE(pMsg);
            rc =  HXR_OUTOFMEMORY;
            goto overandout;
        }

        RTSPTransport* pTransport = (RTSPTransport*)(*i);
        pTransport->m_bHackedRecordFlag = TRUE;
        UINT16 streamNumber = (UINT16)i.get_key();
        UINT16 seqNum = pTransport->getSeqNum(streamNumber);
        UINT32 ulTimestamp = pTransport->getTimestamp(streamNumber);
        SafeSprintf(tmpBuf, (UINT32)lenTmpBuf, "url=" + m_url +
                    "/streamid=%d;seq=%d;rtptime=%ld", streamNumber, seqNum,
                    ulTimestamp);
        if(!bIsFirst)
        {
            streamSequenceNumbers += ", " + CHXString(tmpBuf);
        }
        else
        {
            bIsFirst = FALSE;
            streamSequenceNumbers = tmpBuf;
        }

        delete[] tmpBuf;
    }
    pMsg->addHeader("RTP-Info", streamSequenceNumbers);
    if( m_pSession )
    {
        UINT32 seqNo = m_pSession->getNextSeqNo(this);
        rc = sendRequest(pMsg, seqNo);
    }
  overandout:
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendPauseRequest()
{
    m_bPaused = TRUE;

    /*
     * Stop the internal buffer timers
     */
    if (!m_pIsMethodSupported[PAUSE] || m_transportRequestList.IsEmpty() ||
        !m_pSession)
    {
        return HXR_OK;
    }

    m_pMutex->Lock();

    // only used when m_bNonRSRTP is TRUE
    m_bPlayJustSent = FALSE;

    SendMsgToTransport(PAUSE_BUFFER);

    HX_RESULT rc = SendMsgToServer(RTSP_PAUSE);

    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendResumeRequest()
{
    m_bPaused = FALSE;

    if (!m_pSession)
    {
        return HXR_OK;
    }

    /*
     * Restart the internal buffer timers
     */

    m_pMutex->Lock();

    SendMsgToTransport(RESUME_BUFFER);

    /*
     * Man, iptv, teracast, and darwin server don't like this even though
     * this is perfetly legal...
     */
    if (m_bNonRSRTP && m_bPlayJustSent)
    {
        m_pResp->HandlePlayResponse(HXR_OK);
        m_pMutex->Unlock();
        return HXR_OK;
    }

    UINT32 ulMsgSeqNum = 0;

    HX_RESULT rc = SendMsgToServer(RTSP_PLAY, &ulMsgSeqNum);

    if (SUCCEEDED(rc))
    {
	rc = augmentRequestPendingReplyList(m_ResumeRequestPendingReplyList, 
					    ulMsgSeqNum);
    }
    
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendTeardownRequest()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendTeardownRequest()", this);

    // make sure not to send a TEARDOWN unless SETUP succeeded
    if (m_setupResponseCount <= 0 || !m_pSession)
    {
        // no successful SETUP response received...
        return HXR_OK;
    }

    // it's ok if there is no session by spec.
    m_pMutex->Lock();
    HX_RESULT rc = SendMsgToServer(RTSP_TEARDOWN);
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendPlayerStats(const char* pStats)
{
    HX_RESULT rc = HXR_OK;

    if (!pStats)
    {
        return HXR_INVALID_PARAMETER;
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    if (m_bSDPInitiated && m_bMulticast)
    {
        // don't send stats if
        // - it's not explicitly asked for AND
        // - we failed to receive any multicast packet(m_currentTransport is set
        //   to MulticastMode after we successfully join the multicast group)
        if (m_bMulticastStats && MulticastMode == m_currentTransport)
        {
            m_pMutex->Lock();

            CHXString host;
            CHXString resource;
            UINT32 port = 0;
            IHXValues* pPostHeader = NULL;
            IHXBuffer* pPostBody = NULL;
            if (HXR_OK == preparePOSTStatsMsg(host, port, resource, pPostHeader))
            {
                rc = CreateBufferCCF(pPostBody, m_pContext);
                if (HXR_OK == rc)
                {
                    pPostBody->Set((const unsigned char*)pStats, strlen(pStats)+1);

                    // CHTTPPost instance will self-destruct after POST
                    CHTTPPost* pPost = new CHTTPPost();
                    if (pPost && HXR_OK == pPost->Init(m_pContext, 3000))
                    {
                        rc = pPost->Post((const char*)host,
                                         port,
                                         (const char*)resource,
                                         pPostHeader,
                                         pPostBody);
                    }
                }
                HX_RELEASE(pPostBody);
            }
            HX_RELEASE(pPostHeader);

            m_pMutex->Unlock();
        }
    }
    else
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
    {
        if (!m_pIsMethodSupported[SET_PARAM])
        {
            return rc;
        }

        if(m_pSession && !m_sessionID.IsEmpty())
        {
            m_pMutex->Lock();
            RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
            if(pMsg)
            {
                pMsg->setURL(getAggControlURL());
                pMsg->addHeader("Session", m_sessionID);
                pMsg->addHeader("PlayerStats", pStats);
                UINT32 seqNo = m_pSession->getNextSeqNo(this);
                rc = sendRequest(pMsg, seqNo);
            }
            else
            {
                rc = HXR_OUTOFMEMORY;
            }
            m_pMutex->Unlock();
        }
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendKeepAlive()
{
    HX_RESULT rc = HXR_OK;

    // XXXSMP - Not right! :-)
    m_pMutex->Lock();

    if (!m_pSession)
    {
        // just say alive!
        m_pMutex->Unlock();
        return HXR_OK;
    }

    // If using session timeout code, send an Options message,
    // otherwise, send a SetParam.  The SetParam approach is
    // is for servers that do not specify a session timeout value.
    if (!m_bUseLegacyTimeOutMsg ||
        !m_pIsMethodSupported[SET_PARAM] ||
        m_bNoKeepAlive)
    {
        rc = SendOptionsMsgToServer(TRUE);
    }
    else
    {
        RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
        if(pMsg)
        {
            pMsg->setURL("*");
            MIMEHeader* pAlertHeader = new MIMEHeader("Ping");
            if(pAlertHeader)
            {
                pAlertHeader->addHeaderValue("Pong");
                pMsg->addHeader(pAlertHeader);

                AddCommonHeaderToMsg(pMsg);

                UINT32 seqNo = m_pSession->getNextSeqNo(this);
                sendRequest(pMsg, seqNo);
            }
            else
            {
                rc = HXR_OUTOFMEMORY;
            }
        }
        else
        {
            rc = HXR_OUTOFMEMORY;
        }
    }

    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendPacket(BasePacket* pPacket)
{
    m_pMutex->Lock();
    HX_RESULT rc = HXR_UNEXPECTED;
    RTSPTransport* pTrans = GetTransport(pPacket->GetStreamNumber());
    HX_ASSERT(pTrans);
    if(pTrans)
    {
        rc = pTrans->sendPacket(pPacket);
    }
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendStreamDone(UINT16 streamNumber)
{
    m_pMutex->Lock();
    HX_RESULT rc = HXR_UNEXPECTED;
    RTSPTransport* pTrans = GetTransport(streamNumber);
    HX_ASSERT(pTrans);
    if(pTrans)
    {
        rc = pTrans->streamDone(streamNumber);
    }
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::GetPacket(UINT16 uStreamNumber, REF(IHXPacket*) pPacket)
{
    m_pMutex->Lock();

    /*
     * Must not return HXR_FAIL because player may request a packet
     * before the transport is set up
     */
    HX_RESULT rc = HXR_NO_DATA;

    RTSPTransport* pTrans = GetTransport(uStreamNumber);
    //HX_ASSERT(pTrans);
    if (pTrans)
    {
        UINT32 uSeqNum;
        rc = pTrans->getPacket(uStreamNumber, pPacket, uSeqNum);

        if ((HXR_OK == rc) && m_pSrcBufStats &&
            (!pPacket->IsLost()))
        {
            HX_ASSERT(uSeqNum <= 0xFFFF);
            m_pSrcBufStats->OnPacket(pPacket, UINT16(uSeqNum));
        }
    }

    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::StartPackets(UINT16 uStreamNumber)
{
    m_pMutex->Lock();
    HX_RESULT rc = HXR_FAIL;
    RTSPTransport* pTrans = GetTransport(uStreamNumber);
    HX_ASSERT(pTrans);
    if (pTrans)
    {
        rc = pTrans->startPackets(uStreamNumber);
    }

    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::StopPackets(UINT16 uStreamNumber)
{
    m_pMutex->Lock();
    HX_RESULT rc = HXR_FAIL;
    RTSPTransport* pTrans = GetTransport(uStreamNumber);
    HX_ASSERT(pTrans);
    if (pTrans)
    {
        /*
         * Must not return HXR_FAIL because player may request a packet
         * before the transport is set up
         */

        rc = pTrans->stopPackets(uStreamNumber);
    }

    m_pMutex->Unlock();
    return rc;
}

/*
 * XXX...This had BETTER GET FIXED when we go to full IRMA
 */

STDMETHODIMP_(IHXPendingStatus*)
    RTSPClientProtocol::GetPendingStatus()
{
    AddRef();
    return (IHXPendingStatus*)this;
}

STDMETHODIMP_(IHXStatistics*)
    RTSPClientProtocol::GetStatistics()
{
    AddRef();
    return (IHXStatistics*)this;
}

STDMETHODIMP_(HXBOOL)
    RTSPClientProtocol::HttpOnly()
{
    if(m_pSession)
    {
        return m_pSession->HttpOnly();
    }

    return FALSE;
}

STDMETHODIMP
RTSPClientProtocol::SendGetParameterRequest(UINT32 lParamType,
                                            const char* pParamName)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendGetParameterRequest(): %s", this, pParamName);

    if (!m_pIsMethodSupported[GET_PARAM])
    {
        return HXR_OK;
    }

    HX_RESULT rc = HXR_OK;
    m_pMutex->Lock();
    RTSPGetParamMessage* pMsg = new RTSPGetParamMessage;
    if(pMsg)
    {
        pMsg->setURL("*");
        AddCommonHeaderToMsg(pMsg);

        // send ABD request
        if (0 == strcasecmp(pParamName, "ABD"))
        {
            // DEFAULT_ABD_PROBPKT & DEFAULT_ABD_PROBPKT_SIZE are defined
            // in common/include/protdefs.h
            UINT8  uPktCount = DEFAULT_ABD_PROBPKT;
            UINT32 uPktSize  = DEFAULT_ABD_PROBPKT_SIZE;

            // Allow preferences to override the defaults
            ReadPrefUINT8(m_pPreferences, "AutoBWDetectionPackets", uPktCount);
            ReadPrefUINT32(m_pPreferences, "AutoBWDetectionPacketSize",
                           uPktSize);

            pMsg->addHeader("AutoBWDetection", "1");

            CHXString pktCountStr;
            pktCountStr.AppendULONG(uPktCount);
            pMsg->addHeader("AutoBWDetectionPackets", pktCountStr);

            CHXString pktSizeStr;
            pktSizeStr.AppendULONG(uPktSize);
            pMsg->addHeader("AutoBWDetectionPacketSize", pktSizeStr);

            // Notify all the transports that we are requesting uPktCount
            // ABD probing packets
            CHXMapLongToObj::Iterator i;
            for(i=m_pTransportStreamMap->Begin();
                (rc == HXR_OK) &&  i != m_pTransportStreamMap->End(); ++i)
            {
                RTSPTransport* pTransport = (RTSPTransport*)(*i);
                HX_ASSERT(pTransport);

                if (pTransport)
                {
                    rc = pTransport->SetProbingPacketsRequested(uPktCount);
                }
            }
        }

        if (HXR_OK == rc)
        {
            UINT32 seqNo = m_pSession->getNextSeqNo(this);
            rc = sendRequest(pMsg, pParamName, "text/rtsp-parameters", seqNo);
        }
        else
        {
            HX_DELETE(pMsg);
        }
    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendSetParameterRequest(UINT32 lParamType,
                                            const char* pParamName, IHXBuffer* pParamValue)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendSetParameterRequest(): %s = %s", this, pParamName, pParamValue);

    if (!m_pIsMethodSupported[SET_PARAM])
    {
        return HXR_OK;
    }

    m_pMutex->Lock();
    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
    pMsg->setURL(getAggControlURL());
    AddCommonHeaderToMsg(pMsg);

    pMsg->addHeader(pParamName, (const char*)pParamValue->GetBuffer());
    UINT32 seqNo = m_pSession->getNextSeqNo(this);

    HX_RESULT rc = sendRequest(pMsg, seqNo);
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendSetParameterRequest(const char* pParamName,
                                            const char* pParamValue, const char* pMimeType,
                                            const char* pContent)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SendSetParameterRequest(): %s = %s", this, pParamName, pParamValue);
    if (!m_pIsMethodSupported[SET_PARAM])
    {
        return HXR_OK;
    }

    m_pMutex->Lock();
    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
    pMsg->setURL(getAggControlURL());
    AddCommonHeaderToMsg(pMsg);

    pMsg->addHeader(pParamName, pParamValue);
    UINT32 seqNo = m_pSession->getNextSeqNo(this);

    HX_RESULT rc = sendRequest(pMsg, pContent, pMimeType, seqNo);
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::PacketReady(HX_RESULT status, const char* pSessionID,
                                IHXPacket* pPacket)
{
    m_pMutex->Lock();
    if (pPacket)
    {
	HXLOGL4(HXLOG_TRAN, "RTSPClientProtocol::PacketReady() PacketReady: StrmNum=%hu TS=%lu ASMRule=%hu",
	    pPacket->GetStreamNumber(),
	    pPacket->GetTime(),
	    pPacket->GetASMRuleNumber());
    }
    HX_RESULT rc = m_pResp->HandlePacket(status, pSessionID, pPacket);
    m_pMutex->Unlock();
    return rc;
}

/*
 * OnRTTRequest() and OnBWReport() are server-side functions
 */

STDMETHODIMP
RTSPClientProtocol::OnRTTRequest(HX_RESULT status, const char* pSessionID)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP
RTSPClientProtocol::OnRTTResponse(HX_RESULT status, const char* pSessionID,
                                  UINT32 ulSecs, UINT32 ulUSecs)
{
    m_pMutex->Lock();
    HX_RESULT rc = m_pResp->HandleRTTResponse(status, pSessionID,
                                              ulSecs, ulUSecs);
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::OnBWReport(HX_RESULT status, const char* pSessionID,
                               INT32 aveBandwidth, INT32 packetLoss,
                               INT32 bandwidthWanted)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
RTSPClientProtocol::OnCongestion(HX_RESULT status, const char* pSessionID,
                                 INT32 xmitMultiplier, INT32 recvMultiplier)
{
    m_pMutex->Lock();
    HX_RESULT rc = m_pResp->HandleCongestion(status, pSessionID,
                                             xmitMultiplier, recvMultiplier);
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::OnACK(HX_RESULT status, RTSPResendBuffer* pResendBuffer,
                          UINT16 uStreamNumber, const char* pSessionID,
                          UINT16* pAckList, UINT32 uAckListCount,
                          UINT16* pNakList, UINT32 uNakListCount)
{
    /*
     * While it's ACKing, remote client is alive
     */

    m_bConnectionAlive = TRUE;

    m_pMutex->Lock();
    HX_RESULT rc = handleACK((IHXPacketResend*)this, pResendBuffer,
                             uStreamNumber,
                             pAckList, uAckListCount,
                             pNakList, uNakListCount,
                             FALSE);
    m_pMutex->Unlock();
    return rc;
};

STDMETHODIMP
RTSPClientProtocol::OnStreamDone(HX_RESULT status, UINT16 uStreamNumber)
{
    m_pMutex->Lock();
    HX_RESULT rc = m_pResp->HandleStreamDone(status, uStreamNumber);
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::OnSourceDone(void)
{
    m_pMutex->Lock();
    HX_RESULT rc = m_pResp->HandleSourceDone();
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::OnProtocolError(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::OnProtocolError(): %08x", this, status);
    HX_RESULT rc = HXR_OK;
    // called from transport layer
    m_pMutex->Lock();
    if (m_sessionList.IsEmpty() || m_bSessionSucceeded)
    {
        rc = m_pResp->HandleProtocolError(status);
    }
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::GetStatus
(
    REF(UINT16) uStatusCode,
    REF(IHXBuffer*) pStatusDesc,
    REF(UINT16) ulPercentDone
    )
{
#if 0
    m_pMutex->Lock();
    HX_RESULT rc = HXR_OK;

    CHXMapLongToObj::Iterator i = m_pTransportStreamMap->Begin();
    if(i != m_pTransportStreamMap->End())
    {
        RTSPTransport* pTrans = (RTSPTransport*)(*i);
        HX_ASSERT(pTrans);
        rc = pTrans ? pTrans->getStatus(uStatusCode, pStatusDesc, ulPercentDone) : HXR_OK;
    }
    else
    {
        uStatusCode = HX_STATUS_BUFFERING;
        pStatusDesc = 0;
        ulPercentDone = 0;
    }

    m_pMutex->Unlock();
    return rc;
#else
    return HXR_NOTIMPL;
#endif
}

STDMETHODIMP
RTSPClientProtocol::InitializeStatistics
(
    UINT32 ulRegistryID
    )
{
#ifdef HELIX_FEATURE_CLIENT
    m_ulRegistryID = ulRegistryID;
    return HXR_OK;
#else
    HX_RESULT rc = HXR_FAIL;
    m_pMutex->Lock();
    CHXMapLongToObj::Iterator i = m_pTransportStreamMap->Begin();
    if(i != m_pTransportStreamMap->End())
    {
        RTSPTransport* pTrans = (RTSPTransport*)(*i);
        HX_ASSERT(pTrans);
        rc = pTrans ? pTrans->initializeStatistics(ulRegistryID) : HXR_FAIL;
    }
    m_pMutex->Unlock();
    return rc;
#endif
}

STDMETHODIMP
RTSPClientProtocol::UpdateStatistics()
{
    HX_RESULT rc = HXR_FAIL;

    m_pMutex->Lock();

    CHXSimpleList::Iterator i;
    for (i=m_activeTransportList.Begin();i!=m_activeTransportList.End();++i)
    {
        RTSPTransport* pTrans = (RTSPTransport*)(*i);
        rc = pTrans ? pTrans->updateStatistics() : HXR_FAIL;
    }

    m_pMutex->Unlock();

    return rc;
}

STDMETHODIMP
RTSPClientProtocol::OnPacket(UINT16 uStreamNumber, BasePacket** ppPacket)
{
    BasePacket* pPacket;

    m_pMutex->Lock();
    for (; (pPacket = *ppPacket); ppPacket++)
    {
        SendPacket(pPacket);
    }
    m_pMutex->Unlock();
    return HXR_OK;
}

/*
 * RTSPClientProtocol methods
 */
HX_RESULT
RTSPClientProtocol::HandleUnexpected(RTSPMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleUnexpected()", this);
    m_pMutex->Lock();
    RTSPResponseMessage* pRspMsg = makeResponseMessage(pMsg->seqNo(), "405");
    pRspMsg->addHeader("Allow", allowedMethods());
    sendResponse(pRspMsg);
    delete pRspMsg;
    m_pMutex->Unlock();
    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::HandleBadVersion(RTSPMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleBadVersion()", this);
    m_pMutex->Lock();
    RTSPResponseMessage* pRspMsg = makeResponseMessage(pMsg->seqNo(), "505");
    sendResponse(pRspMsg);
    delete pRspMsg;
    m_pMutex->Unlock();
    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::HandleOptions(RTSPOptionsMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleOptions()", this);
    sendResponse(pMsg->seqNo(), "200");
    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::HandleTeardown(RTSPTeardownMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleTeardown()", this);
    m_pMutex->Lock();
    RTSPResponseMessage* pRespMsg = makeResponseMessage(pMsg->seqNo(), "200");
    sendResponse(pRespMsg);
    delete pRespMsg;
    m_pMutex->Unlock();
    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::HandleGetParam(RTSPGetParamMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleGetParam()", this);
    IHXBuffer* pBuffer = 0;

    m_pMutex->Lock();
    const char* pParamName = pMsg->getContent();
    HX_RESULT rc = m_pResp->HandleGetParameterRequest(
        RTSP_PARAM_STRING, pParamName, &pBuffer);

    RTSPResponseMessage* pRespMsg = 0;
    if(rc == HXR_OK)
    {
        pRespMsg = makeResponseMessage(pMsg->seqNo(), "200");
        sendResponse(pRespMsg, (const char*)pBuffer->GetBuffer(),
                     "text/rtsp-parameters");
    }
    else
    {
        pRespMsg = makeResponseMessage(pMsg->seqNo(), "451");
        sendResponse(pRespMsg);
    }
    delete pRespMsg;
    m_pMutex->Unlock();
    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::HandleSetParam(RTSPSetParamMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleSetParam()", this);
    RTSPResponseMessage* pRespMsg = 0;
    HX_RESULT rc = HXR_OK;
    IHXValues* pReconnectValues = NULL;
    HXBOOL paramOK = FALSE;

    m_pMutex->Lock();
    MIMEHeader* pAlert = pMsg->getHeader("Alert");
    MIMEHeader* pMaxASMBW = pMsg->getHeader("MaximumASMBandwidth");
    MIMEHeader* pDataConvert = pMsg->getHeader("DataConvertBuffer");
    MIMEHeader* pReconnect = pMsg->getHeader("Reconnect");
    MIMEHeader* pAlternateServer = pMsg->getHeader("Alternate-Server");
    MIMEHeader* pAlternateProxy = pMsg->getHeader("Alternate-Proxy");
    MIMEHeader* pLastSeqNo = pMsg->getHeader("LastSeqNo");

    if(pAlert)
    {
        paramOK = TRUE;
        MIMEHeaderValue* pHeaderValue = pAlert->getFirstHeaderValue();
        if(pHeaderValue)
        {
            MIMEParameter* pParam = pHeaderValue->getFirstParameter();
            if(pParam)
            {
                const char* pAlertNumber = (const char*)pParam->m_attribute;
                pParam = pHeaderValue->getNextParameter();
                if(pParam)
                {
                    const char* pAlertText = (const char*)pParam->m_attribute;
                    rc = m_pResp->HandleAlertRequest(HXR_OK,
                                                     strtol(pAlertNumber, 0, 10), pAlertText);
                }
            }
        }
    }
    else if(pMaxASMBW)
    {
        paramOK = TRUE;
        MIMEHeaderValue* pHeaderValue = pMaxASMBW->getFirstHeaderValue();
        if(pHeaderValue)
        {
            MIMEParameter* pParam = pHeaderValue->getFirstParameter();
            if(pParam)
            {
                IHXBuffer * pBuffer = NULL;
		CreateBufferCCF(pBuffer, m_pContext);
                if(pBuffer)
                {
                    rc = pBuffer->Set((const unsigned char *)(pParam->m_attribute).
                                      GetBuffer(1), strlen((const char*)pParam->
                                                           m_attribute)+1);
                    if( rc != HXR_OUTOFMEMORY )
                    {
                        rc = m_pResp->HandleSetParameterRequest(
                            RTSP_PARAM_STRING, "MaximumASMBandwidth", pBuffer);
                        pBuffer->Release();
                    }
                    else
                    {
                        HX_RELEASE(pBuffer);
                    }
                }
                else
                {
                    rc = HXR_OUTOFMEMORY;
                }
            }
        }
    }
    else if (pDataConvert)
    {
        rc = m_pResp->HandleSetParameterRequest("DataConvertBuffer",
                                                "1", pMsg->getContent());
    }
    else if (pReconnect)
    {
        CHXString reconnectFlag = pMsg->getHeaderValue("Reconnect");
        if (reconnectFlag != "" && strcasecmp((const char*)reconnectFlag, "false") == 0)
        {
	    if (HXR_OK == CreateValuesCCF(pReconnectValues, m_pContext))
	    {
		pReconnectValues->SetPropertyULONG32("Reconnect", 0);
		rc = m_pResp->HandleSetParameterResponseWithValues(HXR_OK, pReconnectValues);
		HX_RELEASE(pReconnectValues);
	    }
        }
    }
    else if (pAlternateServer)
    {
        rc = RetrieveReconnectInfo(pAlternateServer, ALTERNATE_SERVER, pReconnectValues);
        rc = m_pResp->HandleSetParameterResponseWithValues(HXR_OK, pReconnectValues);
        HX_RELEASE(pReconnectValues);
    }
    else if (pAlternateProxy)
    {
        rc = RetrieveReconnectInfo(pAlternateProxy, ALTERNATE_PROXY, pReconnectValues);
        rc = m_pResp->HandleSetParameterResponseWithValues(HXR_OK, pReconnectValues);
        HX_RELEASE(pReconnectValues);
    }
    else if (pLastSeqNo)
    {
        // this should rarely happens:
        // the server inidicates it fails to send all the ABD packets as requested
        // by the client
        HX_ASSERT(FALSE);
        m_pSession->AutoBWDetectionDone(HXR_UNEXPECTED, 0);
    }
    else
    {
        rc = HXR_UNEXPECTED;
    }

    if(rc == HXR_OK)
    {
        pRespMsg = makeResponseMessage(pMsg->seqNo(), "200");
    }
    else
    {
        pRespMsg = makeResponseMessage(pMsg->seqNo(), "451");
    }

    if (!m_sessionID.IsEmpty())
    {
        pRespMsg->addHeader("Session", m_sessionID);
    }

    sendResponse(pRespMsg);
    delete pRespMsg;
    m_pMutex->Unlock();
    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::HandleUseProxy(RTSPResponseMessage* pMsg)
{
    m_pMutex->Lock();
    HX_RESULT rc = HXR_OK;

    MIMEHeader* pLocation = pMsg->getHeader("Location");
    if(pLocation)
    {
        MIMEHeaderValue* pURLValue = pLocation->getFirstHeaderValue();
        if(pURLValue)
        {
            CHXString proxyURL = pURLValue->value();
            if(proxyURL.GetLength() > 0)
            {
                rc = m_pResp->HandleUseProxyRequest((const char*)proxyURL);
                goto exit;
            }
        }
    }
    // bad redirect, inform the response object
    rc = m_pResp->HandleUseProxyRequest(NULL);
  exit:
    m_pMutex->Unlock();
    return rc;
}

HX_RESULT
RTSPClientProtocol::HandleRedirect(RTSPRedirectMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleRedirect()", this);

    m_pMutex->Lock();
    HX_RESULT rc = HXR_OK;
    RTSPResponseMessage* pRspMsg = makeResponseMessage(pMsg->seqNo(), "200");
    sendResponse(pRspMsg);
    HX_DELETE(pRspMsg);

    UINT32 msFromNow = 0;

    MIMEHeader* pLocation = pMsg->getHeader("Location");
    if(pLocation)
    {
        MIMEHeader* pRangeHeader = pMsg->getHeader("Range");
        if(pRangeHeader)
        {
            RTSPRange* pRange = (RTSPRange*)pRangeHeader->getFirstHeaderValue();
            if(pRange)
            {
                msFromNow = pRange->m_begin;
            }
        }
        MIMEHeaderValue* pURLValue = pLocation->getFirstHeaderValue();
        if(pURLValue)
        {
            CHXString redirectURL = pURLValue->value();
            if(redirectURL.GetLength() > 0)
            {
                rc = m_pResp->HandleRedirectRequest(redirectURL,msFromNow);
                goto exit;
            }
        }
    }
    rc = m_pResp->HandleRedirectRequest(0, 0);
  exit:
    m_pMutex->Unlock();
    return rc;
}

HX_RESULT
RTSPClientProtocol::HandleRedirectResponse(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleRedirectResponse(): %lu", this, pMsg->errorCodeAsUINT32());
    m_pMutex->Lock();

    HX_RESULT rc = HXR_OK;

    if (m_pPipelinedDescLogic)
    {
        m_pPipelinedDescLogic->OnRedirect(pMsg);
    }

    IHXValues* pRFC822Headers = NULL;
    getRFC822Headers(pMsg, pRFC822Headers);

    if(pRFC822Headers)
    {
        IHXKeyValueList* pRFC822List = NULL;

        if (HXR_OK == pRFC822Headers->QueryInterface(IID_IHXKeyValueList, (void**)&pRFC822List))
        {
            m_pResponseHeaders->AppendAllListItems(pRFC822List);
        }
        HX_RELEASE(pRFC822List);
    }
    HX_RELEASE(pRFC822Headers);

    // tell them this is a redirect...
    IHXValues* pResponseHeaders = NULL;

    if (HXR_OK == m_pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseHeaders))
    {
        m_pResp->HandleOptionsResponse(HXR_REDIRECTION, pResponseHeaders);
        HX_RELEASE(pResponseHeaders);
    }
    else
    {
        HX_ASSERT(pResponseHeaders);
        m_pResp->HandleOptionsResponse(HXR_REDIRECTION, NULL);
    }

    UINT32 msFromNow;
    msFromNow = 0;

    MIMEHeader* pLocation;
    pLocation = pMsg->getHeader("Location");
    if(pLocation)
    {
        MIMEHeader* pRangeHeader = pMsg->getHeader("Range");
        if(pRangeHeader)
        {
            RTSPRange* pRange = (RTSPRange*)pRangeHeader->getFirstHeaderValue();
            if(pRange)
            {
                msFromNow = pRange->m_begin;
            }
        }
        MIMEHeaderValue* pURLValue = pLocation->getFirstHeaderValue();
        if(pURLValue)
        {
            CHXString redirectURL = pURLValue->value();
            if(redirectURL.GetLength() > 0)
            {
                rc = m_pResp->HandleRedirectRequest((const char*)redirectURL,
                                                    msFromNow);
                goto exit;
            }
        }
    }

    rc = m_pResp->HandleRedirectRequest(0, 0);
  exit:
    m_pMutex->Unlock();
    return rc;
}

void
RTSPClientProtocol::SessionCreated(RTSPClientSession* pSession)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SessionCreated(): [%p]", this, pSession);
    m_sessionList.AddTail(pSession);
}

// Session setup succeeded. We'll use this and discard others.
void
RTSPClientProtocol::SessionSucceeded(RTSPClientSession* pSession,
                                     IHXSocket* pSocket)
{
    HX_RESULT rc = HXR_OK;

    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SessionSucceeded(): [%p]", this, pSession);

    m_pMutex->Lock();

    m_bSessionSucceeded = TRUE;

    // remove all but the successful session
    while (!m_sessionList.IsEmpty())
    {
        RTSPClientSession* pTempSession = (RTSPClientSession*)m_sessionList.RemoveHead();
        if (pTempSession != pSession)
        {
            m_pSessionManager->RemoveFromSession(this, pTempSession);
        }
    }

    HX_ASSERT(!m_pSession);
    HX_ASSERT(!m_pSocket);
    m_pSession  = pSession;
    m_pSocket   = pSocket;
    m_cloakPort = pSession->m_pConnectAddr->GetPort();

    if (!m_bInitMsgSent)
    {
	rc = sendInitialMessage(m_pSession, m_pSocket);
    }

    if (!m_bInitDone)
    {
        m_pResp->InitDone(rc);
    }

    m_pMutex->Unlock();
}

void
RTSPClientProtocol::SessionFailed(RTSPClientSession* pSession,
                                  IHXSocket* pSocket)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SessionFailed(): [%p]", this, pSession);

    LISTPOSITION lPos = NULL;

    m_pMutex->Lock();

    lPos = m_sessionList.Find((void*)pSession);
    if (lPos)
    {
        m_sessionList.RemoveAt(lPos);
    }

    if (m_sessionList.IsEmpty())
    {
        m_pResp->InitDone(HXR_NET_CONNECT);
    }

    m_pMutex->Unlock();

    return;
}

HX_RESULT
RTSPClientProtocol::InitDone(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitDone(): %08x",this, status);
    HX_RESULT rc = HXR_OK;

    AddRef();
    m_pMutex->Lock();
    if (!m_bInitDone &&
        (m_sessionList.IsEmpty() || m_bSessionSucceeded || (HXR_OK == status)) &&
        m_pResp)
    {
        m_bInitDone = TRUE;
        rc = m_pResp->InitDone(status);
    }
    m_pMutex->Unlock();
    Release();
    return rc;
}

void
RTSPClientProtocol::AutoBWDetectionDone(HX_RESULT status, UINT32 ulBW)
{
    // save estimated BW(in Kbps) from dynamic ABD to Source's stats
    if (HXR_OK == status && m_pRegistry && m_ulRegistryID)
    {
        HX_ASSERT(ulBW);

        char        szRegName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
        UINT32      ulRegId = 0;
        IHXBuffer*  pParentName = NULL;

        if (HXR_OK == m_pRegistry->GetPropName(m_ulRegistryID, pParentName))
        {
            SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.EstimatedBandwidth", pParentName->GetBuffer());

            ulRegId = m_pRegistry->GetId(szRegName);
            if (!ulRegId)
            {
                m_pRegistry->AddInt(szRegName, (INT32)(ulBW * 1000));
            }
            else
            {
                m_pRegistry->SetIntByName(szRegName, (INT32)(ulBW * 1000));
            }
        }
        HX_RELEASE(pParentName);
    }

    // send the rest of RTSP requests if any
    canSendRemainingSetupRequests((HXR_TIMEOUT != status) ? HXR_OK : HXR_BAD_TRANSPORT);
}

STDMETHODIMP
RTSPClientProtocol::InitSockets()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitSockets()", this);

    HX_RESULT               hr = HXR_OK;
    UINT32                  nMaxUDPPort = MAX_UDP_PORT;
    IHXBuffer*              pBuffer = 0;
    RTSPStreamInfo*         pStreamInfo = NULL;
    HXBOOL                    bGotSocket = FALSE;
    HXBOOL                    bUseUDPPort = FALSE;
    UINT16                  datagramPort = 0;
    UDP_PORTS*              pUDPPort = NULL;
    CHXSimpleList*          pUDPPortList = new CHXSimpleList();
    CHXSimpleList::Iterator i;

    AddRef();

    if (!m_pNetSvc)
    {
        hr = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    m_pUDPSocketStreamMap = new CHXMapLongToObj;
    m_pRTCPSocketStreamMap = new CHXMapLongToObj;

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    if (m_bSDPInitiated && m_bMulticast)
    {
        for(i=m_streamInfoList.Begin();i!=m_streamInfoList.End() && HXR_OK == hr;++i)
        {
            pStreamInfo = (RTSPStreamInfo*)(*i);
            hr = CreateUDPSockets(pStreamInfo->m_streamNumber, pStreamInfo->m_sPort);
        }
    }
    else
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */
    {
        /////////////////////////////////////////////////////////////
        //
        // Handle Specific UDP Port Preferences here....
        //
        ReadPrefBOOL(m_pPreferences, "UseUDPPort", bUseUDPPort);
        if(!bUseUDPPort)
        {
            // If the MaxUDPPort Preference is set, use that instead of our defined limit
            if (HXR_OK == ReadPrefUINT32(m_pPreferences, "MaxUDPPort", nMaxUDPPort))
            {
                if(nMaxUDPPort < MIN_UDP_PORT)
                {
                    nMaxUDPPort = MAX_UDP_PORT;
                }
            }


            pUDPPort = new UDP_PORTS;
            pUDPPort->uFrom = MIN_UDP_PORT;
            pUDPPort->uTo = (UINT16)nMaxUDPPort;

            pUDPPortList->AddTail((void*)pUDPPort);
        }
        else
        {
            if(m_pPreferences->ReadPref("UDPPort", pBuffer) == HXR_OK)
            {
                HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitSockets(): port range(s) = %s", this, pBuffer->GetBuffer());

                ReadUDPPorts(pBuffer, pUDPPortList);
            }
        }
        HX_RELEASE(pBuffer);

        for(i=m_streamInfoList.Begin();i!=m_streamInfoList.End();++i)
        {
            pStreamInfo = (RTSPStreamInfo*)(*i);

            bGotSocket = FALSE;

            CHXSimpleList::Iterator lIterator = pUDPPortList->Begin();
            for (; lIterator != pUDPPortList->End(); ++lIterator)
            {
                pUDPPort = (UDP_PORTS*) (*lIterator);
                HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitSockets(): using %lu to %lu ", this, pUDPPort->uFrom, pUDPPort->uTo);

                if ((pUDPPort->uTo - pUDPPort->uFrom + 1) < 2)
                {
                    continue;
                }

                for (datagramPort = pUDPPort->uFrom; datagramPort <= pUDPPort->uTo; datagramPort += 2)
                {

                    if (datagramPort % 2)
                    {
                        datagramPort = (UINT16)(datagramPort + 1);
                    }

                    if ((pUDPPort->uTo - datagramPort + 1) < 2)
                    {
                        break;
                    }
                    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitSockets(): trying %lu...", this, datagramPort);
                    if (HXR_OK == CreateUDPSockets(pStreamInfo->m_streamNumber, datagramPort))
                    {
                        bGotSocket = TRUE;
                        break;
                    }

                }

                if (bGotSocket)
                {
                    break;
                }

            }

            // bail out if we failed to create UDP sockets, HXR_NET_UDP is returned
            // so we will attempt to switch to TCP if it's enabled
            if (!bGotSocket)
            {
                HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::InitSockets(): HXR_NET_UDP", this);
                hr = HXR_NET_UDP;
            }
        }

        m_currentTransport = UDPMode;
    }

  cleanup:

    if (HXR_OK != hr)
    {
        HX_DELETE(m_pUDPSocketStreamMap);
        HX_DELETE(m_pRTCPSocketStreamMap);
    }

    while (pUDPPortList->GetCount())
    {
        pUDPPort = (UDP_PORTS*)pUDPPortList->RemoveHead();
        HX_DELETE(pUDPPort);
    }
    HX_DELETE(pUDPPortList);

    Release();
    return hr;
}

STDMETHODIMP
RTSPClientProtocol::GetCurrentBuffering(UINT16 uStreamNumber,
                                        REF(UINT32) ulLowestTimestamp,
                                        REF(UINT32) ulHighestTimestamp,
                                        REF(UINT32) ulNumBytes,
                                        REF(HXBOOL) bDone)
{
    ulLowestTimestamp   = 0;
    ulHighestTimestamp  = 0;
    ulNumBytes          = 0;
    bDone               = FALSE;

    HX_ASSERT(m_pTransportStreamMap);

    if (!m_pTransportStreamMap)
    {
        return HXR_OK;
    }

    HX_RESULT rc = HXR_OK;
    m_pMutex->Lock();
    RTSPTransport* pTrans = GetTransport(uStreamNumber);

    //HX_ASSERT(pTrans);
    rc = pTrans ?
        pTrans->GetCurrentBuffering(uStreamNumber,
                                    ulLowestTimestamp,
                                    ulHighestTimestamp,
                                    ulNumBytes,
                                    bDone) : HXR_OK;
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SeekFlush()
{
    HX_RESULT rc = HXR_OK;
    m_pMutex->Lock();

    m_ulLastBWSent = MAX_UINT32;
    if (m_bIsLive)
    {
        CHXMapLongToObj::Iterator i;
        for(i=m_pTransportStreamMap->Begin();
            (rc == HXR_OK) &&  i!=m_pTransportStreamMap->End(); ++i)
        {
            RTSPTransport* pTransport = (RTSPTransport*)(*i);
            UINT16 streamNumber = (UINT16)i.get_key();

            HX_ASSERT(pTransport);

            rc = pTransport ? pTransport->SeekFlush(streamNumber) : HXR_OK;
        }
    }

    if (m_pSrcBufStats)
    {
        m_pSrcBufStats->Reset();
    }

    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP_(HXBOOL)
    RTSPClientProtocol::IsDataReceived(void)
{
    m_pMutex->Lock();
    HXBOOL bReceived = FALSE;
    CHXMapLongToObj::Iterator i = m_pTransportStreamMap->Begin();
    if(i != m_pTransportStreamMap->End())
    {

        RTSPTransport* pTrans = (RTSPTransport*)(*i);
        HX_ASSERT(pTrans);
        bReceived = pTrans ? pTrans->IsDataReceived() : FALSE;
    }

    m_pMutex->Unlock();
    return bReceived;
}

STDMETHODIMP_(HXBOOL)
    RTSPClientProtocol::IsSourceDone(void)
{
    m_pMutex->Lock();
    HXBOOL bDone = FALSE;
    CHXMapLongToObj::Iterator i = m_pTransportStreamMap->Begin();
    if(i != m_pTransportStreamMap->End())
    {
        RTSPTransport* pTrans = (RTSPTransport*)(*i);
        HX_ASSERT(pTrans);
        bDone = pTrans ? pTrans->IsSourceDone() : FALSE;
    }

    m_pMutex->Unlock();
    return bDone;
}

STDMETHODIMP
RTSPClientProtocol::RuleChange(CHXSimpleList* pSubList)
{
    if (!m_pIsMethodSupported[SET_PARAM] || !m_pSession)
    {
        return HXR_OK;
    }

    m_pMutex->Lock();

    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
    pMsg->setURL(getAggControlURL());

    CHXString SubString;
    CHXString UnSubString;

    CHXSimpleList::Iterator i;
    HXBOOL bFirstSub = TRUE;
    HXBOOL bFirstUnSub = TRUE;
    for(i=pSubList->Begin(); i!=pSubList->End(); ++i)
    {
        char tmp[64];
        RTSPSubscription* pSub = (RTSPSubscription*)(*i);

        SafeSprintf(tmp, 64, "stream=%d;rule=%ld", pSub->m_streamNumber,
                    pSub->m_ruleNumber);

        if (pSub->m_bIsSubscribe)
        {
            if(!bFirstSub)
            {
                SubString += "," + CHXString(tmp);
            }
            else
            {
                SubString += tmp;
                bFirstSub = FALSE;
            }
        }
        else
        {
            if(!bFirstUnSub)
            {
                UnSubString += "," + CHXString(tmp);
            }
            else
            {
                UnSubString += tmp;
                bFirstUnSub = FALSE;
            }
        }
    }
    if (!bFirstSub)
    {
        pMsg->addHeader("Subscribe", (const char*)SubString);
    }
    if (!bFirstUnSub)
    {
        pMsg->addHeader("UnSubscribe", (const char*)UnSubString);
    }
    if (!m_sessionID.IsEmpty())
    {
        pMsg->addHeader("Session", m_sessionID);
    }

    UINT32 seqNo = m_pSession->getNextSeqNo(this);
    HX_RESULT rc = sendRequest(pMsg, seqNo);
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::Subscribe(CHXSimpleList* pSubList)
{
    return RuleChange(pSubList);
}

STDMETHODIMP
RTSPClientProtocol::Unsubscribe(CHXSimpleList* pUnsubList)
{
    return RuleChange(pUnsubList);
}

STDMETHODIMP
RTSPClientProtocol::BackChannelPacketReady(IHXPacket* pPacket)
{
    if (!m_pIsMethodSupported[SET_PARAM])
    {
        return HXR_OK;
    }
    m_pMutex->Lock();

    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
    IHXBuffer* pBuffer = pPacket->GetBuffer();

    pMsg->setURL(getAggControlURL());

    char* pEncodedBuffer =
        new char[pBuffer->GetSize() * 2 + 4]; // XXXSMP Overkill

    BinTo64(pBuffer->GetBuffer(), (INT32)pBuffer->GetSize(), pEncodedBuffer);
    int lenTmpBuf = (int)(strlen(pEncodedBuffer) + 12);
    char* tmpBuf = new char[lenTmpBuf];
    SafeSprintf(tmpBuf, (UINT32)lenTmpBuf, "\"%s\"", pEncodedBuffer);
    pMsg->addHeader("BackChannel", tmpBuf);
    SafeSprintf(tmpBuf, (UINT32)lenTmpBuf, "%d", pPacket->GetStreamNumber());
    pMsg->addHeader("StreamNumber", tmpBuf);
    if (!m_sessionID.IsEmpty())
    {
        pMsg->addHeader("Session", m_sessionID);
    }
    delete[] tmpBuf;

    UINT32 seqNo = m_pSession->getNextSeqNo(this);

    pBuffer->Release();
    delete[] pEncodedBuffer;

    HX_RESULT rc = sendRequest(pMsg, seqNo);
    m_pMutex->Unlock();
    return rc;
}

STDMETHODIMP
RTSPClientProtocol::SendRTTRequest()
{
    return DoSendRTTRequest();
}

STDMETHODIMP
RTSPClientProtocol::SendBWReport(INT32 aveBandwidth,
                                 INT32 packetLoss,
                                 INT32 bandwidthWanted)
{
    return DoSendBWReport(aveBandwidth, packetLoss, bandwidthWanted);
}

HX_RESULT
RTSPClientProtocol::DoSendRTTRequest(void)
{
    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::DoSendBWReport(INT32 aveBandwidth,
                                   INT32 packetLoss,
                                   INT32 bandwidthWanted)
{
    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::sendRequest(RTSPRequestMessage* pMsg,
                                UINT32 seqNo)
{
    DEBUG_OUT(m_pErrMsg, DOL_RTSP, (s,"(%u, %p) RTSPReq %u %u",
                                    HX_GET_TICKCOUNT(), this, seqNo, pMsg->tag()));
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendRequest(): %s request; seq = %lu", this, pMsg->tagStr(), seqNo);

    // Our legacy timeout approach was to periodically send messages
    // to the server.  Currently, we only send a keep alive message
    // if we have not sent an rtsp message for the duration
    // of the timeout value.
    if (m_pSessionTimeout && !m_bUseLegacyTimeOutMsg)
    {
        m_pSessionTimeout->OnActivity();
    }

    return RTSPBaseProtocol::sendRequest(pMsg, seqNo);
}

HX_RESULT
RTSPClientProtocol::sendRequest(RTSPRequestMessage* pMsg,
                                const char* pContent,
                                const char* pMimeType,
                                UINT32 seqNo)
{
    DEBUG_OUT(m_pErrMsg, DOL_RTSP, (s,"(%u, %p) RTSPReq %u %u",
                                    HX_GET_TICKCOUNT(), this, seqNo, pMsg->tag()));

    if (m_pSessionTimeout && !m_bUseLegacyTimeOutMsg)
    {
        m_pSessionTimeout->OnActivity();
    }

    return RTSPBaseProtocol::sendRequest(pMsg, pContent, pMimeType, seqNo);
}

HX_RESULT
RTSPClientProtocol::ReportSuccessfulTransport(void)
{
    HX_RESULT   rc = HXR_OK;

    if (!m_bReportedSuccessfulTransport)
    {
        m_bReportedSuccessfulTransport = TRUE;

        IHXPreferredTransportSink* pPreferredTransportSink = NULL;
        if (m_pResp &&
            HXR_OK == m_pResp->QueryInterface(IID_IHXPreferredTransportSink,
                                              (void**)&pPreferredTransportSink))
        {
            pPreferredTransportSink->TransportSucceeded(m_currentTransport, m_cloakPort);
        }
        HX_RELEASE(pPreferredTransportSink);
    }

    return rc;
}

HX_RESULT
RTSPClientProtocol::canSendRemainingSetupRequests(HX_RESULT status)
{
    HX_RESULT   rc = status;
    UINT16      nStreamCount = (UINT16)m_streamInfoList.GetCount();

    if (nStreamCount)
    {
        // report error condition immediately
        if (HXR_OK != status)
        {
            return m_pResp->HandleSetupResponse(status);
        }

        // OnAutoBWDetectionDone needs to be called before we can
        // send the subsequnt SETUP request or call HandleSetupResponse()
        if (ABD_STATE_REQUEST != m_pSession->getABDState())
        {
            // all the streams have been setup successfully
            if (m_setupResponseCount == nStreamCount)
            {
                // all done!
                CHXSimpleList::Iterator i;

                for(i=m_transportRequestList.Begin();i!=m_transportRequestList.End();++i)
                {
                    RTSPTransportRequest* pRequest = (RTSPTransportRequest*)(*i);
                    RTSPTransportInfo* pTransInfo = pRequest->getFirstTransportInfo();

                    while(pTransInfo)
                    {
                        if (BUFFER_DEPTH_UNDEFINED == m_ulBufferDepth)
                        {
                            break;
                        }

                        status = pTransInfo->m_pTransport->SetResendBufferDepth
                            (
                                m_ulBufferDepth
                                );

                        if (HXR_OK != status)
                        {
                            break;
                        }

                        pTransInfo = pRequest->getNextTransportInfo();
                    }
                }

                // we don't need this anymore...
                HX_RELEASE(m_pSetupRequestHeader);

                // Now that we have received all the SETUP responses,
                // then we can clean up the queues which held any UDP
                // packets received before we received the SETUP response.
                // Each queue is cleaned up when it is flushed
                // to the transport in FlushPreSetupResponsePacketsToTransport(),
                // but if some intruder was sending packets on a random port,
                // then those packets would still be held now. Since we now
                // know all the ports for our transport streams, then we can
                // clean up any remaining queues.
                ClearPreSetupResponseQueueMap();

                // Hold onto the HandleSetupResponse() call if we are waiting for the
                // bw info since subsequent RTSP requests(i.e. subscribe via SET_PARAM)
                // rely on such info.
                //
                // HandleSetupResponse() will be called in RTSPClientProtocol::NewConnectionBW()
                // after we have the bw info
                if (ABD_STATE_WAITING != m_pSession->getABDState())
                {
                    // we'd better done with ABD before calling HandleSetupResponse()
                    HX_ASSERT((ABD_STATE_DONE == m_pSession->getABDState()) ||
                              (ABD_STATE_DISABLED == m_pSession->getABDState()));

                    // notify the response object the status after we received
                    // all the SETUP responses
                    rc = m_pResp->HandleSetupResponse(status);
                }
            }
            else if (1 == m_setupResponseCount)
            {
                // send the remaining SETUP requests
                sendRemainingSetupRequests();

                if (PipelineRTSP())
                {
                    if (ABD_STATE_WAITING != m_pSession->getABDState())
                    {
                        // we'd better done with ABD before calling HandleSetupResponse()
                        HX_ASSERT((ABD_STATE_DONE == m_pSession->getABDState()) ||
                                  (ABD_STATE_DISABLED == m_pSession->getABDState()));

                        // pass HXR_WOULD_BLOCK to notify the response object to
                        // proceed & pipeline the rest of RTSP requests even though we haven't
                        // received all the SETUP responses
                        rc = m_pResp->HandleSetupResponse(HXR_WOULD_BLOCK);
                    }
                }
            }
        }
    }

    return rc;
}

HXBOOL
RTSPClientProtocol::IsRealServer(void)
{
    return FALSE;
}

//
// IsRealDataType: Checks the SDP parameter for attribute
//                 a:IsRealDataType
//                 
HXBOOL 
RTSPClientProtocol::IsRealDataType(void)
{
    UINT32  ulIsRealDataType = 0;

    if (m_pSDPFileHeader)
    {
        m_pSDPFileHeader->GetPropertyULONG32("IsRealDataType", ulIsRealDataType);
    }

    return (ulIsRealDataType ? TRUE : FALSE);
}


STDMETHODIMP
RTSPClientProtocol::SetFirstSeqNum(UINT16 uStreamNumber, UINT16 uSeqNum)
{
    m_pMutex->Lock();
    RTSPTransport* pTrans = GetTransport(uStreamNumber);

    HX_ASSERT(pTrans);
    if(pTrans)
    {
        pTrans->setFirstSeqNum(uStreamNumber, uSeqNum);
    }

    m_pMutex->Unlock();

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::SetRTPInfo(UINT16 uStreamNumber, UINT16 uSeqNum,
                               UINT32 ulRTPTime, RTPInfoEnum info)
{
    return _SetRTPInfo(uStreamNumber, uSeqNum, ulRTPTime, info);
}

HX_RESULT
RTSPClientProtocol::_SetRTPInfo(UINT16 uStreamNumber, UINT16 uSeqNum,
                                UINT32 ulRTPTime, RTPInfoEnum info,
			        HXBOOL bOnPauseResume)
{
    m_pMutex->Lock();
    HX_ASSERT(RTPINFO_ERROR != info);

    RTSPTransport* pTrans = GetTransport(uStreamNumber);

    HX_ASSERT(pTrans);
    if(pTrans)
    {
        /*
         *  RTPTransport needs to know exactly what's in RTP-Info
         */
        if (RTPINFO_SEQ_RTPTIME == info)
        {
            pTrans->setFirstSeqNum(uStreamNumber, uSeqNum, bOnPauseResume);
            pTrans->setFirstTimeStamp(uStreamNumber, ulRTPTime, FALSE, bOnPauseResume);
        }
        else if (RTPINFO_SEQ == info)
        {
            pTrans->setFirstSeqNum(uStreamNumber, uSeqNum, bOnPauseResume);
        }
        else if (RTPINFO_RTPTIME == info)
        {
            pTrans->setFirstTimeStamp(uStreamNumber, ulRTPTime, FALSE, bOnPauseResume);
        }
    }

    m_pMutex->Unlock();

    return HXR_OK;
}

/*HX_RESULT RTSPClientProtocol::HandleWriteDataEvent(IHXSockAddr* pLocalDest)
  {
  HX_ASSERT(pLocalDest);
  UINT16 port = HXSockUtil::AddrGetPort(pLocalDest);

  // look up transport associated with port
  RTSPTransport* pTrans = (RTSPTransport*)(*m_pTransportPortMap)[port];
  if (!pTrans)
  {
  pTrans = (RTSPTransport*)(*m_pTransportMPortMap)[port];
  }

  HX_RESULT hr = HXR_FAIL;
  if(pTrans)
  {
  hr = pTrans->HandleWriteDataEvent();
  }
  return hr;
  }*/

void
RTSPClientProtocol::NotifyStreamsRTPInfoProcessed(HXBOOL bOnPauseResume)
{
    m_pMutex->Lock();

    if (m_pControlToStreamNoMap)
    {
        CHXMapStringToOb::Iterator i;
        UINT32* pul;
        RTSPTransport* pTrans;
        UINT16 streamID;

        for (i = m_pControlToStreamNoMap->Begin();
             i != m_pControlToStreamNoMap->End();
             ++i)
        {
            pul = (UINT32*)(*i);
            streamID = (UINT16) (*pul);
            pTrans = GetTransport(streamID);
            if (pTrans)
            {
                pTrans->notifyRTPInfoProcessed(bOnPauseResume);
            }
        }
    }

    m_pMutex->Unlock();
}

HX_RESULT
RTSPClientProtocol::ReadFromDone(HX_RESULT status, IHXBuffer* pBuffer,
                                 IHXSockAddr* pSource, IHXSockAddr* pDest /*local*/)
{
    HX_RESULT hresult = HXR_OK;
    HXBOOL bMCastPort = FALSE;
    RTSPTransport* pTrans = NULL;

    /*
     * XXX HP: While handling the m_pData->done it's possible for the
     *         DispatchMessage call in CancelSelect to cause an
     *         asynchronous DoRead to occur. m_pTransportPortMap has
     *         been deleted inside Done() and we should add checkpoint
     *         here!!
     */
    if (m_bClientDone)
    {
        return hresult;
    }

    m_pMutex->Lock();

    if(status == HXR_OK)
    {
        UINT16 nToPort = pDest->GetPort();

        //XXXBAB - need to get transport by port
        pTrans = (RTSPTransport*)(*m_pTransportPortMap)[nToPort];
        if (!pTrans)
        {
            // data received on multicast port
            pTrans = (RTSPTransport*)(*m_pTransportMPortMap)[nToPort];
            bMCastPort = TRUE;
            m_currentTransport = MulticastMode;
        }

        if (pTrans)
        {
            // make sure the unicast packets received are coming from the same server
            // we are connecting to
            if ((pSource->IsEqualAddr(m_pConnectAddr)) || bMCastPort)
            {
                ReportSuccessfulTransport();

                // XXX HP we use PacketReady() to indicate the liveness of UDP connection
                // for server timeout detection
                PacketReady(HXR_OK, m_sessionID, NULL);

                // drop all the scalable multicast packets when we are paused
                if ((MulticastMode != m_currentTransport) || !m_bSDPInitiated || !m_bPaused)
                {
                    hresult = pTrans->handlePacket(pBuffer);
                    if (m_bSplitterConsumer)
                    {
                        pTrans->releasePackets();
                    }
                }
            }
        }
        else
        {
            // Have we received all the SETUP responses yet?
            // If we have not received all the responses, then we
            // should queue these UDP packets. If we have received
            // all the setup responses, then transports should be mapped
            // into either m_pTransportPortMap or m_pTransportMPortMap,
            // so we don't need to queue these packets anymore.
            UINT16 usStreamCount = (UINT16) m_streamInfoList.GetCount();
            if (m_setupResponseCount < usStreamCount)
            {
                // We have not received all the SETUP responses yet.
                //
                // We don't have a transport mapped yet. This could happen
                // because we have already started to receive UDP packets
                // before we have received the SETUP response on the
                // TCP control channel. We will log this event and
                // then add the packet to a queue. Later, when and if
                // we receive the SETUP response, we will check to
                // see if any packets have been queued.
                HXLOGL2(HXLOG_TRAN, "UDP packet received on unmapped port %u - QUEUEING", nToPort);
                // Add the packet to a queue
                AddPacketToPreSetupResponseQueue(nToPort, pBuffer);
            }
            else
            {
                // We received a packet on a UDP port that we don't
                // have mapped into either m_pTransportPortMap or m_pTransportMPortMap
                // and all the SETUP responses have been received. We will
                // drop this packet. Log this event, however.
                HXLOGL3(HXLOG_TRAN, "UDP packet received on unmapped port %u - DROPPING", nToPort);
            }
        }
    }
    else
    {
        hresult = PacketReady(HXR_FAIL, m_sessionID, 0);
    }

    m_pMutex->Unlock();
    return hresult;
}

STDMETHODIMP
RTSPClientProtocol::SetConnectionTimeout(UINT32 uSeconds)
{
    m_uConnectionTimeout = uSeconds;

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::SetResendBufferDepth(UINT32 uSeconds)
{
    m_ulBufferDepth = uSeconds * 1000;

    CHXMapLongToObj::Iterator i;
    for(i=m_pTransportStreamMap->Begin(); i!=m_pTransportStreamMap->End(); ++i)
    {
        RTSPTransport* pTransport = (RTSPTransport*)(*i);
        HX_ASSERT(pTransport);
        HX_RESULT hresult = pTransport->SetResendBufferDepth(m_ulBufferDepth);
        if (HXR_OK != hresult)
        {
            return hresult;
        }
    }

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::SetResendBufferParameters(THIS_
                                              UINT32 uMinimumDelay,  /* ms */
                                              UINT32 uMaximumDelay,  /* ms */
                                              UINT32 uExtraBufferingDelay /* ms */)
{
    CHXMapLongToObj::Iterator i;
    for(i=m_pTransportStreamMap->Begin(); i!=m_pTransportStreamMap->End(); ++i)
    {

        RTSPTransport* pTransport = (RTSPTransport*)(*i);

        HX_ASSERT(pTransport);
        HX_RESULT hresult = pTransport->SetResendBufferParameters(uMinimumDelay,
                                                                  uMaximumDelay,
                                                                  uExtraBufferingDelay);

        if (HXR_OK != hresult)
        {
            return hresult;
        }
    }
    return HXR_OK;
}

/*
 * IHXTransportSyncServer methods
 */
STDMETHODIMP
RTSPClientProtocol::DistributeSyncAnchor(ULONG32 ulHXTime,
                                         ULONG32 ulNTPTime)
{
    m_pMutex->Lock();

    if (!m_transportRequestList.IsEmpty())
    {
        RTSPTransportRequest* pRequest =
            (RTSPTransportRequest*)m_transportRequestList.GetHead();
        RTSPTransportInfo* pTransInfo = pRequest->getFirstTransportInfo();
        while(pTransInfo)
        {
            pTransInfo->m_pTransport->anchorSync(ulHXTime, ulNTPTime);
            pTransInfo = pRequest->getNextTransportInfo();
        }
    }

    m_pMutex->Unlock();

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::DistributeSync(ULONG32 ulHXTime,
                                   LONG32 lHXTimeOffset)
{
    m_pMutex->Lock();

    if (!m_transportRequestList.IsEmpty())
    {
        RTSPTransportRequest* pRequest =
            (RTSPTransportRequest*)m_transportRequestList.GetHead();
        RTSPTransportInfo* pTransInfo = pRequest->getFirstTransportInfo();
        while(pTransInfo)
        {
            pTransInfo->m_pTransport->handleMasterSync(ulHXTime,
                                                       lHXTimeOffset);
            pTransInfo = pRequest->getNextTransportInfo();
        }
    }

    m_pMutex->Unlock();

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::DistributeStartTime(ULONG32 ulHXRefTime)
{
    return HXR_NOTIMPL;
}

/*
 * IHXTransportBufferLimit methods
 */

/************************************************************************
 *      Method:
 *          IHXTransportBufferLimit::SetByteLimit
 *      Purpose:
 *      Sets the maximum number of bytes that can be buffered in the
 *      transport buffer. If incomming packets would put us over this
 *      limit, then they are replaced with lost packets. A byte limit
 *      of 0 means unlimited buffering.
 */
STDMETHODIMP
RTSPClientProtocol::SetByteLimit(UINT16 uStreamNumber, UINT32 uByteLimit)
{
    HX_RESULT res = HXR_FAILED;
    RTSPStreamInfo* pStreamInfo = NULL;

    m_pMutex->Lock();

    RTSPTransportBuffer* pTransBuf = getTransportBuffer(uStreamNumber);
    if (pTransBuf)
    {
        pTransBuf->SetByteLimit(uByteLimit);
        res = HXR_OK;
    }
    else
    {
        pStreamInfo = getStreamInfo(uStreamNumber);
        HX_ASSERT(pStreamInfo);

        if(pStreamInfo)
        {
            pStreamInfo->m_uByteLimit = uByteLimit;
            res = HXR_OK;
        }
    }

    m_pMutex->Unlock();

    return res;
}

/************************************************************************
 *      Method:
 *          IHXTransportBufferLimit::GetByteLimit
 *      Purpose:
 *      Returns the current byte limit in effect. A value of 0 means
 *      unlimited buffering is allowed
 */
STDMETHODIMP_(UINT32)
    RTSPClientProtocol::GetByteLimit(UINT16 uStreamNumber)
{
    UINT32 ulRet = 0;

    m_pMutex->Lock();

    RTSPTransportBuffer* pTransBuf = getTransportBuffer(uStreamNumber);
    HX_ASSERT(pTransBuf);
    if (pTransBuf)
    {
        ulRet = pTransBuf->GetByteLimit();
    }

    m_pMutex->Unlock();

    return ulRet;
}

/*
 * private RTSPClientProtocol methods
 */

HX_RESULT
RTSPClientProtocol::handleMessage(RTSPMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleMessage()", this);
    HX_RESULT rc = HXR_OK;

    m_pMutex->Lock();

    handleDebug((const char*)pMsg->asString(), TRUE);

    if(pMsg->tag() != RTSPMessage::T_RESP)
    {
        int majorVersion = pMsg->majorVersion();
        int minorVersion = pMsg->minorVersion();
        if((majorVersion == 0 && minorVersion == 0) ||
           (majorVersion > RTSPMessage::MAJ_VERSION))
        {
            rc = HandleBadVersion(pMsg);
            goto exit;
        }
        else if(minorVersion > RTSPMessage::MIN_VERSION)
        {
            rc = HandleBadVersion(pMsg);
            goto exit;
        }
    }

    // XXX HP we use PacketReady() to indicate the liveness of TCP connection
    // for server timeout detection
    if (TCPMode == m_currentTransport ||
        HTTPCloakMode == m_currentTransport)
    {
        PacketReady(HXR_OK, m_sessionID, NULL);
    }

    switch(pMsg->tag())
    {
       case RTSPMessage::T_OPTIONS:
       {
           rc = HandleOptions((RTSPOptionsMessage*)pMsg);
           goto exit;
       }

       case RTSPMessage::T_SET_PARAM:
       {
           rc = HandleSetParam((RTSPSetParamMessage*)pMsg);
           goto exit;
       }

       case RTSPMessage::T_REDIRECT:
       {
           rc = HandleRedirect((RTSPRedirectMessage*)pMsg);
           goto exit;
       }

       case RTSPMessage::T_RESP:
       {
           DEBUG_OUT(m_pErrMsg, DOL_RTSP,
                     (s,"(%u, %p) RTSPResp %u %u",
                      HX_GET_TICKCOUNT(), this, pMsg->seqNo(),
                      ((RTSPResponseMessage*)pMsg)->errorCodeAsUINT32()));

           UINT32 uErrorCode = ((RTSPResponseMessage*)pMsg)->errorCodeAsUINT32();

#if defined(HELIX_FEATURE_RTSP_RESP_SINK)
           //
           //  Log all responses with status codes of 3xx, 4xx, and 5xx
           //
           if ( uErrorCode >= 300 )
           {
               LogRTSPResponseMessage((RTSPResponseMessage*)pMsg);
           }
#endif

           // check for proxy(305) redirect
           if (uErrorCode == 305)
           {
               rc = HandleUseProxy((RTSPResponseMessage*)pMsg);
               goto exit;
           }

           // check for URL redirect
           else if (uErrorCode == 301 || uErrorCode == 302 || uErrorCode == 303)
           {
               rc = HandleRedirectResponse((RTSPResponseMessage*)pMsg);
               goto exit;
           }

           RTSPMessage* pReqMsg = dequeueMessage(pMsg->seqNo());
           if(pReqMsg)
           {
               switch(pReqMsg->tag())
               {
                  case RTSPMessage::T_OPTIONS:
                  {
                      rc = handleOptionsResponse((RTSPResponseMessage*)pMsg, 
                                                 (RTSPOptionsMessage*)pReqMsg);
                  }
                  break;

                  case RTSPMessage::T_GET_PARAM:
                  {
                      rc = handleGetParamResponse((RTSPResponseMessage*)pMsg);
                  }
                  break;

                  case RTSPMessage::T_SET_PARAM:
                  {
                      rc = handleSetParamResponse((RTSPResponseMessage*)pMsg);
                  }
                  break;

                  case RTSPMessage::T_TEARDOWN:
                  {
                      rc = handleTeardownResponse((RTSPResponseMessage*)pMsg);
                      m_state = RTSPClientProtocol::INIT;
                  }
                  break;

                  case RTSPMessage::T_DESCRIBE:
                  {
                      rc = handleDescribeResponse((RTSPResponseMessage*)pMsg);
                  }
                  break;

                  case RTSPMessage::T_ANNOUNCE:
                  {
                      rc = handleAnnounceResponse((RTSPResponseMessage*)pMsg);
                  }
                  break;

                  default:
                  {
                      switch(m_state)
                      {
                         case RTSPClientProtocol::INIT:
                         {
                             switch(pReqMsg->tag())
                             {
                                case RTSPMessage::T_SETUP:
                                {
                                    rc = handleSetupResponse(
                                        (RTSPResponseMessage*)pMsg,
                                        (RTSPSetupMessage*)pReqMsg);
                                    if(rc == HXR_OK)
                                    {
                                        m_state = RTSPClientProtocol::READY;
                                    }
                                }
                                break;

                                default:
                                {
                                    rc = HandleUnexpected(pMsg);
                                }
                                break;
                             }
                         }
                         break;

                         case RTSPClientProtocol::READY:
                         {
                             switch(pReqMsg->tag())
                             {
                                case RTSPMessage::T_SETUP:
                                {
                                    rc = handleSetupResponse(
                                        (RTSPResponseMessage*)pMsg,
                                        (RTSPSetupMessage*)pReqMsg);
                                }
                                break;

                                case RTSPMessage::T_PLAY:
                                {
                                    rc = handlePlayResponse(
                                        (RTSPResponseMessage*)pMsg,
                                        (RTSPPlayMessage*)pReqMsg);
                                    if(rc == HXR_OK)
                                    {
                                        m_state =
                                            RTSPClientProtocol::PLAYING;
                                    }
                                }
                                break;

                                case RTSPMessage::T_RECORD:
                                {
                                    rc = handleRecordResponse(
                                        (RTSPResponseMessage*)pMsg);
                                    if(rc == HXR_OK)
                                    {
                                        m_state =
                                            RTSPClientProtocol::RECORDING;
                                    }
                                }
                                break;

                                default:
                                {
                                    rc = HandleUnexpected(pMsg);
                                }
                                break;
                             }
                         }
                         break;

                         case RTSPClientProtocol::PLAYING:
                         {
                             switch(pReqMsg->tag())
                             {
                                case RTSPMessage::T_PLAY:
                                {
                                    rc = handlePlayResponse(
                                        (RTSPResponseMessage*)pMsg,
                                        (RTSPPlayMessage*)pReqMsg);
                                }
                                break;

                                case RTSPMessage::T_PAUSE:
                                {
                                    rc = handlePauseResponse(
                                        (RTSPResponseMessage*)pMsg);
                                    if(rc == HXR_OK)
                                    {
                                        m_state =
                                            RTSPClientProtocol::READY;
                                    }
                                }
                                break;

                                default:
                                {
                                    rc = HandleUnexpected(pMsg);
                                }
                                break;
                             }
                         }
                         break;

                         case RTSPClientProtocol::RECORDING:
                         {
                             switch(pReqMsg->tag())
                             {
                                case RTSPMessage::T_RECORD:
                                {
                                    rc = handleRecordResponse(
                                        (RTSPResponseMessage*)pMsg);
                                }
                                break;

                                case RTSPMessage::T_PAUSE:
                                {
                                    rc = handlePauseResponse(
                                        (RTSPResponseMessage*)pMsg);
                                    if(rc == HXR_OK)
                                    {
                                        m_state =
                                            RTSPClientProtocol::READY;
                                    }
                                }
                                break;

                                default:
                                {
                                    rc = HandleUnexpected(pMsg);
                                }
                                break;
                             }
                         }
                         break;
                      }
                  }
                  break;
               }

               // Create and init out server timeout object
               if (!m_pKeepAliveCallback)
               {
                   m_pKeepAliveCallback = new RTSPClientProtocol::TimeoutCallback(this, RTSPCLIENT_TIMEOUT_KEEPALIVE);
                   m_pKeepAliveCallback->AddRef();
               }

               UINT32 nTimeOut = 0;

               // Set to server timeout value on creation
               if (!m_pSessionTimeout)
               {
                   m_pSessionTimeout = new CHXKeepAlive;
                   nTimeOut = m_ulServerTimeOut;
               }

               // Check for session timeout
               CHXString sessionID = pMsg->getHeaderValue("Session");
               if(sessionID != "")
               {
                   int i;
                   if (-1 != (i = sessionID.Find('=')))
                   {
                       // Wake up early for session timeout since servers will
                       // disconnect if they do not receive messages on time.
                       nTimeOut = (UINT32)atoi(sessionID.Right((INT32)(sessionID.GetLength()-(i+1))));

                       // Some servers specify timeout in ms not secs
                       if (nTimeOut < 1000)
                           nTimeOut *= 1000;

                       // If session timeout is present, use options message to alert the
                       // server we are still alive.  If not, use setparam.
                       m_bUseLegacyTimeOutMsg = FALSE;

                       // If session timeout value differnt than our current value, we
                       // need to reinit the scheduer.
                       if (nTimeOut == m_ulCurrentTimeOut)
                       {
                           nTimeOut = 0;
                       }
                       // Use the lower of our default timeout and the session timeout
                       else if (m_ulServerTimeOut < nTimeOut)
                       {
                           nTimeOut = m_ulServerTimeOut;
                       }
                   }
               }

               if (nTimeOut && m_pSessionTimeout)
               {
                   // we divided the timeout in half so that we are certain
                   // KeepAlive message will be sent within the timeout period
                   m_pSessionTimeout->Init(m_pScheduler,
                                           nTimeOut/2,
                                           (IHXCallback*)m_pKeepAliveCallback);

                   m_ulCurrentTimeOut = nTimeOut;
               }

               delete pReqMsg;
           }
       }
       break;

       default:
       {
           rc =  HandleUnexpected(pMsg);
           goto exit;
       }
    }

  exit:
    m_pMutex->Unlock();
    return rc;
}

const char*
RTSPClientProtocol::allowedMethods()
{
    return "OPTIONS";
}

HX_RESULT
RTSPClientProtocol::handleTCPData(BYTE* pData, UINT16 dataLen, UINT16 channel, UINT32 ulTimeStamp)
{
    if (!m_pTransportChannelMap || !m_pCommonClassFactory)
        return HXR_FAIL;

    m_pMutex->Lock();
    HX_RESULT rc = HXR_OK;

    IHXBuffer* pBuffer = NULL;

    if (ulTimeStamp)
    {
        IHXTimeStampedBuffer* pTSBuffer = NULL;
        if (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXTimeStampedBuffer, (void**)&pTSBuffer))
        {
            pTSBuffer->SetTimeStamp(ulTimeStamp);
            pTSBuffer->QueryInterface(IID_IHXBuffer, (void**)&pBuffer);
            HX_RELEASE(pTSBuffer);
        }
    }
    else
    {
        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
    }

    if (!pBuffer)
    {
        rc = HXR_OUTOFMEMORY;
        goto overandout;
    }

    rc = pBuffer->Set(pData, dataLen);
    if( rc == HXR_OUTOFMEMORY )
    {
        pBuffer->Release();
        goto overandout;
    }

    RTSPTransport* pTrans;
    if (m_pTransportChannelMap->Lookup(channel, (void*&)pTrans))
    {
        ReportSuccessfulTransport();
        rc = pTrans->handlePacket(pBuffer);
    }
#ifdef _DEBUG
    else
    {
        HX_ASSERT(!"make sure TransportChannelMap has been set up right...");
    }
#endif

  overandout:

    HX_RELEASE(pBuffer);
    m_pMutex->Unlock();
    return rc;
}

/*
 */
SdpFileType
RTSPClientProtocol::GetSdpFileTypeWeNeed(IHXValues* pHeaders)
{
    IHXBuffer* pAgent = NULL;
    SdpFileType sdpType = NONE_SDP;

    /*
     *  Better make sure to come up with a better way to check
     */
    if (FAILED(pHeaders->GetPropertyCString("Server", pAgent)))
    {
        return NONE_SDP;
    }

    if (strstr((const char*)pAgent->GetBuffer(), "RealMedia"))
    {
        sdpType = BACKWARD_COMP_SDP;
    }
    else
    {
        sdpType = INTEROP_SDP;
    }

    HX_RELEASE(pAgent);
    return sdpType;
}

HX_RESULT
RTSPClientProtocol::GetStreamDescriptionInfo(IUnknown* pUnknown, CHXString& mimeTypes)
{
    HX_RESULT               rc = HXR_OK;
    const char*             pMimeType = NULL;
    IHXStreamDescription*   pStreamDesc = NULL;

    if (HXR_OK == pUnknown->QueryInterface(IID_IHXStreamDescription,(void**)&pStreamDesc) &&
        pStreamDesc)
    {
        pStreamDesc->GetStreamDescriptionInfo(pMimeType);
        if(mimeTypes.IsEmpty())
        {
            mimeTypes += pMimeType;
        }
        else
        {
            mimeTypes += ", " + (CHXString)pMimeType;
        }
        pStreamDesc->Release();
    }

    return rc;
}

void
RTSPClientProtocol::SendMsgToTransport(TRANSPORT_MSG msg)
{
    RTSPTransportRequest*   pRequest = NULL;
    RTSPTransportInfo*      pTransInfo = NULL;

    if (!m_transportRequestList.IsEmpty())
    {
        pRequest = (RTSPTransportRequest*)m_transportRequestList.GetHead();
        pTransInfo = pRequest->getFirstTransportInfo();
        while(pTransInfo)
        {
            switch (msg)
            {
               case ENTER_PREFETCH:
                   pTransInfo->m_pTransport->EnterPrefetch();
                   break;
               case LEAVE_PREFETCH:
                   pTransInfo->m_pTransport->LeavePrefetch();
                   break;
               case ENTER_FASTSTART:
                   pTransInfo->m_pTransport->EnterFastStart();
                   break;
               case LEAVE_FASTSTART:
                   pTransInfo->m_pTransport->LeaveFastStart();
                   break;
               case PAUSE_BUFFER:
                   pTransInfo->m_pTransport->pauseBuffers();
                   break;
               case RESUME_BUFFER:
                   pTransInfo->m_pTransport->resumeBuffers();
                   break;
               default:
                   break;
            }
            pTransInfo = pRequest->getNextTransportInfo();
        }
    }
}

void
RTSPClientProtocol::AddCommonHeaderToMsg(RTSPRequestMessage* pMsg)
{
    if (pMsg)
    {
        pMsg->addHeader("User-Agent", m_versionString);
        if (!m_sessionID.IsEmpty())
        {
            pMsg->addHeader("Session", m_sessionID);
        }
    }
}

HX_RESULT
RTSPClientProtocol::SendOptionsMsgToServer(HXBOOL bKeepAlive)
{
    HX_RESULT           rc = HXR_OK;
    RTSPRequestMessage* pMsg = NULL;
    pMsg = new RTSPOptionsMessage(bKeepAlive);
    if (pMsg)
    {
        pMsg->setURL(getAggControlURL());
        AddCommonHeaderToMsg(pMsg);

        UINT32 seqNo = m_pSession->getNextSeqNo(this);
        rc = sendRequest(pMsg, seqNo);
    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }

    return rc;
}

HX_RESULT
RTSPClientProtocol::SendMsgToServer(RTSPMethod msg, UINT32* pulMsgSeqNum)
{
    HX_RESULT           rc = HXR_OK;
    RTSPRequestMessage* pMsg = NULL;

    switch (msg)
    {
       case RTSP_PLAY:
           pMsg = new RTSPPlayMessage;
           break;
       case RTSP_PAUSE:
           pMsg = new RTSPPauseMessage;
           break;
           break;
       case RTSP_TEARDOWN:
           pMsg = new RTSPTeardownMessage;
           break;
       default:
           break;
    }

    if (pMsg)
    {
        pMsg->setURL(getAggControlURL());
        AddCommonHeaderToMsg(pMsg);

        UINT32 seqNo = m_pSession->getNextSeqNo(this);
        rc = sendRequest(pMsg, seqNo);
	if (pulMsgSeqNum)
	{
	    *pulMsgSeqNum = seqNo;
	}
    }
    else
    {
        rc = HXR_OUTOFMEMORY;
    }

    return rc;
}

HX_RESULT
RTSPClientProtocol::handleOptionsResponse
(
    RTSPResponseMessage* pRTSPResponseMessageIncoming,
    RTSPOptionsMessage* pOptionsMsg
    )
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleOptionsResponse(): %lu",this, pRTSPResponseMessageIncoming->errorCodeAsUINT32());

    HX_RESULT   rc = HXR_OK;

    UINT32 uErrorCode = pRTSPResponseMessageIncoming->errorCodeAsUINT32();
    if (m_pPipelinedDescLogic)
    {
        m_pPipelinedDescLogic->OnOptionsResponse(pRTSPResponseMessageIncoming);
    }
        
    if (uErrorCode == 551)
    {
        /* Quite poor, but the client only supports this one require for now */
        return m_pResp->HandleOptionsResponse(HXR_LOADTEST_NOT_SUPPORTED,
                                              NULL);
    }
    else if (uErrorCode == 200)
    {
        // Filter out session timeout message reponses
        if(pOptionsMsg->IsKeepAlive())
        {
            return HXR_OK;
        }

        /*
         * XXXGH...I added this just for the stats mask, but the
         * authentication should be available from the 822 headers too
         */

        IHXValues* pRFC822Headers = NULL;
        getRFC822Headers(pRTSPResponseMessageIncoming, pRFC822Headers);

        if (pRFC822Headers)
        {
	    IHXBuffer* pBuffer = NULL;
            IHXKeyValueList* pRFC822List = NULL;

            if (HXR_OK == pRFC822Headers->QueryInterface(IID_IHXKeyValueList, (void**)&pRFC822List))
            {
                m_pResponseHeaders->AppendAllListItems(pRFC822List);
            }
            HX_RELEASE(pRFC822List);

	    if (HXR_OK == pRFC822Headers->GetPropertyCString("Server", pBuffer))
	    {            
                // Check to see if this is a Windows Media server
                const char* pszWMServer = "WMServer";
                if (!strncmp((const char*) pBuffer->GetBuffer(), pszWMServer, strlen(pszWMServer)) && !m_bHandleWMServers)
                {
                    // This is a Windows Media server and we are not supposed to
                    // handle the RTSP from Windows Media Servers. There is another
                    // component in the system to handle the RTSP client exchange
                    // from Windows Media Servers.
                    HX_RELEASE(pBuffer);
                    return m_pResp->HandleOptionsResponse(HXR_INCOMPATIBLE_RTSP_SERVER, NULL);
                }
                // Get the server version from the "Server" header
		::GetVersionFromString((char*)pBuffer->GetBuffer(), m_ulServerVersion);
	    }
	    HX_RELEASE(pBuffer);
        }

        if (m_sessionID.IsEmpty())
        {
            m_sessionID = pRTSPResponseMessageIncoming->getHeaderValue("Session");
        }

        // verify the result of ABD server support inquery
        if (m_pSession->amIDoingABD(this) &&
            (ABD_STATE_INQUERY == m_pSession->getABDState()))
        {
            CHXString SupportedHeader = pRTSPResponseMessageIncoming->getHeaderValue("Supported");
            // server denies ABD support
            if (SupportedHeader.IsEmpty() || -1 == SupportedHeader.Find("ABD-1.0"))
            {
                // immediately disable ABD so we avoid reentrancy problems
                m_pSession->AutoBWDetectionDone(HXR_NOT_SUPPORTED, 0);
            }
        }

        // Respond to Client Challenge to prove that we are a RealClient
        if (!m_pSession->m_bChallengeDone &&
            HXR_OK != RetrieveChallenge(pRTSPResponseMessageIncoming))
        {
            IHXValues* pResponseHeaders = NULL;

            // check for supported RTSP methods if the server could be non-RS
            if (m_pResponseHeaders &&
                HXR_OK == m_pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseHeaders))
            {
                IHXBuffer* pCmds = NULL;

                // Thanks IPTV for adding a space after Public
                if (HXR_OK == pResponseHeaders->GetPropertyCString("Allow", pCmds) ||
                    HXR_OK == pResponseHeaders->GetPropertyCString("Public", pCmds) ||
                    HXR_OK == pResponseHeaders->GetPropertyCString("Public ", pCmds))
                {
                    // all methods are supported by default
                    if (!strstr((char*)pCmds->GetBuffer(), "SETUP"))
                        m_pIsMethodSupported[SETUP] = FALSE;

                    // Is redirect supported
                    if (!strstr((char*)pCmds->GetBuffer(), "REDIRECT"))
                        m_pIsMethodSupported[REDIRECT] = FALSE;

                    // Is play supported
                    if (!strstr((char*)pCmds->GetBuffer(), "PLAY"))
                        m_pIsMethodSupported[PLAY] = FALSE;

                    // Is pause supported
                    if (!strstr((char*)pCmds->GetBuffer(), "PAUSE"))
                        m_pIsMethodSupported[PAUSE] = FALSE;

                    // Is set_param supported
                    if (!strstr((char*)pCmds->GetBuffer(), "SET_PARAMETER"))
                        m_pIsMethodSupported[SET_PARAM] = FALSE;

                    // Is get_param supported
                    if (!strstr((char*)pCmds->GetBuffer(), "GET_PARAMETER"))
                        m_pIsMethodSupported[GET_PARAM] = FALSE;

                    // Is describe supported
                    if (!strstr((char*)pCmds->GetBuffer(), "DESCRIBE"))
                        m_pIsMethodSupported[DESCRIBE] = FALSE;

                    // Is teardown supported
                    if (!strstr((char*)pCmds->GetBuffer(), "TEARDOWN"))
                        m_pIsMethodSupported[TEARDOWN] = FALSE;

                    // Is record supported
                    if (!strstr((char*)pCmds->GetBuffer(), "RECORD"))
                        m_pIsMethodSupported[RECORD] = FALSE;

                    // Is announce supported
                    if (!strstr((char*)pCmds->GetBuffer(), "ANNOUNCE"))
                        m_pIsMethodSupported[ANNOUNCE] = FALSE;
                }

                HX_RELEASE(pCmds);
            }
            HX_RELEASE(pResponseHeaders);
        }

        if (m_bSDPInitiated)
        {
            IHXValues* pResponseHeaders = NULL;
            if (HXR_OK == m_pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseHeaders))
            {
                // This sets our UseRTP preference in the case of an SDP file.
                // Other cases are addressed by handleDescribeResponse().
                HXBOOL bUseRTP = FALSE;
                if (DetermineIfPreferenceUseRTP(bUseRTP))
                {
                    pResponseHeaders->SetPropertyULONG32("UseRTP", bUseRTP);
                }

                m_url = m_headerControl;

                rc = m_pResp->HandleStreamDescriptionResponse
                    (
                        HXR_OK,
                        m_pSDPFileHeader,
                        m_pSDPStreamHeaders,
                        pResponseHeaders
                        );
            }
            HX_RELEASE(pResponseHeaders);
        }
        else
        {
            rc = m_pResp->HandleOptionsResponse(HXR_OK, pRFC822Headers);
            HX_RELEASE(pRFC822Headers);
        }

        return rc;
    }
    else
    {
        return m_pResp->HandleOptionsResponse(getHXRErrorCode(uErrorCode), NULL);
    }
}

HX_RESULT
RTSPClientProtocol::handleGetParamResponse(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleGetParamResponse(): %lu",this, pMsg->errorCodeAsUINT32());

    if (m_pSession->amIDoingABD(this)  &&
        (ABD_STATE_REQUEST == m_pSession->getABDState()))
    {
        CHXString AutoBWDetection = pMsg->getHeaderValue("AutoBWDetection");
        // the server doesn't support AutoBWDetection
        if (0 != AutoBWDetection.Compare("1"))
        {
            // this is unexpected behavior:
            // the server inidicates it supports ABD in OPTIONS' "Supported:" header
            // but it fails to echo back with the actual ABD request in GET_PARAM
            HX_ASSERT(FALSE);
            m_pSession->AutoBWDetectionDone(HXR_UNEXPECTED, 0);
        }

        return HXR_OK;
    }

    UINT32 uErrorCode = pMsg->errorCodeAsUINT32();

    if (uErrorCode != 200)
    {
        return m_pResp->HandleGetParameterResponse(getHXRErrorCode(uErrorCode), 0);
    }

    IHXBuffer* pBuffer = NULL;
    CreateBufferCCF(pBuffer, m_pContext);
    if(!pBuffer)
    {
        return HXR_OUTOFMEMORY;
    }
    const char* pContent = (char*)pMsg->getContent();
    HX_RESULT ret = pBuffer->Set((BYTE*)pContent, strlen(pContent) + 1);
    if( ret != HXR_OUTOFMEMORY )
    {
        ret = m_pResp->HandleGetParameterResponse(HXR_OK, pBuffer);
    }
    pBuffer->Release();
    return ret;
}

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
HX_RESULT
RTSPClientProtocol::HandleSetParamMulticastTransportHelper(RTSPResponseMessage* pMsg)
{
    HX_ASSERT(m_pNetSvc);

    const char* pMulticastIP = 0;
    const char* pMulticastDestPort = 0;
    const char* pMulticastSourcePort = 0;
    MIMEHeader* pIP = pMsg->getHeader("MulticastIP");
    if (pIP)
    {
        MIMEHeaderValue* pHeaderValue = pIP->getFirstHeaderValue();
        if (pHeaderValue)
        {
            MIMEParameter* pParam = pHeaderValue->getFirstParameter();
            if (pParam)
            {
                pMulticastIP = (const char*)pParam->m_attribute;
            }
        }
    }


    MIMEHeader* pPort = pMsg->getHeader("MulticastPort");
    if (pPort)
    {
        MIMEHeaderValue* pHeaderValue = pPort->getFirstHeaderValue();
        if (pHeaderValue)
        {
            MIMEParameter* pParam = pHeaderValue->getFirstParameter();
            if (pParam)
            {
                pMulticastDestPort = (const char*)pParam->m_attribute;
            }
        }
    }

    MIMEHeader* pSourcePort = pMsg->getHeader("MulticastSourcePort");
    if (pSourcePort)
    {
        MIMEHeaderValue* pHeaderValue = pSourcePort->getFirstHeaderValue();
        if (pHeaderValue)
        {
            MIMEParameter* pParam = pHeaderValue->getFirstParameter();
            if (pParam)
            {
                pMulticastSourcePort = (const char*)pParam->m_attribute;
            }
        }
    }

    HX_RESULT hr = HXR_OK;
    if (pMulticastIP && pMulticastDestPort && pMulticastSourcePort)
    {
        IHXSocket*              pUDPSocket = 0;
        UDPResponseHelper*      pUDPResponseHelper = NULL;
        IHXSockAddr*            pLocalAddr = 0;
        IHXSockAddr*            pMcastAddr = 0;
        UINT16                  nToPort = 0;


        RTSPTransport* pMcastTran = GetTransport(0);
        HX_ASSERT(pMcastTran);
        if (!pMcastTran)
        {
            hr = HXR_FAIL;
            goto bail;
        }

        // map data reception port to this transport; used to look
        // up transport if and when we get a datagram on this port
        nToPort = atoi(pMulticastDestPort);
        (*m_pTransportMPortMap)[nToPort] = pMcastTran;

        // create mcast group address
        hr = HXSockUtil::CreateAddr(m_pNetSvc, HX_SOCK_FAMILY_INANY, pMulticastIP, nToPort, pMcastAddr);
        if (FAILED(hr))
        {
            goto bail;
        }

        // create multicast socket
        HX_ASSERT(!pUDPResponseHelper);
        hr = HXSockUtil::CreateSocket(m_pNetSvc, pUDPResponseHelper,
                                      pMcastAddr->GetFamily(),
                                      HX_SOCK_TYPE_UDP,
                                      HX_SOCK_PROTO_ANY, pUDPSocket);
        if (FAILED(hr))
        {
            goto bail;
        }

        // create local address for binding
        hr = pUDPSocket->CreateSockAddr(&pLocalAddr);
        if (FAILED(hr))
        {
            goto bail;
        }

        if (m_pFWCtlMgr)
        {
            m_pFWCtlMgr->OpenPort(nToPort, HX_NET_FW_IP_PROTOCOL_UDP);
            HX_ASSERT(0 == m_FWPortToBeClosed);
            m_FWPortToBeClosed = nToPort;
        }

        pLocalAddr->SetPort(nToPort);
        pUDPSocket->SetOption(HX_SOCKOPT_REUSEADDR, 1);
        hr = pUDPSocket->Bind(pLocalAddr);
        if (FAILED(hr))
        {
            goto bail;
        }

        HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::HandleSetParamMulticastTransportHelper(): joined %s:%s (source port %s)", this, pMulticastDestPort, pMulticastIP,pMulticastSourcePort);

        pUDPResponseHelper = new UDPResponseHelper(this);
        if (!pUDPResponseHelper)
        {
            hr = HXR_OUTOFMEMORY;
            goto bail;
        }
        pUDPResponseHelper->AddRef();
        pUDPResponseHelper->SetSock(pUDPSocket);
        m_UDPResponseHelperList.AddTail(pUDPResponseHelper);

        pUDPSocket->SetResponse(pUDPResponseHelper);
        pUDPSocket->SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);
        pUDPSocket->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);

        pMcastTran->JoinMulticast(pMcastAddr, pUDPSocket);

      bail:
        HX_RELEASE(pLocalAddr);
        HX_RELEASE(pMcastAddr);
        HX_RELEASE(pUDPSocket);
        if (FAILED(hr))
        {
            HX_RELEASE(pUDPResponseHelper);

            if (0 != m_FWPortToBeClosed && m_pFWCtlMgr)
            {
                m_pFWCtlMgr->ClosePort(m_FWPortToBeClosed, HX_NET_FW_IP_PROTOCOL_UDP);
                m_FWPortToBeClosed = 0;
            }
        }
    }

    return hr;
}
#endif //HELIX_FEATURE_TRANSPORT_MULTICAST

HX_RESULT
RTSPClientProtocol::handleSetParamResponse(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleSetParamResponse(): %lu", this, pMsg->errorCodeAsUINT32());
    IHXValues* pValues = NULL;
    HX_RESULT theErr = HXR_FAIL;

    if (m_pResp)
    {
        UINT32 uErrorCode = pMsg->errorCodeAsUINT32();

        if (uErrorCode != 200)
        {
            if (m_bNonRSRTP)
            {
                return m_pResp->HandleSetParameterResponse(HXR_OK);
            }
            else
            {
                return m_pResp->HandleSetParameterResponse(getHXRErrorCode(uErrorCode));
            }
        }

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
        HX_RESULT hr = HandleSetParamMulticastTransportHelper(pMsg);
        if(FAILED(hr))
        {
            return hr;
        }
#endif

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
        MIMEHeader* pStatsInterval = pMsg->getHeader("UpdateStatsInterval");
        if (pStatsInterval)
        {
            MIMEHeaderValue* pHeaderValue = pStatsInterval->getFirstHeaderValue();
            if (pHeaderValue)
            {
                MIMEParameter* pParam = pHeaderValue->getFirstParameter();
                if (pParam)
                {
                    // stats interval
                    UINT32 ulStatsInterval = (UINT32) atoi((const char*)pParam->m_attribute);

		    theErr = CreateValuesCCF(pValues, m_pContext);
		    if (HXR_OK == theErr)
		    {
                        pValues->SetPropertyULONG32("UpdateStatsInterval", ulStatsInterval);
                    }
                }
            }
        }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

        if( theErr != HXR_OUTOFMEMORY )
        {
            theErr = m_pResp->HandleSetParameterResponseWithValues(HXR_OK, pValues);
        }
        HX_RELEASE(pValues);
    }

    return theErr;
}

HX_RESULT
RTSPClientProtocol::handleTeardownResponse(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleTeardownResponse(): %lu", this, pMsg->errorCodeAsUINT32());
    if(m_pResp)
    {
        UINT32 uErrorCode = pMsg->errorCodeAsUINT32();

        if (uErrorCode != 200)
        {
            m_pResp->HandleTeardownResponse(getHXRErrorCode(uErrorCode));
        }
        else
        {
            m_pResp->HandleTeardownResponse(HXR_OK);
        }
    }

    /*
     * The control channel is now closed
     */

    return HXR_NET_SOCKET_INVALID;
}

HX_RESULT
RTSPClientProtocol::handleRecordResponse(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleRecordResponse(): %lu", this, pMsg->errorCodeAsUINT32());
    UINT32 uErrorCode = pMsg->errorCodeAsUINT32();

    if (uErrorCode != 200)
    {
        return m_pResp->HandleRecordResponse(getHXRErrorCode(uErrorCode));
    }

    if (m_bConnectionlessControl)
    {
        closeSocket();

        if (HXR_OK ==
            m_pResp->QueryInterface(IID_IHXConnectionlessControl,
                                    (void**)&m_pConnectionlessControl))
        {
            m_pConnectionCheckCallback = new RTSPClientProtocol::TimeoutCallback(this, RTSPCLIENT_TIMEOUT_CONNECTION);
            m_pConnectionCheckCallback->AddRef();
            m_uConnectionCheckCallbackHandle =
                m_pScheduler->RelativeEnter(m_pConnectionCheckCallback,
                                            m_uConnectionTimeout * 1000);
        }
    }

    return m_pResp->HandleRecordResponse(HXR_OK);
}

HX_RESULT
RTSPClientProtocol::handlePauseResponse(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handlePauseResponse(): %lu", this, pMsg->errorCodeAsUINT32());
    /*
     * XXX...Bruce is there anything to do here?
     */

    return HXR_OK;
}

HX_RESULT
RTSPClientProtocol::handlePlayResponse(RTSPResponseMessage* pMsg,
                                       RTSPPlayMessage* pPlayMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handlePlayResponse(): %lu", this, pMsg->errorCodeAsUINT32());
    /*  Message Format:
     *  RTSP/0.5 200 302 OK
     *  RTP-Info: url=foo/streamid=0;seq=32;rtptime=40182123,
     *     url=foo/streamid=1;seq=410;rtptime=40199211
     */

    UINT32 uErrorCode = pMsg->errorCodeAsUINT32();
    if (uErrorCode != 200)
    {
        if (uErrorCode == 456)
        {
            return m_pResp->HandlePlayResponse(HXR_INVALID_OPERATION);
        }
        else return m_pResp->HandlePlayResponse(getHXRErrorCode(uErrorCode));
    }

    MIMEHeader* pSequence = pMsg->getHeader("RTP-Info");
    MIMEHeaderValue* pSeqValue = 0;
    UINT16 streamID = 0;
    UINT16 seqNum = 0;
    UINT32 ulRTPTime = 0;
    const char* pControl = 0;
    HXBOOL bIsResumeResponseThisRound;
    RTPInfoEnum RTPErr;
    HXBOOL bIsResumeResponse;
    HXBOOL bIsPlayResponse;
    UINT32 ulNumPlayResponsesTrimmed;

    bIsResumeResponse = trimRequestPendingReplyList(m_ResumeRequestPendingReplyList,
						    pMsg->seqNo());

    bIsPlayResponse = trimRequestPendingReplyList(m_PlayRequestPendingReplyList,
						  pMsg->seqNo(),
						  &ulNumPlayResponsesTrimmed);

    // The response cannot be both play and resume response
    HX_ASSERT(!(bIsResumeResponse && bIsPlayResponse));
    HX_ASSERT(bIsResumeResponse || bIsPlayResponse);

    if (!(bIsResumeResponse || bIsPlayResponse))
    {
	// We are not expecting PLAY/RESUME response - ignore it
	return HXR_OK;
    }

    // Update range entries
    // We want to update the play-range before setting first time-stamp
    // information in the transport to allow mapping to NPT time to
    // occur correctly.
    MIMEHeader* pRange = pMsg->getHeader("Range");
    if (pRange)
    {
        pSeqValue = pRange->getFirstHeaderValue();

        INT32 nFrom = 0, nTo = 0;

        if (pSeqValue)
        {
            MIMEParameter* pParam = pSeqValue->getFirstParameter();

            if (pParam)
            {
                const char* pRange = (const char*) pParam->m_attribute;
                const char* pDash = NULL;
                char* pStopString;
                double dTemp;

                if (pRange)
                {
                    dTemp = strtod(pRange, &pStopString);
                    nFrom = (INT32)(dTemp * 1000);

                    pDash  = strrchr(pRange, '-');
                }

                if (pDash)
                {
                    dTemp = strtod(pDash + 1, &pStopString);
                    nTo = (INT32)(dTemp * 1000);
                }
            }
        }

        if (!m_transportRequestList.IsEmpty())
        {
            RTSPTransportRequest* pRequest =
                (RTSPTransportRequest*)m_transportRequestList.GetHead();

            RTSPTransportInfo* pTransInfo = pRequest->getFirstTransportInfo();

            while(pTransInfo && nTo)
            {
                // set the range in transport...only for RTP
                pTransInfo->m_pTransport->RTSPTransport::setPlayRange((UINT32)nFrom, (UINT32)nTo);
                pTransInfo = pRequest->getNextTransportInfo();
            }
        }
    }

    // If server is operating correcty, we should never see ulNumPlayResponsesTrimmed > 1
    // or ulNumPlayResponsesTrimmed != 0 when bIsResumeResponse == TRUE.
    // The logic here is implemented to make client as robust as possible in cases
    // where server fails to respond to some PLAY/RESUME requests.
    while ((ulNumPlayResponsesTrimmed > 0) || bIsResumeResponse)
    {
	bIsResumeResponseThisRound = FALSE;
	if ((ulNumPlayResponsesTrimmed == 0) && bIsResumeResponse)
	{
	    bIsResumeResponseThisRound = TRUE;
	    bIsResumeResponse = FALSE;
	}

	if (pSequence)
	{
	    pSeqValue = pSequence->getFirstHeaderValue();
	}

	// per spec., "RTP-Info" has to be present in the 1st Play
	// response header received, regardless whether the transport
	// is TNG or RTP
	if (!pSeqValue && !m_bSeqValueReceived)
	{
	    // XXXGo - interop hack
	    if ((!(m_bIPTV || m_bColumbia)) || (m_pControlToStreamNoMap == NULL))
	    {
		return m_pResp->HandlePlayResponse(HXR_BAD_SERVER);
	    }
	}

	if (pSeqValue)
	{
	    do
	    {
		RTPErr = parseRTPInfoHeader(pSeqValue, streamID, seqNum,
		    ulRTPTime, pControl);

		// if m_pControlToStreamNoMap, don't trust the parseRTPInfoHeader
		// because RTP-Info url could be not what we expect and still be ok with
		// spec
		HX_ASSERT(pControl);

		RTSPStreamInfo* pInfo = getStreamInfoFromSetupRequestURL(pControl);

		if (pInfo)
		{
		    streamID = pInfo->m_streamNumber;
		}

		pControl = 0;

		// Without pInfo, we do not know the streamID to set RTPInfo to.
		if ((RTPINFO_ERROR != RTPErr) && pInfo)
		{
		    _SetRTPInfo(streamID, seqNum, ulRTPTime, RTPErr, bIsResumeResponseThisRound);
		}

		pSeqValue = pSequence->getNextHeaderValue();
	    } while (pSeqValue);
	}

	// Meke sure all streams involved are aware that
	// RTP Info has been processed and no more info is comming.
	NotifyStreamsRTPInfoProcessed(bIsResumeResponseThisRound);

	if (ulNumPlayResponsesTrimmed > 0)
	{
	    ulNumPlayResponsesTrimmed--;
	}
    }

    m_bSeqValueReceived = TRUE;

    if (m_bConnectionlessControl)
    {
        closeSocket();
    }

    MIMEHeader* pXPredecPeriod = pMsg->getHeader("x-initpredecbufperiod");

    if (pXPredecPeriod)
    {
        MIMEHeaderValue* pPrerollValue = pXPredecPeriod->getFirstHeaderValue();

        if (pPrerollValue)
        {
            const char* pStart = pPrerollValue->value();
            char* pEnd = 0;
            ULONG32 ulValue = strtoul(pStart, &pEnd, 10);

            if (*pStart && !*pEnd)
            {
                // Handle updated preroll condition
                m_pResp->HandlePrerollChange(RTSP_PREROLL_PREDECBUFPERIOD,
                                             ulValue);
            }
        }
    }

    return m_pResp->HandlePlayResponse(HXR_OK);
}

HX_RESULT
RTSPClientProtocol::handleSetupResponse(RTSPResponseMessage* pMsg,
                                        RTSPSetupMessage* pSetupMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleSetupResponse(): %lu", this, pMsg->errorCodeAsUINT32());

    HX_RESULT status = HXR_OK;
    UINT16 streamNumber = 0;
    IHXValues* pReconnectValues = NULL;
    RTSPStreamInfo* pStreamInfo = 0;
    UINT32 uErrorCode = pMsg->errorCodeAsUINT32();

    if (uErrorCode == 401 || uErrorCode == 407)
    {
        status = handleAuthentication(pMsg);
        return status;
    }
    else if (uErrorCode != 200)
    {
        return m_pResp->HandleSetupResponse(HXR_BAD_TRANSPORT);
    }

    /* SETUP succeeded */
    m_setupResponseCount++;

    // we need to find the right StreamInfo obj..
    pStreamInfo = getStreamInfoFromSetupRequestURL(pSetupMsg->url());

    if(!pStreamInfo)
    {
        return m_pResp->HandleSetupResponse(HXR_BAD_TRANSPORT);
    }
    else
    {
        streamNumber = pStreamInfo->m_streamNumber;
    }

    CHXString reconnectFlag = pMsg->getHeaderValue("Reconnect");
    if (reconnectFlag != "" && strcasecmp((const char*)reconnectFlag, "false") == 0)
    {
	if (HXR_OK == CreateValuesCCF(pReconnectValues, m_pContext))
	{
	    pReconnectValues->SetPropertyULONG32("Reconnect", 0);
	}
    }
    else
    {
        MIMEHeader* pHeader = pMsg->getHeader("Alternate-Server");
        if (pHeader)
        {
            RetrieveReconnectInfo(pHeader, ALTERNATE_SERVER, pReconnectValues);
        }

        pHeader = pMsg->getHeader("Alternate-Proxy");
        if (pHeader)
        {
            RetrieveReconnectInfo(pHeader, ALTERNATE_PROXY, pReconnectValues);
        }
    }

    if (pReconnectValues)
    {
        m_pResp->HandleSetParameterResponseWithValues(HXR_OK, pReconnectValues);
    }
    HX_RELEASE(pReconnectValues);

    CHXString sessionID = pMsg->getHeaderValue("Session");
    if(sessionID != "")
    {
        int i;
        if (-1 != (i = sessionID.Find(';')))
        {
            m_sessionID = sessionID.Left(i);
        }
        else
        {
            m_sessionID = sessionID;
        }
    }

    if (m_pRateAdaptInfo)
    {
        handleRateAdaptResponse(pSetupMsg, pMsg, streamNumber);
    }

    // check whether we're connected to a PV server, and need to behave like a PV client
    MIMEHeader* pHeader = pMsg->getHeader("Server");
    if(pHeader)
    {
        MIMEHeaderValue* pValue = pHeader->getFirstHeaderValue();
        MIMEParameter* pParam = pValue->getFirstParameter();
        const char* pszPVServerPrefix = "PVSS";

        if(!strncmp(pParam->m_attribute, pszPVServerPrefix, strlen(pszPVServerPrefix)))
        {
            pHeader = pMsg->getHeader("Transport");
            if(pHeader)
            {
                pStreamInfo->m_ulSSRCFromSetup = getSSRCFromTransportHeader(pHeader->getFirstHeaderValue());
            }
        }
    }


    //We need to check to see if we are going through any non Real proxies. If
    //we are, we need to make sure we turn off pipelined RTSP, since some
    //non-real proxies don't know how to handle it.
    MIMEHeader* pViaHeader = pMsg->getHeader("Via");
    CHXString it="";
    if( pViaHeader )
    {
        MIMEHeaderValue* pViaValue = pViaHeader->getFirstHeaderValue();
        while( pViaValue )
        {
            pViaValue->asString(it);
            if( -1 == it.Find("RealProxy") )
            {
                //This is a non-Real proxy. Only real proxies are known to work
                //with RTSP piplining. If you find another, add it here.
                m_bPipelineRTSP = FALSE;
            }
            //Test for NetCache appliance.
            if( -1 != it.Find("NetCache NetApp") )
            {
                //Found at least one NetCache Proxy.
                m_bTrimTrailingSlashes = TRUE;
                break;
            }
            pViaValue = pViaHeader->getNextHeaderValue();
        }
    }
    //We also turn off RTSP pipelining if this is not a realserver(helix).
    if( !IsRealServer() )
    {
        m_bPipelineRTSP = FALSE;
    }
 
    UpdateChosenTransport(pMsg);

    status = handleSetupResponseExt(pStreamInfo, pMsg, pSetupMsg);

    // Since the port is mapped into m_pTransportPortMap during
    // handleSetupResponseExt(), then we will now be able
    // to send any UDP packets that arrive in ReadFromDone() to
    // the RTSPTransport. Check to see whether any UDP packets
    // arrived on this port before the transport port was mapped.
    if (SUCCEEDED(status))
    {
        // We have to check both port pStreamInfo->m_sPort
        // and port pStreamInfo->m_sPort + 1, since RTP
        // uses two ports.
        for (UINT16 i = 0; i < 2; i++)
        {
            // Get the port number
            UINT16 usPort = pStreamInfo->m_sPort + i;
            // Any packets received on this port yet?
            if (AnyPreSetupResponsePackets(usPort))
            {
                // We did have packets arrive before now, so flush
                // them to the transport.
                FlushPreSetupResponsePacketsToTransport(usPort);
            }
        }
    }

    if ((HXR_OK == status)              &&
        m_pSession->amIDoingABD(this)   &&
        (ABD_STATE_INQUERY == m_pSession->getABDState()))
    {
        m_pSession->setABDState(ABD_STATE_REQUEST);

        // initiate Auto Bandwidth Detection if the server supports
        //
        // Note, the client poked the UDP port via setPeerAddr() after handling
        //       SETUP response
        status = SendGetParameterRequest(0, "ABD");
        if (HXR_OK != status)
        {
            m_pSession->AutoBWDetectionDone(HXR_UNEXPECTED, 0);
        }
    }
    else
    {
        status = canSendRemainingSetupRequests(status);
    }

    return status;
}

HX_RESULT
RTSPClientProtocol::handleSetupResponseExt(RTSPStreamInfo* pStreamInfo,
                                           RTSPResponseMessage* pMsg,
                                           RTSPSetupMessage* pSetupMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleSetupResponseExt()", this);

    HX_RESULT status = HXR_OK;

    // get transport info
    CHXString transportType;
    UINT16 streamNumber = pStreamInfo->m_streamNumber;
    MIMEHeader* pTransport = pMsg->getHeader("Transport");
    if(pTransport)
    {
        MIMEHeaderValue* pValue = pTransport->getFirstHeaderValue();
        if(!pValue)
        {
            // return some awful error
            HX_ASSERT(false);
            return HXR_FAIL;
        }
        RTSPTransportRequest* pRequest = getTransportRequest(pValue);
        if(!pRequest)
        {
            // return another awful error
            return HXR_FAIL;
        }

        m_pSession->m_bChallengeMet = TRUE;
        m_pSession->m_bChallengeDone = TRUE;

        RTSPTransportInfo* pTransInfo = pRequest->getTransportInfo(streamNumber);

        // get the server address we are connecting to
        // used to filter out any UDP packets received from 3rd party
        HX_ASSERT(!m_pConnectAddr);
        if (!m_pConnectAddr && m_pSocket->GetPeerAddr(&m_pConnectAddr) != HXR_OK)
        {
            HX_ASSERT(FALSE);
            status = HXR_BAD_TRANSPORT;
        }

        IHXSockAddr* pResendAddr = NULL;
        if (m_pPeerAddr)
        {
            m_pPeerAddr->Clone(&pResendAddr);
        }
        else
        {
            m_pConnectAddr->Clone(&pResendAddr);
        }
        if (!pResendAddr)
        {
            return HXR_OUTOFMEMORY;
        }
        pResendAddr->SetPort(pRequest->m_sResendPort);

        pStreamInfo->m_sPort = pTransInfo->m_sPort;

        HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleSetupResponseExt(): stream %u; port %u", this, pStreamInfo->m_streamNumber, pTransInfo->m_sPort);

#if defined(HELIX_FEATURE_RTP)
        switch(pRequest->m_lTransportType)
        {
           case RTSP_TR_RTP_TCP:
           {

               RTPTCPTransport* pRtpTran = (RTPTCPTransport*)pTransInfo->m_pTransport;
               RTCPTCPTransport* pRtcpTran = (RTCPTCPTransport*)pTransInfo->m_pRTCPTransport;

               HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleSetupResponseExt(): RTP TCP transport ", this);

               if ((!m_bHasSyncMasterStream) &&
                   (pStreamInfo->m_eMediaType == RTSPMEDIA_TYPE_AUDIO))
               {
                   pStreamInfo->m_bIsSyncMaster = TRUE;
                   m_bHasSyncMasterStream = TRUE;
               }

               pRtpTran->addStreamInfo(pStreamInfo);
               pRtcpTran->addStreamInfo(pStreamInfo);
               (*m_pTransportStreamMap)[pStreamInfo->m_streamNumber] = pRtpTran;

               SetBuffByteLimit(pRtpTran, pStreamInfo);

               if (!m_activeTransportList.Find(pRtpTran))
               {
                   m_activeTransportList.AddTail(pRtpTran);
               }

               m_pSession->setProtocolInterleave(this,
                                                 pRequest->m_tcpInterleave);
               m_pSession->setProtocolInterleave(this,
                                                 (INT8)(pRequest->m_tcpInterleave + 1));

               pRtpTran->setInterleaveChannel(pRequest->m_tcpInterleave);
               pRtcpTran->setInterleaveChannel((INT8)(pRequest->m_tcpInterleave + 1));
               if (!m_sessionID.IsEmpty())
               {
                   pRtpTran->setSessionID(m_sessionID);
               }

               mapTransportChannel(pRtpTran, pRequest->m_tcpInterleave);
               mapTransportChannel(pRtcpTran, (UINT16)(pRequest->m_tcpInterleave + 1));

               mapControlToStreamNo(pStreamInfo->m_streamControl,
                                    pStreamInfo->m_streamNumber);

               /* Temporary */
               m_uProtocolType = 3;

               if (m_pSession->amIDoingABD(this))
               {
                   // immediately disable ABD so we avoid reentrancy problems
                   m_pSession->AutoBWDetectionDone(HXR_NOT_SUPPORTED, 0);
               }
                
           }
           break;

           case RTSP_TR_RTP_UDP:
           {
               RTPUDPTransport* pRtpTran = (RTPUDPTransport*)pTransInfo->m_pTransport;
               RTCPUDPTransport* pRtcpTran = (RTCPUDPTransport*)pTransInfo->m_pRTCPTransport;

               HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleSetupResponseExt(): RTP UDP transport", this);

               if ((!m_bHasSyncMasterStream) &&
                   (pStreamInfo->m_eMediaType == RTSPMEDIA_TYPE_AUDIO))
               {
                   pStreamInfo->m_bIsSyncMaster = TRUE;
                   m_bHasSyncMasterStream = TRUE;
               }

               pRtpTran->addStreamInfo(pStreamInfo);
               pRtcpTran->addStreamInfo(pStreamInfo);
               (*m_pTransportStreamMap)[pStreamInfo->m_streamNumber] = pRtpTran;
               (*m_pTransportPortMap)[pTransInfo->m_sPort] = pRtpTran;
               (*m_pTransportPortMap)[pTransInfo->m_sPort+1] = pRtcpTran;

               SetBuffByteLimit(pRtpTran, pStreamInfo);

               if (!m_activeTransportList.Find(pRtpTran))
               {
                   m_activeTransportList.AddTail(pRtpTran);
               }

               mapControlToStreamNo(pStreamInfo->m_streamControl, pStreamInfo->m_streamNumber);

               pRtpTran->setPeerAddr(pResendAddr);

               IHXSockAddr* pResendAddr2 = NULL;
               pResendAddr->Clone(&pResendAddr2); //XXXLCM oom
               pResendAddr2->SetPort((UINT16)(pResendAddr->GetPort() + 1));

               pRtcpTran->setPeerAddr(pResendAddr2);

               HX_RELEASE(pResendAddr2);

               if (!m_sessionID.IsEmpty())
               {
                   pRtpTran->setSessionID(m_sessionID);
               }

               /* Temporary */
               m_uProtocolType = 2;

               if (m_pSession->amIDoingABD(this))
               {
                   // immediately disable ABD so we avoid reentrancy problems
                   m_pSession->AutoBWDetectionDone(HXR_NOT_SUPPORTED, 0);
               }

           }
           break;

           default:
           {
               status = HXR_BAD_TRANSPORT;
           }
           break;
        }
#endif /* HELIX_FEATURE_RTP */
        HX_RELEASE(pResendAddr);
    }

    return status;
}

HX_RESULT
RTSPClientProtocol::handleAnnounceResponse(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleAnnounceResponse(): %lu", this, pMsg->errorCodeAsUINT32());

    HX_RESULT rc = HXR_OK;

    if(!m_bSetupRecord) // better only get one of these if recording...
    {
        return HXR_FAIL;
    }

    UINT32 uErrorCode = pMsg->errorCodeAsUINT32();

    if (uErrorCode == 401 || uErrorCode == 407)
    {
        rc = handleAuthentication(pMsg);
        return rc;
    }
    else if(uErrorCode == 409)
    {
        return m_pResp->HandleStreamRecordDescriptionResponse(HXR_ALREADY_OPEN, 0);
    }
    else if (uErrorCode != 200)
    {
        return m_pResp->HandleStreamRecordDescriptionResponse(getHXRErrorCode(uErrorCode), 0);
    }

    IHXValues* pRFC822Headers = NULL;
    getRFC822Headers(pMsg, pRFC822Headers);

    if(pRFC822Headers )
    {
        IHXKeyValueList* pRFC822List = NULL;

        if (HXR_OK == pRFC822Headers->QueryInterface(IID_IHXKeyValueList, (void**)&pRFC822List))
        {
            m_pResponseHeaders->AppendAllListItems(pRFC822List);
        }
        HX_RELEASE(pRFC822List);
    }
    HX_RELEASE(pRFC822Headers);

    IHXValues* pResponseHeaders = NULL;

    if (HXR_OK == m_pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseHeaders))
    {
        rc = m_pResp->HandleStreamRecordDescriptionResponse(HXR_OK,
                                                            pResponseHeaders);
    }
    else
    {
        rc = m_pResp->HandleStreamRecordDescriptionResponse(HXR_FAILED,
                                                            NULL);
    }
    HX_RELEASE(pResponseHeaders);

    return rc;
}

HX_RESULT
RTSPClientProtocol::handleDescribeResponse(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleDescribeResponse(): %lu", this, pMsg->errorCodeAsUINT32());

    HX_RESULT   rc = HXR_OK;
    HXBOOL        bSDResponseCalled = FALSE;

    if (m_pPipelinedDescLogic &&
        (m_pPipelinedDescLogic->OnDescribeResponse(pMsg, rc)))
    {
        return rc;
    }

    if(m_bSetupRecord)  // better only get one of these if playing...
    {
        return HXR_FAIL;
    }

    UINT32 uErrorCode = pMsg->errorCodeAsUINT32();

    if (uErrorCode == 401 || uErrorCode == 407)
    {
        rc = handleAuthentication(pMsg);
        return rc;
    }
    else if (uErrorCode == 551)
    {
        // A Require option was not supported.
        // We could check the Unsupported header
        // to see which one(s) failed, but
        // since we only support one right now..

        m_bEntityRequired = FALSE;

        // Re-Send the describe w/o the Require..
        return m_pResp->HandleStreamDescriptionResponse
            (
                HXR_OK,
                0,
                0,
                0
                );
    }
    else if(uErrorCode == 403)
    {
        return m_pResp->HandleStreamDescriptionResponse
            (
                HXR_FORBIDDEN,
                0,
                0,
                0
                );
    }
    else if (uErrorCode != 200)
    {
        return m_pResp->HandleStreamDescriptionResponse(getHXRErrorCode(uErrorCode), 0, 0, 0);
    }

    // We do not handle content-encoding
    MIMEHeader* pContentEncoding = pMsg->getHeader("Content-Encoding");
    if(pContentEncoding)
    {
        if (pContentEncoding->getFirstHeaderValue())
        {
            return HXR_UNEXPECTED_MSG;
        }
    }

    // Set Context for use by sendSetupRequestMessage()
    CHXString sessionID = pMsg->getHeaderValue("ETag");
    if(!sessionID.IsEmpty())
    {
        m_sessionID = sessionID;
    }

    IHXValues* pRFC822Headers = NULL;
    getRFC822Headers(pMsg, pRFC822Headers);

    if(pRFC822Headers)
    {
        // XXXGo - interop hack...It should be "Server", but IPTV put this
        // string in "User-Agent"...
        IHXBuffer* pAgent  = NULL;
        if (pRFC822Headers->GetPropertyCString("Server", pAgent) != HXR_OK)
        {
            // try this...
            pRFC822Headers->GetPropertyCString("User-Agent", pAgent);
        }

        if (pAgent)
        {
            HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleDescribeResponse(): agent = %s", this, pAgent->GetBuffer());
            if (strncasecmp((const char*)pAgent->GetBuffer(), "Columbia RTSP Server",
                            20) == 0)
            {
                m_bColumbia = TRUE;
                m_bNoKeepAlive = TRUE;
            }
            else if (strncasecmp((const char*)pAgent->GetBuffer(), "Cisco IPTV",
                                 10) == 0)
            {
                m_bIPTV = TRUE;
            }
            else if (strncasecmp((const char*)pAgent->GetBuffer(), "Cisco IP/TV",
                                 11) == 0)
            {
                m_bIPTV = TRUE;
            }
            else if (strncasecmp((const char*)pAgent->GetBuffer(), "QTSS",
                                 4) == 0)
            {
                // once we send SET_PARAM for a keep alive, QTS won't be
                // responsive for any other request...so don't send
                // keep alive.
                m_bNoKeepAlive = TRUE;
                m_bForceUCaseTransportMimeType = TRUE;
            }
            else if (strncasecmp((const char*)pAgent->GetBuffer(), "DSS",
                                 3) == 0)
            {
                m_bForceUCaseTransportMimeType = TRUE;
            }
            HX_RELEASE(pAgent);
        }

        IHXKeyValueList* pRFC822List = NULL;

        if (HXR_OK == pRFC822Headers->QueryInterface(IID_IHXKeyValueList, (void**)&pRFC822List))
        {
            m_pResponseHeaders->AppendAllListItems(pRFC822List);
        }
        HX_RELEASE(pRFC822List);
    }
    HX_RELEASE(pRFC822Headers);

    // Respond to Client Challenge to prove that we are a RealClient
    if (!m_pSession->m_bChallengeDone)
    {
        RetrieveChallenge(pMsg);
    }

    // We need a content base entry to handle relative urls.
    // Check for one in the order specified in:
    // http://www.zvon.org/tmRFC/RFC2326/Output/chapter19.html
    MIMEHeader* pContentBaseHeader = pMsg->getHeader("Content-Base");
    if(pContentBaseHeader)
    {
        MIMEHeaderValue* pValue = pContentBaseHeader->getFirstHeaderValue();
        m_contentBase = pValue->value();
    }

    if (m_contentBase.IsEmpty())
    {
        pContentBaseHeader = pMsg->getHeader("Content-Location");
        if(pContentBaseHeader)
        {
            MIMEHeaderValue* pValue = pContentBaseHeader->getFirstHeaderValue();
            m_contentBase = pValue->value();
        }
    }

    if (m_contentBase.IsEmpty())
    {
        INT32 nOffset = m_url.ReverseFind('/');
        m_contentBase = m_url.Left(nOffset+1);
    }

    // Format the content base member
    if (m_contentBase[m_contentBase.GetLength()-1] != '/')
    {
        INT32 nOffset = m_contentBase.ReverseFind('/');
        m_contentBase.SetAt(nOffset+1, '\0');
        m_contentBase.GetBufferSetLength(nOffset+1);
    }

    MIMEHeader* pContentTypeHeader = pMsg->getHeader("Content-type");
    MIMEHeader* pContentLengthHeader = pMsg->getHeader("Content-length");

    if(pContentTypeHeader && pContentLengthHeader)
    {
        MIMEHeaderValue* pContentValue =
            pContentTypeHeader->getFirstHeaderValue();
        if(!pContentValue)
        {
            // error
            rc = HXR_FAIL;
        }
        else
        {
            IHXBuffer* pBuffer = NULL;
	    rc = CreateAndSetBufferCCF(pBuffer, (BYTE*)pMsg->getContent(), 
		 		       strlen(pMsg->getContent())+1, m_pContext);
	    if (HXR_OK != rc)
	    {
                HX_RELEASE(pBuffer);
                goto cleanup;
	    }

            rc = ParseSDP(pContentValue->value(), pBuffer);
            if (HXR_OK == rc)
            {
                IHXValues* pResponseHeaders = NULL;
                if (HXR_OK == m_pResponseHeaders->QueryInterface(IID_IHXValues, (void**)&pResponseHeaders))
                {
                    HXBOOL bUseRTP = FALSE;
                    if (DetermineIfPreferenceUseRTP(bUseRTP))
                    {
                        pResponseHeaders->SetPropertyULONG32("UseRTP", bUseRTP);
                    }

                    bSDResponseCalled = TRUE;

                    rc = m_pResp->HandleStreamDescriptionResponse
                        (
                            HXR_OK,
                            m_pSDPFileHeader,
                            m_pSDPStreamHeaders,
                            pResponseHeaders
                            );
                }
                HX_RELEASE(pResponseHeaders);
            }
            HX_RELEASE(pBuffer);
        }
    }
    else
    {
        rc = HXR_FAILED;
    }

  cleanup:

    if (HXR_OK != rc && !bSDResponseCalled)
    {
        rc = m_pResp->HandleStreamDescriptionResponse
            (
                rc,
                0,
                0,
                0
                );
    }
    return rc;
}

HXBOOL 
RTSPClientProtocol::DetermineIfPreferenceUseRTP(HXBOOL& bUseRTP)
{
    HXBOOL bSetPreference = FALSE;

    if (!IsRealServer())
    {
        HXBOOL bForceRTP = TRUE;

        ReadPrefBOOL(m_pPreferences, "NonRS", bForceRTP);
        if (bForceRTP)
        {
            bSetPreference = TRUE;
            bUseRTP = TRUE;
        }
    }
    else
    {
        ReadPrefBOOL(m_pPreferences, "UseRTP", bUseRTP);
        if(bUseRTP && IsRealDataType())
        {
            // UseRTP should be interpreted as PreferRTP
            // Preference of RTP is for 3gp,
            // and RM is streamed via RDT.
            // Hence Override the pref 'UseRTP' for RM datatypes
            bSetPreference = TRUE;
            bUseRTP = FALSE;
        }
    }

    return bSetPreference;
}

HX_RESULT
RTSPClientProtocol::sendInitialMessage(RTSPClientSession* pSession,
                                       IHXSocket* pSocket)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::sendInitialMessage()", this);

    HX_RESULT           rc = HXR_OK;
    UINT32              seqNo = 0;
    RTSPOptionsMessage* pMsg = NULL;
    IHXBuffer*          pBuffer = NULL;

    m_pMutex->Lock();
    m_bInitMsgSent = TRUE;

    if (!m_bSessionSucceeded)
    {
        HX_ASSERT(!m_pSession && !m_pSocket);
        m_pSession = pSession;
        m_pSocket = pSocket;
    }

    if (m_bNonRSRTP)
    {
        rc = m_pResp->HandleOptionsResponse(HXR_OK, NULL);
    }
    else
    {
        pMsg = new RTSPOptionsMessage(FALSE);

        // construct "rtsp://host:port" (important: no trailing slash)
        HX_ASSERT(!m_hostName.IsEmpty());
        HXURLRep originalURL(m_url);

        HXURLRep url(HXURLRep::TYPE_NETPATH, "rtsp", "", /* no user info */
                     m_hostName, m_hostPort,
                     originalURL.Path(),
                     originalURL.Query(),
                     originalURL.Fragment());

        m_url = url.GetSchemeAuthorityString();

        pMsg->setURL(m_url);

        pMsg->addHeader("User-Agent", m_versionString);

        // inquery ABD server support if ABD is enabled and ABD hasn't been
        // attempted for this session
        if (m_pSession->amIDoingABD(this) &&
            (ABD_STATE_INIT == m_pSession->getABDState()))
        {
            m_pSession->setABDState(ABD_STATE_INQUERY);
            pMsg->addHeader("Supported", "ABD-1.0");
        }

        /*
         * XXXSMP m_pSessionHeaders can include a "Require" tag from rmacore.
         * Yes this is ugly, and needs fixing when we want to send more options
         */
        if (m_pSessionHeaders &&
            HXR_OK == m_pSessionHeaders->GetPropertyCString("ConnectionlessControl",
                                                            pBuffer))
        {
            m_bConnectionlessControl =
                (strcasecmp((const char*)pBuffer->GetBuffer(), "on") == 0) ?
                TRUE : FALSE;
            pBuffer->Release();
        }

        addRFC822Headers(pMsg, m_pSessionHeaders);

        seqNo = m_pSession->getNextSeqNo(this);

        rc = sendRequest(pMsg, seqNo);

        if ((HXR_OK == rc) && m_pPipelinedDescLogic)
        {
            rc = m_pPipelinedDescLogic->OnOptionsSent(pMsg);
        }
    }


    if (!m_bSessionSucceeded)
    {
        m_pSession = NULL;
        m_pSocket = NULL;
    }

    m_pMutex->Unlock();

    return rc;
}

HX_RESULT
RTSPClientProtocol::getStreamDescriptionMimeType(char*& pMimeType)
{
    HX_RESULT rc = HXR_OK;

    IHXPlugin2Handler* pPlugin2Handler = NULL;

    // we have to have either an IHXPluginHandler or an IHXPlugin2Handler

    m_pContext->QueryInterface(IID_IHXPlugin2Handler,
                               (void**)&pPlugin2Handler);

    if(pPlugin2Handler)
    {
        UINT32 unIndex;
        if (HXR_OK == pPlugin2Handler->FindIndexUsingStrings((char*)PLUGIN_CLASS,
                                                             (char*)PLUGIN_STREAM_DESC_TYPE,
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             unIndex))
        {
            IHXValues* pValues;
            pPlugin2Handler->GetPluginInfo(unIndex, pValues);
            IHXBuffer* pBuffer;
            pValues->GetPropertyCString(PLUGIN_STREAMDESCRIPTION, pBuffer);
            pValues->Release();
            const char* pTemp = (const char*)pBuffer->GetBuffer();
            pMimeType = new_string(pTemp);
            pBuffer->Release();
        }
        else
        {
            rc = HXR_FAIL;
        }
        HX_RELEASE(pPlugin2Handler);
    }
#if defined(HELIX_FEATURE_SERVER)
    else
    {
        // ok we do not have an IHXPlugin2Handler (we must be in the server)
        // so get the PluginHandler
        IHXStreamDescription* pSD = NULL;
        PluginHandler* pPHandler = NULL;
        m_pContext->QueryInterface(IID_IHXPluginHandler,
                                   (void**)&pPHandler);

        if(pPHandler)
        {
            PluginHandler::StreamDescription* pSDHandler;

            pSDHandler = pPHandler->m_stream_description_handler;
            UINT32 ulNumPlugins = pSDHandler->GetNumOfPlugins();
            if(ulNumPlugins > 0)
            {
                // get the first one...
                char* ppszDllPath = 0;
                char* ppszDescription = 0;
                char* ppszCopyright = 0;
                char* ppszMoreInfo = 0;
                HXBOOL  pbMultiple = FALSE;
                char* ppszMimeType = 0;
                pSDHandler->GetPluginInfo(0, &ppszDllPath, &ppszDescription,
                                          &ppszCopyright, &ppszMoreInfo, &pbMultiple,
                                          &ppszMimeType);
                pMimeType = new_string(ppszMimeType);
                rc = HXR_OK;
            }
            else
            {
                rc = HXR_FAIL;
            }
            pPHandler->Release();
        }
        else
        {
            rc = HXR_FAIL;
        }
    }
#endif /* HELIX_FEATURE_SERVER */

    return rc;
}
IHXStreamDescription*
RTSPClientProtocol::getStreamDescriptionInstance(const char* pMimeType)
{
    IHXStreamDescription* pSD =
        HXStreamDescriptionHelper::GetInstance(m_pContext, pMimeType);

#if defined(HELIX_FEATURE_SERVER)
    if (!pSD)
    {
        // we don't have a plugin2handler ... we must be in the
        // server ... ask for a plugin handler

        PluginHandler* pPHandler = 0;
        m_pContext->QueryInterface(IID_IHXPluginHandler, (void**)&pPHandler);

        const char* pFindMimeType = pMimeType;
        if(pPHandler)
        {
            PluginHandler::StreamDescription* pSDHandler;
            PluginHandler::Errors             pluginResult;
            PluginHandler::Plugin*            pPlugin;

            pSDHandler = pPHandler->m_stream_description_handler;
            pluginResult = pSDHandler->Find(pFindMimeType, pPlugin);
            if(PluginHandler::NO_ERRORS == pluginResult)
            {
                IUnknown* pInstance = 0;
                pPlugin->GetInstance(&pInstance);
                if(pInstance)
                {
                    HX_RESULT rc;
                    rc = pInstance->QueryInterface(IID_IHXStreamDescription,
                                                   (void**)&pSD);
                    if(rc == HXR_OK)
                    {
                        IHXPlugin* pSDPlugin = 0;
                        rc = pSD->QueryInterface(IID_IHXPlugin,
                                                 (void**)&pSDPlugin);
                        if(rc == HXR_OK)
                        {
                            pSDPlugin->InitPlugin(m_pContext);
                            pSDPlugin->Release();
                        }
                    }
                    pInstance->Release();
                }
                pPlugin->ReleaseInstance();
            }
            pPHandler->Release();
        }
    }
#endif

    return pSD;
}

void
RTSPClientProtocol::reset()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::reset()", this);

    m_bInitDone = FALSE;
    m_ulServerVersion = 0;


    HX_ASSERT(!m_pSessionManager); // Done() cleans this

    RemoveSDPHeaders();

    clearStreamInfoList();
    clearTransportRequestList();
    clearUDPResponseHelperList();
    ClearPreSetupResponseQueueMap();
    clearRequestPendingReplyList(m_ResumeRequestPendingReplyList);
    clearRequestPendingReplyList(m_PlayRequestPendingReplyList);

    HX_RELEASE(m_pConnectAddr);
    HX_RELEASE(m_pPeerAddr);

    // these maps store weak references
    HX_DELETE(m_pTransportStreamMap);
    HX_DELETE(m_pTransportPortMap);
    HX_DELETE(m_pTransportMPortMap);
    HX_DELETE(m_pTransportChannelMap);

    if (m_FWPortToBeClosed && m_pFWCtlMgr)
    {
        m_pFWCtlMgr->ClosePort(m_FWPortToBeClosed, HX_NET_FW_IP_PROTOCOL_UDP);
        m_pFWCtlMgr->ClosePort((UINT32)(m_FWPortToBeClosed + 1), HX_NET_FW_IP_PROTOCOL_UDP);
        m_FWPortToBeClosed = 0;
    }

    if (m_pControlToStreamNoMap)
    {
        CHXMapStringToOb::Iterator i;
        for(i=m_pControlToStreamNoMap->Begin();i!=m_pControlToStreamNoMap->End();++i)
        {
            UINT32* pul = (UINT32*)(*i);
            delete pul;
        }
        m_pControlToStreamNoMap->RemoveAll();
        HX_DELETE(m_pControlToStreamNoMap);
    }

    CHXSimpleList::Iterator i;
    for(i=m_transportRequestList.Begin();i!=m_transportRequestList.End();++i)
    {
        RTSPTransportRequest* pRequest = (RTSPTransportRequest*)(*i);
        delete pRequest;
    }
    m_transportRequestList.RemoveAll();

    m_activeTransportList.RemoveAll();

    HX_RELEASE(m_pSetupRequestHeader);
    if( m_pResolver )
    {
        m_pResolver->Close();
        HX_RELEASE(m_pResolver);
    }

    HX_RELEASE(m_pResp);
    HX_RELEASE(m_pConnectionlessControl);
    HX_RELEASE(m_pConnectionCheckCallback);
    HX_RELEASE(m_pContext);

    if (m_uConnectionCheckCallbackHandle)
    {
        m_pScheduler->Remove(m_uConnectionCheckCallbackHandle);
        m_uConnectionCheckCallbackHandle = 0;
    }

    HX_RELEASE(m_pKeepAliveCallback);
    HX_DELETE(m_pSessionTimeout);

    if (m_pSrcBufStats)
    {
        m_pSrcBufStats->Close();

        HX_RELEASE(m_pSrcBufStats);
    }

    clearSocketStreamMap(m_pUDPSocketStreamMap);
    clearSocketStreamMap(m_pRTCPSocketStreamMap);

    HX_RELEASE(m_pUAProfDiff);
    HX_RELEASE(m_pUAProfURI);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pFWCtlMgr);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pErrMsg);
    HX_RELEASE(m_pFileHeader);
    HX_RELEASE(m_pSessionHeaders);
    HX_RELEASE(m_pResponseHeaders);
    HX_RELEASE(m_pCloakValues);
    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pScheduler);

    HX_DELETE(m_pPipelinedDescLogic);

    if (m_pRateAdaptInfo)
    {
        m_pRateAdaptInfo->Close();
        HX_DELETE(m_pRateAdaptInfo);
    }
}

void
RTSPClientProtocol::clearStreamInfoList()
{
    CHXSimpleList::Iterator i;
    for(i=m_streamInfoList.Begin();
        i!=m_streamInfoList.End();
        ++i)
    {
        RTSPStreamInfo* pInfo = (RTSPStreamInfo*)(*i);
        delete pInfo;
    }
    m_streamInfoList.RemoveAll();
}

void
RTSPClientProtocol::clearTransportRequestList()
{
    CHXSimpleList::Iterator i;

    for (i = m_transportRequestList.Begin(); i != m_transportRequestList.End();         ++i)
    {
        RTSPTransportRequest* pRequest = (RTSPTransportRequest*)(*i);
        delete pRequest;
    }
    m_transportRequestList.RemoveAll();
}

void
RTSPClientProtocol::clearUDPResponseHelperList()
{
    CHXSimpleList::Iterator i;
    for(i=m_UDPResponseHelperList.Begin();
        i!=m_UDPResponseHelperList.End();
        ++i)
    {
        UDPResponseHelper* pHelper = (UDPResponseHelper*)(*i);
        HX_RELEASE(pHelper);
    }
    m_UDPResponseHelperList.RemoveAll();
}

void
RTSPClientProtocol::clearRequestPendingReplyList(CHXSimpleList& requestPendingReplyList)
{
    UINT32* pDeadCmdSeqNum;

    while (!requestPendingReplyList.IsEmpty())
    {
	pDeadCmdSeqNum = (UINT32*) requestPendingReplyList.RemoveHead();
	delete pDeadCmdSeqNum;
    }
}

HXBOOL
RTSPClientProtocol::trimRequestPendingReplyList(CHXSimpleList& requestPendingReplyList, 
						UINT32 ulReplySeqNum,
						UINT32* pNumRepliesTrimmed)
{
    UINT32* pHeadCmdSeqNum;
    HXBOOL bIsResumeReply = FALSE;
    UINT32 ulNumRepliesTrimmed = 0;


    while (!requestPendingReplyList.IsEmpty())
    {
	pHeadCmdSeqNum = (UINT32*) requestPendingReplyList.GetHead();
	if (((INT32) (ulReplySeqNum - (*pHeadCmdSeqNum))) >= 0)
	{
	    if ((*pHeadCmdSeqNum) == ulReplySeqNum)
	    {
		bIsResumeReply = TRUE;
	    }

	    requestPendingReplyList.RemoveHead();
	    ulNumRepliesTrimmed++;
	    delete pHeadCmdSeqNum;
	}
	else
	{
	    break;
	}
    }

    if (pNumRepliesTrimmed)
    {
	*pNumRepliesTrimmed = ulNumRepliesTrimmed;
    }

    return bIsResumeReply;
}

HX_RESULT
RTSPClientProtocol::augmentRequestPendingReplyList(CHXSimpleList& requestPendingReplyList,
						   UINT32 ulReplySeqNum)
{
    UINT32* pTailCmdSeqNum = new UINT32;
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    if (pTailCmdSeqNum)
    {
	retVal = HXR_OK;
	*pTailCmdSeqNum = ulReplySeqNum;
	requestPendingReplyList.AddTail((void*) pTailCmdSeqNum);
    }

    return retVal;
}

void
RTSPClientProtocol::clearSocketStreamMap(CHXMapLongToObj*& pSocketStreamMap)
{
    if(pSocketStreamMap)
    {
        CHXMapLongToObj::Iterator i;
        for(i=pSocketStreamMap->Begin();
            i!=pSocketStreamMap->End();++i)
        {
            IHXSocket* pSocket = (IHXSocket*)(*i);
            pSocket->Close();
            pSocket->Release();
        }
        HX_DELETE(pSocketStreamMap);
    }
}

RTSPTransportRequest*
RTSPClientProtocol::getTransportRequest(MIMEHeaderValue* pValue)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::getTransportRequest()", this);
    RTSPTransportRequest* pTransportRequest = 0;
    if(pValue)
    {
        UINT16 requestPort = 0;
        UINT16 resendPort = 0;
        INT8 tcpInterleave = 0;

        MIMEParameter* pParam = pValue->getFirstParameter();
        char pTransValue[256]; /* Flawfinder: ignore */
        strncpy(pTransValue, pParam->m_attribute,255);
        pTransValue[255] = '\0';

        pParam = pValue->getNextParameter();
        while(pParam)
        {
            if(0 == pParam->m_attribute.CompareNoCase("client_port"))
            {
                // value range in rtp
                const char* portString = (const char*)pParam->m_value;
                char* pFirstValue = (char*)strchr(portString, '-');
                if(pFirstValue)
                {
                    *pFirstValue = 0;  // get rid of second port value
                }
                requestPort = (UINT16)strtol(portString, 0, 10);

                HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::getTransportRequest(): client_port = %s", this, portString);

            }
            else if(0 == pParam->m_attribute.CompareNoCase("server_port"))
            {
                // value range in rtp
                const char* portString = (const char*)pParam->m_value;
                char* pFirstValue = (char*)strchr(portString, '-');
                if(pFirstValue)
                {
                    *pFirstValue = 0;  // get rid of second port value
                }
                resendPort = (UINT16)strtol(portString, 0, 10);

                HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::getTransportRequest(): server_port = %s", this, portString);
            }
            else if(0 == pParam->m_attribute.CompareNoCase("source"))
            {
                HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::getTransportRequest(): source = %s", this, (const char*) pParam->m_value);
                HX_RELEASE(m_pPeerAddr);
                HXSockUtil::CreateAddr(m_pNetSvc, HX_SOCK_FAMILY_INANY, (const char*) pParam->m_value, 0, m_pPeerAddr);
                HX_ASSERT(m_pPeerAddr);
            }
            else if(0 == pParam->m_attribute.CompareNoCase("interleaved"))
            {
                // it could be a range in RTP (i.e. RTP-RTCP)
                const char* channelString = (const char*)pParam->m_value;
                char* pFirstValue = (char*)strchr(channelString, '-');
                if (pFirstValue)
                {
                    //get rid of second channel value since the second value is
                    //always one higher than the first
                    *pFirstValue = 0;
                }

                tcpInterleave = (INT8)strtol(channelString, 0, 10);
            }
            else if(0 == pParam->m_attribute.CompareNoCase("unicast"))
            {
                SafeStrCat(pTransValue, ";unicast", 256);
            }
            else if(0 == pParam->m_attribute.CompareNoCase("multicast"))
            {
                SafeStrCat(pTransValue, ";multicast", 256);
            }
            pParam = pValue->getNextParameter();
        }

        RTSPTransportTypeEnum transportType =
            RTSPTransportMimeMapper::getTransportType(pTransValue);

        CHXSimpleList::Iterator i;
        for(i=m_transportRequestList.Begin();
            i!=m_transportRequestList.End();++i)
        {
            RTSPTransportRequest* pRequest = (RTSPTransportRequest*)(*i);
            if(pRequest->m_lTransportType == transportType)
            {
                pRequest->m_sPort = requestPort;
                pRequest->m_sResendPort = resendPort;
                pRequest->m_tcpInterleave = tcpInterleave;

                pTransportRequest = pRequest;
            }
            else
            {
                pRequest->m_bDelete = TRUE;
            }
        }
    }

    // remove transport requests marked as delete
    LISTPOSITION pos = m_transportRequestList.GetTailPosition();
    while(pos)
    {
        RTSPTransportRequest* pRequest =
            (RTSPTransportRequest*)m_transportRequestList.GetAt(pos);
        if(pRequest->m_bDelete)
        {
            delete pRequest;
            pos = m_transportRequestList.RemoveAt(pos);
        }
        else
        {
            m_transportRequestList.GetPrev(pos);
        }
    }

    return pTransportRequest;
}

UINT32
RTSPClientProtocol::getSSRCFromTransportHeader(MIMEHeaderValue* pValue)
{
    UINT32 ssrc = 0;
    if(pValue)
    {
        MIMEParameter* pParam = pValue->getFirstParameter();

        while(pParam)
        {
            if(0 == pParam->m_attribute.CompareNoCase("ssrc"))
            {
                const char* ssrcString = (const char*)pParam->m_value;
                ssrc = strtoul(ssrcString, 0, 16);
            }
            pParam = pValue->getNextParameter();
        }
    }

    return ssrc;
}

/*
 * IHXThinnableSource methods.
 */

/************************************************************************
 *      Method:
 *          IHXThinnableSource::LimitBandwidthByDropping
 *      Purpose:
 *
 *          Implemented by protocols that allow infinite thinnability through
 *          LimitBandwidthByDropping
 */

STDMETHODIMP
RTSPClientProtocol::LimitBandwidthByDropping(UINT32 ulStreamNo,
                                             UINT32 ulBandwidthLimit)
{
    if (!m_pIsMethodSupported[SET_PARAM])
    {
        return HXR_OK;
    }
    m_pMutex->Lock();

    RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
    pMsg->setURL(getAggControlURL());

    char tmp[128];
    SafeSprintf(tmp, 128, "stream=%d;LimitBandwidthByDropping=%d", ulStreamNo,
                ulBandwidthLimit);
    pMsg->addHeader("FrameControl", tmp);
    if (!m_sessionID.IsEmpty())
    {
        pMsg->addHeader("Session", m_sessionID);
    }
    UINT32 seqNo = m_pSession->getNextSeqNo(this);
    HX_RESULT hr = sendRequest(pMsg, seqNo);
    m_pMutex->Unlock();
    return hr;
}

STDMETHODIMP
RTSPClientProtocol::SetDeliveryBandwidth(UINT32 ulBandwidth, UINT32 ulMsBackOff)
{
    HX_RESULT hr = HXR_OK;
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::SetDeliveryBandwidth(): bw = %lu", this, ulBandwidth);
    if (!m_pIsMethodSupported[SET_PARAM] || !m_pSession)
    {
        return HXR_OK;
    }
    m_pMutex->Lock();

   if(m_ulLastBWSent != ulBandwidth)
   {
        RTSPSetParamMessage* pMsg = new RTSPSetParamMessage;
        pMsg->setURL(getAggControlURL());

        char tmp[64];
        SafeSprintf(tmp, 64, "Bandwidth=%d;BackOff=%d", ulBandwidth, ulMsBackOff);
        pMsg->addHeader("SetDeliveryBandwidth", tmp); 
        if (!m_sessionID.IsEmpty())
        {
        	pMsg->addHeader("Session", m_sessionID);
        }
        UINT32 seqNo = m_pSession->getNextSeqNo(this);
        hr = sendRequest(pMsg, seqNo);
        m_ulLastBWSent = ulBandwidth;
   }
		
    m_pMutex->Unlock();
    return hr;
}

HX_RESULT
RTSPClientProtocol::closeSocket()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::closeSocket()", this);
    m_pSocket = NULL; // weak ref

    return m_pSession->closeSocket();
}

HX_RESULT
RTSPClientProtocol::reopenSocket()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::reopenSocket()", this);
    m_pMutex->Lock();
    m_pSession->m_bReopenSocket = TRUE;
    HX_RESULT hr = m_pSession->reopenSocket(this);
    m_pMutex->Unlock();
    return hr;
}

HX_RESULT
RTSPClientProtocol::ReopenSocketDone(HX_RESULT status)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::ReopenSocketDone(): %08x", this, status);
    m_pMutex->Lock();

    HX_RESULT hresult = HXR_OK;

    m_pSession->m_bReopenSocket = FALSE;

    if (HXR_OK != status)
    {
        hresult = m_pResp->HandleProtocolError(status);
        goto exit;
    }

    HX_ASSERT(m_pControlBuffer);

    if (!m_pControlBuffer)
    {
        hresult = HXR_FAIL;
        goto exit;
    }

    hresult = sendControlMessage(m_pControlBuffer);

    m_pControlBuffer->Release();
    m_pControlBuffer = 0;

  exit:
    m_pMutex->Unlock();
    return hresult;
}

void
RTSPClientProtocol::DoConnectionCheck()
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::DoConnectionCheck()", this);

    m_uConnectionCheckCallbackHandle = 0;

    HX_ASSERT(m_pConnectionlessControl);

    if (!m_bConnectionAlive)
    {
        HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::DoConnectionCheck(): HXR_SERVER_TIMEOUT", this);
        m_pConnectionlessControl->ConnectionCheckFailed(HXR_SERVER_TIMEOUT);
        return;
    }

    m_bConnectionAlive = FALSE;

    m_uConnectionCheckCallbackHandle =
        m_pScheduler->RelativeEnter(m_pConnectionCheckCallback,
                                    m_uConnectionTimeout * 1000);
}

void
RTSPClientProtocol::mapTransportChannel(RTSPTransport* pTran, UINT16 nChannel)
{
    if (!m_pTransportChannelMap)
    {
        m_pTransportChannelMap = new CHXMapLongToObj();
    }
    (*m_pTransportChannelMap)[nChannel] = pTran;
}

/* Interop */
void
RTSPClientProtocol::mapControlToStreamNo(const char* pControl, UINT16 uStreamNo)
{
    if (!m_pControlToStreamNoMap)
    {
        m_pControlToStreamNoMap = new CHXMapStringToOb();
    }

    UINT16* pu = new UINT16;
    *pu = uStreamNo;
    (*m_pControlToStreamNoMap)[pControl] = pu;
}

HXBOOL
RTSPClientProtocol::getStreamNoFromControl(const char* pControl, REF(UINT16) uStreamNo)
{
    // don't call if there is no map
    HX_ASSERT(m_pControlToStreamNoMap);

    UINT16* ul = NULL;
    if (m_pControlToStreamNoMap->Lookup(pControl, (void*&)ul))
    {
        uStreamNo = (UINT16)*ul;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

CHXString
RTSPClientProtocol::getSetupRequestURL(RTSPStreamInfo* pStreamInfo) const
{
    CHXString ret;

    HX_ASSERT(pStreamInfo);

    if (pStreamInfo)
    {
        if (pStreamInfo->m_streamControl.Find("rtsp:") != -1)
        {
            // absolute url....just use this!
            ret = pStreamInfo->m_streamControl;
        }
        else if (!m_contentBase.IsEmpty())
        {
            // we have Content-Base RTSP header
            ret = m_contentBase + pStreamInfo->m_streamControl;
        }
        else if(!m_headerControl.IsEmpty())
        {
            // this is a=control in session description...
            ret = m_headerControl;
            if(!pStreamInfo->m_streamControl.IsEmpty())
            {
                // we append a '/' only if it isn't already present as a double '/' can cause a problem
                if(ret.GetAt(ret.GetLength() - 1) != '/')
                {
                    ret += "/";
                }
                ret += pStreamInfo->m_streamControl;
            }
        }
        else
        {
            ret = m_url + "/" + pStreamInfo->m_streamControl;
        }
    }

    return ret;
}

CHXString
RTSPClientProtocol::getAggControlURL() const
{
    CHXString ret;

    if (!m_headerControl.IsEmpty())
    {
        // we have Control URI RTSP header
        // a=control:* is handled in ParseSDP() 
        ret = m_headerControl;
    }
    else if (!m_contentBase.IsEmpty())
    {
        // If "Content-Base" field is not present in DESCRIBE response, 
        // then m_contentBase will contain
        // value from "Content-Location", see handleDescribeResponse()
        ret = m_contentBase;
    }
    else
    {
        ret = m_url;
    }

    // If this is a NetCache proxy, we need to strip the trailing '/'. For some
    // reason they can't handle it, as they should per the spec.
    if( m_bTrimTrailingSlashes && '/' == ret[ret.GetLength()-1] )
    {
        ret = ret.Left(ret.GetLength()-1);
    }

    return ret;
}

RTSPStreamInfo*
RTSPClientProtocol::getStreamInfoFromSetupRequestURL(const char* pUrl)
{
    HX_ASSERT(pUrl);

    // we've created a Setup request url above.  this is a reverse
    char* setupURL = NULL;
    HXBOOL bFoundIt = FALSE;
    RTSPStreamInfo* pStreamInfo = NULL;
    CHXSimpleList::Iterator i;

    // If we do not have control URL,
    // we can still proceed as long there is only one stream
    if (pUrl == NULL)
    {
        if (m_streamInfoList.GetCount() == 1)
        {
            return (RTSPStreamInfo*) m_streamInfoList.GetHead();
        }

        return NULL;
    }

    for(i=m_streamInfoList.Begin();i!=m_streamInfoList.End();++i)
    {
        pStreamInfo = (RTSPStreamInfo*)(*i);
        if(pStreamInfo->m_streamControl == pUrl)
        {
            // well, this is my lucky day
            bFoundIt = TRUE;
            break;
        }

        // - get stream identifier by parsing URL
        char* pStream = (char*)strrchr(pUrl, '/');
        if(pStream)
        {
            ++pStream;  // pass '/'
            if(pStreamInfo->m_streamControl == (const char*)pStream)
            {
                bFoundIt = TRUE;
                break;
            }
        }

        if (!m_contentBase.IsEmpty())
        {
            if ((m_contentBase + pStreamInfo->m_streamControl) == pUrl)
            {
                // not so bad...
                bFoundIt = TRUE;
                break;
            }
        }

        if (!m_headerControl.IsEmpty())
        {
            if (m_headerControl == pStreamInfo->m_streamControl)
            {
                // phew....
                bFoundIt = TRUE;
                break;
            }
        }

        // Compare each portion of the url w/ our control
        pStream = (char*)strchr(pUrl, '/');
        while (pStream)
        {
            ++pStream;  // pass '/'
            if(pStreamInfo->m_streamControl == (const char*)pStream)
            {
                bFoundIt = TRUE;
                break;
            }

            pStream = (char*)strchr(pStream, '/');
        }

        if (bFoundIt)
        {
            break;
        }

        // following the recommendation posted at:
        // http://www1.ietf.org/mail-archive/working-groups/mmusic/current/msg01245.html
        int lenURL = (int)(m_url.GetLength() + strlen(pUrl) + 2);
        setupURL = new char[lenURL];
        SafeSprintf(setupURL, (UINT32)lenURL, "%s/%s", (const char*)m_url, pUrl);
        if (pStreamInfo->m_streamControl == (const char*)setupURL)
        {
            delete[] setupURL;
            bFoundIt = TRUE;
            break;
        }
        delete[] setupURL;
    }

    if (bFoundIt)
    {
        return pStreamInfo;
    }
    else
    {
        HX_ASSERT(!"streaminfo not found from setup request url");
        return NULL;
    }
}

STDMETHODIMP
RTSPClientProtocol::InitPacketFilter(RawPacketFilter* pFilter)
{
    m_pPacketFilter = pFilter;
    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::SetUseRTPFlag(HXBOOL bUseRTP)
{
    if( m_pRateAdaptInfo )
    {
        m_pRateAdaptInfo->SetUseRTPFlag(bUseRTP);
    }
    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::LeavePrefetch(void)
{
    m_bPrefetch = FALSE;
    SendMsgToTransport(LEAVE_PREFETCH);

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::EnterFastStart(void)
{
    m_bFastStart = TRUE;
    SendMsgToTransport(ENTER_FASTSTART);

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::LeaveFastStart(void)
{
    m_bFastStart = FALSE;
    SendMsgToTransport(LEAVE_FASTSTART);

    return HXR_OK;
}

STDMETHODIMP
RTSPClientProtocol::InitCloak(UINT16*       pCloakPorts,
                              UINT8         nCloakPorts,
                              IHXValues*    pValues)
{
    m_pCloakPorts = pCloakPorts;
    m_nCloakPorts = nCloakPorts;

    if (pValues)
    {
        m_pCloakValues = pValues;
        m_pCloakValues->AddRef();
    }

    return HXR_OK;
}

UINT16
RTSPClientProtocol::GetCloakPortSucceeded(void)
{
    return m_cloakPort;
}

HX_RESULT
RTSPClientProtocol::RetrieveChallenge(RTSPResponseMessage* pMessage)
{
    return HXR_FAILED;
}

RTSPTransport* RTSPClientProtocol::GetTransport(UINT16 idxStream)
{
    RTSPTransport* pTrans = 0;
    if (m_pTransportStreamMap)
    {
        m_pTransportStreamMap->Lookup(idxStream, (void*&)pTrans);
    }
    return pTrans;
}

STDMETHODIMP
RTSPClientProtocol::SetStatistics(UINT16        uStreamNumber,
                                  STREAM_STATS* pStats)
{
    HX_RESULT   rc = HXR_OK;

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    RTSPTransport* pTrans = GetTransport(uStreamNumber);
    //HX_ASSERT(pTrans);
    if (pTrans)
    {
        rc = pTrans->SetStatistics(uStreamNumber, pStats);
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return rc;
}

HX_RESULT
RTSPClientProtocol::extractRealmInformation(RTSPResponseMessage* pMsg)
{
#if defined(HELIX_FEATURE_AUTHENTICATION)
    HX_RESULT retVal = HXR_OK;

    // this is called from several places where authentication errors
    // are likely to occur. It then pulls out the realm for future reference.
    if (m_pRegistry)
    {
        CHXString authString;

        authString = pMsg->getHeaderValue("Proxy-Authenticate");

        if (m_bUseProxy && !authString.IsEmpty())
        {
            IHXBuffer* pBuffer = NULL;
            retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);

            if (SUCCEEDED(retVal))
            {
                HX_ASSERT(pMsg->errorCodeAsUINT32() == 407);
                retVal = pBuffer->Set((const unsigned char*)(const char*)authString,
                                      authString.GetLength()+1);
                UINT32 regid = m_pRegistry->GetId("proxy-authentication.rtsp.realm.recent");
                if( retVal == HXR_OUTOFMEMORY )
                {
                    HX_RELEASE(pBuffer);
                    return retVal;
                }
                if (!regid)
                {
                    m_pRegistry->AddStr("proxy-authentication.rtsp.realm.recent", pBuffer);
                }
                else
                {
                    m_pRegistry->SetStrByName("proxy-authentication.rtsp.realm.recent", pBuffer);
                }

                HX_RELEASE(pBuffer);
            }
        }

        authString = pMsg->getHeaderValue("WWW-Authenticate");

        if (!authString.IsEmpty())
        {
            IHXBuffer* pBuffer = NULL;
            retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);

            if (SUCCEEDED(retVal))
            {
                HX_ASSERT(pMsg->errorCodeAsUINT32() == 401);
                retVal = pBuffer->Set((const unsigned char*)(const char*)authString,
                                      authString.GetLength()+1);
                if( retVal == HXR_OUTOFMEMORY )
                {
                    HX_RELEASE(pBuffer);
                    return retVal;
                }
                UINT32 regid = m_pRegistry->GetId("authentication.rtsp.realm.recent");
                if (!regid)
                {
                    m_pRegistry->AddStr("authentication.rtsp.realm.recent", pBuffer);
                }
                else
                {
                    m_pRegistry->SetStrByName("authentication.rtsp.realm.recent", pBuffer);
                }

                HX_RELEASE(pBuffer);
            }
        }
    }
    return retVal;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

HX_RESULT
RTSPClientProtocol::extractExistingAuthorizationInformation(IHXValues* pIHXValuesRequestHeaders)
{
#if defined(HELIX_FEATURE_AUTHENTICATION)
    if (pIHXValuesRequestHeaders)
    {
        const char* pName = NULL;
        IHXBuffer* pValue = NULL;

        HX_RESULT result = pIHXValuesRequestHeaders->GetFirstPropertyCString(pName, pValue);

        while (SUCCEEDED(result))
        {
            // check for proxy and www authentication stuff separately because
            // it's plausible that one request will have both of these.

            if (m_bUseProxy && !strcasecmp(pName, "Proxy-Authorization"))
            {
                HX_RESULT retVal = HXR_OK;

                if (m_pRegistry)
                {
                    IHXBuffer* pBuffer = NULL;
                    retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
                    UINT32 regid = 0;

                    if (SUCCEEDED(retVal))
                    {
                        IHXBuffer* pHeaderBuffer = NULL;

                        CHXString key;
                        CHXString recentRealmInfo = "";

                        key = "proxy-authentication.rtsp:";
                        retVal = m_pRegistry->GetStrByName("proxy-authentication.rtsp.realm.recent",
                                                           pHeaderBuffer);

                        if (SUCCEEDED(retVal))
                        {
                            HX_ASSERT(pHeaderBuffer);
                            recentRealmInfo = CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
                            HX_RELEASE(pHeaderBuffer);
                        }

                        key += "proxy-host:";
                        key += recentRealmInfo;

                        HX_ASSERT(!key.IsEmpty());
                        retVal = pBuffer->Set(pValue->GetBuffer(), pValue->GetSize());
                        if( retVal == HXR_OUTOFMEMORY )
                        {
                            HX_RELEASE(pBuffer);
                            HX_RELEASE(pHeaderBuffer);
                            return retVal;
                        }

                        regid = m_pRegistry->GetId((const char*)key);
                        if (!regid)
                        {
                            m_pRegistry->AddStr((const char*)key, pBuffer);
                        }
                        else
                        {
                            m_pRegistry->SetStrByName((const char*)key, pBuffer);
                        }

                        HX_RELEASE(pBuffer);
                        HX_RELEASE(pHeaderBuffer);
                    }
                }
            }

            if (!strcasecmp(pName, "Authorization"))
            {
                HX_RESULT retVal = HXR_OK;

                if (m_pRegistry)
                {
                    IHXBuffer* pBuffer = NULL;
                    retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
                    UINT32 regid = 0;

                    if (SUCCEEDED(retVal))
                    {
                        IHXBuffer* pHeaderBuffer = NULL;

                        CHXString key;
                        CHXString recentRealmInfo = "";

                        key = "authentication.rtsp:";
                        retVal = m_pRegistry->GetStrByName("authentication.rtsp.realm.recent",
                                                           pHeaderBuffer);

                        if (SUCCEEDED(retVal))
                        {
                            HX_ASSERT(pHeaderBuffer);
                            recentRealmInfo = CHXString((const char*)pHeaderBuffer->GetBuffer(), pHeaderBuffer->GetSize());
                            HX_RELEASE(pHeaderBuffer);
                        }

                        key += m_hostName;

                        key += ":";
                        key += recentRealmInfo;

                        HX_ASSERT(!key.IsEmpty());
                        retVal = pBuffer->Set(pValue->GetBuffer(), pValue->GetSize());
                        if( retVal == HXR_OUTOFMEMORY )
                        {
                            return retVal;
                        }

                        regid = m_pRegistry->GetId((const char*)key);
                        if (!regid)
                        {
                            m_pRegistry->AddStr((const char*)key, pBuffer);
                        }
                        else
                        {
                            m_pRegistry->SetStrByName((const char*)key, pBuffer);
                        }

                        HX_RELEASE(pBuffer);
                        HX_RELEASE(pHeaderBuffer);
                    }
                }
            }
            HX_RELEASE(pValue);
            result = pIHXValuesRequestHeaders->GetNextPropertyCString(pName, pValue);
        }
    }
    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

void
RTSPClientProtocol::appendAuthorizationHeaders(RTSPMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::appendAuthorizationHeaders()", this);

#if defined(HELIX_FEATURE_AUTHENTICATION)

    // This is currently getting called from sendPendingStreamDescription
    // although it's feasible that other rtsp messages will need authentication.
    // At this writing none of our servers or proxies do anything other than
    // DESCRIBE, so I'm omitting scattered calls to this routine so I don't
    // waste bandwidth. It's plausible that calling this from everywhere it calls
    // addRFC822Headers would be fully compliant, if a touch wasteful.
    // xxxbobclark
    
    HX_RESULT   retVal = HXR_OK;

    if (m_pRegistry)
    {
        CHXString strExistingAuthorizationHeader = pMsg->getHeaderValue("Authorization");
        CHXString strExistingProxyAuthorizationHeader = pMsg->getHeaderValue("Proxy-Authorization");

        if (strExistingAuthorizationHeader.IsEmpty())
        {
            // if it doesn't exist, see if we've remembered one we can
            // plop down here.

            CHXString key = "authentication.rtsp:";
            IHXBuffer* pFoundRealmBuffer = NULL;
            IHXBuffer* pBuffer = NULL;

            key += m_hostName;
            key += ":";

            retVal = m_pRegistry->GetStrByName("authentication.rtsp.realm.recent",
                                               pBuffer);

            if (SUCCEEDED(retVal))
            {
                key += CHXString((const char*)pBuffer->GetBuffer(), pBuffer->GetSize());

                retVal = m_pRegistry->GetStrByName((const char*)key, pFoundRealmBuffer);

                if (SUCCEEDED(retVal))
                {
                    CHXString strAuthHeader((const char*)pFoundRealmBuffer->GetBuffer(),
                                            pFoundRealmBuffer->GetSize());

                    pMsg->addHeader("Authorization", (const char*)strAuthHeader);
                }
            }
        }

        if (m_bUseProxy && strExistingProxyAuthorizationHeader.IsEmpty())
        {
            // if it doesn't exist, see if we've remembered one we can
            // plop down here.

            CHXString key = "proxy-authentication.rtsp:";
            IHXBuffer* pFoundRealmBuffer = NULL;
            IHXBuffer* pBuffer = NULL;

            key += "proxy-host:";

            retVal = m_pRegistry->GetStrByName("proxy-authentication.rtsp.realm.recent",
                                               pBuffer);

            if (SUCCEEDED(retVal))
            {
                key += CHXString((const char*)pBuffer->GetBuffer(), pBuffer->GetSize());

                retVal = m_pRegistry->GetStrByName((const char*)key, pFoundRealmBuffer);

                if (SUCCEEDED(retVal))
                {
                    CHXString strAuthHeader((const char*)pFoundRealmBuffer->GetBuffer(),
                                            pFoundRealmBuffer->GetSize());

                    pMsg->addHeader("Proxy-Authorization", (const char*)strAuthHeader);
                }
            }
        }
    }
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

HX_RESULT
RTSPClientProtocol::handleAuthentication(RTSPResponseMessage* pMsg)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleAuthentication()", this);
#if defined(HELIX_FEATURE_AUTHENTICATION)
    HX_RESULT rc = HXR_OK;

    rc = extractRealmInformation(pMsg);
    if( rc == HXR_OUTOFMEMORY )
    {
        return rc;
    }

    IHXValues* pIHXValuesResponseHeaders = NULL;

    pMsg->AsValues(pIHXValuesResponseHeaders, m_pContext);

    if(pIHXValuesResponseHeaders)
    {
        HX_RESULT retVal = HXR_OK;
        IHXBuffer* pServerHeaderBuffer = NULL;

        // Add the fake _server value that's used
        // in IHXAuthenticationManager2 implementations. xxxbobclark
        HX_ASSERT(!m_hostName.IsEmpty());
        if (!m_hostName.IsEmpty())
        {
            retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                           (void**)&pServerHeaderBuffer);
            if (SUCCEEDED(retVal))
            {
                if (pMsg->errorCodeAsUINT32() == 407 && m_proxyName.GetLength() > 0)
                {
                    rc = pServerHeaderBuffer->Set((UCHAR*)(const char*)m_proxyName, m_proxyName.GetLength()+1);
                }
                else
                {
                    rc = pServerHeaderBuffer->Set((UCHAR*)(const char*)m_hostName, m_hostName.GetLength()+1);
                }
                pIHXValuesResponseHeaders->SetPropertyCString("_server", pServerHeaderBuffer);
                HX_RELEASE(pServerHeaderBuffer);
            }
        }

        // Add the response status code to the response headers because the
        // client authentication manager needs it
        pIHXValuesResponseHeaders->SetPropertyULONG32("_statuscode", pMsg->errorCodeAsUINT32());

        rc = m_pResp->HandleWWWAuthentication
            (
                HXR_NOT_AUTHORIZED,
                pIHXValuesResponseHeaders
                );
    }
    else
    {
        rc = m_pResp->HandleWWWAuthentication
            (
                HXR_FAIL,
                NULL
                );
    }

    HX_RELEASE(pIHXValuesResponseHeaders);

    return rc;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUTHENTICATION */
}

//
// add x-wap-profile and x-wap-profile-diff headers if they exists
//
void
RTSPClientProtocol::addUAProfHeaders(IHXValues* pHeaders)
{
    if (pHeaders)
    {
        if (m_pUAProfURI && m_pUAProfURI->GetSize() > 0)
        {
            pHeaders->SetPropertyCString("x-wap-profile", m_pUAProfURI);

            if (m_pUAProfDiff && m_pUAProfDiff->GetSize() > 0)
            {
                pHeaders->SetPropertyCString("x-wap-profile-diff",
                                             m_pUAProfDiff);
            }
        }
    }
}

void
RTSPClientProtocol::addRateAdaptationHeaders(RTSPMessage* pMsg,
                                             RTSPStreamInfo* pStreamInfo)
{
    if (pMsg && pStreamInfo && m_pRateAdaptInfo)
    {
        UINT16 streamNum = pStreamInfo->m_streamNumber;
        CHXString url = getSetupRequestURL(pStreamInfo);
        IHXValues* pHdrs = NULL;

        if (HXR_OK == m_pRateAdaptInfo->CreateRateAdaptHeaders(streamNum,
                                                               url,
                                                               pHdrs))
        {
            addRFC822Headers(pMsg, pHdrs);
        }

        HX_RELEASE(pHdrs);
    }
}

void
RTSPClientProtocol::handleRateAdaptResponse(RTSPRequestMessage* pReq,
                                            RTSPResponseMessage* pResp,
                                            UINT16 streamNumber)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::handleRateAdaptResponse()", this);
    if (pReq && pResp && m_pRateAdaptInfo)
    {
        IHXValues* pReqHdrs = NULL;
        IHXValues* pRespHdrs = NULL;

        HX_RESULT res = pReq->AsValues(pReqHdrs, m_pContext);

        if (HXR_OK == res)
        {
            res = pResp->AsValues(pRespHdrs, m_pContext);
        }

        if (HXR_OK == res)
        {
            res = m_pRateAdaptInfo->OnRateAdaptResponse(streamNumber,
                                                        pReqHdrs,
                                                        pRespHdrs);
        }
        HX_RELEASE(pReqHdrs);
        HX_RELEASE(pRespHdrs);
    }
}

CHXString
RTSPClientProtocol::createSessionLinkCharHdr() const
{
    // Session link char parameter keys have the following form.
    // LinkChar-<parameterName>
    // Examples:
    //  LinkChar-GBW
    //  LinkChar-MBW
    return createLinkCharHdrFromPrefs("LinkChar", m_url);
}

CHXString
RTSPClientProtocol::createStreamLinkCharHdr(RTSPStreamInfo* pStreamInfo) const
{
    CHXString ret;

    if (pStreamInfo)
    {
        // Stream specific link char parameter keys have the following form
        // LinkChar-<streamNumber>-<parameterName>
        // Examples:
        // Stream 0 parameters:
        //  LinkChar-0-GBW
        //  LinkChar-0-MBW
        CHXString baseKey = "LinkChar-";
        baseKey.AppendULONG(pStreamInfo->m_streamNumber);

        ret = createLinkCharHdrFromPrefs(baseKey,
                                         getSetupRequestURL(pStreamInfo));
    }

    return ret;
}

CHXString
RTSPClientProtocol::createLinkCharHdrFromPrefs(const CHXString& baseKey,
                                               const CHXString& url) const
{
    CHXString ret;

    HXBOOL bLinkCharEnabled = FALSE;
    UINT32 uGuaranteedBW = 0;
    UINT32 uMaxBW = 0;
    UINT32 uMaxTranferDelay = 0;
    UINT32* pGuaranteedBW = NULL;
    UINT32* pMaxBW = NULL;
    UINT32* pMaxTranferDelay = NULL;

    // The LinkCharEnabled preference controls whether 3GPP-Link-Char
    // headers are sent.

    if ((HXR_OK == ReadPrefBOOL(m_pPreferences,
                                "LinkCharEnabled", bLinkCharEnabled)) &&
        (bLinkCharEnabled))
    {
        if (HXR_OK == ReadPrefUINT32(m_pPreferences, baseKey + "-GBW", uGuaranteedBW))
        {
            pGuaranteedBW = &uGuaranteedBW;
        }

        if (HXR_OK == ReadPrefUINT32(m_pPreferences, baseKey + "-MBW", uMaxBW))
        {
            pMaxBW = &uMaxBW;
        }

        if (HXR_OK == ReadPrefUINT32(m_pPreferences, baseKey + "-MTD", uMaxTranferDelay))
        {
            pMaxTranferDelay = &uMaxTranferDelay;
        }

        ret = createLinkCharHdr(url, pGuaranteedBW, pMaxBW, pMaxTranferDelay);
    }

    return ret;
}

CHXString
RTSPClientProtocol::createLinkCharHdr(const CHXString& url,
                                      UINT32* pGuaranteedBW,
                                      UINT32* pMaxBW,
                                      UINT32* pMaxTranferDelay) const
{
    CHXString ret;

    if (url.GetLength())
    {
        ret += "url=\"" + url + "\"";

        if (pGuaranteedBW)
        {
            ret += ";GBW=";
            ret.AppendULONG(*pGuaranteedBW);
        }

        if (pMaxBW)
        {
            ret += ";MBW=";
            ret.AppendULONG(*pMaxBW);
        }

        if (pMaxTranferDelay)
        {
            ret += ";MTD=";
            ret.AppendULONG(*pMaxTranferDelay);
        }
    }

    return ret;
}

RTSPTransportBuffer*
RTSPClientProtocol::getTransportBuffer(UINT16 uStreamNumber)
{
    RTSPTransportBuffer* pRet = NULL;

    if (m_pTransportStreamMap)
    {
        RTSPTransport* pTrans = GetTransport(uStreamNumber);
        if (pTrans)
        {
            pRet = pTrans->getTransportBuffer(uStreamNumber);
        }
    }

    return pRet;
}

RTSPStreamInfo*
RTSPClientProtocol::getStreamInfo(UINT16 uStreamNumber)
{
    CHXSimpleList::Iterator i;
    RTSPStreamInfo* pStreamInfo = NULL;
    RTSPStreamInfo* pRet = NULL;

    for(i=m_streamInfoList.Begin();i!=m_streamInfoList.End();++i)
    {
        pStreamInfo = (RTSPStreamInfo*)(*i);
        if(pStreamInfo->m_streamNumber == uStreamNumber)
        {
            pRet =  pStreamInfo;
            break;
        }
    }
    return pRet;
}


void
RTSPClientProtocol::SetBuffByteLimit(RTSPTransport* pTrans, RTSPStreamInfo* pStreamInfo)
{
    if(pStreamInfo->m_uByteLimit !=0)
    {
        UINT16 uStreamNumber = pStreamInfo->m_streamNumber;
        RTSPTransportBuffer* pTransBuf = pTrans->getTransportBuffer(uStreamNumber);

        if (pTransBuf)
        {
            pTransBuf->SetByteLimit(pStreamInfo->m_uByteLimit);
            pStreamInfo->m_uByteLimit = 0;
        }
    }
}


// ParseSDP helper
HX_RESULT RTSPClientProtocol::SetMulticastAddrHelper(RTSPStreamInfo* pInfo, const char* pszAddr, UINT16 port)
{
    IHXSockAddr* pAddr = 0;
    HX_RESULT hr = HXSockUtil::CreateAddr(m_pNetSvc,
                                          HX_SOCK_FAMILY_INANY, pszAddr, port, pAddr);
    if (SUCCEEDED(hr))
    {
        HX_ASSERT(HXSockUtil::IsMulticastAddr(pAddr));
        if (HXSockUtil::IsMulticastAddr(pAddr))
        {
            HX_ASSERT(!pInfo->m_pMulticastAddr);
            pInfo->m_pMulticastAddr = pAddr;
            pInfo->m_pMulticastAddr->AddRef();
            m_bMulticast = TRUE;
        }
        HX_RELEASE(pAddr);
    }
    return hr;
}


HX_RESULT
RTSPClientProtocol::ParseSDP(const char* pszContentType, IHXBuffer* pSDPBuffer)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::ParseSDP()", this);
    HX_RESULT   rc = HXR_OK;
    UINT16      nValues = 0;
    UINT32      ulNumStreams = 0;
    IHXValues** ppValues = NULL;
    IHXValues** ppRealHeaders = NULL;;// headers of right BW
    UINT32*     pulSubscriptionBW = NULL;

    IHXStreamDescription* pSD = getStreamDescriptionInstance(pszContentType);

    if (!pSD)
    {
        rc = HXR_FAIL;
        goto cleanup;
    }

    rc = pSD->GetValues(pSDPBuffer, nValues, ppValues);
    if(HXR_REQUEST_UPGRADE == rc)
    {
        // request upgrade...sdpplin has added itself for
        // upgradecollection
        // nothing to do...
    }
    else if (HXR_OK != rc)
    {
        // error
        HX_ASSERT(!"bad sdp file 0");
    }
    else if (nValues <= 1)
    {
        // error
        rc = HXR_FAIL;
        HX_ASSERT("!bad sdp file 1");
    }
    else
    {
        CHXString languages;
        ReadPrefCSTRING(m_pPreferences, "Language", languages);

        rc = CHXAltGroupUtil::SelectAltGroups(m_pContext, 
					      m_pSession ? m_pSession->getConnectionBW() : 0, 
					      languages, 
					      nValues, ppValues);
    }

    if (HXR_OK == rc)
    {
        // get header info
        IHXBuffer*  pControl = NULL;
        IHXBuffer*  pIPAddress = NULL;
        UINT32      ulIsSessionLive = 0;
        UINT32      ulAvgBitRate   = 0;
        UINT32      ulRtpRRBitRate = (ULONG32)-1;
        UINT32      ulRtpRSBitRate = (ULONG32)-1;
        HXBOOL        bRealMedia = FALSE;

        m_pSDPFileHeader = ppValues[0];
        m_pSDPFileHeader->AddRef();

        ppValues[0]->GetPropertyCString("Control", pControl);
        if(pControl)
        {
            if (!strcmp((const char*)pControl->GetBuffer(),"*"))
            {
                m_headerControl = m_contentBase;
            }
            else
            {
                m_headerControl = pControl->GetBuffer();
            }

            HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::ParseSDP(): header control '%s'", this, (const char*)m_headerControl);

            HX_RELEASE(pControl);
        }

        ppValues[0]->GetPropertyULONG32("LiveStream", ulIsSessionLive);
        ppValues[0]->GetPropertyULONG32("AvgBitRate", ulAvgBitRate);
        // Get session level RTP bandwidth modifiers
        ppValues[0]->GetPropertyULONG32("RtcpRRRate", ulRtpRRBitRate);
        ppValues[0]->GetPropertyULONG32("RtcpRSRate", ulRtpRSBitRate);

        if (ulIsSessionLive)
        {
            m_bIsLive = TRUE;
        }

        // get multicast address from the session description
        if (HXR_OK == ppValues[0]->GetPropertyCString("MulticastAddress", pIPAddress))
        {
            IHXSockAddr* pAddr = 0;
            HX_RESULT hr = HXR_OK;
            hr = HXSockUtil::CreateAddr(m_pNetSvc,
                                        HX_SOCK_FAMILY_INANY, (const char*)pIPAddress->GetBuffer(), 0, pAddr);

            // this is often 'any', where connection field for each stream specifies multicast address
            HX_ASSERT(HXSockUtil::IsMulticastAddr(pAddr) || HXSockUtil::IsAnyAddr(pAddr));
            if (HXSockUtil::IsMulticastAddr(pAddr))
            {
                m_bMulticast = TRUE;
                m_sdpMulticastAddr = pIPAddress->GetBuffer();
            }
            else if(HXSockUtil::IsAnyAddr(pAddr))
            {
                // we may discover multicast addresses in streams
                m_sdpMulticastAddr = pIPAddress->GetBuffer();
            }
            HX_RELEASE(pAddr);
            HX_RELEASE(pIPAddress);
        }



        /*
         * Get a number of streams in this presentation
         */

        // don't trust the "StreamCount"
        // cHeaders will represent a number of m= line in SDP
        // ulNumStreams is actual number of streams in this presentation
        // They will be different on sure stream presentation becacause we are
        // duplicating headers for each bandwidth.
        // we have to iterate over the stream headers...
        if (!m_bSDPInitiated || GetStreamCountNoTrust(&ppValues[1], (UINT16)(nValues - 1), ulNumStreams) != TRUE)
        {
            // Well, This is not a SDP file generated by pplyfobj.cpp.
            // Trust the StreamCount!
            m_pSDPFileHeader->GetPropertyULONG32("StreamCount", ulNumStreams);
            ppRealHeaders = new IHXValues*[ulNumStreams];
            for (int i = 0; i < (int)ulNumStreams; i++)
            {
                ppRealHeaders[i] = ppValues[i+1];
                ppRealHeaders[i]->AddRef();
            }
        }
#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
        else
        {
            ULONG32 ulRuleNumber = 0;

            // we should have at least one stream
            HX_ASSERT(ulNumStreams > 0 && m_bSDPInitiated);

            // set the StreamCount
            m_pSDPFileHeader->SetPropertyULONG32("StreamCount", ulNumStreams);

            /*
             * Get a BW for each stream to "subscribe" to
             */
            pulSubscriptionBW = new UINT32[ulNumStreams];
            memset(pulSubscriptionBW, 0, sizeof(UINT32) * ulNumStreams);

            if (GetSubscriptionBW(m_pSDPFileHeader,
                                  &ppValues[1],
                                  nValues - 1,
                                  pulSubscriptionBW,
                                  ulNumStreams,
                                  ulRuleNumber) != TRUE)
            {
                // this should never happen
                HX_ASSERT(FALSE);
                rc = HXR_UNEXPECTED;
                goto cleanup;
            }

            /**************************
             * Get right stream headers to pass to CPurePlaySource
             * We have figured out BW to use for each stream
             * This is expensive, but...hey, it's done only once
             * Iterate over all the stream headers AGAIN, and Get right stream headers
             * for each stream depending on subscription BW
             */
            if (GetRightHeaders(ppRealHeaders,
                                ulNumStreams,
                                &ppValues[1],
                                nValues - 1,
                                pulSubscriptionBW,
                                ulRuleNumber) != TRUE)
            {
                // this should never happen
                HX_ASSERT(FALSE);
                rc = HXR_UNEXPECTED;
                goto cleanup;
            }
        }

        if (m_bSDPInitiated)
        {
            bRealMedia = DetermineIfRMPresentation(&ppRealHeaders[0], ulNumStreams);
        }
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */

        for (UINT32 i=0;i<ulNumStreams;++i)
        {
            // reset...
            HXBOOL    bHasMarkerRule = 0;
            UINT16  markerRule = 0;
            UINT32  streamNumber = 0;
            UINT32  needReliable = 0;
            UINT32  rtpPayloadType = (ULONG32)-1;
            UINT32  sampleSize = 0;
            UINT32  sampleRate = 0;
            UINT32  RTPFactor = 0;
            UINT32  HXFactor = 0;
            UINT32  ulIsLive = ulIsSessionLive;
            UINT32  ulHasOutOfOrderTS = 0;
            UINT32  ulPort = 0;
            IHXBuffer*      pMimeType = NULL;
            IHXBuffer*      pRawRules = NULL;
            RTSPMediaType   eMediaType = RTSPMEDIA_TYPE_UNKNOWN;

            pControl = NULL;

            if (!m_pSDPStreamHeaders)
            {
                m_pSDPStreamHeaders = new CHXSimpleList();
            }

            // build header list
            ppRealHeaders[i]->AddRef();
            m_pSDPStreamHeaders->AddTail(ppRealHeaders[i]);

            // determine stream type
            ppRealHeaders[i]->GetPropertyCString("MimeType", pMimeType);
            if (pMimeType)
            {
                eMediaType = SDPMapMimeToMediaType((const char*) pMimeType->GetBuffer());
                HX_RELEASE(pMimeType);
            }

#if defined(HELIX_FEATURE_ASM)
            // deal with marker rule...
            ppRealHeaders[i]->GetPropertyCString("ASMRuleBook", pRawRules);
            if (pRawRules)
            {
                ASMRuleBook* pRuleBook = new ASMRuleBook(m_pContext, (const char*)pRawRules->GetBuffer());

                IHXValues* pRuleProps = NULL;
                IHXBuffer* pBuffer = NULL;

                for(UINT16 nRule=0; nRule < pRuleBook->GetNumRules(); ++nRule)
                {
                    pRuleProps = NULL;
                    pBuffer = NULL;

                    pRuleBook->GetProperties(nRule, pRuleProps);
                    pRuleProps->GetPropertyCString("marker", pBuffer);

                    if(pBuffer)
                    {
                        int marker = atoi((const char*)pBuffer->GetBuffer());
                        if (1 == marker)
                        {
                            /* we don't allow more than one marker rule */
                            markerRule = (UINT16)nRule;
                            bHasMarkerRule = TRUE;

                            pBuffer->Release();
                            pRuleProps->Release();
                            break;
                        }
                        HX_RELEASE(pBuffer);
                    }
                    HX_RELEASE(pRuleProps);
                }

                HX_DELETE(pRuleBook);
                HX_RELEASE(pRawRules);
            }
#endif /* HELIX_FEATURE_ASM */

            // build stream info list
            RTSPStreamInfo* pInfo = new RTSPStreamInfo;
            ppRealHeaders[i]->GetPropertyULONG32("StreamNumber", streamNumber);
            ppRealHeaders[i]->GetPropertyULONG32("NeedReliablePackets", needReliable);
            ppRealHeaders[i]->GetPropertyULONG32("SamplesPerSecond", sampleRate);
            ppRealHeaders[i]->GetPropertyULONG32("BitsPerSample", sampleSize);
            ppRealHeaders[i]->GetPropertyCString("Control", pControl);
            ppRealHeaders[i]->GetPropertyULONG32("RTPPayloadType", rtpPayloadType);
            ppRealHeaders[i]->GetPropertyULONG32("RTPTimestampConversionFactor", RTPFactor);
            ppRealHeaders[i]->GetPropertyULONG32("HXTimestampConversionFactor", HXFactor);
            ppRealHeaders[i]->GetPropertyULONG32("LiveStream", ulIsLive);
            ppRealHeaders[i]->GetPropertyULONG32("HasOutOfOrderTS", ulHasOutOfOrderTS);
            // Override session level average bitrate
            ppRealHeaders[i]->GetPropertyULONG32("AvgBitRate", ulAvgBitRate);
            // Overide session level RTP bandwidth modifiers
            // with media level modifiers
            ppRealHeaders[i]->GetPropertyULONG32("RtcpRRRate", ulRtpRRBitRate);
            ppRealHeaders[i]->GetPropertyULONG32("RtcpRSRate", ulRtpRSBitRate);
            ppRealHeaders[i]->GetPropertyULONG32("port", ulPort);
            HX_ASSERT(ulPort <= 0xFFFF);

            if(pControl)
            {
                pInfo->m_streamControl = pControl->GetBuffer();
                HX_RELEASE(pControl);
            }
            else
            {
                char tmp[32];
                SafeSprintf(tmp, 32, "streamid=%u", (UINT16)streamNumber);
                pInfo->m_streamControl = tmp;
            }
            pInfo->m_streamNumber = (UINT16)streamNumber;
            pInfo->m_bNeedReliablePackets = needReliable ? TRUE: FALSE;
            pInfo->m_rtpPayloadType = (INT16)rtpPayloadType;
            pInfo->m_sampleRate = sampleRate;
            pInfo->m_sampleSize = sampleSize / 8;
            pInfo->m_RTPFactor = RTPFactor;
            pInfo->m_HXFactor = HXFactor;
            pInfo->m_bHasMarkerRule = bHasMarkerRule;
            pInfo->m_markerRule = markerRule;
            pInfo->m_sPort = (UINT16)ulPort;
            pInfo->m_bIsLive = ulIsLive ? TRUE : FALSE;
            pInfo->m_bHasOutOfOrderTS = ulHasOutOfOrderTS ? TRUE : FALSE;
            pInfo->m_eMediaType = eMediaType;
            pInfo->m_bIsSyncMaster = FALSE;     // decison will be made on setup response
            pInfo->m_ulAvgBitRate = ulAvgBitRate;
            pInfo->m_ulRtpRRBitRate = ulRtpRRBitRate;
            pInfo->m_ulRtpRSBitRate = ulRtpRSBitRate;
            pInfo->m_bRealMedia = bRealMedia;

            if (m_pSrcBufStats)
            {
                m_pSrcBufStats->InitStream((UINT16)streamNumber,
                                           pInfo->m_bIsLive);
            }

            if (m_pRateAdaptInfo)
            {
		UINT32 ulHelixAdaptation = 0;   
		UINT32 ulVersionTarget = HX_ENCODE_PROD_VERSION(11L, 1L, 0L, 0L);

		//
		// due to a bug in released Helix Server, the client needs to disable "Helix-Adaption"
		// if the server version < 11.1.0.0
		//
		// see https://bugs.helixcommunity.org/show_bug.cgi?id=4989 for details
		//
		if ((m_ulServerVersion < ulVersionTarget) &&
		    HXR_OK == ppRealHeaders[i]->GetPropertyULONG32("Helix-Adaptation-Support",
								   ulHelixAdaptation) &&
		    (1 == ulHelixAdaptation))
		{
		    ppRealHeaders[i]->SetPropertyULONG32("Helix-Adaptation-Support", 0);
		}

                m_pRateAdaptInfo->OnStreamHeader((UINT16)streamNumber,
                                                 ppRealHeaders[i]);
            }

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
            // get multicast address from the media description
            if (HXR_OK == ppRealHeaders[i]->GetPropertyCString("MulticastAddress", pIPAddress))
            {
                SetMulticastAddrHelper(pInfo, (const char*)pIPAddress->GetBuffer(), (UINT16)ulPort);
                HX_RELEASE(pIPAddress);
            }
            // otherwise, apply the session multicast address to media component
            else if (m_bMulticast && !m_sdpMulticastAddr.IsEmpty())
            {
                SetMulticastAddrHelper(pInfo, m_sdpMulticastAddr, (UINT16)ulPort);
            }
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */

            m_streamInfoList.AddTail(pInfo);
        }

        if (m_bMulticast)
        {
            m_pSDPFileHeader->SetPropertyULONG32("LiveStream", 1);
            m_bIsLive = TRUE;
        }
    }

  cleanup:

    // clean up..
    for(UINT16 i=0;i<nValues;++i)
    {
        // don't need IHXValues anymore...
        HX_RELEASE(ppValues[i]);
    }
    HX_VECTOR_DELETE(ppValues);
    HX_VECTOR_DELETE(pulSubscriptionBW);

    // release ppRealHeaders too
    if (NULL != ppRealHeaders)
    {
        for (int i = 0; i < (int)ulNumStreams; i++)
        {
            HX_RELEASE(ppRealHeaders[i]);
        }
        HX_VECTOR_DELETE(ppRealHeaders);
    }

    HX_RELEASE(pSD);

    return rc;
}

void
RTSPClientProtocol::RemoveSDPHeaders(void)
{
    HX_RELEASE(m_pSDPFileHeader);
    HX_RELEASE(m_pSDPRequestHeader);

    if (m_pSDPStreamHeaders)
    {
        CHXSimpleList::Iterator i;
        for(i=m_pSDPStreamHeaders->Begin();i!=m_pSDPStreamHeaders->End();++i)
        {
            IHXValues* pStreamHeader = (IHXValues*)(*i);
            HX_RELEASE(pStreamHeader);
        }
        HX_DELETE(m_pSDPStreamHeaders);
    }
}

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
HXBOOL
RTSPClientProtocol::DetermineIfRMPresentation(IHXValues** ppStrmHeaders,
                                              UINT32 ulNumStreams)
{
    HXBOOL bIsRMPresentation = FALSE;

    if (ppStrmHeaders && ulNumStreams)
    {
        IHXValues* pStrmHdr = NULL;
        IHXBuffer* pASMRuleBook = NULL;
        IHXBuffer* pMimeType = NULL;
        UINT32 ulRTPPayload = RTP_PAYLOAD_RTSP + 1;
        HXBOOL bIsRMStream = FALSE;
        UINT32 ulIdx = 0;

        bIsRMPresentation = TRUE;

        for (ulIdx = 0; bIsRMPresentation && (ulIdx < ulNumStreams); ulIdx++)
        {
            pStrmHdr = ppStrmHeaders[ulIdx];

            bIsRMStream = FALSE;

            if (pStrmHdr)
            {
                if (HXR_OK == pStrmHdr->GetPropertyULONG32("RTPPayloadType", ulRTPPayload) &&
                    ulRTPPayload == RTP_PAYLOAD_RTSP)
                {
                    bIsRMStream = TRUE;
                }
                ulRTPPayload = RTP_PAYLOAD_RTSP + 1;

                if (bIsRMStream)
                {
                    bIsRMStream = FALSE;

                    if (HXR_OK == pStrmHdr->GetPropertyCString("ASMRuleBook", pASMRuleBook) &&
                        pASMRuleBook)
                    {
                        bIsRMStream = TRUE;
                    }
                }
                HX_RELEASE(pASMRuleBook);

                if (bIsRMStream)
                {
                    bIsRMStream = FALSE;

                    if (HXR_OK == pStrmHdr->GetPropertyCString("MimeType", pMimeType) &&
                        pMimeType)
                    {
                        if (strstr((const char*) pMimeType->GetBuffer(), RN_COMMON_MIME_TYPE_FRAGMENT))
                        {
                            bIsRMStream = TRUE;
                        }
                    }
                }
                HX_RELEASE(pMimeType);
            }

            bIsRMPresentation = (bIsRMStream && bIsRMPresentation);
        }
    }

    return bIsRMPresentation;
}

HXBOOL
RTSPClientProtocol::GetSubscriptionBW(IHXValues*    pFileHeader,
                                      IHXValues**   ppStrmHeaders,
                                      UINT16        unNumStrmHeaders,
                                      REF(UINT32*)  pulSubscriptionBW,
                                      UINT32        ulNumStreams,
                                      REF(UINT32)   ulRuleNumber)
{
    HX_ASSERT(pFileHeader);
    HX_ASSERT(ppStrmHeaders);
    HX_ASSERT(unNumStrmHeaders >= 1);
    HX_ASSERT(pulSubscriptionBW);
    HX_ASSERT(ulNumStreams >= 1);

    IHXBuffer*  pRuleBuf = NULL;
    IHXBuffer* pBandwidth = NULL;

    ulRuleNumber = 0;

    pFileHeader->AddRef();
    
    UINT32 uConnectionBW = 0;
    IHXConnectionBWInfo* pConnBWInfo = NULL;
    if (HXR_OK == m_pContext->QueryInterface(IID_IHXConnectionBWInfo,
                                             (void**)&pConnBWInfo))
    {
        HX_RESULT rv = pConnBWInfo->GetConnectionBW(uConnectionBW, FALSE);
        if (FAILED(rv))
        {
            HXLOGL2(HXLOG_CBWI, "RTSPClientProtocol::GetSubscriptionBW() "
                    "GetConnectionBW() returned 0x%08x",
                    rv);
        }
    }
    else
    {
        /* Get initial bandwidth guess from Prefs */
        ReadPrefUINT32(m_pPreferences, "Bandwidth", uConnectionBW);
    }

    HX_RELEASE(pConnBWInfo);
    HX_ASSERT(uConnectionBW);

    if (!uConnectionBW)
    {
        HX_ASSERT(!"Don't know connection BW");
        uConnectionBW = 64000;
    }

    CHXString tmp;
    tmp.AppendULONG(uConnectionBW);
    CreateStringBufferCCF(pBandwidth, tmp, m_pCommonClassFactory);

    if (HXR_OK != pFileHeader->GetPropertyCString("ASMRuleBook", pRuleBuf))
    {
        // OK, this is a single stream presentation.  Take an ASMRuleBook from
        // any of stream headers (they are all the same), and use it to decide
        // which stream header to use depending on bit rate
        HX_ASSERT(1 == ulNumStreams);

        // get ASMRuleBook
        HX_ASSERT(NULL != ppStrmHeaders[0]);
        IHXValues* pHeader = ppStrmHeaders[0];
        pHeader->AddRef();

        if (HXR_OK == pHeader->GetPropertyCString("ASMRuleBook", pRuleBuf))
        {
            IHXBuffer*      pBuffer = NULL;
            UINT16          unRules = 0;
            ASMRuleBook     rules((char*)pRuleBuf->GetBuffer());

            unRules = rules.GetNumRules();

            // get subscriptions for this bandwidth
            HXBOOL bSubInfo[256];
            UINT16 unRuleNum = 0;

            IHXValues* pValues = NULL;
	    
	    if (HXR_OK == CreateValuesCCF(pValues, m_pContext))
	    {
		pValues->SetPropertyCString("Bandwidth", pBandwidth);
		rules.GetSubscription(bSubInfo, pValues);
		HX_RELEASE(pValues);
	    }

            // get a rule number that we are interested in
            int y;
            for (y = 0; y < (int)unRules; y++)
            {
                if (TRUE == bSubInfo[y])
                {
                    IHXBuffer* pBw = 0;
                    unRuleNum = y;

                    // make sure AverageBandwidth != 0
                    rules.GetProperties(y, pValues);

                    if (HXR_OK == pValues->GetPropertyCString("AverageBandwidth",
                                                              pBw))
                    {
                        pulSubscriptionBW[0] += atol((const char*)pBw->GetBuffer());
                        HX_RELEASE(pBw);
                    }
                    else
                    {
                        // TimeStampDelivery only stream
                        pulSubscriptionBW[0] = 0;
                    }
                    HX_RELEASE(pValues);
                }
            }
            HX_RELEASE(pRuleBuf);
        }
        else
        {
            // There is no ASMRuleBook at all...
            // This should never happen.
            HX_RELEASE(pFileHeader);
            HX_RELEASE(pBandwidth);
            HX_RELEASE(pHeader);
            HX_ASSERT(FALSE);
            return FALSE;
        }

        HX_RELEASE(pHeader);
    }
    else
    {
        // this is a multiple stream presentation.
        // take ASMRuleBook for a file and figure out BW to use for
        // each stream
        IHXBuffer*      pBuffer     = NULL;
        UINT16          unRules     = 0;

        ASMRuleBook     rules((char*)pRuleBuf->GetBuffer());

        unRules = rules.GetNumRules();

        // get subscriptions for this bandwidth
        HXBOOL bSubInfo[256];

        IHXValues* pValues = NULL;
	
	if (HXR_OK == CreateValuesCCF(pValues, m_pContext))
	{
	    pValues->SetPropertyCString("Bandwidth", pBandwidth);
	    rules.GetSubscription(bSubInfo, pValues);
	    HX_RELEASE(pValues);
	}

        // get a rule number that we are interested in
        // Assuming there is only one TRUE
        int y;
        for (y = 0; y < (int)unRules; y++)
        {
            if (TRUE == bSubInfo[y])
            {
                // there should be only one
                ulRuleNumber = y;
                break;
            }
        }

        // Get a BW for each stream
        rules.GetProperties((int)ulRuleNumber, pValues);
        for (int i = 0; i < (int)ulNumStreams; i++)
        {
            char rgStreamBW[32];
            sprintf(rgStreamBW, "Stream%dBandwidth", i);
            if (HXR_OK == pValues->GetPropertyCString((const char*)rgStreamBW,
                                                      pBuffer))
            {
                pulSubscriptionBW[i] = (UINT32)atol((const char*)pBuffer->GetBuffer());
                HX_RELEASE(pBuffer);
            }
        }
        HX_RELEASE(pValues);
        HX_RELEASE(pRuleBuf);
    }

    HX_RELEASE(pFileHeader);
    HX_RELEASE(pBandwidth);

    return TRUE;
}

HXBOOL
RTSPClientProtocol::GetRightHeaders(REF(IHXValues**)    ppRealHeaders, // out
                                    UINT32              ulNumStreams,
                                    IHXValues**         ppHeaders,
                                    UINT32              cHeaders,
                                    UINT32*             pulSubscriptionBW,
                                    UINT32              ulRuleNumber)
{
    HX_ASSERT(ulNumStreams >= 1);
    HX_ASSERT(ppHeaders);
    HX_ASSERT(pulSubscriptionBW);

    ppRealHeaders = new IHXValues*[ulNumStreams];
    memset(ppRealHeaders, NULL, sizeof(IHXValues*) * ulNumStreams);

    for (int i = 0; i < (int)ulNumStreams; i++)
    {
        ULONG32 ulID = 0;
        ULONG32 ulBW = 0;
        ULONG32 ulRule = 0;
        HXBOOL    bFound = FALSE;

        for (int j = 0; j < (int)cHeaders; j++)
        {
            HX_ASSERT(NULL != ppHeaders[j]);

            IHXValues* pSrcH = ppHeaders[j];
            pSrcH->AddRef();

            // Scalable multicast identifies which uber rule each stream is for
            if (HXR_OK == pSrcH->GetPropertyULONG32("RuleNumber", ulRule))
            {
                if (ulRule == ulRuleNumber)
                {
                    bFound = TRUE;

                    // This is the right heaader,
                    if ((HXR_OK == pSrcH->GetPropertyULONG32("StreamId", ulID)) &&
                        ((int)ulID == i))
                    {
                        ppRealHeaders[i] = pSrcH;
                        ppRealHeaders[i]->AddRef();
                        HX_RELEASE(pSrcH);
                        break; // we found for this stream, go to next one
                    }
                }
            }
            else if ((HXR_OK == pSrcH->GetPropertyULONG32("AvgBitRate", ulBW)) &&
                     (ulBW == pulSubscriptionBW[i]))
            {
                // this one has the right BW, how about stream number?
                if ((HXR_OK == pSrcH->GetPropertyULONG32("StreamId", ulID)) &&
                    ((int)ulID == i))
                {
                    bFound = TRUE;

                    // This is the right heaader,
                    ppRealHeaders[i] = pSrcH;
                    ppRealHeaders[i]->AddRef();
                    HX_RELEASE(pSrcH);
                    break; // we found for this stream, go to next one
                }
            }
            HX_RELEASE(pSrcH);
        }

        if (!bFound)
        {
            // this should never happen
            ppRealHeaders[i] = NULL;
            HX_ASSERT(FALSE);
            return FALSE;
        }
    }

    return TRUE;
}

HX_RESULT
RTSPClientProtocol::preparePOSTStatsMsg(CHXString& host,
                                        UINT32& port,
                                        CHXString& resource,
                                        REF(IHXValues*) pPOSTHeader)
{
    HX_RESULT   rc = HXR_OK;
    UINT32      ulPort = 0;
    IHXBuffer*  pURL = NULL;
    IHXBuffer*  pHost = NULL;
    IHXBuffer*  pResource = NULL;
    IHXBuffer*  pLanguage = NULL;
    IHXValues*  pProperties = NULL;
    CHXURL*     pStatsURL = NULL;
    CHXString   value;

    // make sure ulHeaderEntries matches pszHeaderEntries
    UINT32 ulHeaderEntries = 5;
    const char* pszHeaderEntries[] = {"Bandwidth", "ClientID", "GUID", "RegionData",
                                      "SupportsMaximumASMBandwidth"};

    pPOSTHeader = NULL;

    if (!m_pSDPFileHeader ||
        HXR_OK != m_pSDPFileHeader->GetPropertyCString("StatsURL", pURL))
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    pStatsURL = new CHXURL((const char*)pURL->GetBuffer(), m_pContext);
    if (!pStatsURL)
    {
        rc = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    pProperties = pStatsURL->GetProperties();
    if (!pProperties)
    {
        rc = HXR_FAILED;
        goto cleanup;
    }

    if (HXR_OK != pProperties->GetPropertyBuffer(PROPERTY_HOST, pHost))
    {
        rc = HXR_INVALID_URL_HOST;
        goto cleanup;
    }

    if (HXR_OK != pProperties->GetPropertyULONG32(PROPERTY_PORT, ulPort))
    {
        rc = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (HXR_OK != pProperties->GetPropertyBuffer(PROPERTY_RESOURCE, pResource))
    {
        rc = HXR_INVALID_URL_PATH;
        goto cleanup;
    }

    rc = CreateValuesCCF(pPOSTHeader, m_pContext);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    rc = SetCStringPropertyCCF(pPOSTHeader, "Accept", "*/*", m_pContext);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    rc = SetCStringPropertyCCF(pPOSTHeader, "UserAgent", "RMA/1.0 (compatible; RealMedia)", m_pContext);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    if (m_pSDPRequestHeader)
    {
        m_pSDPRequestHeader->GetPropertyCString("Language", pLanguage);

        IHXBuffer*  pValue = NULL;
        for (UINT32 i = 0; i < ulHeaderEntries; i++)
        {
            if (HXR_OK == m_pSDPRequestHeader->GetPropertyCString(pszHeaderEntries[i], pValue))
            {
                rc = SetCStringPropertyCCF(pPOSTHeader, pszHeaderEntries[i],
                                           (const char*)pValue->GetBuffer(), m_pContext);
                HX_RELEASE(pValue);

                if (HXR_OK != rc)
                {
                    goto cleanup;
                }
            }
        }
    }

    rc = SetCStringPropertyCCF(pPOSTHeader, "Connection", "Keep-Alive", m_pContext);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    value = (const char*)pHost->GetBuffer();
    if (80 != ulPort)
    {
        value += ':';
        value.AppendULONG(ulPort);
    }

    rc = SetCStringPropertyCCF(pPOSTHeader, "Host", (const char*)value, m_pContext);
    if (HXR_OK != rc)
    {
        goto cleanup;
    }

    if (pLanguage)
    {
        rc = SetCStringPropertyCCF(pPOSTHeader, "Accept-Language", (const char*)pLanguage->GetBuffer(), m_pContext);
        if (HXR_OK != rc)
        {
            goto cleanup;
        }
    }

  cleanup:

    if (HXR_OK == rc)
    {
        host = (const char*)pHost->GetBuffer();
        resource = (const char*)pResource->GetBuffer();
        port = ulPort;
    }

    HX_RELEASE(pURL);
    HX_RELEASE(pHost);
    HX_RELEASE(pResource);
    HX_RELEASE(pLanguage);
    HX_RELEASE(pProperties);

    HX_DELETE(pStatsURL);

    return rc;
}
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */

HXBOOL
RTSPClientProtocol::GetStreamCountNoTrust(IHXValues**   ppHeaders,
                                          UINT16        unNumHeader,
                                          REF(UINT32)   ulNumStreams)
{
    HX_ASSERT(NULL != ppHeaders);

    UINT32      ulID = 0;
    HXBOOL        rgFound[256];

    memset(rgFound, 0, 256);

    for (UINT16 i = 0; i < unNumHeader; i++)
    {
        HX_ASSERT(ppHeaders[i] != NULL);
        IHXValues* pSrcHeader = NULL;

        pSrcHeader = ppHeaders[i];
        pSrcHeader->AddRef();

        // "StreamId" is the field that ppfobj.cpp puts for a group of
        // streams
        if (HXR_OK == pSrcHeader->GetPropertyULONG32("StreamId", ulID))
        {
            if (!rgFound[ulID])
            {
                rgFound[ulID] = TRUE;
                ulNumStreams++;
            }
        }
        else
        {
            // OK, trust the "StreamCount".  This is not a SDP file generated
            // by pplyfobj.cpp
            ulNumStreams = 0;
            HX_RELEASE(pSrcHeader);
            return FALSE;
        }
        HX_RELEASE(pSrcHeader);
    }

    return TRUE;
}

HX_RESULT
RTSPClientProtocol::CreateUDPSockets(UINT32 ulStream, UINT16 uPort)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::CreateUDPSockets() str = %lu; port = %u",this, ulStream, uPort);
    HX_RESULT           rc = HXR_OK;
    IHXSocket*          pUDPSocket1 = NULL;
    IHXSocket*          pUDPSocket2 = NULL;
    UDPResponseHelper*  pUDPResponseHelper1 = NULL;
    UDPResponseHelper*  pUDPResponseHelper2 = NULL;
    IHXSockAddr* pAddr = NULL;

    pUDPResponseHelper1 = new UDPResponseHelper(this);
    pUDPResponseHelper2 = new UDPResponseHelper(this);

    if (!pUDPResponseHelper1 || !pUDPResponseHelper2)
    {
        rc = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    pUDPResponseHelper1->AddRef();
    pUDPResponseHelper2->AddRef();

    m_UDPResponseHelperList.AddTail(pUDPResponseHelper1);
    m_UDPResponseHelperList.AddTail(pUDPResponseHelper2);

    HX_ASSERT(m_pConnectAddr); // even for scalable multicast case should be set (in which case it's the multicast group addr)
    if((HXR_OK != HXSockUtil::CreateSocket(m_pNetSvc, pUDPResponseHelper1, m_pConnectAddr->GetFamily(), HX_SOCK_TYPE_UDP, HX_SOCK_PROTO_ANY, pUDPSocket1)) ||
       (HXR_OK != HXSockUtil::CreateSocket(m_pNetSvc, pUDPResponseHelper2, m_pConnectAddr->GetFamily(), HX_SOCK_TYPE_UDP, HX_SOCK_PROTO_ANY, pUDPSocket2)))

    {
        rc = HXR_FAIL;
        goto cleanup;
    }

    pUDPResponseHelper1->SetSock(pUDPSocket1);
    pUDPResponseHelper2->SetSock(pUDPSocket2);

#if defined(HELIX_FEATURE_TRANSPORT_MULTICAST)
    if (m_bMulticast)
    {
        pUDPSocket1->SetOption(HX_SOCKOPT_REUSEADDR, 1); //XXXLCM HX_SOCKOPT_REUSE_PORT?
        pUDPSocket2->SetOption(HX_SOCKOPT_REUSEADDR, 1);
    }
#endif /* HELIX_FEATURE_TRANSPORT_MULTICAST */

    pUDPSocket1->SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);
    pUDPSocket2->SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);

    rc = m_pNetSvc->CreateSockAddr(m_pConnectAddr->GetFamily(), &pAddr);
    if(FAILED(rc))
    {
        goto cleanup;
    }

    if (m_pFWCtlMgr)
    {
        m_pFWCtlMgr->OpenPort(uPort, HX_NET_FW_IP_PROTOCOL_UDP);
        HX_ASSERT(0 == m_FWPortToBeClosed);
        m_FWPortToBeClosed = uPort;
    }

    pAddr->SetPort(uPort);
    rc = pUDPSocket1->Bind(pAddr);
    if(FAILED(rc))
    {
        goto cleanup;
    }

    if (m_pFWCtlMgr)
    {
        m_pFWCtlMgr->OpenPort((UINT32)(uPort + 1), HX_NET_FW_IP_PROTOCOL_UDP);
    }

    pAddr->SetPort((UINT16)(uPort + 1));
    rc = pUDPSocket2->Bind(pAddr);

  cleanup:

    HX_RELEASE(pAddr);

    if (HXR_OK == rc)
    {
        HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::CreateUDPSockets(): adding sockets %p and %p for stream %lu", this, pUDPSocket1, pUDPSocket2, ulStream);
        HX_ASSERT(0 == m_pUDPSocketStreamMap->Lookup(ulStream));
        HX_ASSERT(0 == m_pRTCPSocketStreamMap->Lookup(ulStream));
        (*m_pUDPSocketStreamMap)[(LONG32)ulStream] = pUDPSocket1;
        (*m_pRTCPSocketStreamMap)[(LONG32)ulStream] = pUDPSocket2;

        if (!m_bMulticast)
        {
            // register for socket state notifications
            pUDPSocket1->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
            pUDPSocket2->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);
        }
    }
    else
    {
        if (pUDPSocket1)
        {
            pUDPSocket1->Close();
            pUDPSocket1->Release();
        }

        if (pUDPSocket2)
        {
            pUDPSocket2->Close();
            pUDPSocket2->Release();
        }
    }

    if (HXR_OK != rc || !m_bMulticast)
    {
        // remove the ports from exception list
        // UDP poking will kick in to traverse the firewall
        if (m_FWPortToBeClosed && m_pFWCtlMgr)
        {
            m_pFWCtlMgr->ClosePort(m_FWPortToBeClosed, HX_NET_FW_IP_PROTOCOL_UDP);
            m_pFWCtlMgr->ClosePort((UINT32)(m_FWPortToBeClosed + 1), HX_NET_FW_IP_PROTOCOL_UDP);
            m_FWPortToBeClosed = 0;
        }
    }

    return rc;
}


void
RTSPClientProtocol::ReportError(HX_RESULT theErr)
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientProtocol[%p]::ReportError(): %08x", this, theErr);
    if (m_pSession)
    {
        m_pSession->ReportError(theErr);
    }
}

void
RTSPClientSession::ReportError( HX_RESULT theErr, 
                                const UINT8 unSeverity, 
                                const char* pUserString )
{
    HXLOGL3(HXLOG_RTSP, "RTSPClientSession[%p]::ReportError(): %08x", this, theErr);

    IHXErrorMessages * pErrorNotifier = NULL;
    IUnknown * pPlayer = NULL;
    IHXClientEngine* pEngine = NULL;
    UINT16 nNumPlayers = 0;

    m_pContext->QueryInterface(IID_IHXClientEngine, (void**)&pEngine);
    if (pEngine)
    {
        nNumPlayers = pEngine->GetPlayerCount();
        for (UINT16 ii=0; ii<nNumPlayers; ii++)
        {
            pEngine->GetPlayer(ii,pPlayer);
            if (pPlayer)
            {
                pPlayer->QueryInterface( IID_IHXErrorMessages, (void**)&pErrorNotifier );
            }
            if (pErrorNotifier)
            {
                pErrorNotifier->Report(unSeverity, theErr, 0, pUserString, NULL);
                pErrorNotifier->Release();
            }
            HX_RELEASE(pPlayer);
        }
    }
    HX_RELEASE(pEngine);
}

HX_RESULT
RTSPClientProtocol::RetrieveReconnectInfo(MIMEHeader*   pHeader,
                                          ReconnectType reconnectType,
                                          IHXValues*&   pReconnectValues)
{
    HX_RESULT           rc = HXR_OK;
    UINT32              ulRand = 0;
    IHXBuffer*          pServer = NULL;
    MIMEHeaderValue*    pHeaderValue = NULL;
    MIMEParameter*      pParam0 = NULL;
    MIMEParameter*      pParam1 = NULL;
    ReconnectInfo*      pReconnectInfo = NULL;
    CHXSimpleList       reconnectInfoList;
    CHXSimpleList::Iterator i;

    if (!pReconnectValues)
    {
	CreateValuesCCF(pReconnectValues, m_pContext);
    }

    pReconnectValues->SetPropertyULONG32("Reconnect", 1);

    pHeaderValue = pHeader->getFirstHeaderValue();
    while (pHeaderValue)
    {
        pParam0 = pHeaderValue->getFirstParameter();
        pParam1 = pHeaderValue->getNextParameter();

        if (pParam0)
        {
            pReconnectInfo = new ReconnectInfo;
            pReconnectInfo->m_server = (const char*)pParam0->m_value;
            if (pParam1)
            {
                pReconnectInfo->m_ulPort = (UINT32)atoi((const char*)pParam1->m_value);
            }
        }
        reconnectInfoList.AddTail(pReconnectInfo);

        pHeaderValue = pHeader->getNextHeaderValue();
    }

    if (!reconnectInfoList.IsEmpty())
    {
        ulRand = (HX_GET_TICKCOUNT() % reconnectInfoList.GetCount()) + 1;

        // random select the alternate server\port AND cleanup the list
        for(i = reconnectInfoList.Begin(); i != reconnectInfoList.End(); ++i)
        {
            pReconnectInfo = (ReconnectInfo*)*i;
            switch (ulRand)
            {
               case 0:
                   break;
               case 1:
                   pServer = NULL; 
		   if (HXR_OK == CreateAndSetBufferCCF(pServer, (UCHAR*)(const char*)pReconnectInfo->m_server,
						       pReconnectInfo->m_server.GetLength()+1, m_pContext))
		   {
			if (reconnectType == ALTERNATE_SERVER)
			{
			    pReconnectValues->SetPropertyCString("Alternate-Server", pServer);
			    pReconnectValues->SetPropertyULONG32("Alternate-ServerPort", pReconnectInfo->m_ulPort);
			}
			else if (reconnectType == ALTERNATE_PROXY)
			{
			    pReconnectValues->SetPropertyCString("Alternate-Proxy", pServer);
			    pReconnectValues->SetPropertyULONG32("Alternate-ProxyPort", pReconnectInfo->m_ulPort);
			}

			HX_RELEASE(pServer);
		   }
                   ulRand--;
                   break;
               default:
                   ulRand--;
                   break;
            }
            HX_DELETE(pReconnectInfo);
        }
        reconnectInfoList.RemoveAll();
    }

    return rc;
}

HXBOOL RTSPClientProtocol::PipelineRTSP()
{
    //m_bPipelineRTSP defaults to TRUE and is turned off if we find ourselves
    //going through a NON Real proxy or a non Real server. We do this because
    //our RTSP pipelining code can confuse non-Real proxies and servers (but
    //maybe not all of them). Also, a user can turn off/on pipelining in all
    //cases by using this pref below.
    HXBOOL bRet = m_bPipelineRTSP; 
    ReadPrefBOOL(m_pPreferences, "PipelineRTSP", bRet);

    return bRet;
}

RTSPClientProtocol::UDPResponseHelper::UDPResponseHelper(RTSPClientProtocol* pParent)
    : m_lRefCount(0)
    , m_pSock(0)
    , m_pLocalAddr(0)
    , m_pOwner(pParent)
{
    HX_ASSERT(m_pOwner);
}

void RTSPClientProtocol::UDPResponseHelper::SetSock(IHXSocket* pSock)
{
    HX_ASSERT(pSock);
    HX_ASSERT(!m_pSock);
    m_pSock = pSock;
    m_pSock->AddRef();
}

RTSPClientProtocol::UDPResponseHelper::~UDPResponseHelper()
{
    HX_RELEASE(m_pLocalAddr);
    HX_RELEASE(m_pSock);
}

STDMETHODIMP
RTSPClientProtocol::UDPResponseHelper::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), this },
            { GET_IIDHANDLE(IID_IHXUDPResponse), (IHXUDPResponse*) this },
            { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*) this },
        };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
    RTSPClientProtocol::UDPResponseHelper::AddRef()
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
STDMETHODIMP_(ULONG32)
    RTSPClientProtocol::UDPResponseHelper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (ULONG32)m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
RTSPClientProtocol::UDPResponseHelper::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_ASSERT(m_pOwner);
    if (m_pOwner != NULL)
    {
        switch(uEvent)
        {
           case HX_SOCK_EVENT_READ:
           {
               if (SUCCEEDED(status))
               {
                   if(!m_pLocalAddr)
                   {
                       m_pSock->GetLocalAddr(&m_pLocalAddr);
                   }
                   HX_ASSERT(m_pLocalAddr);

                   IHXBuffer* pBuf = NULL;
                   IHXSockAddr* pAddr = NULL;

                   HX_RESULT hr = m_pSock->ReadFrom(&pBuf, &pAddr);
                   if (hr != HXR_SOCK_WOULDBLOCK)
                   {
                       m_pOwner->ReadFromDone(hr, pBuf, pAddr, m_pLocalAddr);
                   }
                   HX_RELEASE(pAddr);
                   HX_RELEASE(pBuf);
               }
               else
               {
                   m_pOwner->ReadFromDone(HXR_FAIL, NULL, NULL, NULL);
               }
           }
           break;
           case HX_SOCK_EVENT_CLOSE:
               // normally only the client initiates close
               // Connection errors are handled via the TCP socket.
                if (status == HXR_OUTOFMEMORY ||
                    status == HXR_SOCK_CONNRESET)
               {
                   m_pOwner->OnProtocolError(status);
               }
               break;
           default:
               HX_ASSERT(false); // unexpected (didn't select this event)
               break;
        }

    }
    return HXR_OK;
}

RTSPClientProtocol::TimeoutCallback::TimeoutCallback
(
    RTSPClientProtocol* pOwner,
    RTSPClientTimeoutType type
    ) : m_lRefCount(0)
      , m_pOwner(pOwner)
      , m_type(type)
{
    if (m_pOwner)
    {
        m_pOwner->AddRef();
    }
}

RTSPClientProtocol::TimeoutCallback::~TimeoutCallback()
{
    // DON'T use HX_RELEASE
    // m_pOwner isn't derived from a COM interface
    if (m_pOwner)
    {
        m_pOwner->Release();
        m_pOwner = NULL;
    }
}

STDMETHODIMP
RTSPClientProtocol::TimeoutCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), this },
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*) this },
        };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32)
    RTSPClientProtocol::TimeoutCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
    RTSPClientProtocol::TimeoutCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return (ULONG32)m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
RTSPClientProtocol::TimeoutCallback::Func()
{
    if (m_pOwner)
    {
        m_pOwner->AddRef();

        switch(m_type)
        {
           case RTSPCLIENT_TIMEOUT_CONNECTION:
               m_pOwner->DoConnectionCheck();
               break;
           case RTSPCLIENT_TIMEOUT_KEEPALIVE:
               m_pOwner->SendKeepAlive();
               break;
           default:
               break;
        }

        m_pOwner->Release();
    }

    return HXR_OK;
}

#if defined(_MACINTOSH)
RTSPClientProtocol::RTSPClientProtocolCallback::RTSPClientProtocolCallback
(
    RTSPClientProtocol* pOwner
    ) : m_lRefCount(0)
      , m_pOwner(pOwner)
      , m_bIsCallbackPending(FALSE)
      , m_Handle(0)
      , m_pPendingRequestHeaders(NULL)
{
    if (m_pOwner)
    {
        m_pOwner->AddRef();
    }
}

RTSPClientProtocol::RTSPClientProtocolCallback::~RTSPClientProtocolCallback()
{
    HX_RELEASE(m_pOwner);
    HX_RELEASE(m_pPendingRequestHeaders);
}

STDMETHODIMP
RTSPClientProtocol::RTSPClientProtocolCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IUnknown), this },
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*) this },
        };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
    RTSPClientProtocol::RTSPClientProtocolCallback::AddRef()
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
STDMETHODIMP_(ULONG32)
    RTSPClientProtocol::RTSPClientProtocolCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
RTSPClientProtocol::RTSPClientProtocolCallback::Func()
{
    m_bIsCallbackPending = FALSE;
    m_Handle            = 0;

    m_pOwner->AddRef();
    m_pOwner->sendPendingStreamDescription(m_PendingDescURL, m_pPendingRequestHeaders);
    HX_RELEASE(m_pPendingRequestHeaders);
    m_pOwner->Release();
    return HXR_OK;
}
#endif /* _MACINTOSH */

void RTSPClientProtocol::AddPacketToPreSetupResponseQueue(UINT16 usPort, IHXBuffer* pBuffer)
{
    if (usPort && pBuffer)
    {
        // Has the map been created yet?
        if (!m_pPreSetupResponsePacketQueueMap)
        {
            m_pPreSetupResponsePacketQueueMap = new CHXMapLongToObj;
        }
        if (m_pPreSetupResponsePacketQueueMap)
        {
            // Has the queue been created yet?
            LONG32 lKey   = (LONG32) usPort;
            void*  pValue = NULL;
            HXBOOL bSet   = m_pPreSetupResponsePacketQueueMap->Lookup(lKey, pValue);
            if (!bSet)
            {
                CHXSimpleList* pQueue = new CHXSimpleList;
                if (pQueue)
                {
                    pValue = pQueue;
                    POSITION pos = m_pPreSetupResponsePacketQueueMap->SetAt(lKey, pValue);
                    if (pos)
                    {
                        bSet = TRUE;
                    }
                }
            }
            if (bSet && pValue)
            {
                // Get the queue pointer
                CHXSimpleList* pQueue = (CHXSimpleList*) pValue;
                // Get the buffer limit
                UINT32 ulBufferSize  = 0;
                UINT32 ulBufferLimit = GetBufferLimit(usPort);
                // Only computer the buffer size if
                // the buffer limit is not zero
                if (ulBufferLimit)
                {
                    ulBufferSize = GetBufferSize(pQueue);
                }
                // If the buffer limit is 0 (no limit) or
                // if this buffer will not put us over the
                // limit, then add this buffer to the queue
                if (!ulBufferLimit ||
                    (ulBufferSize + pBuffer->GetSize() <= ulBufferLimit))
                {
                    // AddRef the buffer
                    pBuffer->AddRef();
                    // Add the buffer to the queue
                    pQueue->AddTail(pBuffer);
                }
            }
        }
    }
}

HXBOOL RTSPClientProtocol::AnyPreSetupResponsePackets(UINT16 usPort)
{
    HXBOOL bRet = FALSE;

    if (m_pPreSetupResponsePacketQueueMap &&
        m_pPreSetupResponsePacketQueueMap->GetCount() > 0)
    {
        LONG32 lKey   = (LONG32) usPort;
        void*  pValue = NULL;
        if (m_pPreSetupResponsePacketQueueMap->Lookup(lKey, pValue))
        {
            CHXSimpleList* pQueue = (CHXSimpleList*) pValue;
            if (pQueue && pQueue->GetCount() > 0)
            {
                bRet = TRUE;
            }
        }
    }

    return bRet;
}

void RTSPClientProtocol::FlushPreSetupResponsePacketsToTransport(UINT16 usPort)
{
    HXLOGL2(HXLOG_TRAN, "Flushing pre-SETUP response packets to transport", usPort);
    // Get the transport
    RTSPTransport* pTrans = NULL;
    if (m_pTransportPortMap)
    {
        // Get the transport by port number
        pTrans = (RTSPTransport*)(*m_pTransportPortMap)[usPort];
        if (!pTrans && m_pTransportMPortMap)
        {
            // Was data received on multicast port?
            pTrans = (RTSPTransport*)(*m_pTransportMPortMap)[usPort];
        }
    }
    if (pTrans)
    {
        // Get the packet queue
        if (m_pPreSetupResponsePacketQueueMap &&
            m_pPreSetupResponsePacketQueueMap->GetCount() > 0)
        {
            LONG32 lKey   = (LONG32) usPort;
            void*  pValue = NULL;
            if (m_pPreSetupResponsePacketQueueMap->Lookup(lKey, pValue))
            {
                CHXSimpleList* pQueue = (CHXSimpleList*) pValue;
                if (pQueue)
                {
                    // Send all the queued buffers to the transport
                    HX_RESULT rv = HXR_OK;
                    while (pQueue->GetCount() > 0)
                    {
                        // Get the buffer from the queue
                        IHXBuffer* pBuffer = (IHXBuffer*) pQueue->RemoveHead();
                        // If handlePacket() returns an error, then we
                        // will still empty the queue, but not pass any
                        // more buffers after the error is returned.
                        if (pBuffer && SUCCEEDED(rv))
                        {
                            rv = pTrans->handlePacket(pBuffer);
                        }
                        // Remove the queue's ref on the buffer
                        HX_RELEASE(pBuffer);
                    }
                    // Remove the queue from the map
                    m_pPreSetupResponsePacketQueueMap->RemoveKey(lKey);
                    // Delete the queue
                    HX_DELETE(pQueue);
                    // If we have deleted the last entry from
                    // the map, then delete the map. This saves
                    // us from having to do a GetCount() for every
                    // packet after this.
                    if (m_pPreSetupResponsePacketQueueMap->GetCount() == 0)
                    {
                        HX_DELETE(m_pPreSetupResponsePacketQueueMap);
                    }
                }
            }
        }
    }
}

void RTSPClientProtocol::ClearPreSetupResponseQueueMap()
{
    if (m_pPreSetupResponsePacketQueueMap)
    {
        // Delete all queues from the map
        CHXMapLongToObj::Iterator i;
        for (i = m_pPreSetupResponsePacketQueueMap->Begin();
             i != m_pPreSetupResponsePacketQueueMap->End(); ++i)
        {
            CHXSimpleList* pQueue = (CHXSimpleList*) (*i);
            if (pQueue)
            {
                while (pQueue->GetCount() > 0)
                {
                    IHXBuffer* pBuffer = (IHXBuffer*) pQueue->RemoveHead();
                    HX_RELEASE(pBuffer);
                }
            }
            HX_DELETE(pQueue);
        }
        m_pPreSetupResponsePacketQueueMap->RemoveAll();
        // Delete the map
        HX_DELETE(m_pPreSetupResponsePacketQueueMap);
    }

    // Delete the port-to-stream-number map
    HX_DELETE(m_pUDPPortToStreamNumMap);
}

UINT32 RTSPClientProtocol::GetBufferLimit(UINT16 usPort)
{
    UINT32 ulRet = 0;

    if (m_pUDPPortToStreamNumMap)
    {
        // Look up the stream number from the map
        LONG32 lKey   = (LONG32) usPort;
        void*  pValue = NULL;
        if (m_pUDPPortToStreamNumMap->Lookup(lKey, pValue))
        {
            // Get the stream number
            UINT32 ulStreamNumber = (UINT32) pValue;
            UINT16 usStreamNumber = (UINT16) ulStreamNumber;
            // Get the stream info
            RTSPStreamInfo* pStreamInfo = getStreamInfo(usStreamNumber);
            if (pStreamInfo)
            {
                ulRet = pStreamInfo->m_uByteLimit;
            }
        }
    }

    return ulRet;
}

UINT32 RTSPClientProtocol::GetBufferSize(CHXSimpleList* pQueue)
{
    UINT32 ulRet = 0;

    if (pQueue && pQueue->GetCount() > 0)
    {
        LISTPOSITION pos = pQueue->GetHeadPosition();
        while (pos)
        {
            IHXBuffer* pBuffer = (IHXBuffer*) pQueue->GetNext(pos);
            if (pBuffer)
            {
                ulRet += pBuffer->GetSize();
            }
        }
    }

    return ulRet;
}

void RTSPClientProtocol::AddPortToStreamMapping(UINT16 usPort, UINT16 usStreamNumber)
{
    if (!m_pUDPPortToStreamNumMap)
    {
        m_pUDPPortToStreamNumMap = new CHXMapLongToObj;
    }
    if (m_pUDPPortToStreamNumMap)
    {
        m_pUDPPortToStreamNumMap->SetAt((LONG32) usPort, (void*) usStreamNumber);
    }
}

#if defined(HELIX_FEATURE_RTSP_RESP_SINK)
void RTSPClientProtocol::LogRTSPResponseMessage(RTSPResponseMessage* pMsg)
{
    CHXString temp;

    const char* errCode = pMsg->errorCode();
    const char* errMsg  = pMsg->errorMsg();

    temp = errCode;
    temp += ",";
    temp += errMsg;

    HXLOGL2(HXLOG_RTSP, 
        "RTSPClientProtocol[%p]::RTSPResponseMessage(): Log Message %s", 
        this, (const char*)temp);

    if (m_pSession)
    {
        m_pSession->ReportError(HXR_RTSP_RESP_SINK_MESSAGE, HXLOG_INFO, temp);
    }
}
#endif

void RTSPClientProtocol::UpdateChosenTransport(RTSPResponseMessage* pMsg)
{
    MIMEHeader* pTransport = pMsg->getHeader("Transport");
    if(pTransport)
    {
        MIMEHeaderValue* pValue = pTransport->getFirstHeaderValue();
        if(!pValue)
        {
            return;
        }
        RTSPTransportRequest* pRequest = getTransportRequest(pValue);
        if(!pRequest)
        {
            return;
        }
        switch(pRequest->m_lTransportType)
        {
            case RTSP_TR_RDT_MCAST: 
            case RTSP_TR_RTP_MCAST:
            case RTSP_TR_BCNG_MCAST:
                {
                    /* m_currentTransport should be already set to MulticastMode
                     * if server responded with Multicast in SetupResponse
                     */
                    HX_ASSERT(m_currentTransport == MulticastMode);
                }
                break;
            case RTSP_TR_RDT_UDP:
            case RTSP_TR_TNG_UDP:
            case RTSP_TR_RTP_UDP:
            case RTSP_TR_BCNG_UDP:
                {
                    /* m_currentTransport should be set to MulticastMode
                     * or UDPMode if server responded with UDPMode in SetupResponse
                     */
                    HX_ASSERT(m_currentTransport == MulticastMode || 
                        m_currentTransport == UDPMode);
                    m_currentTransport = UDPMode;
                }
                break;
            case RTSP_TR_TNG_TCP:
            case RTSP_TR_RDT_TCP:
            case RTSP_TR_RTP_TCP:
            case RTSP_TR_BCNG_TCP:
                {
                    /* It is possible that the client requests for UDP as preferred transport
                     * but the server selects and reponds with TCPMode. In this case, update
                     * the current Transport mode to match the server selected transport.
                     * Also, the client sends tcp transport when in CloakMode. So update
                     * m_currentTransport to TCPMode if it is not in HTTPCloakMode
                     */
                    if (m_currentTransport != HTTPCloakMode)
                    {
                        m_currentTransport = TCPMode;
                    }
                }
                break;
            case RTSP_TR_NULLSET: /* not sure what to do here leave it as is */
            default:
                    break;
        }
    }	
}

