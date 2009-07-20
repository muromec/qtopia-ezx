/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsmbw.h,v 1.8 2008/10/20 19:35:52 ping Exp $
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _HXSMBW_H_
#define _HXSMBW_H_
#include "hxcom.h"

/*
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE	IHXPlayer		    IHXPlayer;
typedef _INTERFACE	IHXBandwidthManagerInput   IHXBandwidthManagerInput;
class HXSource;
class CHXSimpleList;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXBandwidthManager
 *
 *  Purpose:
 *
 *	Implemented by the ASM Manager to allow sources to register
 *	themselves.
 *
 *  IID_IHXBandwidthManager:
 *
 *	{00000800-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXBandwidthManager, 0x00000800, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXBandwidthManager

DECLARE_INTERFACE_(IHXBandwidthManager, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXBandwidthManager methods
     */
    STDMETHOD(RegisterSource)	(THIS_
				HXSource* pSource,
				IUnknown* pUnknown) PURE;

    STDMETHOD(RegisterSourcesDone)	(THIS) PURE;

    STDMETHOD_(HXBOOL, NotEnoughBandwidth)(THIS) PURE;

    STDMETHOD(UnRegisterSource)	(THIS_
				HXSource* pSource) PURE;

    /* If the source has enough data, it may tell the bandwidth
     * manager to cut down on accelerated buffering. 
     */
    STDMETHOD(ChangeAccelerationStatus)	(THIS_
				HXSource* pSource,
				HXBOOL	   bMayBeAccelerated,
				HXBOOL	   bUseAccelerationFactor,
				UINT32	   ulAccelerationFactor) PURE;

    /* Called by HXPlayer at end of each presentation.  */
    STDMETHOD(PresentationDone)	(THIS) PURE;

    STDMETHOD(ChangeBW) (THIS_ 
		UINT32 newBW, 
		HXSource* pSource) PURE;

    /* Returns the amount of bandwidth available for upshifting */
    STDMETHOD(GetUpshiftBW) (THIS_ REF(UINT32) uUpshiftBW) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXSourceBandwidthInfo
 *
 *  Purpose:
 *
 *	Implemented by RTSP Transport to associate it with the ASM
 *	manager and set the transmission rate.
 *
 *  IID_IHXSourceBandwidthInfo:
 *
 *	{00000801-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXSourceBandwidthInfo, 0x00000801, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXSourceBandwidthInfo

DECLARE_INTERFACE_(IHXSourceBandwidthInfo, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXSourceBandwidthInfo methods
     */
    STDMETHOD(InitBw)		(THIS_
				IHXBandwidthManagerInput* pBwMgr) PURE;

    STDMETHOD(SetTransmitRate)	(THIS_
				UINT32 ulBitRate) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXBandwidthManagerInput
 *
 *  Purpose:
 *
 *	Implemented by ASM Managers to allow the transport to report
 *	information about bandwidth usage & stats.
 *
 *  IID_IHXBandwidthManagerInput:
 *
 *	{00000802-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXBandwidthManagerInput, 0x00000802, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXBandwidthManagerInput

typedef enum _transport_type
{
    TNG_TCP, TNG_UDP
} TRANSPORT_TYPE;


#define REPORT_DATA_PACKET_LOST 0xffffffff

DECLARE_INTERFACE_(IHXBandwidthManagerInput, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXBandwidthManagerInput methods
     */
    STDMETHOD(ReportDataPacket)		(THIS_
					UINT32 ulTimeStamp,
					UINT32 ulArrivedTimeStamp,
					UINT32 ulSize) PURE;

    STDMETHOD(ReportUpshiftInfo)	(THIS_
					UINT32 ulTimeStamp,
					UINT32 ulSize) PURE;

    STDMETHOD(ReportLatency)		(THIS_
					UINT32 ulServerTime,
					UINT32 ulClientTime) PURE;

    STDMETHOD(SetCongestionFactor)	(THIS_
					UINT32 ulFactor) PURE;

    STDMETHOD(SetTransportType)		(THIS_
					TRANSPORT_TYPE type) PURE;


};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXStreamBandwidthNegotiator
 *
 *  Purpose:
 *
 *	Implemented by ASM Stream objects to give basic information to
 *	the ASM manager.
 *
 *  IID_IHXStreamBandwidthNegotiator:
 *
 *	{00000803-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXStreamBandwidthNegotiator, 0x00000803, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXStreamBandwidthNegotiator

DECLARE_INTERFACE_(IHXStreamBandwidthNegotiator, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXStreamBandwidthNegotiator methods
     */
    STDMETHOD(SetBandwidthUsage)	(THIS_
					REF(UINT32) ulRecvBitRate,
					REF(HXBOOL)   bTimeStampDelivery) PURE;

    STDMETHOD(HandleSlowSource)         (THIS_
					UINT32 ulRecvBitRate) PURE;

    STDMETHOD(GetFixedBandwidth)	(THIS_
					REF(UINT32) ulBitRate) PURE;

    STDMETHOD(GetThresholdInfo)		(THIS_
					float*	    pThreshold,
					REF(UINT32) ulNumThreshold) PURE;

    STDMETHOD(UnRegister)		(THIS) PURE;

    STDMETHOD_(ULONG32,GetNumThresholds)(THIS) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXStreamBandwidthBias
 *
 *  Purpose:
 *
 *	Implemented by the ASM stmeams (queried from IHXStream)
 *	in order to set/get the current bias factor for
 *      allocation.
 *
 *  IID_IHXStreamBandwidthBias:
 *
 *	{00000804-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXStreamBandwidthBias, 0x00000804, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXStreamBandwidthBias

DECLARE_INTERFACE_(IHXStreamBandwidthBias, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXStreamBandwidthBias methods
     */
    STDMETHOD(GetBiasFactor)	(THIS_
				REF(INT32) lBiasFactor) PURE;

    STDMETHOD(SetBiasFactor)	(THIS_
				INT32 lBiasFactor) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXThinnableSource
 *
 *  Purpose:
 *
 *	Implemented by protocols that allow infinite thinnability through
 *      LimitBandwidthByDropping
 *
 *  IID_IHXThinnableSource
 *
 *	{00000805-b4c8-11d0-9995-00a0248da5f0}
 *
 */

DEFINE_GUID(IID_IHXThinnableSource, 0x00000805, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXThinnableSource

DECLARE_INTERFACE_(IHXThinnableSource, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXThinnableSource methods
     */
    STDMETHOD(LimitBandwidthByDropping)	(THIS_
					UINT32 ulStreamNo,
					UINT32 ulBandwidthLimit) PURE;

    STDMETHOD(SetDeliveryBandwidth)	(THIS_
					UINT32 ulBandwidth,
					UINT32 ulMsBackOff) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXBandwidthNudger
 *
 *  Purpose:
 *
 *	Implemented by the ASM Bandwidth manager (query off IHXPlayer).
 *      This interface allows you to artifically simulate bandwidth
 *	constrained conditions.
 *
 *  IID_IHXBandwidthNudger
 *
 *	{00000806-b4c8-11d0-9995-00a0248da5f0}
 *
 */

DEFINE_GUID(IID_IHXBandwidthNudger, 0x00000806, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXBandwidthNudger

DECLARE_INTERFACE_(IHXBandwidthNudger, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXBandwidthNudger methods
     */
    STDMETHOD(GetBandwidth)	(THIS_
				REF(UINT32) ulBandwidth) PURE;

    STDMETHOD(SetBandwidth)	(THIS_
				UINT32 ulBandwidth) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXASMProps
 *
 *  Purpose:
 *
 *	Implemented by the ASM Stream Object (query off IHXStream).
 *      This interface allows you to get the current predata as stated
 *	in the rulebook.
 *
 *  IID_IHXASMProps
 *
 *	{00000807-b4c8-11d0-9995-00a0248da5f0}
 *
 */

DEFINE_GUID(IID_IHXASMProps, 0x00000807, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXASMProps

DECLARE_INTERFACE_(IHXASMProps, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXASMProps methods
     */
    STDMETHOD(GetPreData)	(THIS_
				REF(UINT32) ulPreData) PURE;
    STDMETHOD(GetBandwidth)	(THIS_
				REF(UINT32) ulBandwidth) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAtomicRuleChange
 *
 *  Purpose:
 *
 *	Implemented by the RTSP layer in rmacore.
 *      This interface allows you to atomically subscribe and unsubscribe
 *	to a bunch of rules.
 *
 *  IID_IHXAtomicRuleChange
 *
 *	{00000808-b4c8-11d0-9995-00a0248da5f0}
 *
 */

DEFINE_GUID(IID_IHXAtomicRuleChange, 0x00000808, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXAtomicRuleChange

DECLARE_INTERFACE_(IHXAtomicRuleChange, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXAtomicRuleChange methods
     */
    STDMETHOD(RuleChange)    (THIS_
			     REF(CHXSimpleList) RuleChanges) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAtomicRuleGather
 *
 *  Purpose:
 *
 *	Implemented by the RTSP layer in rmacore.
 *      This interface allows you to atomically subscribe and unsubscribe
 *	to a bunch of rules.
 *
 *  IID_IHXAtomicRuleGather
 *
 *	{00000809-b4c8-11d0-9995-00a0248da5f0}
 *
 */

DEFINE_GUID(IID_IHXAtomicRuleGather, 0x00000809, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXAtomicRuleGather

DECLARE_INTERFACE_(IHXAtomicRuleGather, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXAtomicRuleGather methods
     */
    STDMETHOD(RuleGather)    (THIS_
			     CHXSimpleList* pList) PURE;
    STDMETHOD(RuleFlush)    (THIS_
			     CHXSimpleList* pList) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPlayerState
 *
 *  Purpose:
 *
 *	Implemented by the HXPlayer object in rmacore.
 *      This interface allows you to determine if we are in a playing
 *	state.
 *
 *  IID_IHXPlayerState
 *
 *	{0000080A-b4c8-11d0-9995-00a0248da5f0}
 *
 */

DEFINE_GUID(IID_IHXPlayerState, 0x0000080A, 0xb4c8, 0x11d0,
	    0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXPlayerState

DECLARE_INTERFACE_(IHXPlayerState, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXPlayerState methods
     */
    STDMETHOD_(HXBOOL, IsPlaying)    (THIS) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXBandwidthManager)
DEFINE_SMART_PTR(IHXSourceBandwidthInfo)
DEFINE_SMART_PTR(IHXBandwidthManagerInput)
DEFINE_SMART_PTR(IHXStreamBandwidthNegotiator)
DEFINE_SMART_PTR(IHXStreamBandwidthBias)
DEFINE_SMART_PTR(IHXThinnableSource)
DEFINE_SMART_PTR(IHXBandwidthNudger)
DEFINE_SMART_PTR(IHXASMProps)
DEFINE_SMART_PTR(IHXAtomicRuleChange)
DEFINE_SMART_PTR(IHXAtomicRuleGather)
DEFINE_SMART_PTR(IHXPlayerState)

#endif /* _HXSMBW_H_ */
