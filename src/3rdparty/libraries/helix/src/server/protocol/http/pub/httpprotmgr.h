/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpprotmgr.h,v 1.2 2004/08/24 21:37:31 tmarshall Exp $
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

#ifndef _HTTPPROTMGR_H_
#define _HTTPPROTMGR_H_

#include "hxprot.h"
#include "hxstring.h"
#include "hxslist.h"
#include "httpmsg.h"

class HTTPParser;
class CByteQueue;
class BWCalculator;

/*
 * This class acts as a shim between the old HTTP code (old cloaking), the
 * Client object, and later, the new HTTP code (admin system, new cloaking).
 */

class HTTPProtocolMgr : public HXProtocol
{
public:
    HTTPProtocolMgr(IHXSocket* pSock);
    virtual ~HTTPProtocolMgr(void);

    STDMETHOD(QueryInterface)   (REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)   (THIS);
    STDMETHOD_(UINT32,Release)  (THIS);

    virtual void        init(Process* proc, IHXSocket* pSock);
    virtual const char* versionStr(void) { return "HTTP/1.0"; }

private:
    INT32               m_nRefCount;

public:
    IHXTCPSocket*       m_pCtrl;    // Compatibility socket for old HTTP code
};

#endif  // _HTTPPROTMGR_H_
