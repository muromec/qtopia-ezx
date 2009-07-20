/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: looseprs.cpp,v 1.13 2007/07/06 20:43:45 jfinnecy Exp $
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
#include "hxstrutl.h"
#include "hxmap.h"
#include "xmlencod.h"
#include "looseprs.h"
#include <ctype.h>

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

static const int MAX_ERROR_LEN = 80;

XMLParser::XMLParser(HXBOOL bStrictCompliance, const char* pEncoding,
		     HXBOOL bAllowNonXMLComments):
    m_bStrictCompliance(bStrictCompliance),
    m_ulCurrentLine(1),
    m_ulCurrentCol(1),
    m_ulTagStartLine(1),
    m_ulTagStartCol(1),
    m_pLastError(0),
    m_bAllowNonXMLComments(bAllowNonXMLComments),
    m_bCommentWasFound(FALSE),
    m_bStoreErrors(FALSE),
	m_bXMLandSMIL10FullCompliance(FALSE)
{
    m_pCurrentFrame = new XMLFrame;
    m_comment_state = 0;
    m_comment_get_arg = 0;
    m_comment_pos = 0;
    if(pEncoding)
    {
	m_pEncoding = new_string(pEncoding);
    }
    else
    {
	m_pEncoding = new_string("US-ASCII");	// default encoding
    }
}

XMLParser::~XMLParser()
{
    HX_DELETE(m_pLastError);
    HX_VECTOR_DELETE(m_pEncoding);
    delete m_pCurrentFrame;
}

void
XMLParser::Reset(void)
{
    if (m_pCurrentFrame != NULL) delete m_pCurrentFrame;
    m_pCurrentFrame = NULL;
}

// class function to get version/encoding of XML content
HX_RESULT
XMLParser::GetPrologInfo(const char* pBuf,
			 UINT32 ulBufLen,
			 char*& pVersion,
			 char*& pEncoding)
{
    HX_RESULT rc = HXR_FAIL;

    const char* pCh = pBuf;
    enum { VERSION, ENCODING } nPrologAttribute = VERSION;
    char quoteType = '"';

    HXBOOL bDone = FALSE;
    HXBOOL bInComment = FALSE;
    int state = 0;
    const char* pValueStart = NULL;

    while(!bDone &&
        pCh < pBuf + ulBufLen)
    {
        switch(state)
        {
            case 0: // looking for either a comment open
                    // or a prolog
            {
                if(*pCh == '<')
                {
                    if(*(pCh + 1) == '!' &&
                       *(pCh + 2) == '-' &&
                       *(pCh + 3) == '-')
                    {
                        if(bInComment)
                        {
                            bDone = TRUE;   // no nested comments
                        }
                        pCh += 4;   // skip over
                        bInComment = TRUE;
                        state = 1;
                    }
                    else if(*(pCh + 1) == '?' &&
                            *(pCh + 2) == 'x' &&
                            *(pCh + 3) == 'm' &&
                            *(pCh + 4) == 'l')
                    {
                        pCh += 5;   // skip over 
                        state = 2;
                    }
                    else
                    {
                        bDone = TRUE;   // no prolog
                    }
                }
                else if(isspace(*pCh))
                {
                    pCh++;
                }
                else
                {
                    bDone = TRUE;   // can't find prolog
                }
            }
            break;

            case 1: // comment end
            {
                if(*pCh == '-' &&
                   *(pCh + 1) == '-' &&
                   *(pCh + 2) == '>')
                {
                    pCh += 3;
                    bInComment = FALSE;
                    state = 0;
                }
                else
                {
                    pCh++;
                }
            }
            break;

            case 2: // known attribute in prolog
            {
                if(strncmp(pCh, "version", 7) == 0)
                {
                    pCh += 7;
                    nPrologAttribute = VERSION;
                    state = 3;
                }
                else if(strncmp(pCh, "encoding", 8) == 0)
                {
                    pCh += 8;
                    nPrologAttribute = ENCODING;
                    state = 3;
                }
                else
                {
                    pCh++;
                }
            }
            break;

            case 3: // '='
            {
                if(*pCh == '=')
                {
                    state = 4;
                }
                pCh++;
            }
            break;

            case 4: // quote type
            {
                if(*pCh == '"')
                {
                    quoteType = '"';
                    pValueStart = pCh + 1;
                    state = 5;
                }
                else if(*pCh == '\'')
                {
                    quoteType = '\'';
                    pValueStart = pCh + 1;
                    state = 5;
                }
                else
                {
                    bDone = TRUE;   // badly formed
                }
                pCh++;
            }
            break;

            case 5: // get value
            {
                if(*pCh == quoteType)
                {
                    if (pValueStart)
                    {
                        INT32 lValLen = pCh - pValueStart;
                        if (lValLen > 0)
                        {
                            char* pTmp = new char [lValLen + 1];
                            if (pTmp)
                            {
                                strncpy(pTmp, pValueStart, lValLen);
                                pTmp[lValLen] = '\0';
                                if(nPrologAttribute == VERSION)
                                {
                                    pVersion = pTmp;
                                }
                                else if(nPrologAttribute == ENCODING)
                                {
                                    pEncoding = pTmp;
                                }
                                rc = HXR_OK;    // got one!
                                // reset for next string
                                state = 2;
                            }
                        }
                    }
                }
                pCh++;
            }
            break;
        }
    }
    return rc;
}

char
XMLParser::GetEscapeMacro(const char*& ptr, const char* end)
{
    char returnCh;
    if(*ptr != '&')
    {
	returnCh = *ptr;
    }
    else
    {
	int maxLen = end - ptr;
	if((maxLen > 5) && strncmp(ptr, "&apos;", 6) == 0)
	{
	    returnCh = '\'';
	    ptr += 6;
	}
	else if((maxLen > 5) && strncmp(ptr, "&quot;", 6) == 0)
	{
	    returnCh = '"';
	    ptr += 6;
	}
	else if((maxLen > 3) && strncmp(ptr, "&lt;", 4) == 0)
	{
	    returnCh = '<';
	    ptr += 4;
	}
	else if((maxLen > 3) && strncmp(ptr, "&gt;", 4) == 0)
	{
	    returnCh = '>';
	    ptr += 4;
	}
	else if((maxLen > 4) && strncmp(ptr, "&amp;", 5) == 0)
	{
	    returnCh = '&';
	    ptr += 5;
	}
	else
	{
	    returnCh = '&';
	    ptr++;
	}
    }
    return returnCh;
}

GetStringResult
XMLParser::GetString(const char*& ptr, const char* end, 
		      char*& val, UINT32 type)
{
    GetStringResult retval = GSInvalid;

    CHXXMLEncode xmlStr(m_pEncoding, (BYTE*)ptr, end - ptr);
    UINT16 uLen = 0;
    ptr = (const char*)xmlStr.GetNextChar(uLen);
    while(isspace(*ptr) && ptr < end)
    {
	ptr = (const char*)xmlStr.GetNextChar(uLen);
    }
    if((const char*)ptr >= end)
    {
	return GSNoValue;
    }
    if(*ptr == '>')
    {
	ptr = (const char*)xmlStr.GetNextChar(uLen);
	return GSNoValue;
    }
    if(*ptr == '/' && *(ptr + 1) == '>')
    {
	xmlStr += 2;
	ptr = (const char*)xmlStr++;
	return GSNoValue;
    }

    // temp buffer to copy string value
    char* pVal = new char[end - ptr + 1];
    char* pValPtr = pVal;
    char* pValStartPtr = pVal;

    switch(type)
    {
	case TagType:
	{
	    // The main tag name, delimited by space
	    if(*ptr == '/')
	    {
		retval = GSEndTag;
		pValStartPtr++;
	    }
	    while(!isspace(*ptr) && *ptr != '>' && ptr < end)
	    {
		*pValPtr++ = *ptr;
		if(uLen == 2)
		{
		    *pValPtr++ = *(ptr + 1);
		}
		ptr = (const char*)xmlStr.GetNextChar(uLen);
	    }
	    break;
	}
	case AttributeName:
	{
	    // Delimited by whitespace or =
	    while(!isspace(*ptr) && *ptr != '=' && *ptr != '>' && ptr < end)
	    {
		*pValPtr++ = *ptr;
		if(uLen == 2)
		{
		    *pValPtr++ = *(ptr + 1);
		}
		ptr = (const char*)xmlStr.GetNextChar(uLen);
	    }
	    HXBOOL foundequals = FALSE;
	    if(ptr < end)
	    {
		// Set the ptr to past the =
		while((isspace(*ptr) || *ptr == '=') && ptr < end)
		{
		    if(*ptr == '=')
			foundequals=TRUE;
		    ptr = (const char*)xmlStr.GetNextChar(uLen);
		}
	    }
	    if(!foundequals)
	    {
		retval = GSValueOnly;
	    }
	    break;
	}
	case AttributeValue:
	case AttributeValueNoQuote:
	case AttributeValueDirective:
	{
	    if(*ptr == '"')
	    {
		ptr = (const char*)xmlStr.GetNextChar(uLen);
		while(ptr<end && *ptr != '"')
		{
		    if(*ptr == '&')
		    {
			*pValPtr = GetEscapeMacro(ptr, end);
			pValPtr++;
			xmlStr.SetCurrent((BYTE*)ptr);
		    }
		    else
		    {
			*pValPtr++ = *ptr;
			if(uLen == 2)
			{
			    *pValPtr++ = *(ptr + 1);
			}
		    }
		    ptr = (const char*)xmlStr.GetNextChar(uLen);
		}
		if(*ptr != '"')
		{
		    return GSMissingQuote;
		}
		/* Skip the quote */
		ptr = (const char*)xmlStr.GetNextChar(uLen);
		//Fixes 28799 (which is really an XML authoring error)
		// if m_bXMLandSMIL10FullCompliance is FALSE:
		if (m_bXMLandSMIL10FullCompliance  &&
			!isspace(*ptr)  &&  '>' != *ptr  &&
			(('/' != *ptr  &&  '?' != *ptr)
			||  '>' != *(ptr+1)) )
		{
		    //[SMIL 1.0 Compliance] Fixes PR 23995.  Junk following a
		    // name="value" construct should be treated as an error,
		    // e.g., the comma should be treated as an error in the
		    // following: <region height="10", width="20"/>
		    return GSInvalid;
		}
	    }
	    else if(*ptr == '\'')
	    {
		ptr = (const char*)xmlStr.GetNextChar(uLen);
		while(*ptr != '\'' && ptr < end)
		{
		    if(*ptr == '&')
		    {
			*pValPtr = GetEscapeMacro(ptr, end);
			pValPtr++;
			xmlStr.SetCurrent((BYTE*)ptr);
		    }
		    else
		    {
			*pValPtr++ = *ptr;
			if(uLen == 2)
			{
			    *pValPtr++ = *(ptr + 1);
			}
		    }
		    ptr = (const char*)xmlStr.GetNextChar(uLen);
		}
		if(*ptr != '\'')
		{
		    delete [] pVal;
		    return GSMissingQuote;
		}
		/* Skip the quote */
		ptr = (const char*)xmlStr.GetNextChar(uLen);
	    }
	    else if(*ptr == '[' && type == AttributeValueDirective)
	    {
		ptr = (const char*)xmlStr.GetNextChar(uLen);
		while(*ptr != ']' && ptr < end)
		{
		    *pValPtr++ = *ptr;
		    if(uLen == 2)
		    {
			*pValPtr++ = *(ptr + 1);
		    }
		    ptr = (const char*)xmlStr.GetNextChar(uLen);
		}
		if(*ptr != ']')
		{
		    delete[] pVal;
		    return GSMissingQuote;
		}
		/* skip the ']' */
		ptr = (const char*)xmlStr.GetNextChar(uLen);
	    }
	    else
	    {
		if(m_bStrictCompliance && 
		   type != AttributeValueNoQuote &&
		   type != AttributeValueDirective)
		{
		    /* error - value must be quoted */
		    delete [] pVal;
		    return GSMissingQuote;
		}
		else
		{
		    /* don't care!!! */
		    while(!isspace(*ptr) && *ptr != '>' && ptr < end)
		    {
			*pValPtr++ = *ptr;
			if(uLen == 2)
			{
			    *pValPtr++ = *(ptr + 1);
			}
			ptr = (const char*)xmlStr.GetNextChar(uLen);
		    }
		}
	    }
	    break;
	}
    }

    *pValPtr = '\0';

    val = new_string(pValStartPtr);
    delete [] pVal;

    if(retval == GSInvalid)
	return GSFoundExpected;
    else
	return retval;
}

void
XMLParser::FindCommentClose(const char*& buf, const char* start,
			    const char* end)
{
    UINT16 nCommentDepth = 1;

    CHXXMLEncode xmlStr(m_pEncoding, (BYTE*)start, end - start);
    UINT16 uLen = 0;

    const char* pos = (const char*)xmlStr.GetNextChar(uLen);
    while(pos < end && m_comment_state > 0)
    {
	switch(m_comment_state)
	{
	    case 1:
		if(*pos == '-')
		    m_comment_state = 2;
		else if (*pos == '<')
		    m_comment_state = 4;
		else if (*pos == '>' && m_bAllowNonXMLComments)
		{
		    nCommentDepth--;
		    if (nCommentDepth == 0)
		    {
			m_comment_state = 0;
			buf = (const char*)xmlStr.GetNextChar(uLen);
		    }
		    else
			m_comment_state = 1;
		}
		else if(m_comment_start)
		{
		    if(*pos == '#')
		    {
			if(end - pos < 8)
			{
			    buf = pos;
			    return;
			}
			pos = (const char*)xmlStr.GetNextChar(uLen);
			if(strncasecmp(pos, "include", 7) == 0)
			{
			    pos += 7;
			    m_comment_get_arg = 1;
			    m_comment_pos = 0;
			    strcpy(m_comment_command, "include");
                            while ((pos = (const char*)xmlStr.GetNextChar(uLen)) && *pos != 'e')
                                ;       // Skip include
                            pos = (const char*)xmlStr.GetNextChar(uLen); // Skip the 'e'
			}
		    }
		}
		break;
	    case 2:
		if(*pos == '-')
		    m_comment_state = 3;
		else
		    m_comment_state = 1;
		break;
	    case 3:
		if(*pos == '>')
		{
		    nCommentDepth--;
		    if (nCommentDepth == 0)
		    {
			m_comment_state = 0;
			buf = (const char*)xmlStr.GetNextChar(uLen);
		    }
		    else
			m_comment_state = 1;
		}
		else
		    m_comment_state = 1;
		break;
	    case 4:
		// Ignore nested comments while looking for our end tag
		if (*pos == '!')
		    m_comment_state = 5;
		else
		    m_comment_state = 1;
		break;
	    case 5:
		if (*pos == '-')
		    m_comment_state = 6;
		else
		    m_comment_state = 1;
		break;
	    case 6:
		if (*pos == '-')
		{
		    nCommentDepth++;
		}
		m_comment_state = 1;
		break;
	}
	if(m_comment_state > 0)
	{
	    switch(m_comment_get_arg)
	    {
		case 1:
		    if(*pos != '"' && !isspace(*pos))
			m_comment_get_arg = 0;
		    else if(*pos == '"')
			m_comment_get_arg = 2;
		    break;
		case 2:
		    if(*pos != '"')
                    {
			if (m_comment_pos < 1023)
                        {
                            m_comment_arg[m_comment_pos++] = *pos;
                        }
                    }
		    else
		    {
			if (m_comment_pos < 1024)
                        {
                            m_comment_arg[m_comment_pos] = 0;
                        }
			m_comment_get_arg = 3;
		    }
		    break;
		default:
		    break;
	    }
	}
	pos = (const char*)xmlStr.GetNextChar(uLen);
    }
}

XMLParseResult
XMLParser::Parse(const char*& buf, UINT32 len, XMLTag*& tag, HXBOOL bIsFinal)
{
    const char* open;
    const char* close;
    const char* cur;
    const char* afterclose;
    
    tag = NULL;

    if(m_comment_state > 0)
    {
	FindCommentClose(buf, buf, buf+len);
	if(m_comment_state != 0)
	{
	    SetError(m_pLastError, XMLErrorNoClose, 0, 0, buf, len, 0);
	    return XMLPNoClose;
	}
	else if(m_comment_get_arg != 3)
	{
	    tag = new XMLTag(m_bStrictCompliance, m_bStoreErrors);
	    tag->new_attribute()->value = new_string("");   // dummy tag
	    return XMLPComment;
	}
	// Got a comment command
	tag = new XMLTag(m_bStrictCompliance, m_bStoreErrors);
	tag->new_attribute()->value = new_string(m_comment_arg);
	tag->m_cur_attribute->name = new_string(m_comment_command);
	return XMLPComment;
    }

    if(*buf != '<')
    {
	// If there isn't a tag right away, tell the user there's just plain
	// text here.
	UINT32 ulLine = 0;
	UINT32 ulCol = 0;
	const char* errPos = NULL;
	UINT32 errLen = 0;
	
	cur = buf;
	while(((UINT32)(cur - buf) < len) && (*cur != '<'))
	{
	    if(*cur == '\n')
	    {
		m_ulCurrentLine++;
		m_ulCurrentCol = 1;
	    }
	    else
	    {
		m_ulCurrentCol++;
	    }

	    if (m_bStoreErrors)
	    {
		// check for ]]>
		// validate refferences.
		if (cur - buf > 3 && *cur == ']' && *(cur+1) == ']' &&
		    *(cur+2) == '>' && !errPos)
		{
		    ulLine = m_ulCurrentLine;
		    ulCol = m_ulCurrentCol;
		    errPos = buf;
		    errLen = len;
		}
	    }
	    cur++;
	}
	tag = new XMLTag(m_bStrictCompliance, m_bStoreErrors);
	char* pText = new char[cur - buf + 1];
	strncpy(pText, buf, cur - buf); /* Flawfinder: ignore */
	pText[cur - buf] = '\0';
	tag->new_attribute()->value = new_string(pText);
	if (errPos)
	{
	    XMLError* err = NULL;
	    SetError(err, XMLErrorInvalidGTafter2RSQB, 
		ulLine, ulCol, errPos, errLen, 0);
	    tag->m_errs->Add(err);
	}
	delete [] pText;
	buf = cur;
	return XMLPPlainText;
    }
    open = buf;
    CHXXMLEncode xmlStr(m_pEncoding, (BYTE*)open, len);
    UINT16 uLen = 0;

    m_ulTagStartLine = m_ulCurrentLine;
    m_ulTagStartCol = m_ulCurrentCol;

    HXBOOL   bInDoubleQuote = FALSE;
    HXBOOL   bInSingleQuote = FALSE;
    HXBOOL   bInComment	  = FALSE;
    HXBOOL   bInDeclaration = FALSE;
    UINT16 nCommentDepth  = 0;
    
    UINT32 ulLine = 0;
    UINT32 ulCol = 0;
    const char* errPos = NULL;
    UINT32 errLen = 0;

    if(*(open+1) && *(open+1) == '!' &&
       *(open+2) && *(open+2) == '-' && 
       *(open+3) && *(open+3) == '-')
    {
	// '<!--' starts a comment
	bInComment = TRUE;
    }
    for(close = (const char*)xmlStr.GetNextChar(uLen); close < buf+len; close = (const char*)xmlStr.GetNextChar(uLen))
    {
	if(*close == '\n')
	{
	    m_ulCurrentLine++;
	    m_ulCurrentCol = 1;
	}
	else
	{
	    m_ulCurrentCol++;
	}
	if(*close == '"' && !bInComment)
	{
	    if(!bInSingleQuote)
	    {
		if(bInDoubleQuote)
		{
		    bInDoubleQuote = FALSE;
		}
		else
		{
		    bInDoubleQuote = TRUE;
		}
	    }
	}
	else if(*close == '\'' && !bInComment)
	{
	    if(!bInDoubleQuote)
	    {
		if(bInSingleQuote)
		{
		    bInSingleQuote = FALSE;
		}
		else
		{
		    bInSingleQuote = TRUE;
		}
	    }
	}
	else if(*close == '[' && !bInDeclaration)
	{
	    bInDeclaration = TRUE;
	}
	else if(*close == ']' && bInDeclaration)
	{
	    bInDeclaration = FALSE;
	}
	// Increase the depth if we find a comment within a comment
	else if(*(close) == '<' && bInComment)
	{
	    if(*(close+1) && *(close+1) == '!' &&
	       *(close+2) && *(close+2) == '-' && 
	       *(close+3) && *(close+3) == '-')
	    {
		// '<!--' starts a comment
		nCommentDepth++;
	    }
	}
	else if(*close == '>')
	{
	    // If we are in a comment, we should only stop at a comment end
	    // (Comments must end with "-->")
	    if (bInComment)
	    {
		if ((!m_bAllowNonXMLComments &&
		     (close - open) > 5      && 
		     *(close-1) == '-'       && 
		     *(close-2) == '-')          ||
		    (m_bAllowNonXMLComments  &&
		     (close - open) > 3))
		{
		    nCommentDepth--;
		    if (!nCommentDepth)
		    {
			break;
		    }
		}
	    }
	    else
	    {
		if (!bInDoubleQuote && !bInSingleQuote && !bInDeclaration)
		{
		    break;
		}
	    }
	}

	if (m_bStoreErrors && bInComment && !errPos)
	{
	    if (*close == '-' && *(close+1) == '-' 
		&& *(close + 2) != '>' && (close - open > 4))
	    {
		ulLine = m_ulCurrentLine;
		ulCol = m_ulCurrentCol;
		errPos = buf;
		errLen = len;
	    }
	}
    }

    if( (close<=buf+len) && *close != '>')
    {
	if(!bIsFinal)
	{
	    return XMLPNotDone;
	}
	SetError(m_pLastError, XMLErrorNoClose, 0, 0, buf, len, 0);
	buf = open;
	return XMLPNoClose;
    }

    afterclose = close+1;

    if(*(open+1) == '!')
    {
	if(*(open+2) == '-' && *(open+3) == '-')
	{
	    // '<!--' starts a comment
	    m_comment_state = 1;
	    m_comment_start = TRUE;
	    m_bCommentWasFound = TRUE;
	    FindCommentClose(buf, open+4, buf + len);
	    if(m_comment_state != 0)
	    {
		SetError(m_pLastError, XMLErrorNoClose, 0, 0, buf, len, 0);
		return XMLPNoClose;
	    }
	    else if(m_comment_get_arg != 3)
	    {
		tag = new XMLTag(m_bStrictCompliance, m_bStoreErrors);
		const char* pBeginComment = open + 4;
		int commentLen = buf - pBeginComment - 3;
		tag->new_attribute()->value = new_string(pBeginComment, commentLen);
		if (errPos)
		{
		    XMLError* err = NULL;
		    SetError(err, XMLErrorTwoDashNotAllowed, ulLine,
			ulCol, errPos, errLen, 0);
		    tag->m_errs->Add(err);
		}
		return XMLPComment;
	    }
	    // Got a comment command
	    tag = new XMLTag(m_bStrictCompliance, m_bStoreErrors);
	    tag->new_attribute()->value = new_string(m_comment_arg);
	    tag->m_cur_attribute->name = new_string(m_comment_command);
	    return XMLPComment;
	}
	XMLParseResult rc = ParseTag(open+1, close, XMLDirectiveTag, tag);
	if(XMLPTag == rc)
	{
	    // TODO - Scan Directive 
	    buf = afterclose;
	    return XMLPDirective;
	}
	else if(XMLPAttributeValueNotQuoted == rc)
	{
	    SetError(m_pLastError, XMLErrorMissingQuote, 0, 0, buf, len, 0);
	}
	else
	{
	    SetError(m_pLastError, XMLErrorNoClose, 0, 0, buf, len, 0);
	}
	buf = afterclose;
	return XMLPBadDirective;
    }
    
    if(*(open + 1) == '?')
    {
	// A Processing Instruction
	XMLParseResult rc = ParseTag(open+1, close, XMLProcInstTag, tag);
	if(XMLPTag == rc)
	{
	    buf = afterclose;
	    if (m_bStrictCompliance)
	    {
		//[SMIL 1.0 Compliance] Fixes PR 9862.  No comment can
		// precede a processor instruction (xml prolog):
		if (m_bCommentWasFound  &&  m_bXMLandSMIL10FullCompliance)
		{
		    SetError(m_pLastError, XMLErrorCommentBeforeProcInst,
			    0, 0, buf, len, 0);
		    return XMLPCommentBeforeProcInst;
		}
		
		if (m_bStoreErrors)
		{
		    XMLError* err = NULL;
		    SetError(err, XMLErrorCommentBeforeProcInst,
			    0, 0, buf, len, 0);
		    tag->m_errs->Add(err);
		}
	    }

	    if (m_bStoreErrors)
	    {
		ScanTag(open+1, close, tag);
	    }
	    
	    return XMLPProcInst;
	}
        SetError(m_pLastError, XMLErrorNoClose, 0, 0, buf, len, 0);
	return XMLPBadProcInst;
    }
    // just a plain old tag
    XMLParseResult rc = ParseTag(open, close, XMLPlainTag, tag);
    if(XMLPTag == rc)
    {
	buf = afterclose;
	if (m_bStoreErrors)
	{
	    ScanTag(open, close, tag);
	}
	return XMLPTag;
    }
    else if(XMLPBadEndTag == rc)
    {
	if(m_pCurrentFrame && m_pCurrentFrame->name)
	{
	    SetError(m_pLastError, XMLErrorBadEndTag, 0, 0, buf, len, m_pCurrentFrame->name);
	}
	else
	{
	    SetError(m_pLastError, XMLErrorBadEndTag, 0, 0, buf, len, 0);
	}
    }
    else if(XMLPBadAttribute == rc)
    {
	SetError(m_pLastError, XMLErrorBadAttribute, 0, 0, buf, len, 0);
    }
    else if(XMLPAttributeValueNotQuoted == rc)
    {
	SetError(m_pLastError, XMLErrorMissingQuote, 0, 0, buf, len, 0);
    }
    else if(XMLPDupAttribute == rc)
    {
	SetError(m_pLastError, XMLErrorDupAttribute, 0, 0, buf, len, 0);
    }
    else
    {
	SetError(m_pLastError, XMLUnknownError, 0, 0, buf, len, 0);
    }

    return rc;
}

XMLParseResult
XMLParser::ParseTag(const char* open, const char* close, XMLTagType tType, XMLTag*& tag)
{
    const char* cur = open+1;
    HXBOOL bHasAttributeNames = TRUE;
    HXBOOL bUseNonQuotedValues = FALSE;
    HXBOOL bHasDirectives = FALSE;

    tag = new XMLTag(m_bStrictCompliance, m_bStoreErrors);

    switch(tType)
    {
	case XMLPlainTag:
	{
	    if(*(close - 1) == '/')
	    {
		tag->m_need_close = FALSE;
		close--;
	    }
	}
	break;
	case XMLProcInstTag:
	{
	    tag->m_need_close = FALSE;
	    if(*(close - 1) == '?')
	    {
		close--;
	    }
	}
	break;
	case XMLDirectiveTag:
	{
	    bHasAttributeNames = FALSE;
	    bUseNonQuotedValues = TRUE;
	    bHasDirectives = TRUE;
	    tag->m_need_close = FALSE;
	}
	break;
	default:
	{
	    tag->m_need_close = FALSE;
	}
	break;
    }
    tag->m_type = tType;

    GetStringResult res = GetString(cur, close, tag->m_name, TagType);
    if(res == GSEndTag)
    {
	tag->m_type = XMLEndTag;
	tag->m_need_close = FALSE;
	if(!m_pCurrentFrame ||
	    !m_pCurrentFrame->name)
	{
	    return XMLPBadEndTag;
	}

	if(m_bStrictCompliance)
	{
	    if(strcmp(tag->m_name, m_pCurrentFrame->name) != 0)
	    {
		return XMLPBadEndTag;
	    }
	}
	else
	{
	    if(strcasecmp(tag->m_name, m_pCurrentFrame->name) != 0)
	    {
		return XMLPBadEndTag;
	    }
	}
	tag->elem = m_pCurrentFrame->elemcount;
	if(m_pCurrentFrame)
	    delete m_pCurrentFrame;
	m_pCurrentFrame = (XMLFrame*)m_pStack.Pop();
	return XMLPTag;
    }
    else if(res == GSMissingQuote)
    {
	delete tag;
	tag = NULL;
	return XMLPAttributeValueNotQuoted;
    }
    else if(tag->m_name && tag->m_need_close)
    {
	tag->elem = m_pCurrentFrame->elemcount++;

	XMLFrame* frame = new XMLFrame;
	frame->elemcount = 0;
	frame->name = new_string(tag->m_name);
	m_pStack.Push(m_pCurrentFrame);
	m_pCurrentFrame = frame;
    }
    else
    {
	tag->elem = m_pCurrentFrame->elemcount++;
    }

    if(GSFoundExpected != res)
    {
	delete tag;
	tag = NULL;
	return XMLPNoTagType;
    }
    else
    {
	while(cur < close)
	{
	    if(bHasAttributeNames)
	    {
		GetStringResult res = GetString(cur, close, 
						tag->new_attribute()->name,
						AttributeName);
		if(res == GSNoValue)
		{
		    delete tag->m_cur_attribute;
		    tag->m_numAttributes--;
		    break;
		}
		switch(res)
		{
		    case GSValueOnly:
			// The user of this parser will fill in the name of this
			// attribute
			tag->m_cur_attribute->value = tag->m_cur_attribute->name;
			tag->m_cur_attribute->name = NULL;
			continue;
		    case GSFoundExpected:
			break;
		    default:
			delete tag;
			tag = NULL;
			return XMLPBadAttribute;
		}
	    }
	    else
	    {
		tag->new_attribute()->name = 0;
	    }

	    if(bUseNonQuotedValues)
	    {
		if(bHasDirectives)
		{
		    res = GetString(cur, close,
				    tag->m_cur_attribute->value,
				    AttributeValueDirective);
		}
		else
		{
		    res = GetString(cur, close,
				    tag->m_cur_attribute->value,
				    AttributeValueNoQuote);
		}
	    }
	    else
	    {
		res = GetString(cur, close,
				tag->m_cur_attribute->value,
				AttributeValue);
	    }
	    if(res == GSMissingQuote)
	    {
		delete tag;
		tag = NULL;
		return XMLPAttributeValueNotQuoted;
	    }
	    else if(res != GSFoundExpected)
	    {
		delete tag;
		tag = NULL;
		return XMLPBadAttribute;
	    }
	}
    }

    if(m_bStrictCompliance)
    {
	// error on duplicate attributes
	CHXMapStringToOb dupMap;
	HXBOOL bDupFound = FALSE;
	XMLAttribute* pAttr = NULL;
	for(UINT32 i=0;i<tag->m_numAttributes;++i)
	{
	    pAttr = tag->attribute(i);

	    void* pLookupValue = NULL;
	    if(pAttr->name)
	    {
		if(dupMap.Lookup(pAttr->name, pLookupValue))
		{
		    bDupFound = TRUE;
		    break;
		}
		else
		{
		    dupMap.SetAt(pAttr->name, NULL);
		}
	    }
	}
	if(bDupFound)
	{
#if defined(XXXEH_CHECK_THIS_IN_AFTER_U2_RELEASE)
	    return XMLPDupAttribute;
#else /* XXXEH- Back out BAB's fix for PR 9172 because it breaks a lot of
       * old content (this rebreaks PR 9172 and fixes PR 12447)
       */
	    HX_ASSERT(1); //line exists only for setting a breakpoint.
#endif
	}
    }

    return XMLPTag;
}



XMLParseResult
XMLParser::ScanTag(const char* open, const char* close, XMLTag* tag)
{
    const char* cur = open+1;
    char cQuote = '"';

    CHXXMLEncode xmlStr(m_pEncoding, (BYTE*)cur, close - cur);
    UINT16 uLen = 0;

    //tag->m_need_close
    //tag->m_type;


    // check the spacing....
    switch (tag->m_type)
    {
    case XMLEndTag:
	{
	    // scan name...
	    if (!xmlStr.IsNameValid((const BYTE*)tag->m_name, strlen(tag->m_name)))
	    {
		XMLError* err = NULL;
		SetError(err, XMLErrorInvalidName, m_ulTagStartLine, m_ulTagStartCol, 
			    tag->m_name, strlen(tag->m_name), 0);
		tag->m_errs->Add(err);
	    }
	}
	break;
    case XMLPlainTag:
    case XMLProcInstTag:
	{
	    // scan tag
	    if (!xmlStr.IsNameValid((const BYTE*)tag->m_name, strlen(tag->m_name)))
	    {
		XMLError* err = NULL;
		SetError(err, XMLErrorInvalidName, m_ulTagStartLine, m_ulTagStartCol, 
			    tag->m_name, strlen(tag->m_name), 0);
		tag->m_errs->Add(err);
	    }

	    // check PI name
	    //if (!xmlStr.IsPINameValid(tag->m_name, strlen(tag->m_name)))
	    //{
	//	XMLError* err = NULL;
	//	SetError(err, XMLErrorInvalidPITarget, m_ulTagStartLine, m_ulTagStartCol, 
	//		    tag->m_name, strlen(tag->m_name), 0);
	//	tag->m_errs->Add(err);
	  //  }

	    // check the spacing.
	    enum { InTagName, InTag, InBeginAttributeName, InAttributeName, 
		InEndAttributeName, InBeginAttributeValue, InAttributeValue, Done } state;
	    state = InTagName;
	    
	    for (const char* pos = cur; *pos && pos < close && state != Done;
		pos = (const char*)xmlStr.GetNextChar(uLen))
	    {
		switch (state)
		{
		case InTagName:
		    {
			// go to first white space
			if ( isspace(*pos) )
			{
			    state = InBeginAttributeName;
			}
			else if ( *pos == '>' )
			{
			    // done
			    state = Done;
			}
		    }
		    break;
		case InTag:
		    {
			if ( *pos == '>' || (*pos == '/' && 
			    *(pos+1) == '>'))
			{
			    // done.
			    state = Done;
			}
			else
			{
			    // grab the first char... keep it and switch states.
			    // it should be a space... 
			    state = InBeginAttributeName;
			    if (!isspace(*pos))
			    {
				XMLError* err = NULL;
				SetError(err, XMLErrorMissingReqSpace, m_ulTagStartLine, m_ulTagStartCol, 
					    tag->m_name, strlen(tag->m_name), 0);
				tag->m_errs->Add(err);
			    }
			}
		    }
		    break;
		case InBeginAttributeName:
		    {
			if ( isspace(*pos) )
			{
			    // continue...
			}
			else if ( *pos == '=' )
			{
			    XMLError* err = NULL;
			    SetError(err, XMLErrorMissingEquals, m_ulTagStartLine, m_ulTagStartCol, 
					tag->m_name, strlen(tag->m_name), 0);
			    tag->m_errs->Add(err);
			    state = InBeginAttributeValue;
			}
			else if ( *pos == '>' || (*pos == '/' 
			    && *(pos+1) == '>'))
			{
			    // done
			    state = Done;
			}
			else
			{
			    state = InAttributeName;
			}
		    }
		    break;
		case InAttributeName:
		    {
			if ( isspace(*pos) )
			{
			    state = InEndAttributeName;
			}
			else if ( *pos == '=' )
			{
			    state = InBeginAttributeValue;
			}
			else if ( *pos == '>' )
			{
			    XMLError* err = NULL;
			    SetError(err, XMLErrorMissingEquals, m_ulTagStartLine, m_ulTagStartCol, 
					tag->m_name, strlen(tag->m_name), 0);
			    
			    tag->m_errs->Add(err);
			    // done
			    state = Done;
			}
			else if (*pos == '\'' || *pos == '"')
			{
			    XMLError* err = NULL;
			    SetError(err, XMLErrorBadAttribute, m_ulTagStartLine, m_ulTagStartCol, 
					tag->m_name, strlen(tag->m_name), 0);
			    tag->m_errs->Add(err);

			    cQuote = *pos;
			    state = InAttributeValue;
			}
		    }
		    break;
		case InEndAttributeName:
		    {
			if ( isspace(*pos) )
			{
			    // continue..
			}
			else if ( *pos == '=' )
			{
			    state = InBeginAttributeValue;
			}
			else if ( *pos == '>' )
			{
			    XMLError* err = NULL;
			    SetError(err, XMLErrorMissingEquals, m_ulTagStartLine, m_ulTagStartCol, 
					tag->m_name, strlen(tag->m_name), 0);
			    tag->m_errs->Add(err);
			    state = Done;
			}
			else
			{
			    // hmm. we got a non whitespace before the =

			    //First, let's see if we have a ["] or a [']
			    // (i.e., an attribute value start) in which
			    // case the author must have forgotten to
			    // put an '=' between the name/value pair.
			    // In this case, we need to keep the renderers
			    // from firing off an error with old bad content,
			    // so we pretend we're in the "InAttributeValue"
			    // state:
			    if ( *pos == '\'' )
			    {
				XMLError* err = NULL;
				SetError(err, XMLErrorBadAttribute, m_ulTagStartLine, m_ulTagStartCol, 
					    tag->m_name, strlen(tag->m_name), 0);
				tag->m_errs->Add(err);
				cQuote = *pos;
				state = InAttributeValue;
			    }
			    else if ( *pos == '\"' )
			    {
				XMLError* err = NULL;
				SetError(err, XMLErrorBadAttribute, m_ulTagStartLine, m_ulTagStartCol, 
					    tag->m_name, strlen(tag->m_name), 0);
				tag->m_errs->Add(err);
				cQuote = *pos;
				state = InAttributeValue;
			    }
			    else
			    {
				XMLError* err = NULL;
				SetError(err, XMLErrorBadAttribute, m_ulTagStartLine, m_ulTagStartCol, 
						tag->m_name, strlen(tag->m_name), 0);
				tag->m_errs->Add(err);
				// lets go back to the attribute name state.
				state = InAttributeName;
			    }
			}
		    }
		    break;
		case InBeginAttributeValue:
		    {
			if ( isspace(*pos) )
			{
			}
			else if ( *pos == '\'' || *pos == '\"')
			{
			    cQuote = *pos;
			    state = InAttributeValue;
			}
			else if ( *pos == '>' )
			{
			    XMLError* err = NULL;
			    SetError(err, XMLErrorMissingEquals, m_ulTagStartLine, m_ulTagStartCol, 
					tag->m_name, strlen(tag->m_name), 0);
			    tag->m_errs->Add(err);
			    // done
			    state = Done;
			}
		    }
		    break;
		case InAttributeValue:
		    {
			if ( *pos == cQuote )
			{
			    state = InTag;
			}
		    }
		    break;
		case Done:
			break;
		default:
			HX_ASSERT(0);
		}
	    }

	}
	break;
    case XMLCommentTag:
	{
	    // we will not scan...
	}
	break;
    case XMLDirectiveTag:
	{
	    // TODO: scan Directive.
	}
	break;
    }

    // error on duplicate attributes
    // also validate the names and attributes.
    CHXMapStringToOb dupMap;
    HXBOOL bDupFound = FALSE;
    XMLAttribute* pAttr = NULL;
    const char* name = NULL;
    for(UINT32 i=0;i<tag->m_numAttributes;++i)
    {
	pAttr = tag->attribute(i);

	if (!xmlStr.IsNameValid((const BYTE*)pAttr->name, strlen(pAttr->name)))
	{
	    XMLError* err = NULL;
	    SetError(err, XMLErrorInvalidName, m_ulTagStartLine, m_ulTagStartCol, 
		    pAttr->name, strlen(pAttr->name), 0);
	    tag->m_errs->Add(err);
	}
	if (!xmlStr.IsAttValueValid((const BYTE*)pAttr->value, strlen(pAttr->value)))
	{
	    XMLError* err = NULL;
	    SetError(err, XMLErrorInvalidCharInDoc, m_ulTagStartLine, m_ulTagStartCol, 
		    pAttr->value, strlen(pAttr->value), 0);
	    tag->m_errs->Add(err);
	}


	void* pLookupValue = NULL;
	if(pAttr->name)
	{
	    if(dupMap.Lookup(pAttr->name, pLookupValue))
	    {
		name = pAttr->name;
		bDupFound = TRUE;
		break;
	    }
	    else
	    {
		dupMap.SetAt(pAttr->name, NULL);
	    }
	}
    }

    if	(bDupFound)
    {	
	XMLError* err = NULL;
	SetError(err, XMLErrorDupAttribute, m_ulTagStartLine, m_ulTagStartCol, 
		name, strlen(name), 0);
	tag->m_errs->Add(err);
    }

    return XMLPTag;
}
    
void XMLParser::SetEncoding(const char* pszEncoding)
{
    if (pszEncoding)
    {
        INT32 lLen = strlen(pszEncoding);
        if (lLen > 0)
        {
            HX_VECTOR_DELETE(m_pEncoding);
            m_pEncoding = new char [lLen + 1];
            if (m_pEncoding)
            {
                strcpy(m_pEncoding, pszEncoding);
            }
        }
    }
}

HX_RESULT XMLParser::GetEncoding(REF(char*) rpszEncoding)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pEncoding)
    {
        HX_VECTOR_DELETE(rpszEncoding);
        rpszEncoding = new char [strlen(m_pEncoding) + 1];
        if (rpszEncoding)
        {
            strcpy(rpszEncoding, m_pEncoding);
            retVal = HXR_OK;
        }
    }

    return retVal;
}

void
XMLParser::SetError(REF(XMLError*) pErr, XMLErrorTag tag, INT32 lLine,
    INT32 lPos, const char* pErrorText, INT32 lErrorTextLen,
    const char* pFrameText)
{
    HX_DELETE(m_pLastError);

    INT32 lTextLen = 
	(lErrorTextLen > MAX_ERROR_LEN) ? MAX_ERROR_LEN: lErrorTextLen;
    char tmpBuf[MAX_ERROR_LEN * 2]; // overdo it a bit...

    // convert control characters to spaces
    INT32 j = 0;
    for(INT32 i = 0; i < lTextLen; ++i)
    {
	if(iscntrl(pErrorText[i]))
	{
	    tmpBuf[j++] = ' ';
	}
	else
	{
	    tmpBuf[j++] = pErrorText[i];
	}
    }
    tmpBuf[j] = 0;

    pErr = new XMLError(tag, lLine, lPos, tmpBuf, pFrameText);
}

XMLTag::XMLTag(HXBOOL bStrictCompliance, HXBOOL bStoreErrors):
    m_bStrictCompliance(bStrictCompliance)
{
    m_numAttributes = 0;
    m_name = NULL;
    m_type = XMLPlainTag;
    m_need_close = TRUE;
    m_errs = NULL;

    if (bStoreErrors)
    {
	m_errs = new CHXPtrArray();
    }
}

XMLTag::~XMLTag()
{
    UINT32 i;
    for(i = 0; i < m_numAttributes; i++)
    {
	delete (XMLAttribute*)m_attributes[(int)i];
    }

    HX_VECTOR_DELETE(m_name);

    if (m_errs)
    {
	UINT32 size = m_errs->GetSize();
	for(i = 0; i < size; i++)
	{
	    delete (XMLError*)(*m_errs)[(int)i];
	}
	HX_DELETE(m_errs);
    }
}

XMLAttribute*
XMLTag::new_attribute()
{
    m_cur_attribute = new XMLAttribute;
    
    m_attributes.SetAtGrow((int)m_numAttributes, m_cur_attribute);
    m_numAttributes++;

    return m_cur_attribute;
}

const char*
XMLTag::get_attribute(const char* name)
{
    for(UINT32 i = 0; i < m_numAttributes; i++)
    {
	if(((XMLAttribute*)m_attributes[(int)i])->name)
	{
	    if(m_bStrictCompliance)
	    {
		if(strcmp(((XMLAttribute*)m_attributes[(int)i])->name, name) == 0)
		{
		    return (const char*)((XMLAttribute*)m_attributes[(int)i])->value;
		}
	    }
	    else
	    {
		if(strcasecmp(((XMLAttribute*)m_attributes[(int)i])->name, name) == 0)
		{
		    return (const char*)((XMLAttribute*)m_attributes[(int)i])->value;
		}
	    }
	}
    }
    return NULL;
}


/*
 * XMLError methods
 */

XMLError::XMLError(XMLErrorTag errorTag,
		   INT32 lLineNumber,
		   INT32 lLinePosition,
		   const char* pErrorString,
		   const char* pFrameString):
    m_pErrorString(0),
    m_pFrameString(0),
    m_lLineNumber(lLineNumber),
    m_lLinePosition(lLinePosition),
    m_errorTag(errorTag)
{
    if(pErrorString)
    {
	m_pErrorString = new_string(pErrorString);
    }
    if(pFrameString)
    {
	m_pFrameString = new_string(pFrameString);
    }
}

XMLError::~XMLError()
{
    delete[] m_pErrorString;
    delete[] m_pFrameString;
}

