/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rtcptcptran.h,v 1.2 2006/10/12 22:46:11 tknox Exp $
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

#ifndef UT_RTCP_TCP_TRANSPORT_H
#define UT_RTCP_TCP_TRANSPORT_H

#include "cppunit/extensions/HelperMacros.h"

#include "hxcom.h"
#include "hxcomptr.h"

#include "ihxpckts.h"

#include "ut_basepkt.h"
#include "ut_socket.h"
#include "ut_context.h"

class CUTRTSPTransportResponse;
class CUTContainerFactory;

typedef HXCOMPtr<ServerRTPTCPTransport>    SPServerRTPTCPTransport;
typedef HXCOMPtr<ServerRTCPTCPTransport>   SPServerRTCPTCPTransport;
typedef HXCOMPtr<CUTRTSPTransportResponse> SPCUTRTSPTransportResponse;

class CUTServerRTCPTCPTransportTestDriver : public CppUnit::TestFixture
{
public:

    CUTServerRTCPTCPTransportTestDriver ();
    virtual ~CUTServerRTCPTCPTransportTestDriver ();

    virtual void setUp ();
    virtual void tearDown ();

    CPPUNIT_TEST_SUITE( CUTServerRTCPTCPTransportTestDriver );
    CPPUNIT_TEST( testWritePacketNULLBuffer );
    CPPUNIT_TEST( testWritePacketEmptyBuffer );
    CPPUNIT_TEST( testWritePacketTooFullBuffer );
    CPPUNIT_TEST( testWritePacketJustRightBuffer );
    CPPUNIT_TEST_SUITE_END();

    void testWritePacketNULLBuffer ();
    void testWritePacketEmptyBuffer ();
    void testWritePacketTooFullBuffer ();
    void testWritePacketJustRightBuffer ();

private:

    SPServerRTPTCPTransport             m_spBaseTransport;
    SPServerRTCPTCPTransport            m_spTransport;
    SPCUTRTSPTransportResponse          m_spTransportResponse;
    SPCUTContext                        m_spContext;
    SPIHXCommonClassFactory             m_spCCF;
    SPCUTMockSocket                     m_spSocket;
    SPIHXBuffer                         m_spBuffer;
    CUTContainerFactory*                m_pContFact;
};

#endif /* UT_RTCP_TCP_TRANSPORT_H */
