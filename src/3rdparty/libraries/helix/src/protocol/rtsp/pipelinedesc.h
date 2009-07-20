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

#ifndef PIPELINE_DESC_H
#define PIPELINE_DESC_H

#include "hxtypes.h"
#include "hxstring.h"
#include "hxslist.h"
#include "rtspmsg.h"

class RTSPClientProtocol;

class PipelinedDescribeLogic
{
public:
    virtual ~PipelinedDescribeLogic();

    virtual HX_RESULT OnInit(IUnknown* pContext, IHXValues* pInfo,
                             RTSPClientProtocol* pRTSPProto) = 0;
    virtual HX_RESULT OnOptionsSent(RTSPRequestMessage* pMsg) = 0;
    virtual HX_RESULT OnSendStreamDescribeRequest() = 0;
    virtual void OnDescribeSent(RTSPRequestMessage* pMsg) = 0;
    virtual void OnRedirect(RTSPResponseMessage* pResp) = 0;
    virtual void OnOptionsResponse(RTSPResponseMessage* pResp) = 0;
    virtual HXBOOL OnDescribeResponse(RTSPResponseMessage* pResp, 
                                    REF(HX_RESULT) res) = 0;
};

class CHXPipelinedDescribeLogic : public PipelinedDescribeLogic
{
public:
    CHXPipelinedDescribeLogic();
    virtual ~CHXPipelinedDescribeLogic();

    virtual HX_RESULT OnInit(IUnknown* pContext, IHXValues* pInfo,
                             RTSPClientProtocol* pRTSPProto);

    virtual HX_RESULT OnOptionsSent(RTSPRequestMessage* pMsg);
    virtual HX_RESULT OnSendStreamDescribeRequest();
    virtual void OnDescribeSent(RTSPRequestMessage* pMsg);
    virtual void OnRedirect(RTSPResponseMessage* pResp);
    virtual void OnOptionsResponse(RTSPResponseMessage* pResp);
    virtual HXBOOL OnDescribeResponse(RTSPResponseMessage* pResp, 
                                    REF(HX_RESULT) res);

private:
    enum State
    {
        pdsIdle,
        pdsDescribeSent,
        pdsResponseRecvd,
        pdsSendStreamDescPending,
        pdsError
    };

    HX_RESULT cacheDescribeResponse(RTSPResponseMessage* pMsg);
    void      transitionToIdle();
    HXBOOL PipelineDescribe();

    RTSPClientProtocol*  m_pRTSPProto;
    IUnknown*            m_pContext;
    CHXString		 m_path;
    State                m_state;
    UINT32               m_uDescSeqNo;
    RTSPResponseMessage* m_pCachedDescResp;
};
#endif /* PIPELINE_DESC_H */
