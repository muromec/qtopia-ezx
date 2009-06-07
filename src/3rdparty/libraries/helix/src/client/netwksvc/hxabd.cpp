/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxabd.cpp,v 1.18 2005/05/04 20:58:24 albertofloyd Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"
#include "hlxclib/time.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "dbcs.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "ihxpckts.h"
#include "hxmon.h"
#include "hxtbuf.h"
#include "hxtset.h"
#include "hxsockutil.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "portaddr.h"
#include "hxver.h"
#include "hxtick.h"
#include "unkimp.h"
#include "rtsputil.h"
#include "hxmarsh.h"
#include "growingq.h"
#include "bufnum.h"
#include "tngpkt.h"
#include "hxabdutil.h"
#include "hxabd.h"
#include "errdbg.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXABDCalibrator::CHXABDCalibrator(IUnknown* pContext)
                 :m_lRefCount(0)
                 ,m_pContext(pContext)
                 ,m_pSocket(NULL)
                 ,m_pResolve(NULL)
                 ,m_pScheduler(NULL)
                 ,m_pErrMsg(NULL)
                 ,m_pNetServices(NULL)
                 ,m_bInitialized(FALSE)
                 ,m_bABDPending(FALSE)
                 ,m_bReadHeaderDone(FALSE)
                 ,m_nABDServers(0)
                 ,m_nCurrentABDServer(0)
                 ,m_pABDResponse(NULL)
                 ,m_pAutoBWCalibrationSinkList(NULL)
                 ,m_ulABDTimeoutCBHandle(0)
                 ,m_pABDTimeoutCB(NULL)
                 ,m_pInQueue(NULL)
                 ,m_nProbingPacketsReceived(0)
                 ,m_nProbingPacketsRequested(MAX_ABD_PROBPKT)
                 ,m_ulProbingPacketSize(MAX_ABD_PROBPKT_SIZE)
                 ,m_nABDMode(3)
{
    HX_ADDREF(m_pContext);
    memset(&m_ulProbingPacketsGap[0], 0, sizeof(UINT) * MAX_ABD_PROBPKT);
    memset(&m_pABDServers[0], 0, sizeof(void*) * MAX_ABD_SERVERS);
    memset(&m_pABDProbPktInfo[0], 0, sizeof(void*) * MAX_ABD_PROBPKT);
}

CHXABDCalibrator::~CHXABDCalibrator()
{
    Close();
}

/*
 * IUnknown methods
 */

STDMETHODIMP
CHXABDCalibrator::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), this },
        { GET_IIDHANDLE(IID_IHXAutoBWCalibration), (IHXAutoBWCalibration*) this }
    };

    if(QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj) == HXR_OK)
    {
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
CHXABDCalibrator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
CHXABDCalibrator::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *      IHXAutoBWCalibration methods
 */
STDMETHODIMP
CHXABDCalibrator::InitAutoBWCalibration(IHXValues* pValues)
{
    HX_RESULT       rc = HXR_OK;
    const char*     pszServers = NULL;
    UINT32          ulValue = 0;
    CHXString       szServers;
    IHXBuffer*      pBuffer = NULL;
    IHXPreferences* pPreferences = NULL;

    if (m_bInitialized)
    {
        goto exit;
    }

    if (!m_pContext)
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    if (!m_pNetServices &&
        HXR_OK != m_pContext->QueryInterface(IID_IHXNetServices, (void**)&m_pNetServices))
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    if (!m_pScheduler &&
        HXR_OK != m_pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler))
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    if (!m_pErrMsg)
    {
        m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrMsg);
    }

    // retreive ABD attributes from the request header
    if (pValues &&
        HXR_OK == pValues->GetPropertyCString("ABDServers", pBuffer))
    {
        pszServers = (const char*)pBuffer->GetBuffer();
    }

    if (pValues &&
        HXR_OK == pValues->GetPropertyULONG32("AutoBWDetectionPackets", ulValue))
    {
        m_nProbingPacketsRequested = (UINT8)ulValue;
    }

    if (pValues &&
        HXR_OK == pValues->GetPropertyULONG32("AutoBWDetectionPacketSize", ulValue))
    {
        m_ulProbingPacketSize = ulValue;
    }

    if (pValues &&
        HXR_OK == pValues->GetPropertyULONG32("AutoBWDetectionMode", ulValue) &&
        (ulValue > 0) && (ulValue < 4))
    {
        m_nABDMode = (UINT8)ulValue;
    }

    // ABD attributes can be overwritten by the preferences
    if (HXR_OK == m_pContext->QueryInterface(IID_IHXPreferences, (void**)&pPreferences))
    {
        ReadPrefUINT8(pPreferences, "AutoBWDetectionPackets", m_nProbingPacketsRequested);
        ReadPrefUINT32(pPreferences, "AutoBWDetectionPacketSize", m_ulProbingPacketSize);

        ReadPrefUINT8(pPreferences, "AutoBWDetectionMode", m_nABDMode);

        if (HXR_OK == ReadPrefCSTRING(pPreferences, "AutoBWDetectionServers", szServers))
        {
            pszServers = (const char*)szServers;
        }
    }

    if (!pszServers)
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    DeleteServers();
    ParseServers(pszServers);

    if (!m_pInQueue)
    {
        m_pInQueue = new CByteGrowingQueue(1500);
        if (!m_pInQueue)
        {
            rc = HXR_OUTOFMEMORY;
            goto exit;
        }
        m_pInQueue->SetMaxSize(3000);
    }

exit:

    if (HXR_OK == rc)
    {
        m_bInitialized = TRUE;
    }

    HX_RELEASE(pBuffer);
    HX_RELEASE(pPreferences);

    return rc;
}

STDMETHODIMP
CHXABDCalibrator::StartAutoBWCalibration()
{
    HX_RESULT           rc = HXR_OK;
    char                szPort[HX_PORTSTRLEN]; /* Flawfinder: ignore */
    IHXResolveResponse* pResolveResponse = NULL;

    if (m_bABDPending)
    {
        rc = HXR_WOULD_BLOCK;
        goto exit;
    }

    if (!m_bInitialized)
    {
        rc = InitAutoBWCalibration(NULL);
        
        if (HXR_OK != rc)
        {
            goto exit;
        }
    }

    srand(time(NULL));
    m_nCurrentABDServer = rand() % m_nABDServers;

    // Create an IHXResolve
    rc = m_pNetServices->CreateResolver(&m_pResolve);
    if (HXR_OK != rc)
    {
        rc = HXR_FAILED;
        goto exit;
    }

    m_pABDResponse = CHXABDResponse::CreateObject();
    if (!m_pABDResponse)
    {
        rc = HXR_OUTOFMEMORY;
        goto exit;
    }

    // Create the response object. This object holds
    // the response interfaces for both IHXSocket and
    // IHXResolve (IHXSocketResponse and IHXResolveResponse,
    // respectively).
    m_pABDResponse->InitObject(this);
    m_pABDResponse->AddRef();

    // Get the IHXResolveResponse from the response object
    if (HXR_OK != m_pABDResponse->QueryInterface(IID_IHXResolveResponse,
                                                 (void**) &pResolveResponse))
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    // Tell the IHXResolve object about the response object
    if (HXR_OK != m_pResolve->Init(pResolveResponse))
    {
        rc = HXR_INVALID_PARAMETER;
        goto exit;
    }

    sprintf(szPort, "%lu", m_pABDServers[m_nCurrentABDServer]->ulPort);
    // Call IHXResolve::GetAddrInfo() to resolve the host name
    if (HXR_OK != m_pResolve->GetAddrInfo((const char*)*(m_pABDServers[m_nCurrentABDServer]->pServer),
                                          szPort, NULL))
    {
        rc = HXR_INVALID_HOST;
        goto exit;
    }

    // Schedule a timeout callback
    m_pABDTimeoutCB = new TimeoutCallback(this);
    if (!m_pABDTimeoutCB)
    {
        rc = HXR_OUTOFMEMORY;
        goto exit;
    }

    m_pABDTimeoutCB->AddRef();
    m_ulABDTimeoutCBHandle = m_pScheduler->RelativeEnter(m_pABDTimeoutCB, 10000);

exit:

    HX_RELEASE(pResolveResponse);

    if (HXR_OK == rc)
    {
        m_bABDPending = TRUE;

        if (m_pAutoBWCalibrationSinkList)
        {
            CHXSimpleList::Iterator i;
            for(i=m_pAutoBWCalibrationSinkList->Begin();i!=m_pAutoBWCalibrationSinkList->End();++i)
            {
                IHXAutoBWCalibrationAdviseSink* pSink = (IHXAutoBWCalibrationAdviseSink*)(*i);
                pSink->AutoBWCalibrationStarted(*(m_pABDServers[m_nCurrentABDServer]->pServer));
            }
        }
    }
    else
    {
        Reset();
    }

    return rc;
}

STDMETHODIMP
CHXABDCalibrator::StopAutoBWCalibration()
{
    return ReportABDStatus(HXR_ABORT, 0);
}

STDMETHODIMP
CHXABDCalibrator::AddAutoBWCalibrationSink(IHXAutoBWCalibrationAdviseSink* pSink)
{
    if (!pSink)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (!m_pAutoBWCalibrationSinkList)
    {
        m_pAutoBWCalibrationSinkList = new CHXSimpleList();
        m_pAutoBWCalibrationSinkList->AddTail(pSink);
        pSink->AddRef();
    }
    else
    {
        LISTPOSITION pos = m_pAutoBWCalibrationSinkList->Find(pSink);
        if (!pos)
        {
            m_pAutoBWCalibrationSinkList->AddTail(pSink);
            pSink->AddRef();
        }
    }

    return HXR_OK;
}

STDMETHODIMP
CHXABDCalibrator::RemoveAutoBWCalibrationSink(IHXAutoBWCalibrationAdviseSink* pSink)
{
    if (m_pAutoBWCalibrationSinkList)
    {
        LISTPOSITION pos = m_pAutoBWCalibrationSinkList->Find(pSink);
        if (pos)
        {
            m_pAutoBWCalibrationSinkList->RemoveAt(pos);
            HX_RELEASE(pSink);
        }
    }

    return HXR_OK;
}

void
CHXABDCalibrator::Close()
{
    Reset();
    DeleteServers();

    if (m_pAutoBWCalibrationSinkList)
    {
        CHXSimpleList::Iterator i;
        for(i=m_pAutoBWCalibrationSinkList->Begin();i!=m_pAutoBWCalibrationSinkList->End();++i)
        {
            IHXAutoBWCalibrationAdviseSink* pSink = (IHXAutoBWCalibrationAdviseSink*)(*i);
            HX_RELEASE(pSink);
        }
        HX_DELETE(m_pAutoBWCalibrationSinkList);
    }

    HX_DELETE(m_pInQueue);

    HX_RELEASE(m_pErrMsg);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pNetServices);
    HX_RELEASE(m_pContext);
}

HX_RESULT
CHXABDCalibrator::GetAddrInfoDone(HX_RESULT status, UINT32 nVecLen,
                                  IHXSockAddr** ppAddrVec)
{
    HX_RESULT           rc = HXR_OK;
    IHXSocketResponse*  pSocketResponse = NULL;
    IHXSockAddr**       ppConvSockAddr = NULL;

    if ((HXR_OK != status) || !ppAddrVec || !nVecLen)
    {
        rc = status;
        goto exit;
    }

    rc = m_pNetServices->CreateSocket(&m_pSocket);
    if (HXR_OK != rc)
    {
        goto exit;
    }

    // Get the response object's IHXSocketResponse interface
    rc = m_pABDResponse->QueryInterface(IID_IHXSocketResponse, (void**) &pSocketResponse);
    if (HXR_OK != rc)
    {
        goto exit;
    }

    // Set the response interface
    rc = m_pSocket->SetResponse(pSocketResponse);
    if (HXR_OK != rc)
    {
        goto exit;
    }

    // Initialize the socket
    rc = m_pSocket->Init(HX_SOCK_FAMILY_INANY,
                         HX_SOCK_TYPE_TCP,
                         HX_SOCK_PROTO_ANY);
    if (HXR_OK != rc)
    {
        goto exit;
    }

    rc = m_pSocket->SetOption(HX_SOCKOPT_RCVBUF, m_ulProbingPacketSize);
    if (HXR_OK != rc)
    {
        goto exit;
    }

    rc = m_pSocket->SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);
    if (HXR_OK != rc)
    {
        goto exit;
    }

    // Select the events we want
    m_pSocket->SelectEvents(HX_SOCK_EVENT_READ    |
                            HX_SOCK_EVENT_WRITE   |
                            HX_SOCK_EVENT_CONNECT |
                            HX_SOCK_EVENT_CLOSE);

    // Copy these (convert/filter to proper family supported by socket)
    rc = HXSockUtil::AllocAddrVec(ppAddrVec,
                                  nVecLen,
                                  ppConvSockAddr,
                                  m_pSocket->GetFamily(),
                                  true,
                                  m_pNetServices);

    if (HXR_OK != rc)
    {
        goto exit;
    }

    // Now call ConnectToAny
    rc = m_pSocket->ConnectToAny(nVecLen, ppConvSockAddr);
    if (HXR_OK != rc)
    {
        goto exit;
    }

exit:

    // Free the addresses we converted
    HXSockUtil::FreeAddrVec(ppConvSockAddr, nVecLen);
    HX_RELEASE(pSocketResponse);

    if (HXR_OK != rc)
    {
        ReportABDStatus(rc, 0);
    }

    return rc;
}

HX_RESULT
CHXABDCalibrator::GetNameInfoDone(HX_RESULT status, const char* pszNode,
                                  const char* pszService)
{
    return HXR_UNEXPECTED;
}

HX_RESULT
CHXABDCalibrator::EventPending(UINT32 uEvent, HX_RESULT status)
{
    HX_RESULT rc = HXR_OK;

    switch (uEvent)
    {
        case HX_SOCK_EVENT_READ:
            {
                // We've been notified that the socket
                // has data ready to be read. So read from the socket
                IHXBuffer* pReadBuffer = NULL;

                rc = m_pSocket->Read(&pReadBuffer);
                if (HXR_OK == rc)
                {
                    HandleSocketRead(rc, pReadBuffer);
                }
                else
                {
                    ReportABDStatus(rc, 0);
                }
                HX_RELEASE(pReadBuffer);
            }
            break;
        case HX_SOCK_EVENT_WRITE:
            break;
        case HX_SOCK_EVENT_CONNECT:
            {
                if (FAILED(status))
                {
                    ReportABDStatus(HXR_NET_CONNECT, 0);
                }
                else
                {
                    HandleSocketWrite();
                }
            }
            break;
        case HX_SOCK_EVENT_CLOSE:
            {
                ReportABDStatus(HXR_SERVER_DISCONNECTED, 0);
            }
            break;
        default:
            break;
    }

    return rc;
}

void
CHXABDCalibrator::Reset(void)
{
    UINT8       i = 0;

    if (m_ulABDTimeoutCBHandle)
    {
        m_pScheduler->Remove(m_ulABDTimeoutCBHandle);
        m_ulABDTimeoutCBHandle = 0;
    }
    HX_RELEASE(m_pABDTimeoutCB);

    for (i = 0; i < MAX_ABD_PROBPKT; i++)
    {
        if (!m_pABDProbPktInfo[i])
        {
            break;
        }
        HX_DELETE(m_pABDProbPktInfo[i]);
    }
    memset(&m_pABDProbPktInfo[0], 0, sizeof(void*) * MAX_ABD_PROBPKT);

    m_bABDPending = FALSE;
    m_bReadHeaderDone = FALSE;
    m_nProbingPacketsReceived = 0;
    memset(&m_ulProbingPacketsGap[0], 0, sizeof(UINT32) * MAX_ABD_PROBPKT);

    if (m_pSocket)
    {
        m_pSocket->Close();
    }

    if( m_pResolve )
    {
        m_pResolve->Close();
        HX_RELEASE(m_pResolve);
    }
    
    HX_RELEASE(m_pSocket);
    HX_RELEASE(m_pABDResponse);
}

HX_RESULT
CHXABDCalibrator::HandleSocketWrite()
{
    HX_RESULT               rc = HXR_OK;
    CHXString               szGetRequest;
    IHXBuffer*              pBuffer  = NULL;
    IHXCommonClassFactory*  pCCF = NULL;

    const char szAccept[] = "Accept: */*\r\n";
    const char szPragma[] = "Pragma: no-cache\r\n";

    szGetRequest = "GET /ABD HTTP/1.0\r\n";
    szGetRequest += szAccept;
    szGetRequest += szPragma;
    szGetRequest += "User-Agent: " + CHXString(USER_AGENT_STRING) + "10.0.0.0\r\n";
    szGetRequest += "AutoBWDetection: 1\r\n";

    AddABDHeader(szGetRequest, "AutoBWDetectionMode", m_nABDMode);
    AddABDHeader(szGetRequest, "AutoBWDetectionPackets",
                 m_nProbingPacketsRequested);
    AddABDHeader(szGetRequest, "AutoBWDetectionPacketSize",
                 m_ulProbingPacketSize);

    szGetRequest +="\r\n";

    if (HXR_OK != m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&pCCF))
    {
        rc = HXR_FAILED;
        goto exit;
    }

    rc = pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
    if (HXR_OK != rc)
    {
        goto exit;
    }

    rc = pBuffer->Set((UCHAR*)(const char*)szGetRequest, szGetRequest.GetLength());
    if (HXR_OK != rc)
    {
        goto exit;
    }

    rc = m_pSocket->Write(pBuffer);

exit:

    HX_RELEASE(pBuffer);
    HX_RELEASE(pCCF);
    return rc;
}

HX_RESULT
CHXABDCalibrator::HandleSocketRead(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT   rc = HXR_OK;
    char*       pEOH = NULL;
    char*       pBuf = NULL;
    HXBOOL        bGotData = FALSE;
    UINT32      ulAvailable = 0;
    UINT32      ulUsed = 0;
    UINT32      ulTS = 0;
    double      dwResult = 0;   
    TNGBWProbingPacket pkt;
    UINT32      ulPayloadSize = pkt.static_size();
    IHXTimeStampedBuffer* pTSBuffer = NULL;

    if (pBuffer)
    {
        if (0 == m_pInQueue->EnQueue(pBuffer->GetBuffer(), (UINT16)pBuffer->GetSize()))
        {
            rc = HXR_OUTOFMEMORY;
            goto exit;
        }
    }

    ulAvailable = m_pInQueue->GetQueuedItemCount();
    if (0 == ulAvailable)
    {
        goto exit;
    }

    pBuf = new char[ulAvailable];
    if (!pBuf)
    {
        rc = HXR_OUTOFMEMORY;
        goto exit;
    }

    for (;;)
    {
        if (0 == ulAvailable)
        {
            break;
        }

        m_pInQueue->DeQueue(pBuf, (UINT16)ulAvailable);

        if (!m_bReadHeaderDone)
        {
            // look for message header
            if ((pEOH = (char*) HXFindString(pBuf, "\n\n")) ||
                (pEOH = (char*) HXFindString(pBuf, "\r\r")) )
            {
                pEOH += 2;
                m_bReadHeaderDone = TRUE;
            }
            else if ((pEOH = (char*) HXFindString(pBuf, "\r\n\r\n")))
            {
                pEOH += 4;
                m_bReadHeaderDone = TRUE;
            }
            else
            {
                m_pInQueue->EnQueue(pBuf, (UINT16)ulAvailable);
                break;
            }

            if (m_bReadHeaderDone)
            {
                if (!HXFindString(pBuf, "HTTP/1.0 200 OK") ||
                    !HXFindString(pBuf, "AutoBWDetection: 1"))
                {
                    rc = HXR_NOT_SUPPORTED;
                    goto exit;
                }
            }

            m_pInQueue->EnQueue(pEOH, (UINT16)(ulAvailable - (pEOH - pBuf)));
        }
        else
        {
            assert(pBuf[0] == (char)0x80 && 
                   pBuf[1] == (char)0xFF &&
                   pBuf[2] == (char)0x0B);

            bGotData = FALSE;
            ulUsed = 0;

            if (ulAvailable >= ulPayloadSize)
            {
                UINT16  tcpDataLen = (UINT16)getshort((BYTE*)&pBuf[3]);
                UINT32  currentDataLen = ulAvailable;
                UINT32  timestamp = 0;

                if (currentDataLen >= tcpDataLen)
                {
                    m_pABDProbPktInfo[m_nProbingPacketsReceived] = new ABD_PROBPKT_INFO;

                    m_pABDProbPktInfo[m_nProbingPacketsReceived]->seq = (UINT8)pBuf[5];

                    timestamp = ((UINT8)pBuf[6])<<24; timestamp |= ((UINT8)pBuf[7])<<16;
	            timestamp |= ((UINT8)pBuf[8])<<8; timestamp |= ((UINT8)pBuf[9]);
                    m_pABDProbPktInfo[m_nProbingPacketsReceived]->sendTime = timestamp;

                    if(HXR_OK == pBuffer->QueryInterface(IID_IHXTimeStampedBuffer, (void **)&pTSBuffer))
                    {
                        ulTS = pTSBuffer->GetTimeStamp();
                        HX_RELEASE(pTSBuffer);
                    }
                    else
                    {
                        HX_ASSERT(FALSE);
                    }

                    ulUsed = tcpDataLen + ulPayloadSize;
                    ulAvailable -= ulUsed;

                    m_pABDProbPktInfo[m_nProbingPacketsReceived]->recvTime = ulTS;
                    m_pABDProbPktInfo[m_nProbingPacketsReceived]->dwSize = ulUsed;                   
                    m_nProbingPacketsReceived++;
                    
                    bGotData = TRUE;
                }
            }
            m_pInQueue->EnQueue(&pBuf[ulUsed], (UINT16)ulAvailable);

            if (!bGotData)
            {
                break;
            }
        }

        ulAvailable = m_pInQueue->GetQueuedItemCount();
    }

    if (m_nProbingPacketsReceived == m_nProbingPacketsRequested)
    {
        dwResult = CalculateABD(1, TCPMode, &m_pABDProbPktInfo[0], m_nProbingPacketsReceived);
        if (dwResult > 0.0)
        {
            ReportABDStatus(HXR_OK, (UINT32)dwResult);
        }
        else
        {
            rc = HXR_FAILED;
        }
    }

exit:

    HX_VECTOR_DELETE(pBuf);

    if (HXR_OK != rc)
    {
        ReportABDStatus(rc, 0);
    }

    return rc;
}

HX_RESULT
CHXABDCalibrator::HandleABDTimeout()
{
    return ReportABDStatus(HXR_TIMEOUT, 0);
}

HX_RESULT
CHXABDCalibrator::ReportABDStatus(HX_RESULT rc, UINT32 ulBW)
{
    DEBUG_OUT(m_pErrMsg, DOL_TRANSPORT, (s,"ABDCalibrator: %lu %luKbps", rc, ulBW));    

    if (m_bABDPending)
    {
        Reset();

        CHXSimpleList::Iterator i;
        if (m_pAutoBWCalibrationSinkList)
        {
            for(i=m_pAutoBWCalibrationSinkList->Begin();i!=m_pAutoBWCalibrationSinkList->End();++i)
            {
                IHXAutoBWCalibrationAdviseSink* pSink = (IHXAutoBWCalibrationAdviseSink*)(*i);
                pSink->AutoBWCalibrationDone(rc, ulBW);
            }
        }
    }

    return HXR_OK;
}

HX_RESULT
CHXABDCalibrator::DeleteServers(void)
{
    HX_RESULT   rc = HXR_OK;
    UINT8       i = 0;

    for (i = 0; i < MAX_ABD_SERVERS; i++)
    {
        if (!m_pABDServers[i])
        {
            break;
        }

        HX_DELETE(m_pABDServers[i]);
    }

    m_nABDServers = 0;

    return rc;
}

HX_RESULT
CHXABDCalibrator::ParseServers(const char* pszServers)
{
    HX_RESULT   rc = HXR_OK;
    UINT8       i = 0;
    INT32       column = -1;
    UINT32      ulPort = DEFAULT_ABD_SERVER_PORT;
    char*       pszToken = NULL;
    CHXString   pToken;
    CHXString*  pServer = NULL;
    CHXString*  pPort = NULL;

    pszToken = strtok((char*)pszServers, ",");

    while (pszToken)
    {
        ulPort = DEFAULT_ABD_SERVER_PORT;
        pToken = pszToken;

        // trim off the spaces
        pToken.TrimLeft();
        pToken.TrimRight();

        column = pToken.Find(':');
        if (-1 != column)
        {
            pServer = new CHXString(pToken.Left(column));
            pServer->TrimRight();

            pPort = new CHXString(pToken.Right(pToken.GetLength() - (column + 1)));
            pPort->TrimLeft();
            
            ulPort = atol((const char*)*pPort);
        }
        else
        {
            pServer = new CHXString(pToken);
        }
            
        m_pABDServers[i] = new ABD_SERVER_INFO;
        m_pABDServers[i]->pServer = pServer;
        m_pABDServers[i]->ulPort = ulPort;
    
        HX_DELETE(pPort);

        i++;
        pszToken = strtok(NULL, ",");
    }

    m_nABDServers = i;

    return rc;
}

CHXABDCalibrator::TimeoutCallback::TimeoutCallback
(
    CHXABDCalibrator* pOwner
) : m_lRefCount(0)
  , m_pOwner(pOwner)
{
}

CHXABDCalibrator::TimeoutCallback::~TimeoutCallback()
{
}

STDMETHODIMP
CHXABDCalibrator::TimeoutCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), this },
        { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*) this },
    };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32)
CHXABDCalibrator::TimeoutCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CHXABDCalibrator::TimeoutCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CHXABDCalibrator::TimeoutCallback::Func()
{
    if (m_pOwner)
    {
        m_pOwner->HandleABDTimeout();
    }

    return HXR_OK;
}

void CHXABDCalibrator::AddABDHeader(CHXString& mesg, 
                                    const char* pName, ULONG32 ulValue)
{
    mesg += pName;
    mesg += ": ";
    mesg.AppendULONG(ulValue);
    mesg += "\r\n";
}

BEGIN_INTERFACE_LIST(CHXABDResponse)
    INTERFACE_LIST_ENTRY(IID_IHXSocketResponse,  IHXSocketResponse)
    INTERFACE_LIST_ENTRY(IID_IHXResolveResponse, IHXResolveResponse)
END_INTERFACE_LIST

CHXABDResponse::CHXABDResponse()
               :m_pOwner(NULL)
{
}

void
CHXABDResponse::InitObject(CHXABDCalibrator* pOwner)
{
    m_pOwner = pOwner;
}

CHXABDResponse::~CHXABDResponse()
{
    m_pOwner = NULL;
}

STDMETHODIMP CHXABDResponse::GetAddrInfoDone(HX_RESULT status,
                                             UINT32 nVecLen,
                                             IHXSockAddr** ppAddrVec)
{
    return m_pOwner ? m_pOwner->GetAddrInfoDone(status, nVecLen, ppAddrVec) : HXR_OK;
}

STDMETHODIMP CHXABDResponse::GetNameInfoDone(HX_RESULT status,
                                             const char* pszNode,
                                             const char* pszService)
{
    return m_pOwner ? m_pOwner->GetNameInfoDone(status, pszNode, pszService) : HXR_OK;
}

STDMETHODIMP CHXABDResponse::EventPending(UINT32 uEvent, HX_RESULT status)
{
    return m_pOwner ? m_pOwner->EventPending(uEvent, status) : HXR_OK;
}
