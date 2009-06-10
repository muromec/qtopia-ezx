/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: source.h,v 1.6 2007/08/18 00:21:15 dcollins Exp $ 
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

#ifndef _SOURCE_H_
#define _SOURCE_H_

#include "sink.h"

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPSourceControl
 * 
 *  Purpose:
 * 
 *      Control of Sources
 * 
 *  IID_IHXPSourceControl:
 * 
 *      {00000200-b4c8-11d0-9995-00a0248da5f0}
 *
 * 
 */
DEFINE_GUID(IID_IHXPSourceControl, 0x00000200, 0xb4c8, 0x11d0, 0x99, 0x95,
		    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXPSourceControl

DECLARE_INTERFACE_(IHXPSourceControl, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(Init)     	(THIS_
				IHXPSinkControl*		pSink) PURE;

    STDMETHOD(Done)		(THIS) PURE;

    STDMETHOD(GetFileHeader)	(THIS_
				IHXPSinkControl*		pSink) PURE;

    STDMETHOD(GetStreamHeader)	(THIS_
				IHXPSinkControl*		pSink,
				UINT16 unStreamNumber) PURE;

    STDMETHOD(Seek)		(THIS_
				UINT32		ulSeekTime) PURE;

    STDMETHOD_(BOOL,IsLive)	(THIS) PURE;

    STDMETHOD(SetLatencyParams)	(THIS_
				 UINT32 ulLatency,
                                 BOOL bStartAtTail,
				 BOOL bStartAtHead) PURE;
};


DEFINE_GUID(IID_IHXPSourcePackets, 0x00000201, 0xb4c8, 0x11d0, 0x99,
		    0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXPSourcePackets

DECLARE_INTERFACE_(IHXPSourcePackets, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(Init)     	(THIS_
				IHXPSinkPackets*	pSinkPackets) PURE;

    STDMETHOD(GetPacket)	(THIS_
    				UINT16 unStreamNumber) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPSourceLivePackets
 * 
 *  Purpose:
 * 
 *      Source of Live Packets
 * 
 *  IID_IHXPSourceControl:
 * 
 *      {2BFB61AD-BDA0-4cd1-A859-576C95C6BA08}
 *
 * 
 */

DEFINE_GUID(IID_IHXPSourceLivePackets, 
	    0x2bfb61ad, 0xbda0, 0x4cd1, 0xa8, 0x59, 0x57, 0x6c, 0x95, 0xc6, 0xba, 0x8);

#undef  INTERFACE
#define INTERFACE   IHXPSourceLivePackets

DECLARE_INTERFACE_(IHXPSourceLivePackets, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(Init)     	(THIS_
				IHXPSinkPackets*	pSinkPackets) PURE;

    STDMETHOD(StartPackets)	(THIS_
				 UINT16 unStreamNumber) PURE;

    STDMETHOD(StopPackets)	(THIS_
				 UINT16 unStreamNumber) PURE;
};


DEFINE_GUID(IID_IHXPSourceLiveResync, 0x00000202, 0xb4c8, 0x11d0, 0x99,
		    0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXPSourceLiveResync

DECLARE_INTERFACE_(IHXPSourceLiveResync, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(Resync)		(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXServerPacketSource
 * 
 *  Purpose:
 * 
 *      Source of packets
 * 
 *  IID_IHXServerPacketSource:
 * 
 *      {00000203-b4c8-11d0-99-98-0-a0-24-8d-a5-f0}
 *
 * 
 */
DEFINE_GUID(IID_IHXServerPacketSource, 0x00000203, 0xb4c8, 0x11d0, 0x99,
		    0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXServerPacketSource

DECLARE_INTERFACE_(IHXServerPacketSource, IUnknown)
{
    STDMETHOD(SetSink)(THIS_ IHXServerPacketSink* pSink) PURE;
    STDMETHOD(StartPackets) (THIS) PURE;
    STDMETHOD(GetPacket) (THIS) PURE;

    //
    // Called when the Sink's PacketReady had returned a
    // WouldBlock error which is cleared
    //
    STDMETHOD(SinkBlockCleared)(THIS_ UINT32 ulStream) PURE;
    STDMETHOD (EnableTCPMode) (THIS) PURE;
};


#endif /* _SOURCE_H_ */
