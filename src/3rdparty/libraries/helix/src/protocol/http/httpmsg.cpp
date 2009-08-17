/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpmsg.cpp,v 1.14 2006/05/12 01:48:24 atin Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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
#include "hxtypes.h"
#include "hxstrutl.h"
#include "hxstring.h"
#include "hxslist.h"
#include "mimehead.h"
#include "ihxpckts.h"
#include "hxauth.h"
#include "httpmsg.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// static definitions

const int HTTPMessage::MAJ_VERSION = 1;
const int HTTPMessage::MIN_VERSION = 0;

/*
 * HTTPAuthentication methods
 */

HTTPAuthentication::HTTPAuthentication(const char* authString,
    AuthenticationType authType):
    m_authType(authType),
    m_authString(authString),
    m_authValues(NULL)
{
}

HTTPAuthentication::HTTPAuthentication(IHXValues* pAuth):
    m_authType(AU_DIGEST),
    m_authValues(pAuth)
{
    if(m_authValues)
        m_authValues->AddRef();
}

HTTPAuthentication::~HTTPAuthentication()
{
    if(m_authValues)
        m_authValues->Release();
}

void
HTTPAuthentication::asString(CHXString& str)
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

                    str = CHXString("Digest realm=") +
                        pRealm->GetBuffer() + ",\r\n" +
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
                    str = CHXString("Basic realm=") +
                        pRealm->GetBuffer();
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
HTTPAuthentication::asString()
{
    CHXString str;
    asString(str);
    return str;
}

HTTPMessage::HTTPMessage()
    : m_isCloakedMsg(FALSE)
{
    setVersion(MAJ_VERSION, MIN_VERSION);
}

HTTPMessage::~HTTPMessage()
{
    clearHeaderList();
}

void
HTTPMessage::addHeader(MIMEHeader* pHeader)
{
    m_headers.AddTail(pHeader);
}

void 
HTTPMessage::addHeader(const char* pName, const char* pValue)
{
    MIMEHeader* pHeader = new MIMEHeader(pName);
    pHeader->addHeaderValue(pValue);
    addHeader(pHeader);
}

MIMEHeader*
HTTPMessage::getHeader(const char* pName)
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
HTTPMessage::getHeaderValue(const char* pName)
{
    MIMEHeader* pHeader = getHeader(pName);
    if(pHeader)
    {
	MIMEHeaderValue* pHeaderValue = pHeader->getFirstHeaderValue();
	if(pHeaderValue)
	{
	    return pHeaderValue->value();
	}
    }
    return CHXString("");
}

int
HTTPMessage::getHeaderValue(const char* pName, UINT32& value)
{
    CHXString strValue = getHeaderValue(pName);
    if(strValue != "")
    {
	value = (UINT32)strtol((const char*)strValue, 0, 10);
	return 1;
    }
    return 0;
}

MIMEHeader*
HTTPMessage::getFirstHeader()
{
    m_listpos = m_headers.GetHeadPosition();
    if(m_listpos)
	return (MIMEHeader*)m_headers.GetNext(m_listpos);
    return 0;
}

MIMEHeader*
HTTPMessage::getNextHeader()
{
    if(m_listpos)
	return (MIMEHeader*)m_headers.GetNext(m_listpos);
    return 0;
}

void
HTTPMessage::clearHeaderList()
{
    MIMEHeader* pHeader = getFirstHeader();
    while(pHeader)
    {
	delete pHeader;
	pHeader = getNextHeader();
    }
}
void 
HTTPMessage::setContent(BYTE* pBuf, UINT32 pBufLen)
{
    m_content = CHXString((const char*)pBuf, HX_SAFEINT(pBufLen));
}

int
HTTPMessage::contentLength()
{
    return (int)m_content.GetLength();
}

void
HTTPRequestMessage::asString(char* pBuf, int& msgLen, UINT32 ulBufLen)
{
	int lenTmpBuf = (int)(strlen(url()) + strlen(tagStr()) + 80);
    char* tmpBuf = new char[lenTmpBuf];
    SafeSprintf(tmpBuf, (UINT32)lenTmpBuf, "%s %s HTTP/%d.%d\r\n",
	tagStr(), url(), majorVersion(), minorVersion());

    CHXString msgStr = tmpBuf;

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
    SafeStrCpy(pBuf, msgStr, ulBufLen);
    msgLen = (int)strlen(pBuf);

    delete[] tmpBuf;
}

void
HTTPResponseMessage::asString(char* pMsg, int& msgLen, UINT32 ulBufLen)
{
	int lenTmpBuf = (int)(64 + m_errorMsg.GetLength());
    char* pTmpBuf = new char[lenTmpBuf];
    SafeSprintf
    (
	pTmpBuf,  (UINT32)lenTmpBuf,
	"HTTP/%d.%d %s %s\r\n",
	majorVersion(), 
	minorVersion(), 
	(const char*)m_errorCode,
	(const char*)m_errorMsg
    );

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
    if(contentLength() > 0)
    {
	msgStr += getContent();
    }
    msgStr += "\r\n";
    SafeStrCpy(pMsg, msgStr, ulBufLen);
    msgLen = (int)strlen(pMsg);
}
