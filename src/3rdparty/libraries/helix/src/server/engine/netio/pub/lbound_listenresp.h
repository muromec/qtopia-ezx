/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: lbound_listenresp.h,v 1.8 2004/07/24 23:10:19 tmarshall Exp $
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

#ifndef _LBOUND_RTSP_LISTENRESP_H_
#define _LBOUND_RTSP_LISTENRESP_H_

#include "inetwork.h"
#include "hxnet.h"

struct IHXBuffer;
class CLBSocket;

class LBoundRTSPListenResponse
{
public:
    LBoundRTSPListenResponse(Process* _proc);

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG32) AddRef();
    STDMETHODIMP_(ULONG32) Release();

    HX_RESULT OnConnection(CLBSocket* pNewSock,
                           Process* pProc);

private:
    virtual ~LBoundRTSPListenResponse(void);

    LONG32                      m_lRefCount;
    Process*                    m_pProc;
};

#endif /*_LBOUND_RTSP_LISTENRESP_H_*/
