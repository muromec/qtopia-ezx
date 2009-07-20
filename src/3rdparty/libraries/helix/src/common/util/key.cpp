/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: key.cpp,v 1.13 2005/05/03 16:14:29 albertofloyd Exp $
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

#include "hlxclib/string.h"
#include "hxtypes.h"
#include "key.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_NUM_LEVELS 255

Key::Key(const char * s, char d)
            : _key_str(NULL)
            , _curr_ptr(NULL)
            , _curr_level(0)
            , _last_sub_str(NULL)
            , _delim(0)
            , _size(0)
            , _num_levels(0)
            , _sub_strs(NULL)
            , m_LastError(HXR_OK)
{
    if (!s && !*s)
	return;

    _curr_ptr = s;

    // We have to go through this two step process because of a problem with
    // the 16 bit compiler.
    // Since xtmp_ptrs is to be used as an array of char*'s, use a
    // #define (instead of just "1024") and reuse that define, below, to
    // prevent going past the end of xtmp_ptrs when _num_levels >
    // 1024/(sizeof(char*))
    char* xtmp_ptrs = new char [(MAX_NUM_LEVELS+1) * sizeof(char*)]; /* Flawfinder: ignore */
    if (!xtmp_ptrs)
    {
        m_LastError = HXR_OUTOFMEMORY;
        return;
    }
    const char** tmp_ptrs = (const char**)xtmp_ptrs;

    tmp_ptrs[0] = s;
    /*
     * loop to find out how many levels are there in this key string
     * pointers in the string to the various sub-strings are stored in
     * a temporary array, which will then be xferred to a dynamic array
     * stored along with the key. this will speed up the sub-string 
     * operations done later.
     */
    for (_num_levels = 1, _size = 1;
            // Check to make sure _num_levels does not result in us writing
            // past the end of tmp_ptrs  (Fixes PR 136246):
            *_curr_ptr != '\0'  &&  _num_levels < MAX_NUM_LEVELS;
            _curr_ptr++, _size++)
    {
	if (*_curr_ptr == d)
	{
	    if (_curr_ptr > s)
	    {
		tmp_ptrs[_num_levels] = _curr_ptr;
		_num_levels++;
	    }
	}
    }
    if(_num_levels >= MAX_NUM_LEVELS)
    {
        m_LastError = HXR_UNEXPECTED;
        HX_VECTOR_DELETE(xtmp_ptrs);
        return;
    }

    tmp_ptrs[_num_levels] = _curr_ptr;

    _sub_strs = new char* [_num_levels+1];
    if(!_sub_strs)
    {
        m_LastError = HXR_OUTOFMEMORY;
        HX_VECTOR_DELETE(xtmp_ptrs);
        return;
    }
    _key_str = new char[_size];
    if(!_key_str)
    {
        m_LastError = HXR_OUTOFMEMORY;
        HX_VECTOR_DELETE(xtmp_ptrs);
        HX_VECTOR_DELETE(_sub_strs);
        return;
    }
    strcpy(_key_str, s); /* Flawfinder: ignore */
    _sub_strs[0] = _key_str;

    for (int i = 1; i < _num_levels+1; i++)
	_sub_strs[i] = _sub_strs[0] + (tmp_ptrs[i] - tmp_ptrs[0]);

    _curr_ptr = _key_str;
    _curr_level = 0;
    _delim = d;

    _last_sub_str = _sub_strs[_num_levels-1];

    if (*_last_sub_str == _delim) ++_last_sub_str;

    delete[] xtmp_ptrs;
}

/*
 *  Function Name:  	get_sub_str
 *  Input Params:   	char* delim
 *  Return Value:   	int
 *  Description:
 *  	gets a sub-string of "key_str" delimited by "delim" and returns
 *  the number of bytes in the sub-string.
 */
int
Key::get_sub_str(char* buf, int buf_len, char delim)
{
    int c_len = 0;
    if (_curr_ptr && _curr_level >= _num_levels)
	return 0;

    c_len = _sub_strs[_curr_level+1]-_sub_strs[_curr_level];
    if (c_len >= buf_len)
	c_len = buf_len;

    // XXXAAK -- for now use strncpy
    strncpy(buf, _sub_strs[_curr_level], c_len); /* Flawfinder: ignore */
    *(buf+c_len) = '\0';
    _curr_level++;
	   
    _curr_ptr = (_sub_strs[_curr_level]) ? _sub_strs[_curr_level]+1
					 : _sub_strs[_curr_level];
    return c_len;
}

/*
 *  Function Name:  	Key::append_sub_str
 *  Input Params:   	char* buf, int buf_len, char delim
 *  Return Value:   	int
 *  Description:
 *  	appends the next sub-string to the string in the buffer
 *  passed as a parameter. if the buffer is empty the sub-string
 *  is just copied into it, but if it is not-empty then it first
 *  appends a delimiter and then the sub-string.
 */
int
Key::append_sub_str(char* buf, int buf_len, char delim)
{
    int c_len = 0;

    // if we have reached the end of the key string
    if (_curr_level >= _num_levels)
	return 0;

    // loop until u reach the end of the buf before we append to it
    if (*buf)
    {
	for (; c_len < buf_len && *buf != '\0'; buf++, c_len++)
	    ;
	// return if no more space in the buffer
	if (c_len >= buf_len)
	    return 0;

	*buf = '\0';
    }

    int num_chars = _sub_strs[_curr_level+1]-_sub_strs[_curr_level];
    /*
     * if the combined length exceeds the buffer len then reduce
     * the number of chars copied to fit the buffer
     */
    if ((c_len+num_chars) >= buf_len)
	num_chars = buf_len - c_len;

    c_len += num_chars;

    // XXXAAK -- for now use strncpy
    strncpy(buf, _sub_strs[_curr_level], num_chars); /* Flawfinder: ignore */
    *(buf+num_chars) = '\0';
    _curr_level++;
	   
    _curr_ptr = (_sub_strs[_curr_level]) ? _sub_strs[_curr_level]+1
					 : _sub_strs[_curr_level];
    return c_len;
}

/*
 *  Function Name:  	Key::is_a_sub_str_of const
 *  Input Params:   	char* str
 *  Return Value:   	HXBOOL
 *  Description:
 *  	check if the string "str" is a sub-string of the key string
 *  if it is then return TRUE else return FALSE. one criterion for'
 *  this operation is that a legal sub-string is defined as one which
 *  ends with a matching delimiter of the _key_str or a '\0' char.
 *  for example,
 *      foo is a VALID sub-string of "foo.bar.shmoo"
 *      and so is "foo.bar".
 *      whereas "foo.b" is NOT a VALID sub-string of "foo.bar.shmoo"
 */
HXBOOL
Key::is_a_sub_str_of(char* str) const
{
    if (!str || !*str)
	return FALSE;

    char* tmp = _key_str;

    for (; *str != '\0'; str++, tmp++)
    {
	if (*str != *tmp)
	    return FALSE;
    }

    if (*tmp != _delim && *tmp != '\0')
	return FALSE;

    return TRUE;
}
