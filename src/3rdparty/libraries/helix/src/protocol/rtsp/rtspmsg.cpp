/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspmsg.cpp,v 1.13 2008/10/20 21:29:41 demiurgo Exp $
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

//#include "hlxclib/stdlib.h"
//#include "hlxclib/stdio.h"

#include "hxcom.h"
#include "hxstring.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "hxtypes.h"

#include "smpte.h"
#include "nptime.h"

#include "mimehead.h"
#include "rtspmsg.h"

#include "ihxpckts.h"
#include "hxauth.h"

#include "chxpckts.h"
#include "pckunpck.h"
#include "hxbuffer.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

RTSPRange::RTSPRange(UINT32 begin, UINT32 end, RangeType rType):
    m_rangeType(rType),
    m_begin(begin),
    m_end(end)
{
    if(m_rangeType == TR_SMPTE)
    {
	SMPTETimeCode tBegin(m_begin);
	SMPTETimeCode tEnd(m_end);
	char tmpBuf[80]; /* Flawfinder: ignore */

	if(m_begin != RTSP_PLAY_RANGE_BLANK && 
	   m_end == RTSP_PLAY_RANGE_BLANK)
	{
	    SafeSprintf(tmpBuf, 80, "%s-", (const char*)tBegin);
	}
	else if(m_begin != RTSP_PLAY_RANGE_BLANK && 
		m_end != RTSP_PLAY_RANGE_BLANK)
	{
	    SafeSprintf(tmpBuf, 80, "%s-%s", (const char*)tBegin, (const char*)tEnd);
	}
	else if(m_begin == RTSP_PLAY_RANGE_BLANK && 
	        m_end != RTSP_PLAY_RANGE_BLANK)
	{
	    SafeSprintf(tmpBuf, 80, "-%s", (const char*)tEnd);
	}
	else if(m_begin == RTSP_PLAY_RANGE_BLANK &&
	        m_end == RTSP_PLAY_RANGE_BLANK)
	{
	    strcpy(tmpBuf, "-"); /* Flawfinder: ignore */
	}
	else if(m_begin == 0 && m_end == 0)
	{
	    SafeSprintf(tmpBuf, 80, "0-0"); 
	}
	else
	{
	    tmpBuf[0] = 0;
	}
	addParameter(tmpBuf);
    }
    else if(m_rangeType == TR_NPT)
    {
	NPTime tBegin((INT32)m_begin);
	NPTime tEnd((INT32)m_end);
	char tmpBuf[80]; /* Flawfinder: ignore */

	if(m_begin != RTSP_PLAY_RANGE_BLANK && 
	   m_end == RTSP_PLAY_RANGE_BLANK)
	{
	    SafeSprintf(tmpBuf, 80, "%s-", (const char*)tBegin);
	}
	else if(m_begin != RTSP_PLAY_RANGE_BLANK && 
		m_end != RTSP_PLAY_RANGE_BLANK)
	{
	    SafeSprintf(tmpBuf, 80, "%s-%s", (const char*)tBegin, (const char*)tEnd);
	}
	else if(m_begin == RTSP_PLAY_RANGE_BLANK && 
	        m_end != RTSP_PLAY_RANGE_BLANK)
	{
	    SafeSprintf(tmpBuf, 80, "-%s", (const char*)tEnd);
	}
	else if(m_begin == RTSP_PLAY_RANGE_BLANK &&
	        m_end == RTSP_PLAY_RANGE_BLANK)
	{
	    SafeSprintf(tmpBuf, 80, "-"); 
	}
	else if(m_begin == 0 && m_end == 0)
	{
	    SafeSprintf(tmpBuf, 80, "0-0"); 
	}
	else
	{
	    tmpBuf[0] = 0;
	}
	addParameter(tmpBuf);
    }
    else if(m_rangeType == TR_CLOCK)
    {
    }
}

void
RTSPRange::asString(CHXString& str)
{
    MIMEParameter* pParam = getFirstParameter();
    if(pParam)
    {
	if(m_rangeType == TR_SMPTE)
	{
	    str = CHXString("smpte=") + pParam->m_attribute;
	}
	else if(m_rangeType == TR_NPT)
	{
	    str = CHXString("npt=") + pParam->m_attribute;
	}
    }
    else
    {
	str = "";
    }
}

CHXString
RTSPRange::asString()
{
    CHXString str;
    asString(str);
    return str;
}


/*
 * RTSPAuthentication methods
 */

RTSPAuthentication::RTSPAuthentication(const char* authString,
    AuthenticationType authType):
    m_authType(authType),
    m_authString(authString),
    m_authValues(NULL)
{
}

RTSPAuthentication::RTSPAuthentication(IHXValues* pAuth):
    m_authType(AU_DIGEST),
    m_authValues(pAuth)
{
    if(m_authValues)
	m_authValues->AddRef();
}

RTSPAuthentication::~RTSPAuthentication()
{
    if(m_authValues)
	m_authValues->Release();
}

void
RTSPAuthentication::asString(CHXString& str)
{
    if(m_authType == AU_HX_PRIVATE)
    {
	str = CHXString("HXPrivate nonce=\"") + m_authString + "\"";
    }
    else if(m_authType == AU_DIGEST)
    {
	UINT32 subAuthType;

	if(HXR_OK != m_authValues->GetPropertyULONG32("AuthType", subAuthType))
	{
	    str = "";
	    return;
	}

	switch(subAuthType)
	{
	    case HX_AUTH_DIGEST:
	    {
		IHXBuffer* pRealm = 0;
		IHXBuffer* pNonce = 0;
		IHXBuffer* pOpaque= 0;
		
		if(HXR_OK == m_authValues->GetPropertyCString("Realm", pRealm) &&
		   HXR_OK == m_authValues->GetPropertyCString("Nonce", pNonce) &&
		   HXR_OK == m_authValues->GetPropertyCString("Opaque", pOpaque))
		{
	
		    str = CHXString("Digest realm=\"") + 
			pRealm->GetBuffer() + "\",\r\n" +
			"    nonce=" + pNonce->GetBuffer() + ",\r\n" +
			"    opaque=" + pOpaque->GetBuffer();
		}
		if(pRealm) pRealm->Release();
		if(pNonce) pNonce->Release();
		if(pOpaque) pOpaque->Release();
		break;
	    }
	    case HX_AUTH_BASIC:
	    {
		IHXBuffer* pRealm = 0;
		IHXBuffer* pUserName = 0;
		IHXBuffer* pPassword = 0;
		IHXBuffer* pResponse = 0;
		if(HXR_OK == m_authValues->GetPropertyCString("Realm", pRealm))
		{
		    str = CHXString("Basic realm=\"") + 
			pRealm->GetBuffer() + "\"";
		}
		else if(HXR_OK == m_authValues->GetPropertyCString(
		    "Response", pResponse))
		{
		    str = CHXString("Basic ") + pResponse->GetBuffer();
		}
		else if(HXR_OK == m_authValues->GetPropertyCString(
		                                   "UserName", pUserName) &&
		    HXR_OK == m_authValues->GetPropertyCString("Password",
							       pPassword))
		{
		    str = CHXString(pUserName->GetBuffer()) + 
			":" + pPassword->GetBuffer();
		}
		if(pUserName) pUserName->Release();
		if(pPassword) pPassword->Release();
		if(pResponse) pResponse->Release();
		if(pRealm) pRealm->Release();
		break;
	    }
	    default:
		str = "";
		break;
	}
    }
    else
    {
	str = "";
    }
}

CHXString
RTSPAuthentication::asString()
{
    CHXString str;
    asString(str);
    return str;
}

RTSPDigestAuthorization::RTSPDigestAuthorization(IHXValues* pValues):
    m_values(pValues)
{
    if(m_values)
	m_values->AddRef();
}

void
RTSPDigestAuthorization::asString(CHXString& str)
{
    char buf[1024]; /* Flawfinder: ignore */
    IHXBuffer* strBuf;

    INT32 lBytes = 0;
    lBytes += SafeSprintf(buf, 1024, "Digest ");
    if(HXR_OK == m_values->GetPropertyCString("UserName", strBuf))
    {
	lBytes += SafeSprintf(buf + lBytes, (UINT32)(1024 - lBytes), "username=\"%s\",", strBuf->GetBuffer());
	strBuf->Release();
    }
    if(HXR_OK == m_values->GetPropertyCString("Realm", strBuf))
    {
	lBytes += SafeSprintf(buf + lBytes, (UINT32)(1024 - lBytes), "realm=\"%s\",", strBuf->GetBuffer());
	strBuf->Release();
    }
    if(HXR_OK == m_values->GetPropertyCString("Response", strBuf))
    {
	lBytes += SafeSprintf(buf + lBytes, (UINT32)(1024 - lBytes), "response=\"%s\",", strBuf->GetBuffer());
	strBuf->Release();
    }
    if(HXR_OK == m_values->GetPropertyCString("URI", strBuf))
    {
	lBytes += SafeSprintf(buf + lBytes, (UINT32)(1024 - lBytes), "uri=\"%s\",", strBuf->GetBuffer());
	strBuf->Release();
    }
    if(HXR_OK == m_values->GetPropertyCString("UserName", strBuf))
    {
	lBytes += SafeSprintf(buf + lBytes, (UINT32)(1024 - lBytes), "nonce=\"%s\",", strBuf->GetBuffer());
	strBuf->Release();
    }
    if(HXR_OK == m_values->GetPropertyCString("UserName", strBuf))
    {
	lBytes += SafeSprintf(buf + lBytes, (UINT32)(1024 - lBytes), "opaque=\"%s\",", strBuf->GetBuffer());
	strBuf->Release();
    }
    char* comma = strrchr(buf, ',');
    if(comma)
	*comma = 0;

    str = CHXString(buf);
}

CHXString
RTSPDigestAuthorization::asString()
{
    CHXString str;
    asString(str);
    return str;
}


/*
 * RTSPPEPInfo methods
 */

RTSPPEPInfo::RTSPPEPInfo(HXBOOL bStrengthMust):
    m_bStrengthMust(bStrengthMust)
{
}

RTSPPEPInfo::~RTSPPEPInfo()
{
}

void
RTSPPEPInfo::asString(CHXString& str)
{
    if(m_bStrengthMust)
    {
	str = "{ {strength must} }";
    }
    else
    {
	str = "{ {strength may} }";
    }
}

CHXString
RTSPPEPInfo::asString()
{
    CHXString str;
    asString(str);
    return str;
}


// static definitions

const int RTSPMessage::MAJ_VERSION = 1;
const int RTSPMessage::MIN_VERSION = 0;

RTSPMessage::RTSPMessage():
    m_seqNo(0)
{
    setVersion(MAJ_VERSION, MIN_VERSION);
}

RTSPMessage::~RTSPMessage()
{
    clearHeaderList();
}

void
RTSPMessage::addHeader(MIMEHeader* pHeader, HXBOOL bAtHead)
{
    if(bAtHead)
    {
	m_headers.AddHead(pHeader);
    }
    else
    {
	m_headers.AddTail(pHeader);
    }
}

void 
RTSPMessage::addHeader(const char* pName, const char* pValue, HXBOOL bAtHead)
{
    MIMEHeader* pHeader = new MIMEHeader(pName);
    pHeader->addHeaderValue(pValue);
    addHeader(pHeader, bAtHead);
}

MIMEHeader*
RTSPMessage::getHeader(const char* pName)
{
    LISTPOSITION pos = m_headers.GetHeadPosition();
    while(pos)
    {
	MIMEHeader* pHeader = (MIMEHeader*)m_headers.GetNext(pos);
	if(strcasecmp(pHeader->name(), pName) == 0)
	{
	    return pHeader;
	}
    }
    return 0;
}

CHXString
RTSPMessage::getHeaderValue(const char* pName)
{
    MIMEHeader* pHeader = getHeader(pName);
    if(pHeader)
    {
	MIMEHeaderValue* pHeaderValue = pHeader->getFirstHeaderValue();
	if(pHeaderValue)
	{
	    CHXString str = pHeaderValue->value();
	    return str;
	}
    }
    return CHXString("");
}

int
RTSPMessage::getHeaderValue(const char* pName, UINT32& value)
{
    CHXString strValue = getHeaderValue(pName);
    if(strValue.GetLength() > 0)
    {
	value = (UINT32)strtol((const char*)strValue, 0, 10);
	return 1;
    }
    return 0;
}

MIMEHeader*
RTSPMessage::getFirstHeader()
{
    m_listpos = m_headers.GetHeadPosition();
    if(m_listpos)
	return (MIMEHeader*)m_headers.GetNext(m_listpos);
    return 0;
}

MIMEHeader*
RTSPMessage::getNextHeader()
{
    if(m_listpos)
	return (MIMEHeader*)m_headers.GetNext(m_listpos);
    return 0;
}

void
RTSPMessage::clearHeaderList()
{
    MIMEHeader* pHeader = getFirstHeader();
    while(pHeader)
    {
	delete pHeader;
	pHeader = getNextHeader();
    }
}

int
RTSPMessage::contentLength()
{
    return (int)m_content.GetLength();
}

HX_RESULT 
RTSPMessage::AsValues(REF(IHXValues*) pIHXValuesHeaders, IUnknown* pContext)
{
    HX_RESULT rc = HXR_OK;

    if (!pContext)
    {
	return HXR_INVALID_PARAMETER;
    }

    rc = CreateValuesCCF(pIHXValuesHeaders, pContext);
    if (HXR_OK == rc)
    {
	IHXBuffer* pIHXBufferValue = NULL;
	MIMEHeader* pMIMEHeaderCurrent = getFirstHeader();
	CHXString CHXStringValue;

	while(pMIMEHeaderCurrent)
	{
	    CHXStringValue.Empty();
	    pMIMEHeaderCurrent->asString(CHXStringValue);

	    CHXBuffer::FromCharArray
	    (
		CHXStringValue,
		&pIHXBufferValue
	    );

	    pIHXValuesHeaders->SetPropertyCString
	    (
		pMIMEHeaderCurrent->name(),
		pIHXBufferValue
	    );

	    HX_RELEASE(pIHXBufferValue);

	    pMIMEHeaderCurrent = getNextHeader();
	}
    }

    return rc;
}

CHXString
RTSPRequestMessage::asString()
{
	int lenTmpBuf = (int)(strlen(tagStr()) + strlen(url()) + 80);
    char* pTmpBuf = new char[lenTmpBuf];
    SafeSprintf(pTmpBuf, (UINT32)lenTmpBuf, "%s %s RTSP/%d.%d\r\n",
	tagStr(), url(), majorVersion(), minorVersion());

    CHXString msgStr = pTmpBuf;
    delete[] pTmpBuf;

    MIMEHeader* pHeader = getFirstHeader();
    while(pHeader)
    {
	msgStr += pHeader->name();
	msgStr += ": ";
	pHeader->asString(msgStr);
	pHeader = getNextHeader();
    }
    msgStr += "\r\n";
    if(contentLength() > 0)
    {
	msgStr += getContent();
    }
    return msgStr;
}

void
RTSPRequestMessage::asString(char* pBuf, int& msgLen)
{
    CHXString msgStr = asString();
    SafeStrCpy(pBuf, msgStr, (UINT32)msgLen);
    msgLen = (int)strlen(pBuf);
}

CHXString
RTSPResponseMessage::asString()
{
	int lenTmpBuf = (int)(m_errorMsg.GetLength() + m_errorCode.GetLength() + 80);
    char* pTmpBuf = new char[lenTmpBuf];
    SafeSprintf(pTmpBuf, (UINT32)lenTmpBuf, "RTSP/%d.%d %s %s\r\n",
	majorVersion(), minorVersion(), (const char*)m_errorCode,
	(const char*)m_errorMsg);

    CHXString msgStr = pTmpBuf;
    delete[] pTmpBuf;

    MIMEHeader* pHeader = getFirstHeader();
    while(pHeader)
    {
	msgStr += pHeader->name();
	msgStr += ": ";
	pHeader->asString(msgStr);
	pHeader = getNextHeader();
    }
    msgStr += "\r\n";
    if(contentLength() > 0)
    {
	msgStr += getContent();
    }
    return msgStr;
}

CHXString
RTSPResponseMessage::GetHeaderValuesAsString(const char* pName)
{
    CHXString msgStr = "";
    MIMEHeader* pHeader = getHeader(pName);
    if (pHeader)
    {
        pHeader->asString(msgStr);
    }
    return msgStr;
}

void
RTSPResponseMessage::asString(char* pMsg, int& msgLen)
{
    CHXString msgStr = asString();
    SafeStrCpy(pMsg, msgStr, (UINT32)msgLen);
    msgLen = (int)strlen(pMsg);
}
