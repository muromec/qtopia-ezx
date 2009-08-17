/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dict.h,v 1.3 2004/07/09 18:21:27 hubbe Exp $
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

#ifndef	_DICT_H_
#define	_DICT_H_

extern unsigned int default_strhash(const char *key);

class Dict;

class Dict_entry
{
public:
    char*		key;
    void*		obj;
private:
    friend class	Dict;

    unsigned int	hash;
    Dict_entry*		next;
};

class	Dict
{
public:
    typedef const char*	Key;

			Dict(unsigned int nbuckets=16);
			Dict(int(*comp)(const char*,const char*),
			     unsigned int(*hash)(const char*) = default_strhash,
			     unsigned int nbuckets=16);
			~Dict();

    Dict_entry*		enter(Key key, void* obj);
    void*		remove(Key key);
    Dict_entry*		find(Key key);
    void		first(unsigned int&h, Dict_entry*& e);
    void		next(unsigned int& h, Dict_entry*& e);
    int			size();
#ifdef XXXAAK_AWAITING_CR
    Dict_entry*		enter(Key key, void* obj, UINT32& hashId);
    void*		remove(UINT32 hashId);
    Dict_entry*		find(UINT32 hashId);
#endif

private:
    unsigned int	_count;
    unsigned int	_nbuckets;
    Dict_entry**	_table;
    int			(*_compare)(const char*, const char*);
    unsigned int	(*_hash)(const char*);
    void		init();
};

inline int
Dict::size()
{
    return _count;
}

inline void
Dict::first(unsigned int&h, Dict_entry*& e)
{
    for (unsigned int i = 0; i < _nbuckets; i++)
	if (_table[i])
	{
	    h = i;
	    e = _table[i];
	    return;
	}
    e = 0;
}

class	Dict_iterator
{
public:
			Dict_iterator(Dict*d);
    Dict_entry*		operator*();
    Dict_entry*		operator ++() { _d->next(_h, _e); return _e;}
    int			operator!=(const Dict_iterator& rhs);
    int			operator==(const Dict_iterator& rhs);
private:
    Dict*		_d;
    unsigned int	_h;
    Dict_entry*		_e;
};

inline
Dict_iterator::Dict_iterator(Dict*d): _d(d)
{
    _d->first(_h,_e);
}

inline int		
Dict_iterator::operator!=(const Dict_iterator& rhs)
{
    return _e != rhs._e;
}

inline int 
Dict_iterator::operator==(const Dict_iterator& rhs)
{
    return _e == rhs._e;
}
inline Dict_entry*
Dict_iterator::operator*()
{
    return _e;
}
#endif/*_DICT_H_*/
