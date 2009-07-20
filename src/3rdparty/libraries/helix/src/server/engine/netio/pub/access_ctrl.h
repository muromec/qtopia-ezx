/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: access_ctrl.h,v 1.6 2005/04/08 00:18:10 seansmith Exp $
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

#ifndef _ACCESS_CTRL_H_
#define _ACCESS_CTRL_H_

#include "hxnet.h"
#include "hxslist.h"

struct IHXErrorMessages;
class Process;
class ServerRegistry;

typedef enum _AccessTypeTag {SAC_Allow, SAC_Deny} AccessType;

const INT32 MAX_SERVER_PORTS = 1024;
const INT32 MAX_HOST_ADDRS = 32767;
const INT32 MAX_ACCESS_RULES = 1024;

class CAccessRule : public IHXResolveResponse
{
public:
    CAccessRule(UINT32 ulRuleNum,
                AccessType accesstype, HXSockType socktype,
                IHXBuffer* pLocalAddrBuf, IHXBuffer* pPeerAddrBuf,
                UINT32 ulNumPorts, UINT32* pPorts,
                Process* proc);
    ~CAccessRule(void);
    void                Resolve(void);

    // IUnknown
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXResolveResponse
    STDMETHOD(GetAddrInfoDone)  (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone)  (THIS_ HX_RESULT status, const char* pszNode, const char* pszService);

    UINT32              GetRuleNum(void) { return m_ulRuleNum; }
    AccessType          GetAccessType(void) { return m_accesstype; }
    HXSockType          GetSockType(void) { return m_socktype; }
    HX_RESULT           Match(IHXSockAddr* pLocalAddr, IHXSockAddr* pPeerAddr, BOOL& bMatch);

private:
    void                _resolveAddr();
    
    UINT32              m_ulRuleNum;
    AccessType          m_accesstype;
    HXSockType          m_socktype;
    UINT32              m_ulNumLocalAddr;
    IHXSockAddr**       m_ppLocalAddr;
    UINT32              m_ulLocalMask;
    UINT32              m_ulNumPeerAddr;
    IHXSockAddr**       m_ppPeerAddr;
    UINT32              m_ulPeerMask;
    UINT32              m_ulNumPorts;
    UINT32*             m_pPorts;
    BOOL                m_bReady;
    BOOL                m_bNotReadyWarningLogged;
    BOOL                m_bValid;
    Process*            m_proc;

    // Address resolution stuff
    IHXBuffer*          m_pLocalAddrBuf;
    IHXBuffer*          m_pPeerAddrBuf;
    IHXResolve*         m_pResolver;

    // IUnknown
    ULONG32             m_ulRefCount;
};

class CServerAccessControl : public IHXSocketAccessControl
{
public:
    CServerAccessControl(IHXErrorMessages* pErrMsg);
    virtual ~CServerAccessControl(void);

    HX_RESULT Init(Process* proc);
    HX_RESULT Reload(void) { return HXR_NOTIMPL; }

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD(AddressCheck)         (THIS_ IHXSockAddr* pSource,
                                           IHXSockAddr* pDest);

private:
    HX_RESULT _createRuleChain(void);
    HX_RESULT _parseAddr(const char* pAddrStr, IHXSockAddr*& pAddr, UINT32& uMask);
    HX_RESULT _parsePortsRule(const char* propName, UINT32 propId, UINT32* portArray, UINT32* numOfPorts);
    void      _parseRule(const char* ruleNumStr, UINT32 regId);

    void      AddRule(UINT32 uRuleNum, CAccessRule* pRule);

private:
    INT32               m_nRefCount;
    CHXSimpleList       m_rules;
    Process*            m_proc;
    ServerRegistry*     m_registry;
    IHXErrorMessages*   m_pErrMsg;
};

#endif /* _ACCESS_CTRL_H_ */
