/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsrcbufstats.h,v 1.6 2007/01/11 19:41:27 milko Exp $ 
 *   
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef HXSRCBUFSTATS_H
#define HXSRCBUFSTATS_H

#include "hxcom.h"
#include "hxmap.h"
#include "hxcore.h"
#include "ihxtranstime.h"
#include "ihx3gpp.h"

class HXStreamPktBufInfo;

class HXSourceBufferStats : public IHXSourceBufferingStats3,
                            public IHXTransportTimeSink,
                            public IHX3gppNADU
{
public:
    HXSourceBufferStats();
    
    HX_RESULT Init(IUnknown* pContext);
    virtual void Close();

    HX_RESULT InitStream(UINT16  uStreamNumber, HXBOOL bIsLive);

    
    void OnPacket(IHXPacket* pPacket);

    void OnPacket(IHXPacket* pPacket,
                  UINT16 uSeqNum);

    void Reset();

    HX_RESULT LowestSeqNumber(UINT16  uStreamNumber, 
                              REF(UINT32) nLowestSeq);

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXSourceBufferingStats3 methods
     */
    STDMETHOD(GetCurrentBuffering)  (THIS_ UINT16 uStreamNumber,
				     REF(UINT32) ulLowestTimestamp, 
				     REF(UINT32) ulHighestTimestamp,
				     REF(UINT32) ulNumBytes,
				     REF(HXBOOL) bDone);

    STDMETHOD(GetTotalBuffering)  (THIS_ UINT16 uStreamNumber,
				   REF(UINT32) ulLowestTimestamp, 
				   REF(UINT32) ulHighestTimestamp,
				   REF(UINT32) ulNumBytes,
				   REF(HXBOOL) bDone);

    /*
     * IHXTransportTimeSink methods
     */
    STDMETHOD(OnTransportTime) (THIS_ UINT32 ulCurrentTime);

    /*
     * IHX3gppNADU Methods
     */
    STDMETHOD(GetNADUInfo) (THIS_ UINT16 uStreamNumber,
                            REF(UINT16) uPlayoutDelay,
                            REF(UINT16) uNextSeqNumber,
                            REF(UINT16) uNextUnitNumber,
                            REF(UINT16) uFreeBufferSpace);

    STDMETHOD(SetNADUParameters) (THIS_ UINT16 uStreamNumber,
                                  UINT32 uFrequency,
                                  UINT32 uBufferSize);

    STDMETHOD(GetNADUFrequency) (THIS_ UINT16 uStreamNumber,
                                 REF(UINT32) uFrequency);

    STDMETHOD(GetNADUBufferSize) (THIS_ UINT16 uStreamNumber,
                                  REF(UINT32) uBufferSize);

protected:
    ~HXSourceBufferStats();

    // Used by Release()
    ULONG32 DecRefCount();

    HXBOOL Initialized() const {return (m_pTimeMan) ? TRUE: FALSE;}
    void DoClose();

    HXStreamPktBufInfo* GetPktBufInfo(UINT16  uStreamNumber);

private:
    ULONG32 m_lRefCount;
    IHXTransportTimeManager* m_pTimeMan;

    CHXMapLongToObj m_streamPktInfo;
    HXBOOL m_bIsLive;
    HXBOOL m_bFirstPacket;
    UINT32 m_uFirstLivePacketTS;
};

#endif /* HXSRCBUFSTATS_H */
