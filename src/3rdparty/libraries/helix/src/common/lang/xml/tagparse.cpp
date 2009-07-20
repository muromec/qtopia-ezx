/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tagparse.cpp,v 1.8 2007/07/06 20:43:45 jfinnecy Exp $
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

/*
 *      XML tag parser
 *
 *	Not to be confused with the full XML parser, this parser will only
 *      tell you what tags appear in a document, and will parse their
 *      attributes for you. It will not perform any validation.
 */

#include <ctype.h>

#include "hxcom.h"
#include "hxtypes.h"
#include "hxstrutl.h"
#include "hxmap.h"
#include "xmlencod.h"
#include "looseprs.h"
#include "tagparse.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


XMLTagParser::XMLTagParser(const char* pEncoding)
    : m_comment_state(0)
    , m_comment_get_arg(0)
    , m_comment_pos(0)
{
    if(pEncoding)
    {
	m_pEncoding = new_string(pEncoding);
    }
    else
    {
	m_pEncoding = new_string("US-ASCII");	// default encoding
    }
}

XMLTagParser::~XMLTagParser()
{
    HX_DELETE(m_pEncoding);
}

XMLParseResult
XMLTagParser::Parse(const char*& buf, 
		    UINT32 len, 
		    XMLTag*& tag)
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
	    return XMLPNoClose;
	}
	else if(m_comment_get_arg != 3)
	{
	    tag = new XMLTag(FALSE);
	    tag->new_attribute()->value = new_string("");   // dummy tag
	    return XMLPComment;
	}
	// Got a comment command
	tag = new XMLTag(FALSE);
	tag->new_attribute()->value = new_string(m_comment_arg);
	tag->m_cur_attribute->name = new_string(m_comment_command);
	return XMLPComment;
    }

    if(*buf != '<')
    {
	// If there isn't a tag right away, tell the user there's just plain
	// text here.
	cur = buf;
	while(((UINT32)(cur - buf) < len) && (*cur != '<'))
	{
	    cur++;
	}
	tag = new XMLTag(FALSE);
	char* pText = new char[cur - buf + 1];
	strncpy(pText, buf, cur - buf); /* Flawfinder: ignore */
	pText[cur - buf] = '\0';
	tag->new_attribute()->value = new_string(pText);
	delete [] pText;
	buf = cur;
	return XMLPPlainText;
    }
    open = buf;

    HXBOOL   bInDoubleQuote = FALSE;
    HXBOOL   bInSingleQuote = FALSE;
    HXBOOL   bInComment	  = FALSE;
    HXBOOL   bInDeclaration = FALSE;
    UINT16 nCommentDepth  = 0;
    if(*(open+1) && *(open+1) == '!' &&
       *(open+2) && *(open+2) == '-' && 
       *(open+3) && *(open+3) == '-')
    {
	// '<!--' starts a comment
	bInComment = TRUE;
    }
    for(close = open; close < buf+len; close++)
    {
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
		if ((close - open) > 5 && 
		    *(close-1) == '-'  && 
		    *(close-2) == '-')
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
    }

    if(*close != '>')
    {
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
	    FindCommentClose(buf, open+4, buf + len);
	    if(m_comment_state != 0)
	    {
		return XMLPNoClose;
	    }
	    else if(m_comment_get_arg != 3)
	    {
		tag = new XMLTag(FALSE);
		const char* pBeginComment = open + 4;
		int commentLen = buf - pBeginComment - 3;
		tag->new_attribute()->value = new_string(pBeginComment, commentLen);
		return XMLPComment;
	    }
	    // Got a comment command
	    tag = new XMLTag(FALSE);
	    tag->new_attribute()->value = new_string(m_comment_arg);
	    tag->m_cur_attribute->name = new_string(m_comment_command);
	    return XMLPComment;
	}		
	XMLParseResult rc = ParseTag(open+1, close, XMLDirectiveTag, tag);
	if(XMLPTag == rc)
	{
	    buf = afterclose;
	    return XMLPDirective;
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
	    return XMLPProcInst;
	}
	return XMLPBadProcInst;
    }

    // Just a plain old tag
    XMLParseResult rc = ParseTag(open, close, XMLPlainTag, tag);
    if(XMLPTag == rc)
    {
	buf = afterclose;
	return XMLPTag;
    }

    return rc;
}

XMLParseResult
XMLTagParser::ParseTag(const char* open, 
		       const char* close, 
		       XMLTagType tType, 
		       XMLTag*& tag)
{
    const char* cur = open+1;
    HXBOOL bHasAttributeNames = TRUE;
    HXBOOL bUseNonQuotedValues = TRUE;
    HXBOOL bHasDirectives = FALSE;

    tag = new XMLTag(FALSE);

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

	return XMLPTag;
    }
    else if(res == GSMissingQuote)
    {
	delete tag;
	tag = NULL;
	return XMLPAttributeValueNotQuoted;
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

    return XMLPTag;
}

GetStringResult
XMLTagParser::GetString(const char*& ptr, 
			const char* end, 
		        char*& val, 
			UINT32 type)
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
		while(*ptr != '"' && ptr < end)
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
XMLTagParser::FindCommentClose(const char*& buf, 
			       const char* start,
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
			    strcpy(m_comment_command, "include"); /* Flawfinder: ignore */
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
			if (m_comment_pos < 1023) m_comment_arg[m_comment_pos++] = *pos;
		    else
		    {
			if (m_comment_pos < 1024) m_comment_arg[m_comment_pos] = 0;
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

char
XMLTagParser::GetEscapeMacro(const char*& ptr, const char* end)
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

