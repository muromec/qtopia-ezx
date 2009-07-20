/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspbase.cpp,v 1.30 2008/05/12 20:35:03 jwei Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxengin.h"
#include "hxnet.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "safestring.h"
//#include "rtmlpars.h"
#include "mimehead.h"
#include "mimescan.h"
#include "timerep.h"
#include "rtspmsg.h"
#include "rtsppars.h"
#include "rtspmdsc.h"
#include "basepkt.h"
#include "servrsnd.h"
#include "rtspbase.h"
#include "dbcs.h" // for HXIsEqual
#include "hxtlogutil.h"
#include "hxescapeutil.h"  //HXEscapeUtil::EscapeSymbol

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

// XXXHP: NEED TO MODIFY THE SAME STRUCTURE AT DLLMAIN.CPP!!!
struct RTSPError
{
    const char* pErrNo;
    const char* pErrMsg;
};

//
// XXXBAB - turn this into a dictionary
//
// XXXHP: NEED TO REGENERATE ON WIN16 EVERYTIME THIS ARRAY
//        IS MODIFIED!!!
#ifdef _WIN16
extern RTSPError* RTSPErrorTable;
#else
static const RTSPError RTSPErrorTable[] =
{
    { "100", "Continue" },

    { "200", "OK" },
    { "201", "Created" },
    { "250", "Low On Storage Space" },

    { "300", "Multiple Choices" },
    { "301", "Moved Permanently" },
    { "302", "Moved Temporarily" },
    { "303", "See Other" },
    { "304", "Not Modified" },
    { "305", "Use Proxy" },

    { "400", "Bad Request" },
    { "401", "Unauthorized" },
    { "402", "Payment Required" },
    { "403", "Forbidden" },
    { "404", "Not Found" },
    { "405", "Method Not Allowed" },
    { "406", "Not Acceptable" },
    { "407", "Proxy Authentication Required" },
    { "408", "Request Time-out" },
    { "409", "Conflict" },
    { "410", "Gone" },
    { "411", "Length Required" },
    { "412", "Precondition Failed" },
    { "413", "Request Entity Too Large" },
    { "414", "Request-URI Too Large" },
    { "415", "Unsupported Media Type" },
    { "451", "Parameter Not Understood" },
    { "452", "Conference Not Found" },
    { "453", "Not Enough Bandwidth" },
    { "454", "Session Not Found" },
    { "455", "Method Not Valid In This State" },
    { "456", "Header Field Not Valid For Resource" },
    { "457", "Invalid Range" },
    { "458", "Parameter Is Read-Only" },
    { "459", "Aggregate Operation Not Allowed" },
    { "460", "Only Aggregate Operation Allowed" },
    { "461", "Unsupported Transport" },
    { "462", "Destination Unreachable" },
    { "500", "Internal Server Error" },
    { "501", "Not Implemented" },
    { "502", "Bad Gateway" },
    { "503", "Service Unavailable" },
    { "504", "Gateway Time-out" },
    { "505", "RTSP Version not supported" },
    { "551", "Option not supported" }
};
#endif

const RTSPRequireOptions RTSPBaseProtocol::RTSPRequireOptionsTable
    [RTSPBaseProtocol::m_NumRTSPRequireOptions] =
    {
        {"com.real.retain-entity-for-setup", "DESCRIBE", 8},
        {"com.real.load-test-password-enabled", "OPTIONS", 7},
        {"aggregate-transport", "DESCRIBE", 8}
    };

const RTSPAcceptEncodingOptions RTSPBaseProtocol::RTSPAcceptEncodingOptionsTable
    [RTSPBaseProtocol::m_NumRTSPAcceptEncodingOptions] =
    {
        {"mei", "DESCRIBE", 8},
        {"mei", "SETUP", 5}
    };





/*
 * RTSPBaseProtocol methods
 */

RTSPBaseProtocol::RTSPBaseProtocol():
    m_pContext(NULL),
    m_pCommonClassFactory(NULL),
    m_pSocket(NULL), // weak (no addref)
    m_pFastSocket(NULL),
    m_uControlBytesSent(0),
    m_bConnectionlessControl(FALSE),
    m_pControlBuffer(NULL),
    m_bMessageDebug(FALSE)
{
}

RTSPBaseProtocol::~RTSPBaseProtocol()
{
    clearMessages();

    HX_RELEASE(m_pControlBuffer);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pContext);
}


HX_RESULT
RTSPBaseProtocol::enqueueMessage(RTSPMessage* pMsg)
{
    m_msgQueue.AddTail(pMsg);
    return HXR_OK;
}

RTSPMessage*
RTSPBaseProtocol::dequeueMessage(UINT32 seqNo)
{
    LISTPOSITION pos = m_msgQueue.GetHeadPosition();
    while(pos)
    {
        RTSPMessage* pMsg = (RTSPMessage*)m_msgQueue.GetAt(pos);
        if(pMsg->seqNo() == seqNo)
        {
            (void)m_msgQueue.RemoveAt(pos);
            return pMsg;
        }
        (void)m_msgQueue.GetNext(pos);
    }
    return 0;
}

void
RTSPBaseProtocol::clearMessages()
{
    LISTPOSITION pos = m_msgQueue.GetHeadPosition();
    while(pos)
    {
        RTSPMessage* pMsg = (RTSPMessage*)m_msgQueue.GetNext(pos);
        delete pMsg;
    }
}

const char*
RTSPBaseProtocol::getErrorText(const char* pErrNo)
{
    int tabSize = sizeof(RTSPErrorTable) / sizeof(RTSPErrorTable[0]);
    for (int i = 0; i < tabSize; ++i)
    {
        if (strcmp(pErrNo, RTSPErrorTable[i].pErrNo) == 0)
        {
            return RTSPErrorTable[i].pErrMsg;
        }
    }
    return "";
}

HX_RESULT
RTSPBaseProtocol::sendRequest(RTSPRequestMessage* pMsg,
    const char* pContent, const char* pMimeType, UINT32 seqNo)
{
    if(pContent)
    {
        char tmpBuf[32];
        pMsg->addHeader("Content-type", pMimeType);
        SafeSprintf(tmpBuf, 32,"%d", strlen(pContent));
        pMsg->addHeader("Content-length", tmpBuf);
        pMsg->setContent(pContent);
    }
    return sendRequest(pMsg, seqNo);
}

HX_RESULT
RTSPBaseProtocol::sendRequest(RTSPRequestMessage* pMsg, UINT32 seqNo)
{
    HX_RESULT retVal = HXR_OK;
    // set sequence number
    char seqBuf[32];
    SafeSprintf(seqBuf, 32, "%ld", seqNo);
    pMsg->addHeader("CSeq", seqBuf, TRUE);      // add to head of list
    pMsg->setSeqNo(seqNo);
    enqueueMessage(pMsg);
    CHXString msgStr = pMsg->asString();

    IHXBuffer* pBuffer = NULL;
    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (BYTE*)(const char*)msgStr, 
				        msgStr.GetLength(), m_pContext))
    {
	retVal = sendControlMessage(pBuffer);
	HX_RELEASE(pBuffer);
    }
    else
    {
        retVal = HXR_OUTOFMEMORY;
    }
    return retVal;
}

void
RTSPBaseProtocol::handleDebug(IHXBuffer* pMsgBuf, HXBOOL bInBound)
{
    char*   pszMsg = NULL;
    UINT32  ulMsgBuf = 0;

    if (pMsgBuf)
    {
        ulMsgBuf = pMsgBuf->GetSize() + 1;        
        
        pszMsg = new char[ulMsgBuf];
        if (pszMsg)
        {
            memset(pszMsg, 0, ulMsgBuf);
            memcpy(pszMsg, pMsgBuf->GetBuffer(), ulMsgBuf-1);

            handleDebug(pszMsg, bInBound);
        }        
        HX_VECTOR_DELETE(pszMsg);
    }
}

void
RTSPBaseProtocol::handleDebug(const char* pszMsg, HXBOOL bInBound)
{
    CHXString pMsg;
    char szMsg[MAX_DISPLAY_NAME];

    if (pszMsg)
    {
        if(bInBound)
        {
            SafeSprintf(szMsg, MAX_DISPLAY_NAME, "RTSPClientProtocol[%p]\nIN:\n", this); 
        }
        else
        {
            SafeSprintf(szMsg, MAX_DISPLAY_NAME, "RTSPClientProtocol[%p]\nOUT:\n", this); 
        }
        pMsg = szMsg;
        pMsg += pszMsg;

        if(m_bMessageDebug && m_messageDebugFileName)
        {
            FILE* fp = fopen(m_messageDebugFileName, "a");
            if(!fp)
            {
                return;
            }

            fprintf(fp, "%s\n", (const char*)pMsg);
            fclose(fp);
        }

        CHXString pMsgString = HXEscapeUtil::EscapeSymbol(pMsg, '%');
        HXLOGL2(HXLOG_RTSP, pMsgString);
    }
}

void
RTSPBaseProtocol::handleTiming(IHXBuffer* pMsgBuf, HXBOOL bInbound)
{
    // Only implemented by subclasses in server modules
}

RTSPResponseMessage*
RTSPBaseProtocol::makeResponseMessage(UINT32 seqNo, const char* pErrNo)
{
    RTSPResponseMessage* pMsg = new RTSPResponseMessage;

    // set sequence number
    char seqBuf[32];
    SafeSprintf(seqBuf,32,"%ld", seqNo);
    pMsg->addHeader("CSeq", seqBuf, TRUE);      // add to head of list
    pMsg->setSeqNo(seqNo);

    pMsg->setErrorCode(pErrNo);
    pMsg->setErrorMsg(getErrorText(pErrNo));
    UTCTimeRep utcNow;
    pMsg->addHeader("Date", utcNow.asRFC1123String());
    return pMsg;
}

HX_RESULT
RTSPBaseProtocol::sendResponse(UINT32 seqNo, const char* pErrNo)
{
    RTSPResponseMessage* pMsg = makeResponseMessage(seqNo, pErrNo);
    HX_RESULT err = sendResponse(pMsg);
    delete pMsg;
    return err;
}

HX_RESULT
RTSPBaseProtocol::sendResponse(
    RTSPResponseMessage* pMsg, const char* pContent, const char* pMimeType)
{
    if (pContent != NULL)
    {
        char tmpBuf[32];
        pMsg->addHeader("Content-type", pMimeType);
        SafeSprintf(tmpBuf,32, "%d", strlen(pContent));  // only for strings
        pMsg->addHeader("Content-length", tmpBuf);
        pMsg->setContent(pContent);
    }

    CHXString msgStr = pMsg->asString();

    IHXBuffer* pBuffer = NULL;
    if (HXR_OK == CreateAndSetBufferCCF(pBuffer, (BYTE*)(const char*)msgStr, 
					msgStr.GetLength(), m_pContext))
    {
	sendControlMessage(pBuffer);
	HX_RELEASE(pBuffer);
    }
    return HXR_OK;
}

void
RTSPBaseProtocol::addRFC822Headers(RTSPMessage* pMsg,
                                   IHXValues* pRFC822Headers)
{
    HX_RESULT         res;
    MIMEHeader*       pMimeHeader;
    const char*       pName = NULL;
    IHXBuffer*       pValue = NULL;
    IHXKeyValueList* pKeyedHdrs;

    if (!pRFC822Headers)
    {
        return;
    }

    // Find out if the IHXValues supports IHXKeyValueList
    // XXX showell - eventually, we should just make all callers
    // give us an IHXKeyValueList, since it's more efficient,
    // and so we don't overwrite duplicate headers.
    res = pRFC822Headers->QueryInterface(IID_IHXKeyValueList,
                                         (void**) &pKeyedHdrs);

    if (res == HXR_OK)
    {
        IHXKeyValueListIter* pListIter = NULL;
        pKeyedHdrs->GetIter(pListIter);
        HX_ASSERT(pListIter);

        while (pListIter->GetNextPair(pName, pValue) == HXR_OK)
        {
            pMimeHeader = new MIMEHeader(pName);
            pMimeHeader->addHeaderValue((char*)pValue->GetBuffer());
            pMsg->addHeader(pMimeHeader);
            HX_RELEASE(pValue);
        }
        HX_RELEASE(pListIter);
    }
    else
    {
        res = pRFC822Headers->GetFirstPropertyCString(pName, pValue);
        while (res == HXR_OK)
        {
            pMimeHeader = new MIMEHeader(pName);
            pMimeHeader->addHeaderValue((char*)pValue->GetBuffer());
            pMsg->addHeader(pMimeHeader);
            pValue->Release();

            res = pRFC822Headers->GetNextPropertyCString(pName, pValue);
        }
    }

    HX_RELEASE(pKeyedHdrs);
}

void
RTSPBaseProtocol::getRFC822Headers(RTSPMessage* pMsg,
    REF(IHXValues*) pRFC822Headers)
{
    MIMEHeader*         pHeader = NULL;
    IUnknown*           pUnknown = NULL;
    IHXKeyValueList*    pList = NULL;

    pRFC822Headers = NULL;

    if (!m_pCommonClassFactory)
    {
        goto cleanup;
    }

    if (HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXKeyValueList,
                                                        (void**) &pUnknown))
    {
        goto cleanup;
    }

    if (HXR_OK != pUnknown->QueryInterface(IID_IHXKeyValueList,
                                           (void**) &pList))
    {
        goto cleanup;
    }

    pHeader = pMsg->getFirstHeader();

    while (pHeader)
    {
        MIMEHeaderValue* pHeaderValue;

        /*
         * XXX...There is way too much memcpy() going on here
         */

        pHeaderValue = pHeader->getFirstHeaderValue();

        CHXString HeaderString;

        while (pHeaderValue)
        {
            CHXString TempString;

            pHeaderValue->asString(TempString);
            HeaderString += TempString;
            pHeaderValue = pHeader->getNextHeaderValue();
            if (pHeaderValue)
            {
                HeaderString += ", ";
            }
        }

        IHXBuffer *pBuffer = NULL;
        CHXBuffer::FromCharArray((const char*) HeaderString, &pBuffer);

        pList->AddKeyValue(pHeader->name(), pBuffer);

        HX_RELEASE(pBuffer);

        pHeader = pMsg->getNextHeader();
    }

    // XXX showell - Yet another item for rnvalues cleanup phase II.  We should
    // just change this function so its callers don't expect IHXValues, since
    // the IHXKeyValueList interface is better for header data.
    if (HXR_OK != pList->QueryInterface(IID_IHXValues,
                                        (void**) &pRFC822Headers))
    {
        pRFC822Headers = NULL;
    }

cleanup:

    HX_RELEASE(pList);
    HX_RELEASE(pUnknown);
}

HX_RESULT
RTSPBaseProtocol::sendControlMessage(IHXBuffer* pBuffer)
{
    HX_RESULT hxr = HXR_OK;

    handleDebug(pBuffer, FALSE);
    handleTiming(pBuffer, FALSE);

    if (!m_pSocket)
    {
        m_pControlBuffer = pBuffer;
        m_pControlBuffer->AddRef();
        hxr = reopenSocket();
    }
    else
    {
        m_uControlBytesSent += pBuffer->GetSize();
        if (m_pFastSocket)
        {
            hxr = m_pFastSocket->BufferedWrite(pBuffer);
            m_pFastSocket->FlushWrite();
        }
        else
        {
	    hxr = m_pSocket->Write(pBuffer);
	}
    }

    return hxr;
}


HX_RESULT
RTSPBaseProtocol::handleACK(IHXPacketResend* pPacketResend,
                            RTSPResendBuffer* pResendBuffer,
                            UINT16 uStreamNumber,
                            UINT16* pAckList,
                            UINT32 uAckListCount,
                            UINT16* pNakList,
                            UINT32 uNakListCount,
                            HXBOOL bIgnoreACK)
{
    if (!pResendBuffer)
    {
        return HXR_UNEXPECTED;
    }

    /*
     * ACKs and NAKs only have meaning for resend buffers
     *
     * NOTE: keep the ACKing/NAKing in order by starting at the back of
     *       the lists
     */


    INT32 i;

    if (!bIgnoreACK)
    {
        for (i = (INT32)(uAckListCount - 1); i >= 0; i--)
        {
            pResendBuffer->Remove(pAckList[i]);
        }
    }


    if (uNakListCount)
    {
        //XXXGH...must be BasePacket
        BasePacket** ppPacket = new BasePacket*[uNakListCount + 1];

        /*
         * Only allow 10 packets to be resent
         */

        UINT16 j = 0;
        for (i = (INT32)(uNakListCount - 1); i >= 0 && j < 10; i--)
        {
            BasePacket* pPacket;

            pPacket = pResendBuffer->Find(pNakList[i], TRUE);

            if (pPacket)
            {
                ppPacket[j++] = pPacket;
                pPacket->AddRef();
            }
        }
        ppPacket[j] = 0;

        //XXX..will the BasePacket have the stream number in it?
        pPacketResend->OnPacket(uStreamNumber, ppPacket);

        BasePacket* pReleasePacket = NULL;
        BasePacket** ppReleasePacket = ppPacket;

        for (; (pReleasePacket = *ppReleasePacket); ppReleasePacket++)
        {
            HX_RELEASE(pReleasePacket);
        }

        HX_VECTOR_DELETE(ppPacket);
    }

    return HXR_OK;
}

/*
 *  The grammer in RFC2326, I think, is wrong....But in any case, there is no
 *  gurantee seq_no and rtptime are present in RTP-Info.
 */
RTPInfoEnum
RTSPBaseProtocol::parseRTPInfoHeader(
    MIMEHeaderValue* pSeqValue, UINT16& streamID, UINT16& seqNum,
    UINT32& ulTimestamp, const char*& pControl)
{
    HXBOOL bFoundSeqNo = FALSE;
    HXBOOL bFoundRTPTime = FALSE;

    MIMEParameter* pParam = pSeqValue->getFirstParameter();
    while (pParam != NULL)
    {
        if(pParam->m_attribute == "url")
        {
            // Note: We don't currently do anything with the first section
            // of the "url" attribute (the actual player-requested URL). If
            // we ever do, please note that all ';' characters were escaped
            // by the server when this message was created, because ';' has
            // special meaning as a delimiter in this message. Remember to
            // unescape all instances of "%3b" to ";" before using the URL.

            const char* pUrl = (const char*) pParam->m_value;
            const char* pEq  = strrchr(pUrl, '=');
            if (pEq != NULL)
            {
                streamID = (UINT16)strtol(pEq + 1, 0, 10);
            }

            // take the control string...
            pControl = pUrl;
        }
        else if(pParam->m_attribute == "seq")
        {
            bFoundSeqNo = TRUE;
            seqNum = (UINT16)strtol((const char*)pParam->m_value,0,10);
        }
        else if(pParam->m_attribute == "rtptime")
        {
            bFoundRTPTime = TRUE;
            ulTimestamp = (UINT32)strtoul((const char*)pParam->m_value, 0, 10);
        }

        pParam = pSeqValue->getNextParameter();
    }

    if (bFoundSeqNo)
    {
        if (bFoundRTPTime)
        {
            return RTPINFO_SEQ_RTPTIME;
        }

        return RTPINFO_SEQ;
    }

    if (bFoundRTPTime)
    {
        return RTPINFO_RTPTIME;
    }

    return RTPINFO_EMPTY;
}

HXBOOL
RTSPBaseProtocol::isRequired(RTSPMessage* pRTSPMessageToSearch,
                             UINT32 ulOptionToFind)
{
    HXBOOL bRetainState = FALSE;
    MIMEHeader* pMIMEHeaderRequire;
    pMIMEHeaderRequire = pRTSPMessageToSearch->getHeader("Require");

    // If a require header was found
    if (pMIMEHeaderRequire != NULL)
    {
        MIMEHeaderValue* pMIMEHeaderValueRequire =
            pMIMEHeaderRequire->getFirstHeaderValue();

        while (pMIMEHeaderValueRequire != NULL)
        {
            if (!strcasecmp(RTSPRequireOptionsTable[ulOptionToFind].pcharOption,
                            pMIMEHeaderValueRequire->value()))
            {
                bRetainState = TRUE;
            }

            pMIMEHeaderValueRequire = pMIMEHeaderRequire->getNextHeaderValue();
        }

    }

    return bRetainState;
}
