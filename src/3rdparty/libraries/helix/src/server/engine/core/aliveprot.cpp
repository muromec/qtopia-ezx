/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: aliveprot.cpp,v 1.6 2008/07/28 22:56:06 dcollins Exp $
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
#include "hxerror.h"
#include "ihxpckts.h"
#include "hxnet.h"
#include "hxprot.h"
#include "hxprotmgr.h"
#include "servbuffer.h"
#include "servsockimp.h"
#include "aliveprot.h"

HXAliveSniffer::HXAliveSniffer(void)
{
    // Empty
}

HXAliveSniffer::~HXAliveSniffer(void)
{
    // Empty
}

BOOL
HXAliveSniffer::Match(IHXBuffer* pBuf)
{
    UINT32 len = pBuf->GetSize();
    BYTE* buf = pBuf->GetBuffer();
    // NB: Core writes 10 chars but previous code accepted 3
    if (len >= 3 && memcmp(buf, "!!!", 3) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

HXProtocol*
HXAliveSniffer::Create(CHXServSocket* pSock)
{
    pSock->Dispatch();

    return new HXAliveProtocol();
}

HXAliveProtocol::HXAliveProtocol(void) :
    HXProtocol()
{
    // Empty
}

HXAliveProtocol::~HXAliveProtocol(void)
{
    // Empty
}

STDMETHODIMP_(ULONG32)
HXAliveProtocol::AddRef(void)
{
    HX_ASSERT(FALSE);
    return 0;
}

STDMETHODIMP_(ULONG32)
HXAliveProtocol::Release(void)
{
    HX_ASSERT(FALSE);
    return 0;
}

void
HXAliveProtocol::init(Process* proc, IHXSocket* pSock)
{
    const char pMessage[] = "200 Looks like I'm fine!\r\n";

    m_proc = proc;
    m_pSock = pSock;
    m_pSock->AddRef();

    // Read the pending buffer for graceful closure.
    IHXBuffer* pBuf = NULL;
    m_pSock->Read(&pBuf);
    HX_RELEASE(pBuf);

    pBuf = new ServerBuffer(TRUE);
    pBuf->Set((UCHAR*)pMessage, sizeof(pMessage)-1);
    m_pSock->Write(pBuf);
    HX_RELEASE(pBuf);

    Done(HXR_OK);
}

const char*
HXAliveProtocol::versionStr(void)
{
    HX_ASSERT(FALSE);
    return NULL;
}
