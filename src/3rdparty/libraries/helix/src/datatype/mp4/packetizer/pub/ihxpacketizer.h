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
#ifndef _HX_PACKETIZER_H_
#define _HX_PACKETIZER_H_

#include "hxcom.h"
#include "hxtypes.h"

// {CB389E11-BA5D-43e9-8557-094034B8EF49}
DEFINE_GUID(IID_IHXPacketizer, 
0xcb389e11, 0xba5d, 0x43e9, 0x85, 0x57, 0x9, 0x40, 0x34, 0xb8, 0xef, 0x49);

#undef  INTERFACE
#define INTERFACE   IHXPacketizer

// forward declaration
_INTERFACE IHXTMediaSample ;

DECLARE_INTERFACE_(IHXPacketizer, IUnknown)
{
    // push another frame into this packet.
    STDMETHOD(AddSample)(THIS_ IHXTMediaSample* pSample) PURE;
    
    // do we have enough data for one entire packet?
    STDMETHOD_(BOOL, IsPacketReady)(THIS) CONSTMETHOD PURE;
    
    //	Get the size of the next assembled packet,
    //	provided there is one, otherwise return 0.
    STDMETHOD_(UINT32, GetPacketSize)(THIS) CONSTMETHOD PURE;
    
    // assemble a packet. If not enough data has been collected, return
    // HXR_NO_DATA...?
    STDMETHOD(AssemblePacket)(THIS_ IHXTMediaSample* pSample) PURE;
    
    // while we are fragmenting AUs, this returns 1
    STDMETHOD_(BOOL, HasFragments)(THIS) CONSTMETHOD PURE ;

    // reset the packetizer to start state.
    STDMETHOD_(void, Reset)(THIS) PURE ;
};

// {E387ACED-81C0-42cb-AF39-745E71CC79F2}
DEFINE_GUID(IID_IHXPacketizerStrategy, 
0xe387aced, 0x81c0, 0x42cb, 0xaf, 0x39, 0x74, 0x5e, 0x71, 0xcc, 0x79, 0xf2);

#undef  INTERFACE
#define INTERFACE   IHXPacketizerStrategy

DECLARE_INTERFACE_(IHXPacketizerStrategy, IUnknown)
{
    // returns the packet number that the next AU goes into and increments AU number
    STDMETHOD_(UINT32, GetNextPacketIndex)(THIS) PURE ;
    // returns the earliest packet that is still being filled up.
    // packets with earlier indices can be sent away.
    STDMETHOD_(UINT32, GetLowMark)(THIS) CONSTMETHOD PURE ;

    // not sure if we need these. In theory, the interleaver can detect these
    // by doing a trial run. But since supplying these is easy, and the trial run
    // inconvenient at best, let's have these.

    // as defined in the RFC.
    STDMETHOD_(UINT32, GetMaxDisplacement)(THIS) CONSTMETHOD PURE ;

    // as defined in the RFC
    STDMETHOD_(UINT32, GetDeInterleaveBuffersize)(THIS) CONSTMETHOD PURE ;

    // Max gap in sequence numbers of consecutive AUs in one packet
    STDMETHOD_(UINT32, GetMaxIndexDelta)(THIS) CONSTMETHOD PURE ;

    // reset the strategy to use it again.
    STDMETHOD_(void, Reset)(THIS) PURE ;
};

#endif // _HX_PACKETIZER_H_
