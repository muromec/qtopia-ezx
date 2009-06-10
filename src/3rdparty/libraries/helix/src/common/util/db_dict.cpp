/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: db_dict.cpp,v 1.11 2008/01/18 04:54:26 vkathuria Exp $
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

#include "hxtypes.h"
// #include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hlxclib/ctype.h"
#include "hxassert.h"
#include "debug.h"

#include "db_misc.h"
#ifdef _SYMBIAN
//XXXgfw Wow, symbian's unistd.h #defines remove to be unlink.
//Very uncool.
#undef remove
#endif
#include "watchlst.h"
#include "property.h"
#include "db_dict_abs.h"
#include "db_dict.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// Chris Torek's hash function
UINT32
hash_torek(const char *key)
{
    UINT32 h = 0x31fe;

    while (*key)
	h = h*33 + *key++;
    return h;
}

HX_RESULT
DB_dict::init()
{
    _table = new DB_node*[_size];
    if(!_table)
    {
        return HXR_OUTOFMEMORY;
    }
    memset(_table, 0, HX_SAFESIZE_T(sizeof(void *) * _size));
    return HXR_OK;
}
void
DB_dict::strtolower(char* str)
{
    for (; *str != 0; str++)
	if (isupper(*str))
	    *str = tolower(*str);
}

// Need func ptr, in case strcasecmp is a macro
static int FuncStrcasecmp(const char* a, const char* b)
{
    return strcasecmp(a,b);
}

DB_dict::DB_dict()
          : DB_implem(0, 16, hash_torek, FuncStrcasecmp)
{
    init();
}

DB_dict::DB_dict(DB_node* parent, Keytype nbuckets,
	               Keytype (*hash)(const char*), 
                       int (*compare)(const char*, const char*))
	   : DB_implem(parent, nbuckets, hash, compare)
{
    init();
    DPRINTF(D_REGISTRY, ("DB_dict(this(%p)) -- DB_node(parent(%p))\n",
	    this, parent));
}

DB_dict::~DB_dict()
{
    for (Keytype i = 0; i < _size; i++)
    {
	DB_node* nexte;
	for (DB_node* e = _table[i]; e != 0; e = nexte)
	{
	    nexte = e->next;
	    delete e->obj;
	    delete e;
	}
    }
    delete[] _table;
}

Keytype
DB_dict::del(const char * key)
{
    DB_node* e, **ep;
    char* k = (char *)key;
    strtolower(k);
    Keytype h = _hash(k);
    for (ep = &_table[h%_size]; (e = *ep) != 0; ep = &e->next)
    {
	if (h != e->hash)
	    continue;

	*ep = e->next;
	delete e->obj;
	delete e;
	--_count;
	return h;
    }
    return 0;       
}

Keytype
DB_dict::del(DB_node* node)
{
    DB_node* e, **ep;
    Keytype h = node->hash;
    for (ep = &_table[h%_size]; (e = *ep) != 0; ep = &e->next)
    {
	if (node != e)
	    continue;

	*ep = e->next;
	delete e->obj;
	delete e;
	--_count;
	return h;
    }
    return 0;       
}

DB_node *
DB_dict::next(DB_node * e)
{
    if (e->next)
	return e->next;
    Keytype h = e->hash % _size;
    for (Keytype i = h+1; i < _size; i++)
	if (_table[i] != 0)
	    return _table[i];
    return 0;
}

DB_node *    	
DB_dict::add(char * key_str, Property * new_p)
{
    strtolower(key_str);
    Keytype h = _hash(key_str);
    DB_node* e, *nexte;
    _count++;

    // Grow the table if 66% full
    Keytype nb = _count*3;
    if (nb > _size*2)
    {
	DB_node** tab = new DB_node*[nb];
        if( !tab )
        {
            _count--;
            return NULL;
        }
	Keytype i;
	memset(tab, 0, HX_SAFESIZE_T(sizeof(void *) * nb));

	for (i = 0; i < _size; i++)
	{
	    for (e = _table[i]; e != 0; e = nexte)
	    {
		nexte = e->next;
		e->next = tab[e->hash%nb];
		tab[e->hash%nb] = e;
	    }
	}
	delete[] _table;
	_table = tab;
	_size = nb;
    }
    e =  new DB_node;
    if (e)
    {
	e->next = _table[h%_size];
	e->hash = h;
	e->obj = new_p;
	e->owner_db = this;
	_table[h%_size] = e;
	return e;
    }
    else
	return 0;
}

DB_node *  	
DB_dict::find(char * key_str)
{
    char* k = key_str;
    strtolower(k);
    Keytype h = _hash(k);
    for (DB_node* e = _table[h%_size]; e != 0; e = e->next)
	if (strcasecmp(k, e->get_data()->get_key_str()) == 0)
	    return e;
    return 0;
}
