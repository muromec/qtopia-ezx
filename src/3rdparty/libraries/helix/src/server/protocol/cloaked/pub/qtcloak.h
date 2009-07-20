/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: qtcloak.h,v 1.6 2007/05/23 18:58:21 seansmith Exp $
 *
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _QTCLOAK_H_
#define _QTCLOAK_H_

#include "hxtypes.h"

class BaseCallback;
class CByteQueue;
class HTTPParser;
class CloakedHTTPResponse;
class CloakedHTTPRVResponse;
class CHXString;
class Dict;
class HTTPMessage;
class HTTPGetMessage;
class HTTPPostMessage;
class HTTPRequestMessage;
class HTTPResponseMessage;
class Process;
class HTTPProtocolMgr;

class CloakConn;
struct IHXBuffer;

class CQTCloakGETHandler : public CBaseCloakGETHandler
{
public:
    CQTCloakGETHandler(void);
    virtual ~CQTCloakGETHandler(void);

    STDMETHOD_(HTTP_TYPE, GetHandlerType)   (THIS);
    STDMETHOD_(void, OnDispatch)            (THIS);
    STDMETHOD_(void, OnRequest)             (THIS_ HTTPRequestMessage* pMsg);
};

class CQTCloakPOSTHandler : public CBaseCloakPOSTHandler
{
public:
    CQTCloakPOSTHandler(void);
    virtual ~CQTCloakPOSTHandler(void);

    // IHXHTTPDemuxResponse
    STDMETHOD_(HTTP_TYPE, GetHandlerType)   (THIS);
    STDMETHOD_(void, OnRequest)             (THIS_ HTTPRequestMessage* pMsg);
    STDMETHOD_(void, OnData)                (THIS_ IHXBuffer* pBuf);

    void        ProcessPendingData(void);

protected:
    UINT32      decodeBuf(BYTE* pData, UINT32 uDataLen, IHXBuffer*& rpDecBuf);
};

#endif  /* _QTCLOAK_H_ */
