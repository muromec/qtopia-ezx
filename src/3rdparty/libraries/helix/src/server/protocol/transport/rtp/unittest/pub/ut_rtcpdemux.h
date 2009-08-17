/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rtcpdemux.h,v 1.1 2007/01/31 18:07:28 seansmith Exp $
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
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef UT_RTP_DEMUX_H
#define UT_RTP_DEMUX_H

#include "cppunit/extensions/HelperMacros.h"

#include "hxcom.h"
#include "hxcomptr.h"

#include "ihxpckts.h"
#include "bufnum.h"
#include "pkthndlr.h"
#include "rtpwrap.h"

#include "ut_basepkt.h"
#include "ut_socket.h"
#include "ut_context.h"

class ServerRTCPDemux;
class CUTContainerFactory;
class CUTMockServUDPTrans;

typedef HXCOMPtr<CUTMockServUDPTrans> SPCUTMockServUDPTrans;
typedef HXCOMPtr<CUTRTSPTransportResponse> SPCUTRTSPTransportResponse;
typedef HXCOMPtr<ServerRTCPDemux> SPServerRTCPDemux;


class CUTRTCPDemuxTestDriver : public CppUnit::TestFixture
{
public:
    CUTRTCPDemuxTestDriver();
    virtual ~CUTRTCPDemuxTestDriver();

    virtual void setUp();
    virtual void tearDown();

    CPPUNIT_TEST_SUITE(CUTRTCPDemuxTestDriver);
    CPPUNIT_TEST(testPacketNoTransports);
    CPPUNIT_TEST(testPacketBadSSRC);
    CPPUNIT_TEST(testPacketGoodSSRC);
    CPPUNIT_TEST(test2Packets2GoodSSRCs);
    CPPUNIT_TEST_SUITE_END();

    void testPacketNoTransports();
    void testPacketBadSSRC();
    void testPacketGoodSSRC();
    void test2Packets2GoodSSRCs();

private:
    SPServerRTCPDemux               m_spDemux;
    SPCUTMockServUDPTrans           m_spMockTransA;
    SPCUTMockServUDPTrans           m_spMockTransB;
    SPCUTContext                    m_spContext;
    SPIHXCommonClassFactory         m_spCCF;
    CUTContainerFactory*            m_pContFact;

    // per test items
    RTCPPacker*                     m_pPackerA;
    RTCPPacker*                     m_pPackerB;
    RTCPPacket*                     m_pRRPacketA;
    RTCPPacket*                     m_pRRPacketB;
    ReceptionReport*                m_pRR;
    RTCPPacket*                     m_pSDESPacket;
};


class CUTMockServUDPTrans : public ServerRTCPUDPTransport
{
public:
    CUTMockServUDPTrans();

    HX_RESULT handlePacketList(CHXSimpleList* pList, IHXBuffer* pBuffer);
    BOOL GotPacket();
    void Reset();

private:
    BOOL    m_bHandlePacketListCalled;
};


#endif /* UT_RTP_UDP_TRANSPORT_H */
