/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_test_engine.h,v 1.5 2003/04/08 21:42:08 damonlan Exp $ 
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

#define TEST_NUM_SESSIONS 1024
#define TEST_NUM_SINKS    1024

class TestSinkListElem : public HXListElem
{
 public:
    TestSinkListElem(IHXBuffer* pId, QoSTestSink* pSink);
    ~TestSinkListElem();

    IHXBuffer*        m_pId;
    QoSTestSink*      m_pSink;
};

class TestSessionListElem : public HXListElem
{
 public:
    TestSessionListElem(IHXBuffer* pId);
    ~TestSessionListElem();

    IHXBuffer*        m_pId;
};

class QoSTestEngine
{
 public:
    QoSTestEngine();
    ~QoSTestEngine();

    HX_RESULT TestController();
    HX_RESULT TestSignalBus();
    HX_RESULT TestProfileManager();

 private:
    IHXBuffer* MakeSessionId();

    IHXQoSSignalBusController*  m_pController;
    HXList                      m_TestSinks;
    HXList                      m_TestSessions;
    UINT32                      m_ulNextSessionId;
};
