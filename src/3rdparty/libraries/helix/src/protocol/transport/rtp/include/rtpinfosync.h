/* ***** BEGIN LICENSE BLOCK *****  
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
#ifndef _RTP_INFO_SYNC_H_
#define _RTP_INFO_SYNC_H_

#include "tconverter.h"
#include "ntptime.h"

/* Forward Declarations */
class RTCPPacketBase;

#define HX_INVALID_STREAM 0xFFFF

class IRTPSyncResponse
{
public:
    STDMETHOD_(UINT32, AddRef)  (THIS) PURE;
    STDMETHOD_(UINT32, Release) (THIS) PURE;

    STDMETHOD(SyncDone)         (THIS) PURE;
};

class RTPInfoSynchData
{
 public:
    RTPInfoSynchData();
    ~RTPInfoSynchData();
    
    void Reset();

    HXBOOL  m_bHasSR;
    HXBOOL  m_bHasPacket;
    NTPTime m_ntpTimeFromSR;
    UINT32  m_ulRTPTimeFromSR;
    UINT32  m_ulRTPPacketTime;
    UINT32  m_ulRTPStartTime;   // units: rtp time
    UINT32  m_ulRTPFrequency;   // RTP timestamp frequency (per second)
};

class RTPInfoSynch
{
 public:
    RTPInfoSynch();
    ~RTPInfoSynch();

    ULONG32 AddRef();
    ULONG32 Release();

    HX_RESULT InitSynch(UINT16 unStreamCount, 
                        IRTPSyncResponse* pResponse = NULL,
                        UINT16 unMasterStream = HX_INVALID_STREAM);
    HX_RESULT SetTSFrequency(UINT32 ulRTPFrequency, UINT16 unStream);
    HX_RESULT Done();
    
    HX_RESULT   OnRTPPacket   (IHXRTPPacket* pPacket, UINT16 unStream);
    HX_RESULT   OnRTCPPacket  (RTCPPacketBase* pPacket, UINT16 unStream);
    HX_RESULT   GetRTPStartTime(UINT16 unStream, REF(UINT32) ulStartTime);

    inline HXBOOL IsSynced() { return m_bRTPTimesGenerated; }

 private:
    INT32	            m_lRefCount;
    RTPInfoSynchData*       m_pSynchData;
    UINT16                  m_unStreamCount;
    UINT16                  m_unSRCount;
    UINT16                  m_unSynchStream;
    HXBOOL                  m_bNeedSyncStream;
    HXBOOL                  m_bHaveAllSRs;
    HXBOOL                  m_bHaveAllPackets;
    UINT16                  m_unSyncPacketsReceived;
    HXBOOL                  m_bRTPTimesGenerated;
    IRTPSyncResponse*     m_pResponse;

    void CalculateSyncTimes();
};

#endif /*_RTP_INFO_SYNC_H_ */
