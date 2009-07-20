/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rdtudptran.cpp,v 1.6 2007/01/11 02:22:57 tknox Exp $
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

#include "hxslist.h"
#include "chxmaplongtoobj.h"
#include "hxnet.h"
#include "sockio.h" // For HX_IOV_MAX
#include "basepkt.h"
#include "rtspif.h"
#include "rtsptran.h"

#include "ut_contfact.h"

#include "rdt_udp.h"
#include "ut_rdtvptran.h"

#include "ut_rtsptranresp.h"
#include "ut_rdtudptran.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( CUTRDTUDPTransportTestDriver );

CUTRDTUDPTransportTestDriver::CUTRDTUDPTransportTestDriver ()
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
}

CUTRDTUDPTransportTestDriver::~CUTRDTUDPTransportTestDriver ()
{
    delete m_pContFact;
    m_spContext->Close();
    m_spSocket.Clear();
}

void
CUTRDTUDPTransportTestDriver::setUp ()
{
    m_spTransport          = new RDTUDPVectorPacketTransport(TRUE, TRUE, RTSP_TR_RDT_UDP, 0, FALSE);
    m_spTransportResponse  = new CUTRTSPTransportResponse();
    m_spSocket             = new CUTMockSocket();

    m_pContFact->CreateBuffer (m_spBuffer, 8, 'd');

    m_spTransport->init(static_cast<IUnknown*>(m_spContext.Ptr()),
            static_cast<IHXSocket*>(m_spSocket.Ptr()),
            m_spTransportResponse.Ptr());

    m_spTransport->m_pStreamHandler = new TransportStreamHandler(m_spTransport.Ptr());
    m_spTransport->m_pStreamHandler->AddRef();
    m_spTransport->m_pStreamHandler->initStreamData(0, 0, FALSE, TRUE,
            0, 0, 0x10000, 0, FALSE, NULL,  RTSPMEDIA_TYPE_UNKNOWN);
}

void
CUTRDTUDPTransportTestDriver::tearDown ()
{
    HX_RELEASE( m_spTransport->m_pStreamHandler );
    m_spTransport->Done();
    m_spTransport.Clear();
    m_spTransportResponse.Clear();
}

void
CUTRDTUDPTransportTestDriver::testWritePacketMulticast ()
{
    m_spTransport->m_bMulticast = TRUE;

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITETO);
    m_spSocket->setExpectedArg(m_spBuffer->GetBuffer());
    m_spSocket->setExpectedArg(m_spBuffer->GetSize());
    m_spSocket->setExpectedArg(static_cast<IHXSockAddr*>(0));

    m_spBuffer->AddRef();       // writePacket assumes it can release the buffer

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->writePacket(m_spBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTUDPTransportTestDriver::testWritePacketNonMulticast ()
{
    m_spTransport->m_bMulticast = FALSE;

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITE);
    m_spSocket->setExpectedArg(m_spBuffer->GetBuffer());
    m_spSocket->setExpectedArg(m_spBuffer->GetSize());

    m_spBuffer->AddRef();       // writePacket assumes it can release the buffer

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->writePacket(m_spBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTUDPTransportTestDriver::testSendPacketAfterDone ()
{
    m_spTransport->m_bDone = TRUE;

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->sendPacket(static_cast<BasePacket*>(0)) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );

    m_spTransport->m_bDone = FALSE;
}

void
CUTRDTUDPTransportTestDriver::testSendPacketNullPointer ()
{
    BasePacket* pBaseNull = 0;

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->sendPacket(pBaseNull) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTUDPTransportTestDriver::testSendPacketEmptyPacket ()
{
    SPCUTBasePacket spEmptyPacket = new CUTBasePacket;
    spEmptyPacket->SetPacket(static_cast<IHXPacket*>(0));

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->sendPacket(spEmptyPacket.Ptr()) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTUDPTransportTestDriver::testSendPacketsAfterDone ()
{
    m_spTransport->m_bDone = TRUE;
    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->sendPackets(static_cast<BasePacket**>(0)) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );

    m_spTransport->m_bDone = FALSE;
}

void
CUTRDTUDPTransportTestDriver::testSendPacketsNullPointer ()
{
    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->sendPackets(static_cast<BasePacket**>(0)) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTUDPTransportTestDriver::testSendPacketsNoPackets ()
{
    BasePacket* pBaseNull = 0;

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->sendPackets(&pBaseNull) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTUDPTransportTestDriver::testSendPacketsEmptyPacket ()
{
    SPCUTBasePacket spEmptyPacket = new CUTBasePacket();
    BasePacket* pPacketVec[2];
    pPacketVec[0] = spEmptyPacket.Ptr();
    pPacketVec[1] = 0;

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->sendPackets(&pPacketVec[0]) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTUDPTransportTestDriver::testSendPacketsTooManyPackets ()
{
    static const int nIOVExtra = 2;
    static const int nMaxPackets = HX_IOV_MAX + nIOVExtra;

    SPCUTBasePacket spBasePacketVec [nMaxPackets];
    BasePacket*  pBasePacketVec  [nMaxPackets + 1]; // For the trailing null

    for (int i = 0; i < nMaxPackets; ++i)
    {
        m_pContFact->CreateBasePacket (spBasePacketVec[i], 8, 'a');
        pBasePacketVec[i] = spBasePacketVec[i].Ptr();
        pBasePacketVec[i]->m_uSequenceNumber = i;
    }
    pBasePacketVec[nMaxPackets] = 0;

    m_spTransport->CallParentVectorPacket (false);

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITEV);
    m_spSocket->setExpectedArg(nMaxPackets - nIOVExtra);

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->sendPackets(&pBasePacketVec[0]) ) );
}

void
CUTRDTUDPTransportTestDriver::testSendPacketsOnePacket ()
{
    SPCUTBasePacket spPacket;
    m_pContFact->CreateBasePacket (spPacket, 8, 'v');

    m_spTransport->CallParentVectorPacket (false);

    BasePacket* pPacketVec[2];
    pPacketVec[0] = spPacket.Ptr();
    pPacketVec[1] = 0;

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->sendPackets(&pPacketVec[0]) ) );
    //CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTUDPTransportTestDriver::testSendPacketsEnoughPackets ()
{
    static const int nMaxPackets = HX_IOV_MAX / 2 - 1;

    SPCUTBasePacket spPacketVec [nMaxPackets];
    BasePacket*  pPacketVec  [nMaxPackets + 1]; // For the trailing null

    for (int i = 0; i < nMaxPackets; ++i)
    {
        m_pContFact->CreateBasePacket (spPacketVec[i], 8, 'i');
        pPacketVec[i] = spPacketVec[i].Ptr();
    }
    pPacketVec[nMaxPackets] = 0;

    m_spTransport->CallParentVectorPacket (false);

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->sendPackets(&pPacketVec[0]) ) );
    //CPPUNIT_ASSERT( m_spSocket->Validate() );
}

