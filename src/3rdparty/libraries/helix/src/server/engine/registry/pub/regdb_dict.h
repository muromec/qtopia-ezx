/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: regdb_dict.h,v 1.2 2003/01/23 23:42:57 damonlan Exp $ 
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

#ifndef _REGDB_DICT_H_
#define _REGDB_DICT_H_

#include "hxtypes.h"
#include "regmem.h"

/*
 * 
 * "Client"
 *
 * DB_dict*
 *  _
 * |_|
 * |_|             "Client.5"
 * |_|             
 * |_|   DB_node*  Property*
 * |_|       _       _
 * |_|----->|_|---->|_|---+
 * |_|                    | 
 * |_|                    V
 *                    DB_dict*
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
 * the "Client" (DB_dict*) is the OWNER DB of "Client.5" (DB_node*)
 * and the Property*
 * i.e.
 *     client5DB_node->get_db() will return the DB_dict* for Client
 *     client5Property->_owner_db is the SAME AS the DB_dict* for Client
 *
 * the "Client.5" (DB_node*) is the OWNER NODE of its Property
 * i.e.
 *     client5Property->_owner_node is the SAME AS client5DB_node*
 *
 * the "Client.5" (DB_node*) is the OWNER NODE for "Client.5" (DB_dict*)
 * i.e.
 *     client5DB_dict->owner_node() is the SAME AS client5DB_node*
 */

class ServRegProperty;
class ServRegDB_dict;
class ServRegDB_node;

extern UINT32 serv_hash_torek(const char*key);

typedef UINT32 Keytype;


class ServRegDB_node
{
public:
    REGISTRY_CACHE_MEM

    ServRegDB_node() : obj(0), hash(0), _id(0), next(0), owner_db(0) {}
    ~ServRegDB_node() {}

    inline void                 key(Keytype k) { hash = k; }
    inline Keytype              get_key()      { return hash; }

    inline void                 id(Keytype k)  { _id = k; }
    inline Keytype              get_id()       { return _id; }

    inline void                 data(ServRegProperty* p) { obj = p; }
    inline ServRegProperty*     get_data()               { return obj; }

    inline void                 db(ServRegDB_dict* ldb) { owner_db = ldb; }
    inline ServRegDB_dict*      get_db()                { return owner_db; }

    ServRegProperty*    obj;

private:
    friend class        ServRegDB_dict;

    Keytype             hash;
    Keytype             _id;
    ServRegDB_node*     next;
    ServRegDB_dict*     owner_db;
};

class ServRegDB_dict 
{
public:
    REGISTRY_CACHE_MEM

    ServRegDB_dict(RegistryMemCache* pCache);
    ServRegDB_dict(ServRegDB_node* parent, 
                   RegistryMemCache* pCache,
                   Keytype nbuckets = 16,
                   Keytype(*hash)(const char*) = serv_hash_torek,
                   int(*comp)(const char*,const char*) = strcmp);
    ~ServRegDB_dict();

    // add a node to the database and return the node pointer
    ServRegDB_node*     add(char* key_str, ServRegProperty* new_p);

    // remove the node from the database and return its object
    Keytype             del(const char* key_str);
    Keytype             del(ServRegDB_node* node);
    
    // return the first non-null node in the database
    ServRegDB_node*     first();

    ServRegDB_node*     find(char* key_str);

    // return the non-null node after node "d"
    ServRegDB_node*     next(ServRegDB_node* d);

    inline Keytype      hash(const char* str) const { return _hash(str); }
    inline ServRegDB_node* owner_node() const       { return _owner_node; }

    inline Keytype      size() const  { return _size; }
    int                 count() const { return _count; }

private:
    void                init();
    void                strtolower(char* str);

    ServRegDB_node**    _table;
    RegistryMemCache*   m_pCache;

    // string comparison function
    int                         (*_compare)(const char*,const char*);
    // hashing function
    UINT32                      (*_hash)(const char*);
    // initial size for buckets/list elements/...
    UINT32              _size;
    // number of elements in the database
    int                 _count;
    // parent node which contains this dictionary
    ServRegDB_node*     _owner_node;
};



inline ServRegDB_node*
ServRegDB_dict::first()
{
    for (Keytype i = 0; i < _size; i++)
        if (_table[i])
            return _table[i];
    return 0;
};

#endif //_REGDB_DICT_H_
