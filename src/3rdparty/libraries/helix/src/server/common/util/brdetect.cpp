/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: brdetect.cpp,v 1.2 2005/11/01 20:05:00 seansmith Exp $ 
 *   
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.  
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

#include "brdetect.h"
#include "ihxpckts.h"
#include "hxassert.h"
#include "hxstrutl.h"

#define MAX_STREAM_HEADERS 32

BRDetector::BRDetector() :
    m_ppStreamHeaders(NULL),
    m_ulStreamCount(0)
{
}

BRDetector::~BRDetector()
{
    if (m_ppStreamHeaders)
    {
        for (UINT32 i = 0; i < m_ulStreamCount; i++)
        {
            HX_RELEASE(m_ppStreamHeaders[i]);
        }
    }

    HX_VECTOR_DELETE(m_ppStreamHeaders);
}

HX_RESULT
BRDetector::SetStreamCount(UINT32 ulStreamCount)
{
    HX_ASSERT(m_ulStreamCount == 0);
    if (m_ulStreamCount > 0)
    {
        return HXR_FAIL;
    }

    if (ulStreamCount < 1 
        || ulStreamCount > MAX_STREAM_HEADERS)
    {
        return HXR_FAIL;
    }
    
    m_ulStreamCount = ulStreamCount;

    m_ppStreamHeaders = new IHXValues*[m_ulStreamCount];
    memset(m_ppStreamHeaders, 0, sizeof(IHXValues*)*m_ulStreamCount);

    return HXR_OK;
}

HX_RESULT
BRDetector::OnStreamHeader(IHXValues* pValues)
{
    HX_ASSERT(pValues && m_ppStreamHeaders);
    if (!m_ppStreamHeaders
        || !pValues 
        || m_ulStreamCount < 1)
    {
        return HXR_FAIL;
    }
    
    UINT32 ulStreamNum = 0;
    if (FAILED(pValues->GetPropertyULONG32("StreamNumber", ulStreamNum)))
    {
        ulStreamNum = 0;
    }
    else if (ulStreamNum >= m_ulStreamCount)
    {
        return HXR_FAIL;
    }

    HX_ASSERT(m_ppStreamHeaders[ulStreamNum] == NULL);
    if (m_ppStreamHeaders[ulStreamNum])
    {
        return HXR_FAIL;
    }

    pValues->AddRef();
    m_ppStreamHeaders[ulStreamNum] = pValues;

    return HXR_OK;
}

HX_RESULT
BRDetector::RuleIsTSD(IHXBuffer* pRuleBook, UINT32 ulRuleNum, BOOL& bTSD)
{
    const char* pRuleStart; 
    const char* pRuleEnd;
    const char* pTSDRule;
    UINT32 ulCurRule = 0;
    bTSD = FALSE;

    HX_ASSERT(pRuleBook);
    if (!pRuleBook)
    {
        return HXR_FAIL;
    }

    pRuleStart = (char*)pRuleBook->GetBuffer();

    while (*pRuleStart && ulCurRule < ulRuleNum)
    {
        if (*pRuleStart == ';')
        {
            ulCurRule++;
        }
        pRuleStart++;
    }
    if (!*pRuleStart || ulCurRule != ulRuleNum)
    {
        return HXR_FAIL;
    }
    pRuleEnd = pRuleStart;
    while (*pRuleEnd && *pRuleEnd != ';')
    {
        pRuleEnd++;
    }

    // verify that rule exists and ends with semicolon
    HX_ASSERT(pRuleEnd > pRuleStart && *pRuleEnd == ';');
    if (pRuleEnd == pRuleStart || *pRuleEnd != ';')
    {
        return HXR_FAIL;
    }

    pTSDRule = StrStrCaseInsensitive((char*)pRuleStart, "TimeStampDelivery=");
    if (pTSDRule && pTSDRule < pRuleEnd &&
        (pTSDRule[18] == 't' || pTSDRule[18] == 'T'))
    {
        bTSD = TRUE;
    }

    return HXR_OK;
}

HX_RESULT
BRDetector::GetType(UINT32 ulStreamNum, UINT32 ulRuleNum, StreamType& type)
{
    IHXValues* pHdr = NULL;
    IHXBuffer* pMimeType = NULL;
    UINT32 ulABR = 0;
    UINT32 ulMBR = 0;
    IHXBuffer* pRuleBook = NULL;

    if (ulStreamNum >= m_ulStreamCount)
    {
        return HXR_FAIL;
    }
    HX_ASSERT(m_ppStreamHeaders);
    if (!m_ppStreamHeaders)
    {
        return HXR_FAIL;
    }
    if (!m_ppStreamHeaders[ulStreamNum])
    {
        return HXR_FAIL;
    }
    pHdr = m_ppStreamHeaders[ulStreamNum];

    // we have enough data to continue!
    if (FAILED(pHdr->GetPropertyCString("MimeType", pMimeType)))
    {
        return HXR_FAIL;
    }

    // exclude this mime type when identifying TSD/CBR streams
    if (strcmp((const char*)pMimeType->GetBuffer(), "application/x-pn-realevent") == 0)
    {
        HX_RELEASE(pMimeType);
        type = STRM_INVALID;
        return HXR_OK;
    }
    HX_RELEASE(pMimeType);

    // if AvgBitRate and MaxBitRate are in the header, and
    // aren't equal, then this is TSD stream
    if (SUCCEEDED(pHdr->GetPropertyULONG32("AvgBitRate", ulABR))
        && SUCCEEDED(pHdr->GetPropertyULONG32("MaxBitRate", ulMBR)))
    {
        if (ulABR != ulMBR && 
            ulABR > 0 && ulMBR > 0)
        {
            type = STRM_TSD;
            return HXR_OK;
        }
    }

    // if there is no rulebook, then default to TSD
    if (FAILED(pHdr->GetPropertyCString("ASMRuleBook", pRuleBook)))
    {
        type = STRM_TSD;
        return HXR_OK;
    }

    // find out if desired rule is TSD
    BOOL bTSD;
    if (SUCCEEDED(RuleIsTSD(pRuleBook, ulRuleNum, bTSD)))
    {
        type = bTSD ? STRM_TSD : STRM_CBR;
    }
    else
    {
        HX_RELEASE(pRuleBook);
        return HXR_FAIL;
    }
    
    HX_RELEASE(pRuleBook);

    return HXR_OK;
}


