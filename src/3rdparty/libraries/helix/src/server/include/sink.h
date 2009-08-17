/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sink.h,v 1.10 2005/10/10 20:03:36 jzeng Exp $ 
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

#ifndef _SINK_H_
#define _SINK_H_

#include "hxcom.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxsrc.h"

_INTERFACE IHXServerPacketSource;
class ServerPacket;
_INTERFACE IHXList;

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPSinkControl
 * 
 *  Purpose:
 * 
 *      Control of Sinks
 * 
 *  IID_IHXPSinkControl:
 * 
 *      {00000300-b4c8-11d0-9995-00a0248da5f0}
 *
 * 
 */
DEFINE_GUID(IID_IHXPSinkControl, 0x00000300, 0xb4c8, 0x11d0, 0x99, 0x95,
		    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXPSinkControl

DECLARE_INTERFACE_(IHXPSinkControl, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(InitDone)     	(THIS_
				HX_RESULT		ulStatus) PURE;

    STDMETHOD(FileHeaderReady) 	(THIS_
				HX_RESULT		ulStatus,
				IHXValues*		pHeader) PURE;

    STDMETHOD(StreamHeaderReady)(THIS_
				HX_RESULT		ulStatus,
				IHXValues*		pHeader) PURE;

    STDMETHOD(StreamDone)  	(THIS_
				UINT16			unStreamNumber) PURE;

    STDMETHOD(SeekDone)  	(THIS_
				HX_RESULT		ulStatus) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPSinkPackets
 * 
 *  Purpose:
 * 
 *      Packet Sink
 * 
 *  IID_IHXPSinkPackets:
 * 
 *      {00000301-b4c8-11d0-9995-00a0248da5f0}
 *
 * 
 */
DEFINE_GUID(IID_IHXPSinkPackets, 0x00000301, 0xb4c8, 0x11d0, 0x99, 0x95,
		    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXPSinkPackets

DECLARE_INTERFACE_(IHXPSinkPackets, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(PacketReady)  	(THIS_
				HX_RESULT		ulStatus,
				IHXPacket*		pPacket) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPSinkInfo
 * 
 *  Purpose:
 * 
 *      Packet Sink Information
 * 
 *  IID_IHXPSinkInfo:
 * 
 *      {00000302-b4c8-11d0-9995-00a0248da5f0}
 *
 * 
 */
DEFINE_GUID(IID_IHXPSinkInfo, 0x00000302, 0xb4c8, 0x11d0, 0x99, 0x95,
		    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);
#undef  INTERFACE
#define INTERFACE   IHXPSinkInfo

DECLARE_INTERFACE_(IHXPSinkInfo, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD_(ULONG32,GetBandwidth)  	(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXServerPacketSink
 * 
 *  Purpose:
 * 
 *      Packet Sink
 * 
 *  IID_IHXServerPacketSink:
 * 
 *      {00000303-b4c8-11d0-9995-00a0248da5f0}
 *
 * 
 */
DEFINE_GUID(IID_IHXServerPacketSink, 0x00000303, 0xb4c8, 0x11d0, 0x99, 0x95,
		    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXServerPacketSink

DECLARE_INTERFACE_(IHXServerPacketSink, IUnknown)
{
    STDMETHOD(SetSource) (THIS_
                          IHXServerPacketSource* pSource) PURE;

    //
    // Called by a ServerPacketSource.  When EWouldBlock is
    // returned, the Source must stop invoking PacketReady
    // until we call its WouldBlockCleared method
    //
    STDMETHOD(PacketReady)  	(THIS_ ServerPacket* pPacket) PURE;
    
    
    //
    // Called by a ServerPacketSource as a hint to the sink that
    // packets won't be coming for a while and it would be a good
    // time to flush its buffer.  The sink doesn't have to obey
    //
    STDMETHOD(Flush)(THIS) PURE;

    STDMETHOD(SourceDone)(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXLivePacketBufferQueue
 * 
 *  Purpose:
 * 
 *      Packet buffer to the previous key frame 
 * 
 *  IHXLivePacketBufferQueue:
 * 
 *      {4a17f495-0e71-437e-af2c-b5d8e5c07a88}
 * 
 */
DEFINE_GUID(IID_IHXLivePacketBufferQueue, 0x4a17f495, 0x0e71, 0x437e, 0xaf, 0x2c,
            0xb5, 0xd8, 0xe5, 0xc0, 0x7a, 0x88);

#undef  INTERFACE
#define INTERFACE   IHXLivePacketBufferQueue

DECLARE_INTERFACE_(IHXLivePacketBufferQueue, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(GetPacket)        (THIS_
                                 UINT32 ulIndex,
                                 IHXPacket*& pPacket) PURE;
    STDMETHOD_(ULONG32,GetSize) (THIS) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXLivePacketBufferProvider
 *
 *  Purpose:
 *
 *      Packet buffer to the previous key frame
 *
 *  IID_IHXLivePacketBufferProvider:
 *
 *      {493f4272-d80f-411b-b76a-c7d9bb4be8ee}
 *
 */
DEFINE_GUID(IID_IHXLivePacketBufferProvider, 0x493f4272, 0xd80f, 0x411b, 0xb7, 0x6a,
        0xc7, 0xd9, 0xbb, 0x4b, 0xe8, 0xee);
#undef  INTERFACE
#define INTERFACE   IHXLivePacketBufferProvider

DECLARE_INTERFACE_(IHXLivePacketBufferProvider, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;

    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(GetPacketBufferQueue)    (THIS_
                                        UINT16 strmNum,
                                        UINT16 ruleNum,
                                        IHXLivePacketBufferQueue*& pQueue) PURE;
};

/****************************************************************************
 *
 *  Interface:
 *
 *      IHXTokenBufferFilter
 *
 *  Purpose:
 *
 *      A general purpose token buffer filter
 *
 *  IID_IHXTokenBufferFilter:
 *
 *      {6682dae4-92ed-4f3c-80c4-7e8ed4c178eb}
 *
 */
DEFINE_GUID(IID_IHXTokenBufferFilter, 0x6682dae4, 0x92ed, 0x4f3c, 0x80, 0xc4,
        0x7e, 0x8e, 0xd4, 0xc1, 0x78, 0xeb);
#undef  INTERFACE
#define INTERFACE   IHXLivePacketBufferProvider

DECLARE_INTERFACE_(IHXTokenBufferFilter, IUnknown)
{
    STDMETHOD(Init)             (THIS_
                                    IUnknown* pContext) PURE;

    STDMETHOD(AddTokens)        (THIS_
                                    UINT32 ulTokens) PURE;

    STDMETHOD(RemoveTokens)     (THIS_
                                    UINT32 ulTokens) PURE;

    STDMETHOD_(INT32,GetTokenCount) (THIS) PURE;

    STDMETHOD(SetMinTokenCeiling) (THIS_
                                    UINT32 ulMinCeiling) PURE;

    STDMETHOD(SetBandwidth)     (THIS_
                                    UINT32 ulBandwidth) PURE;
    
    STDMETHOD(UpdateTokens)     (THIS) PURE;

};

#endif /* _SINK_H_ */
