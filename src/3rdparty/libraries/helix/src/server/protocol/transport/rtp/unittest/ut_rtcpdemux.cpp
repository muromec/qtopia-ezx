/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: 
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

#include "rtp_udp.h"

#include "ut_contfact.h"
#include "ut_rtsptranresp.h"
#include "ut_scheduler.h"
#include "ut_rtcpdemux.h"
#include "rtcp_demux.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( CUTRTCPDemuxTestDriver );

CUTRTCPDemuxTestDriver::CUTRTCPDemuxTestDriver()
    : m_spMockTransA()
    , m_spMockTransB()
    , m_spDemux()
    , m_spContext()
    , m_spCCF()
    , m_pContFact(NULL)
{
    m_spContext     = new CUTContext();
    m_spCCF.Query (m_spContext.Ptr());

    m_pContFact     = new CUTContainerFactory(m_spCCF.Ptr ());
    m_spMockTransA  = new CUTMockServUDPTrans();
    m_spMockTransB  = new CUTMockServUDPTrans();
}

CUTRTCPDemuxTestDriver::~CUTRTCPDemuxTestDriver()
{
    HX_DELETE(m_pContFact);
    m_spContext->Close();
}

void
CUTRTCPDemuxTestDriver::setUp()
{
    m_spDemux           = new ServerRTCPDemux();
    m_spMockTransA->Reset();
    m_spMockTransA->Reset();

    // compound RTCP packet for demux to handle
    m_pPackerA          = new RTCPPacker();
    m_pPackerB          = new RTCPPacker();

    // Reception Report packets
    m_pRRPacketA        = new RTCPPacket();
    m_pRRPacketB        = new RTCPPacket();
    m_pRRPacketA->version_flag = 2;
    m_pRRPacketA->packet_type = 201;
    m_pRRPacketB->version_flag = 2;
    m_pRRPacketB->packet_type = 201;
    m_pRR               = new ReceptionReport();

    // Source Description packet
    m_pSDESPacket       = new RTCPPacket();
    m_pSDESPacket->version_flag = 2;
    m_pSDESPacket->packet_type = 202;
}

void
CUTRTCPDemuxTestDriver::tearDown()
{
    HX_DELETE(m_pPackerA);
    HX_DELETE(m_pPackerB);
    HX_DELETE(m_pRRPacketA);
    HX_DELETE(m_pRRPacketB);
    HX_DELETE(m_pRR);
    HX_DELETE(m_pSDESPacket);
}

void
CUTRTCPDemuxTestDriver::testPacketNoTransports()
{
    m_pRRPacketA->count = 1;
    m_pRRPacketA->length = 7;
    m_pRR->ssrc = 2062007128;
    m_pRRPacketA->SetReceiverReport(m_pRR, 1);

    SPIHXBuffer spBuf = new CHXBuffer;
    IHXBuffer* pBuf = spBuf.Ptr();
    m_pPackerA->Set(m_pRRPacketA);
    m_pPackerA->Set(m_pSDESPacket);
    m_pPackerA->Pack(pBuf);

    CPPUNIT_ASSERT(FAILED(m_spDemux->HandlePacket(spBuf)));
}

void
CUTRTCPDemuxTestDriver::testPacketBadSSRC()
{
    m_spDemux->AddTransport(2062007128, m_spMockTransA.Ptr());
    m_spDemux->AddTransport(1234567890, m_spMockTransB.Ptr());

    m_pRRPacketA->count = 1;
    m_pRRPacketA->length = 7;
    m_pRR->ssrc = 3333333333;
    m_pRRPacketA->SetReceiverReport(m_pRR, 1);

    SPIHXBuffer spBuf = new CHXBuffer;
    IHXBuffer* pBuf = spBuf.Ptr();
    m_pPackerA->Set(m_pRRPacketA);
    m_pPackerA->Set(m_pSDESPacket);
    m_pPackerA->Pack(pBuf);

    CPPUNIT_ASSERT(FAILED(m_spDemux->HandlePacket(spBuf)));
    CPPUNIT_ASSERT(!m_spMockTransA->GotPacket());
    CPPUNIT_ASSERT(!m_spMockTransB->GotPacket());
}

void
CUTRTCPDemuxTestDriver::testPacketGoodSSRC()
{
    m_spDemux->AddTransport(2062007128, m_spMockTransA.Ptr());
    m_spDemux->AddTransport(1234567890, m_spMockTransB.Ptr());

    m_pRRPacketA->count = 1;
    m_pRRPacketA->length = 7;
    m_pRR->ssrc = 2062007128;
    m_pRRPacketA->SetReceiverReport(m_pRR, 1);

    SPIHXBuffer spBuf = new CHXBuffer;
    IHXBuffer* pBuf = spBuf.Ptr();
    m_pPackerA->Set(m_pRRPacketA);
    m_pPackerA->Set(m_pSDESPacket);
    m_pPackerA->Pack(pBuf);

    CPPUNIT_ASSERT(SUCCEEDED(m_spDemux->HandlePacket(spBuf)));
    CPPUNIT_ASSERT(m_spMockTransA->GotPacket());
    CPPUNIT_ASSERT(!m_spMockTransB->GotPacket());
}

void
CUTRTCPDemuxTestDriver::test2Packets2GoodSSRCs()
{
    m_spDemux->AddTransport(2062007128, m_spMockTransA.Ptr());
    m_spDemux->AddTransport(1234567890, m_spMockTransB.Ptr());
    SPIHXBuffer spBuf = new CHXBuffer;
    IHXBuffer* pBuf = spBuf.Ptr();
 
    // first packet
    m_pRRPacketA->count = 1;
    m_pRRPacketA->length = 7;
    m_pRR->ssrc = 2062007128;
    m_pRRPacketA->SetReceiverReport(m_pRR, 1);
    
    m_pPackerA->Set(m_pRRPacketA);
    m_pPackerA->Set(m_pSDESPacket);
    m_pPackerA->Pack(pBuf);

    CPPUNIT_ASSERT(SUCCEEDED(m_spDemux->HandlePacket(spBuf)));
    CPPUNIT_ASSERT(m_spMockTransA->GotPacket());
    CPPUNIT_ASSERT(!m_spMockTransB->GotPacket());

    m_spMockTransA->Reset();
    m_spMockTransB->Reset();
    
    // second packet
    m_pRRPacketB->count = 1;
    m_pRRPacketB->length = 7;
    m_pRR->ssrc = 1234567890;
    m_pRRPacketB->SetReceiverReport(m_pRR, 1);

    m_pPackerB->Set(m_pRRPacketB);
    m_pPackerB->Set(m_pSDESPacket);
    m_pPackerB->Pack(pBuf);

    CPPUNIT_ASSERT(SUCCEEDED(m_spDemux->HandlePacket(spBuf)));
    CPPUNIT_ASSERT(!m_spMockTransA->GotPacket());
    CPPUNIT_ASSERT(m_spMockTransB->GotPacket());
}

CUTMockServUDPTrans::CUTMockServUDPTrans() 
    : ServerRTCPUDPTransport(TRUE)
    , m_bHandlePacketListCalled(FALSE)
{}

HX_RESULT
CUTMockServUDPTrans::handlePacketList(CHXSimpleList* pList, IHXBuffer* pBuffer)
{
    m_bHandlePacketListCalled = TRUE;

    RTCPPacket* pPkt;
    while (!pList->IsEmpty())
    {
        pPkt = (RTCPPacket*)pList->RemoveHead();
        HX_DELETE(pPkt);
    }

    return HXR_OK;
}

BOOL 
CUTMockServUDPTrans::GotPacket()
{
    return m_bHandlePacketListCalled; 
}

void 
CUTMockServUDPTrans::Reset() 
{ 
    m_bHandlePacketListCalled = FALSE;
}
