/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0 
 *  
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved. 
 *  
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of RealNetworks Community Source License 
 * Version 1.0 (the "License"). You may not use this file except in 
 * compliance with the License executed by both you and RealNetworks.  You 
 * may obtain a copy of the License at  
 * http://www.helixcommunity.org/content/rcsl.  You may also obtain a 
 * copy of the License by contacting RealNetworks directly.  Please see the 
 * License for the rights, obligations and limitations governing use of the 
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
/*
 *  $Id: hxpcktflwctrl.h,v 1.7 2005/07/07 22:03:42 jzeng Exp $
 *
 *  declaration of IHXSessionControl interface
 */
#ifndef _HXPCKTFLWCTRL_H_
#define _HXPCKTFLWCTRL_H_

#include "hxtypes.h"
#include "hxcom.h" // for IUnknown base class

// this interface isn't completely COM-compliant due to the following non-COM
// dataypes used as method arguments
class Transport;
class ASMRuleBook;

_INTERFACE IHXValues;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSessionControl
 * 
 *  Purpose:
 * 
 *	Packet Flow Session Control
 * 
 *  IID_IHXPacketFlowControl
 * 
 *     {00000130-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXPacketFlowControl, 
    0x00000130, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPacketFlowControl

DECLARE_INTERFACE_(IHXPacketFlowControl, IUnknown)
{
 public:
    STDMETHOD(Play) (THIS) PURE;

    STDMETHOD(StartSeek) (THIS_
                          UINT32 ulTime) PURE;

    STDMETHOD(Activate)(THIS) PURE;

    STDMETHOD(WantWouldBlock)(THIS) PURE;

    STDMETHOD(SeekDone)(THIS) PURE;

    STDMETHOD(SetEndPoint)(THIS_
                           UINT32 ulEndPoint, 
                           BOOL bPause) PURE;

    STDMETHOD(SetStartingTimestamp)(THIS_ 
                                UINT32 ulStartingTimestamp) PURE;

    STDMETHOD(RegisterStream)(THIS_
                              Transport* pTransport,
                              UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader) PURE;

    STDMETHOD(RegisterStream)(THIS_
                              Transport* pTransport,
                              UINT16 uStreamGroupNumber,
                              UINT16 uStreamNumber,
                              ASMRuleBook* pRuleBook,
                              IHXValues* pHeader) PURE;

    STDMETHOD(GetSequenceNumber)(THIS_
                                 UINT16 uStreamNumber,
                                 UINT16& uSequenceNumber) PURE;

    STDMETHOD(Pause)(THIS_
                     UINT32 ulPausePoint) PURE;

    STDMETHOD(StreamDone)(THIS_
                          UINT16 uStreamNumber, BOOL bForce = FALSE) PURE;

    STDMETHOD(SetDropRate)(THIS_
                           UINT16 uStreamNumber, 
                           UINT32 uDropRate) PURE;

    STDMETHOD(SetDropToBandwidthLimit)(THIS_
                                       UINT16 uStreamNumber,
                                       UINT32 ulBandwidthLimit) PURE;

    STDMETHOD(SetDeliveryBandwidth)(THIS_
                                    UINT32 ulBackOff,
                                    UINT32 ulBandwidth) PURE;

    STDMETHOD(HandleSubscribe)(THIS_
                               INT32 lRuleNumber,
                               UINT16 unStreamNumber) PURE;

    STDMETHOD(HandleUnSubscribe)(THIS_
                                 INT32 lRuleNumber,
                                 UINT16 unStreamNumber) PURE;

    STDMETHOD_(ULONG32, GetDeliveryRate)(THIS) PURE;

    STDMETHOD(ControlDone)(THIS) PURE;

    STDMETHOD_(float, SetSpeed)(THIS_
                                float fSpeed) PURE;

    STDMETHOD(SetBandwidth)(THIS_
                            UINT32 ulBandwidth) PURE;
};


#endif // _HXPCKTFLWCTRL_H_
