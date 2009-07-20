/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxqostran.h,v 1.13 2005/01/18 21:33:51 damonlan Exp $ 
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

#ifndef _HX_QOS_TRAN_H_
#define _HX_QOS_TRAN_H_

/* foreward declaration of interfaces */
typedef _INTERFACE IHXQoSSignalBus           IHXQoSSignalBus;
typedef _INTERFACE IHXScheduler              IHXScheduler;
typedef _INTERFACE IHXServerPacketSource     IHXServerPacketSource;
typedef _INTERFACE IHXQoSRateShaper          IHXQoSRateShaper;

typedef UINT8 HX_QOS_CC_TYPE;

/* Congestion Control Types */
#define HX_QOS_CC_TYPE_NONE    1
#define HX_QOS_CC_TYPE_TFRC    2
#define HX_QOS_CC_TYPE_AIMD    3
#define HX_QOS_CC_TYPE_BCC     4
#define HX_QOS_CC_TYPE_DEFAULT HX_QOS_CC_TYPE_NONE   

/*
 * IHXQoSCongestionControl
 * {99C0A316-FBBC-41a9-9628-EECCCCF2203F}
 */
DEFINE_GUID(IID_IHXQoSCongestionControl, 
	    0x99c0a316, 0xfbbc, 0x41a9, 0x96, 0x28, 0xee, 0xcc, 0xcc, 0xf2, 0x20, 0x3f);

#define CLSID_IHXQoSCongestionControl IID_IHXQoSCongestionControl

#undef  INTERFACE
#define INTERFACE   IHXQoSCongestionControl 
DECLARE_INTERFACE_(IHXQoSCongestionControl, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXQoSCongestionControl
    STDMETHOD (Init)   (THIS_ IHXBuffer* pSessionId, 
			UINT16             unStrmNumber,
			UINT32 /* bytes */ ulMediaRate,
			UINT32 /* bytes */ ulPacketSize) PURE;
};

/*
 * IHXQoSCongestionEquation
 * {CE97019F-6A3D-4c26-8155-C8AEBADC7762}
 */
DEFINE_GUID(IID_IHXQoSCongestionEquation, 
	    0xce97019f, 0x6a3d, 0x4c26, 0x81, 0x55, 0xc8, 0xae, 0xba, 0xdc, 0x77, 0x62);

#define CLSID_IHXQoSCongestionEquation IID_IHXQoSCongestionEquation

#undef  INTERFACE
#define INTERFACE   IHXQoSCongestionEquation
DECLARE_INTERFACE_(IHXQoSCongestionEquation, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXQoSCongestionEquation
    STDMETHOD (Init)    (THIS_ IUnknown* pContext,
			 IHXBuffer* pSessionId,
			 UINT16 nStreamNumber) PURE;

    STDMETHOD (Update)   (THIS_ UINT32 ulNumRecvd,
			  UINT32 ulNumLoss,
			  double fInboundRate,
			  double fOutboundRate,
			  double fRawLoss, 
			  double fRawRTT) PURE;

    STDMETHOD (FeedbackTimeout) (THIS) PURE;

    STDMETHOD (SetMediaRate) (THIS_ UINT32 ulMediaRate, BOOL bReferenceRate) PURE;

    STDMETHOD (SetMediaPacketSize) (THIS_ UINT32 ulMediaPacketSize) PURE;

    STDMETHOD (SetMaximumRate) (THIS_ UINT32 ulMaxRate) PURE;

    STDMETHOD (GetRate) (THIS_ REF(UINT32) /* OUT */ ulRate) PURE;

    STDMETHOD (GetEquationType) (THIS_ REF(HX_QOS_CC_TYPE) /* OUT */ cType) PURE;
};

/*
 * IID_IHXQoSRateShapeAggregator 
 * {18496BEA-F2A7-45fc-B312-885214119AB2}
 */
DEFINE_GUID(IID_IHXQoSRateShapeAggregator, 
	    0x18496bea, 0xf2a7, 0x45fc, 0xb3, 0x12, 0x88, 0x52, 0x14, 0x11, 0x9a, 0xb2);

#define CLSID_IHXQoSRateShapeAggregator IID_IHXQoSRateShapeAggregator

#undef  INTERFACE
#define INTERFACE   IHXQoSRateShapeAggregator
DECLARE_INTERFACE_(IHXQoSRateShapeAggregator, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXQoSRateShapeAggregator
    STDMETHOD (Init)             (THIS_ UINT16 nStreamCount) PURE;

    STDMETHOD (Done)             ( THIS ) PURE;

    STDMETHOD (AddRateShaper)    (THIS_ IHXQoSRateShaper* pShaper) PURE;

    STDMETHOD (RemoveRateShaper) (THIS_ IHXQoSRateShaper* pShaper) PURE;

    STDMETHOD (AddTokens)        (THIS_ UINT16 nTokens, UINT16 nFromStream) PURE;

    STDMETHOD (RemoveTokens)     (THIS_ UINT16 nTokens, UINT16 nFromStream) PURE;

    STDMETHOD (GetTokens)        (THIS_ REF(UINT16) /*OUT*/ nTokens) PURE;
};

/*
 * IID_IHXQoSRateShaper
 * {2A287694-EF37-4266-858F-E84FE36ABD90}
 */
DEFINE_GUID(IID_IHXQoSRateShaper, 
	    0x2a287694, 0xef37, 0x4266, 0x85, 0x8f, 0xe8, 0x4f, 0xe3, 0x6a, 0xbd, 0x90);

#undef  INTERFACE
#define INTERFACE   IHXQoSRateShaper
DECLARE_INTERFACE_(IHXQoSRateShaper, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   ( THIS_ REFIID riid, void** ppvObj ) PURE;
    STDMETHOD_(ULONG32,AddRef)  ( THIS ) PURE;
    STDMETHOD_(ULONG32,Release) ( THIS ) PURE;

    // IHXQoSRateShaper
    STDMETHOD (Init)             (THIS_ IHXQoSRateShapeAggregator* pAggregator) PURE;

    STDMETHOD (StreamCleared)    (THIS_ UINT16 nStreamNumber) PURE;

    STDMETHOD (GetStreamNumber)  (THIS_ REF(UINT16) /*OUT*/ nStreamNumber) PURE;

    STDMETHOD (GetMaxTokens)     (THIS_ REF(UINT32) /*OUT*/ ulMaxTokens) PURE;
};


#endif /* _HX_QOS_TRAN_H_ */
