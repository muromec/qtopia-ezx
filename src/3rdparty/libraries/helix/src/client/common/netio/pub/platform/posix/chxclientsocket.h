/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxclientsocket.h,v 1.9 2008/08/27 10:36:40 lovish Exp $
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

#ifndef CHXCLIENTSOCKET_H__
#define CHXCLIENTSOCKET_H__

#include "hxsocketselector.h"

#include "hxnet.h"
#include "netdrv.h"
#include "sockimp.h"

class CHXNetServices;

class CHXClientSocket 
: public CHXSocket
, public HXSocketSelector::EventHandler
{
public:
  
    CHXClientSocket(CHXNetServices* pNetSvc, IUnknown* pUnknown, HXSockFamily f, HXSockType t, HXSockProtocol p, HX_SOCK sock = HX_SOCK_NONE);
    CHXClientSocket(CHXNetServices* pNetSvc, IUnknown* pUnknown);
    ~CHXClientSocket();

    // HXSocketSelector::EventHandler
    HX_RESULT OnSelectEvent(sockobj_t fd, UINT32 event, HX_RESULT err);

    // IHXSocket
    STDMETHOD(Init)  (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p);
    STDMETHOD(Close) (THIS);

private:
    // CHXSocket
    virtual HX_RESULT Select(UINT32 uEventMask, HXBOOL bImplicit = TRUE);
    
    HX_RESULT DoClientSockInit();
    HX_RESULT InitSelector();
    void CloseSelector();
    HX_RESULT InitRcvBufSize();
    HX_RESULT InitPerReadBufSize();

private:
    HXSocketSelector* m_pSelector;
};

#endif /*CHXCLIENTSOCKET_H__*/
