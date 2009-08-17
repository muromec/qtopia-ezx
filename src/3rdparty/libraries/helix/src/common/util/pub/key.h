/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: key.h,v 1.5 2005/03/14 19:36:40 bobclark Exp $
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

#ifndef _KEY_H_
#define _KEY_H_

#include "hxtypes.h"
#include "hxresult.h"

/*
 *  Class Name:  	Key
 *  Description:
 *  	key strings for the log database. has the property of being
 *  hierarchical, i.e. the keys can be,
 *  "server" -- can or cannot contain other elements
 *  "server.foo" -- "server" contains "foo"
 *  "server.foo.bar" -- "server.foo" contains "bar"
 */
class Key
{
private:
    char*  	_key_str;
    const char* _curr_ptr;
    int		_curr_level;
    char*	_last_sub_str;
    char   	_delim;
    int    	_size;
    int		_num_levels;
    char**	_sub_strs;

public:
    	   	Key(const Key & k) {}
    	   	Key(const char* str, char delim = '.');
    	   	~Key();
    char*	get_key_str() const;
    int		get_sub_str(char* buf, int len, char delim = '\0');
    int		append_sub_str(char* buf, int buf_len, char delim = '\0');
    HXBOOL	last_sub_str();
    HXBOOL	is_a_sub_str_of(char* str) const;
    int		size() { return _size; }
    char	delim() { return _delim; }
    int		num_levels() { return _num_levels; }
    void	reset();
    HX_RESULT   m_LastError;
};

inline
Key::~Key()
{
    delete [] _key_str;
    delete [] _sub_strs;
}


/*
 *  Function Name:  	Key::get_key_str const
 *  Input Params:   	
 *  Return Value:   	inline char *
 *  Description:
 *  	returns a pointer to the entire key string.
 */
inline char *
Key::get_key_str() const
{
    return _key_str;
}

/*
 *  Function Name:  	Key::reset
 *  Input Params:   	
 *  Return Value:   	void
 *  Description:
 *  	resets the "curr_ptr" pointer to the begining of the "key_str".
 *  this is used in case someone wants to get the sub-strings of "key_str"
 *  again.
 */
inline void
Key::reset()
{
    _curr_ptr = _key_str;
    _curr_level = 0;
}

inline HXBOOL
Key::last_sub_str()
{
    return (_curr_ptr >= _last_sub_str) ? TRUE : FALSE;
}

#endif // _KEY_H_
