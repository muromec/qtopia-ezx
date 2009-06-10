/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rtspmcasttran.cpp,v 1.4 2006/12/21 05:12:04 tknox Exp $
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

#include "hxcom.h"
#include "hxresult.h"
#include "hxslist.h"
#include "hxccf.h"
#include "hxmarsh.h"
#include "chxmaplongtoobj.h"
#include "chxmapstringtoob.h"
#include "hxnet.h"
#include "basepkt.h"
#include "rtspif.h"
#include "rtsptran.h"
#include "transport.h"
#include "servlist.h"

#include "rtspmcast.h"

#include "ut_globals.h"
#include "ut_scheduler.h"
#include "ut_rtsptranresp.h"
#include "ut_contfact.h"
#include "ut_rtspmcasttran.h"
#include "ut_bufsock.h"
#include "ut_sockaddr.h"
#include "ut_transport.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( CUTRTSPServerMulticastTransportTestDriver );

CUTRTSPServerMulticastTransportTestDriver::CUTRTSPServerMulticastTransportTestDriver ()
    : m_spTransport ()
    , m_pTransports (NULL)
    , m_spTransportResponse ()
    , m_spContext ()
    , m_spCCF ()
    , m_spSocket ()
    , m_pContFact (NULL)
    , m_spPacket ()
{
    m_spContext = new CUTContext ();

    m_spCCF.Query (m_spContext.Ptr ());
    m_pContFact = new CUTContainerFactory (m_spCCF.Ptr ());
}

CUTRTSPServerMulticastTransportTestDriver::~CUTRTSPServerMulticastTransportTestDriver ()
{
    delete m_pContFact;
    m_spContext->Close();  // Break the circular dependency
}

void
CUTRTSPServerMulticastTransportTestDriver::setUp ()
{
    const int nSize = 100;
    m_pContFact->CreateBasePacket (m_spPacket, nSize, 'T');

    m_pTransports          = new CHXMapStringToOb;
    m_spTransport          = new RTSPServerMulticastTransport(m_spContext.Ptr(), m_pTransports);
}

void
CUTRTSPServerMulticastTransportTestDriver::tearDown ()
{
    m_spTransport.Clear();
    delete m_pTransport;
}

RTSPMcastInfo*
CUTRTSPServerMulticastTransportTestDriver::CreateRTSPMcastInfo (Transport* pTran, const char* pKey)
{
    RTSPMcastInfo *pInfo = new RTSPMcastInfo;
    pInfo->m_pTransport = pTran;
    pTran->AddRef(); // Keep it around after the RTSPMcastInfo goes away
    pInfo->m_ulPlayerCount = 0;
    pInfo->m_ulAddress = 0;
    pInfo->m_bAddrAllocated = TRUE;
    memset (pInfo->m_bRuleOn, TRUE, sizeof (pInfo->m_bRuleOn));
    m_pTransports->SetAt (pKey, pInfo);

    return pInfo;
}

void
CUTRTSPServerMulticastTransportTestDriver::testSendPacketNULLPacket ()
{
    SPCUTMockTransport spBaseTransport = new CUTMockTransport ();
    spBaseTransport->setExpectedMethod (CUTMockTransport::FTC_NO_CALL);

    m_spTransport->sendPacket(static_cast<BasePacket*>(0));

    CPPUNIT_ASSERT( spBaseTransport->Validate() );
}

void
CUTRTSPServerMulticastTransportTestDriver::testSendPacketNoTransport ()
{
    SPCUTMockTransport spBaseTransport = new CUTMockTransport ();
    spBaseTransport->setExpectedMethod (CUTMockTransport::FTC_NO_CALL);

    m_spTransport->sendPacket(m_spPacket.Ptr());

    CPPUNIT_ASSERT( spBaseTransport->Validate() );
}

void
CUTRTSPServerMulticastTransportTestDriver::testSendPacketBadTransport ()
{
    SPCUTMockTransport spBaseTransport = new CUTMockTransport ();
    spBaseTransport->setExpectedMethod (CUTMockTransport::FTC_NO_CALL);

    RTSPMcastInfo *pInfoNoAddr = CreateRTSPMcastInfo (spBaseTransport.Ptr (), "NoAddr");
    pInfoNoAddr->m_bAddrAllocated = FALSE;

    RTSPMcastInfo *pInfoNoRule = CreateRTSPMcastInfo (spBaseTransport.Ptr (), "NoRule");
    memset (pInfoNoRule->m_bRuleOn, FALSE, sizeof (pInfoNoRule->m_bRuleOn));

    m_spTransport->sendPacket(m_spPacket.Ptr());

    CPPUNIT_ASSERT( spBaseTransport->Validate() );
}

void
CUTRTSPServerMulticastTransportTestDriver::testSendPacketOneTransport ()
{
    SPCUTMockTransport spBaseTransport = new CUTMockTransport ();

    RTSPMcastInfo *pInfoOneTransport = CreateRTSPMcastInfo (spBaseTransport.Ptr (), "One");

    spBaseTransport->setExpectedMethod (CUTMockTransport::FTC_SEND_PACKET);
    spBaseTransport->setExpectedArg (m_spPacket.Ptr());

    m_spTransport->sendPacket(m_spPacket.Ptr());

    CPPUNIT_ASSERT( spBaseTransport->Validate() );
}

void
CUTRTSPServerMulticastTransportTestDriver::testSendPacketNTransports ()
{
    SPCUTMockTransport spBaseTransport1 = new CUTMockTransport ();
    spBaseTransport1->setExpectedMethod (CUTMockTransport::FTC_SEND_PACKET);
    spBaseTransport1->setExpectedArg (m_spPacket.Ptr());

    SPCUTMockTransport spBaseTransport2 = new CUTMockTransport ();
    spBaseTransport2->setExpectedMethod (CUTMockTransport::FTC_SEND_PACKET);
    spBaseTransport2->setExpectedArg (m_spPacket.Ptr());

    RTSPMcastInfo *pInfoTransport1 = CreateRTSPMcastInfo (spBaseTransport1.Ptr (), "One");
    RTSPMcastInfo *pInfoTransport2 = CreateRTSPMcastInfo (spBaseTransport2.Ptr (), "Two");

    m_spTransport->sendPacket(m_spPacket.Ptr());

    CPPUNIT_ASSERT( spBaseTransport1->Validate() );
    CPPUNIT_ASSERT( spBaseTransport2->Validate() );
}

