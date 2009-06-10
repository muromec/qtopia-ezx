/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: listenresp.h,v 1.3 2005/01/25 22:55:21 jzeng Exp $
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

#ifndef _LISTENRESP_H_
#define _LISTENRESP_H_

#include "hxcom.h"
#include "hxerror.h"
#include "hxmon.h"
#include "sockio.h"
#include "tcpio.h"
#include "proc.h"
#include "engine.h"
#include "base_callback.h"
#include "servsockimp.h"

class ServerRegistry;

class ListenResponse : public IHXActivePropUser,
                       public IHXListeningSocketResponse
{
public:
    ListenResponse(Process* _proc);
    virtual ~ListenResponse(void);

    void Close(void);

    // IUnknown
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXActivePropUser
    STDMETHOD(SetActiveInt)     (THIS_ const char* pName, UINT32 ul,
                                       IHXActivePropUserResponse* pResponse);
    STDMETHOD(SetActiveStr)     (THIS_ const char* pName, IHXBuffer* pBuffer,
                                       IHXActivePropUserResponse* pResponse);
    STDMETHOD(SetActiveBuf)     (THIS_ const char* pName, IHXBuffer* pBuffer,
                                       IHXActivePropUserResponse* pResponse);
    STDMETHOD(DeleteActiveProp) (THIS_ const char* pName,
                                       IHXActivePropUserResponse* pResponse);

    // IHXListeningSocketResponse (must be overridden)
    STDMETHOD(OnConnection)         (THIS_ IHXSocket* ppNewSock,
                                           IHXSockAddr* ppSource) PURE;
    STDMETHOD(OnError)              (THIS_ HX_RESULT err) PURE;

protected:
    HX_RESULT InitSocket(UINT16 uPort);

protected:
    INT32                       m_nRefCount;
    Process*                    m_pProc;
    IHXErrorMessages*           m_pMessages;
    ServerRegistry*             m_pRegistry;
    IHXNetServices*             m_pNetSvc;
    IHXSockAddr*                m_pAddr;
    IHXListeningSocket*         m_pSock;

    IHXBuffer*                  m_ppActiveRes[2];
};

#endif /*_LISTENRESP_H_*/
