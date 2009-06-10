/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: db_dict.h,v 1.5 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef	_DB_DICT_H_
#define	_DB_DICT_H_

#include "db_dict_abs.h"

/*
 * 
 * "Client"
 *
 * DB_implem*
 *  _
 * |_|
 * |_|             "Client.5"
 * |_|             
 * |_|   DB_node*  Property*
 * |_|       _       _
 * |_|----->|_|---->|_|---+
 * |_|                    | 
 * |_|                    V
 *                    DB_implem*
 *                        _
 *                       |_|
 *                       |_|
 *                       |_|
 *                       |_|             "Client.4.protocol"
 *                       |_|             
 *                       |_|   DB_node*  Property*
 *                       |_|       _       _
 *                       |_|----->|_|---->|_|--->"RTSP"
 *                       |_|
 *                       |_|
 *
 * the "Client" (DB_implem*) is the OWNER DB of "Client.5" (DB_node*)
 * and the Property*
 * i.e.
 *     client5DB_node->get_db() will return the DB_implem* for Client
 *     client5Property->_owner_db is the SAME AS the DB_implem* for Client
 *
 * the "Client.5" (DB_node*) is the OWNER NODE of its Property
 * i.e.
 *     client5Property->_owner_node is the SAME AS client5DB_node*
 *
 * the "Client.5" (DB_node*) is the OWNER NODE for "Client.5" (DB_implem*)
 * i.e.
 *     client5DB_implem->owner_node() is the SAME AS client5DB_node*
 */

extern UINT32 hash_torek(const char*key);

class Property;
class DB_dict;
class DB_node;

class DB_node
{
public:
			DB_node() : obj(0), hash(0), next(0), owner_db(0) {}
			~DB_node() {}
    void		key(Keytype k) { hash = k; }
    Keytype		get_key() { return hash; }
    void		id(Keytype k) { _id = k; }
    Keytype		get_id() { return _id; }
    void		data(Property* p) { obj = p; }
    Property*		get_data() { return obj; }
    void		db(DB_implem* ldb) { owner_db = ldb; }
    DB_implem*		get_db() { return owner_db; }

    Property*		obj;

private:
    friend class	DB_dict;

    Keytype		hash;
    Keytype		_id;
    DB_node*		next;
    DB_implem* 		owner_db;
};

class DB_dict : public DB_implem
{
public:
		DB_dict();
		DB_dict(DB_node* parent, 
		        Keytype nbuckets = 16,
		        Keytype(*hash)(const char*) = hash_torek,
		        int(*comp)(const char*,const char*) = strcmp);
		~DB_dict();

    DB_node*    add(char* key_str, Property* new_p);
    Keytype  	del(const char* key_str);
    Keytype  	del(DB_node* node);
    DB_node*   	first();
    DB_node*  	find(char* key_str);
    DB_node*	next(DB_node* d);
    DB_node*	owner_node() const;
    Keytype 	hash(const char* str) const;
    Keytype 	size() const;
    int 	count() const;

private:
    HX_RESULT	init();
    void	strtolower(char* str);

    DB_node**	_table;
};

inline Keytype
DB_dict::size() const
{
    return _size;
}

inline int
DB_dict::count() const
{
    return _count;
}

inline DB_node*
DB_dict::first()
{
    for (Keytype i = 0; i < _size; i++)
	if (_table[i])
	    return _table[i];
    return 0;
}

inline DB_node*
DB_dict::owner_node() const
{
    return _owner_node;
}

inline Keytype
DB_dict::hash(const char* str) const
{
    return _hash(str);
}

#endif //_DB_DICT_H_
