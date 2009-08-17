/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rn1cloak.h,v 1.8 2005/07/23 01:58:27 darrick Exp $
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

#ifndef _RN1CLOAK_H_
#define _RN1CLOAK_H_

#include "hxtypes.h"
#include "basecloak.h"

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
class Player;
class Process;
class HTTPProtocolMgr;
class CloakedGUIDDict;
class CloakConn;

struct IHXBuffer;


#define GUID_STR_LENGTH                     36


class CRN1CloakGETHandler : public CBaseCloakGETHandler
{
public:
    CRN1CloakGETHandler();
    ~CRN1CloakGETHandler();

    // IHXHTTPDemuxResponse
    STDMETHOD_(HTTP_TYPE, GetHandlerType)   (THIS);
    STDMETHOD_(void, OnDispatch)            (THIS);
    STDMETHOD_(void, OnRequest)             (THIS_ HTTPRequestMessage* pMsg);
    STDMETHOD_(void, OnClosed)              (THIS);
 
    virtual BOOL        IsMultiPOST(void) { return m_bIsMultiPOST; }

    void                PostReceived(void);
    void                PostTimeout(void);

    class CloakGETShutdownCallback : public BaseCallback
    {
    public:
        CloakGETShutdownCallback(CRN1CloakGETHandler* pCGH)
        : m_pCGH(pCGH)
        {
            m_pCGH->AddRef();
        }
        ~CloakGETShutdownCallback();
        STDMETHOD(Func) (THIS);

        CRN1CloakGETHandler* m_pCGH;
    };

    class PostCallback : public BaseCallback
    {
    public:
        PostCallback(CRN1CloakGETHandler* pOwner, Process* pProc);
        virtual ~PostCallback(void);
        STDMETHOD(Func) (THIS);

    private:
        CRN1CloakGETHandler*    m_pOwner;
        Process*                m_pProc;
    };

protected:

    virtual HX_RESULT           ParseCloakSessionId(const char* szBuf);

    PostCallback*               m_pPostCB;
    UINT32                      m_ulPostCbId;
    BOOL                        m_bIsMultiPOST;
};

class CRN1CloakPOSTHandler : public CBaseCloakPOSTHandler
{
public:
    CRN1CloakPOSTHandler(void);

    // IHXHTTPDemuxResponse
    STDMETHOD_(HTTP_TYPE, GetHandlerType)   (THIS);
    STDMETHOD_(void, OnDispatch)            (THIS);
    STDMETHOD_(void, OnRequest)             (THIS_ HTTPRequestMessage* pMsg);
    STDMETHOD_(void, OnData)                (THIS_ IHXBuffer* pBuf);

protected:

    virtual HX_RESULT           ParseCloakSessionId(const char* szBuf);
    virtual HX_RESULT           SetupPOSTChannel(IHXBuffer* pBuf);
    virtual HX_RESULT           HandlePOSTData(IHXBuffer* pBuf);

    UINT32      decodeBuf(BYTE* pData, UINT32 uDataLen, IHXBuffer*& rpDecBuf, BOOL& rbPostDone);

    enum parsestate_t { PARSE_NONE, PARSE_GUID, PARSE_DATA };

protected:

    parsestate_t                m_parse_state;
};

#endif  /* _RN1CLOAK_H_ */
