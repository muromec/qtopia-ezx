/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: iresolv.cpp,v 1.36 2008/05/29 10:51:48 stiwary Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hlxclib/time.h"
#include "hlxclib/ctype.h"
#include "timeval.h"
#include "hxccf.h"
#include "hxengin.h"
#include "hxbuffer.h"
#include "ihxpckts.h"
#include "nettypes.h"
#include "netdrv.h"
#include "hxinetaddr.h"
#include "hxnet.h"
#include "sockimp.h"
#include "sockaddrimp.h"
#include "hxresult.h"
#include "hxerror.h"
#include "hxassert.h"

#include "hxmap.h"
#include "hxslist.h"

#include "iresolv.h"
#include "resolvcache.h"

// RCODE values
#define RC_OK       0       /* Success */
#define RC_FMT      1       /* Format error - server cannot grok */
#define RC_FAIL     2       /* Server failed */
#define RC_EXIST    3       /* No such host/domain */
#define RC_NOTIMPL  4       /* Not implemented */
#define RC_ACCESS   5       /* Access denied */

// CLASS values
#define CL_IN       1       /* Internet */
#define CL_CS       2
#define CL_CH       3
#define CL_HS       4

// RRTYPE and QTYPE values (QTYPE is a superset of RRTYPE)
#define RR_A         1      /* Address */
#define RR_NS        2
#define RR_MD        3
#define RR_MF        4
#define RR_CNAME     5      /* Canonical name (alias) */
#define RR_SOA       6
#define RR_MB        7
#define RR_MG        8
#define RR_MR        9
#define RR_NULL     10
#define RR_WKS      11
#define RR_PTR      12      /* Pointer */
#define RR_HINFO    13
#define RR_MINFO    14
#define RR_MX       15
#define RR_TXT      16
// ... more? ...
#define RR_AAAA     28

#define MAX_PKT_LEN 512     /* Max UDP packet length per RFC */
#define MAX_RRCNT    64     /* Max RR's in a packet (local limit) */

#define ERROR_REPORT(str) (m_pErrMsg ? \
    m_pErrMsg->Report(HXLOG_ERR, HXR_FAIL, (ULONG32)HXR_FAIL, str, 0) : 0)

// This is a singly linked list for responses waiting on the same query
struct request_info
{
    struct request_info* pnext;
    IHXResolveResponse*  presp;
};

/*****************************************************************************
 *
 * Supporting code for resolver
 *
 *****************************************************************************/

// Domains to search, max 6
static char         g_szSearchDomains[6][MAX_HOST_LEN];
static unsigned int g_nSearchDomains;

// Nameservers to query, max 3
static IHXSockAddr* g_pNameservers[3];
static int          g_nNameservers;

// ReadConfig needs to happen only once
static HXBOOL g_bConfigRead = FALSE;

inline void
tv_add(HXTime& tv1, const HXTime& tv2)
{
    tv1.tv_sec += tv2.tv_sec;
    tv1.tv_usec += tv2.tv_usec;
    while(tv1.tv_usec >= 1000*1000)
    {
        tv1.tv_usec -= 1000*1000;
        tv1.tv_sec++;
    }
}

inline void
tv_sub(HXTime& tv1, const HXTime& tv2)
{
    tv1.tv_sec -= tv2.tv_sec;
    tv1.tv_usec -= tv2.tv_usec;
    while(tv1.tv_usec < 0)
    {
        tv1.tv_usec += 1000*1000;
        tv1.tv_sec--;
    }
}

#if 0 /* Inverse mappings not yet used, but should be soon */
// inet_aton for in-addr.arpa hack: "4.3.2.1.in-addr.arpa" -> 1.2.3.4
static int
inet_aton_rev(const char* cp, struct in_addr* inp)
{
    int ret = 0;
    char host[HX_ADDRSTRLEN_IN4];
    const char* end = cp;
    while (*end && (isdigit(*end) || '.' == *end))
        end++;
    if (end > cp && end-cp < HX_ADDRSTRLEN_IN4 &&
        strcasecmp(end, "in-addr.arpa") == 0)
    {
        struct in_addr addr;
        end--;
        memcpy(host, cp, end-cp);
        host[end-cp] = '\0';
        ret = inet_aton(host, &addr);
        if (ret)
        {
            memcpy(&inp->s_addr, &addr.s_addr, sizeof(struct in_addr));
        }
    }
    return ret;
}
// inet_ntoa for in-addr.arpa hack: 1.2.3.4 -> "4.3.2.1.in-addr.arpa"
static char*
inet_ntoa_rev(struct in_addr in)
{
    static char host[4*4+12+1];
    BYTE qa[4];
    memcpy(qa, &in.s_addr, 4);
    sprintf(host, "%u.%u.%u.%u.in-addr.arpa", qa[3], qa[2], qa[1], qa[0]);
    return host;
}
// IPv6 version of inet_aton_rev
static int
inet6_aton_rev(const char* cp, struct in6_addr* inp)
{
    int ret = 0;
    char host[HX_ADDRSTRLEN_IN6];
    const char* end = cp;
    while (*end && (isxdigit(*end) || ':' == *end))
        end++;
    if (end > cp && end-cp < HX_ADDRSTRLEN_IN6 &&
        strcasecmp(end, "ip6.arpa") == 0)
    {
        struct in6_addr addr;
        end--;
        memcpy(host, cp, end-cp);
        host[end-cp] = '\0';
        ret = inet6_aton(host, &addr);
        if (ret)
        {
            memcpy(&inp->s_addr, &addr.s_addr, sizeof(struct in6_addr));
        }
    }
    return ret;
}
// IPv6 version of inet_ntoa_rev
static char*
inet6_ntoa_rev(struct in6_addr in)
{
    static char host[32*2+8+1];
    BYTE qa[16];
    memcpy(qa, &in.s_addr, 4);
    for (n = 15; n >= 0; n--)
    {
        strcat(host, "%x.%x.", qa[n].lo, qa[n].hi);
    }
    strcat(host, "ip6.arpa");
    return host;
}
#endif

/*****************************************************************************
 *
 * CNodeQuery
 *
 *****************************************************************************/

static BYTE s_byQueryTmpl[] = {
    0x00, 0x00,     // Query ID (fill this in)
    0x01, 0x00,     // QR=0, OPCODE=0, AA=0, TC=0, RD=1, RA=0, Z=0, RCODE=0
    0x00, 0x01,     // QD count
    0x00, 0x00,     // AN count
    0x00, 0x00,     // NS count
    0x00, 0x00      // AR count
};

CNodeQuery::CNodeQuery(HXSockFamily f, const char* pNode) :
    m_nRefCount(0),
    m_pOwner(NULL),
    m_pCCF(NULL),
    m_pScheduler(NULL),
    m_pSock(NULL),
    m_qidIN4(0),
    m_qidIN6(0),
    m_CallbackHandle(0)
{
    m_family = f;
    strncpy(m_szNode, pNode, MAX_HOST_LEN - 1);
    m_szNode[MAX_HOST_LEN - 1] = '\0';
    m_nCurDomain = 0;
    m_nCurServer = 0;
}

CNodeQuery::~CNodeQuery(void)
{
    HX_RELEASE(m_pSock);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pOwner);
}

void
CNodeQuery::Init(CIntResolver* pOwner, IUnknown* punkContext, IHXSocket* pSock)
{
    m_pOwner = pOwner;
    m_pOwner->AddRef();
    m_pSock = pSock;
    m_pSock->AddRef();

    punkContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    HX_ASSERT(m_pCCF != NULL);

    punkContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    HX_ASSERT(m_pScheduler != NULL);

    // Initial query delta in milliseconds
    m_nDelta = 4000/g_nNameservers;

    m_nRetries = 0;
    m_nCurDomain = -1;
    m_nCurServer = 0;

    m_CallbackHandle = m_pScheduler->RelativeEnter(this, m_nDelta);

    if (m_family == HX_SOCK_FAMILY_INANY || m_family == HX_SOCK_FAMILY_IN6)
    {
        SendRequest(HX_SOCK_FAMILY_IN6);
    }
    if (m_family == HX_SOCK_FAMILY_INANY || m_family == HX_SOCK_FAMILY_IN4)
    {
        SendRequest(HX_SOCK_FAMILY_IN4);
    }
}

void
CNodeQuery::SendRequest(HXSockFamily f)
{
    // Get FQDN
    char szNode[MAX_HOST_LEN];

    if (m_nCurDomain >= 0)
    {
        sprintf(szNode, "%s.%s", m_szNode, g_szSearchDomains[m_nCurDomain]);
    }
    else
    {
        strcpy(szNode, m_szNode);
    }

    // Create the query buffer
    IHXBuffer* pBuf = NULL;
    BYTE* p;
    size_t len = 512;

    UINT16 qid = m_pOwner->NextQueryID();

    m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);
    pBuf->SetSize(len);
    p = pBuf->GetBuffer();

    // Encode the header
    memcpy(p, s_byQueryTmpl, sizeof(s_byQueryTmpl));
    *(p+0) = (qid >> 8); *(p+1) = (qid & 0xFF);
    p += sizeof(s_byQueryTmpl);
    len -= sizeof(s_byQueryTmpl);

    // Encode the hostname
    if (EncodeName(szNode, p, len) && len >= 4)
    {
        // Fill in qtype, qclass
        switch (f)
        {
        case HX_SOCK_FAMILY_IN4:
            *(p+0) = 0; *(p+1) = RR_A;
            *(p+2) = 0; *(p+3) = CL_IN;
            HX_ASSERT(m_qidIN4 == 0);
            m_qidIN4 = qid;
            break;
        case HX_SOCK_FAMILY_IN6:
            *(p+0) = 0; *(p+1) = RR_AAAA;
            *(p+2) = 0; *(p+3) = CL_IN;
            HX_ASSERT(m_qidIN6 == 0);
            m_qidIN6 = qid;
            break;
        default:
            HX_ASSERT(FALSE);
            return;
        }
        len -= 4;
        pBuf->SetSize(512-len);
        m_pOwner->AddQuery(qid, this);
        m_pSock->WriteTo(pBuf, g_pNameservers[m_nCurServer]);
    }
    HX_RELEASE(pBuf);
}

void
CNodeQuery::HandleResponse(IHXBuffer* pBuf, IHXSockAddr* pSource)
{
    BYTE*   buf = pBuf->GetBuffer();
    size_t  len = pBuf->GetSize();
    UINT16  ustmp;

    size_t  pos = 0;
    dns_hdr hdr;

    /* XXXTDM: verify source addr? */

    memcpy(&ustmp, buf+pos, 2); pos += 2; hdr.id =    hx_ntohs(ustmp);
    memcpy(&ustmp, buf+pos, 2); pos += 2; hdr.flags = hx_ntohs(ustmp);
    memcpy(&ustmp, buf+pos, 2); pos += 2; hdr.qdcnt = hx_ntohs(ustmp);
    memcpy(&ustmp, buf+pos, 2); pos += 2; hdr.ancnt = hx_ntohs(ustmp);
    memcpy(&ustmp, buf+pos, 2); pos += 2; hdr.nscnt = hx_ntohs(ustmp);
    memcpy(&ustmp, buf+pos, 2); pos += 2; hdr.arcnt = hx_ntohs(ustmp);

    // QR=1, OPCODE=0, AA=x, TC=0, RD=1, RA=x, Z=0, RCODE=x
    // 10000x01x000xxxx
    if ((hdr.flags & 0xfb70) != 0x8100 || hdr.qdcnt != 1)
    {
        // Bad header, ignore it
        return;
    }

    dns_qr_hdr      qrhdr;
    dns_rr_hdr      rrhdr;
    char            szHost[MAX_HOST_LEN];

    while (hdr.qdcnt--)
    {
        if (!ParseQuestionHeader(&qrhdr, buf, len, pos))
        {
            return;
        }
    }

    // We could take further steps to validate the response here...

    if (hdr.id == m_qidIN4) m_qidIN4 = 0;
    if (hdr.id == m_qidIN6) m_qidIN6 = 0;

    UINT rc = (hdr.flags & 0xf);
    if (rc == RC_OK)
    {
        // NB: this should be ~2.5k (40*64).
        query_res resvec[MAX_RRCNT];
        UINT32 rescnt = 0;
        UINT32 hdrcnt = 0;

        // Remove query for this qid only
        m_pOwner->DelQuery(hdr.id, this);

        while (rescnt < MAX_RRCNT && hdr.ancnt--)
        {
            if (!ParseAnswerHeader(&rrhdr, buf, len, pos))
            {
                return;
            }
            memset(&resvec[rescnt], 0, sizeof(query_res));
            resvec[rescnt].ttl = HX_MIN(86400, rrhdr.ttl); // Max TTL 1 day
            resvec[rescnt].rtype = rrhdr.rtype;
            switch (rrhdr.rtype)
            {
            case RR_A:
                if (rrhdr.rdlen % 4 != 0)
                {
                    // IPv4 addrs must be a multiple of 4
                    break;
                }
                hdrcnt = rrhdr.rdlen / 4;
                while (rescnt < MAX_RRCNT && hdrcnt)
                {
                    resvec[rescnt].a4.sin_family = AF_INET;
                    memcpy(&resvec[rescnt].a4.sin_addr, buf+pos, 4);
                    resvec[rescnt].a4.sin_port = 0;
                    hdrcnt--;
                    ++rescnt;
                }
                pos += rrhdr.rdlen;
                break;
            case RR_AAAA:
                if (rrhdr.rdlen % 16 != 0)
                {
                    // IPv6 addrs must be a multipe of 16
                    break;
                }
                hdrcnt = rrhdr.rdlen / 16;
                while (rescnt < MAX_RRCNT && hdrcnt)
                {
                    resvec[rescnt].a6.sin6_family = AF_INET6;
                    memcpy(&resvec[rescnt].a6.sin6_addr, buf+pos, 16);
                    resvec[rescnt].a6.sin6_port = 0;
                    hdrcnt--;
                    ++rescnt;
                }
                pos += rrhdr.rdlen;
                break;
            case RR_CNAME:
                if (!DecodeName(szHost, buf, len, pos))
                {
                    return;
                }
                break;
            case RR_PTR:
                //XXX: don't think we should be getting these
                if (!DecodeName(szHost, buf, len, pos))
                {
                    return;
                }
                break;
            default:
                // Looks valid but useless
                break;
            }
        }

        m_pOwner->QueryResult(m_szNode, rescnt, resvec);

        if (m_qidIN4 == 0 && m_qidIN6 == 0)
        {
            AddRef();
            m_pOwner->QueryDone(HXR_OK, m_szNode);
            if (m_CallbackHandle)
            {
                m_pScheduler->Remove(m_CallbackHandle);
            }
            Release();
        }
    }
    else
    {
        /*
         * Negative caching (RFC 2308)
         * ===========================
         *
         * NXDOMAIN: The name does not exist.  If there is an SOA entry in the
         * additional section, cache the negative response for RR.TTL seconds
         * (do NOT use the SOA.MINTTL because we are a stub resolver).  Fail
         * all pending queries with the same name regardless of address type.
         *
         * NODATA: The name exists but not with the requested address type.
         * Use the above procedure but only for the requested address type.
         */

        if (m_qidIN4 == 0 && m_qidIN6 == 0)
        {
            m_nCurDomain++;
            if ((unsigned int)m_nCurDomain >= g_nSearchDomains)
            {
                AddRef();
                // We exhausted the search list
                m_pOwner->QueryDone(HXR_FAIL, m_szNode);
                if (m_CallbackHandle)
                {
                    m_pScheduler->Remove(m_CallbackHandle);
                }
                Release();
            }
            else
            {
                m_nDelta = 4000/g_nNameservers;
                m_nRetries = 0;

                strcpy(szHost, m_szNode);
                strcat(szHost, ".");
                strcat(szHost, g_szSearchDomains[m_nCurDomain]);

                //XXX: reset server iterator?

                if (m_family == HX_SOCK_FAMILY_INANY || m_family == HX_SOCK_FAMILY_IN4)
                {
                    SendRequest(HX_SOCK_FAMILY_IN4);
                }
                if (m_family == HX_SOCK_FAMILY_INANY || m_family == HX_SOCK_FAMILY_IN6)
                {
                    SendRequest(HX_SOCK_FAMILY_IN6);
                }
            }
        }
    }
}

void
CNodeQuery::RemoveCallback()
{
       if (m_CallbackHandle && m_pScheduler)
       {
               m_pScheduler->Remove(m_CallbackHandle);
               m_CallbackHandle = 0;
       }
}

STDMETHODIMP
CNodeQuery::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CNodeQuery::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CNodeQuery::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP
CNodeQuery::Func(void)
{
    // XXXTDM: if we sent multiple queries and got at least one response,
    //         we should call m_pOwner->QueryDone(HXR_OK, m_szNode) here

    AddRef();
    m_CallbackHandle = 0;

    // Query timed out -- next nameserver
    m_nCurServer++;
    if(m_nCurServer >= g_nNameservers)
    {
        // Nameserver list exhausted, bump timeout
        m_nCurServer = 0;
        m_nRetries++;
        if(m_nRetries >= 3)
        {
            // Absolute timeout -- fail the query
            m_pOwner->QueryDone(HXR_FAIL, m_szNode);
            Release();
            return HXR_OK;
        }
        m_nDelta *= 2;
    }

    char szHost[MAX_HOST_LEN];
    strcpy(szHost, m_szNode);
    if(m_nCurDomain >= 0)
    {
        strcat(szHost, ".");
        strcat(szHost, g_szSearchDomains[m_nCurDomain]);
    }

    m_CallbackHandle = m_pScheduler->RelativeEnter(this, m_nDelta);

    if (m_family == HX_SOCK_FAMILY_INANY || m_family == HX_SOCK_FAMILY_IN4)
    {
        if (m_qidIN4)
        {
            m_pOwner->DelQuery(m_qidIN4, this);
            m_qidIN4 = 0;
            SendRequest(HX_SOCK_FAMILY_IN4);
        }
    }
    if (m_family == HX_SOCK_FAMILY_INANY || m_family == HX_SOCK_FAMILY_IN6)
    {
        if (m_qidIN6)
        {
            m_pOwner->DelQuery(m_qidIN6, this);
            m_qidIN6 = 0;
            SendRequest(HX_SOCK_FAMILY_IN6);
        }
    }
    Release();
    return HXR_OK;
}

HXBOOL
CNodeQuery::EncodeName(const char* szName, BYTE*& rpbuf, size_t& rlen)
{
    while (*szName && rlen > 0)
    {
        const char* pLabel = szName;
        BYTE nLabelLen = 0;
        BYTE nOverLen = HX_MIN(64, rlen-1);
        while(*szName && '.' != *szName && nLabelLen < nOverLen)
        {
            nLabelLen++;
            szName++;
        }
        if(nLabelLen == nOverLen)
        {
            return FALSE;
        }
        *rpbuf++ = nLabelLen;
        rlen--;
        memcpy(rpbuf, pLabel, nLabelLen);
        rpbuf += nLabelLen;
        rlen -= nLabelLen;
        if(*szName == '.')
        {
            szName++;
        }
    }
    if(rlen < 1)
    {
        return FALSE;
    }
    *rpbuf++ = 0;
    rlen--;

    return TRUE;
}

HXBOOL
CNodeQuery::ParseQuestionHeader(dns_qr_hdr* phdr, const BYTE* pbuf, size_t buflen, size_t& rpos)
{
    char szHost[MAX_HOST_LEN];
    UINT16 usTmp;

    if(! DecodeName( szHost, pbuf, buflen, rpos ))
    {
        return FALSE;
    }
    if(rpos+4 > buflen)
    {
        return FALSE;
    }

    strcpy(phdr->szHost, szHost);
    memcpy(&usTmp, pbuf+rpos, 2); rpos += 2;
    phdr->qtype = ntohs(usTmp);
    memcpy(&usTmp, pbuf+rpos, 2); rpos += 2;
    phdr->qclass = ntohs(usTmp);
    if(phdr->qclass != CL_IN)
    {
        return FALSE;
    }

    return TRUE;
}

HXBOOL
CNodeQuery::ParseAnswerHeader(dns_rr_hdr* phdr, const BYTE* pbuf, size_t buflen, size_t& rpos)
{
    char szHost[MAX_HOST_LEN];
    UINT16 usTmp;
    UINT32 ulTmp;

    if(! DecodeName( szHost, pbuf, buflen, rpos ))
    {
        return FALSE;
    }
    if(rpos+10 > buflen)
    {
        return FALSE;
    }

    memcpy(&usTmp, pbuf+rpos, 2); rpos += 2;
    phdr->rtype = ntohs(usTmp);
    memcpy(&usTmp, pbuf+rpos, 2); rpos += 2;
    phdr->rclass = ntohs(usTmp);
    memcpy(&ulTmp, pbuf+rpos, 4); rpos += 4;
    phdr->ttl = ntohl(ulTmp);
    memcpy(&usTmp, pbuf+rpos, 2); rpos += 2;
    phdr->rdlen = ntohs(usTmp);
    if(phdr->rclass != CL_IN)
    {
        return FALSE;
    }

    return TRUE;
}

HXBOOL
CNodeQuery::DecodeName(char* pname, const BYTE* pbuf, size_t buflen, size_t& rpos)
{
    size_t pos = rpos;
    size_t namelen = 0;
    HX_ASSERT(buflen > 0 && buflen <= 512 && pos < buflen);

    HXBOOL bHasPtr = FALSE;
    while(pbuf[pos])
    {
        UINT8 len = pbuf[pos];
        if(!(len & 0xC0))
        {
            // Label
            pos++;
            if(len >= buflen-pos || len+namelen > MAX_HOST_LEN-2)
            {
                return FALSE;
            }
            memcpy(pname, pbuf+pos, len);
            pos += len;
            *(pname+len) = '.';
            len++;
            pname += len;
            namelen += len;
            if(!bHasPtr)
            {
                rpos += len;
            }
        }
        else
        {
            // Pointer
            if((len & 0xC0) != 0xC0 || pos > buflen-2)
            {
                return FALSE;
            }
            pos = (UINT16)(pbuf[pos] & 0x3F)*256 + (UINT16)(pbuf[pos+1]);
            if(pos >= buflen-1)
            {
                return FALSE;
            }
            if(!bHasPtr)
            {
                rpos += 2;
            }
            bHasPtr = TRUE;
        }
    }
    if(!namelen)
    {
        return FALSE; //XXX: is root domain
    }
    *(pname-1) = '\0';
    if(!bHasPtr)
    {
        rpos++;
    }
    return TRUE;
}

/*****************************************************************************
 *
 * CIntResolver
 *
 *****************************************************************************/

CIntResolver::CIntResolver(IUnknown* punkContext,
                CResolverCache* pCache /* = NULL */) :
    m_nRefCount(0),
    m_punkContext(punkContext),
    m_pNetSvc(NULL),
    m_usQueryID(rand()),
    m_pSock(NULL),
    m_pResponse(NULL),
    m_pCache(pCache),
    m_port(0)
{
    m_punkContext->AddRef();
    if (m_pCache != NULL)
    {
        m_pCache->AddRef();
    }

    m_punkContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
    HX_ASSERT(m_pCCF != NULL);

    m_punkContext->QueryInterface(IID_IHXNetServices, (void**)&m_pNetSvc);
    HX_ASSERT(m_pNetSvc != NULL);

    if (!g_bConfigRead)
    {
        g_bConfigRead = TRUE;
        ReadConfig();
    }
}

CIntResolver::~CIntResolver(void)
{
    Close();

    HX_RELEASE(m_pCache);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_punkContext);
}

UINT16
CIntResolver::NextQueryID(void)
{
    UINT16 qid;
    do
    {
        // An arbitrary nonlinear sequence that cycles through all values
        qid = m_usQueryID = m_usQueryID*9+5;
    }
    while (qid == 0);
    return qid;
}

void
CIntResolver::AddQuery(UINT16 qid, CNodeQuery* pQuery)
{
    POSITION pos = m_mapHostQueries.Lookup(qid);
    if (pos)
    {
        HX_ASSERT(FALSE);
        m_mapHostQueries.RemoveKey(qid);
    }
    pQuery->AddRef();
    m_mapHostQueries.SetAt(qid, pQuery);
}

void
CIntResolver::DelQuery(UINT16 qid, CNodeQuery* pQuery)
{
    HX_ASSERT(m_mapHostQueries[qid] == pQuery);
    m_mapHostQueries.RemoveKey(qid);
    if (pQuery != NULL)
    {
        pQuery->Release();
    }
}

void
CIntResolver::QueryResult(const char* pNode, UINT32 rescnt, query_res* resvec)
{
    UINT32 n;

    time_t tNow;
    UINT32 uVecLen;
    time_t* ptExpireVec;
    IHXSockAddr** ppAddrVec;

    tNow = time(NULL);
    uVecLen = rescnt;
    ptExpireVec = new time_t[rescnt];
    ppAddrVec = new IHXSockAddr*[rescnt];

    for (n = 0; n < rescnt; n++)
    {
        ptExpireVec[n] = tNow + resvec[n].ttl;
        switch (resvec[n].rtype)
        {
        case RR_A:
            ppAddrVec[n] = new CHXSockAddrIN4(m_punkContext, &resvec[n].a4);
            ppAddrVec[n]->AddRef();
            break;
        case RR_AAAA:
            ppAddrVec[n] = new CHXSockAddrIN6(m_punkContext, &resvec[n].a6);
            ppAddrVec[n]->AddRef();
            break;
        default:
            HX_ASSERT(FALSE);
            ppAddrVec[n] = NULL;
        }
    }

    if (m_pCache != NULL && uVecLen > 0)
    {
        m_pCache->AddAddrInfo(pNode, uVecLen, ptExpireVec, ppAddrVec);
    }

    for (n = 0; n < rescnt; n++)
    {
        ppAddrVec[n]->Release();
    }
    delete[] ppAddrVec;
    delete[] ptExpireVec;
}

void
CIntResolver::QueryDone(HX_RESULT status, const char* pNode)
{
    //XXXTDM: Need to handle case where we don't have a cache, perhaps with
    //        member variables.
    UINT32 uAddrVecLen = 0;
    IHXSockAddr** ppAddrVec = NULL;
    if (status == HXR_OK && m_pCache != NULL)
    {
        m_pCache->FindAddrInfo(pNode, uAddrVecLen, ppAddrVec);
    }
    if (uAddrVecLen > 0)
    {
        IHXSockAddr** ppAnswerVec = new IHXSockAddr*[uAddrVecLen];
        UINT32 n;
        for (n = 0; n < uAddrVecLen; n++)
        {
            ppAddrVec[n]->Clone(&ppAnswerVec[n]);
            ppAnswerVec[n]->SetPort(m_port);
        }
        if (m_pResponse)
        {
            m_pResponse->GetAddrInfoDone(HXR_OK, uAddrVecLen, ppAnswerVec);
        }
        for (n = 0; n < uAddrVecLen; n++)
        {
            HX_RELEASE(ppAnswerVec[n]);
        }
        delete[] ppAnswerVec;
    }
    else
    {
        if (m_pResponse)
        {
            m_pResponse->GetAddrInfoDone(HXR_FAIL, 0, NULL);
        }
    }
}

STDMETHODIMP
CIntResolver::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXResolve*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXResolve))
    {
        AddRef();
        *ppvObj = (IHXResolve*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXSocketResponse*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CIntResolver::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CIntResolver::Release(void)
{
    HX_ASSERT(m_nRefCount > 0);
    INT32 rc = InterlockedDecrement(&m_nRefCount);
    if (rc == 0)
    {
        delete this;
    }
    return rc;
}

STDMETHODIMP
CIntResolver::Init(IHXResolveResponse* pResponse)
{
    HX_RESULT hxr;
    IHXSockAddr* pAddr = NULL;

    // Create a UDP socket and bind it to <any>
    HX_RELEASE(m_pSock);
    hxr = m_pNetSvc->CreateSocket(&m_pSock);
    if (hxr != HXR_OK)
    {
        return hxr;
    }
    hxr = m_pSock->Init(HX_SOCK_FAMILY_IN4,
                        HX_SOCK_TYPE_UDP,
                        HX_SOCK_PROTO_ANY);
    if (hxr != HXR_OK)
    {
        return hxr;
    }
    hxr = m_pSock->SetResponse(this);
    if (hxr != HXR_OK)
    {
        return hxr;
    }
    hxr = m_pSock->CreateSockAddr(&pAddr);
    if (hxr != HXR_OK)
    {
        return hxr;
    }
    hxr = m_pSock->Bind(pAddr);
    HX_RELEASE(pAddr);
    if (hxr != HXR_OK)
    {
        return hxr;
    }

    m_pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_CLOSE);

    // All systems go.  Save response.
    m_pResponse = pResponse;
    if (m_pResponse != NULL)
    {
        m_pResponse->AddRef();
    }

    return HXR_OK;
}

STDMETHODIMP
CIntResolver::Close()
{
    POSITION pos;

    pos = m_mapHostQueries.GetStartPosition();
    while(pos)
    {
        LONG32 lQueryID = 0;
        CNodeQuery* pQuery = NULL;
        m_mapHostQueries.GetNextAssoc(pos, lQueryID, (void*&)pQuery);
        pQuery->RemoveCallback();
        HX_RELEASE(pQuery);
    }
    m_mapHostQueries.RemoveAll();

    if (m_pSock)
    {
        m_pSock->Close();
        HX_RELEASE(m_pSock);
    }

    HX_RELEASE(m_pResponse);

    return HXR_OK;
}

STDMETHODIMP
CIntResolver::GetAddrInfo(const char* pNode, const char* pService,
                IHXAddrInfo* pHints)
{
    if (m_pSock == NULL || m_pResponse == NULL)
    {
        return HXR_UNEXPECTED;
    }

    UINT32 n;
    IHXSockAddr* pAddr = NULL;
    UINT16 port = 0;

    // XXX allow symbolic port names
    if (pService != NULL)
    {
        m_port = (UINT16)atoi(pService);
    }

    // Determine the desired socket family
    HXSockFamily f = HX_SOCK_FAMILY_INANY;
    if (pHints != NULL)
    {
        switch (pHints->GetFamily())
        {
        case AF_INET:
            f = HX_SOCK_FAMILY_IN4;
            break;
        case AF_INET6:
            f = HX_SOCK_FAMILY_IN6;
            break;
        default:
            f = HX_SOCK_FAMILY_INANY;
        }
    }

    // Check for an unspecified nodename
    if (pNode == NULL)
    {
        m_pNetSvc->CreateSockAddr(f, &pAddr);
        if (pHints != NULL && !(pHints->GetFlags() & HX_ADDRFLAGS_PASSIVE))
        {
            IHXBuffer* pNodeBuf = NULL;
            m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pNodeBuf);
            switch (pHints->GetFamily())
            {
            case AF_INET:
                pNode = "127.0.0.1";
                break;
            case AF_INET6:
                pNode = "::1";
                break;
            default:
                HX_ASSERT(FALSE);
            }
            pNodeBuf->Set((UCHAR*)pNode, strlen(pNode)+1);
            pAddr->SetAddr(pNodeBuf);
            HX_RELEASE(pNodeBuf);
        }
        m_pResponse->GetAddrInfoDone(HXR_OK, 1, &pAddr);
        HX_RELEASE(pAddr);
        return HXR_OK;
    }

    // Check for a numeric nodename.  Try IPv4 first, then IPv6
    if (f == HX_SOCK_FAMILY_INANY || f == HX_SOCK_FAMILY_IN4)
    {
        IHXBuffer* pNodeBuf = NULL;
        m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pNodeBuf);
        pNodeBuf->Set((UCHAR*)pNode, strlen(pNode)+1);

        m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
        if (pAddr->SetAddr(pNodeBuf) == HXR_OK)
        {
            pAddr->SetPort(m_port);
            m_pResponse->GetAddrInfoDone(HXR_OK, 1, &pAddr);
            HX_RELEASE(pAddr);
            HX_RELEASE(pNodeBuf);
            return HXR_OK;
        }
        HX_RELEASE(pAddr);
        HX_RELEASE(pNodeBuf);
    }
    if (f == HX_SOCK_FAMILY_INANY || f == HX_SOCK_FAMILY_IN6)
    {
        IHXBuffer* pNodeBuf = NULL;
        m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pNodeBuf);
        pNodeBuf->Set((UCHAR*)pNode, strlen(pNode)+1);

        m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN6, &pAddr);
        if (pAddr->SetAddr(pNodeBuf) == HXR_OK)
        {
            pAddr->SetPort(m_port);
            m_pResponse->GetAddrInfoDone(HXR_OK, 1, &pAddr);
            HX_RELEASE(pAddr);
            HX_RELEASE(pNodeBuf);
            return HXR_OK;
        }
        HX_RELEASE(pAddr);
        HX_RELEASE(pNodeBuf);
    }

    // Address is not numeric, verify AI_NUMERICHOST is not set
    if (pHints != NULL && (pHints->GetFlags() & HX_ADDRFLAGS_NUMERICHOST))
    {
        m_pResponse->GetAddrInfoDone(HXR_FAIL, 0, NULL);
        return HXR_OK;
    }

    /*
     * At this point we have a non-numeric hostname that needs resolved.  If
     * we have a cache, search for the given name.  If that fails, append
     * each of the domains in the domain list and retry.
     */
    UINT32 uAddrVecLen = 0;
    IHXSockAddr** ppAddrVec = NULL;
    if (m_pCache != NULL)
    {
        if (!m_pCache->FindAddrInfo(pNode, uAddrVecLen, ppAddrVec) &&
            g_nSearchDomains > 0)
        {
            char szFQDN[MAX_HOST_LEN];
            for (n = 0; n < g_nSearchDomains; n++)
            {
                sprintf(szFQDN, "%s.%s", pNode, g_szSearchDomains[n]);
                if (m_pCache->FindAddrInfo(szFQDN, uAddrVecLen, ppAddrVec))
                {
                    break;
                }
            }
        }
    }
    if (uAddrVecLen > 0)
    {
        IHXSockAddr** ppAnswerVec = new IHXSockAddr*[uAddrVecLen];
        for (n = 0; n < uAddrVecLen; n++)
        {
            ppAddrVec[n]->Clone(&ppAnswerVec[n]);
            ppAnswerVec[n]->SetPort(m_port);
        }
        m_pResponse->GetAddrInfoDone(HXR_OK, uAddrVecLen, ppAnswerVec);
        for (n = 0; n < uAddrVecLen; n++)
        {
            HX_RELEASE(ppAnswerVec[n]);
        }
        delete[] ppAnswerVec;
        return HXR_OK;
    }

    // Looks like we have to send a query
    CNodeQuery* pQuery = new CNodeQuery(f, pNode);
    pQuery->AddRef();
    pQuery->Init(this, m_punkContext, m_pSock);
    pQuery->Release();

    return HXR_OK;
}

STDMETHODIMP
CIntResolver::GetNameInfo(IHXSockAddr* pAddr, UINT32 uFlags)
{
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

STDMETHODIMP
CIntResolver::EventPending(UINT32 uEvent, HX_RESULT status)
{
    IHXBuffer* pBuf = NULL;
    IHXSockAddr* pSource = NULL;

    AddRef();
    switch (uEvent)
    {
    case HX_SOCK_EVENT_READ:
        if (SUCCEEDED(m_pSock->ReadFrom(&pBuf, &pSource)))
        {
            HandleResponse(pBuf, pSource);
        }
        HX_RELEASE(pSource);
        HX_RELEASE(pBuf);
        break;
    case HX_SOCK_EVENT_CLOSE:
        // This is a catastrophic failure; should we attempt recovery?
        HX_ASSERT(FALSE);
        break;
    default:
        HX_ASSERT(FALSE);
    }
    Release();
    return HXR_OK;
}

void
CIntResolver::HandleResponse(IHXBuffer* pBuf, IHXSockAddr* pSource)
{
    BYTE*   buf;
    UINT32  len;

    buf = pBuf->GetBuffer();
    len = pBuf->GetSize();

    if (len < 6*sizeof(UINT16) || len > (ssize_t)MAX_PKT_LEN)
    {
        // Packet length is out of bounds, ignore it
        return;
    }

    // Get Query ID
    UINT16 ustmp;
    UINT16 qid;
    memcpy(&ustmp, buf, sizeof(ustmp));
    qid = hx_ntohs(ustmp);

    // Find pending query
    CNodeQuery* pQuery = NULL;
    m_mapHostQueries.Lookup((LONG32)qid, (void*&)pQuery);
    if (pQuery == NULL)
    {
        // No match so ignore it
        return;
    }

    pQuery->HandleResponse(pBuf, pSource);
}

static HXBOOL
parse_tokval(char* buf, const char* tok, char** ppval)
{
    HXBOOL rv = FALSE;
    int toklen = strlen(tok);
    if (!strncmp(buf, tok, toklen) && isspace(buf[toklen]))
    {
        char* pval = buf+toklen;
        while (isspace(*pval)) pval++;
        if (*pval)
        {
            *ppval = pval;
            rv = TRUE;
        }
    }
    return rv;
}

HXBOOL
CIntResolver::ReadConfig(void)
{
    g_nSearchDomains = 0;
    g_nNameservers = 0;

    IHXNetServices* pNetSvc = NULL;
    m_punkContext->QueryInterface(IID_IHXNetServices, (void**)&pNetSvc);
    if (pNetSvc == NULL)
    {
        return FALSE;
    }

#if defined(_UNIX)
    FILE* fp;
    char linebuf[1024];
    char* pval;
    char* p;

    fp = fopen("/etc/resolv.conf", "r");
    if(fp != NULL)
    {
        while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
        {
            if (parse_tokval(linebuf, "nameserver", &pval) &&
                g_nNameservers < 3)
            {
                IHXBuffer* pBuf = NULL;
                IHXSockAddr* pAddr = NULL;

                // Find end of addr and chop off any trailing whitespace
                p = pval;
                while (*p && !isspace(*p)) p++;
                *p = '\0';

                pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
                CHXBuffer::FromCharArray(pval, &pBuf);
                if (pAddr->SetAddr(pBuf) == HXR_OK)
                {
                    pAddr->SetPort(53);
                    g_pNameservers[g_nNameservers++] = pAddr;
                }
                else
                {
                    pAddr->Release();
                    pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN6, &pAddr);
                    if (pAddr->SetAddr(pBuf) == HXR_OK)
                    {
                        pAddr->SetPort(53);
                        g_pNameservers[g_nNameservers++] = pAddr;
                    }
                    else
                    {
                        pAddr->Release();
                    }
                }
                HX_RELEASE(pBuf);
            }
            else if(parse_tokval( linebuf, "domain", &pval ))
            {
                p = pval;
                while(*p && !isspace(*p)) p++;
                *p = '\0';
                g_nSearchDomains = 1;
                strcpy(g_szSearchDomains[0], pval);
            }
            else if(parse_tokval( linebuf, "search", &pval ))
            {
                while(*pval)
                {
                    p = pval;
                    while(*p && !isspace(*p))
                    {
                        p++;
                    }
                    if(g_nSearchDomains < 6)
                    {
                        strncpy(g_szSearchDomains[g_nSearchDomains], pval, p-pval);
                        g_szSearchDomains[g_nSearchDomains][p-pval] = '\0';
                        g_nSearchDomains++;
                    }
                    pval = p;
                    while(*pval && isspace(*pval))
                    {
                        pval++;
                    }
                }
            }
        }
        fclose(fp);
    }
#elif defined(_WIN32)
    /*
     * Win32 keeps per-interface resolver configurations in:
     *
     *   HKLM\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces\
     *
     * This is a tree with each adapter listed as a CLSID.  If an adapter has
     * a NameServer key, it represents the hardcoded resolver list from the
     * network control panel.  If it does not exist, check for the DHCP
     * assigned config in the DhcpNameServer key.
     */
    long  rc;
    HKEY  hkiflist, hkif;
    TCHAR kname[255];
    DWORD knamelen;
    BYTE  val[255];
    DWORD len;

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"), 0, KEY_READ, &hkiflist);
    if (rc == ERROR_SUCCESS)
    {
        DWORD dwIdx;
        knamelen = sizeof(kname);
        for (dwIdx = 0; knamelen=sizeof(kname),
             RegEnumKeyEx(hkiflist, dwIdx, kname, &knamelen, NULL, NULL,
                          NULL, NULL) != ERROR_NO_MORE_ITEMS && g_nNameservers < 3;
             dwIdx++)
        {
            rc = RegOpenKeyEx(hkiflist, kname, 0, KEY_READ, &hkif);
            if (rc == ERROR_SUCCESS)
            {
                len = sizeof(val);
                rc = RegQueryValueEx(hkif, TEXT("NameServer"), NULL, NULL,
                                     val, &len);
                if (rc != ERROR_SUCCESS || val[0] == '\0')
                {
                    len = sizeof(val);
                    rc = RegQueryValueEx(hkif, TEXT("DhcpNameServer"), NULL, NULL,
                                         val, &len);
                }
                if (rc == ERROR_SUCCESS && val[0] != '\0')
                {
                    char* pcur = (char*)val;
                    char* pnext;
                    do
                    {
                        pnext = strchr(pcur, ' ');
                        if(pnext == NULL)
                        {
                            pnext = strchr(pcur, ',');
                        }

                        if (pnext != NULL)
                        {
                            *pnext++ = '\0';
                        }

                        IHXBuffer* pBuf = NULL;
                        IHXSockAddr* pAddr = NULL;
                        pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
                        CHXBuffer::FromCharArray(pcur, &pBuf);
                        if (pAddr->SetAddr(pBuf) == HXR_OK)
                        {
                            pAddr->SetPort(53);
                            g_pNameservers[g_nNameservers++] = pAddr;
                        }
                        else
                        {
                            pAddr->Release();
                            pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN6, &pAddr);
                            if (pAddr->SetAddr(pBuf) == HXR_OK)
                            {
                                pAddr->SetPort(53);
                                g_pNameservers[g_nNameservers++] = pAddr;
                            }
                            else
                            {
                                pAddr->Release();
                            }
                        }
                        HX_RELEASE(pBuf);
                        pcur = pnext;
                    }
                    while (pcur != NULL && g_nNameservers < 3);
                }
                //RegCloseKey(hkif);
            }
        }
        //RegCloseKey(hkiflist);
    }
#else
#error "Don't know how to read platform resolver configuration"
#endif

    // If no nameservers are specified, use localhost
    if(g_nNameservers == 0)
    {
        IHXBuffer* pBuf = NULL;
        IHXSockAddr* pAddr = NULL;
        CHXBuffer::FromCharArray("127.0.0.1", &pBuf);
        pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_IN4, &pAddr);
        pAddr->SetAddr(pBuf);
        pAddr->SetPort(53);
        HX_RELEASE(pBuf);
        g_pNameservers[g_nNameservers++] = pAddr;
    }

    HX_RELEASE(pNetSvc);

    return TRUE;
}
