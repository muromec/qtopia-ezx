/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 
#ifndef _PACKET_REORDER_SHIM_H_
#define _PACKET_REORDER_SHIM_H_

#include "hxcom.h"
#include "unkimp.h"

_INTERFACE IHXScheduler;
_INTERFACE IHXCommonClassFactory;
_INTERFACE IHXRawSinkObject;

class CPacketOrderShim
    : public CUnknownIMP
    , public IHXPacketOrderer
    , public IHXPSinkPackets
{
public:
    CPacketOrderShim();

    //	*** IUnknown ***
    DECLARE_UNKNOWN(CPacketOrderShim)

    //	*** IHXPacketOrderer ***
    STDMETHOD(Initialize)(THIS_ IUnknown* pContext, IHXRawSinkObject* pSinkObject, UINT32 ulStreamCount, UINT32 nAgingDuration, UINT32 nPacketSendInterval);
    STDMETHOD(Terminate)(THIS_ INT32 nStreamNumber);
    STDMETHOD(GetContext)(THIS_ IUnknown** ppContext);
    STDMETHOD(GetOrderingDuration)(THIS_ UINT32* pnAgingDuration);
    STDMETHOD(GetPacketSendInterval)(THIS_ UINT32* pnPacketSendInterval);
    STDMETHOD(GetStreamCount)(THIS_ UINT32* pulStreamCount);
    STDMETHOD(GetPacketSink)(THIS_ IHXRawSinkObject** ppSinkObject);

    //	*** IHXPSinkPackets ***
    STDMETHOD(PacketReady)(THIS_ HX_RESULT status, IHXPacket* pPacket);

protected:
    
    //	Only need one instance for the head of the queue
    class CPacketAgedCallback
	: public CUnknownIMP
	, public IHXCallback
    {
    public:
	CPacketAgedCallback(CPacketOrderShim* pPacketOrderShim = NULL);

	//	*** IUnknown ***
	DECLARE_UNKNOWN(CPacketAgedCallback)

	//	*** IHXCallback ***
	STDMETHOD(Func)(THIS);

    protected:
	CallbackHandle m_hCB;

	//  !!! Don't addref so no circular reference!!!
	CPacketOrderShim* m_pPacketOrderShim;
	//	Grant friendship so can access protected members of CPacketOrderShim
	friend class CPacketOrderShim;
    };

    //	Grant friendship so can access protected members of CPacketOrderShim
    friend class CPacketAgedCallback;

protected:
    virtual ~CPacketOrderShim();

    HX_RESULT DelayPacketForOrdering(IHXPacket* pPacket);

    //	Sends packets when have aged
    HX_RESULT SendPackets(BOOL bSendAll = FALSE, INT32 nStreamNumber = ALL_STREAMS);
    HX_RESULT SendPacketsForStream(BOOL bSendAll, INT32 nStreamNumber);
    HX_RESULT CreateLostPacket(UINT32 ulStream, CQueueEntry* pQueueEntry);

    BOOL m_bInitialized;

    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pClassFactory;
    IHXScheduler* m_pScheduler;
    IHXRawSinkObject* m_pSinkObject;
    UINT32 m_ulStreamCount;
    UINT32 m_ulStreamCountTerminated;
    UINT32 m_nAgingDuration;
    UINT32 m_nPacketSendInterval;

    BOOL m_bFirstPacket;
    BOOL m_bDelayForOrdering;

    //	These variables are used for aging packets
    CPacketAgedCallback** m_ppPacketAgedCallback;

    //	Queue implementation for ordering packets
    CInorderPacketQueue* m_pqueueInOrderPacket;

//#define _LOG_PACKET_ORDERING

#ifdef _LOG_PACKET_ORDERING

#define _LOG_PACKET_ORDERING_FILENAME "PacketOrdering.txt"

    FILE* m_hLogFile;
#endif	//  _LOG_PACKET_ORDERING
};


#endif /* _PACKET_REORDER_SHIM_H_ */
