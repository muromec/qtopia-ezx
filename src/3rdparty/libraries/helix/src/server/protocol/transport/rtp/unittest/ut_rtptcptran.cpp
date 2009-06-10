/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rtptcptran.cpp,v 1.5 2006/12/21 19:16:22 tknox Exp $
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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
 * "Contributor"(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "hxslist.h"
#include "chxmaplongtoobj.h"
#include "hxnet.h"
#include "hxtick.h"
#include "random32.h"
#include "sockio.h" // For HX_IOV_MAX
#include "basepkt.h"
#include "rtspif.h"
#include "ntptime.h"    // Evil dependency
#include "rtcputil.h"
#include "rtsptran.h"

#include "hxmarsh.h"

#include "source.h"
#include "sink.h"

#include "rtp_tcp.h"
#include "timeval.h"

#include "ut_contfact.h"
#include "ut_rtsptranresp.h"
#include "ut_scheduler.h"
#include "ut_rtptcptran.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( CUTServerRTPTCPTransportTestDriver );

CUTServerRTPTCPTransportTestDriver::CUTServerRTPTCPTransportTestDriver ()
    : m_spTransport ()
    , m_spTransportResponse ()
    , m_spContext ()
    , m_spCCF ()
    , m_spSocket ()
    , m_spBuffer ()
{
    m_spContext = new CUTContext ();
    m_spCCF.Query (m_spContext.Ptr ());

    m_pContFact = new CUTContainerFactory (m_spCCF.Ptr ());

    m_pContFact->CreateBuffer (m_spBuffer, 8, 'l');
}

CUTServerRTPTCPTransportTestDriver::~CUTServerRTPTCPTransportTestDriver ()
{
    delete m_pContFact;
    m_spContext->Close();
}

void
CUTServerRTPTCPTransportTestDriver::setUp ()
{
    m_spTransport          = new ServerRTPTCPTransport(TRUE);
    m_spTransportResponse  = new CUTRTSPTransportResponse();
    m_spSocket             = new CUTMockSocket();

    m_spTransport->init(static_cast<IUnknown*>(m_spContext.Ptr()),
            static_cast<IHXSocket*>(m_spSocket.Ptr()),
            m_spTransportResponse.Ptr());
}

void
CUTServerRTPTCPTransportTestDriver::tearDown ()
{
    HX_DELETE(m_spTransport->m_pReportHandler);
    m_spTransport->Done();
    m_spTransportResponse.Clear();
}

void
CUTServerRTPTCPTransportTestDriver::testWritePacketNULLBuffer ()
{
    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->writePacket(static_cast<IHXBuffer*>(0)) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTPTCPTransportTestDriver::testWritePacketEmptyBuffer ()
{
    SPIHXBuffer spHeadBuffer(m_spCCF.Ptr(), CLSID_IHXBuffer);
    char pHead [4] = {'$', 0xff, 0x0, 0x0};
    m_pContFact->CreateBuffer(spHeadBuffer, pHead, sizeof(pHead));

    SPIHXBuffer spEmptyBuffer(m_spCCF.Ptr(), CLSID_IHXBuffer);

    IHXBuffer* ppBufferVector[2];
    ppBufferVector[0] = spHeadBuffer.Ptr();
    ppBufferVector[1] = spEmptyBuffer.Ptr();

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITEV);
    m_spSocket->setExpectedArg(2UL); // Size of the Vector, 1 hdr, 1 pkt
    m_spSocket->setExpectedArg(const_cast<const IHXBuffer**>(static_cast<IHXBuffer**>(&ppBufferVector[0])));

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->writePacket(spEmptyBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTPTCPTransportTestDriver::testWritePacketTooFullBuffer ()
{
    const UINT32 dataLen = 0x10000;
    SPIHXBuffer spTooFullBuffer;
    m_pContFact->CreateBuffer (spTooFullBuffer, dataLen, 'a');

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->writePacket(spTooFullBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTPTCPTransportTestDriver::testWritePacketJustRightBufferNonBlocking ()
{
    const UINT32 dataLen = 0x1000;
    SPIHXBuffer spJustRightBuffer;
    m_pContFact->CreateBuffer (spJustRightBuffer, dataLen, 'n');

    const UINT32 headerLen = 0x04;
    char header[headerLen];
    header[0] = '$';
    header[1] = m_spTransport->m_tcpInterleave;
    header[2] = 0x10;
    header[3] = 0x00;

    SPIHXBuffer spHeaderBuffer;
    m_pContFact->CreateBuffer (spHeaderBuffer, header, headerLen);

    const UINT32 vectorLen = 2;
    const IHXBuffer* pBufferVector[vectorLen];
    pBufferVector[0] = spHeaderBuffer.Ptr();
    pBufferVector[1] = spJustRightBuffer.Ptr();

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITEV);
    m_spSocket->setExpectedArg(&pBufferVector[0]);
    m_spSocket->setExpectedArg(vectorLen);

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->writePacket(spJustRightBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTPTCPTransportTestDriver::testWritePacketJustRightBufferBlocking ()
{
    const UINT32 dataLen = 0x1000;
    SPIHXBuffer spJustRightBuffer;
    m_pContFact->CreateBuffer (spJustRightBuffer, dataLen, 'e');

    const UINT32 headerLen = 0x04;
    char header[headerLen];
    header[0] = '$';
    header[1] = m_spTransport->m_tcpInterleave;
    header[2] = 0x10;
    header[3] = 0x00;

    SPIHXBuffer spHeaderBuffer;
    m_pContFact->CreateBuffer (spHeaderBuffer, header, headerLen);

    const UINT32 vectorLen = 2;
    const IHXBuffer* pBufferVector[vectorLen];
    pBufferVector[0] = spHeaderBuffer.Ptr();
    pBufferVector[1] = spJustRightBuffer.Ptr();

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITEV, HXR_SOCK_BUFFERED);
    m_spSocket->setExpectedArg(&pBufferVector[0]);
    m_spSocket->setExpectedArg(vectorLen);

    CPPUNIT_ASSERT( m_spTransport->writePacket(spJustRightBuffer.Ptr()) == HXR_BLOCKED );

    CPPUNIT_ASSERT( m_spTransport->m_bBlocked );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTPTCPTransportTestDriver::testSendPacketAfterDone ()
{
    m_spTransport->m_bDone = TRUE;

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->sendPacket(static_cast<BasePacket*>(0)) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTPTCPTransportTestDriver::testSendPacketReflectPacket ()
{
    m_spTransport->m_ulPayloadWirePacket = 1;
    m_spTransport->m_pStreamHandler = new TransportStreamHandler(FALSE);
	m_spTransport->m_pStreamHandler->AddRef();
    m_spTransport->m_pStreamHandler->initStreamData(0           //UINT16 streamNumber,
                                     , 0                        //UINT16 streamGroupNumber,
                                     , FALSE                    //HXBOOL needReliable,
                                     , TRUE                     //HXBOOL bIsSource,
                                     , 0                        //INT16 rtpPayloadType,
                                     , FALSE                    //HXBOOL bPushData,
                                     , 0                        //UINT32 wrapSequenceNumber,
                                     , 0                        //UINT32 ulBufferDepth,
                                     , FALSE                    //HXBOOL bHasOutOfOrderTS = FALSE,
                                     , NULL                     //CHXTimestampConverter* pTSConverter = NULL,
                                     , RTSPMEDIA_TYPE_UNKNOWN   //RTSPMediaType eMediaType = RTSPMEDIA_TYPE_UNKNOWN
                                     );

    const UINT32 dataLen = 0x1000;
    SPIHXBuffer spJustRightBuffer;
    m_pContFact->CreateBuffer (spJustRightBuffer, dataLen, 'a');
    //spJustRightBuffer->AddRef();

    const UINT32 headerLen = 0x04;
    char header[headerLen];
    header[0] = '$';
    header[1] = m_spTransport->m_tcpInterleave;
    header[2] = 0x10;
    header[3] = 0x00;

    SPIHXBuffer spHeaderBuffer;
    m_pContFact->CreateBuffer (spHeaderBuffer, header, headerLen);
    //spHeaderBuffer->AddRef();

    const UINT32 vectorLen = 2;
    const IHXBuffer* pBufferVector[vectorLen];
    pBufferVector[0] = spHeaderBuffer.Ptr();
    pBufferVector[1] = spJustRightBuffer.Ptr();

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITEV);
    m_spSocket->setExpectedArg(&pBufferVector[0]);
    m_spSocket->setExpectedArg(vectorLen);

    SPCUTBasePacket spPacket;
    m_pContFact->CreateBasePacket (spPacket, spJustRightBuffer);

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->sendPacket(spPacket.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTPTCPTransportTestDriver::testSendPacketNonReflectPacket ()
{
    const UINT16 streamNumber = 0;
    m_spTransport->m_ulPayloadWirePacket = 0;
    m_spTransport->m_pStreamHandler = new TransportStreamHandler(FALSE);
    m_spTransport->m_pStreamHandler->AddRef();
    m_spTransport->m_pStreamHandler->initStreamData(streamNumber //UINT16 streamNumber,
                                     , 0                        //UINT16 streamGroupNumber,
                                     , FALSE                    //HXBOOL needReliable,
                                     , TRUE                     //HXBOOL bIsSource,
                                     , 0                        //INT16 rtpPayloadType,
                                     , FALSE                    //HXBOOL bPushData,
                                     , 0                        //UINT32 wrapSequenceNumber,
                                     , 0                        //UINT32 ulBufferDepth,
                                     , FALSE                    //HXBOOL bHasOutOfOrderTS = FALSE,
                                     , NULL                     //CHXTimestampConverter* pTSConverter = NULL,
                                     , RTSPMEDIA_TYPE_UNKNOWN   //RTSPMediaType eMediaType = RTSPMEDIA_TYPE_UNKNOWN
                                     );
    UINT32 ulTicks = random32((int)HX_GET_TICKCOUNT());
    m_spTransport->m_pReportHandler->SetSSRC (ulTicks);

    m_spTransport->m_pFirstPlayTime = new Timeval (0, 0);

    SPCUTMockSocket spRTCPSocket = new CUTMockSocket();
    ServerRTCPTCPTransport* pRTCPTran = new ServerRTCPTCPTransport(TRUE);
    pRTCPTran->init(m_spContext.Ptr()
            , spRTCPSocket.Ptr()
            , m_spTransport.Ptr()
            , m_spTransportResponse.Ptr()
            , streamNumber
            );
    m_spTransport->setRTCPTransport(pRTCPTran);

    const UINT32 senderLen = 0x04;
    BYTE sender[senderLen];
    sender[0] = '$';
    sender[1] = m_spTransport->m_tcpInterleave;
    sender[2] = 0x00;
    sender[3] = '4';

    spRTCPSocket->Reset();
    spRTCPSocket->setExpectedMethod (CUTMockSocket::FTC_WRITE_MULTIPLE);
    spRTCPSocket->setExpectedArg (&sender[0]);
    spRTCPSocket->setExpectedArg (senderLen);

    const UINT32 extraHeaderSize = 0x0c;
    const UINT32 dataLen = 0x1000 + extraHeaderSize;
    char *buffer = new char[dataLen];
    memset (buffer, 'j', dataLen);
    buffer[0] = 0x80;
    buffer[1] = 0x7f;
    for (int ii = 2; ii < extraHeaderSize; ++ii)
    {
        buffer[ii] = 0x00;
    }
    putlong(reinterpret_cast<UINT8*>(buffer+8), ulTicks);

    SPIHXBuffer spExpectedBuffer;
    m_pContFact->CreateBuffer (spExpectedBuffer, buffer, dataLen);
    //spExpectedBuffer->AddRef();

    SPIHXBuffer spJustRightBuffer;
    m_pContFact->CreateBuffer (spJustRightBuffer, buffer + extraHeaderSize, dataLen - extraHeaderSize);

    const UINT32 headerLen = 0x04;
    char header[headerLen];
    header[0] = '$';
    header[1] = m_spTransport->m_tcpInterleave;
    header[2] = 0x10;
    header[3] = 0x0c;

    SPIHXBuffer spHeaderBuffer;
    m_pContFact->CreateBuffer (spHeaderBuffer, header, headerLen);

    const UINT32 vectorLen = 2;
    const IHXBuffer* pBufferVector[vectorLen];
    pBufferVector[0] = spHeaderBuffer.Ptr();
    pBufferVector[1] = spExpectedBuffer.Ptr();

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITEV);
    m_spSocket->setExpectedArg(&pBufferVector[0]);
    m_spSocket->setExpectedArg(vectorLen);

    SPCUTBasePacket spPacket;
    m_pContFact->CreateBasePacket (spPacket, spJustRightBuffer);

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->sendPacket(spPacket.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );

    delete [] buffer;
    pRTCPTran->m_bSendBye = FALSE;
    pRTCPTran->Done();

    //The transport will delete the RTCP transport.
    //delete pRTCPTran;

    // The RTCP transport deletes this handler, so don't do it in teardown!
    m_spTransport->m_pReportHandler = NULL;
}

