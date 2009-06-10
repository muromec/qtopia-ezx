/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrtspprot2.h,v 1.1 2006/12/21 19:05:06 tknox Exp $
 *
 * Portions Copyright (c) 1995-2007 RealNetworks, Inc. All Rights Reserved.
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef _HX_RTSP_PROT2_H_
#define _HX_RTSP_PROT2_H_

#include "hxcom.h"
#include "hxtypes.h"

#include "ihxtrans.h"

DEFINE_GUID(IID_IHXRTSPProtocol2, 0xa960b96b, 0x6f55, 0x4c6b,
            0xad, 0x97, 0x7d, 0x76, 0x92, 0xd3, 0xcd, 0xb2);

#undef  INTERFACE
#define INTERFACE   IHXRTSPProtocol2

/**
 * \brief IHXRTSPProtocol2 is a clone of IHXRTSPProtocol but using IHXTransport.
 *
 * IHXRTSPProtocol used RTSPTransport, which made it unsuitable for general purpose
 * use in the transport refactoring work. This lets us change CRTSPProtocol to use
 * only IHXTransport.
 * \note IID_IHXRTSPProtocol2 {A960B96B-6F55-4C6B-AD97-7D7692D3CDB2}
 */
DECLARE_INTERFACE_(IHXRTSPProtocol2, IUnknown)
{
    //* \ref IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    //* \ref IUnknown
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    //* \ref IUnknown
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    // IHXRTSPProtocol2
    STDMETHOD(Init)                 (THIS_ IHXRTSPProtocolResponse* presp,
                                           IUnknown* punkContext) PURE;
    STDMETHOD(GetSocket)            (THIS_ REF(IHXSocket*) pSock) PURE;
    STDMETHOD(SetTransport)         (THIS_ BYTE byChan, IHXTransport* pTran)
                                                                          PURE;
    STDMETHOD(Connect)              (THIS_ const char* szHost,
                                           const char* szPort) PURE;
    STDMETHOD(Accept)               (THIS_ IHXSocket* psock) PURE;
    STDMETHOD(Close)                (THIS) PURE;
    STDMETHOD(SendRequest)          (THIS_ IHXRTSPRequestMessage* pReq) PURE;
    STDMETHOD(SendResponse)         (THIS_ IHXRTSPResponseMessage* pRsp) PURE;
    STDMETHOD(SendPacket)           (THIS_ IHXRTSPInterleavedPacket* pPkt) PURE;
};

#endif /* _HX_RTSP_PROT2_H_ */
