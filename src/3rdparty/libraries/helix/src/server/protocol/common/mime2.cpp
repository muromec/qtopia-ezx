/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mime2.cpp,v 1.8 2006/10/17 18:29:40 jzeng Exp $ 
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
/*
 *  XXXTDM: we need a mime_scan() function that takes a buffer and scans
 *  it for designated chars (eg. "=;" for params).  It should return the
 *  number of bytes used and the actual end of the token.
 *
 *  XXXTDM: It also wouldn't hurt to have a whitespace trim function.
 */

#include "hxtypes.h"

//XXXTDM: it would be nice if this was in hxtypes.h
#if !defined(_VXWORKS)
#ifdef _UNIX
#include <ctype.h>
#endif
#endif

#include <stdlib.h>
#include <stdio.h>

#include "hxcom.h"

#include "ihxpckts.h"
#include "hxengin.h"

#include "ihxlist.h"
#include "hxrtsp2.h"
#include "mime2.h"

#include "chxpckts.h"
#include "hxbuffer.h"
#include "hxsbuffer.h"
#include "hxlistp.h"

#ifdef _WINCE
#include <wincestr.h>
#endif

//XXX: move the mime_* functions to a shared spot
typedef const char* CPCHAR;
typedef const unsigned char* CPBYTE;

#define MIME_RES_OK     1
#define MIME_RES_FAIL   2

static int
mime_parse_header(IHXBuffer* pbufPacket, UINT32 pos, UINT32 linelen, 
    IHXBuffer** ppbufKey, IHXBuffer** ppbufVal, 
    BOOL bReplaceDelimiters, int nReplacementDelimiter,
    IHXFastAlloc* pFastAlloc)
{
    CPCHAR pbuf;
    CPCHAR pcur;

    pbuf = (CPCHAR)pbufPacket->GetBuffer() + pos;

    // Trim trailing WS
    BOOL bIsSpace = TRUE;
    while (linelen > 1 && bIsSpace)
    {
        if (pbuf[linelen-1] == '\r' || pbuf[linelen-1] == '\n')
        {
            if (bReplaceDelimiters)
            {
                char* foo = (char *)pbuf+linelen-1;
                *foo = nReplacementDelimiter;
            }
            linelen--;
        }
        else if (isspace(pbuf[linelen-1]))
            linelen--;
        else
            bIsSpace = FALSE;
    }

    pcur = (CPCHAR)memchr(pbuf, ':', linelen);
    if (pcur == NULL)
    {
        return MIME_RES_FAIL;
    }
    if (bReplaceDelimiters)
    {
        char* foo = (char *)pcur;
        *foo = nReplacementDelimiter;
    }

    *ppbufKey = new CHXStaticBuffer(pbufPacket, 0, (pcur-pbuf));
    (*ppbufKey)->AddRef();

    // Skip colon and space/tab(s)
    pcur++;
    while ((*pcur == ' ') || (*pcur == '\t')) pcur++;

    int ulValLen = 0;
    if (linelen >= pcur - pbuf)
        ulValLen = linelen - (pcur - pbuf);

    *ppbufVal = new CHXStaticBuffer(pbufPacket, (pcur-pbuf),
                                                 ulValLen);

    (*ppbufVal)->AddRef();

    return MIME_RES_OK;
}

/*****************************************************************************
 *
 * CMIMEParameter
 *
 *****************************************************************************/

CMIMEParameter::CMIMEParameter(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0),
    m_pBufParam(NULL),
    m_pBufAttr(NULL),
    m_pBufVal(NULL),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CMIMEParameter::~CMIMEParameter(void)
{
    HX_RELEASE(m_pBufVal);
    HX_RELEASE(m_pBufAttr);
    HX_RELEASE(m_pBufParam);
    HX_RELEASE(m_pFastAlloc);
}

void
CMIMEParameter::Init(IHXBuffer* pbufParam)
{
    HX_ASSERT(pbufParam != NULL);
    HX_ASSERT(m_pBufParam == NULL);
    pbufParam->AddRef();
    m_pBufParam = pbufParam;
}

void
CMIMEParameter::Parse(void)
{
    UINT32 pos = 0;
    UINT32 len = m_pBufParam->GetSize();
    CPCHAR pbuf = (CPCHAR)m_pBufParam->GetBuffer();
    CPCHAR pcur = pbuf;

    CPCHAR pattrbegin;
    CPCHAR pattrend;
    CPCHAR pvalbegin;
    CPCHAR pvalend;

    // Trim leading WS
    while (pos < len && isspace(*pcur))
    {
        pcur++;
        pos++;
    }
    pattrbegin = pcur;

    // Find separator
    BOOL bInQuote = FALSE;
    while (pos < len && ((*pcur != '=' && *pcur != ';') || bInQuote))
    {
        if (*pcur == '"')
        {
            bInQuote = !bInQuote;
        }
        pcur++;
        pos++;
    }
    pattrend = pcur;
    pcur++;
    pos++;

    // Trim trailing WS
    while (pattrend > pattrbegin && isspace(*(pattrend-1)))
    {
        pattrend--;
    }

    pvalbegin = NULL;
    pvalend = NULL;

    // See if it's an attr=val
    if (pos < len && *(pcur-1) == '=')
    {
        while (pos < len && isspace(*pcur))
        {
            pcur++;
            pos++;
        }
        pvalbegin = pcur;

        while (pos < len && (*pcur != ';' || bInQuote))
        {
            if (*pcur == '"')
            {
                bInQuote = !bInQuote;
            }
            pcur++;
            pos++;
        }
        pvalend = pcur;
        pcur++;
        pos++;

        // Trim trailing WS
        while (pvalend > pvalbegin && isspace(*(pvalend-1)))
        {
            pvalend--;
        }
    }

    m_pBufAttr = new CHXStaticBuffer(m_pBufParam, 
        (pattrbegin-pbuf), (pattrend-pattrbegin));
    m_pBufAttr->AddRef();
    if (pvalbegin != NULL)
    {
        m_pBufVal = new CHXStaticBuffer(m_pBufParam, 
            (pvalbegin-pbuf), (pvalend-pvalbegin));
        m_pBufVal->AddRef();
    }

}

/*** IUnknown methods ***/

STDMETHODIMP
CMIMEParameter::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXMIMEParameter))
    {
        AddRef();
        *ppvObj = (IHXMIMEParameter*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CMIMEParameter::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CMIMEParameter::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXMIMEParameter methods ***/

STDMETHODIMP_(UINT32)
CMIMEParameter::GetSize(void)
{
    UINT32 len;

    if (m_pBufParam != NULL)
    {
        return m_pBufParam->GetSize();
    }
    HX_ASSERT(m_pBufAttr != NULL && m_pBufVal != NULL);

    // attr=val
    len = m_pBufAttr->GetSize() + 1 + m_pBufVal->GetSize();

    return len;
}

STDMETHODIMP_(UINT32)
CMIMEParameter::Write(BYTE* pbuf)
{
    BYTE*  p;

    if (m_pBufParam != NULL)
    {
        memcpy(pbuf, m_pBufParam->GetBuffer(), m_pBufParam->GetSize());
        return m_pBufParam->GetSize();
    }

    p = pbuf;
    memcpy(p, m_pBufAttr->GetBuffer(), m_pBufAttr->GetSize());
    p += m_pBufAttr->GetSize();
    *p++ = '=';
    memcpy(p, m_pBufVal->GetBuffer(), m_pBufVal->GetSize());
    p += m_pBufVal->GetSize();

    return (p-pbuf);
}

STDMETHODIMP
CMIMEParameter::Get(REF(IHXBuffer*) pbufAttr, REF(IHXBuffer*) pbufVal)
{
    if (m_pBufAttr == NULL)
    {
        Parse();
    }

    if (m_pBufAttr != NULL)
    {
        m_pBufAttr->AddRef();
    }
    pbufAttr = m_pBufAttr;
    if (m_pBufVal != NULL)
    {
        m_pBufVal->AddRef();
    }
    pbufVal = m_pBufVal;

    return HXR_OK;
}

STDMETHODIMP
CMIMEParameter::Set(IHXBuffer* pbufAttr, IHXBuffer* pbufVal)
{
    HX_RELEASE(m_pBufParam);

    HX_RELEASE(m_pBufAttr);
    if (pbufAttr != NULL)
    {
        pbufAttr->AddRef();
    }
    m_pBufAttr = pbufAttr;
    HX_RELEASE(m_pBufVal);
    if (pbufVal != NULL)
    {
        pbufVal->AddRef();
    }
    m_pBufVal = pbufVal;

    return HXR_OK;
}

/*****************************************************************************
 *
 * CMIMEField
 *
 *****************************************************************************/

CMIMEField::CMIMEField(IHXFastAlloc* pFastAlloc) :
      m_ulRefCount(0)
    , m_pBufField(NULL)
    , m_pListParams(NULL)
    , m_ulParamCount(0)
    , m_pParam(NULL)
    , m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CMIMEField::~CMIMEField(void)
{
    if (m_pListParams)
    {
        int i = 0;
        IHXListIterator* pIterParams = 0;
        pIterParams = m_pListParams->Begin();
        while (pIterParams->HasItem())
        {
            IUnknown* pUnkParam = m_pListParams->Remove(pIterParams);
            HX_RELEASE(pUnkParam);
            if (!pIterParams->HasItem())
                break;
            pIterParams->MoveNext();
        }
        HX_RELEASE(pIterParams);
        HX_RELEASE(m_pListParams);
    }

    HX_RELEASE(m_pBufField);
    HX_RELEASE(m_pParam);
    HX_RELEASE(m_pFastAlloc);
}

void
CMIMEField::Init(IHXBuffer* pbufField)
{
    HX_ASSERT(pbufField != NULL);
    HX_ASSERT(m_pBufField == NULL);
    pbufField->AddRef();
    m_pBufField = pbufField;
}

void
CMIMEField::Parse(void)
{
    UINT32 pos = 0;
    UINT32 len = m_pBufField->GetSize();
    CPCHAR pbuf = (CPCHAR)m_pBufField->GetBuffer();
    CPCHAR pcur = pbuf;

    HX_ASSERT(m_pListParams == NULL);
    m_pListParams = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
    m_pListParams->AddRef();

    while (pos < len)
    {
        CPCHAR pbegin;
        CPCHAR pend;

        // Trim leading WS
        while (pos < len && isspace(*pcur))
        {
            pcur++;
            pos++;
        }
        pbegin = pcur;

        // Find separator
        BOOL bInQuote = FALSE;
        while (pos < len && (*pcur != ';' || bInQuote))
        {
            if (*pcur == '"')
            {
                bInQuote = !bInQuote;
            }
            pcur++;
            pos++;
        }
        pend = pcur;
        pcur++;
        pos++;

        // Trim trailing WS
        while (pend > pbegin && isspace(*(pend-1)))
        {
            pend--;
        }

        if (pend > pbegin)
        {
            CHXStaticBuffer* pbufParam =
                new CHXStaticBuffer(m_pBufField, 
                    (pbegin-pbuf), (pend-pbegin));
            CMIMEParameter* pParam =
                new (m_pFastAlloc) CMIMEParameter(m_pFastAlloc);
            IUnknown* punkParam = NULL;
            pParam->Init(pbufParam);
            pParam->QueryInterface(IID_IUnknown, (void**)&punkParam);
            m_pListParams->InsertTail(punkParam);
            punkParam->Release();
            if (m_ulParamCount == 0)
            {
                HX_RELEASE(m_pParam);
                pParam->AddRef();
                m_pParam = (IHXMIMEParameter *)pParam;
                m_ulParamCount++;
            }
        }
    }

}

/*** IUnknown methods ***/

STDMETHODIMP
CMIMEField::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXMIMEField))
    {
        AddRef();
        *ppvObj = (IHXMIMEField*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CMIMEField::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CMIMEField::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXMIMEField methods ***/

STDMETHODIMP_(UINT32)
CMIMEField::GetSize(void)
{
    UINT32 len=0;
    IHXListIterator* pIter;
    IUnknown* punkParam;
    IHXMIMEParameter* pParam;

    if (m_pBufField != NULL)
    {
        return m_pBufField->GetSize();
    }
    HX_ASSERT(m_pListParams != NULL && m_pListParams->GetCount() >= 1);

    pIter = m_pListParams->Begin();
    while (pIter->HasItem())
    {
        punkParam = pIter->GetItem();
        punkParam->QueryInterface(IID_IHXMIMEParameter, (void**)&pParam);
        len += pParam->GetSize();
        pParam->Release();
        punkParam->Release();
        pIter->MoveNext();
    }
    pIter->Release();

    len += m_pListParams->GetCount() - 1; // ';' between params

    return len;
}

STDMETHODIMP_(UINT32)
CMIMEField::Write(BYTE* pbuf)
{
    BYTE*  p;
    IHXListIterator* pIter;
    IUnknown* punkParam;
    IHXMIMEParameter* pParam;

    if (m_pBufField != NULL)
    {
        memcpy(pbuf, m_pBufField->GetBuffer(), m_pBufField->GetSize());
        return m_pBufField->GetSize();
    }
    HX_ASSERT(m_pListParams != NULL && m_pListParams->GetCount() >= 1);

    p = pbuf;

    BOOL bFirst = TRUE;
    pIter = m_pListParams->Begin();
    while (pIter->HasItem())
    {
        punkParam = pIter->GetItem();
        punkParam->QueryInterface(IID_IHXMIMEParameter, (void**)&pParam);
        bFirst ? bFirst = FALSE : *p++ = ';';
        p += pParam->Write(p);
        pParam->Release();
        punkParam->Release();
        pIter->MoveNext();
    }
    pIter->Release();

    return (p-pbuf);
}

STDMETHODIMP
CMIMEField::GetFirstParam(REF(IHXMIMEParameter *)pParam)
{
    if (m_pListParams == NULL)
    {
        Parse();
    }
    HX_ASSERT(m_pListParams != NULL);
    if (m_pParam)
    {
        m_pParam->AddRef();
        pParam = m_pParam;
        return HXR_OK;
    }
    pParam = 0;
    return HXR_FAIL;
}

STDMETHODIMP
CMIMEField::GetParamList(REF(IHXList*) plistParams)
{
    if (m_pListParams == NULL)
    {
        Parse();
    }
    HX_ASSERT(m_pListParams != NULL);

    HX_RELEASE(m_pBufField);
    m_pListParams->AddRef();
    plistParams = m_pListParams;

    return HXR_OK;
}

STDMETHODIMP
CMIMEField::GetParamListConst(REF(IHXList*) plistParams)
{
    if (m_pListParams == NULL)
    {
        Parse();
    }
    HX_ASSERT(m_pListParams != NULL);

    plistParams = m_pListParams->AsConst();

    return HXR_OK;
}

/*****************************************************************************
 *
 * CMIMEHeader
 *
 *****************************************************************************/

CMIMEHeader::CMIMEHeader(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0)
    , m_pBufHeader(NULL)
    , m_pBufKey(NULL)
    , m_pBufVal(NULL)
    , m_pListHeaders(NULL)
    , m_pListFields(NULL)
    , m_ulFieldCount(0)
    , m_pField(NULL)
    , m_bReplaceDelimiters(TRUE)
    , m_nReplacementDelimiter(0)
    , m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CMIMEHeader::~CMIMEHeader(void)
{
    if (m_pListFields)
    {
        int i = 0;
        IHXListIterator* pIterFields = 0;
        pIterFields = m_pListFields->Begin();
        while (pIterFields->HasItem())
        {
            IUnknown* pUnkField = m_pListFields->Remove(pIterFields);
            HX_RELEASE(pUnkField);
            if (!pIterFields->HasItem())
                break;
            pIterFields->MoveNext();
        }
        HX_RELEASE(pIterFields);
    }
    HX_RELEASE(m_pListFields);

    HX_RELEASE(m_pListHeaders);
    HX_RELEASE(m_pBufVal);
    HX_RELEASE(m_pBufKey);
    HX_RELEASE(m_pBufHeader);
    HX_RELEASE(m_pField);
    HX_RELEASE(m_pFastAlloc);
}

HX_RESULT
CMIMEHeader::Init(IHXBuffer* pbufHeader)
{
    HX_ASSERT(pbufHeader != NULL);
    HX_ASSERT(m_pBufHeader == NULL);

    // First physical line must contain a ':'
    BYTE* pBuf = pbufHeader->GetBuffer();
    UINT32 nLen = pbufHeader->GetSize();
    HX_RESULT hxr = HXR_FAIL;
    UINT32 n;
    for (n = 0; n < nLen; n++)
    {
        if (pBuf[n] == ':')
        {
            hxr = HXR_OK;
            break;
        }
        if (pBuf[n] == '\n')
        {
            break;
        }
    }
    if (hxr == HXR_OK)
    {
        pbufHeader->AddRef();
        m_pBufHeader = pbufHeader;
    }

    return hxr;
}

HX_RESULT
CMIMEHeader::Init(IHXList* pHeaders)
{
    // Get the key from the first one.
    IHXMIMEHeader* pMimeHead = NULL;
    IUnknown* pUnk = pHeaders->GetHead();
    HX_RESULT res = pUnk->QueryInterface(IID_IHXMIMEHeader, 
                    (void**)&pMimeHead);
    if (SUCCEEDED(res) && m_pBufKey == NULL)
    {
        res = pMimeHead->GetKey(m_pBufKey);
    }

    if (SUCCEEDED(res) && m_pBufVal == NULL)
    {
        res = pMimeHead->GetValueAsBuffer(m_pBufVal);
    }

    if (SUCCEEDED(res) && m_pListHeaders == NULL)
    {
        m_pListHeaders = pHeaders;
        m_pListHeaders->AddRef();
    }

    HX_RELEASE(pUnk);
    HX_RELEASE(pMimeHead);

    return res;
}

void
CMIMEHeader::ParseHeader(void)
{        
    mime_parse_header(m_pBufHeader, 0, m_pBufHeader->GetSize(), 
        &m_pBufKey, &m_pBufVal, m_bReplaceDelimiters, m_nReplacementDelimiter,
        m_pFastAlloc);
}

void
CMIMEHeader::ParseFields(void)
{
    if (m_pListHeaders)
    {
        ParseHeaderList(m_pListHeaders);
    }
    else if (m_pBufVal)
    {
        ParseFields(m_pBufVal);
    }
}

void
CMIMEHeader::ParseFields(IHXBuffer* pBufVal)
{
    UINT32 pos = 0;
    UINT32 len = pBufVal->GetSize();
    CPCHAR pbuf = (CPCHAR)pBufVal->GetBuffer();
    CPCHAR pcur = pbuf;

    if (!m_pListFields)
    {
        m_pListFields = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_pListFields->AddRef();
    }

    while (pos < len)
    {
        CPCHAR pbegin;
        CPCHAR pend;

        // Trim leading WS
        while (pos < len && isspace(*pcur))
        {
            pcur++;
            pos++;
        }
        pbegin = pcur;

        // Find separator
        BOOL bInQuote = FALSE;
        while (pos < len && (*pcur != ',' || bInQuote))
        {
            if (*pcur == '"')
            {
                bInQuote = !bInQuote;
            }
            pcur++;
            pos++;
        }
        pend = pcur;
        pcur++;
        pos++;

        // Trim trailing WS
        while (pend > pbegin && isspace(*(pend-1)))
        {
            pend--;
        }

        CHXStaticBuffer* pbufField 
            = new CHXStaticBuffer(pBufVal, (pbegin-pbuf),
                                                 (pend-pbegin));
        CMIMEField* pField = new (m_pFastAlloc) CMIMEField(m_pFastAlloc);
        IUnknown* punkField = NULL;
        pField->Init(pbufField);
        pField->QueryInterface(IID_IUnknown, (void**)&punkField);
        m_pListFields->InsertTail(punkField);
        punkField->Release();
        if (m_ulFieldCount == 0)
        {
            HX_RELEASE(m_pField);
            pField->AddRef();
            m_pField = (IHXMIMEField *)pField;
            m_ulFieldCount++;
        }
    }
}

void
CMIMEHeader::ParseHeaderList(IHXList* pHeaders)
{
    IHXListIterator* pIter = pHeaders->Begin();
    IUnknown* pUnk = NULL;
    IHXMIMEHeader* pMimeHead = NULL;
    IHXBuffer* pHeadBuf = NULL;
    while(pIter->HasItem())
    {
        pUnk = pIter->GetItem();
        if (SUCCEEDED(pUnk->QueryInterface(IID_IHXMIMEHeader, 
            (void**)&pMimeHead)) && 
            SUCCEEDED(pMimeHead->GetValueAsBuffer(pHeadBuf)))
        {
            ParseFields(pHeadBuf);
            HX_RELEASE(pHeadBuf);
        }
        HX_RELEASE(pMimeHead);
        HX_RELEASE(pUnk);

        pIter->MoveNext();
    }
    HX_RELEASE(pIter);
}

/*** IUnknown methods ***/

STDMETHODIMP
CMIMEHeader::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXMIMEHeader))
    {
        AddRef();
        *ppvObj = (IHXMIMEHeader*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CMIMEHeader::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CMIMEHeader::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXMIMEHeader methods ***/

void
CMIMEHeader::ReplaceDelimiters(BOOL bReplaceDelimiters,
    int nReplacementDelimiter)
{
    m_bReplaceDelimiters = bReplaceDelimiters;
    m_nReplacementDelimiter = nReplacementDelimiter;
}

STDMETHODIMP_(UINT32)
CMIMEHeader::GetSize(void)
{
    UINT32 ulLen = 0;
    IHXListIterator* pIter;
    IUnknown* punkItem;
    IHXMIMEField* pField = NULL;
    IHXMIMEHeader* pHeader = NULL;

    if (m_pBufHeader != NULL)
    {
    	return m_pBufHeader->GetSize();
    }
    HX_ASSERT(m_pBufKey != NULL);

    // If we have a list of full headers, add GetSize on those 
    // headers. Will include the key, CRLF, etc. for each line.
    if (m_pListHeaders)
    {
        pIter = m_pListHeaders->Begin();
        while (pIter->HasItem())
        {
            punkItem = pIter->GetItem();
            if (punkItem && 
                SUCCEEDED(punkItem->QueryInterface(IID_IHXMIMEHeader, 
                (void**)&pHeader)))
            {
                ulLen += pHeader->GetSize();
                pHeader->Release();
                punkItem->Release();
            }
            pIter->MoveNext();
        }
        pIter->Release();
    }

    else
    {
        ulLen = m_pBufKey->GetSize();
        ulLen += 2; // ": "

        if (m_pBufVal != NULL)
        {
            ulLen += m_pBufVal->GetSize();
        }
        else
        {
            HX_ASSERT(m_pListFields != NULL && m_pListFields->GetCount() >= 1);
            pIter = m_pListFields->Begin();
            while (pIter->HasItem())
            {
                punkItem = pIter->GetItem();
                if (punkItem && 
                    SUCCEEDED(punkItem->QueryInterface(IID_IHXMIMEField, 
                    (void**)&pField)))
                {
                    ulLen += pField->GetSize();
                    pField->Release();
                    punkItem->Release();
                }
                pIter->MoveNext();
            }
            pIter->Release();
            ulLen += m_pListFields->GetCount() - 1; // ',' between fields
        }

        ulLen += 2; // CRLF
    }

    return ulLen;
}

STDMETHODIMP_(UINT32)
CMIMEHeader::Write(BYTE* pbuf)
{
    BYTE*  p;
    IHXListIterator* pIter;
    IUnknown* punkItem;
    IHXMIMEHeader* pHeader;
    IHXMIMEField* pField;
    UINT32 ulLen = 0;

    if (m_pBufHeader != NULL)
    {
        memcpy(pbuf, m_pBufHeader->GetBuffer(), m_pBufHeader->GetSize());

        if (m_bReplaceDelimiters)
        {
            char* hdr = (char *)m_pBufHeader->GetBuffer();
            UINT32 len = m_pBufHeader->GetSize();

            char* ptr = (char *)memchr(hdr, m_nReplacementDelimiter, len);
            if (ptr)
            {
                *(pbuf + (ptr - hdr)) = ':';
            }
            if (*(pbuf+len-1) == m_nReplacementDelimiter)
                *(pbuf+len-1) = '\n';
            if (*(pbuf+len-2) == m_nReplacementDelimiter)
                *(pbuf+len-2) = '\r';
        }

        return m_pBufHeader->GetSize();
    }

    // If we have a headers list, just write them all 
    // consecutively
    else if (m_pListHeaders)
    {
        pIter = m_pListHeaders->Begin();
        while (pIter->HasItem())
        {
            punkItem = pIter->GetItem();
            if (punkItem && 
                SUCCEEDED(punkItem->QueryInterface(IID_IHXMIMEHeader, 
                (void**)&pHeader)))
            {
                ulLen += pHeader->Write(pbuf + ulLen);
                pHeader->Release();
            }
            HX_RELEASE(punkItem);
            pIter->MoveNext();
        }
        pIter->Release();

        return ulLen;
    }

    HX_ASSERT(m_pBufKey != NULL);

    p = pbuf;
    memcpy(p, m_pBufKey->GetBuffer(), m_pBufKey->GetSize());
    p += m_pBufKey->GetSize();
    *p++ = ':';
    *p++ = ' ';

    if (m_pBufVal != NULL)
    {
        memcpy(p, m_pBufVal->GetBuffer(), m_pBufVal->GetSize());
        p += m_pBufVal->GetSize();
    }
    else
    {
        BOOL bFirst = TRUE;
        HX_ASSERT(m_pListFields != NULL && m_pListFields->GetCount() >= 1);
        pIter = m_pListFields->Begin();
        while (pIter->HasItem())
        {
            punkItem = pIter->GetItem();
            if (punkItem && 
                SUCCEEDED(punkItem->QueryInterface(IID_IHXMIMEField, 
                (void**)&pField)))
            {
                bFirst ? bFirst = FALSE : *p++ = ',';
                p += pField->Write(p);
                pField->Release();
            }
            HX_RELEASE(punkItem);
            pIter->MoveNext();
        }
        pIter->Release();
    }

    *p++ = '\r';
    *p++ = '\n';

    return (p-pbuf);
}

STDMETHODIMP
CMIMEHeader::GetKey(REF(IHXBuffer*) pbufKey)
{
    if (m_pBufKey == NULL)
    {
        // See if we need to parse the header
        if (m_pBufHeader != NULL)
        {
            ParseHeader();
        }
        if (m_pBufKey == NULL)
        {
            pbufKey = NULL;
            return HXR_FAIL;
        }
    }

    m_pBufKey->AddRef();
    pbufKey = m_pBufKey;

    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::SetKey(IHXBuffer* pbufKey)
{
    HX_RELEASE(m_pBufHeader);
    HX_RELEASE(m_pBufKey);
    if (pbufKey != NULL)
    {
        pbufKey->AddRef();
        m_pBufKey = pbufKey;
    }

    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::GetFirstField(REF(IHXMIMEField *)pField)
{
    if (m_pListFields == NULL)
    {
        // See if we need to parse the header
        if (m_pBufKey == NULL && m_pBufHeader != NULL)
        {
            ParseHeader();
        }
        // See if we need to parse the fields
        if (m_pListFields == NULL)
        {
            ParseFields();
        }
    }

    HX_ASSERT(m_pListFields != NULL);
    if (m_pField)
    {
        m_pField->AddRef();
        pField = m_pField;
        return HXR_OK;
    }
    pField = 0;
    return HXR_FAIL;
}

STDMETHODIMP
CMIMEHeader::GetFieldList(REF(IHXList*) plistFields)
{
    if (m_pListFields == NULL)
    {
        // See if we need to parse the header
        if (m_pBufKey == NULL && m_pBufHeader != NULL)
        {
            ParseHeader();
        }
        // See if we need to parse the fields
        if (m_pListFields == NULL)
        {
           ParseFields();
        }
    }

    HX_ASSERT(m_pListFields != NULL);

    HX_RELEASE(m_pBufHeader);
    m_pListFields->AddRef();
    plistFields = m_pListFields;

    HX_RELEASE(m_pBufVal);

    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::GetFieldListConst(REF(IHXList*) plistFields)
{
    if (m_pListFields == NULL)
    {
        // See if we need to parse the header
        if (m_pBufKey == NULL && m_pBufHeader != NULL)
        {
            ParseHeader();
        }
        // See if we need to parse the fields
        if (m_pListFields == NULL)
        {
            ParseFields();
        }
    }
    HX_ASSERT(m_pListFields != NULL);

    plistFields = m_pListFields->AsConst();

    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::GetValueAsInt(REF(INT32) val)
{
    if (m_pBufVal == NULL)
    {
        // See if we need to parse the header
        if (m_pBufHeader != NULL)
        {
            ParseHeader();
        }
        if (m_pBufVal == NULL)
        {
            return HXR_FAIL;
        }
    }

    INT32 sign = 1;
    UINT32 n = 0;
    UINT32 len = m_pBufVal->GetSize();
    CPCHAR p = (CPCHAR)m_pBufVal->GetBuffer();

    if (len > 0 && *p == '-')
    {
        sign = -1;
        p++;
        n++;
    }

    if (n >= len)
    {
        return HXR_FAIL;
    }

    while (n < len)
    {
        if (!isdigit(*p))
        {
            return HXR_FAIL;
        }

        val = val*10 + (*p - '0');
        p++;
        n++;
    }

    val *= sign;

    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::SetValueFromInt(INT32 val)
{
    char buf[1+10+2+1];
    if (m_pListFields)
    {
        int i = 0;
        IHXListIterator* pIterFields = 0;
        pIterFields = m_pListFields->Begin();
        while (pIterFields->HasItem())
        {
            IUnknown* pUnkField = m_pListFields->Remove(pIterFields);
            HX_RELEASE(pUnkField);
            if (!pIterFields->HasItem())
                break;
            pIterFields->MoveNext();
        }
        HX_RELEASE(pIterFields);
    }
    HX_RELEASE(m_pListFields);
    HX_RELEASE(m_pBufVal);
    HX_RELEASE(m_pBufHeader);
    sprintf(buf, "%d", val);
    return CHXBuffer::FromCharArray(buf, strlen(buf), &m_pBufVal);
}

STDMETHODIMP
CMIMEHeader::GetValueAsUint(REF(UINT32) val)
{
    if (m_pBufVal == NULL)
    {
        // See if we need to parse the header
        if (m_pBufHeader != NULL)
        {
            ParseHeader();
        }
        if (m_pBufVal == NULL)
        {
            return HXR_FAIL;
        }
    }

    UINT32 n = 0;
    UINT32 len = m_pBufVal->GetSize();
    CPCHAR p = (CPCHAR)m_pBufVal->GetBuffer();

    if (n >= len)
    {
        return HXR_FAIL;
    }

    while (n < len)
    {
        if (!isdigit(*p))
        {
            return HXR_FAIL;
        }

        val = val*10 + (*p - '0');
        p++;
        n++;
    }

    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::SetValueFromUint(UINT32 val)
{
    char buf[10+2+1];
    if (m_pListFields)
    {
        int i = 0;
        IHXListIterator* pIterFields = 0;
        pIterFields = m_pListFields->Begin();
        while (pIterFields->HasItem())
        {
            IUnknown* pUnkField = m_pListFields->Remove(pIterFields);
            HX_RELEASE(pUnkField);
            if (!pIterFields->HasItem())
                break;
            pIterFields->MoveNext();
        }
        HX_RELEASE(pIterFields);
    }
    HX_RELEASE(m_pListFields);
    HX_RELEASE(m_pBufVal);
    HX_RELEASE(m_pBufHeader);
    sprintf(buf, "%lu", val);
    return CHXBuffer::FromCharArray(buf, strlen(buf), &m_pBufVal);
}

STDMETHODIMP
CMIMEHeader::GetValueAsBuffer(REF(IHXBuffer*) pbufVal)
{
    if (m_pBufVal == NULL)
    {
        // See if we need to parse the header
        if (m_pBufHeader != NULL)
        {
            ParseHeader();
        }
        if (m_pBufVal == NULL)
        {
            return HXR_FAIL;
        }
    }

    //XXXTDM: handle escaping (eg. escaped quotes?)
    m_pBufVal->AddRef();
    pbufVal = m_pBufVal;

    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::SetValueFromBuffer(IHXBuffer* pbufVal)
{
    if (m_pListFields)
    {
        int i = 0;
        IHXListIterator* pIterFields = 0;
        pIterFields = m_pListFields->Begin();
        while (pIterFields->HasItem())
        {
            IUnknown* pUnkField = m_pListFields->Remove(pIterFields);
            HX_RELEASE(pUnkField);
            if (!pIterFields->HasItem())
                break;
            pIterFields->MoveNext();
        }
        HX_RELEASE(pIterFields);
    }
    HX_RELEASE(m_pListFields);
    HX_RELEASE(m_pBufVal);
    HX_RELEASE(m_pBufHeader);
    if (pbufVal != NULL)
    {
        pbufVal->AddRef();
        m_pBufVal = pbufVal;
    }
    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::SetFromString(const char* szKey, const char* szVal)
{
    char*  p;
    UINT32 keylen;
    UINT32 vallen;

    if (m_pListFields)
    {
        int i = 0;
        IHXListIterator* pIterFields = 0;
        pIterFields = m_pListFields->Begin();
        while (pIterFields->HasItem())
        {
            IUnknown* pUnkField = m_pListFields->Remove(pIterFields);
            HX_RELEASE(pUnkField);
            if (!pIterFields->HasItem())
                break;
            pIterFields->MoveNext();
        }
        HX_RELEASE(pIterFields);
    }
    HX_RELEASE(m_pListFields);
    HX_RELEASE(m_pBufVal);
    HX_RELEASE(m_pBufKey);
    HX_RELEASE(m_pBufHeader);

    m_pBufHeader = new CHXBuffer;
    m_pBufHeader->AddRef();
    keylen = strlen(szKey);
    vallen = strlen(szVal);
    m_pBufHeader->SetSize(keylen + 2 + vallen + 2);
    p = (char*)m_pBufHeader->GetBuffer();
    memcpy(p, szKey, keylen);
    p += keylen;
    *p++ = ':';
    *p++ = ' ';
    memcpy(p, szVal, vallen);
    p += vallen;
    *p++ = '\r';
    *p++ = '\n';

    return HXR_OK;
}

STDMETHODIMP
CMIMEHeader::SetFromBuffer(const char* szKey, IHXBuffer* pbufVal)
{
    char*  p;
    UINT32 keylen;
    UINT32 vallen;

    if (m_pListFields)
    {
        int i = 0;
        IHXListIterator* pIterFields = 0;
        pIterFields = m_pListFields->Begin();
        while (pIterFields->HasItem())
        {
            IUnknown* pUnkField = m_pListFields->Remove(pIterFields);
            HX_RELEASE(pUnkField);
            if (!pIterFields->HasItem())
                break;
            pIterFields->MoveNext();
        }
        HX_RELEASE(pIterFields);
    }
    HX_RELEASE(m_pListFields);
    HX_RELEASE(m_pBufVal);
    HX_RELEASE(m_pBufKey);
    HX_RELEASE(m_pBufHeader);

    m_pBufHeader = new CHXBuffer;
    m_pBufHeader->AddRef();
    keylen = strlen(szKey);
    vallen = pbufVal->GetSize();
    m_pBufHeader->SetSize(keylen + 2 + vallen + 2);
    p = (char*)m_pBufHeader->GetBuffer();
    memcpy(p, szKey, keylen);
    p += keylen;
    *p++ = ':';
    *p++ = ' ';
    memcpy(p, pbufVal->GetBuffer(), vallen);
    p += vallen;
    *p++ = '\r';
    *p++ = '\n';

    return HXR_OK;
}
