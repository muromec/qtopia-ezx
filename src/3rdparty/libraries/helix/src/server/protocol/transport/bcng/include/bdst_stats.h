/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bdst_stats.h,v 1.3 2003/01/24 01:00:30 damonlan Exp $ 
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

#ifndef _BDST_STATS_H_
#define _BDST_STATS_H_

#include "bandcalc.h"

#define BASE_STATS_ENTRY "BroadcastDistribution.Statistics"
#define STATS_COUNT_ENTRY "BroadcastDistribution.Statistics.Count"
#define STATS_INDEX_ENTRY "BroadcastDistribution.Statistics.Index"

class SenderStats : public IHXCallback
{
  
 public:
    ULONG32		    m_lRefCount;
    SenderStats(char* pURL, IUnknown* pContext);
    ~SenderStats();
    
    void Stop();
    void SetTransmissionBegun(BOOL bBegun) {m_bTransmissionBegun = bBegun;}
    void SetAvgBandwidthCalc(AvgBandwidthCalc* pAvgBandwidthCalc) 
	{m_pAvgBandwidthCalc = pAvgBandwidthCalc;}

    /*  stats accumulators              */
    UINT32		    m_ulPackets;
    UINT32		    m_ulBCCount;
    UINT32		    m_ulPushCount;
    UINT32		    m_ulPullCount;
    UINT32		    m_ulRelayCount;
    UINT32		    m_ulTCPCount;
    UINT32		    m_ulUDPCount;
    UINT32		    m_ulMCCount;
    UINT32		    m_ulPacketsLostBeforeTransport;
    UINT32		    m_ulFECPacketsSent;
    UINT32		    m_ulResendsRequested;
    UINT32		    m_ulResendsHonored;
    UINT32                  m_ulBufferOverruns;
    UINT32                  m_ulTimeStampDeliveredPackets;
    UINT32                  m_ulTimeStampDeliveredPacketsOnTime;
    /*   handles to stats in the registry */
    UINT32		    m_ulPackets_id;
    UINT32		    m_ulBCCount_id;
    UINT32		    m_ulPushCount_id;
    UINT32		    m_ulPullCount_id;
    UINT32		    m_ulRelayCount_id;
    UINT32		    m_ulTCPCount_id;
    UINT32		    m_ulUDPCount_id;
    UINT32		    m_ulMCCount_id;
    UINT32		    m_ulPacketsLostBeforeTransport_id;
    UINT32		    m_ulFECPacketsSent_id;
    UINT32		    m_ulResendsRequested_id;
    UINT32		    m_ulResendsHonored_id;
    UINT32                  m_ulBufferOverruns_id;
    UINT32                  m_ulTimeStampDeliveredPackets_id;
    UINT32                  m_ulTimeStampDeliveredPacketsOnTime_id;
    
    UINT32		    m_ulEntry_id;
    UINT32		    m_ulCBHandle;

    IHXScheduler*	    m_pScheduler;
    IHXRegistry*	    m_pRegistry;
    IHXCommonClassFactory* m_pCommonClassFactory;
    char*                   m_pURL;
    BOOL                    m_bTransmissionBegun;
    AvgBandwidthCalc*       m_pAvgBandwidthCalc;
    

    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    STDMETHOD(Func)		(THIS);
};
#endif /*_BDST_STATS_H_ */
