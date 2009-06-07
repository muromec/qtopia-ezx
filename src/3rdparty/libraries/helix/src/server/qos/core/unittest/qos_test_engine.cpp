/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_test_engine.cpp,v 1.8 2003/04/10 22:46:03 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "hxccf.h"

#include "mutex.h"
#include "chxmapstringtoob.h"
#include "hxbuffer.h"
#include "servlist.h"

#include "qos_sig_bus_ctl.h"
#include "qos_sig_bus.h"
#include "qos_test_sink.h"
#include "qos_prof_mgr.h"
#include "qos_test_engine.h"

/* Node in the test sink list */
TestSinkListElem::TestSinkListElem(IHXBuffer* pId, QoSTestSink* pSink) :
    m_pId (NULL),
    m_pSink (NULL)
{
    if (pId && pSink)
    {
	m_pId = pId;
	m_pId->AddRef();

	m_pSink = pSink;
	m_pSink->AddRef();
    }
}
    
TestSinkListElem::~TestSinkListElem()
{
    HX_RELEASE(m_pSink);
    HX_RELEASE(m_pId);
}

/* Node in the test session list */
TestSessionListElem::TestSessionListElem(IHXBuffer* pId) :
    m_pId (NULL)
{
    if (pId)
    {
	m_pId = pId;
	m_pId->AddRef();
    }
}
    
TestSessionListElem::~TestSessionListElem()
{
    HX_RELEASE(m_pId);
}

/* Driver for the QoS Signal Bus Tests */
QoSTestEngine::QoSTestEngine() :
    m_pController (NULL),
    m_ulNextSessionId (0)
{
    QoSSignalBusController* pCtl = new QoSSignalBusController();
    pCtl->QueryInterface(IID_IHXQoSSignalBusController, (void**)& m_pController);

    HX_ASSERT(m_pController);
    
    srand((unsigned)time(NULL));
    m_ulNextSessionId = rand();
}

QoSTestEngine::~QoSTestEngine()
{
    HX_RELEASE(m_pController);
}

HX_RESULT
QoSTestEngine::TestController()
{
    IHXBuffer* pTmpId          = NULL;
    IHXQoSSignalBus* pTmpBus   = NULL;
    TestSessionListElem* pElem = NULL;
    HX_RESULT         hRes     = HXR_OK;

    /* Generate a bunch of signal buses */
    printf("QoSSignalBusController Test: Creating Buses... ");

    for (UINT16 i = 0; i < TEST_NUM_SESSIONS; i++)
    {
	pTmpId = MakeSessionId();
	m_TestSessions.insert(new TestSessionListElem(pTmpId));
	
	if (FAILED(m_pController->CreateSignalBus(pTmpId, HX_QOS_PROFILE_DEFAULT)))
	{
	    printf("Failed!\n");
	    hRes = HXR_FAIL;
	    break;
	}

	HX_RELEASE(pTmpId);
    }

    if (hRes == HXR_OK)
    {
	printf("OK\n");
	
	/* Check to see that they are all there */
	printf("QoSSignalBusController Test: Validating Bus Creation... ");
	HXList_iterator j(&m_TestSessions);
	
	for(; *j != 0; ++j)
	{
	    pElem = (TestSessionListElem*)(*j);
	    HX_RELEASE(pTmpBus);
	    
	    if (FAILED(m_pController->GetSignalBus(pElem->m_pId, pTmpBus)) || !pTmpBus)
	    {
		printf("Failed !\n");
		hRes = HXR_FAIL;
		break;
	    }
	}
    }

    if (hRes == HXR_OK)
    {
	printf("OK\n");
	
	/* Destroy them */
	printf("QoSSignalBusController Test: Destroying Buses... ");
	HXList_iterator k(&m_TestSessions);
	
	for(; *k != 0; ++k)
	{
	    pElem = (TestSessionListElem*)(*k);
	    HX_RELEASE(pTmpBus);
	    
	    if (FAILED(m_pController->DestroySignalBus(pElem->m_pId)))
	    {
		printf("Failed!\n");
		hRes = HXR_FAIL;
		break;
	    }
	}
    }

    if (hRes == HXR_OK)
    {
	printf("OK\n");
    }

    pElem = NULL;
    while(pElem = (TestSessionListElem*)(m_TestSessions.remove_head()))
    {
	HX_DELETE(pElem);
    }

    return hRes;
}

HX_RESULT
QoSTestEngine::TestSignalBus()
{
    HX_ASSERT(m_pController);

    IHXQoSSignalSource* pSrc = (IHXQoSSignalSource*)(new QoSSignalSource(m_pController));
    pSrc->AddRef();
    
    IHXBuffer* pSessionId = MakeSessionId();
    IHXQoSSignalBus* pTmpBus = NULL;
    TestSinkListElem* pElem = NULL;

    HX_ASSERT(SUCCEEDED(m_pController->CreateSignalBus(pSessionId, HX_QOS_PROFILE_DEFAULT)));
    HX_ASSERT(SUCCEEDED(m_pController->GetSignalBus(pSessionId, pTmpBus)) || !pTmpBus);

    for (UINT16 i = 0; i < TEST_NUM_SINKS; i++)
    {
	m_TestSinks.insert(new TestSinkListElem
			   (pSessionId, (new QoSTestSink(pSrc, pSessionId))));
								    
    }

    /* Check to see that they are all there */
    printf("QoSSignalBusController Test: Validating Signal Transmission... ");
    HXList_iterator j(&m_TestSinks);
    
    for(; *j != 0; ++j)
    {
	pElem = (TestSinkListElem*)(*j);
	pElem->m_pSink->TestSignal(pTmpBus);
    }

    printf("OK\n");

    printf("QoSSignalBusController Test: Dettaching Signal Sinks... ");
    HXList_iterator k(&m_TestSinks);
    
    for(; *k != 0; ++k)
    {
	pElem = (TestSinkListElem*)(*k);
	pElem->m_pSink->Close();
    }

    printf("OK\n");

    pElem = NULL;
    while(pElem = (TestSinkListElem*)(m_TestSinks.remove_head()))
    {
	HX_DELETE(pElem);
    }
    
    HX_RELEASE(pSessionId);
    HX_RELEASE(pSrc);
    HX_RELEASE(pTmpBus);

    return HXR_OK;
}

HX_RESULT
QoSTestEngine::TestProfileManager()
{
    printf("QoSSignal Test: Testing Profile Manager...");
    HX_QOS_PROFILE_TYPE cType;

    char* pTmp1 = new_string("example-rma-useragent;example-rma-datatype;example-rma-xport");
    char* pTmp2 = new_string("example-gen-useragent;example-gen-datatype");					   
    char* pTmp3 = new_string("doesnotexist");

    IHXBuffer* pBuf1 = (IHXBuffer*)(new CHXBuffer());
    IHXBuffer* pBuf2 = (IHXBuffer*)(new CHXBuffer());
    IHXBuffer* pBuf3 = (IHXBuffer*)(new CHXBuffer());

    pBuf1->AddRef();
    pBuf2->AddRef();
    pBuf3->AddRef();

    pBuf1->Set((const UCHAR*)pTmp1, strlen(pTmp1)+1);
    pBuf2->Set((const UCHAR*)pTmp2, strlen(pTmp2)+1);
    pBuf3->Set((const UCHAR*)pTmp3, strlen(pTmp3)+1);

    IHXQoSProfileSelector* pSelector = (IHXQoSProfileSelector*)(new QoSProfileSelector());
    
//     HX_ASSERT(HXR_OK == pSelector->SelectProfile(pBuf1, cType));
//     HX_ASSERT(cType == HX_QOS_PROFILE_RMA);

//     HX_ASSERT(HXR_OK == pSelector->SelectProfile(pBuf2, cType));;
//     HX_ASSERT(cType == HX_QOS_PROFILE_RTP_GENERIC);

//     HX_ASSERT(HXR_OK != pSelector->SelectProfile(pBuf3, cType));
//     HX_ASSERT(!cType);

    HX_RELEASE(pBuf1);
    HX_RELEASE(pBuf2);
    HX_RELEASE(pBuf3);

    HX_DELETE(pTmp1);
    HX_DELETE(pTmp2);
    HX_DELETE(pTmp3);

    printf("NO-OP, build map before testing!\n");
    return HXR_OK;
}

IHXBuffer* 
QoSTestEngine::MakeSessionId()
{
    char tmp[64];
    IHXBuffer* pBuf = (IHXBuffer*)(new CHXBuffer());
    
    HX_ASSERT(pBuf);
    pBuf->AddRef();

    sprintf(tmp, "%ld-%ld", ++m_ulNextSessionId, 256);
    pBuf->Set((const UCHAR*)tmp, strlen(tmp)+1);

    return pBuf;
}

