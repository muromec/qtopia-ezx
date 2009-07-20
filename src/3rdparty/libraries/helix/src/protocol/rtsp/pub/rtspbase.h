/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspbase.h,v 1.18 2008/01/24 20:39:28 cdunn Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _RTSPBASE_H_
#define _RTSPBASE_H_

#include "rtspif.h"
#include "hxslist.h"

const UINT16 MAX_RTSP_MSG   = 4096;             // XXXBAB adjust this
#if defined(HELIX_FEATURE_EMBEDDED_UI)
const UINT32 MAX_QUEUE_SIZE = (UINT32)0x1ffff;
#else
const UINT16 MAX_QUEUE_SIZE = (UINT16)0x7fff;   // XXXGLENN adjust this
#endif
const UINT32 MAX_RECVBUF_SIZE = 1000000;

struct RTSPRequireOptions
{
    const char* pcharOption;
    const char* pcharMessagesSupporting;
    UINT32 ulMsgsSupLen;
};

struct RTSPAcceptEncodingOptions
{
    const char* pcharOption;
    const char* pcharMessagesSupporting;
    UINT32 ulMsgsSupLen;
};

struct RTSPTransportMimeType
{
    RTSPTransportTypeEnum m_lTransportType;
    RTSPTransportSubTypeEnum m_lTransportSubType;
    const char* m_pMimeType;
};

class RTSPMessage;
class RTSPRequestMessage;
class RTSPResponseMessage;
class MIMEHeaderValue;



class RTSPBaseProtocol
{
public:
    RTSPBaseProtocol();
    ~RTSPBaseProtocol();

    enum { RTSP_REQUIRE_ENTITY = 0, RTSP_REQUIRE_LOADTEST, 
           RTSP_REQUIRE_AGGREGATE_TRANSPORT, m_NumRTSPRequireOptions };
    static const RTSPRequireOptions RTSPRequireOptionsTable[
        m_NumRTSPRequireOptions];

    enum { RTSP_ENCODING_MEI_DESCRIBE = 0, RTSP_ENCODING_MEI_SETUP,
        m_NumRTSPAcceptEncodingOptions };
    static const RTSPAcceptEncodingOptions RTSPAcceptEncodingOptionsTable[
        m_NumRTSPAcceptEncodingOptions];

    HXBOOL                isRequired
                        (
                            RTSPMessage* pRTSPMessageToSearch,
                            UINT32 ulOptionToFind
                        );
protected:

   

    HX_RESULT           enqueueMessage(RTSPMessage* pMsg);
    RTSPMessage*        dequeueMessage(UINT32 seqNo);
    void                clearMessages();
    virtual HX_RESULT   sendRequest(RTSPRequestMessage* pMsg, UINT32 seqNo);
    virtual HX_RESULT   sendRequest(RTSPRequestMessage* pMsg,
                                const char* pContent,
                                const char* pMimeType, UINT32 seqNo);
    RTSPResponseMessage* makeResponseMessage(UINT32 seqNo, const char* pErrNo);
    HX_RESULT           sendResponse(UINT32 seqNo, const char* pErrNo);
    HX_RESULT           sendResponse(RTSPResponseMessage* pMsg,
                                const char* pContent = 0,
                                const char* pMimeType = 0);
    const char*         getErrorText(const char* pErrNo);
    void                addRFC822Headers(RTSPMessage* pMsg,
                                IHXValues* pRFC822Headers);
    void                getRFC822Headers(RTSPMessage* pMsg,
                                REF(IHXValues*) pRFC822Headers);
    HX_RESULT           sendControlMessage(IHXBuffer* pBuffer);
    HX_RESULT           handleACK(IHXPacketResend* pPacketResend,
                                RTSPResendBuffer* pResendBuffer,
                                UINT16 uStreamNumber,
                                UINT16* pAckList, UINT32 uAckListCount,
                                UINT16* pNakList, UINT32 uNakListCount,
                                HXBOOL bIgnoreACK);
    RTPInfoEnum         parseRTPInfoHeader(MIMEHeaderValue* pSeqValue,
                                UINT16& streamID, UINT16& seqNum,
                                UINT32& ulTimestamp, const char*& pControl);
    virtual HX_RESULT   closeSocket() {return HXR_OK;}
    virtual HX_RESULT   reopenSocket() {return HXR_OK;}

    virtual void handleDebug(IHXBuffer* pMsgBuf, HXBOOL bInbound);
    virtual void handleDebug(const char* pMsg, HXBOOL bInBound);
    virtual void handleTiming(IHXBuffer* pMsgBuf, HXBOOL bInbound);

    IUnknown*                   m_pContext;
    IHXCommonClassFactory*      m_pCommonClassFactory;
    IHXSocket*                  m_pSocket;
    IHXBufferedSocket*		m_pFastSocket;
    UINT32                      m_uControlBytesSent;
    HXBOOL                      m_bConnectionlessControl;
    IHXBuffer*                  m_pControlBuffer;

    HXBOOL                      m_bMessageDebug;
    CHXString                   m_messageDebugFileName;

    
private:
    CHXSimpleList               m_msgQueue;
};

#endif /* _RTSPBASE_H_ */
