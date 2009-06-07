/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pushpcktflow.h,v 1.7 2004/02/10 23:01:30 jmevissen Exp $
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
#ifndef _PUSHPCKTFLOW_H_
#define _PUSHPCKTFLOW_H_

#include "basicpcktflow.h"

class PushPacketFlow : public BasicPacketFlow
{
public:
    PushPacketFlow(Process* p,
                   IHXSessionStats* pSessionStats,
                   UINT16 unStreamCount,
                   PacketFlowManager* pFlowMgr,
                   IHXPSourceLivePackets* pSourceLivePackets,
                   BOOL bIsMulticast);

    ~PushPacketFlow();

protected:
    virtual void HandleResume();

    virtual void InitialStartup();
    virtual void Pause(BOOL bWouldBlock, UINT32 ulPausePoint = 0);

    void Done();

    void TransmitPacket(ServerPacket* pPacket);
    void MeterPacket(ServerPacket* pPacket);

    STDMETHOD(StartSeek)(UINT32 ulTime);

    STDMETHOD(SeekDone)();

    STDMETHOD(RegisterStream)(Transport* pTransport,
                              UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader);

    HX_RESULT SessionPacketReady(HX_RESULT ulStatus, IHXPacket* pPacket);

    STDMETHOD(Activate)();

    STDMETHOD(HandleSubscribe)(INT32 lRuleNumber, UINT16 unStreamNumber);

    STDMETHOD(HandleUnSubscribe)(INT32 lRuleNumber, UINT16 unStreamNumber);

    virtual void RescheduleTSD(UINT16 unStream, float fOldRatio) {}

private:
    IHXPSourceLivePackets*  m_pSourceLivePackets;
    IHXASMSource*           m_pASMSource;
};

#endif // _PUSHPCKTFLOW_H_
