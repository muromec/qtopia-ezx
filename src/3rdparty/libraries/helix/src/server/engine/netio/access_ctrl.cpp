/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: access_ctrl.cpp,v 1.17 2005/04/08 21:21:49 seansmith Exp $
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
 *  given below is the access control structure in the server's registry
 *  AccessControl               COMPOSITE
 *    100                       COMPOSITE
 *       TCP                    COMPOSITE
 *          Allow               COMPOSITE
 *              To      STRING value
 *              From    STRING value
 *              Ports           COMPOSITE
 *                  elem0 NUMERIC value
 *                  elem1 NUMERIC value
 *    101
 * ...
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxnet.h"
#include "hxstrutl.h"
#include "netbyte.h"    /* for IsNumericAddr() */
#include "debug.h"
#include "proc.h"
#include "base_errmsg.h"
#include "servreg.h"
#include "server_context.h"
#include "access_ctrl.h"

#include "sockaddrimp.h"
#include "servsockimp.h"

CAccessRule::CAccessRule(UINT32 ulRuleNum,
        AccessType accesstype, HXSockType socktype,
        IHXBuffer* pLocalAddrBuf, IHXBuffer* pPeerAddrBuf,
        UINT32 ulNumPorts, UINT32* pPorts, 
        Process* proc) :
    m_ulRefCount(0),
    m_ulRuleNum(ulRuleNum),
    m_accesstype(accesstype),
    m_socktype(socktype),
    m_ulNumLocalAddr(0),
    m_ppLocalAddr(NULL),
    m_ulLocalMask(0),
    m_ulNumPeerAddr(0),
    m_ppPeerAddr(NULL),
    m_ulPeerMask(0),
    m_ulNumPorts(0),
    m_pPorts(NULL),
    m_bReady(FALSE),
    m_bNotReadyWarningLogged(FALSE),
    m_bValid(TRUE),
    m_pLocalAddrBuf(NULL),
    m_pPeerAddrBuf(NULL),
    m_pResolver(NULL),
    m_proc(proc)
{
    if (pLocalAddrBuf)
    {
        const char* pAddr = (const char *)pLocalAddrBuf->GetBuffer();
        if (strcasecmp(pAddr, "any") != 0)
        {
            const char* pMask = strchr(pAddr, '/');
            m_pLocalAddrBuf = new CHXBuffer;
            if (pMask)
            {
                m_pLocalAddrBuf->Set((const UCHAR*)pAddr, pMask - pAddr + 1);
                m_pLocalAddrBuf->GetBuffer()[pMask - pAddr] = 0;
                m_ulLocalMask = (UINT32)atoi(pMask+1);
            }
            else
            {
                m_pLocalAddrBuf->Set((const UCHAR*)pAddr, pLocalAddrBuf->GetSize());
            }
            m_pLocalAddrBuf->AddRef();
        }
    }
    if (pPeerAddrBuf)
    {
        const char* pAddr = (const char *)pPeerAddrBuf->GetBuffer();
        if (strcasecmp(pAddr, "any") != 0)
        {
            const char* pMask = strchr(pAddr, '/');
            m_pPeerAddrBuf = new CHXBuffer;
            if (pMask)
            {
                m_pPeerAddrBuf->Set((const UCHAR*)pAddr, pMask - pAddr + 1);
                m_pPeerAddrBuf->GetBuffer()[pMask - pAddr] = 0;
                m_ulPeerMask = (UINT32)atoi(pMask+1);
            }
            else
            {
                m_pPeerAddrBuf->Set((const UCHAR*)pAddr, pPeerAddrBuf->GetSize());
            }
            m_pPeerAddrBuf->AddRef();
        }
    }
    if (ulNumPorts)
    {
        HX_ASSERT (ulNumPorts <= MAX_SERVER_PORTS);
        m_ulNumPorts = ulNumPorts;
        m_pPorts = new UINT32[m_ulNumPorts];
        HX_ASSERT (m_pPorts);
        memcpy(m_pPorts, pPorts, m_ulNumPorts * sizeof(UINT32));
    }
}

CAccessRule::~CAccessRule(void)
{
    UINT32 n;
    for (n = 0; n < m_ulNumLocalAddr; n++)
    {
        m_ppLocalAddr[n]->Release();
    }
    for (n = 0; n < m_ulNumPeerAddr; n++)
    {
        m_ppPeerAddr[n]->Release();
    }
    HX_VECTOR_DELETE(m_ppLocalAddr);
    HX_VECTOR_DELETE(m_ppPeerAddr);
    HX_RELEASE(m_pLocalAddrBuf);
    HX_RELEASE(m_pPeerAddrBuf);
    HX_VECTOR_DELETE(m_pPorts);
    HX_RELEASE(m_pResolver);
}

void
CAccessRule::Resolve(void)
{
    // sequentially resolve addresses (async)
    m_proc->pc->net_services->CreateResolver(&m_pResolver);
    HX_ASSERT(m_pResolver);
    _resolveAddr();
}

STDMETHODIMP
CAccessRule::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CAccessRule::AddRef(void)
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32)
CAccessRule::Release(void)
{
    HX_ASSERT(m_ulRefCount > 0);
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;

    return 0;
}

HX_RESULT
CAccessRule::Match(IHXSockAddr* pLocalAddr, IHXSockAddr* pPeerAddr, BOOL& bMatch)
{
    bMatch = FALSE;
    if (!m_bReady)
    {
        // access control needs to deny access if a rule isn't ready
        if (!m_bNotReadyWarningLogged)
        {
            IHXErrorMessages* pErrorMessages = NULL;
            if (SUCCEEDED(m_proc->pc->server_context->QueryInterface(IID_IHXErrorMessages, (void**)&pErrorMessages)))
            {
                char szBuf[256];
                sprintf(szBuf, "access_ctrl: address check performed before rule %d was ready", m_ulRuleNum);
                pErrorMessages->Report(HXLOG_WARNING, HXR_FAIL, 0, szBuf, NULL);
            }
            HX_RELEASE(pErrorMessages);
            m_bNotReadyWarningLogged = TRUE;
        }
        return HXR_FAIL;
    }
    if (!m_bValid)
    {
        // there was a problem resolving one of the addresses so the
        // rule is ignored
        return HXR_OK;
    }

    // match against local addrs
    if (m_ulNumLocalAddr > 0)
    {
        bMatch = FALSE;
        UINT32 ulMask;

        // check against each stored local addr
        for (UINT32 n = 0; n < m_ulNumLocalAddr; n++)
        {
            IHXSockAddr* pCurAddr = m_ppLocalAddr[n];
            ulMask = m_ulLocalMask;
            if (!ulMask)
            {
                // use default mask: 32 for v4 and 128 for v6
                ulMask = (pCurAddr->GetFamily() == PF_INET) ? 32 : 128;
            }
            if (pCurAddr->IsEqualNet(pLocalAddr, ulMask))
            {
                bMatch = TRUE;
                break;
            }
        }

        if (!bMatch)
        {
            return HXR_OK;
        }
    }

    // match against peer addrs
    if (m_ulNumPeerAddr > 0)
    {
        bMatch = FALSE;
        UINT32 ulMask;

        // check against each stored peer addr
        for (UINT32 n = 0; n < m_ulNumPeerAddr; n++)
        {
            IHXSockAddr* pCurAddr = m_ppPeerAddr[n];
            ulMask = m_ulPeerMask;
            if (!ulMask)
            {
                // use default mask: 32 for v4 and 128 for v6
                ulMask = (pCurAddr->GetFamily() == PF_INET) ? 32 : 128;
            }
            if (m_ppPeerAddr[n]->IsEqualNet(pPeerAddr, ulMask))
            {
                bMatch = TRUE;
                break;
            }
        }

        if (!bMatch)
        {
            return HXR_OK;
        }
    }

    // match port list
    if (m_ulNumPorts > 0)
    {
        bMatch = FALSE;
        UINT32 ulPort = pLocalAddr->GetPort();

        UINT32 n;
        for (n = 0; n < m_ulNumPorts; n++)
        {
            if (m_pPorts[n] == ulPort)
            {
                bMatch = TRUE;
                break;
            }
        }

        if (!bMatch)
        {
            return HXR_OK;
        }
    }

    // if we made it here, then it must be a match!
    bMatch = TRUE;

    return HXR_OK;
}

// IHXResolveResponse methods

STDMETHODIMP
CAccessRule::GetAddrInfoDone(HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec)
{
    if (FAILED(status))
    {
        // resolution failed for at least one addr of the rule
        m_bValid = FALSE;
        IHXErrorMessages* pErrorMessages = NULL;
        if (SUCCEEDED(m_proc->pc->server_context->QueryInterface(IID_IHXErrorMessages, (void**)&pErrorMessages)))
        {
            char szBuf[256];
            sprintf(szBuf, "access_ctrl: failed to resolve a hostname for rule %d.", m_ulRuleNum);
            pErrorMessages->Report(HXLOG_ERR, HXR_FAIL, 0, szBuf, NULL);
        }
        HX_RELEASE(pErrorMessages);

        return status;
    }

    // addresses are resolved sequentially
    if (m_pLocalAddrBuf)
    {
        HX_RELEASE(m_pLocalAddrBuf);
        m_pResolver->Close();

        // store returned addrs
        m_ppLocalAddr = new IHXSockAddr*[nVecLen];
        m_ulNumLocalAddr = nVecLen;
        UINT32 n;
        for (n = 0; n < m_ulNumLocalAddr; n++)
        {
            m_ppLocalAddr[n] = ppAddrVec[n];
            m_ppLocalAddr[n]->AddRef();
        }
        _resolveAddr();
    }
    else if (m_pPeerAddrBuf)
    {
        HX_RELEASE(m_pPeerAddrBuf);
        m_pResolver->Close();

        // store returned addrs
        m_ppPeerAddr = new IHXSockAddr*[nVecLen];
        m_ulNumPeerAddr = nVecLen;
        UINT32 n;
        for (n = 0; n < m_ulNumPeerAddr; n++)
        {
            m_ppPeerAddr[n] = ppAddrVec[n];
            m_ppPeerAddr[n]->AddRef();
        }
        _resolveAddr();
    }
    else
    {
        HX_ASSERT(0);
    }

    return HXR_OK;
}

STDMETHODIMP
CAccessRule::GetNameInfoDone(HX_RESULT status, const char* pszNode, const char* pszService)
{
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}

void
CAccessRule::_resolveAddr(void)
{
    if (m_pLocalAddrBuf)
    {
        // resolve local addr
        m_pResolver->Init(this);
        m_pResolver->GetAddrInfo((const char*)m_pLocalAddrBuf->GetBuffer(), NULL, NULL);
    }
    else if (m_pPeerAddrBuf)
    {
        // resolve peer addr
        m_pResolver->Init(this);
        m_pResolver->GetAddrInfo((const char*)m_pPeerAddrBuf->GetBuffer(), NULL, NULL);
    }
    else
    {
        // resolution is complete, or there were no addrs to resolve.
        // either way, the rule is ready.
        m_bReady = TRUE;
    }
}

CServerAccessControl::CServerAccessControl(IHXErrorMessages* pErrMsg) :
    m_proc(NULL),
    m_registry(NULL),
    m_pErrMsg(pErrMsg),
    m_nRefCount(0)
{
    HX_ADDREF(m_pErrMsg);
}

CServerAccessControl::~CServerAccessControl(void)
{
    if (!m_rules.IsEmpty())
    {
        CAccessRule* pRule = NULL;
        POSITION pos = m_rules.GetHeadPosition();
        while (pos)
        {
            pRule = (CAccessRule*)m_rules.GetNext(pos);
            HX_RELEASE(pRule);
        }
        m_rules.RemoveAll();
    }
    HX_RELEASE(m_pErrMsg);
}

HX_RESULT
CServerAccessControl::Init(Process* proc)
{
    m_proc = proc;
    m_registry = proc->pc->registry;
    HX_RESULT res = _createRuleChain();

    return res;
}

STDMETHODIMP
CServerAccessControl::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXSocketAccessControl*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXSocketAccessControl))
    {
        AddRef();
        *ppvObj = (IHXSocketAccessControl*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CServerAccessControl::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
CServerAccessControl::Release(void)
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
CServerAccessControl::AddressCheck(IHXSockAddr* pLocalAddr,
                                   IHXSockAddr* pPeerAddr)
{
    HX_RESULT hxr = HXR_FAIL;
    UINT32 numDenyRules = 0;
    CAccessRule* pRule = NULL;

    IHXBuffer* pLocalAddrBuf = NULL;
    UINT32 nLocalPort;
    IHXBuffer* pPeerAddrBuf = NULL;
    UINT32 nPeerPort;
    pLocalAddr->GetAddr(&pLocalAddrBuf);
    nLocalPort = pLocalAddr->GetPort();
    pPeerAddr->GetAddr(&pPeerAddrBuf);
    nPeerPort = pPeerAddr->GetPort();

    DPRINTF(0x02000000, ("ServerAccessControl: Validating "
            "Local=<%s>:<%hu> Peer=<%s>:<%hu>\n",
            (const char*)pLocalAddrBuf->GetBuffer(), nLocalPort,
            (const char*)pPeerAddrBuf->GetBuffer(), nPeerPort));
    HX_RELEASE(pLocalAddrBuf);
    HX_RELEASE(pPeerAddrBuf);
    
    if (m_rules.IsEmpty())
    {
        return HXR_OK;
    }

    POSITION pos = m_rules.GetHeadPosition();
    while (pos)
    {
        pRule = (CAccessRule*)m_rules.GetNext(pos);

        if (pRule->GetAccessType() == SAC_Deny)
        {
            numDenyRules++;
        }

        BOOL bMatch;
        if (FAILED(pRule->Match(pLocalAddr, pPeerAddr, bMatch)))
        {
            // to be safe, deny access until rules are ready
            return HXR_FAIL;
        }

        if (bMatch)
        {
            break;
        }
    }

    // If all rules are deny rules, make the default fall thru rule
    // be to allow access.
    if (m_rules.GetCount() == numDenyRules)
    {
        hxr = HXR_OK;
    }

    if (pRule != NULL)
    {
        hxr = (pRule->GetAccessType() == SAC_Allow) ? HXR_OK : HXR_FAIL;
    }

    return hxr;
}

HX_RESULT
CServerAccessControl::_createRuleChain(void)
{
    HX_RESULT res = HXR_OK;
    IHXValues* props = 0;
    const char* propName = 0;   // here it represents the rule number
    UINT32 propID = 0;

    m_registry->GetPropList("config.AccessControl", props, m_proc);

    /*
     * XXXAAK -- remember to change ServerRegistry::_getPropList()
     * to return NULL (IHXValues *) in case the property does not exist
     */
    if (props)
    {
        props->GetFirstPropertyULONG32(propName, propID);
        if (!propName || !propID)
        {
            res = HXR_FAIL;
            goto endInit;
        }

        while(propName && propID)
        {
            _parseRule(propName, propID);
            propName = 0;
            propID = 0;
            props->GetNextPropertyULONG32(propName, propID);
        }
    }
    else
    {
        res = HXR_FAIL;
    }

endInit:
    return res;
}

HX_RESULT
CServerAccessControl::_parsePortsRule(const char* propName,
    UINT32 propId, UINT32* portArray, UINT32* numOfPorts)
{
    HX_RESULT res = HXR_OK;
    IHXValues* props = 0;
    const char* portPropName = 0;       // here it represents the rule number
    UINT32 portPropId = 0;
    INT32 portNum = 0;
    IHXBuffer* wildcard = 0; // "any"

    res = m_registry->GetPropList(propId, props, m_proc);
    if (HXR_OK != res)
        goto done;

    props->GetFirstPropertyULONG32(portPropName, portPropId);
    if (!portPropName || !portPropId)
    {
        res = HXR_FAIL;
        goto done;
    }

    for (*numOfPorts = 0;
        *numOfPorts < MAX_SERVER_PORTS && portPropName && portPropId;
        (*numOfPorts)++)
    {
        portNum = 0;
        res = m_registry->GetInt(portPropId, &portNum, m_proc);
        if (HXR_OK != res)
        {
            res = m_registry->GetStr(portPropId, wildcard, m_proc);
            if (HXR_OK != res)
                break;
            else
            {
                if (wildcard->GetSize() &&
                    !(strcasecmp("any", (const char *)wildcard->GetBuffer())))
                {
                    *numOfPorts = 0;
                    goto done;
                }
            }
        }
        // printf("*numOfPorts(%ld), portNum(%ld)\n", *numOfPorts, portNum);
        portArray[*numOfPorts] = (UINT32)portNum;

        portPropName = 0;
        portPropId = 0;
        props->GetNextPropertyULONG32(portPropName, portPropId);
    }

done:
    props->Release();

    return res;
}

void
CServerAccessControl::_parseRule(const char* ruleNumStr,
    UINT32 regId)
{
    IHXValues* props = 0;
    const char* propName = 0;   // here it represents the rule number
    UINT32 propID = 0;
    CAccessRule* pRule = NULL;
    char* ruleVar = 0;
    IHXBuffer* value = 0;

    // Values used for creating rule
    UINT32 ruleNum = 0;
    HXSockType socktype = HX_SOCK_TYPE_NONE;
    AccessType accessType = SAC_Allow;
    IHXBuffer* pLocalAddrBuf = 0;
    IHXBuffer* pPeerAddrBuf = 0;
    UINT32 ingressPorts[MAX_SERVER_PORTS];
    UINT32 numPorts = 0;

    ruleVar = (char*)strrchr(ruleNumStr, '.');
    if (!ruleVar || !*ruleVar)
    {
        goto fin;
    }
    ruleVar++;  // advance past the '.'
    ruleNum = (UINT32)atoi(ruleVar);

    m_registry->GetPropList(regId, props, m_proc);
    if (!props)
    {
        goto fin;
    }

    props->GetFirstPropertyULONG32(propName, propID);
    if (!propName || !propID)
    {
        goto fin;
    }

    while (propName && propID)
    {
        HX_RELEASE(value);
        ruleVar = (char*)strrchr(propName, '.');
        if (!ruleVar || !*ruleVar)
        {
            break;
        }
        ruleVar++;      // advance past the '.'

        if (!strcasecmp(ruleVar, "Transmission") ||
            !strcasecmp(ruleVar, "Protocol"))
        {
            if (HXR_OK != m_registry->GetStr(propID, value, m_proc))
            {
                break;
            }

            // NB: The access rules are only consulted for TCP
            // listening sockets at the moment, so only TCP is useful.
            if (!strcasecmp((const char*)value->GetBuffer(), "TCP"))
            {
                socktype = HX_SOCK_TYPE_TCP;
            }
            else if (!strcasecmp((const char*)value->GetBuffer(), "UDP"))
            {
                socktype = HX_SOCK_TYPE_UDP;
            }
        }
        else if (!strcasecmp(ruleVar, "Access"))
        {
            if (HXR_OK != m_registry->GetStr(propID, value, m_proc))
            {
                break;
            }

            if (!strcasecmp((const char*)value->GetBuffer(), "Allow"))
            {
                accessType = SAC_Allow;
            }
            else if (!strcasecmp((const char*)value->GetBuffer(), "Deny"))
            {
                accessType = SAC_Deny;
            }
        }
        else if (!strcasecmp(ruleVar, "To"))
        {
            if (HXR_OK != m_registry->GetStr(propID, pLocalAddrBuf, m_proc))
            {
                break;
            }
        }
        else if (!strcasecmp(ruleVar, "From"))
        {
            if (HXR_OK != m_registry->GetStr(propID, pPeerAddrBuf, m_proc))
            {
                break;
            }
        }
        else if (!strcasecmp(ruleVar, "Ports"))
        {
            if (HXR_OK != _parsePortsRule(propName, propID, ingressPorts,
                &numPorts))
                break;
        }

        propName = 0;
        propID = 0;
        props->GetNextPropertyULONG32(propName, propID);
    }

    pRule = new CAccessRule(ruleNum, accessType, socktype,
                            pLocalAddrBuf, pPeerAddrBuf,
                            numPorts, 
                            (numPorts > 0) ? ingressPorts : NULL,
                            m_proc);
    AddRule(ruleNum, pRule);
    pRule->Resolve();

fin:
    HX_RELEASE(value);
    HX_RELEASE(pLocalAddrBuf);
    HX_RELEASE(pPeerAddrBuf);
}

void
CServerAccessControl::AddRule(UINT32 uRuleNum, CAccessRule* pRule)
{
    pRule->AddRef();
    POSITION pos = m_rules.GetHeadPosition();
    POSITION nextpos;
    CAccessRule* pCurRule;
    while (pos)
    {
        nextpos = pos;
        pCurRule = (CAccessRule*)m_rules.GetNext(nextpos);
        if (pCurRule->GetRuleNum() > uRuleNum)
        {
            m_rules.InsertBefore(pos, pRule);
            return;
        }
        pos = nextpos;
    }
    m_rules.AddTail(pRule);
}
