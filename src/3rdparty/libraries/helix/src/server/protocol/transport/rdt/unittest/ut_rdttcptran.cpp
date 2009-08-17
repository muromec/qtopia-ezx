/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rdttcptran.cpp,v 1.6 2007/01/11 20:28:43 seansmith Exp $
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
#include "hxmarsh.h"
#include "chxmaplongtoobj.h"
#include "hxnet.h"
#include "basepkt.h"
#include "rtspif.h"
#include "rtsptran.h"

#include "ut_contfact.h"

#include "rdt_tcp.h"
#include "ut_rtsptranresp.h"
#include "ut_rdttcptran.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( CUTRDTTCPTransportTestDriver );

CUTRDTTCPTransportTestDriver::CUTRDTTCPTransportTestDriver ()
    : m_spTransport ()
    , m_spTransportResponse ()
    , m_spContext ()
    , m_spCCF ()
    , m_spSocket ()
    , m_spBuffer ()
{
    m_spContext = new CUTContext();
    m_spCCF.Query (m_spContext.Ptr ());
    
    m_pContFact = new CUTContainerFactory (m_spCCF.Ptr ());
}

CUTRDTTCPTransportTestDriver::~CUTRDTTCPTransportTestDriver ()
{
    delete m_pContFact;
	m_spContext->Close();
    m_spSocket.Clear();
}

void
CUTRDTTCPTransportTestDriver::setUp ()
{
    m_spTransport          = new RDTTCPTransport(TRUE);
    m_spTransportResponse  = new CUTRTSPTransportResponse();
    m_spSocket             = new CUTMockSocket();

    m_spTransport->init (m_spContext.Ptr(), m_spSocket.Ptr(),
                        '_', m_spTransportResponse.Ptr());
}

void
CUTRDTTCPTransportTestDriver::tearDown ()
{
    m_spTransport->Done();
    m_spTransportResponse.Clear();
}

void
CUTRDTTCPTransportTestDriver::testWritePacketNoData ()
{
    m_spSocket->Reset();
    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    m_pContFact->CreateBuffer (m_spBuffer, "", 0);
    m_spBuffer->AddRef();       // writePacket assumes it can release the buffer

    CPPUNIT_ASSERT( FAILED( m_spTransport->writePacket(m_spBuffer.Ptr()) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTTCPTransportTestDriver::testWritePacketTooMuchData ()
{
    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_NO_CALL);

    m_pContFact->CreateBuffer (m_spBuffer, 65536, 'd');
    m_spBuffer->AddRef();       // writePacket assumes it can release the buffer

    CPPUNIT_ASSERT( FAILED( m_spTransport->writePacket(m_spBuffer.Ptr()) ) );
    CPPUNIT_ASSERT( m_spSocket->Validate() );
}

void
CUTRDTTCPTransportTestDriver::testWritePacketEnoughData ()
{
    static BYTE buffer[] = "xxxxAbCdEfG";
    const int nBufSize = sizeof(buffer);
    const int nHdrSize = 4;
    const int nResSize = nBufSize - nHdrSize;
    BYTE *pResultBuffer = new BYTE[nResSize];

    buffer[0] = '$';
    buffer[1] = m_spTransport->m_tcpInterleave;
    putshort(&buffer[2], (UINT16)nResSize);
    memcpy (pResultBuffer, buffer + nHdrSize, nResSize);

    m_pContFact->CreateBuffer (m_spBuffer, (char*)pResultBuffer, nResSize);
    m_spBuffer->AddRef();       // writePacket assumes it can release the buffer

    m_spSocket->setExpectedMethod(CUTMockSocket::FTC_WRITE);
    m_spSocket->setExpectedArg(&buffer[0]);
    m_spSocket->setExpectedArg(nBufSize);

    CPPUNIT_ASSERT( SUCCEEDED( m_spTransport->writePacket(m_spBuffer.Ptr()) ) );

    CPPUNIT_ASSERT( m_spSocket->Validate() );

    delete[] pResultBuffer;
}

