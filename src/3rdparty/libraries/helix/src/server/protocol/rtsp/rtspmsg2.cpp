/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rtspmsg2.cpp,v 1.19 2008/04/10 17:19:56 dcollins Exp $ 
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
 * When we receive data over the control channel, it is usually a complete
 * RTSP message (or more than one, in the case of pipelining).  We take
 * advantage of this fact by just keeping pointers to the headers and
 * content, as appropriate.  This class uses the new CHXStaticBuffer class
 * to provide consistent access to the data regardless of whether the data
 * is originally from the network or has changed since.
 *
 * The pointers to the header keys and values are kept in allocated pointer
 * arrays that start out with a reasonable size.  This further reduces the
 * overhead, as we typically only allocate two blocks for storing our
 * pointers (one for keys, one for values).  If a list were used, we would
 * have two allocations per header.
 *
 * We could store the key and value pointers in a single array by pairs. 
 * That would decrease the readability of the code slightly but it would
 * also be slightly more efficient.
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
#include "rtspmsg2.h"

#include "chxpckts.h"
#include "hxbuffer.h"
#include "hxsbuffer.h"
#include "hxlistp.h"
#include "urlparser.h"

#ifdef _WINCE
#include <wincestr.h>
#endif

// tspecials defined in RFC2326 ABNF
static const char g_tspecials[] = "()<>@,;:\\\"/[]?={} \t";

// RTSP token char
#define IS_TOKEN_CHAR(c) ( ((c)>=' ') && (strchr(g_tspecials,(c))==NULL) )

// RTSP url char: 0x21..0x7e (eg. not control or space or high-bit)
// According to my reading of RFC2326, '<', '>', and '"' (0x3c, 0x3e, 0x22)
// should not be allowed.  However, the player sends them unescaped.
// Note: See CHXURL for the player's encoding/decoding routines.
#define IS_URL_CHAR(c) ( (c)>=0x21 && (c)<=0x7e )

//XXX: move the mime_* functions to a shared spot
typedef const char* CPCHAR;
typedef const unsigned char* CPBYTE;

/*
 * Parse a logical MIME line, which may contain multiple physical lines.
 * Returns:
 *   RTSP_RES_DONE    if line is parsed successfully
 *   RTSP_RES_PARTIAL if line is incomplete
 *   RTSP_RES_INVALID if line is not valid
 */
static int
mime_parse_line(CPCHAR pbuf, UINT32 buflen, UINT32* plinelen)
{
    CPCHAR pcur;        /* current position */
    UINT32 nleft;       /* length of remaining buffer */
    UINT32 physlen;     /* length of current physical line */

    BOOL bFirstLine = TRUE;

    if (buflen == 0)
    {
        return RTSP_RES_PARTIAL;
    }

    pcur = pbuf;
    nleft = buflen;

    /*
     * We get false positives if we check for CR only because a CRLF can
     * be split between packets.  This makes the CRLF appear as two line
     * separators.  Therefore, we only look for LF to end a line and we
     * ignore CR.  This lets us catch either CRLF or LF alone as an EOL.
     *
     * There is a possibility that we can miss a continued line if the
     * line is split between an LF and the succeeding LWS.  Therefore,
     * we don't consider a line to be complete unless either (1) the
     * next character after the LF is available and it's not LWS, or
     * (2) the line is empty, in which case it cannot be continued.
     */
    do
    {
        physlen = 0;

        /* Find LF that terminates line */
        while (*pcur != '\n')
        {
            if (nleft <= 1)
            {
                return RTSP_RES_PARTIAL;
            }

            physlen++;
            pcur++;
            nleft--;
        }

        /* If we saw a CRLF, physlen needs adjusted */
        if (physlen > 0 && *(pcur-1) == '\r')
        {
            physlen--;
        }

        /* Advance past the LF */
        pcur++;
        nleft--;

        /* Check for empty line */
        if (physlen == 0)
        {
            break;
        }

        if (bFirstLine)
        {
            bFirstLine = FALSE;

            /* Make sure we have a colon on the first physical line */
            if (memchr(pbuf, ':', physlen) == NULL)
            {
                return RTSP_RES_INVALID;
            }
        }

        /*
         * Must be able to check for continuation.  Note that this check
         * keeps the while () condition from overrunning the buffer.
         */
        if (nleft == 0)
        {
            return RTSP_RES_PARTIAL;
        }
    }
    while (*pcur == ' ' || *pcur == '\t');

    *plinelen = buflen - nleft;

    return RTSP_RES_DONE;
}

#define HEADER_ARRAY_GROW   8

#define RTSP_VER_1_0 ((1<<8) + (0))

// These correspond with enum RtspVerb and must be sorted
struct rtsp_verb { CPCHAR str; UINT32 len; };
static rtsp_verb s_Verbs[] = {
    { "-NONE-",          6 },
    { "ANNOUNCE",        8 },
    { "DESCRIBE",        8 },
    { "GET_PARAMETER",  13 },
    { "OPTIONS",         7 },
    { "PAUSE",           5 },
    { "PLAY",            4 },
    { "RECORD",          6 },
    { "REDIRECT",        8 },
    { "SETUP",           5 },
    { "SET_PARAMETER",  13 },
    { "TEARDOWN",        8 },
    { NULL,              0 }
};
static const UINT s_nVerbs = sizeof(s_Verbs)/sizeof(rtsp_verb) - 1;

struct StatusMapEntry { UINT nCode; CPCHAR szName; };

// These must be sorted
static StatusMapEntry s_mapStatus[] =
{
    { 100, "Continue" },

    { 200, "OK" },
    { 201, "Created" },
    { 202, "Accepted" },
    { 250, "Low on Storage Space" },

    { 300, "Multiple Choices" },
    { 301, "Moved Permanently" },
    { 302, "Moved Temporarily" },
    { 303, "See Other" },
    { 304, "Not Modified" },
    { 305, "Use Proxy" },

    { 400, "Bad Request" },
    { 401, "Unauthorized" },
    { 402, "Payment Required" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 406, "Not Acceptable" },
    { 407, "Proxy Authentication Required" },
    { 408, "Request Time-out" },
    { 410, "Gone" },
    { 411, "Length Required" },
    { 412, "Precondition Failed" },
    { 413, "Request Entity Too Large" },
    { 414, "Request-URI Too Large" },
    { 415, "Unsupported Media Type" },
    { 451, "Parameter Not Understood" },
    { 452, "Conference Not Found" },
    { 453, "Not Enough Bandwidth" },
    { 454, "Session Not Found" },
    { 455, "Method Not Valid in This State" },
    { 456, "Header Field Not Valid for Resource" },
    { 457, "Invalid Range" },
    { 458, "Parameter Is Read-Only" },
    { 459, "Aggregate operation not allowed" },
    { 460, "Only aggregate operation allowed" },
    { 461, "Unsupported transport" },
    { 462, "Destination unreachable" },

    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 502, "Bad Gateway" },
    { 503, "Service Unavailable" },
    { 504, "Gateway Time-out" },
    { 505, "RTSP Version not supported" },
    { 551, "Option not supported" },
    {   0, NULL }
};
static const UINT s_nStatusEntries = sizeof(s_mapStatus)/sizeof(StatusMapEntry) - 1;

/*****************************************************************************
 *
 * CRTSPMessageBase
 *
 *****************************************************************************/

CRTSPMessageBase::CRTSPMessageBase(IHXFastAlloc* pFastAlloc)
    : m_ulRefCount(0)
    , m_iRecvState(RS_READY)
    , m_ulRecvLeft(0)
    , m_uVer(0x0100)
    , m_nMajorVersion(1)
    , m_nMinorVersion(0)
    , m_plistHeaders(NULL)
    , m_pbufContent(NULL)
    , m_bReplaceDelimiters(FALSE)
    , m_nReplacementDelimiter(0)
    , m_pFastAlloc(pFastAlloc)
    , m_pSessionID(NULL)
    , m_pHostname(NULL)
    , m_uPort(0)
    , m_bFullyQualifiedURL(FALSE)
    , m_bProxyGeneratedRequest(FALSE)
    , m_bIsProxy(FALSE)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CRTSPMessageBase::~CRTSPMessageBase(void)
{
    HX_RELEASE(m_pbufContent);
    HX_RELEASE(m_plistHeaders);
    HX_RELEASE(m_pFastAlloc);
    HX_RELEASE(m_pSessionID);
    HX_RELEASE(m_pHostname);
}

/*** IUnknown methods ***/

STDMETHODIMP
CRTSPMessageBase::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPMessage))
    {
        AddRef();
        *ppvObj = (IHXRTSPMessage*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CRTSPMessageBase::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CRTSPMessageBase::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXRTSPMessage methods ***/

STDMETHODIMP_(UINT16)
CRTSPMessageBase::GetVersion(void)
{
    return m_uVer;
}

STDMETHODIMP
CRTSPMessageBase::SetVersion(UINT16 ver)
{
    m_uVer = ver;
    return HXR_OK;
}

STDMETHODIMP_(int)
CRTSPMessageBase::GetMajorVersion(void)
{
    return m_nMajorVersion;
}

STDMETHODIMP
CRTSPMessageBase::SetMajorVersion(int ver)
{
    m_nMajorVersion = ver;
    return HXR_OK;
}

STDMETHODIMP_(int)
CRTSPMessageBase::GetMinorVersion(void)
{
    return m_nMinorVersion;
}

STDMETHODIMP
CRTSPMessageBase::SetMinorVersion(int ver)
{
    m_nMinorVersion = ver;
    return HXR_OK;
}

STDMETHODIMP_(UINT32)
CRTSPMessageBase::GetCSeq(void)
{
    IHXMIMEHeader* pHdrCSeq = NULL;
    IHXBuffer* pbufVal = NULL;
    UINT32 cseq = 0;

    if (GetHeader("CSeq", pHdrCSeq) == HXR_OK)
    {
        if (pHdrCSeq->GetValueAsBuffer(pbufVal) == HXR_OK)
        {
            CPCHAR p = (CPCHAR)pbufVal->GetBuffer();
            UINT32 n = pbufVal->GetSize();
            while (n > 0)
            {
                if (!isdigit(*p))
                {
                    break;
                }
                cseq = cseq*10 + (*p-'0');
                p++;
                n--;
            }
            pbufVal->Release();
        }
        pHdrCSeq->Release();
    }

    return cseq;
}

STDMETHODIMP
CRTSPMessageBase::SetCSeq(UINT32 ulCSeq)
{
    char szCSeq[4+2+12+2+1]; // "CSeq" ": " <UINT32> CRLF "\0"
    UINT32 nCSeqLen;

    IHXBuffer* pbuf = NULL;
    CMIMEHeader* pHdr = NULL;

    nCSeqLen = sprintf(szCSeq, "CSeq: %lu\r\n", ulCSeq);
    CHXBuffer::FromCharArray(szCSeq, nCSeqLen, &pbuf);
    pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    pHdr->Init(pbuf);
    pbuf->Release();
    if (m_bReplaceDelimiters)
        pHdr->ReplaceDelimiters(m_bReplaceDelimiters, m_nReplacementDelimiter);

    return SetHeader(pHdr);
}

STDMETHODIMP
CRTSPMessageBase::GetContent(REF(IHXBuffer*) pbufContent)
{
    if (m_pbufContent != NULL)
    {
        m_pbufContent->AddRef();
    }
    pbufContent = m_pbufContent;

    return HXR_OK;
}

STDMETHODIMP
CRTSPMessageBase::SetContent(IHXBuffer* pbuf)
{
    if (m_pbufContent != NULL)
    {
        m_pbufContent->Release();
        RemoveHeader("Content-length");
    }
    if (pbuf != NULL)
    {
        IHXMIMEHeader* pHeader;
        char szLen[12];

        pbuf->AddRef();
        m_pbufContent = pbuf;

        sprintf(szLen, "%u", pbuf->GetSize());
        pHeader = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        pHeader->SetFromString("Content-length", szLen);
        AddHeader(pHeader);
    }

    return HXR_OK;
}

STDMETHODIMP
CRTSPMessageBase::GetHeaderList(REF(IHXList*) plistHeaders)
{
    if (m_plistHeaders == NULL)
    {
        m_plistHeaders = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistHeaders->AddRef();
    }

    plistHeaders = m_plistHeaders;
    m_plistHeaders->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CRTSPMessageBase::GetHeaderListConst(REF(IHXList*) plistHeaders)
{
    if (m_plistHeaders == NULL)
    {
        m_plistHeaders = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistHeaders->AddRef();
    }

    plistHeaders = m_plistHeaders->AsConst();

    return HXR_OK;
}


STDMETHODIMP
CRTSPMessageBase::GetHeader(const char* key, REF(IHXMIMEHeader*) pHeader)
{
    IHXListIterator* pIter;
    IUnknown* punkHeader;
    IHXMIMEHeader* pCurHeader;
    IHXBuffer* pbufCurKey;
    UINT32 keylen;
    IHXMIMEHeader* pMimeHead = NULL;
    CHXVector* pMultiHeaders = NULL;
    
    pHeader = NULL;

    if (m_plistHeaders != NULL)
    {
        keylen = strlen(key);
        pIter = m_plistHeaders->Begin();
        HX_ASSERT(pIter != NULL);
        while (pIter->HasItem())
        {
            punkHeader = pIter->GetItem();
            HX_ASSERT(punkHeader != NULL);
            punkHeader->QueryInterface(IID_IHXMIMEHeader, (void**)&pCurHeader);
            HX_ASSERT(pCurHeader != NULL);
            if (m_bReplaceDelimiters)
            {
                pCurHeader->ReplaceDelimiters(m_bReplaceDelimiters,
                                                      m_nReplacementDelimiter);
            }
            if (pCurHeader->GetKey(pbufCurKey) == HXR_OK)
            {
                HX_ASSERT(pbufCurKey != NULL);
                if (keylen == pbufCurKey->GetSize() && strncasecmp(key,
                                 (CPCHAR)pbufCurKey->GetBuffer(), keylen) == 0)
                {
                    if (!pMimeHead)
                    {
                        pMimeHead = pCurHeader;
                    }
                    else if (!pMultiHeaders)
                    {
                        // We have more than one. Start a list.
                        pMultiHeaders = new (m_pFastAlloc)CHXVector
                                        (m_pFastAlloc);
                        pMultiHeaders->InsertTail(pMimeHead);
                        pMultiHeaders->InsertTail(pCurHeader);
                    }
                    else
                    {
                        pMultiHeaders->InsertTail(pCurHeader);
                    }
                }
                pbufCurKey->Release();
            }
            pCurHeader->Release();
            punkHeader->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }
    
    // If we found multiple headers with this name, merge
    // them into a single CMIMEHeader object and return
    if (pMultiHeaders)
    {
        CMIMEHeader* pNewHead = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
        if (pNewHead)
        {
            pNewHead->Init((IHXList*)pMultiHeaders);
            pHeader = pNewHead;
            pHeader->AddRef();

            return HXR_OK;
        }
    }

    // If we only found one, just return it
    else if (pMimeHead)
    {
        pHeader = pMimeHead;
        pHeader->AddRef();

        return HXR_OK;
    }

    return HXR_FAIL;
}

STDMETHODIMP
CRTSPMessageBase::RemoveHeader(const char* key)
{
    HX_RESULT hxr = HXR_OK;
    BOOL bFound;
    IHXListIterator* pIter;
    IUnknown* punkHeader;
    IHXMIMEHeader* pCurHeader;
    IHXBuffer* pbufCurKey;
    UINT32 keylen;

    bFound = FALSE;
    if (m_plistHeaders != NULL)
    {
        keylen = strlen(key);
        pIter = m_plistHeaders->Begin();
        HX_ASSERT(pIter != NULL);
        while (pIter->HasItem())
        {
            punkHeader = pIter->GetItem();
            HX_ASSERT(punkHeader != NULL);
            punkHeader->QueryInterface(IID_IHXMIMEHeader, (void**)&pCurHeader);
            HX_ASSERT(pCurHeader != NULL);
            pCurHeader->GetKey(pbufCurKey);
            HX_ASSERT(pbufCurKey != NULL);
            pCurHeader->Release();
            punkHeader->Release();

            if (keylen == pbufCurKey->GetSize() &&
                strncasecmp(key, (CPCHAR)pbufCurKey->GetBuffer(), keylen) == 0)
            {
                IUnknown* punkItem;
                punkItem = m_plistHeaders->Remove(pIter);
                punkItem->Release();
                bFound = TRUE;
                pbufCurKey->Release();
                break;
            }
            pbufCurKey->Release();

            pIter->MoveNext();
        }
        pIter->Release();
    }

    return hxr;
}



/*
 * Add new header without checking for duplicates.
 */
STDMETHODIMP
CRTSPMessageBase::AddHeader(IHXMIMEHeader* pHeader)
{
    IUnknown* punkHeader;

    HX_ASSERT(pHeader != NULL);

    if (m_plistHeaders == NULL)
    {
        m_plistHeaders = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistHeaders->AddRef();
    }

    pHeader->QueryInterface(IID_IUnknown, (void**)&punkHeader);
    HX_ASSERT(punkHeader != NULL);
    m_plistHeaders->InsertTail(punkHeader);
    punkHeader->Release();

    return HXR_OK;
}

/*
 * Set header to value: if key exists it will be replaced,
 * if not a new header will be created.
 */
STDMETHODIMP
CRTSPMessageBase::SetHeader(IHXMIMEHeader* pHeader)
{
    HX_RESULT hxr;
    IHXListIterator* pIter;
    IUnknown* punkCurHeader;
    IHXMIMEHeader* pCurHeader;
    IUnknown* punkHeader;
    IHXBuffer* pbufKey;
    IHXBuffer* pbufCurKey;
    UINT32 keylen;
    CPCHAR keybuf;
    BOOL bFound;

    HX_ASSERT(pHeader != NULL);

    if (m_plistHeaders == NULL)
    {
        m_plistHeaders = new (m_pFastAlloc) CHXVector(m_pFastAlloc);
        m_plistHeaders->AddRef();
    }

    if ((hxr = pHeader->GetKey(pbufKey)) != HXR_OK)
    {
        return hxr;
    }
    keylen = pbufKey->GetSize();
    keybuf = (CPCHAR)pbufKey->GetBuffer();

    bFound = FALSE;
    hxr = HXR_FAIL;
    pIter = m_plistHeaders->Begin();
    while (pIter->HasItem())
    {
        punkCurHeader = pIter->GetItem();
        HX_ASSERT(punkCurHeader != NULL);
        punkCurHeader->QueryInterface(IID_IHXMIMEHeader, (void**)&pCurHeader);
        HX_ASSERT(pCurHeader != NULL);
        pCurHeader->GetKey(pbufCurKey);
        HX_ASSERT(pbufCurKey != NULL);
        pCurHeader->Release();
        punkCurHeader->Release();

        if (keylen == pbufCurKey->GetSize() &&
            strncasecmp(keybuf, (CPCHAR)pbufCurKey->GetBuffer(), keylen) == 0)
        {
            pHeader->QueryInterface(IID_IUnknown, (void**)&punkHeader);
            HX_ASSERT(punkHeader != NULL);
            m_plistHeaders->Replace(pIter, punkHeader);
            punkHeader->Release();
            hxr = HXR_OK;
            bFound = TRUE;
            pbufCurKey->Release();
            break;
        }
        pbufCurKey->Release();

        pIter->MoveNext();
    }
    pIter->Release();
    pbufKey->Release();

    if (!bFound)
    {
        hxr = AddHeader(pHeader);
    }

    return hxr;
}

void
CRTSPMessageBase::ReplaceDelimiters(BOOL bReplaceDelimiters,
    int nReplacementDelimiter)
{
    m_bReplaceDelimiters = bReplaceDelimiters;
    m_nReplacementDelimiter = nReplacementDelimiter;
}

UINT32
CRTSPMessageBase::GetHeaderCount(void)
{
    UINT32 n = 0;
    if (m_plistHeaders != NULL)
    {
        n = m_plistHeaders->GetCount();
    }
    return n;
}

int
CRTSPMessageBase::ParseHeader(IHXBuffer* pbufLine, UINT32* ppos)
{
    CPCHAR      pbuf;
    UINT32      buflen;
    UINT32      linelen;
    IHXBuffer* pbufKey = NULL;
    IHXBuffer* pbufVal = NULL;

    pbuf = (CPCHAR)pbufLine->GetBuffer() + *ppos;
    buflen = pbufLine->GetSize() - *ppos;
    int mimeres = mime_parse_line(pbuf, buflen, &linelen);
    if (mimeres != RTSP_RES_DONE)
    {
        return mimeres;
    }
    if (GetHeaderCount() >= RTSP_MAX_HEADER_COUNT ||
        linelen > RTSP_MAX_HEADER_SIZE)
    {
        return RTSP_RES_INVALID;
    }

    // linelen is the physical line length, not the logical length.
    // An empty line begins with a CR or LF.
    if (*pbuf == '\r' || *pbuf == '\n')
    {
        // Empty line is end of headers
        *ppos += linelen;
        m_iRecvState = RS_DATA;
        if (m_ulRecvLeft == 0)
        {
            m_iRecvState = RS_FIN;
            return RTSP_RES_DONE;
        }
        return RTSP_RES_AGAIN;
    }

    IHXBuffer* pbufHeader = new CHXStaticBuffer(pbufLine, *ppos, linelen);
    HX_ADDREF(pbufHeader);
    CMIMEHeader* pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);
    if (pHdr->Init(pbufHeader) != HXR_OK)
    {
        // We could AddRef() above and Release() both here and after the
        // AddHeader() call below, but this is less work
        delete pHdr;
        HX_RELEASE(pbufHeader);
        *ppos += buflen;
        return RTSP_RES_INVALID;
    }

    if (m_bReplaceDelimiters)
        pHdr->ReplaceDelimiters(m_bReplaceDelimiters, m_nReplacementDelimiter);

    // Extract content length manually so we don't force a MIME parse
    if (pbufHeader->GetSize() > 14 &&
        strncasecmp((CPCHAR)pbufHeader->GetBuffer(), "Content-Length:", 15) == 0)
    {
        CPCHAR pval = (CPCHAR)pbufHeader->GetBuffer() + 15;
        while (*pval == ' ' || *pval == '\t') pval++;
        //XXX: check overflow?
        m_ulRecvLeft = (UINT32)atoi(pval);
        // If the content length is too long, bail
        if (m_ulRecvLeft > RTSP_MAX_ENTITY_SIZE)
        {
	    delete pHdr;
            HX_RELEASE(pbufHeader);
            return RTSP_RES_INVALID;
        }
    }

    AddHeader(pHdr);

    *ppos += linelen;

    HX_RELEASE(pbufHeader);
    return RTSP_RES_AGAIN;
}

int CRTSPMessageBase::ParseData(IHXBuffer* pbufLine, UINT32* ppos)
{
    CPBYTE pbuf = pbufLine->GetBuffer() + *ppos;
    UINT32 buflen = pbufLine->GetSize() - *ppos;
    UINT32 curlen = min(m_ulRecvLeft, buflen);

    if (m_pbufContent == NULL)
    {
        /* this is our first content buffer */
        //XXX: check for content == buffer here?  it's quite unlikely
        m_pbufContent = new CHXStaticBuffer(pbufLine, *ppos, curlen);
        m_pbufContent->AddRef();
    }
    else
    {
        /* append new buffer to the current buffer */
        IHXBuffer* pbufNew = new CHXBuffer;
        pbufNew->AddRef();
        pbufNew->SetSize(m_pbufContent->GetSize() + curlen);
        memcpy(pbufNew->GetBuffer(), m_pbufContent->GetBuffer(), m_pbufContent->GetSize());
        memcpy(pbufNew->GetBuffer() + m_pbufContent->GetSize(), pbuf, curlen);
        m_pbufContent->Release();
        m_pbufContent = pbufNew;
    }

    m_ulRecvLeft -= curlen;
    *ppos += curlen;

    if (m_ulRecvLeft == 0)
    {
        /* we got all the data */
        m_iRecvState = RS_FIN;
        return RTSP_RES_DONE;
    }

    return RTSP_RES_AGAIN;
}

const char*
CRTSPMessageBase::GetSessionID(void)
{
    if (m_pSessionID)
    {
        return (const char*)m_pSessionID->GetBuffer();
    }

    IHXMIMEHeader* pHeader = 0;

    GetHeader("Session", pHeader);

    if (pHeader)
    {
        IHXBuffer* pBufVal = 0;

        pHeader->GetValueAsBuffer(pBufVal);
        HX_RELEASE(pHeader);
        if (pBufVal && (pBufVal->GetSize() > 0))
        {
            m_pSessionID = pBufVal;
            return (const char*)m_pSessionID->GetBuffer();
        }
        HX_RELEASE(pBufVal);
    }

    return NULL;
}

HX_RESULT
CRTSPMessageBase::SetSessionID(const char* pszNewSessID)
{
    HX_RELEASE(m_pSessionID);

    if (!pszNewSessID || !strlen(pszNewSessID))
    {
        return RemoveHeader("Session");
    }
    
    CMIMEHeader* pHdr = new (m_pFastAlloc) CMIMEHeader(m_pFastAlloc);

    pHdr->SetFromString("Session", pszNewSessID);
    pHdr->ReplaceDelimiters(TRUE, '\0');

    return SetHeader(pHdr);
}

HX_RESULT
CRTSPMessageBase::CheckURL(IHXBuffer* pURLBuf)
{
    const char* pUrl = (const char*)pURLBuf->GetBuffer();
    UINT32 ulUrlLen = pURLBuf->GetSize();
    char* pszTmp = NULL;

    if (ulUrlLen == 1)
    {
        if (pUrl[0] == '*')
        {
            // m_bIsFullyQualified is initially FALSE;
            return HXR_OK;
        }
    }

    // protocol had better be "rtsp"
    if (ulUrlLen <= 7 || strncasecmp(pUrl, "rtsp://", 7))
    {
        return HXR_INVALID_PROTOCOL;
    }
    
    // let the URL class parse the url
    CHXURLParser theUrl(pUrl, ulUrlLen);

    const char* pszHost = NULL;
    UINT32 ulHostLen = theUrl.GetHost(pszHost);
    if (!pszHost || pszHost[0] == 0)
    {
        return HXR_INVALID_PROTOCOL;            
    }

    HX_RELEASE(m_pHostname);
    m_pHostname = new CHXBuffer;
    m_pHostname->Set((const UCHAR*)pszHost, ulHostLen+1);
    pszTmp = (char*)m_pHostname->GetBuffer();
    pszTmp[ulHostLen] = 0;
    m_pHostname->AddRef();

    // check hostname length to avoid buffer over flow later
    if (m_pHostname->GetSize() > 255)
    {
        return HXR_INVALID_URL_HOST;
    }

    const char* pszPort = NULL;
    UINT32 ulPortLen = theUrl.GetPort(pszPort);
    if (pszPort && pszPort[0])
    {
        m_uPort = strtol(pszPort, NULL, 10);
    }
    else
    {
        m_uPort = 554;
    }

    const char* pszName = NULL;
    UINT32 ulNameLen = theUrl.GetFileName(pszName);
    if (pszName && pszName[0])
    {
        m_bFullyQualifiedURL = TRUE;
    }

    return HXR_OK;
}

const char*
CRTSPMessageBase::GetHostname()
{
    if (m_pHostname)
    {
        return (const char*)m_pHostname->GetBuffer();
    }
    
    return NULL;
}


HX_RESULT 
CRTSPMessageBase::ReplaceHeader(const char* szKey, const char* szNewVal)
{
    CMIMEHeader* pHdr = new (m_pFastAlloc)CMIMEHeader(m_pFastAlloc);

    pHdr->SetFromString(szKey, szNewVal);
    pHdr->ReplaceDelimiters(TRUE, '\0');

    return SetHeader(pHdr);
}


/*****************************************************************************
 *
 * CRTSPRequestMessage
 *
 *****************************************************************************/

CRTSPRequestMessage::CRTSPRequestMessage(IHXFastAlloc* pFastAlloc) :
    CRTSPMessageBase(pFastAlloc),
    m_pbufMessage(NULL),
    m_pbufVerb(NULL),
    m_pbufUrl(NULL)
{
    // Empty
}

CRTSPRequestMessage::~CRTSPRequestMessage(void)
{
    HX_RELEASE(m_pbufUrl);
    HX_RELEASE(m_pbufVerb);
    HX_RELEASE(m_pbufMessage);
}

/*** IUnknown methods ***/

STDMETHODIMP
CRTSPRequestMessage::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPMessage))
    {
        AddRef();
        *ppvObj = (IHXRTSPMessage*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPConsumer))
    {
        AddRef();
        *ppvObj = (IHXRTSPConsumer*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPRequestMessage))
    {
        AddRef();
        *ppvObj = (IHXRTSPRequestMessage*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CRTSPRequestMessage::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CRTSPRequestMessage::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXRTSPConsumer methods ***/

STDMETHODIMP
CRTSPRequestMessage::ReadDone(IHXBuffer* pbufPacket, UINT32* ppos)
{
    int     iOldState = m_iRecvState;
    UINT32  uOldPos = *ppos;
    UINT32  len = pbufPacket->GetSize();
    int     res = RTSP_RES_AGAIN;

    while (res == RTSP_RES_AGAIN && *ppos < len)
    {
        switch(m_iRecvState)
        {
        case RS_READY:
            res = ParseCommand(pbufPacket, ppos);
            break;
        case RS_HDR:
            res = ParseHeader(pbufPacket, ppos);
            break;
        case RS_DATA:
            res = ParseData(pbufPacket, ppos);
            break;
        case RS_FIN:
            *ppos = len;
            res = RTSP_RES_DONE;
            break;
        default:
            HX_ASSERT(FALSE);
            *ppos = len;
            res = RTSP_RES_DONE;
        }
    }

    // If we got a complete message, save the buffer.  This comes in handy
    // for the RTSP timing code and it doesn't cost us much.
    if (iOldState == RS_READY && m_iRecvState == RS_FIN)
    {
        HX_ASSERT(m_pbufMessage == NULL);

        // Entire message was contained in this buffer
        if (uOldPos == 0 && *ppos == len)
        {
            // The buffer IS the message
            pbufPacket->AddRef();
            m_pbufMessage = pbufPacket;
        }
        else
        {
            // The buffer contains multiple messages
            m_pbufMessage = new CHXStaticBuffer(pbufPacket,
                                                uOldPos, 
                                                (*ppos-uOldPos));
            m_pbufMessage->AddRef();
        }
    }

    if (m_iRecvState == RS_FIN)
    {
        res = RTSP_RES_DONE;
    }

    return res;
}

STDMETHODIMP_(UINT32)
CRTSPRequestMessage::GetSize(void)
{
    IHXListIterator* pIter;
    IUnknown* punkCurHeader;
    IHXMIMEHeader* pCurHeader;

    HX_ASSERT(m_pbufVerb != NULL && m_pbufUrl != NULL);

    if (m_pbufMessage != NULL && !m_bIsProxy)
    {
        return m_pbufMessage->GetSize();
    }

    UINT32 len = 0;

    len += m_pbufVerb->GetSize();
    len++;
    len += m_pbufUrl->GetSize();
    len++;
    len += 8 + 2;

    if (m_plistHeaders != NULL)
    {
        pIter = m_plistHeaders->Begin();
        HX_ASSERT(pIter != NULL);
        while (pIter->HasItem())
        {
            punkCurHeader = pIter->GetItem();
            HX_ASSERT(punkCurHeader != NULL);
            punkCurHeader->QueryInterface(IID_IHXMIMEHeader, (void**)&pCurHeader);
            HX_ASSERT(pCurHeader != NULL);
            len += pCurHeader->GetSize();
            pCurHeader->Release();
            punkCurHeader->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }
    len += 2;

    if (m_pbufContent != NULL)
    {
        len += m_pbufContent->GetSize();
    }

    return len;
}

STDMETHODIMP_(UINT32)
CRTSPRequestMessage::Write(BYTE* pbuf)
{
    IHXListIterator* pIter;
    IUnknown* punkCurHeader;
    IHXMIMEHeader* pCurHeader;

    HX_ASSERT(m_pbufVerb != NULL && m_pbufUrl != NULL);

    if (m_bIsProxy)
    {
        // proxy messages are modified regularly so we can't depend on the
        // stored message buffer. release it so we force re-write.
        HX_RELEASE(m_pbufMessage);
    }

    if (m_pbufMessage)
    {
        UINT32 len = m_pbufMessage->GetSize();
        memcpy(pbuf, m_pbufMessage->GetBuffer(), len);
        return len;
    }

    BYTE* p = pbuf;

    memcpy( p, m_pbufVerb->GetBuffer(), m_pbufVerb->GetSize() );
    p += m_pbufVerb->GetSize();
    *p++ = ' ';
    memcpy( p, m_pbufUrl->GetBuffer(), m_pbufUrl->GetSize() );
    p += m_pbufUrl->GetSize();
    *p++ = ' ';
    p += sprintf((char*)p, "RTSP/%d.%d\r\n", m_uVer/256, m_uVer%256);

    if (m_plistHeaders != NULL)
    {
        pIter = m_plistHeaders->Begin();
        HX_ASSERT(pIter != NULL);
        while (pIter->HasItem())
        {
            punkCurHeader = pIter->GetItem();
            HX_ASSERT(punkCurHeader != NULL);
            punkCurHeader->QueryInterface(IID_IHXMIMEHeader, (void**)&pCurHeader);
            HX_ASSERT(pCurHeader != NULL);
            p += pCurHeader->Write(p);
            pCurHeader->Release();
            punkCurHeader->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }
    *p++ = '\r';
    *p++ = '\n';

    if (m_pbufContent != NULL)
    {
        memcpy(p, m_pbufContent->GetBuffer(), m_pbufContent->GetSize());
        p += m_pbufContent->GetSize();
    }

    return (p-pbuf);
}

STDMETHODIMP_(UINT32)
CRTSPRequestMessage::AsBuffer(REF(IHXBuffer*) pbuf)
{
    if (m_bIsProxy)
    {
        // proxy messages are modified regularly so we can't depend on the
        // stored message buffer. release it so we force re-write.
        HX_RELEASE(m_pbufMessage);
    }

    if (m_pbufMessage == NULL)
    {
        // We need a local here because if GetSize() or Write() see that
        // m_pbufMessage is set, they will attempt to use it as the source.
        IHXBuffer* pbufNew = new CHXBuffer;
        pbufNew->AddRef();
        pbufNew->SetSize(GetSize());
        Write(pbufNew->GetBuffer());
        m_pbufMessage = pbufNew;
    }

    m_pbufMessage->AddRef();
    pbuf = m_pbufMessage;
    return m_pbufMessage->GetSize();
}

/*** IHXRTSPRequestMessage methods ***/

STDMETHODIMP_(RTSPMethod)
CRTSPRequestMessage::GetMethod(void)
{
    return (RTSPMethod)GetVerb();
}

STDMETHODIMP_(UINT32)
CRTSPRequestMessage::GetVerb(void)
{
    UINT32 verb = RTSP_VERB_EXTENSION;

    //XXXDC flame this before release!!!
    HX_ASSERT(m_pbufVerb);

    UINT32 verblen = m_pbufVerb->GetSize();
    CPBYTE verbbuf = m_pbufVerb->GetBuffer();
    for (UINT32 n = 0; n < s_nVerbs; n++)
    {
        if (verblen == s_Verbs[n].len &&
            memcmp(verbbuf, s_Verbs[n].str, verblen) == 0)
        {
            verb = n;
            break;
        }
    }

    return verb;
}

STDMETHODIMP
CRTSPRequestMessage::SetVerb(UINT32 verb)
{
    HX_ASSERT(verb < s_nVerbs);

    HX_RELEASE(m_pbufVerb);
    m_pbufVerb = new CHXStaticBuffer((UCHAR*)s_Verbs[verb].str,
                                                    s_Verbs[verb].len);
    m_pbufVerb->AddRef();
    return HXR_OK;
}

STDMETHODIMP
CRTSPRequestMessage::GetVerbEx(REF(IHXBuffer*) pbufVerb)
{
    if (m_pbufVerb != NULL)
    {
        m_pbufVerb->AddRef();
    }
    pbufVerb = m_pbufVerb;
    return HXR_OK;
}

STDMETHODIMP
CRTSPRequestMessage::SetVerbEx(IHXBuffer* pbufVerb)
{
    HX_RELEASE(m_pbufVerb);
    if (pbufVerb != NULL)
    {
        pbufVerb->AddRef();
    }
    m_pbufVerb = pbufVerb;
    return HXR_OK;
}

STDMETHODIMP
CRTSPRequestMessage::GetUrl(REF(IHXBuffer*) pbufUrl)
{
    if (m_pbufUrl != NULL)
    {
        m_pbufUrl->AddRef();
    }
    pbufUrl = m_pbufUrl;
    return HXR_OK;
}

STDMETHODIMP
CRTSPRequestMessage::SetUrl(IHXBuffer* pbufUrl)
{
    if (CheckURL(pbufUrl) != HXR_OK)
    {
        return HXR_FAIL;
    }

    HX_RELEASE(m_pbufUrl);
    if (pbufUrl != NULL)
    {
        pbufUrl->AddRef();
    }
    m_pbufUrl = pbufUrl;
    return HXR_OK;
}

int CRTSPRequestMessage::ParseCommand(IHXBuffer* pbufPkt, UINT32* ppos)
{
    UINT32 pos = 0;
    UINT32 len = pbufPkt->GetSize() - *ppos;
    CPCHAR pbuf = (CPCHAR)pbufPkt->GetBuffer();
    CPCHAR pcur = (CPCHAR)pbufPkt->GetBuffer() + *ppos;
    UINT32 linelen;
    CPCHAR pverb, pverbend;
    CPCHAR purl, purlend;
    char* pTmp = NULL;

    // LF must end line, CR is optional
    CPCHAR peol = (CPCHAR)memchr(pcur, '\n', len);
    if (peol == NULL)
    {
        if (len > RTSP_MAX_COMMAND_SIZE)
        {
            return RTSP_RES_INVALID;
        }
        return RTSP_RES_PARTIAL;
    }
    linelen = (peol-pcur+1);
    if (linelen > RTSP_MAX_COMMAND_SIZE)
    {
        return RTSP_RES_INVALID;
    }

    // Ignore empty lines (RFC 2068, s4.1)
    if ((pcur[0] == '\n') || (pcur[0] == '\r' && pcur[1] == '\n'))
    {
        *ppos += linelen;
        return RTSP_RES_AGAIN;
    }

    /*
     * RFC2326, s6.1 says "Method SP Request-URI SP RTSP-Version CRLF".  We
     * should check for exactly one space between verb and url.  However,
     * this was decided to be too strict so we allow [ \t]+ instead of SP.
     */

    // Mark verb boundaries
    pverb = pcur;
    while (IS_TOKEN_CHAR(*pcur))
    {
        pcur++;
        pos++;
    }
    pverbend = pcur;

    if (pverb == pcur || (*pcur != ' ' && *pcur != '\t'))
    {
        *ppos += len;
        return RTSP_RES_INVALID;
    }
    while (*pcur == ' ' || *pcur == '\t')
    {
        pcur++;
        pos++;
    }

    // Mark url boundaries
    purl = pcur;
    while (IS_URL_CHAR(*pcur))
    {
        pcur++;
        pos++;
    }
    purlend = pcur;

    if (purl == pcur || (*pcur != ' ' && *pcur != '\t'))
    {
        *ppos += len;
        return RTSP_RES_INVALID;
    }

   /** \todo Do we want to flag an invalid request if the scheme is wrong?
     *       If so, heres where we do it */
#if 0
    if (memcmp(purl, "rtsp", 4) != 0)
    {
        *ppos += len;
        return RTSP_RES_INVALID;
    }
#endif

    while (*pcur == ' ' || *pcur == '\t')
    {
        pcur++;
        pos++;
    }

    // Get RTSP version
    UINT16 nMajor = 0;
    UINT16 nMinor = 0;

    if (memcmp(pcur, "RTSP/", 5) != 0)
    {
        *ppos += len;
        return RTSP_RES_INVALID;
    }
    pcur += 5;
    pos += 5;
    while (isdigit(*pcur))
    {
        nMajor = min(255, nMajor*10 + (*pcur-'0'));
        pcur++;
        pos++;
    }
    if (*pcur != '.')
    {
        *ppos += len;
        return RTSP_RES_INVALID;
    }
    pcur++;
    pos++;
    while (isdigit(*pcur))
    {
        nMinor = min(255, nMinor*10 + (*pcur-'0'));
        pcur++;
        pos++;
    }
    if (*pcur != '\r' && *pcur != '\n')
    {
        *ppos += len;
        return RTSP_RES_INVALID;
    }

    // proxy NULL terminates the verb and URL for ease of use.
    if (m_bIsProxy)
    {
        pTmp = (char*)pverbend;
        *pTmp = 0;
        pTmp = (char*)purlend;
        *pTmp = 0;
    }

    // Everything checks out so save the info
    HX_RELEASE(m_pbufVerb);
    m_pbufVerb = new CHXStaticBuffer(pbufPkt, 
                                     (pverb-pbuf),
                                     (pverbend-pverb));
    m_pbufVerb->AddRef();

    HX_RELEASE(m_pbufUrl);
    m_pbufUrl = new CHXStaticBuffer(pbufPkt, 
                                     (purl-pbuf),
                                     (purlend-purl));
    m_pbufUrl->AddRef();

    CheckURL(m_pbufUrl);

    m_uVer = (nMajor << 8) + nMinor;
    m_nMajorVersion = nMajor;
    m_nMinorVersion = nMinor;

    *ppos += linelen;
    m_iRecvState = RS_HDR;

    return RTSP_RES_AGAIN;
}

const char*
CRTSPRequestMessage::TagStr()
{
    if (m_pbufVerb)
    {
        return (const char*)m_pbufVerb->GetBuffer();
    }
    else
    {
        return ("UNKNOWN");
    }
}

/*****************************************************************************
 *
 * CRTSPResponseMessage
 *
 *****************************************************************************/

CRTSPResponseMessage::CRTSPResponseMessage(IHXFastAlloc* pFastAlloc) :
    CRTSPMessageBase(pFastAlloc),
    m_pbufMessage(NULL),
    m_statuscode(0),
    m_pbufReason(NULL),
    m_reqType(RTSP_UNKNOWN)
{
    // Empty
}

CRTSPResponseMessage::~CRTSPResponseMessage(void)
{
    HX_RELEASE(m_pbufReason);
    HX_RELEASE(m_pbufMessage);
}

/*** IUnknown methods ***/

STDMETHODIMP
CRTSPResponseMessage::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPMessage))
    {
        AddRef();
        *ppvObj = (IHXRTSPMessage*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPConsumer))
    {
        AddRef();
        *ppvObj = (IHXRTSPConsumer*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPResponseMessage))
    {
        AddRef();
        *ppvObj = (IHXRTSPResponseMessage*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CRTSPResponseMessage::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CRTSPResponseMessage::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXRTSPConsumer methods ***/

STDMETHODIMP
CRTSPResponseMessage::ReadDone(IHXBuffer* pbufPacket, UINT32* ppos)
{
    int     iOldState = m_iRecvState;
    UINT32  uOldPos = *ppos;
    UINT32  len = pbufPacket->GetSize();
    int     res = RTSP_RES_AGAIN;

    while (res == RTSP_RES_AGAIN && *ppos < len)
    {
        switch(m_iRecvState)
        {
        case RS_READY:
            res = ParseCommand(pbufPacket, ppos);
            break;
        case RS_HDR:
            res = ParseHeader(pbufPacket, ppos);
            break;
        case RS_DATA:
            res = ParseData(pbufPacket, ppos);
            break;
        case RS_FIN:
        default:
            HX_ASSERT(FALSE);
            *ppos = len;
            res = RTSP_RES_DONE;
        }
    }

    // If we got a complete message, save the buffer.  This comes in handy
    // for the RTSP timing code and it doesn't cost us much.
    if (iOldState == RS_READY && m_iRecvState == RS_FIN)
    {
        HX_ASSERT(m_pbufMessage == NULL);

        // Entire message was contained in this buffer
        if (uOldPos == 0 && *ppos == len)
        {
            // The buffer IS the message
            pbufPacket->AddRef();
            m_pbufMessage = pbufPacket;
        }
        else
        {
            // The buffer contains multiple messages
            m_pbufMessage = new CHXStaticBuffer(pbufPacket,
                                                     uOldPos, (*ppos-uOldPos));
            m_pbufMessage->AddRef();
        }
    }

    //XXXTDM: see the if() at the end of request's ReadDone

    return res;
}

STDMETHODIMP_(UINT32)
CRTSPResponseMessage::GetSize(void)
{
    IHXListIterator* pIter;
    IUnknown* punkCurHeader;
    IHXMIMEHeader* pCurHeader;

    HX_ASSERT(m_pbufReason != NULL);

    if (m_pbufMessage != NULL && !m_bIsProxy)
    {
        return m_pbufMessage->GetSize();
    }

    UINT32 len = 0;

    len += 8+1+3+1;
    len += m_pbufReason->GetSize();
    len += 2;

    if (m_plistHeaders != NULL)
    {
        pIter = m_plistHeaders->Begin();
        HX_ASSERT(pIter != NULL);
        while (pIter->HasItem())
        {
            punkCurHeader = pIter->GetItem();
            HX_ASSERT(punkCurHeader != NULL);
            punkCurHeader->QueryInterface(IID_IHXMIMEHeader, (void**)&pCurHeader);
            HX_ASSERT(pCurHeader != NULL);
            len += pCurHeader->GetSize();
            pCurHeader->Release();
            punkCurHeader->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }
    len += 2;

    if (m_pbufContent != NULL)
    {
        len += m_pbufContent->GetSize();
    }

    return len;
}

STDMETHODIMP_(UINT32)
CRTSPResponseMessage::Write(BYTE* pbuf)
{
    IHXListIterator* pIter;
    IUnknown* punkCurHeader;
    IHXMIMEHeader* pCurHeader;

    HX_ASSERT(m_pbufReason != NULL);

    BYTE* p = pbuf;

    sprintf((char *)p, "RTSP/%d.%d %d ", m_uVer/256, m_uVer%256, m_statuscode);
    p += 8+1+3+1;
    memcpy(p, m_pbufReason->GetBuffer(), m_pbufReason->GetSize());
    p += m_pbufReason->GetSize();
    *p++ = '\r';
    *p++ = '\n';

    if (m_plistHeaders != NULL)
    {
        pIter = m_plistHeaders->Begin();
        HX_ASSERT(pIter != NULL);
        while (pIter->HasItem())
        {
            punkCurHeader = pIter->GetItem();
            HX_ASSERT(punkCurHeader != NULL);
            punkCurHeader->QueryInterface(IID_IHXMIMEHeader, (void**)&pCurHeader);
            HX_ASSERT(pCurHeader != NULL);            
            p += pCurHeader->Write(p);
            pCurHeader->Release();
            punkCurHeader->Release();
            pIter->MoveNext();
        }
        pIter->Release();
    }
    *p++ = '\r';
    *p++ = '\n';

    if (m_pbufContent != NULL)
    {
        memcpy(p, m_pbufContent->GetBuffer(), m_pbufContent->GetSize());
        p += m_pbufContent->GetSize();
    }

    return (p-pbuf);
}

STDMETHODIMP_(UINT32)
CRTSPResponseMessage::AsBuffer(REF(IHXBuffer*) pbuf)
{
    if (m_bIsProxy)
    {
        // proxy messages are modified regularly so we can't depend on the
        // stored message buffer. release it so we force re-write.
        HX_RELEASE(m_pbufMessage);
    }

    if (m_pbufMessage == NULL)
    {
        // We need a local here because if GetSize() or Write() see that
        // m_pbufMessage is set, they will attempt to use it as the source.
        IHXBuffer* pbufNew = new CHXBuffer;
        pbufNew->AddRef();
        pbufNew->SetSize(GetSize());
        Write(pbufNew->GetBuffer());
        m_pbufMessage = pbufNew;
    }

    m_pbufMessage->AddRef();
    pbuf = m_pbufMessage;
    return m_pbufMessage->GetSize();
}

/*** IHXRTSPResponseMessage methods ***/

STDMETHODIMP
CRTSPResponseMessage::GetStatusCode(REF(UINT32) status)
{
    status = m_statuscode;
    return HXR_OK;
}

/*
 * XXXTDM
 *  - need to handle nonstandard status codes
 *  - should binsearch s_mapStatus
 *  - should have strlen in s_mapStatus
 */
STDMETHODIMP
CRTSPResponseMessage::SetStatusCode(UINT32 status)
{
    HX_RELEASE(m_pbufReason);
    m_statuscode = status;

    StatusMapEntry* psme = s_mapStatus;
    while (psme->nCode != 0 && psme->nCode != status)
    {
        psme++;
    }
    HX_ASSERT(psme->nCode == status);

    m_pbufReason = new CHXStaticBuffer((UCHAR*)psme->szName, strlen(psme->szName));
    m_pbufReason->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CRTSPResponseMessage::GetStatusText(REF(IHXBuffer*) pbufStatus)
{
    if (m_pbufReason != NULL)
    {
        m_pbufReason->AddRef();
    }
    pbufStatus = m_pbufReason;

    return HXR_OK;
}

STDMETHODIMP
CRTSPResponseMessage::SetStatusText(IHXBuffer* pbufStatus)
{
    HX_RELEASE(m_pbufReason);
    if (pbufStatus)
    {
        pbufStatus->AddRef();
    }
    m_pbufReason = pbufStatus;

    return HXR_OK;
}

int CRTSPResponseMessage::ParseCommand(IHXBuffer* pbufPkt, UINT32* ppos)
{
    UINT32 pos = 0;
    UINT32 len = pbufPkt->GetSize() - *ppos;
    CPCHAR pbuf = (CPCHAR)pbufPkt->GetBuffer();
    CPCHAR pcur = (CPCHAR)pbufPkt->GetBuffer() + *ppos;
    UINT32 linelen;
    UINT32 reasonlen;

    // LF must end line, CR is optional
    CPCHAR peol = (CPCHAR)memchr(pcur, '\n', len);
    if (peol == NULL)
    {
        if (len > RTSP_MAX_COMMAND_SIZE)
        {
            return RTSP_RES_INVALID;
        }
        return RTSP_RES_PARTIAL;
    }
    linelen = (peol-pcur+1);
    if (linelen > RTSP_MAX_COMMAND_SIZE)
    {
        return RTSP_RES_INVALID;
    }

    // Ignore empty lines (RFC 2068, s4.1)
    if ((pcur[0] == '\n') || (pcur[0] == '\r' && pcur[1] == '\n'))
    {
        *ppos += linelen;
        return RTSP_RES_AGAIN;
    }

    // "RTSP/#.# ### "
    if (strncasecmp(pcur, "RTSP/", 5) != 0 ||
        !isdigit(pcur[5]) || pcur[6] != '.' || !isdigit(pcur[7]) ||
        pcur[8] != ' ' ||
        !isdigit(pcur[9]) || !isdigit(pcur[10]) || !isdigit(pcur[11]) ||
        pcur[12] != ' ')
    {
        *ppos += len;
        return RTSP_RES_INVALID;
    }

    m_uVer = ((pcur[5]-'0') << 8) + (pcur[7]-'0');
    m_nMajorVersion = pcur[5] - '0';
    m_nMinorVersion = pcur[7] - '0';
    pcur += 9;
    pos += 9;

    m_statuscode = (pcur[0]-'0')*100 + (pcur[1]-'0')*10 + (pcur[2]-'0');
    pcur += 3;
    pos += 3;

    pcur++;
    pos++;
    reasonlen = linelen-pos;
    while (pcur[reasonlen-1] == '\r' || pcur[reasonlen-1] == '\n')
    {
        reasonlen--;
    }
    if (reasonlen == 0)
    {
        *ppos += len;
        return RTSP_RES_INVALID;
    }
    m_pbufReason = new CHXBuffer((UCHAR*)pcur, reasonlen);
    m_pbufReason->AddRef();

    *ppos += linelen;
    m_iRecvState = RS_HDR;

    return RTSP_RES_AGAIN;
}

/*****************************************************************************
 *
 * CRTSPInterleavedPacket
 *
 *****************************************************************************/

CRTSPInterleavedPacket::CRTSPInterleavedPacket(IHXFastAlloc* pFastAlloc) :
    m_ulRefCount(0),
    m_iRecvState(RS_READY),
    m_uRecvLeft(0),
    m_pbufPacket(NULL),
    m_pFastAlloc(pFastAlloc)
{
    if (m_pFastAlloc)
    {
        m_pFastAlloc->AddRef();
    }
}

CRTSPInterleavedPacket::~CRTSPInterleavedPacket(void)
{
    HX_RELEASE(m_pbufPacket);
    HX_RELEASE(m_pFastAlloc);
}

/*** IUnknown methods ***/

STDMETHODIMP
CRTSPInterleavedPacket::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPConsumer))
    {
        AddRef();
        *ppvObj = (IHXRTSPConsumer*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTSPInterleavedPacket))
    {
        AddRef();
        *ppvObj = (IHXRTSPInterleavedPacket*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CRTSPInterleavedPacket::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CRTSPInterleavedPacket::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

/*** IHXRTSPConsumer methods ***/

STDMETHODIMP
CRTSPInterleavedPacket::ReadDone(IHXBuffer* pbufPacket, UINT32* ppos)
{
    CPBYTE pbuf = pbufPacket->GetBuffer() + *ppos;
    UINT32 len = pbufPacket->GetSize() - *ppos;
    int res = RTSP_RES_AGAIN;

    if (m_iRecvState == RS_READY)
    {
        // This is the first (and possibly only) buffer for this packet

        // Must have at least 4 bytes: '$' CHAN LEN
        if (len >= 4)
        {
            UINT32 uLen = ((UINT16)pbuf[2] << 8) + (UINT16)pbuf[3];
            m_iRecvState = RS_DATA;

            if (*ppos == 0 && len == uLen+4)
            {
                // The buffer IS the packet, just grab it
                pbufPacket->AddRef();
                m_pbufPacket = pbufPacket;
                *ppos = len;
                m_iRecvState = RS_FIN;
                res = RTSP_RES_DONE;
            }
            else
            {
                if (len >= uLen+4)
                {
                    // We got the complete packet
                    m_pbufPacket =
                        new CHXStaticBuffer(pbufPacket, *ppos,
                                                           uLen+4);
                    m_pbufPacket->AddRef();
                    *ppos += uLen+4;
                    m_iRecvState = RS_FIN;
                    res = RTSP_RES_DONE;
                }
                else
                {
                    // We got the first part of a packet, so save what we got
                    m_pbufPacket = new CHXBuffer;
                    m_pbufPacket->AddRef();
                    m_pbufPacket->SetSize(uLen+4);
                    memcpy(m_pbufPacket->GetBuffer(), pbuf, len);
                    m_uRecvLeft = (uLen+4) - len;
                    *ppos += len;
                }
            }
        }
        else
        {
            res = RTSP_RES_PARTIAL;
        }
    }
    else
    {
        // This is a continuation
        HX_ASSERT(m_iRecvState == RS_DATA);
        HX_ASSERT(m_pbufPacket != NULL);

        UINT32 uPacketOffset;
        UINT32 uLen;

        uLen = min(m_uRecvLeft, pbufPacket->GetSize());
        uPacketOffset = m_pbufPacket->GetSize() - m_uRecvLeft;

        memcpy(m_pbufPacket->GetBuffer()+uPacketOffset, pbuf, uLen);
        m_uRecvLeft -= uLen;
        *ppos += uLen;
        res = (m_uRecvLeft == 0) ? RTSP_RES_DONE : RTSP_RES_AGAIN;
    }

    return res;
}

STDMETHODIMP_(UINT32)
CRTSPInterleavedPacket::GetSize(void)
{
    HX_ASSERT(m_pbufPacket != NULL);

    return m_pbufPacket->GetSize();
}

STDMETHODIMP_(UINT32)
CRTSPInterleavedPacket::Write(BYTE* pbuf)
{
    HX_ASSERT(m_pbufPacket != NULL);

    UINT32 uPacketLen = m_pbufPacket->GetSize();
    memcpy(pbuf, m_pbufPacket->GetBuffer(), uPacketLen);

    return uPacketLen;
}

STDMETHODIMP_(UINT32)
CRTSPInterleavedPacket::AsBuffer(REF(IHXBuffer*) pbuf)
{
    HX_ASSERT(m_pbufPacket != NULL);

    UINT32 uPacketLen = m_pbufPacket->GetSize();
    m_pbufPacket->AddRef();
    pbuf = m_pbufPacket;

    return uPacketLen;
}

/*** IHXRTSPInterleavedPacket methods ***/

STDMETHODIMP
CRTSPInterleavedPacket::Get(REF(BYTE) byChan, REF(IHXBuffer*) pbufData)
{
    HX_ASSERT(m_pbufPacket != NULL);

    BYTE* pBuf = m_pbufPacket->GetBuffer();
    byChan = *(pBuf+1);

    pbufData = new CHXStaticBuffer(m_pbufPacket, 4,
                                                  m_pbufPacket->GetSize()-4);
    pbufData->AddRef();

    return HXR_OK;
}

STDMETHODIMP
CRTSPInterleavedPacket::Set(BYTE byChan, IHXBuffer* pbufData)
{
    HX_ASSERT(pbufData != NULL);
    HX_ASSERT(m_pbufPacket != NULL);

    UINT32 uDataLen;
    BYTE* pCurPos;

    uDataLen = pbufData->GetSize();
    if (uDataLen > 0xFFFF)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pbufPacket);
    m_pbufPacket = new CHXBuffer;
    m_pbufPacket->AddRef();
    m_pbufPacket->SetSize(uDataLen+4);
    pCurPos = m_pbufPacket->GetBuffer();

    // Interleaved header is: '$' chan len_hi len_lo
    *pCurPos++ = 0x24; // '$'
    *pCurPos++ = byChan;
    *pCurPos++ = uDataLen / 256;
    *pCurPos++ = uDataLen % 256;
    memcpy(pCurPos, pbufData->GetBuffer(), uDataLen);

    return HXR_OK;
}
