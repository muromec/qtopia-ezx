/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: regkey.h,v 1.4 2009/05/30 19:11:00 atin Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
 *  Class: ServRegKey
 *
 *  Description:
 *  	key strings for the log database. has the property of being
 *  hierarchical, i.e. the keys can be,
 *  "server" -- can or cannot contain other elements
 *  "server.foo" -- "server" contains "foo"
 *  "server.foo.bar" -- "server.foo" contains "bar"
 */

#ifndef _REGKEY_H_
#define _REGKEY_H_

#include "hxtypes.h"
#include "regmem.h"

class ServRegKey
{
public:
    ServRegKey(const char* szKey, char chDelim = '.');
    ~ServRegKey();
    
    inline char*    get_key_str() const { return m_pszKey; }
    int             get_sub_str(char* pszBuf, int nBufLen, char chDelim = '\0');
    int             append_sub_str(char* pszBuf, int nBufLen, char chDelim = '\0');
    inline BOOL     last_sub_str();
    BOOL            is_a_sub_str_of(char* pszBuf) const;
    inline int      size()       { return m_nSize; }
    inline char     delim()      { return m_chDelim; }
    inline int      num_levels() { return m_nLevels; }
    inline void     reset();
    inline BOOL	    is_valid()	 { return m_pszKey ? TRUE : FALSE; }

private:
    char*           m_pszKey;
    const char*     m_pCurrPtr;
    int             m_nCurrLevel;
    char*           m_pszLastSubStr;
    char            m_chDelim;
    int             m_nSize;
    int             m_nLevels;
    char**          m_pSubStrs;

};

inline
ServRegKey::~ServRegKey()
{
        if (m_pszKey)
	    delete[] m_pszKey;
	if (m_pSubStrs)
	    delete[] m_pSubStrs;

    m_pszKey = 0;
    m_pCurrPtr = 0;
    m_nCurrLevel = 0;
    m_pszLastSubStr = 0;
    m_chDelim = 0;
    m_nSize = 0;
    m_nLevels = 0;
    m_pSubStrs = 0;
}


/*
 *  ServRegKey::reset
 *  	resets the "curr_ptr" pointer to the begining of the "m_pszKey".
 *  this is used in case someone wants to get the sub-strings of "m_pszKey"
 *  again.
 */
inline void
ServRegKey::reset()
{
    m_pCurrPtr = m_pszKey;
    m_nCurrLevel = 0;
}

inline BOOL
ServRegKey::last_sub_str()
{
    return (m_pCurrPtr >= m_pszLastSubStr) ? TRUE : FALSE;
}

#endif // _REGKEY_H_
