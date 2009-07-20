/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rtcptcptran.cpp,v 1.4 2006/10/19 20:56:08 tknox Exp $
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
#include "sockio.h" // For HX_IOV_MAX
#include "basepkt.h"
#include "rtspif.h"
#include "rtsptran.h"

#include "sink.h"
#include "source.h"
#include "rtp_tcp.h"

#include "ut_contfact.h"
#include "ut_rtsptranresp.h"
#include "ut_rtcptcptran.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( CUTServerRTCPTCPTransportTestDriver );

CUTServerRTCPTCPTransportTestDriver::CUTServerRTCPTCPTransportTestDriver ()
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

    m_pContFact->CreateBuffer (m_spBuffer, 8, 't');
}

CUTServerRTCPTCPTransportTestDriver::~CUTServerRTCPTCPTransportTestDriver ()
{
    delete m_pContFact;
    m_spContext->Close();
}

void
CUTServerRTCPTCPTransportTestDriver::setUp ()
{
    static const UINT32 ulStreamNum = 0;

    m_spBaseTransport      = new ServerRTPTCPTransport(TRUE);
    m_spTransport          = new ServerRTCPTCPTransport(TRUE);
    m_spTransportResponse  = new CUTRTSPTransportResponse();
    m_spSocket             = new CUTMockSocket();

    m_spTransport->init(static_cast<IUnknown*>(m_spContext.Ptr()),
                        static_cast<IHXSocket*>(m_spSocket.Ptr()),
                        m_spBaseTransport.Ptr(),
                        m_spTransportResponse.Ptr(),
                        ulStreamNum);
}

void
CUTServerRTCPTCPTransportTestDriver::tearDown ()
{
    m_spTransport->Done();
    m_spBaseTransport->Done();
    m_spTransportResponse.Clear();
}

void
CUTServerRTCPTCPTransportTestDriver::testWritePacketNULLBuffer ()
{
    m_spSocket->Reset(); // During set up, methods get called on the socket
    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->writePacket(static_cast<IHXBuffer*>(0)) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTCPTCPTransportTestDriver::testWritePacketEmptyBuffer ()
{
    BYTE pHead [4] = {'$', 0xff, 0x0, 0x0};
    SPIHXBuffer spEmptyBuffer(m_spCCF.Ptr(), CLSID_IHXBuffer);

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITE_MULTIPLE);
    m_spSocket->setExpectedArg(sizeof(pHead));
    m_spSocket->setExpectedArg(&pHead[0]);

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->writePacket(spEmptyBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTCPTCPTransportTestDriver::testWritePacketTooFullBuffer ()
{
    const UINT32 ulSize = 0x10000; 

    SPIHXBuffer spBuffer;
    m_pContFact->CreateBuffer(spBuffer, ulSize, 'k');

    m_spSocket->Reset(); // During set up, methods get called on the socket
    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    CPPUNIT_ASSERT( FAILED( m_spTransport->writePacket(spBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTServerRTCPTCPTransportTestDriver::testWritePacketJustRightBuffer ()
{
    const UINT32 ulHeaderSize = 4;
    const UINT32 ulSize = 0x100;
    const UINT32 ulTotalSize = ulSize + ulHeaderSize;

    SPIHXBuffer spBuffer, spCompleteBuffer;
    m_pContFact->CreateBuffer(spBuffer, ulSize);

    // Make the header
    m_pContFact->CreateBuffer(spCompleteBuffer, ulTotalSize);
    BYTE* pHead = spCompleteBuffer->GetBuffer();
    *pHead++ = '$';
    *pHead++ = 0xff;
    *pHead++ = 0x01;
    *pHead++ = 0x00;

    m_spSocket->setExpectedMethod (CUTMockSocket::FTC_WRITE_MULTIPLE);
    m_spSocket->setExpectedArg(spCompleteBuffer->GetSize());
    m_spSocket->setExpectedArg(const_cast<const BYTE*>(static_cast<BYTE*>(spCompleteBuffer->GetBuffer())));
    m_spSocket->setExpectedArg(static_cast<IHXSockAddr*>(0));

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->writePacket(spBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

