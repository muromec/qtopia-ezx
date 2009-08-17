/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bwcalc.h,v 1.3 2006/07/14 20:46:22 ghori Exp $ 
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

#include "base_callback.h"
#include "servpckts.h"
#include "rtspif.h"
#include "proc.h"

// Seconds between recalculation of average bandwidth for 
// timestamp-delivered streams
const UINT32 BANDWIDTH_RECALC_RATE = 2;

class BWCalculator
{
public:
    BWCalculator(Process* pProc, BOOL bNullSetup);

    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    void CommitPendingBandwidth	();
    void PacketSent		(ServerPacket* pPacket);
    void BytesSent		(UINT32 ulBytes);
    void RecalcAvgBandwidth	();

    class RecalcCallback: public BaseCallback
    {
    public:
	RecalcCallback(BWCalculator* pBWCalculator);
	~RecalcCallback();

	STDMETHOD(Func)	(THIS);
	BWCalculator*	m_pBWCalculator;
    };
    friend class RecalcCallback;

private:
    ~BWCalculator();

    LONG32			    m_lRefCount;
    BOOL			    m_bNullSetup;
    BOOL			    m_bBandwidthPending;
    UINT32			    m_ulTotalBytesSent;
    UINT32			    m_ulElapsedTime;
    UINT32			    m_ulAvgBandwidth;
    UINT32			    m_ulScheduledSendID;
    RecalcCallback*		    m_pRecalcCallback;
    Process*			    m_pProc;
};
