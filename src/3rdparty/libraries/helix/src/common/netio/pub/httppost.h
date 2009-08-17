/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httppost.h,v 1.3 2005/03/14 19:36:36 bobclark Exp $
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

#ifndef _HTTPPOST_H_
#define _HTTPPOST_H_

#include "hxnet.h"

class CHTTPPost :   public IHXSocketResponse,
                    public IHXResolveResponse
{
public:
    CHTTPPost();
    ~CHTTPPost();

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                 void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXResolveResponse methods
    STDMETHOD(GetAddrInfoDone) (THIS_
                                HX_RESULT status,
                                UINT32 nVecLen,
                                IHXSockAddr** ppAddrVec);

    STDMETHOD(GetNameInfoDone) (THIS_
                                HX_RESULT status,
                                const char* pszNode,
                                const char* pszService);

    // IHXSocketResponse methods
    STDMETHOD(EventPending) (THIS_ UINT32 uEvent, HX_RESULT status);

    HX_RESULT   Init(IUnknown* pContext, UINT32 ulTimeout);
    // pMsg = POST message, needs to be constructed by the caller
    HX_RESULT   Post(const char* pHost, UINT32 ulPort, BYTE* pMsg, UINT32 ulMsgSize);
    // POST message is constructed internally based on the pHeader & pBody
    HX_RESULT   Post(const char* pHost, UINT32 ulPort, const char* resource,
                     IHXValues* pHeader, IHXBuffer* pBody);
private:

    class CHTTPPostTimeoutCallback : public IHXCallback
    {
    public:
        CHTTPPostTimeoutCallback(CHTTPPost* pOwner);
        ~CHTTPPostTimeoutCallback();

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface)       (THIS_
                                        REFIID riid,
                                        void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)      (THIS);

        STDMETHOD_(ULONG32,Release)     (THIS);

        // *** IHXCallback method ***
        STDMETHOD(Func) (THIS);

    private:
        LONG32                  m_lRefCount;
        CHTTPPost*              m_pOwner;
    };
    friend class CHTTPPostTimeoutCallback;

    IUnknown*                   m_pContext;
    IHXScheduler*               m_pScheduler;
    IHXNetServices*             m_pNetServices;
    IHXBuffer*                  m_pPostMsg;
    IHXSocket*                  m_pSocket;
    IHXResolve*                 m_pResolver;
    LONG32                      m_lRefCount;
    HXBOOL                        m_bReleasePending;
    UINT32                      m_ulConnTimeout;
    CallbackHandle              m_ulCallbackID;
    CHTTPPostTimeoutCallback*   m_pCallback;

    HX_RESULT   ConnectDone     (HX_RESULT status);
    HX_RESULT   ReadDone        (HX_RESULT status, IHXBuffer* pBuffer);
    HX_RESULT   WriteReady      (HX_RESULT status);
    HX_RESULT   Closed          (HX_RESULT status);

    void        Close(void);
    void        RemoveCallback(void);
};

#endif //  _HTTPPOST_H_

