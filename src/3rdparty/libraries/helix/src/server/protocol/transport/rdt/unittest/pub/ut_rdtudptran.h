/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rdtudptran.h,v 1.3 2006/12/18 18:42:13 tknox Exp $
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

#ifndef UT_RDT_UDP_TRANSPORT_H
#define UT_RDT_UDP_TRANSPORT_H

#include "cppunit/extensions/HelperMacros.h"

#include "hxcom.h"
#include "hxcomptr.h"

#include "ihxpckts.h"

#include "ut_basepkt.h"
#include "ut_socket.h"
#include "ut_context.h"
#include "ut_rdtvptran.h"

class CUTRTSPTransportResponse;
class CUTContainerFactory;

typedef HXCOMPtr<RDTUDPVectorPacketTransport> SPRDTUDPVectorPacketTransport;
typedef HXCOMPtr<CUTRTSPTransportResponse> SPCUTRTSPTransportResponse;

/** \class CUTRDTUDPTransportTestDriver
  * \brief CUTRDTUDPTransportTestDriver Unit test the RDTUDPTransport.
  * 
  * See also the unit test plan \ref "http://systems.dev.prognet.com/Products/Server/Projects/2006-planning/features/transportrefactor/unittestplan/"
  */
class CUTRDTUDPTransportTestDriver : public CppUnit::TestFixture
{
public:

    CUTRDTUDPTransportTestDriver ();
    virtual ~CUTRDTUDPTransportTestDriver ();

    virtual void setUp ();
    virtual void tearDown ();

    CPPUNIT_TEST_SUITE( CUTRDTUDPTransportTestDriver );
    CPPUNIT_TEST( testWritePacketMulticast );
    CPPUNIT_TEST( testWritePacketNonMulticast );
    CPPUNIT_TEST( testSendPacketAfterDone );
    //CPPUNIT_TEST( testSendPacketNullPointer );
    //CPPUNIT_TEST( testSendPacketEmptyPacket );
    CPPUNIT_TEST( testSendPacketsAfterDone );
    //CPPUNIT_TEST( testSendPacketsNullPointer );
    //CPPUNIT_TEST( testSendPacketsNoPackets );
    //CPPUNIT_TEST( testSendPacketsEmptyPacket );
    CPPUNIT_TEST( testSendPacketsTooManyPackets );
    CPPUNIT_TEST( testSendPacketsOnePacket );
    CPPUNIT_TEST( testSendPacketsEnoughPackets );
    CPPUNIT_TEST_SUITE_END();

    /**
      * \brief Test RDTUDPTransport::writePacket for multicast.
      */
    void testWritePacketMulticast ();

    /**
      * \brief Test RDTUDPTransport::writePacket for non-multicast.
      */
    void testWritePacketNonMulticast ();

    /**
      * \brief Test RDTUDPTransport::writePacket for call after m_bDone is set.
      */
    void testSendPacketAfterDone ();

    /**
      * \brief Test RDTUDPTransport::writePacket with a NULL pointer.
      */
    void testSendPacketNullPointer ();

    /**
      * \brief Test RDTUDPTransport::writePacket with an empty buffer.
      */
    void testSendPacketEmptyPacket ();

    /**
      * \brief Test RDTUDPTransport::sendPackets for a call after m_bDone is set.
      */
    void testSendPacketsAfterDone ();

    /**
      * \brief Test RDTUDPTransport::sendPackets with a NULL pointer.
      */
    void testSendPacketsNullPointer ();

    /**
      * \brief Test RDTUDPTransport::sendPackets with a pointer to NULL.
      */
    void testSendPacketsNoPackets ();

    /**
      * \brief Test RDTUDPTransport::sendPackets with a vector consisting of 1 empty packet.
      */
    void testSendPacketsEmptyPacket ();

    /**
      * \brief Test RDTUDPTransport::sendPackets with too many packets.
      */
    void testSendPacketsTooManyPackets ();

    /**
      * \brief Test RDTUDPTransport::sendPackets with a vector of just one packet.
      */
    void testSendPacketsOnePacket ();

    /**
      * \brief Test RDTUDPTransport::sendPackets with a vector of more than one but not too many packets.
      */
    void testSendPacketsEnoughPackets ();

private:

    SPRDTUDPVectorPacketTransport       m_spTransport;
    SPIHXBuffer                         m_spBuffer;
    SPCUTRTSPTransportResponse          m_spTransportResponse;
    SPCUTContext                        m_spContext;
    SPIHXCommonClassFactory             m_spCCF;
    SPCUTMockSocket                     m_spSocket;
    CUTContainerFactory*                m_pContFact;
};

#endif /* UT_RDT_UDP_TRANSPORT_H */
