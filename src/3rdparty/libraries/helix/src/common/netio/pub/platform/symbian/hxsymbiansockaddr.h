/* ***** BEGIN LICENSE BLOCK *****
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _SYMBIAN_SOCKADDR_H_
#define _SYMBIAN_SOCKADDR_H_

#include "hxnet.h"
#include "hxcom.h"
#include "hxccf.h"
#include "in_sock.h"


/* CHXSockAddr
   Provides a base class for IN4 and IN6 specific address classes
*/

class CHXInetSockAddr  :  public IHXSockAddr
{
private:    
    // Unimplemented
    CHXInetSockAddr(const CHXInetSockAddr&);
    CHXInetSockAddr& operator=(const CHXInetSockAddr&);
public:
    CHXInetSockAddr(IUnknown *pContext);
    virtual ~CHXInetSockAddr(void);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;


    // IHXSockAddr functions
    STDMETHOD_(HXSockFamily,GetFamily)      (void);
    STDMETHOD_(HXBOOL,IsEqual)                (THIS_ IHXSockAddr* pOther);
    STDMETHOD(Copy)                         (THIS_ IHXSockAddr* pOther);

    // unimplemented
    STDMETHOD(Clone)                        (THIS_ IHXSockAddr** ppNew) PURE;

    STDMETHOD(GetAddr)                      (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(SetAddr)                      (THIS_ IHXBuffer* pBuf);
    STDMETHOD_(HXBOOL,IsEqualAddr)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD_(UINT16, GetPort)             (THIS);
    STDMETHOD(SetPort)                      (THIS_ UINT16 port);
    STDMETHOD_(HXBOOL,IsEqualPort)            (THIS_ IHXSockAddr* pOther);
    STDMETHOD(MaskAddr)                     (THIS_ UINT32 nBits);
    STDMETHOD_(HXBOOL,IsEqualNet)             (THIS_ IHXSockAddr* pOther,
                                               UINT32 nBits);
    //  to get the native address
    void    SetNative(const TInetAddr &addr);
    void    GetNative(TInetAddr &addr);
protected:
    TInetAddr                   m_addr;
    INT32                       m_nRefCount;
    IUnknown                    *m_pContext;
    IHXCommonClassFactory       *m_pCommonClassFactory;
};

class CHXSockAddrIN4  :  public IHXSockAddrIN4
                         ,public CHXInetSockAddr
{
private:    
    // Unimplemented
    CHXSockAddrIN4(const CHXSockAddrIN4&);
    CHXSockAddrIN4& operator=(const CHXSockAddrIN4&);

public:
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    CHXSockAddrIN4(IUnknown *pContext);
    virtual ~CHXSockAddrIN4();

    // IHXSockAddrIN4 functions
    STDMETHOD_(HXIN4AddrClass,GetAddrClass) (THIS);

    // CHXInetSockAddr override
    STDMETHOD(Clone)                        (THIS_ IHXSockAddr** ppNew);
protected:
};

class CHXSockAddrIN6  :  public IHXSockAddrIN6
                         ,public CHXInetSockAddr
{
private:    
    // Unimplemented
    CHXSockAddrIN6(const CHXSockAddrIN6&);
    CHXSockAddrIN6& operator=(const CHXSockAddrIN6&);

public:
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    CHXSockAddrIN6(IUnknown *pContext);
    virtual ~CHXSockAddrIN6();

    // CHXInetSockAddr
    STDMETHOD(Clone)                        (THIS_ IHXSockAddr** ppNew);

    // IHXSockAddrIN6

    STDMETHOD(GetFullAddr)                  (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(GetFullAddrZ)                 (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(ExtractIN4Addr)               (THIS_ IHXSockAddrIN4** ppAddr);
    STDMETHOD_(HXIN6AddrClass,GetAddrClass) (THIS);
    STDMETHOD_(UINT32,GetFlowInfo)          (THIS);
    STDMETHOD(SetFlowInfo)                  (THIS_ UINT32 uFlowInfo);
    STDMETHOD_(UINT32,GetScopeId)           (THIS);
    STDMETHOD(SetScopeId)                   (THIS_ UINT32 uScopeId);
protected:
};

#endif /* ndef _SYMBIAN_SOCKADDR_H_ */

