/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: pcktflowmgr.h,v 1.16 2008/03/24 06:58:39 manvendras Exp $
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

#ifndef _PCKTFLWMGR_H_
#define _PCKTFLWMGR_H_

#include "hxdtcvt.h"

#include "hxformt.h"
#include "hxengin.h"
#include "hxasm.h"
#include "source.h"
#include "timeval.h"
#include "base_callback.h"
#include "servpckts.h"
#include "hxmap.h"
#include "servlist.h"
#include "loadinfo.h"
#include "rtspif.h"
#include "hxpcktflwctrl.h"
#include "pcktflowwrap.h"

class Process;
class Transport;
class ASMRuleBook;
class BWCalculator;
class PacketStreamDoneCallback;
class PacketStream;
class DataConvertShim;
class PacketFlowTimeCallback;
class BasicPacketFlow;

_INTERFACE IHXPacketFlowControl;

#define MAX_RESENDS_PER_SECOND 256

class PacketFlowManager
{
public:
    PacketFlowManager(Process* pProc, BOOL bIsRTP);
    ~PacketFlowManager();

    void RegisterSource(IUnknown* pSourceCtrl,
                        IHXPacketFlowControl** ppPacketFlowControl,
                        IHXSessionStats* pSessionStats,
                        UINT16 unStreamCount,
                        BOOL bIsLive,
                        BOOL bIsMulticast,
                        DataConvertShim* pDataConv,
                        BOOL bIsFCS = FALSE);
    
    void RegisterSource(IUnknown* pSourceCtrl,
                        IHXPacketFlowControl** ppPacketFlowControl,
                        IHXSessionStats* pSessionStats,
                        UINT16 unStreamCount,
                        BOOL bIsLive,
                        BOOL bIsMulticast,
                        DataConvertShim* pDataConv,
                        Client* pClient,
                        const char* szPlayerSessionId,
                        BOOL bIsFCS = FALSE);

    void SessionDone(BasicPacketFlow* pFlow);

    void ChangeDeliveryBandwidth(INT32 lChange, BOOL bReportChange);
    void RecalcConstantBitRate();
private:
    void CommitPendingBandwidth();
//    void RecalcConstantBitRate();

    Process*	 m_pProc;
    BOOL	 m_bIsRTP;
    BOOL	 m_bInitialPlayReceived;
    HXList	 m_PacketFlows;
    UINT32	 m_ulPendingBitRate;
    UINT32	 m_ulDeliveryBitRate;
    UINT32	 m_ulActualDeliveryRate;
    Timeval	 m_tNextSendTime;
    Timeval	 m_tLastSendTime;
    UINT32	 m_ulBackToBackCounter;
    UINT32	 m_ulBackToBackFreq;
    BOOL	 m_bInformationalAggregatable;
    BOOL	 m_bDidLock;
    INT32        m_ulConfigId;

    friend class BasicPacketFlow;
    friend class PushPacketFlow;
    friend class PullPacketFlow;
    friend class PacketStreamDoneCallback;
    friend class PacketStream;
};

class PacketStreamDoneCallback: public BaseCallback
{
public:
    MEM_CACHE_MEM

    STDMETHOD(Func) (THIS);

    BasicPacketFlow*    m_pFlow;
    Transport*        m_pTransport;
    PacketStream*	m_pStream;
    UINT16		m_unStreamNumber;
};

#endif // _PCKTFLWMGR_H_
