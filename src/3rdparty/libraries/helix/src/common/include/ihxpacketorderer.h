/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxpacketorderer.h,v 1.4 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _IHX_PACKET_ORDERER_H_
#define _IHX_PACKET_ORDERER_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXRawSinkObject		IHXRawSinkObject;

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPacketOrderer
 * 
 *  Purpose:
 * 
 *      Uses scheduler to age and order packets when ordered packets are expected
 *	but unordered packets are received.
 * 
 *  IID_IHXPacketOrderer:
 * 
 *      {132BCEAF-BE58-4090-929C-50736DB30E68}
 *
 * 
 */
DEFINE_GUID(IID_IHXPacketOrderer, 
0x132bceaf, 0xbe58, 0x4090, 0x92, 0x9c, 0x50, 0x73, 0x6d, 0xb3, 0xe, 0x68);

const INT32 ALL_STREAMS = -1;

#undef  INTERFACE
#define INTERFACE   IHXPacketOrderer

DECLARE_INTERFACE_(IHXPacketOrderer, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    //	*** IHXPacketOrderer ***
    STDMETHOD(Initialize)(THIS_ IUnknown* pContext, IHXRawSinkObject* pSinkObject, UINT32 ulStreamCount, UINT32 nAgingDuration, UINT32 nPacketSendInterval) PURE;
    STDMETHOD(Terminate)(THIS_ INT32 nStreamNumber = ALL_STREAMS) PURE;
    STDMETHOD(GetContext)(THIS_ IUnknown** ppContext) PURE;
    STDMETHOD(GetOrderingDuration)(THIS_ UINT32* pnAgingDuration) PURE;
    STDMETHOD(GetPacketSendInterval)(THIS_ UINT32* pnPacketSendInterval) PURE;
    STDMETHOD(GetStreamCount)(THIS_ UINT32* pulStreamCount) PURE;
    STDMETHOD(GetPacketSink)(THIS_ IHXRawSinkObject** ppSinkObject) PURE;
};

#endif	//  _IHX_PACKET_ORDERER_H_
