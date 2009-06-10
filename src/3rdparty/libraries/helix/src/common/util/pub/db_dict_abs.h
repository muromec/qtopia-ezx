/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: db_dict_abs.h,v 1.4 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _DB_DICT_ABS_H_
#define _DB_DICT_ABS_H_

#include "hxtypes.h"

/*
 * db_dict_abs.h
 *
 * abstract base class defining the api for the underlying database
 * implementation.
 */

extern UINT32 hash_torek(const char*key);

typedef UINT32 		Keytype;

class DB_node;
class Property;

class DB_implem
{
public:
    			DB_implem();
    			DB_implem(DB_node* parent, 
				UINT32 size,
	 			UINT32(*hash)(const char*), 
				int(*comp)(const char*, const char*));
    virtual 		~DB_implem();

    // add a node to the database and return the node pointer
    virtual DB_node*  	add(char* key_str, Property* new_p) = 0;

    // remove the node from the database and return its object
    virtual UINT32 	del(const char* key_str) = 0;
    virtual UINT32 	del(DB_node* node) = 0;

    // return the first non-null node in the database
    virtual DB_node* 	first() = 0;

    virtual DB_node* 	find(char* key_str) = 0;

    // return the non-null node after node "d"
    virtual DB_node* 	next(DB_node* d) = 0;

    virtual UINT32 	hash(const char* str) const = 0;
    virtual DB_node*	owner_node() const = 0;

    virtual UINT32 	size() const = 0;
    virtual int 	count() const = 0;

protected:
    // string comparison function
    int	    	 	(*_compare)(const char*,const char*);
    // hashing function
    UINT32 		(*_hash)(const char*);
    // initial size for buckets/list elements/...
    UINT32		_size;
    // number of elements in the database
    int			_count;
    // parent node which contains this dictionary
    DB_node*		_owner_node;
};

inline
DB_implem::DB_implem()
	 : _compare(0), _hash(0),  _size(0), _count(0), _owner_node(0)
{
}

inline
DB_implem::DB_implem(DB_node* owner_node, UINT32 size,
		     UINT32(*hash)(const char*),
		     int(*comp)(const char*,const char*))
	 : _compare(comp), _hash(hash),  _size(size), _count(0), 
	   _owner_node(owner_node)
{
}

inline
DB_implem::~DB_implem()
{
}

#endif // _DB_DICT_ABS_H_
