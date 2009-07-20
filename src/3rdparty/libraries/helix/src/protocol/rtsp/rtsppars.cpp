/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtsppars.cpp,v 1.15 2007/07/06 20:51:35 jfinnecy Exp $
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
#include "hxtypes.h"
#include "hxassert.h"
#include "debug.h"

#include "hxstrutl.h"
#include "hxstring.h"
#include "hxslist.h"
#include "mimehead.h"
#include "rtsppars.h"
#include "smpte.h"
#include "nptime.h"
#include "rtspmsg.h"
#include "chxpckts.h"
#include "hxauth.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


RTSPParser::RTSPParser()
{
}

RTSPParser::~RTSPParser()
{
    clearMessageLines();
}

/*
 * Extract major/minor version info
 */
int 
RTSPParser::parseProtocolVersion(const CHXString& prot,
    int& majorVersion, int& minorVersion)
{
    if(strncasecmp(prot, "RTSP/", 5) != 0)
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
 * Parse option in form 'value *[;name[=value]]'
 */
int 
RTSPParser::parseHeaderValue(const char* pValue, MIMEHeader* pHeader)
{
    if(strlen(pValue) == 0)
	return 0;

    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);

    MIMEHeaderValue* pHeaderValue = 0;

    MIMEToken tok = scanner.nextToken(";=");
    while(tok.hasValue())
    {
	if(!pHeaderValue)
	{
	    CHXString attribute = tok.value();
	    pHeaderValue = new MIMEHeaderValue;
	    if(!pHeaderValue)
	    {
	        return 0;
	    }
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
RTSPParser::parseRangeValue(const char* pValue, MIMEHeader* pHeader)
{
    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);

    MIMEToken nextTok = scanner.nextToken("=");
    if(strcasecmp(nextTok.value(), "smpte") == 0)
    {
	UINT32 tBegin = RTSP_PLAY_RANGE_BLANK;
	UINT32 tEnd = RTSP_PLAY_RANGE_BLANK;
	nextTok = scanner.nextToken("-");
	if(nextTok.hasValue())
	{
	    SMPTETimeCode tCode1(nextTok.value());
	    tBegin = UINT32(tCode1);
	}
	if((nextTok.lastChar() != MIMEToken::T_EOL) ||
	   (nextTok.lastChar() != MIMEToken::T_EOF))
	{
	    nextTok = scanner.nextToken("\n");
	    if(nextTok.hasValue())
	    {
		SMPTETimeCode tCode2(nextTok.value());
		tEnd = UINT32(tCode2);
	    }
	}
	pHeader->addHeaderValue(new RTSPRange(tBegin, tEnd,
	    RTSPRange::TR_SMPTE));
    }
    else if(strcasecmp(nextTok.value(), "npt") == 0)
    {
	UINT32 tBegin = RTSP_PLAY_RANGE_BLANK;
	UINT32 tEnd = RTSP_PLAY_RANGE_BLANK;
	nextTok = scanner.nextToken("-");
	if(nextTok.hasValue())
	{
	    NPTime t1(nextTok.value());
	    tBegin = UINT32(t1);
	}
	if((nextTok.lastChar() != MIMEToken::T_EOL) ||
	   (nextTok.lastChar() != MIMEToken::T_EOF))
	{
	    nextTok = scanner.nextToken("\n");
	    if(nextTok.hasValue())
	    {
		NPTime t2(nextTok.value());
		tEnd = UINT32(t2);
	    }
	}
	pHeader->addHeaderValue(new RTSPRange(tBegin, tEnd,
	    RTSPRange::TR_NPT));
    }
    else if(strcasecmp(nextTok.value(), "clock") == 0)
    {
	//XXXBAB fill this in
    }
    return 0;
}

int
RTSPParser::parseAuthenticationValue(const char* pValue, MIMEHeader* pHeader)
{
    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);

    MIMEToken nextTok = scanner.nextToken(" ");
    if(strcasecmp(nextTok.value(), "HXPrivate") == 0)
    {
	nextTok = scanner.nextToken("=");
	if(strcasecmp(nextTok.value(), "nonce") == 0)
	{
	    nextTok = scanner.nextToken();
	    pHeader->addHeaderValue(new RTSPAuthentication(nextTok.value(),
		RTSPAuthentication::AU_HX_PRIVATE));
	}
    }
    return 0;
}

int
RTSPParser::parsePEPInfoHeaderValues(const char* pValue, MIMEHeader* pHeader)
{
    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);
    HXBOOL bStrengthMust = FALSE;

    MIMEToken nextTok = scanner.nextToken(" {}");
    while(nextTok.lastChar() != MIMEToken::T_EOF)
    {
	if(strcasecmp(nextTok.value(), "strength") == 0)
	{
	    nextTok = scanner.nextToken(" }");
	    if(strcasecmp(nextTok.value(), "must") == 0)
	    {
		bStrengthMust = TRUE;
		break;
	    }
	}
	nextTok = scanner.nextToken(" {}");
    }
    pHeader->addHeaderValue(new RTSPPEPInfo(bStrengthMust));
    return 0;
}

int
RTSPParser::parseRTPInfoHeaderValues(const char* pValue, MIMEHeader* pHeader)
{
    // There is a bug in RFC2326 related to the ambiguity of "url=" parameter in
    // RTP-Info. More details can be found at:
    // http://sourceforge.net/tracker/index.php?func=detail&group_id=23194&atid=377744&aid=448521
    //
    // For now, we only can do the best we can:
    // 1. find stream URL entry based on ",url="
    // 2. read "url", "seq" and "rtptime" within the stream URL entry
    if (!pValue || 0 == strlen(pValue))
    {
        return 0;
    }

    const char* pCurrentEntry = NULL;
    const char* pNextEntry = NULL;
    CHXString   value = pValue;
    CHXString   entry;

    pCurrentEntry = strstr(pValue, "url=");
    // RTP-Info starts with "url="
    HX_ASSERT(pCurrentEntry == pValue);

    while ((pNextEntry = NextRTPInfoEntry(pCurrentEntry+4, "url=", ',')))
    {
        // retreive stream URL entry
        entry = value.Mid(pCurrentEntry - pValue, pNextEntry - pCurrentEntry);

        // retrieve "url", "seq" and "rtptime"
        SetRTPInfoEntry(entry, pHeader);
        
        pCurrentEntry = pNextEntry;
    }

    entry = value.Mid(pCurrentEntry - pValue);
    SetRTPInfoEntry(entry, pHeader);

    return 0;
}

const char*
RTSPParser::NextRTPInfoEntry(const char* pValue, const char* pTarget, const char cDelimiter)
{
    const char* pEntry = NULL;

    while ((pEntry = strstr(pValue, pTarget)))
    {
        const char* pTemp = pEntry;

        // a valid entry should begin with:
        // ",url=" OR ", url="

        // rewind and skip all the spaces in between
        while (*(--pTemp) == ' ') {}

        // the first non-space character should be the delimiter
        if (*pTemp == cDelimiter)
        {
            break;
        }
        // not valid entry, keep looking 
        else
        {
            pValue = pEntry + strlen(pTarget);
        }
    }

    return pEntry;
}

int 
RTSPParser::ReadRTPInfoEntry(CHXString      in, 
                             INT32          i,
                             INT32          length,
                             CHXString&     out)
{
    UINT32 ulLength = 0;
    CHXString temp;

    if (length > 0)
    {
        temp = in.Mid(i, length);
    }
    else
    {
        temp = in.Mid(i);
    }

    temp.TrimLeft();
    temp.TrimRight();

    ulLength = temp.GetLength();
    // remove trailing ',' or ';' in case there is any
    if (temp[ulLength-1] == ',' || temp[ulLength-1] == ';')
    {
        out = temp.Mid(0, (INT32)(ulLength - 1));
        out.TrimRight();
    }
    else
    {
        out = temp;
    }

    return 0;
}

int
RTSPParser::SetRTPInfoEntry(CHXString in, MIMEHeader* pHeader)
{
    INT32 lURL = -1, lSeq = -1, lRTPTime = -1;
    CHXString URLValue, SeqValue, RTPTimeValue;
    MIMEHeaderValue* pHeaderValue = new MIMEHeaderValue;

    const char* pIn = (const char*)in;    
    const char* pSeq = NextRTPInfoEntry(pIn, "seq=", ';');
    const char* pRTPTime = NextRTPInfoEntry(pIn, "rtptime=", ';');

    lURL = in.Find("url=");
    HX_ASSERT(lURL == 0);

    if (pSeq)
    {
        lSeq = pSeq - pIn;
    }

    if (pRTPTime)
    {
        lRTPTime = pRTPTime - pIn;
    }

    // both seq and rtptime are present
    if (lSeq > 0 && lRTPTime > 0)
    {
        // url;seq;rtptime
        if (lRTPTime > lSeq)
        {
            ReadRTPInfoEntry(in, lURL+4, lSeq-lURL-5, URLValue);
            ReadRTPInfoEntry(in, lSeq+4, lRTPTime-lSeq-5, SeqValue);
            ReadRTPInfoEntry(in, lRTPTime+8, -1, RTPTimeValue);
        }
        // url;rtptime;seq
        else
        {
            ReadRTPInfoEntry(in, lURL+4, lRTPTime-lURL-5, URLValue);
            ReadRTPInfoEntry(in, lRTPTime+8, lSeq-lRTPTime-9, RTPTimeValue);
            ReadRTPInfoEntry(in, lSeq+4, -1, SeqValue);
        }
    }
    // url;seq
    else if (lSeq > 0)
    {
        ReadRTPInfoEntry(in, lURL+4, lSeq-lURL-5, URLValue);
        ReadRTPInfoEntry(in, lSeq+4, -1, SeqValue);
    }
    // url;rtptime
    else if (lRTPTime > 0)
    {
        ReadRTPInfoEntry(in, lURL+4, lRTPTime-lURL-5, URLValue);
        ReadRTPInfoEntry(in, lRTPTime+8, -1, RTPTimeValue);
    }
    // invalid case, either seq or rtptime has to be present
    else
    {
        HX_ASSERT(FALSE);
    }

    if (!URLValue.IsEmpty())
    {
        pHeaderValue->addParameter("url", URLValue);
    }

    if (!SeqValue.IsEmpty())
    {
        pHeaderValue->addParameter("seq", SeqValue);
    }

    if (!RTPTimeValue.IsEmpty())
    {
        pHeaderValue->addParameter("rtptime", RTPTimeValue);
    }

    pHeader->addHeaderValue(pHeaderValue);

    return 0;
}

int
RTSPParser::parseBackChannelValue(const char* pValue, MIMEHeader* pHeader)
{
    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);

    MIMEToken nextTok = scanner.nextToken();
    pHeader->addHeaderValue(new MIMEHeaderValue(nextTok.value()));
    return 0;
}

int
RTSPParser::parseAlertValue(const char* pValue, MIMEHeader* pHeader)
{
    MIMEInputStream input(pValue, strlen(pValue));
    MIMEScanner scanner(input);
    MIMEToken nextTok;
    MIMEHeaderValue* pHeaderValue = new MIMEHeaderValue;

    // Parse Alert number
    nextTok = scanner.nextToken(";");
    if (nextTok.hasValue())
    {
	pHeaderValue->addParameter(nextTok.value(), "");
    }

    // Parse Alert text
    nextTok = scanner.nextToken("");
    if (nextTok.hasValue())
    {
	pHeaderValue->addParameter(nextTok.value(), "");
    }

    pHeader->addHeaderValue(pHeaderValue);

    return 0;
}

/*
 * Parse header values as list of comma separated values
 */
int 
RTSPParser::defaultParseHeaderValues(const char* pValue, MIMEHeader* pHeader)
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

int
RTSPParser::parseWWWAuthenticateHeaderValues(const char* pValue, 
					     MIMEHeader* pHeader)
{
/*
 * Not needed anymore, authentication model has changed
 * 
 */
    return 0;
}

int
RTSPParser::parseDigestAuthorizationHeaderValues(const char* pValue,
						 MIMEHeader* pHeader)
{
/*
 * Not needed anymore, authentication model has changed
 * 
 */
    return 0;
}

/*
 * construct a MIMEHeader and add header values to it
 * format: 'HeaderName: option, option=value;option=value, option'
 */
MIMEHeader* 
RTSPParser::parseHeader(CHXString& str)
{
    MIMEHeader* pHeader = 0;
    MIMEInputStream input(str);
    MIMEScanner scanner(input);
    MIMEToken nextTok = scanner.nextToken(":");

    if(nextTok.hasValue())
    {
	pHeader = new MIMEHeader(nextTok.value());
	nextTok = scanner.nextToken("\n");

	if(strcasecmp(pHeader->name(), "Range") == 0)
	{
	    parseRangeValue(nextTok.value(), pHeader);
	}
	else if
	(
	    (strcasecmp(pHeader->name(), "WWW-Authenticate") == 0)
	    ||
	    (strcasecmp(pHeader->name(), "Authenticate") == 0)
	    ||
	    (strcasecmp(pHeader->name(), "Authorization") == 0)
	    ||
	    // XXX HP get the absolute redirect URL which could 
	    // contain ',' and would be treated as field separater
	    // in defaultParseHeaderValues()
	    (strcasecmp(pHeader->name(), "Location") == 0)
            ||
            (strcasecmp(pHeader->name(), "Content-base") == 0)
	)
	{
	    MIMEHeaderValue* pHeaderValue = new MIMEHeaderValue;
	    if(pHeaderValue)
	    {
		pHeaderValue->addParameter(nextTok.value());
		pHeader->addHeaderValue(pHeaderValue);
	    }
	}
	else if((strcasecmp(pHeader->name(), "PEP-Info") == 0) ||
	        (strcasecmp(pHeader->name(), "C-PEP-Info") == 0))
	{
	    parsePEPInfoHeaderValues(nextTok.value(), pHeader);
	}
        else if(strcasecmp(pHeader->name(), "RTP-Info") == 0)
        {
            parseRTPInfoHeaderValues(nextTok.value(), pHeader);
        }
	else if(strcasecmp(pHeader->name(), "BackChannel") == 0)
	{
	    parseBackChannelValue(nextTok.value(), pHeader);
	}
	else if(strcasecmp(pHeader->name(), "Alert") == 0)
	{
	    parseAlertValue(nextTok.value(), pHeader);
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
RTSPRequestMessage*
RTSPParser::parseRequestLine(CHXString& str)
{
    int majorVersion, minorVersion;
    RTSPRequestMessage* pReqMsg = 0;
    MIMEInputStream input(str);
    MIMEScanner scanner(input);

    // build message
    MIMEToken nextTok = scanner.nextToken();
    const char* pMsgName = nextTok.value();

    if(strcasecmp(pMsgName, "SETUP") == 0)
    {
	pReqMsg = new RTSPSetupMessage;
    }
    else if(strcasecmp(pMsgName, "REDIRECT") == 0)
    {
	pReqMsg = new RTSPRedirectMessage;
    }
    else if(strcasecmp(pMsgName, "PLAY") == 0)
    {
	pReqMsg = new RTSPPlayMessage;
    }
    else if(strcasecmp(pMsgName, "PAUSE") == 0)
    {
	pReqMsg = new RTSPPauseMessage;
    }
    else if(strcasecmp(pMsgName, "SET_PARAMETER") == 0)
    {
	pReqMsg = new RTSPSetParamMessage;
    }
    else if(strcasecmp(pMsgName, "GET_PARAMETER") == 0)
    {
	pReqMsg = new RTSPGetParamMessage;
    }
    else if(strcasecmp(pMsgName, "TEARDOWN") == 0)
    {
	pReqMsg = new RTSPTeardownMessage;
    }
    else if(strcasecmp(pMsgName, "DESCRIBE") == 0)
    {
	pReqMsg = new RTSPDescribeMessage;
    }
    else if(strcasecmp(pMsgName, "OPTIONS") == 0)
    {
	pReqMsg = new RTSPOptionsMessage(FALSE);
    }
    else if(strcasecmp(pMsgName, "RECORD") == 0)
    {
	pReqMsg = new RTSPRecordMessage;
    }
    else if(strcasecmp(pMsgName, "ANNOUNCE") == 0)
    {
	pReqMsg = new RTSPAnnounceMessage;
    }
    else
    {
	pReqMsg = new RTSPUnknownMessage;
    }

    // get URL
    nextTok = scanner.nextToken(" ");
    pReqMsg->setURL(nextTok.value());

    // get version info
    nextTok = scanner.nextToken();
    if(parseProtocolVersion(nextTok.value(), majorVersion, minorVersion))
    {
	pReqMsg->setVersion(majorVersion, minorVersion);
    }
    else
    {
	pReqMsg->setVersion(0,0);	// indicate bad version
    }
    
    return pReqMsg;
}

/*
 * Parse response line in format 'VERSION ERR_CODE SEQ_NO ERR_MSG'
 */
RTSPResponseMessage*
RTSPParser::parseResponseLine(CHXString& str)
{
    int majorVersion, minorVersion;
    RTSPResponseMessage* pRespMsg = 0;

    MIMEInputStream input(str);
    MIMEScanner scanner(input);

    MIMEToken nextTok = scanner.nextToken();

    pRespMsg = new RTSPResponseMessage;

    if(parseProtocolVersion(nextTok.value(), 
	majorVersion, minorVersion))
    {
	pRespMsg->setVersion(majorVersion, minorVersion);
    }
    else
    {
	pRespMsg->setVersion(0,0);	// indicate bad version
    }

    // get error code
    nextTok = scanner.nextToken();
    pRespMsg->setErrorCode(nextTok.value());

    // get error message
    nextTok = scanner.nextToken("\n");
    pRespMsg->setErrorMsg(nextTok.value());

    return pRespMsg;
}

/*
 * Parse response message
 */
RTSPMessage* 
RTSPParser::parseResponse()
{
    RTSPResponseMessage* pRespMsg = 0;
    
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
    // get sequence number
    UINT32 seqNo = 0;
    pRespMsg->getHeaderValue("CSeq", seqNo);
    pRespMsg->setSeqNo(seqNo);
    return pRespMsg;
}

/*
 * Parse request message
 */
RTSPMessage* 
RTSPParser::parseRequest()
{
    RTSPRequestMessage* pReqMsg = 0;
    
    LISTPOSITION pos = m_msglines.GetHeadPosition();
    CHXString* pStr = (CHXString*)m_msglines.GetNext(pos);
    pReqMsg = parseRequestLine(*pStr);
    if(!pReqMsg)
	return 0;
    while(pos)
    {
	pStr = (CHXString*)m_msglines.GetNext(pos);
	MIMEHeader* pHeader = parseHeader(*pStr);
	if(pHeader)
	    pReqMsg->addHeader(pHeader);
    }
    // get sequence number
    UINT32 seqNo = 0;
    pReqMsg->getHeaderValue("CSeq", seqNo);
    pReqMsg->setSeqNo(seqNo);
    return pReqMsg;
}

/*
 * Add each line in message to list m_msglines.
 * Look for two carriage returns to delimit
 * message header.
 */
UINT32 
RTSPParser::scanMessageHeader(const char* pMsg, UINT32 nMsgLen)
{
    // first, get rid of any leading whitespace and <CR><LF> chars

    const char* pCh = pMsg;
    while(*pCh == '\n' || *pCh == '\r' || *pCh == ' ' || *pCh == '\t')
    {
	pCh++;
    }
    UINT32 offset = (UINT32)(pCh - pMsg);
    if(offset > nMsgLen)
    {
	return 0;
    }

    MIMEInputStream input(pCh, nMsgLen - offset);
    MIMEScanner scanner(input);

    MIMEToken tok;
    int gotEOL = 0;
    UINT32 scanOffset = 0;
    do
    {
	tok = scanner.nextToken("\n");
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
	m_msglines.AddTail((void*)(new CHXString((const char*)tok.value())));
    } while(tok.lastChar() != MIMEToken::T_EOF);

    if(scanOffset == 0)
    {
	return 0;
    }
    else
    {
	return scanOffset + offset;
    }
}

/*
 * Parse message pointed to by pMsg with length nMsgLen.
 * If not a complete message, return 0, otherwise return
 * RTSPMessage pointer.
 */
RTSPMessage* 
RTSPParser::parse(const char* pMsg, UINT32& nMsgLen)
{
    RTSPMessage* pRTSPMsg = 0;
    clearMessageLines();
    INT32 msgOffset = (INT32)scanMessageHeader(pMsg, nMsgLen);
    if(msgOffset > 0)
    {
	if(m_msglines.GetCount() == 0)
	{
	    nMsgLen = 0;
	    return 0;
	}
	CHXString* str = (CHXString*)m_msglines.GetHead();
	if(strncasecmp((*str), "RTSP/", 5) == 0)
	{
	    pRTSPMsg = parseResponse();
	}
	else 
	{
	    pRTSPMsg = parseRequest();
	}

	if(pRTSPMsg)
	{
	    UINT32 contentLength = 0;
	    if(pRTSPMsg->getHeaderValue("Content-length", contentLength))
	    {
		if((UINT32)msgOffset + contentLength <= (UINT32)nMsgLen)
		{
		    CHXString content(&pMsg[msgOffset], 
		    	HX_SAFEINT(contentLength));
		    pRTSPMsg->setContent(content);
		    nMsgLen = msgOffset + contentLength;
		}
		else	// content-length not satisfied
		{
		    // XXXBAB - update nMsglen as number of bytes needed 
		    // to complete message
		    delete pRTSPMsg;
		    pRTSPMsg = 0;
		    nMsgLen = 0;
		}
	    }
	    else
	    {
		nMsgLen = (UINT32)msgOffset;
	    }
	}
    }
    else
    {
	nMsgLen = 0;
    }
    return pRTSPMsg;
}

/*
 * Delete contents of m_msglines
 */
void 
RTSPParser::clearMessageLines()
{
    LISTPOSITION pos = m_msglines.GetHeadPosition();
    while(pos)
    {
	CHXString* str = (CHXString*)m_msglines.GetNext(pos);
	delete str;
    }
    m_msglines.RemoveAll();
}
