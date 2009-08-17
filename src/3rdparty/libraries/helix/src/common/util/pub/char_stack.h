/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: char_stack.h,v 1.3 2004/07/09 18:23:36 hubbe Exp $
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

#ifndef CHAR_STACK_H
#define CHAR_STACK_H

/*
 * CharStack :
 * This is a utility class to help with parsing tasks. It allows you
 * to accumulate characters into a buffer without having to worry about
 * buffer allocation issues.
 */

class CharStack {
public:
    CharStack(int buf_len = 64);
    ~CharStack();

    char& operator*();
    CharStack& operator++();
    void operator++(int);
    const char* Finish();
    int Length() const;
    void Reset();

    const char* StrSep(const char* str, const char* sep);

private:
    CharStack(const CharStack&); // don't implement
    CharStack& operator=(const CharStack&);

    char* NewBuf();

    int m_Len;
    char* m_pBuf;
    char* m_pCur;
};

inline 
CharStack::CharStack(int buflen) :  
    m_Len(buflen),
    m_pBuf(0),
    m_pCur(0)
{
    NewBuf();
}

inline 
CharStack::~CharStack()
{
    delete [] m_pBuf;
    m_pBuf = 0;
    m_Len = 0;
    m_pCur = 0;
}

inline 
char& CharStack::operator*()
{
    return m_pCur - m_pBuf < m_Len ? *m_pCur : *NewBuf();
}

inline 
CharStack& CharStack::operator++()
{
    ++m_pCur;
    return *this;
}

inline 
void CharStack::operator++(int)
{
    m_pCur++;
}

inline 
int CharStack::Length() const
{
    return m_pCur - m_pBuf;
}

inline 
void CharStack::Reset()
{
    m_pCur = m_pBuf;
}

inline 
const char* CharStack::Finish()
{
    if (m_pCur - m_pBuf >= m_Len)
	 NewBuf();

    *m_pCur = 0;
    return m_pBuf;
}

#endif /* CHAR_STACK_H */
