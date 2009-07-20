/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mimescan.cpp,v 1.11 2007/07/06 20:51:32 jfinnecy Exp $
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

#include "hlxclib/ctype.h"
//#include "hlxclib/string.h"

#if !defined(_WINDOWS) && !defined(_OPENWAVE)
#include <unistd.h>	// not supported by VC++
#endif

#include "mimescan.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


MIMEInputStream::MIMEInputStream(const char* pBuf, UINT32 nBufSize):
    m_nBufSize(nBufSize),
    m_nCurOffset(0),
    m_bUndoValid(FALSE),
    m_nUndo(-1)
{
    m_pBuffer = new char[m_nBufSize];
    if( m_pBuffer )
    {
        memcpy(m_pBuffer, pBuf, HX_SAFESIZE_T(m_nBufSize)); /* Flawfinder: ignore */
    }
}

MIMEInputStream::MIMEInputStream(const CHXString& str)
{
    const char* pBuf = (const char*)str;
    m_nCurOffset = 0;
    m_nBufSize = str.GetLength();
    m_pBuffer = new char[m_nBufSize];
    if( m_pBuffer )
    {
        memcpy(m_pBuffer, pBuf, HX_SAFESIZE_T(m_nBufSize)); /* Flawfinder: ignore */
    }
    m_bUndoValid = FALSE;
    m_nUndo = -1;
}

MIMEInputStream::~MIMEInputStream()
{
    HX_VECTOR_DELETE(m_pBuffer);
}

int 
MIMEInputStream::read()
{
    if(m_bUndoValid)
    {
	int rc = m_nUndo;
	m_bUndoValid = FALSE;
	m_nUndo = -1;
	return rc;
    }

    if(m_nCurOffset < m_nBufSize)
    {
	int chRet = m_pBuffer[m_nCurOffset];

	//XXXkshoop treat a EOF in the string as an end of file.
	// never read beyond it. don't inc the offset.
	if( chRet != -1)
	{
	    chRet = (int)(unsigned char)(m_pBuffer[m_nCurOffset]);
	    ++m_nCurOffset;
	    return chRet;
	}
    }

    return -1;
}

int
MIMEInputStream::peek()
{
    if(m_bUndoValid)
    {
	return m_nUndo;
    }
    //XXXkshoop ignore whether the location contains EOF 
    // since we don't inc the offset.
    if(m_nCurOffset < m_nBufSize)
    {
	return m_pBuffer[m_nCurOffset];
    }
    return -1;
}

int 
MIMEInputStream::read(char* pBuf, UINT32 nLen)
{
    UINT32 offset = 0;
    int ch;

    while((offset < nLen) && (ch = read()) >= 0)
	pBuf[offset++] = (char)ch;
    return HX_SAFEINT(offset);
}

UINT32 
MIMEInputStream::available()
{
    return m_nBufSize - m_nCurOffset;
}

UINT32
MIMEInputStream::max_distance_to(char* p)
{
    UINT32 ulDist = 0;
    if (m_bUndoValid)
    {
	if (strchr(p, m_nUndo) || m_nUndo == -1)
	{
	    return 0;
	}
	ulDist++;
    }
    UINT32 ulTempIndex = m_nCurOffset;
    while (ulTempIndex < m_nBufSize &&
	!strchr(p, m_pBuffer[ulTempIndex]) &&
	m_pBuffer[ulTempIndex] != 0xff)
    {
	ulDist++;
	ulTempIndex++;
    }
    return  ulDist;
}

MIMEScanner::MIMEScanner(MIMEInputStream& input): m_input(input)
{
}

MIMEScanner::~MIMEScanner()
{
}

static const char* const tspecials = " \t=:;,-";

MIMEToken 
MIMEScanner::nextToken(char* upTo)
{
    int bInQuote = 0;	// are we in a quoted string?
    m_tokstr = "";

    skipWS();

    /*
     * if a token end character is specified, ignore
     * the tspecials, handle '\n' as a special case
     * since lines can be continued by using leading
     * white space on the next line.
     */ 
    if(upTo)
    {
	m_tokstr.SetMinBufSize((INT32)m_input.max_distance_to(upTo));
	int ch = m_input.read();
	while(ch != -1)
	{
	    if(strchr(upTo, ch) && (ch != '\n'))
		break;
	    if(ch == '\r')
	    {
		ch = m_input.read();
		if(ch == '\n')
		{
		    ch = m_input.read();
		    if(ch == ' ' || ch == '\t')
		    {
			if (m_tokstr == "")
			{
			    // If a line is blank, we will not allow it to be
			    // continued on the next line, since this is 
			    // probably the dividing line between headers and
			    // content. The content may have whitespace at
			    // the beginning (PR #23661) which we want to
			    // treat as content and not part of the headers.
			    m_input.putBack(ch);
			    return MIMEToken(m_tokstr, MIMEToken::T_EOL);
			}
			else
			{
			    skipWS();
			}
		    }
		    else
		    {
			m_input.putBack(ch);
			return MIMEToken(m_tokstr, MIMEToken::T_EOL);
		    }
		}
		else if(ch == ' ' || ch == '\t')
		{
		    skipWS();
		}
		else
		{
		    m_input.putBack(ch);
		    return MIMEToken(m_tokstr, MIMEToken::T_EOL);
		}
	    }
	    else if(ch == '\n')
	    {
		ch = m_input.read();
		if(ch == '\r')
		{
		    ch = m_input.read();
		    if(ch == ' ' || ch == '\t')
		    {
			if (m_tokstr == "")
			{
			    // If a line is blank, we will not allow it to be
			    // continued on the next line, since this is 
			    // probably the dividing line between headers and
			    // content. The content may have whitespace at
			    // the beginning (PR #23661) which we want to
			    // treat as content and not part of the headers.
			    m_input.putBack(ch);
			    return MIMEToken(m_tokstr, MIMEToken::T_EOL);
			}
			else
			{
			    skipWS();
			}
		    }
		    else
		    {
			m_input.putBack(ch);
			return MIMEToken(m_tokstr, MIMEToken::T_EOL);
		    }
		}
		else if(ch == ' ' || ch == '\t')
		{
		    skipWS();
		}
		else
		{
		    m_input.putBack(ch);
		    return MIMEToken(m_tokstr, MIMEToken::T_EOL);
		}
	    }
	    else
	    {
		m_tokstr += (char)ch;
	    }
	    ch = m_input.read();
	}
	return MIMEToken(m_tokstr, (char)ch);
    }

    int ch = m_input.read();
    switch(ch)
    {
	case -1:
	    return MIMEToken(MIMEToken::T_EOF);
	    break;
	case '\r':
	    if((ch = m_input.read()) != '\n')
		m_input.putBack(ch);
	    return MIMEToken(MIMEToken::T_EOL);
	    break;
	case '\n':
	    if((ch = m_input.read()) != '\r')
		m_input.putBack(ch);
	    return MIMEToken(MIMEToken::T_EOL);
	case '"':
	    bInQuote = 1;
	    ch = m_input.read();	// get to next char
	    break;
	default:
	    break;	// assume T_STRING - need more error processing here
    }

    // read string
    while(ch != MIMEToken::T_EOF) 
    {
	if(bInQuote)
	{
	    if(ch == '"')
	    {
		ch = m_input.read();	// next char to be put back
		bInQuote = 0;
		break;
	    }
	}
	else
	{
	    if(strchr(tspecials, ch) || iscntrl(ch))
		break;
	}
	/*
	 * Handle escaped double quotes
	 */
	if(ch == '\\')
	{
	    ch = m_input.peek();
	    if(ch == '\"')
	    {
		ch = m_input.read();
	    }
	}
	    
	m_tokstr += (char)ch;
	ch = m_input.read();
    }
    m_input.putBack(ch);
    return MIMEToken(m_tokstr, (char)ch);
}

void 
MIMEScanner::skipWS()
{
    int ch = m_input.read();
    while(ch == ' ' || ch == '\t')
	ch = m_input.read();
    m_input.putBack(ch);
}
