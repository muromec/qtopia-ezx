/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rtspmcasttran.h,v 1.3 2006/12/21 05:12:30 tknox Exp $
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

#ifndef UT_RTSPSERVERMULTICAST_TRANSPORT_H
#define UT_RTSPSERVERMULTICAST_TRANSPORT_H

#include "cppunit/extensions/HelperMacros.h"

#include "hxcom.h"
#include "hxcomptr.h"

#include "ut_socket.h"
#include "ut_basepkt.h"
#include "ut_context.h"

class Transport;
class RTSPServerMulticastTransport;
class CUTContainerFactory;

typedef HXCOMPtr<RTSPServerMulticastTransport> SPRTSPServerMulticastTransport;

/** \class CUTRTSPServerMulticastTransportTestDriver
  * \brief CUTRTSPServerMulticastTransportTestDriver Unit test the RTSPServerMulticastTransport.
  * 
  * See also the unit test plan \ref "http://systems.dev.prognet.com/Products/Server/Projects/2006-planning/features/transportrefactor/unittestplan/"
  */
class CUTRTSPServerMulticastTransportTestDriver : public CppUnit::TestFixture
{
public:

    CUTRTSPServerMulticastTransportTestDriver ();
    virtual ~CUTRTSPServerMulticastTransportTestDriver ();

    virtual void setUp ();
    virtual void tearDown ();

    CPPUNIT_TEST_SUITE( CUTRTSPServerMulticastTransportTestDriver );
    CPPUNIT_TEST( testSendPacketNULLPacket );
    CPPUNIT_TEST( testSendPacketNoTransport );
    CPPUNIT_TEST( testSendPacketBadTransport );
    CPPUNIT_TEST( testSendPacketOneTransport );
    CPPUNIT_TEST( testSendPacketNTransports );
    CPPUNIT_TEST_SUITE_END();

    /**
      * \brief Test RTSPServerMulticastTransport::sendPacket with a NULL pointer.
      */
    void testSendPacketNULLPacket ();

    /**
      * \brief Test RTSPServerMulticastTransport::sendPacket with m_pTransports == NULL
      */
    void testSendPacketNoTransport ();

    /**
      * \brief Test RTSPServerMulticastTransport::sendPacket with bad values in pInfo
      */
    void testSendPacketBadTransport ();

    /**
      * \brief Test RTSPServerMulticastTransport::sendPacket with one transport in m_pTransports.
      */
    void testSendPacketOneTransport ();

    /**
      * \brief Test RTSPServerMulticastTransport::sendPacket with N > 1 transports in m_pTransports.
      */
    void testSendPacketNTransports ();


private:

    RTSPMcastInfo* CreateRTSPMcastInfo          (Transport* pTran,
                                                const char* pKey);

    SPRTSPServerMulticastTransport  m_spTransport;
    CHXMapStringToOb*               m_pTransports;
    SPCUTRTSPTransportResponse      m_spTransportResponse;
    SPCUTContext                    m_spContext;
    SPIHXCommonClassFactory         m_spCCF;
    SPCUTMockSocket                 m_spSocket;
    CUTContainerFactory*            m_pContFact;
    SPCUTBasePacket                 m_spPacket;
};

#endif /* UT_RTSPSERVERMULTICAST_TRANSPORT_H */
