/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprot.h,v 1.19 2007/05/23 18:53:06 seansmith Exp $
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

#ifndef _HXPROT_H_
#define _HXPROT_H_

class  Client;
class  Process;
class  CHXSimpleList;
struct IHXPacket;
struct IHXSocket;
struct IHXValues;
class  Transport;

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "srcerrs.h"
#include "clientsession.h"

class HXProtocol
{
public:
    HXProtocol(void);
    virtual ~HXProtocol(void);

    inline Client* client(void) { return m_pClient; }

    virtual void        init(Process* proc, IHXSocket* pSock) = 0;
    virtual int         setupHeader(IHXValues* pHeader,
                                    ClientSession* pSession,
                                    HX_RESULT status);
    virtual int         addToHeader(const char* pName, IHXBuffer* pValue)
                        {
                            return -1;
                        };
    virtual int         setupStreams(CHXSimpleList* headers,
                                     ClientSession* pSession,
                                     HX_RESULT result);
    virtual int         playDone(const char* pSessionID);
    virtual int         disconnect(const char* pSessionID);
    virtual int         sendAlert(const char* pSessionID, StreamError err);
    virtual int         sendAlert(const char* pSessionID, IHXBuffer* pAlert);
    virtual int         sendRedirect(const char* pSessionID,
                                     const char* pURL,
                                     UINT32 ulSecsFromNow);
    virtual int         sendProxyRedirect(const char* pSessionID,
                                     const char* pURL) { return HXR_NOTIMPL; }
    virtual Transport*  getTransport(ClientSession* pSession,
                                     UINT16 streamNumber, UINT32 bIsReliable);
    virtual UINT32      controlBytesSent();
    /*
     * The following 3 routines are included so that the RTSPProtocol can
     * be cleaned up properly
     */

    STDMETHOD_(UINT32,AddRef)   (THIS) PURE;
    STDMETHOD_(UINT32,Release)  (THIS) PURE;
    virtual void                Done(HX_RESULT status);

    virtual HX_RESULT           SendSetParam(const char* url,
                                        const char* pSessionID,
                                        const char* pName,
                                        const char* pValue,
                                        const char* pMimeType,
                                        const char* pContent)
                                {
                                    return HXR_NOTIMPL;
                                }

    virtual void        SessionDone(const char* sessionID) {}

    virtual void        SetScaleDone(HX_RESULT status,
                                     ClientSession* pSession,
                                     FIXED32 fScale) {}

    virtual void        SeekByPacketDone(HX_RESULT status, UINT32 ulStartingTimestamp) {}

    virtual HX_RESULT   SetStreamStartTime(const char* pszSessionID,
                                           UINT32 ulStreamNum,
                                           UINT32 ulTimestamp) { return HXR_NOTIMPL; }

    virtual void        SetStatus(UINT32 ulCode) {}

protected:
    void                dumpMsg(const Byte* pMsg, int nMsgLen);

    IHXSocket*          m_pSock;
    Client*             m_pClient;
    Process*            m_proc;
};

#endif /* _HXPROT_H_ */
