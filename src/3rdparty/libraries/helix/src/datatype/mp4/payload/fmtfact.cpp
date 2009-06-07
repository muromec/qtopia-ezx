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

#include "fmtfact.h"

PayloadFormatFactory::PayloadFormatFactory()
{}

PayloadFormatFactory::~PayloadFormatFactory()
{}

HX_RESULT PayloadFormatFactory::BuildFormat(IUnknown* pContext, 
					    HXBOOL bPacketize,
					    IHXValues* pHeader,
					    REF(IMP4APayloadFormat*) pFmt)
{
    HX_RESULT res = HXR_FAILED;
    CHXSimpleList::Iterator itr = m_builders.Begin();

    pFmt = 0;

    HXBOOL bRequestUpgrade = FALSE;

    for(;(itr != m_builders.End()) && !SUCCEEDED(res); ++itr)
    {
	IMP4APayloadFormat* pTmp = 0;

	if (SUCCEEDED(((FormatBuildFunc)(*itr))(pTmp)))
        {
            HX_RESULT retVal = pTmp->Init(pContext, bPacketize);
            if (SUCCEEDED(retVal))
            {
                retVal = pTmp->SetStreamHeader(pHeader);
                if (SUCCEEDED(retVal))
                {
	            pFmt = pTmp;
	            pTmp->AddRef();

	            res = HXR_OK;
                }
                else if (retVal == HXR_REQUEST_UPGRADE)
                {
                    bRequestUpgrade = TRUE;
                }
            }
	}
	HX_RELEASE(pTmp);
    }

    // We we still have a failure code here and bRequestUpgrade
    // is TRUE, then one of the depacketizers loaded and init'd,
    // but returned HXR_REQUEST_UPGRADE on SetStreamHeader(). This
    // indicates that the depacketizer could handle the stream
    // mime type, but there was something about the version it 
    // couldn't handle. So we need to return HXR_REQUEST_UPGRADE
    // back to the CAudioFormat, so that it will return HXR_REQUEST_UPGRADE
    // to the CAudioRenderer, which will cause an AU.
    if (FAILED(res) && bRequestUpgrade)
    {
        res = HXR_REQUEST_UPGRADE;
    }

    return res;
}

void PayloadFormatFactory::RegisterBuilder(FormatBuildFunc pBuildFunc)
{
    m_builders.AddTail((void*)pBuildFunc);
}

VideoPayloadFormatFactory::VideoPayloadFormatFactory()
{}

VideoPayloadFormatFactory::~VideoPayloadFormatFactory()
{}

HX_RESULT VideoPayloadFormatFactory::BuildFormat(IUnknown* pContext, 
					    HXBOOL bPacketize,
					    IHXValues* pHeader,
					    REF(IMP4VPayloadFormat*) pFmt)
{
    HX_RESULT res = HXR_FAILED;
    CHXSimpleList::Iterator itr = m_builders.Begin();

    pFmt = 0;

    HXBOOL bRequestUpgrade = FALSE;

    for(;(itr != m_builders.End()) && !SUCCEEDED(res); ++itr)
    {
	IMP4VPayloadFormat* pTmp = 0;

	if (SUCCEEDED(((VideoFormatBuildFunc)(*itr))(pTmp)))
        {
            HX_RESULT retVal = pTmp->Init(pContext, bPacketize);
            if (SUCCEEDED(retVal))
            {
                retVal = pTmp->SetStreamHeader(pHeader);
                if (SUCCEEDED(retVal))
                {
	            pFmt = pTmp;
	            pTmp->AddRef();

	            res = HXR_OK;
                }
                else if (retVal == HXR_REQUEST_UPGRADE)
                {
                    bRequestUpgrade = TRUE;
                }
            }
	}
	HX_RELEASE(pTmp);
    }

    // We we still have a failure code here and bRequestUpgrade
    // is TRUE, then one of the depacketizers loaded and init'd,
    // but returned HXR_REQUEST_UPGRADE on SetStreamHeader(). This
    // indicates that the depacketizer could handle the stream
    // mime type, but there was something about the version it 
    // couldn't handle. So we need to return HXR_REQUEST_UPGRADE
    // back to the CAudioFormat, so that it will return HXR_REQUEST_UPGRADE
    // to the CAudioRenderer, which will cause an AU.
    if (FAILED(res) && bRequestUpgrade)
    {
        res = HXR_REQUEST_UPGRADE;
    }

    return res;
}

void VideoPayloadFormatFactory::RegisterBuilder(VideoFormatBuildFunc pBuildFunc)
{
    m_builders.AddTail((void*)pBuildFunc);
}
