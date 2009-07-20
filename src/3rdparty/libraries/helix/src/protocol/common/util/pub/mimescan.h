/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mimescan.h,v 1.9 2007/07/06 20:51:33 jfinnecy Exp $
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
 * Tokenizer and buffered input classes to scan MIME messages
 */

#ifndef _MIMESCAN_H_
#define _MIMESCAN_H_

#include <ctype.h>
#include "hxstring.h"
#include "hxstrutl.h"

class MIMEInputStream
{
public:
    MIMEInputStream(const char* pBuf, UINT32 nBufLen);
    MIMEInputStream(const CHXString& str);
    ~MIMEInputStream();

    int read();
    int peek();
    int read(char* pBuf, UINT32 nBufLen);
    UINT32 available();
    UINT32 offset() { return (!m_bUndoValid) ? m_nCurOffset : m_nCurOffset-1; }
    UINT32 length() { return m_nBufSize; }
    void putBack(int ch) 
    {
	// If somone tries to push back a EOF ignore it..
	if(ch != -1)
	{
	    m_bUndoValid = TRUE;
	    m_nUndo = ch; 
	}
    }
    UINT32 max_distance_to(char* p);

private:
    char* m_pBuffer;
    UINT32 m_nBufSize;
    UINT32 m_nCurOffset;
    HXBOOL m_bUndoValid;
    int m_nUndo;
};

/*
 * Stores the token string and the last character for look-ahead
 */
class MIMEToken
{
public:
    enum { T_EOF=-1, T_EOL='\n' };
    MIMEToken():
	m_value(""),
	m_lastChar(-1)
    {}
    MIMEToken(const char* value, char lastChar):
	m_value(value),
	m_lastChar(lastChar)
    {}
    MIMEToken(char lastChar):
	m_value(""),
	m_lastChar(lastChar)
    {}
    const CHXString& value() { return m_value; }
    const signed char lastChar() { return (signed char)m_lastChar; }
    int hasValue();

private:
    CHXString m_value;
    int m_lastChar;
};

inline int
MIMEToken::hasValue()
{
    int b = 0;
    for(unsigned int i=0; i < m_value.GetLength(); i++)
    {
	if(!IS_SPACE(m_value[i]))
	{
	    b = 1;
	    break;
	}
    }
    return b;
}

class MIMEScanner
{
public:
    MIMEScanner(MIMEInputStream& input);
    ~MIMEScanner();

    MIMEToken nextToken(char* upTo = 0);
    UINT32 offset() { return m_input.offset(); }
    UINT32 length() { return m_input.length(); }

private:
    void skipWS();

    CHXString m_tokstr;
    CHXString m_line;
    MIMEInputStream& m_input;
};

#endif /* _MIMESCAN_H_ */
