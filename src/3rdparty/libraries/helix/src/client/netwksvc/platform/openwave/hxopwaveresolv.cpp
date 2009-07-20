/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxopwaveresolv.cpp,v 1.5 2004/07/09 18:45:44 hubbe Exp $
 * 
 * * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
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

#include "hxopwaveresolv.h"
#include "debug.h"
#include "smartptr.h"

#define D_RESOLVER 0x10000000

//#define NO_OPENWAVE_DNS

UINT32 HXOpwaveResolver::ubMaxHostNameLen = 256L;

HXOpwaveResolver::HXOpwaveResolver() 
: OpDNSRequest()
, m_lRefCount(0)
, m_pResponse(0)
, m_psHostname(NULL)
, m_bInitialized(FALSE)
, m_nNumAddr(0)
, m_nCurAddrIndex(0)
, m_dnsStatus(HXR_OK)
{
}



HXOpwaveResolver::~HXOpwaveResolver()
{
    
    HX_RELEASE(m_pResponse);
    HX_DELETE(m_psHostname);
}

/*
*  IUnknown methods
*/
STDMETHODIMP HXOpwaveResolver::QueryInterface(THIS_
                                              REFIID riid,
                                              void** ppvObj)
{
    
    if (IsEqualIID(riid, IID_IHXResolver))
    {
        
        AddRef();
        *ppvObj = (IHXResolver*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXResolver*)this;
        return HXR_OK;
    }
    
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
    
}



STDMETHODIMP_(ULONG32) HXOpwaveResolver::AddRef(THIS)
{
    
    return InterlockedIncrement(&m_lRefCount);
    
}



STDMETHODIMP_(ULONG32)HXOpwaveResolver::Release(THIS)
{
    
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}


/*
*	IHXResolver methods
*/
STDMETHODIMP HXOpwaveResolver::Init(THIS_ IHXResolverResponse*  pResponse)
{
    
    DPRINTF(D_RESOLVER, ("HXOpwaveResolver::Init()\n"));
    
    HX_RELEASE(m_pResponse);
    m_pResponse = pResponse;
    
    if (m_pResponse)
    {
        m_pResponse->AddRef();
    }
    
    m_bInitialized = TRUE;
    return  HXR_OK;
    
}



STDMETHODIMP HXOpwaveResolver::GetHostByName(THIS_ const char* pHostName)
{
    
    DPRINTF(D_RESOLVER, ("HXOpwaveResolver::GetHostByName(%s)\n", pHostName));
    
    HX_RESULT res = HXR_OK;
    
    if (!pHostName)
    {
        res = HXR_INVALID_PARAMETER;
    }
    
    if (SUCCEEDED(res))
    {
        if (!m_psHostname)
        {
            m_psHostname = new char[ubMaxHostNameLen];
            if (!m_psHostname)
            {
                res = HXR_OUTOFMEMORY;
            }
        }
    }

    if (SUCCEEDED(res))
    {
        strncpy(m_psHostname, pHostName, ubMaxHostNameLen);
        lookupAddr(m_psHostname);
    }

#ifdef NO_OPENWAVE_DNS
/// Suppose the pHostName is in decimal form like
/// 207.188.30.91, we convert it here manually into a UINT32
    UINT32 ipv4Host = 0;
    char* pStop = pHostName;
    char* pHead = pHostName;
    INT32 nShift = 24;
    while (nShift >= 0)
    {
        UINT32 ulTemp = strtoul(pHead, &pStop, 10);
        ipv4Host += ulTemp << nShift;
        pHead = pStop + 1;
        nShift -= 8;
    }
    m_pResponse->GetHostByNameDone(m_dnsStatus, ipv4Host);
#endif

    return res;
    
}


/// OpDNSRequest callback to notify requests
void 
HXOpwaveResolver::onAddrResult(const ipv4_t *addrs, unsigned numAddrs)
{
    m_nNumAddr = numAddrs;
    if (addrs && m_nNumAddr > 0)
    {
        m_nCurAddrIndex = 0;
        m_pResponse->GetHostByNameDone(m_dnsStatus, addrs[m_nCurAddrIndex]);
    }
}

void
HXOpwaveResolver::onErrorResult()
{
    m_dnsStatus = HXR_FAIL;
}
