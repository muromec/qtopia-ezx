/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httppars.cpp,v 1.15 2008/08/18 21:53:19 ping Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxassert.h"
#include "debug.h"

#include "hxstrutl.h"
#include "hxstring.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "hxauth.h"

#include "httppars.h"
#include "httpmsg.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


HTTPParser::HTTPParser(IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
#if defined HELIX_FEATURE_SERVER
    m_bMessageLimitExceeded = FALSE;
#endif /* HELIX_FEATURE_SERVER */
}

HTTPParser::~HTTPParser()
{
    clearMessageLines();
    HX_RELEASE(m_pContext);
}

/*
 * Extract major/minor version info
 */
int 
HTTPParser::parseProtocolVersion(const CHXString& prot,
    int& majorVersion, int& minorVersion)
{
    if(strncasecmp(prot, "HTTP/", 5) != 0)
    	return 0;

    int nVerOffset = prot.Find('.');
    if(nVerOffset > 5)
    {
	CHXString majVersion = prot.Mid(5, nVerOffset-5);
	majorVersion = (int)strtol(majVersion, 0, 10);
	CHXString minVersion = prot.Mid(nVerOffset+1);
	minorVersion = (int)strtol(minVersion, 0, 10);
	return 1;
    }
    return 0;
}

/*
 * Parse option in form 'name[=value] *[;name[=value]]'
 */
int 
HTTPParser::parseHeaderValue(const char* pValue, MIMEHeader* pHeader)
{
    if(strlen(pValue) == 0)
	return 0;

    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);

    MIMEHeaderValue* pHeaderValue = 0;

    MIMEToken tok = scanner.nextToken(";");
    while(tok.hasValue())
    {
	if(!pHeaderValue)
	{
	    CHXString attribute = tok.value();
	    pHeaderValue = new MIMEHeaderValue;
	    if(tok.lastChar() == '=')
	    {
		tok = scanner.nextToken(";");
		CHXString value = tok.value();
		pHeaderValue->addParameter(attribute, value);
	    }
	    else
	    {
		pHeaderValue->addParameter(attribute);
	    }
	}
	else if(tok.lastChar() == '=')
	{
	    CHXString attribute = tok.value();
	    tok = scanner.nextToken(";");
	    CHXString value = tok.value();
	    pHeaderValue->addParameter(attribute, value);
	}
	else
	{
	    CHXString attribute = tok.value();
	    pHeaderValue->addParameter(attribute, "");
	}
	tok = scanner.nextToken("=;");
    }
    if(pHeaderValue)
	pHeader->addHeaderValue(pHeaderValue);
    return 0;
}

int
HTTPParser::parseWWWAuthenticateHeaderValues(const char* pValue,
                                             MIMEHeader* pHeader)
{
    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);
    IHXValues* authValues = NULL;

    /*
      char* pNonce = 0;
      char* pRealm = 0;
      char* pOpaque= 0;
      */

    MIMEToken nextTok = scanner.nextToken(" ");
    if(strcasecmp(nextTok.value(), "Digest") == 0)
    {
	if (m_pContext)
	{
	    CreateValuesCCF(authValues, m_pContext);
	}
	// Client should always set m_pContext
#ifndef HELIX_FEATURE_CLIENT
	else
	{
	    authValues = new CHXHeader;
	    authValues->AddRef();
	}
#endif
        if (authValues)
        {
            authValues->SetPropertyULONG32("AuthType", HX_AUTH_DIGEST);
            while(nextTok.hasValue())
            {
                nextTok = scanner.nextToken("=,");
                if(strcasecmp(nextTok.value(), "nonce") == 0)
                {
                    nextTok = scanner.nextToken("=,");
		    IHXBuffer* pBuf = NULL;
		    if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)(const char*)nextTok.value(),
						        nextTok.value().GetLength()+1, m_pContext))
		    {
		        authValues->SetPropertyCString("Nonce", pBuf);
		        HX_RELEASE(pBuf);
		    }
                }
                else if(strcasecmp(nextTok.value(), "realm") == 0)
                {
                    nextTok = scanner.nextToken("=,");
		    IHXBuffer* pBuf = NULL;
		    if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)(const char*)nextTok.value(),
						        nextTok.value().GetLength()+1, m_pContext))
		    {
		        authValues->SetPropertyCString("Realm", pBuf);
		        HX_RELEASE(pBuf);
		    }
                }
                else if(strcasecmp(nextTok.value(), "opaque") == 0)
                {
                    nextTok = scanner.nextToken("=,");
		    IHXBuffer* pBuf = NULL;
		    if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)(const char*)nextTok.value(),
						        nextTok.value().GetLength()+1, m_pContext))
		    {
		        authValues->SetPropertyCString("Opaque", pBuf);
		        HX_RELEASE(pBuf);
		    }
                }
            }
            pHeader->addHeaderValue
                (new HTTPAuthentication(authValues));
            authValues->Release();
        }
    }
    else if(strcasecmp(nextTok.value(), "Basic") == 0)
    {
	if (m_pContext)
	{
	    CreateValuesCCF(authValues, m_pContext);
	}
	// Client should always set m_pContext
#ifndef HELIX_FEATURE_CLIENT
	else
	{
	    authValues = new CHXHeader();
	    authValues->AddRef();
	}
#endif
        if (authValues)
        {
            authValues->SetPropertyULONG32("AuthType", HX_AUTH_BASIC);
            while(nextTok.hasValue())
            {
                nextTok = scanner.nextToken("=,");
                if(strcasecmp(nextTok.value(), "realm") == 0)
                {
                    nextTok = scanner.nextToken("=,");
                    char * realmstr = new char[strlen(nextTok.value()) + 1];
                    char* valstart, *valend;
                    valstart = (char*)strchr(nextTok.value(), '\"');
                    valend = (char*)strrchr(nextTok.value(), '\"');
                    if(valstart && valend && valstart != valend)
                    {
                        valstart++;
                        memcpy(realmstr, valstart, (size_t)(valend - valstart));
                        realmstr[valend - valstart] = 0;
                    }
                    else
                    {
                        strcpy(realmstr, nextTok.value());
                    }

		    IHXBuffer* pBuf = NULL;
		    if (HXR_OK == CreateAndSetBufferCCF(pBuf, (BYTE*)realmstr, 
						        strlen(realmstr)+1, m_pContext))
		    {
		        authValues->SetPropertyCString("Realm", pBuf);
		        HX_RELEASE(pBuf);
		    }
                    delete [] realmstr;
                }
            }
            pHeader->addHeaderValue(new HTTPAuthentication(authValues));
            authValues->Release();
        }
    }
    else
    {
	// Just add un-parsed value
        pHeader->addHeaderValue(pValue);
    }
    return 0;
}

/*
 * Parse header values as list of comma separated values
 */
int 
HTTPParser::defaultParseHeaderValues(const char* pValue, MIMEHeader* pHeader)
{
    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);
    
    MIMEToken nextTok = scanner.nextToken(",");
    while(nextTok.hasValue())
    {
	parseHeaderValue(nextTok.value(), pHeader);
	if((nextTok.lastChar() == MIMEToken::T_EOL) ||
	   (nextTok.lastChar() == MIMEToken::T_EOF))
	    return 0;
	nextTok = scanner.nextToken(",");
    }
    return 0;
}

/*
 * construct an MIMEHeader and add header values to it
 * format: 'HeaderName: option, option=value, option'
 */
MIMEHeader* 
HTTPParser::parseHeader(CHXString& str)
{
    MIMEHeader* pHeader = 0;
    MIMEInputStream input(str);
    MIMEScanner scanner(input);
    MIMEToken nextTok = scanner.nextToken(":");

    if(nextTok.hasValue())
    {
	pHeader = new MIMEHeader(nextTok.value());
	nextTok = scanner.nextToken("\n");
	if(strcasecmp(pHeader->name(), "WWW-Authenticate") == 0)
	{
	    parseWWWAuthenticateHeaderValues(nextTok.value(), pHeader);
	}
	else
	{
	    defaultParseHeaderValues(nextTok.value(), pHeader);
	}
    }
    return pHeader;
}

/*
 * Parse request line in format 'METHOD URL VERSION SEQ_NO'
 */
HTTPRequestMessage*
HTTPParser::parseRequestLine(CHXString& str)
{
    int majorVersion, minorVersion;
    HTTPRequestMessage* pReqMsg = 0;
    MIMEInputStream input(str);
    MIMEScanner scanner(input);

    // build message
    MIMEToken nextTok = scanner.nextToken();

    if(strcasecmp(nextTok.value(), "GET") == 0)
    {
	pReqMsg = new HTTPGetMessage;
    }
    else if(strcasecmp(nextTok.value(), "HEAD") == 0)
    {
	pReqMsg = new HTTPHeadMessage;
    }
    else if(strcasecmp(nextTok.value(), "POST") == 0)
    {
	pReqMsg = new HTTPPostMessage;
    }
    else
    {
	pReqMsg = new HTTPUnknownMessage;
    }

    // get URL
    nextTok = scanner.nextToken("\t \n");
    pReqMsg->setURL(nextTok.value());

    // get version info
    nextTok = scanner.nextToken();
    if(parseProtocolVersion(nextTok.value(), majorVersion, minorVersion))
    {
	pReqMsg->setVersion(majorVersion, minorVersion);
    }
    else
    {
	pReqMsg->setVersion(0,0);
    }
    return pReqMsg;
}

/*
 * Parse response line in format 'VERSION ERR_CODE SEQ_NO ERR_MSG'
 */
HTTPResponseMessage*
HTTPParser::parseResponseLine(CHXString& str)
{
    int majorVersion, minorVersion;
    HTTPResponseMessage* pRespMsg = 0;

    MIMEInputStream input(str);
    MIMEScanner scanner(input);

    MIMEToken nextTok = scanner.nextToken();

    pRespMsg = new HTTPResponseMessage;
    if(parseProtocolVersion(nextTok.value(), 
	majorVersion, minorVersion))
    {
	pRespMsg->setVersion(majorVersion, minorVersion);
    }
    else
    {
	pRespMsg->setVersion(0,0);
    }

    // get error code
    nextTok = scanner.nextToken();
    pRespMsg->setErrorCode(nextTok.value());

    // get error message
    nextTok = scanner.nextToken("\n");	// go to EOL
    pRespMsg->setErrorMsg(nextTok.value());

    return pRespMsg;
}

/*
 * Parse response message
 */
HTTPMessage* 
HTTPParser::parseResponse()
{
    HTTPResponseMessage* pRespMsg = 0;
    
    LISTPOSITION pos = m_msglines.GetHeadPosition();
    CHXString* pStr = (CHXString*)m_msglines.GetNext(pos);
    pRespMsg = parseResponseLine(*pStr);
    if(!pRespMsg)
	return 0;
    while(pos)
    {
	pStr = (CHXString*)m_msglines.GetNext(pos);
	MIMEHeader* pHeader = parseHeader(*pStr);
	if(pHeader)
	    pRespMsg->addHeader(pHeader);
    }
    return pRespMsg;
}

/*
 * Parse request message
 */
HTTPMessage* 
HTTPParser::parseRequest()
{
    HTTPRequestMessage* pReqMsg = 0;
    
    LISTPOSITION pos = m_msglines.GetHeadPosition();
    CHXString* pStr = (CHXString*)m_msglines.GetNext(pos);
    pReqMsg = parseRequestLine(*pStr);
    if(!pReqMsg)
	return 0;

    if(pReqMsg->majorVersion() < 0)
	return pReqMsg;

    while(pos)
    {
	pStr = (CHXString*)m_msglines.GetNext(pos);
	MIMEHeader* pHeader = parseHeader(*pStr);
	if(pHeader)
	    pReqMsg->addHeader(pHeader);
    }
    return pReqMsg;
}

/*
 * Add each line in message to list m_msglines.
 * Look for two carriage returns to delimit
 * message header.
 */
int 
HTTPParser::scanMessageHeader(const char* pMsg, UINT32 nMsgLen)
{
    // first, get rid of any leading whitespace and <CR><LF> chars

    const char* pCh = pMsg;
    UINT32 offset = 0;

    while(pCh[offset] == '\n' || pCh[offset] == '\r' || pCh[offset] == ' ' || 
          pCh[offset] == '\t')
    {
	if (++offset >= nMsgLen)
	    return 0;
    }
    pCh += offset;

    MIMEInputStream input(pMsg, nMsgLen - offset);
    MIMEScanner scanner(input);

    MIMEToken tok;
    int gotEOL = 0;
    UINT32 scanOffset = 0;
    do
    {
	tok = scanner.nextToken("\n");
	if (tok.hasValue())
	{
#if defined HELIX_FEATURE_SERVER
	    if (m_msglines.GetCount() >= MAX_HTTP_HEADER_COUNT ||
	        tok.value().GetLength() >= MAX_HTTP_HEADER_SIZE)
	    {
		m_bMessageLimitExceeded = TRUE;
		return 0;
	    }
#endif /* HELIX_FEATURE_SERVER */
		m_msglines.AddTail((void*)(new CHXString((const char*)tok.value())));
	}
	if(tok.lastChar() == MIMEToken::T_EOL)
	{
	    if(gotEOL && !tok.hasValue())
	    {
		scanOffset = scanner.offset();
		break;
	    }
	    else
	    {
		gotEOL = 1;
	    }
	}
	else
	{
	    gotEOL = 0;
	}
    } while(tok.lastChar() != MIMEToken::T_EOF);

    if(scanOffset == 0)
    {
	return 0;
    }
    else
    {
	return HX_SAFEINT(scanOffset + offset);
    }
}

#if !defined HELIX_FEATURE_SERVER

/*
 * Parse message pointed to by pMsg with length nMsgLen.
 * If not a complete message, return 0, otherwise return
 * HTTPMessage pointer.
 */
HTTPMessage* 
HTTPParser::parse(const char* pMsg, UINT32& nMsgLen)
{
    HTTPMessage* pHTTPMsg = 0;
    clearMessageLines();
    int msgOffset = scanMessageHeader(pMsg, nMsgLen);

    CHXString* str = NULL;

    if(msgOffset > 0)
    {
	if(m_msglines.GetCount() == 0)
	{
	    nMsgLen = 0;
	    return 0;
	}
        str = (CHXString*) m_msglines.GetHead();
	if(strncasecmp((*str), "HTTP/", 5) == 0)
	{
	    pHTTPMsg = parseResponse();
	}
	else 
	{
	    pHTTPMsg = parseRequest();
	}

	if(pHTTPMsg)
	{
	    nMsgLen = (UINT32)msgOffset;
	}
    }
    else
    {
	/* Might be a HTTP/0.9 request.  No extra blank line. Drat. */
	if(m_msglines.GetCount() == 0)
	{
	    nMsgLen = 0;
	    return 0;
	}
	pHTTPMsg = parseRequest();
	if(!pHTTPMsg || pHTTPMsg->majorVersion() > 0)
	{
	    nMsgLen = 0;
	    HX_DELETE(pHTTPMsg);
	}
    }
    return pHTTPMsg;
}

#else /* !HELIX_FEATURE_SERVER */
/*
 * Parse message pointed to by pMsg with length nMsgLen.
 * If not a complete message, return 0, otherwise return
 * HTTPMessage pointer.
 */
HTTPMessage* 
HTTPParser::parse(const char* pMsg, UINT32& nMsgLen, HXBOOL& bMessageTooLarge)
{
    if (nMsgLen > MAX_HTTP_MSG_SIZE)
    {
	m_bMessageLimitExceeded = TRUE;
	bMessageTooLarge = TRUE;
	return 0;
    }

    HTTPMessage* pHTTPMsg = 0;
    clearMessageLines();
    int msgOffset = scanMessageHeader(pMsg, nMsgLen);

    if (m_bMessageLimitExceeded)
    {
	bMessageTooLarge = TRUE;
	return 0;
    }
    if(msgOffset > 0)
    {
	if(m_msglines.GetCount() == 0)
	{
	    nMsgLen = 0;
	    return 0;
	}
  	CHXString* str = (CHXString*)m_msglines.GetHead();
	if(strncasecmp((*str), "HTTP/", 5) == 0)
	{
	    pHTTPMsg = parseResponse();
	}
	else 
	{
	    pHTTPMsg = parseRequest();
	}

	if(pHTTPMsg)
	{
	    nMsgLen = msgOffset;
	}
    }
    else
    {
	//nMsgLen = 0;

	/* Might be a HTTP/0.9 request.  No extra blank line. Drat. */
	if(m_msglines.GetCount() == 0)
	{
	    nMsgLen = 0;
	    return 0;
	}
	pHTTPMsg = parseRequest();
	if(!pHTTPMsg || pHTTPMsg->majorVersion() > 0)
	{
	    nMsgLen = 0;
	    HX_DELETE(pHTTPMsg);
	}
    }
    return pHTTPMsg;
}

#endif /* !HELIX_FEATURE_SERVER */

/*
 * Delete contents of m_msglines
 */
void 
HTTPParser::clearMessageLines()
{
    LISTPOSITION pos = m_msglines.GetHeadPosition();
    while(pos)
    {
	CHXString* str = (CHXString*)m_msglines.GetNext(pos);
	delete str;
    }
    m_msglines.RemoveAll();
}
