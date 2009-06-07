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
#include "pipelinedesc.h"
#include "ihxpckts.h" // IHXValues
#include "hxcore.h"  // IHXPlayer2
#include "hxfiles.h" // IHXRequest
#include "hxprefs.h" // IHXPreferences
#include "hxprefutil.h" // ReadPrefBOOL()
#include "rtspclnt.h" // RTSPClientProtocol

PipelinedDescribeLogic::~PipelinedDescribeLogic()
{}

CHXPipelinedDescribeLogic::CHXPipelinedDescribeLogic() :
    m_pRTSPProto(NULL),
    m_pContext(NULL),
    m_state(pdsIdle),
    m_uDescSeqNo(0),
    m_pCachedDescResp(NULL)
{}

CHXPipelinedDescribeLogic::~CHXPipelinedDescribeLogic()
{
    HX_RELEASE(m_pContext);
    HX_DELETE(m_pCachedDescResp);
    m_pRTSPProto = NULL;
}

HX_RESULT 
CHXPipelinedDescribeLogic::OnInit(IUnknown* pContext,
                                  IHXValues* pInfo,
                                  RTSPClientProtocol* pRTSPProto)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    transitionToIdle();

    if (pContext && pInfo && pRTSPProto)
    {
        HX_RELEASE(m_pContext);

        m_pRTSPProto = pRTSPProto;

        m_pContext = pContext;
        m_pContext->AddRef();

        IHXBuffer* pBuffer = NULL;
        if (pInfo &&
            (HXR_OK == pInfo->GetPropertyCString("path", pBuffer)))
        {
            m_path = (const char*)pBuffer->GetBuffer();
        
            res = HXR_OK;

            HX_RELEASE(pBuffer);
        }
    }

    return res;
}

HX_RESULT CHXPipelinedDescribeLogic::OnOptionsSent(RTSPRequestMessage* pMsg)
{
    HX_RESULT res = HXR_OK;

    if (PipelineDescribe() && !m_path.IsEmpty() && m_pContext && 
        m_pRTSPProto && (pdsIdle == m_state))
    {
        IHXPlayer2* pPlayer = NULL;
        IHXRequest* pRequest = NULL;
        IHXValues* pRequestHdrs = NULL;

        if (HXR_OK == m_pContext->QueryInterface(IID_IHXPlayer2, 
                                                 (void**)&pPlayer) &&
            HXR_OK == pPlayer->GetRequest(pRequest) &&
            HXR_OK == pRequest->GetRequestHeaders(pRequestHdrs))
        {
            m_state = pdsDescribeSent;

            res = m_pRTSPProto->sendPendingStreamDescription(m_path, 
                                                             pRequestHdrs,
                                                             FALSE);

            if (HXR_OK != res)
            {
                // The send failed for some reason.
                // Transition back to the pdsIdle state
                // and set the return code to HXR_OK.
                // We want it to look like we didn't
                // try to pipeline the DESCRIBE.
                transitionToIdle();
                
                res = HXR_OK;
            }
        }

        HX_RELEASE(pRequestHdrs);
        HX_RELEASE(pRequest);
        HX_RELEASE(pPlayer);
    }

    return res;
}

HX_RESULT CHXPipelinedDescribeLogic::OnSendStreamDescribeRequest()
{
    HX_RESULT res = HXR_FAILED;

    switch(m_state) {
    case pdsDescribeSent :
        // A pipelined describe is pending
        m_state = pdsSendStreamDescPending;

        res = HXR_OK;
        break;
    case pdsResponseRecvd :

        // Handle the cached DESCRIBE response.
        if (m_pCachedDescResp)
        {
            RTSPResponseMessage* pMsg = m_pCachedDescResp;
            m_pCachedDescResp = NULL;

            if (m_pRTSPProto)
            {
                m_pRTSPProto->handleDescribeResponse(pMsg);
            }

            delete pMsg;
        }

        transitionToIdle();
        res = HXR_OK;
        break;
    };

    return res;
}

void CHXPipelinedDescribeLogic::OnDescribeSent(RTSPRequestMessage* pMsg)
{
    if (pdsDescribeSent == m_state)
    {
        // Cache the sequence number of the pipelined
        // DESCRIBE request so we can identify the
        // response
        m_uDescSeqNo = pMsg->seqNo();
    }
}

void CHXPipelinedDescribeLogic::OnRedirect(RTSPResponseMessage* pResp)
{
    if (pdsResponseRecvd == m_state)
    {
        // We've received the pipelined DESCRIBE
        // response before a SendStreamDescriptionRequest()
        // call and then we were redirected.
        // Transition to pdsIdle because the redirect
        // basically cancels the pipelined describe
        transitionToIdle();
    }
    else if (pdsIdle != m_state)
    {
        if (pResp->seqNo() == m_uDescSeqNo)
        {
            // The DESCRIBE response has the redirect.
            // We transition to pdsIdle because we know
            // the response is no longer pending
            transitionToIdle();
        }
        else
        {
            // We know the pipelined DESCRIBE response is
            // still pending. Transition to the pdsError
            // state until we receive the response
            m_state = pdsError;
        }
    }
}

void CHXPipelinedDescribeLogic::OnOptionsResponse(RTSPResponseMessage* pResp)
{
    if ((pResp->errorCodeAsUINT32() != 200) && 
        (pdsDescribeSent == m_state))
    {
        // Transition to the pdsError state. This
        // will cause us to ignore the pipelined DESCRIBE response
        // when it arrives. The reason for this is we want to make
        // it look like the pipelined DESCRIBE was never sent
        m_state = pdsError;
    }
}

HXBOOL CHXPipelinedDescribeLogic::OnDescribeResponse(RTSPResponseMessage* pResp, 
                                                   REF(HX_RESULT) res)
{
    HXBOOL bRet = FALSE;

    if ((pdsIdle != m_state) && (pResp->seqNo() == m_uDescSeqNo))
    {
        // This is the response to the pipelined DESCRIBE

        if (pdsError == m_state)
        {
            // We got an error before we got the pipelined DESCRIBE response.
            // Transition to the pdsIdle state and return
            transitionToIdle();

            res = HXR_OK;
            bRet = TRUE;
        }
        else if (pdsDescribeSent == m_state)
        {
            // We have gotten the DESCRIBE response before
            // anyone has called SendStreamDescriptionRequest().
            // We need to cache the message until we receive
            // the SendStreamDescriptionRequest() call.
            // Transition to the pdsResponseRecvd response state

            m_state = pdsResponseRecvd;

            res = cacheDescribeResponse(pResp);
            bRet = TRUE;
        }
        else
        {
            // We are here because a pipelined DESCRIBE response arrived after
            // SendStreamDescriptionRequest() was called or the response
            // was received before the SendStreamDescriptionRequest() call
            // and SendStreamDescriptionRequest() is now calling us with
            // the cached response. In either of these cases we want to
            // transition to pdsIdle and process this response.

            HX_ASSERT((pdsSendStreamDescPending == m_state) ||
                      (pdsResponseRecvd == m_state));

            transitionToIdle();
        }
    }

    return bRet;
}

HX_RESULT 
CHXPipelinedDescribeLogic::cacheDescribeResponse(RTSPResponseMessage* pMsg)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pMsg)
    {
        HX_DELETE(m_pCachedDescResp);

        m_pCachedDescResp = new RTSPResponseMessage;

        if (m_pCachedDescResp)
        {
            // Copy the response message

            m_pCachedDescResp->setVersion(pMsg->majorVersion(),
                                                 pMsg->minorVersion());
            m_pCachedDescResp->setContent(pMsg->getContent());
            
            MIMEHeader* pHdr = pMsg->getFirstHeader();

            res = HXR_OK;
                        
            while(pHdr && (HXR_OK == res))
            {
                MIMEHeader* pNewHdr = new MIMEHeader(*pHdr);
                
                if (pNewHdr)
                {
                    m_pCachedDescResp->addHeader(pNewHdr);

                    pHdr = pMsg->getNextHeader();
                }
                else
                {
                    res = HXR_OUTOFMEMORY;
                }
            }

            m_pCachedDescResp->setSeqNo(pMsg->seqNo());

            m_pCachedDescResp->setErrorCode(pMsg->errorCode());
            m_pCachedDescResp->setErrorMsg(pMsg->errorMsg());

            HX_ASSERT(m_pCachedDescResp->asString() == pMsg->asString());
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

void CHXPipelinedDescribeLogic::transitionToIdle()
{
    m_state = pdsIdle;
    m_uDescSeqNo = 0;
    HX_DELETE(m_pCachedDescResp);
}

HXBOOL CHXPipelinedDescribeLogic::PipelineDescribe()
{
    HXBOOL bRet = FALSE;

    if (m_pContext)
    {
        IHXPreferences* pPrefs = NULL;
        
        if (HXR_OK == m_pContext->QueryInterface(IID_IHXPreferences,
                                                 (void**)&pPrefs))
        {
            ReadPrefBOOL(pPrefs, "PipelineDESCRIBE", bRet);
        }

        HX_RELEASE(pPrefs);
    }
    return bRet;
}
