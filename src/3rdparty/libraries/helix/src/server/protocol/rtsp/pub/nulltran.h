/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: nulltran.h,v 1.6 2006/12/21 19:05:06 tknox Exp $
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
#ifndef _NULLTRAN_H_
#define _NULLTRAN_H_

#include "transport.h"

class NullSetupTransport: public Transport
{
public:
    NullSetupTransport(void);

    // IUnknown methods
    STDMETHOD(QueryInterface)           (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)          (THIS);
    STDMETHOD_(ULONG32,Release)         (THIS);

    // Transport methods
    STDMETHOD_(void, Done)              (THIS) {}
    virtual void Reset                  (void) {}
    virtual void Restart                (void) {}
    virtual RTSPTransportTypeEnum tag   (void) { return RTSP_TR_NULLSET; }
    virtual HX_RESULT sendPacket        (BasePacket* pPacket) { return HXR_OK; }
    virtual HX_RESULT handlePacket      (IHXBuffer* pBuffer) { return HXR_OK; }
    virtual HX_RESULT streamDone        (UINT16 streamNumber,
                                         UINT32 uReasonCode = 0,
                                         const char* pReasonText = NULL) { return HXR_OK; }
    virtual IHXSocket* getUDPSocket     (void) { return NULL; }
    virtual HX_RESULT setFirstSeqNum    (UINT16 streamNumber,
                                         UINT16 seqNum) { return HXR_OK; }
    STDMETHOD_(BOOL,isNullSetup)        (THIS) { return TRUE; }

    virtual HX_RESULT init              (IUnknown* pContext,
                                         IHXUDPSocket* pSocket,
                                         IHXRTSPTransportResponse* pResp)
                                           { return HXR_OK; }

private:
    ~NullSetupTransport();
};

#endif /* _NULLTRAN_H_ */
