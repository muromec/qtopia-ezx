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

typedef _INTERFACE  IHXRTPInfoSynch	    IHXRTPInfoSynch;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRTPInfoSynch
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXRTPInfoSynch:
 * 
 *	{1D9DF3AD-7429-4efc-B54D-D67E256304C4}
 * 
 */

DEFINE_GUID(IID_IHXRTPInfoSynch, 
	    0x1d9df3ad, 0x7429, 0x4efc, 0xb5, 0x4d, 0xd6, 0x7e, 0x25, 0x63, 0x4, 0xc4);

#undef  INTERFACE
#define INTERFACE   IHXRTPInfoSynch

DECLARE_INTERFACE_(IHXRTPInfoSynch, IUnknown)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    STDMETHOD(InitSynch)(THIS_ 
			 UINT16 unStreamCount) PURE;
    STDMETHOD(RTPSynch) (THIS_ UINT16 unMaster) PURE;
    STDMETHOD(OnRTPPacket) (THIS_ IHXBuffer* pRTPPacket, 
			    UINT16 unStream,
			    REF(HXBOOL) bSynched,
			    REF(UINT32) ulSequenceNumber,
			    REF(UINT32) ulTimestamp) PURE;
    STDMETHOD(OnRTCPPacket)(THIS_ IHXBuffer* pRTCPPacket, 
			    UINT16 unStream) PURE;
    STDMETHOD(SetTSConverter)(THIS_ 
			      CHXTimestampConverter::ConversionFactors 
			      conversionFactors,
			      UINT16 unStream) PURE;
    STDMETHOD(IsStreamSynched)(THIS_ UINT16 unStream, REF(HXBOOL) bIsSynched) PURE;
    STDMETHOD(Done)     (THIS) PURE;
};

class RTPInfoSynchData
{
 public:
    RTPInfoSynchData();
    ~RTPInfoSynchData();
    
    void Reset();

    HXBOOL    m_bHasSR;
    INT32   m_lRTPtoNTPOffset; //units: msec ; additive conversion from RTP to NTP for this stream
    UINT32  m_ulRTPInfoTime;   //units: rtp time ; time to be placed in rtp info field
    HXBOOL    m_bSynched; 

    CHXTimestampConverter* m_pTSConverter;
};


class RTPInfoSynch: public IHXRTPInfoSynch
{
 public:
    RTPInfoSynch			();
    ~RTPInfoSynch			();

    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      	(THIS);
    STDMETHOD_(ULONG32,Release)     	(THIS);

    
    STDMETHOD(InitSynch)(THIS_ 
			 UINT16 unStreamCount);
    STDMETHOD(RTPSynch) (THIS_ UINT16 unMaster);
    STDMETHOD(OnRTPPacket) (THIS_ IHXBuffer* pRTPPacket, 
			    UINT16 unStream,
			    REF(HXBOOL) bSynched,
			    REF(UINT32) ulSequenceNumber,
			    REF(UINT32) ulTimestamp);
    STDMETHOD(OnRTCPPacket)(THIS_ IHXBuffer* pRTCPPacket, UINT16 unStream);
    STDMETHOD(SetTSConverter)(THIS_ 
			      CHXTimestampConverter::ConversionFactors 
			      conversionFactors,
			      UINT16 unStream);
    STDMETHOD(IsStreamSynched)(THIS_ UINT16 unStream, REF(HXBOOL) bIsSynched);
    STDMETHOD(Done)     (THIS);

 private:
    INT32	              m_lRefCount;
    RTPInfoSynchData*         m_pSynchData;
    UINT16                    m_unStreamCount;
    UINT16                    m_unSRCount;
    UINT16                    m_unSynchStream;
    HXBOOL                      m_bHaveAllSRs;
    HXBOOL                      m_bRTPTimesGenerated;
};

#endif /*_RTP_INFO_SYNC_H_ */
