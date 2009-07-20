/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: regprop.h,v 1.3 2003/03/19 21:30:14 gwright Exp $ 
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

#ifndef _REGPROP_H
#define _REGPROP_H

#include <string.h>
#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"

#include "hxmon.h"
#include "regkey.h"
#include "regdb_misc.h"
#include "regdb_dict.h"
#include "regmem.h"
#include "watchlst.h"

struct IHXPropWatchResponse;

class PropWatch
{
public:
    PropWatch() : m_pResponse(0){}
    ~PropWatch() {}
    IHXPropWatchResponse*      m_pResponse;
};

class ServRegProperty
{
public:
    REGISTRY_CACHE_MEM

    ServRegProperty();
    ServRegProperty(ServRegKey* n, HXPropType t, RegistryMemCache* pCache);
    ServRegProperty(char* n, HXPropType t, RegistryMemCache* pCache);
    virtual ~ServRegProperty();

    // mutator methods
    HX_RESULT           key_str(char* s);
    HX_RESULT           type(const HXPropType t);

    HX_RESULT           buf_val(IHXBuffer* v, HXPropType t = PT_BUFFER);
    HX_RESULT           int_val(const INT32 v);
    HX_RESULT           int_ref_val(INT32* v);
    HX_RESULT           db_val(ServRegDB_dict* v);

    // accessor methods
    const ServRegKey*   get_key_obj() const;
    const char*         get_key_str() const;
    int                 get_key_str_len() const;
    void                get_type(HXPropType* t) const;
    HXPropType         get_type() const;

    HX_RESULT           get_buf_val(IHXBuffer** v, 
                                    HXPropType t = PT_BUFFER) const;
    const IHXBuffer*   get_buf_val(HXPropType t = PT_BUFFER) const;
    HX_RESULT           get_int_val(INT32* v) const;
    INT32               get_int_val() const;
    HX_RESULT           get_int_ref_val(INT32* v) const;
    INT32               get_int_ref_val() const;
    HX_RESULT           get_int_ref(INT32** v) const;
    INT32*              get_int_ref() const;
    HX_RESULT           get_db_val(ServRegDB_dict** v) const;
    const ServRegDB_dict* get_db_val() const;

    void                set_deleted(ServRegDB_dict* ldb, ServRegDB_node* d,
                                    UINT32 h);
    BOOL                is_deleted();
    void                set_read_only(BOOL read_only);
    BOOL                is_read_only();
    void                SetStringAccessAsBufferById();

    WatchList*          m_pWatchList;
    LONG32              m_lWatchCount;

    /*
     * Make these public so that CommonRegistry can use them
     */

    ServRegDB_dict*     _owner_db;
    ServRegDB_node*     _owner_node;
    UINT32              _id;
    BOOL                _alternate_string_access_ok;

private:
    ServRegKey*         _prop_name;
    RegistryMemCache*   _reg_mem_cache;
    HXPropType         _prop_type;
    // (in bytes) mainly for strings/binary blobs
    INT32               _prop_size;
    BOOL                _deleted;
    BOOL                _read_only;
    union VALUE
    {
        IHXBuffer*         t_buf;
        INT32               t_int;
        INT32*              t_iref;
        ServRegDB_dict*     t_imp;
    } _prop_val;
};

inline
ServRegProperty::ServRegProperty()
    : _prop_name(0)
    , _reg_mem_cache(0)
    , _prop_type(PT_UNKNOWN)
    , _prop_size(0)
    , _deleted(FALSE)
    , _read_only(FALSE)
    , _alternate_string_access_ok(0)
    , _owner_db(0)
    , _owner_node(0)
    , _id(0)
    , m_lWatchCount(0)
{
    _prop_val.t_buf = 0;
    m_pWatchList = new WatchList;
}

inline
ServRegProperty::ServRegProperty(ServRegKey* k,
                                 HXPropType t,
                                 RegistryMemCache* pMem) 
    : _prop_name(k)
    , _reg_mem_cache(pMem)
    , _prop_type(t)
    , _prop_size(0)
    , _deleted(FALSE)
    , _read_only(FALSE)
    , _alternate_string_access_ok(0)
    , _owner_db(0)
    , _owner_node(0)
    , _id(0)
    , m_lWatchCount(0)
{
    _prop_val.t_buf = 0;
    m_pWatchList = new WatchList;
}

inline
ServRegProperty::ServRegProperty(char* n,
                                 HXPropType t,
                                 RegistryMemCache* pMem)
    : _prop_name(new(pMem) ServRegKey(n, pMem))
    , _reg_mem_cache(pMem)
    , _prop_type(t)
    , _prop_size(0)
    , _deleted(FALSE)
    , _read_only(FALSE)
    , _alternate_string_access_ok(0)
    , _owner_db(0)
    , _owner_node(0)
    , _id(0)
    , m_lWatchCount(0)
{
    _prop_val.t_buf = 0;
    m_pWatchList = new WatchList;
}

inline
ServRegProperty::~ServRegProperty()
{
    for (WatchList_iterator wli(m_pWatchList); *wli != 0; ++wli)
    {
        WListElem* wle = *wli;
        PropWatch* pw = (PropWatch *)wle->data;
        delete pw;
        m_pWatchList->removeElem(wle);
        delete wle;
        m_lWatchCount--;
    }
    delete m_pWatchList;

    delete _prop_name;
    switch (_prop_type)
    {
        case PT_COMPOSITE:
            delete _prop_val.t_imp;
            break;
        case PT_STRING:
            if (_prop_val.t_buf)
                _prop_val.t_buf->Release();
            break;
        case PT_BUFFER:
            if (_prop_val.t_buf)
                _prop_val.t_buf->Release();
            break;
        default:
            break;
    }
}

inline HX_RESULT
ServRegProperty::type(const HXPropType t)
{
    _prop_type = t;
    return HXR_OK;
}

inline HX_RESULT
ServRegProperty::int_val(const INT32 v)
{
    if (_read_only)
    {
        return HXR_FAIL;
    }
    else if (_prop_type != PT_INTEGER)
    {
        _prop_val.t_int = 0;
        return HXR_FAIL;
    }
    else
    {
        _prop_val.t_int = v;
        return HXR_OK;
    }
}

inline HX_RESULT
ServRegProperty::int_ref_val(INT32* v)
{
    if (_read_only)
    {
        return HXR_FAIL;
    }
    else if (_prop_type != PT_INTREF)
    {
        _prop_val.t_iref = 0;
        return HXR_FAIL;
    }
    else
    {
        _prop_val.t_iref = v;
        return HXR_OK;
    }
}

inline HX_RESULT
ServRegProperty::buf_val(IHXBuffer* v, HXPropType t)
{
    HX_RESULT theErr = HXR_OK;

    if (_read_only)
    {
        return HXR_FAIL;
    }
    else if (_prop_type != t)
    {
        theErr = HXR_FAIL;
        goto cleanup;
    }
  
    if (_prop_val.t_buf)
    {
        _prop_val.t_buf->Release();
        _prop_val.t_buf = 0;
    }

    if (!v)
    {
        theErr = HXR_OK;
        goto cleanup;
    }

    _prop_val.t_buf = v;
    v->AddRef();

cleanup:
    return theErr;
}

inline const ServRegKey*
ServRegProperty::get_key_obj() const
{
    return _prop_name;
}

inline const char*
ServRegProperty::get_key_str() const
{
    return _prop_name->get_key_str();
}

inline int
ServRegProperty::get_key_str_len() const
{
    return _prop_name->size();
}

inline void
ServRegProperty::get_type(HXPropType* t) const
{
    *t = _prop_type;
}

inline HXPropType
ServRegProperty::get_type() const
{
    return _prop_type;
}

inline HX_RESULT
ServRegProperty::get_buf_val(IHXBuffer** v, HXPropType t) const
{
    HX_RESULT theErr = HXR_OK;

    *v = 0;

    if (_prop_type != t)
    {
        theErr = HXR_FAIL;
        goto cleanup;
    }
   
    if (!_prop_val.t_buf)
    {
        theErr = HXR_FAIL;
        goto cleanup;
    }

    _prop_val.t_buf->AddRef();
    *v = _prop_val.t_buf;

cleanup:
    return theErr;
}

inline HX_RESULT
ServRegProperty::get_int_val(INT32* v) const
{
    if (_prop_type != PT_INTEGER)
    {
        *v = 0;
        return HXR_FAIL;
    }
    else
    {
        *v = _prop_val.t_int;
        return HXR_OK;
    }
}

inline HX_RESULT
ServRegProperty::get_int_ref_val(INT32* v) const
{
    if (_prop_type != PT_INTREF)
    {
        *v = 0;
        return HXR_FAIL;
    }
    else
    {
        *v = *_prop_val.t_iref;
        return HXR_OK;
    }
}

inline HX_RESULT
ServRegProperty::get_int_ref(INT32** v) const
{
    if (_prop_type != PT_INTREF)
    {
        *v = 0;
        return HXR_FAIL;
    }
    else
    {
        *v = _prop_val.t_iref;
        return HXR_OK;
    }
}

inline HX_RESULT
ServRegProperty::get_db_val(ServRegDB_dict** v) const
{
    if (_prop_type != PT_COMPOSITE)
    {
        *v = 0;
        return HXR_FAIL;
    }
    else
    {
        *v = _prop_val.t_imp;
        return HXR_OK;
    }
}

inline INT32
ServRegProperty::get_int_val() const
{
    if (_prop_type != PT_INTEGER)
        return 0;
    else
        return _prop_val.t_int;
}

inline const IHXBuffer*
ServRegProperty::get_buf_val(HXPropType t) const
{
    if (_prop_type != t)
        return 0;
    
    if (!_prop_val.t_buf)
        return 0;
        
    _prop_val.t_buf->AddRef();
    return _prop_val.t_buf;
}

inline INT32*
ServRegProperty::get_int_ref() const
{
    if (_prop_type != PT_INTREF)
        return 0;
    else
    {
        return _prop_val.t_iref;
    }
}

inline const ServRegDB_dict*
ServRegProperty::get_db_val() const
{
    if (_prop_type != PT_COMPOSITE)
        return 0;
    else
        return _prop_val.t_imp;
}

inline void
ServRegProperty::set_deleted(ServRegDB_dict* ldb, ServRegDB_node* d, UINT32 h)
{
    _owner_db = ldb;
    _owner_node = d;
    _id = h;
    _deleted = TRUE;
}

inline BOOL
ServRegProperty::is_deleted()
{
    return _deleted;
}

inline void
ServRegProperty::set_read_only(BOOL read_only)
{
    _read_only = read_only;
}

inline BOOL
ServRegProperty::is_read_only()
{
    return _read_only;
}

inline void
ServRegProperty::SetStringAccessAsBufferById()
{
    _alternate_string_access_ok = TRUE;
}

#endif // _REGPROP_H
