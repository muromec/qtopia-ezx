/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: property.h,v 1.11 2008/01/18 04:54:28 vkathuria Exp $
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

#ifndef _PROPERTY_H
#define _PROPERTY_H

#include "hlxclib/string.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxbuffer.h"

#include "hxmon.h"
#include "key.h"
#include "db_misc.h"
#include "db_dict_abs.h"

#include "watchlst.h"

struct IHXPropWatchResponse;

class PropWatch
{
public:
    PropWatch() : m_pResponse(0){}
    ~PropWatch() {}
    IHXPropWatchResponse* m_pResponse;
};

class Property
{
private: 
    Key*  		_prop_name;
    HXPropType 	_prop_type;
    // (in bytes) mainly for strings/binary blobs
    INT32		_prop_size;
    HXBOOL 	  	_deleted;
    HXBOOL		_read_only;
    union VALUE
    {
	IHXBuffer*	t_buf;
	INT32		t_int;
	INT32*		t_iref;
	DB_implem*	t_imp;
    } _prop_val;
	
public:
    WatchList*		m_pWatchList;
    LONG32		m_lWatchCount;

    Property();
    Property(Key* n, HXPropType t);
    Property(char* n, HXPropType t);
    virtual ~Property();

    // mutator methods
    virtual HX_RESULT key_str(char* s);
    virtual HX_RESULT type(const HXPropType t);

    virtual HX_RESULT buf_val(IHXBuffer* v, HXPropType t = PT_BUFFER);
    virtual HX_RESULT int_val(const INT32 v);
    virtual HX_RESULT int_ref_val(INT32* v);
    virtual HX_RESULT db_val(DB_implem* v);

    // accessor methods
    virtual const Key*	get_key_obj() const;
    virtual const char* get_key_str() const;
    virtual int         get_key_str_len() const;
    virtual void        get_type(HXPropType* t) const;
    virtual HXPropType get_type() const;

    virtual HX_RESULT       	get_buf_val(IHXBuffer** v, 
					    HXPropType t = PT_BUFFER) const;
    virtual const IHXBuffer*	get_buf_val(HXPropType t = PT_BUFFER) const;
    virtual HX_RESULT       	get_int_val(INT32* v) const;
    virtual INT32      		get_int_val() const;
    virtual HX_RESULT       	get_int_ref_val(INT32* v) const;
    virtual INT32    		get_int_ref_val() const;
    virtual HX_RESULT    	get_db_val(DB_implem** v) const;
    virtual const DB_implem* 	get_db_val() const;

    virtual void		set_deleted(DB_implem* ldb, DB_node* d,
				            UINT32 h);
    virtual HXBOOL		is_deleted();
    virtual void		set_read_only(HXBOOL read_only);
    virtual HXBOOL		is_read_only();
    virtual void		SetStringAccessAsBufferById();

    /*
     * Make these public so that CommonRegistry can use them
     */

    DB_implem*		_owner_db;
    DB_node*		_owner_node;
    UINT32		_id;
    HXBOOL 	  	_alternate_string_access_ok;
    HX_RESULT		m_LastError;
};

inline
Property::Property()
         : _prop_name(0), _prop_type(PT_UNKNOWN), _prop_size(0),
	   _deleted(FALSE), _read_only(FALSE), _owner_db(0), 
	   _owner_node(0), _id(0), _alternate_string_access_ok(0),
           m_LastError(HXR_OK)
{
    _prop_val.t_int = 0x0000;
    _prop_val.t_buf = 0;
    m_pWatchList = new WatchList;
    if(!m_pWatchList)
    {
        m_LastError = HXR_OUTOFMEMORY;
    }
    m_lWatchCount = 0;
}

inline
Property::Property(Key* k, HXPropType t) 
         : _prop_name(k), _prop_type(t), _prop_size(0), 
	   _deleted(FALSE), _read_only(FALSE), _owner_db(0), 
           _owner_node(0), _id(0), _alternate_string_access_ok(1),
           m_LastError(HXR_OK)

{
    // XXX by default set the integer value to ZERO.
    _prop_val.t_int = 0x0000;
    _prop_val.t_buf = 0;
    m_pWatchList = new WatchList;
    if(!m_pWatchList)
    {
        m_LastError = HXR_OUTOFMEMORY;
    }
    m_lWatchCount = 0;
}

inline
Property::Property(char* n, HXPropType t)
         : _prop_name(new Key(n)), _prop_type(t), _prop_size(0),
	   _deleted(FALSE), _read_only(FALSE), _owner_db(0), 
           _owner_node(0), _id(0), _alternate_string_access_ok(1),
           m_LastError(HXR_OK)
{
    // XXX by default set the integer value to ZERO.
    _prop_val.t_int = 0x0000;
    _prop_val.t_buf = 0;
    m_pWatchList = new WatchList;
    if(!m_pWatchList)
    {
        m_LastError = HXR_OUTOFMEMORY;
    }
    m_lWatchCount = 0;
}

inline
Property::~Property()
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
Property::type(const HXPropType t)
{
    _prop_type = t;
    return HXR_OK;
}

inline HX_RESULT
Property::int_val(const INT32 v)
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
Property::int_ref_val(INT32* v)
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
Property::buf_val(IHXBuffer* v, HXPropType t)
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

inline const Key*
Property::get_key_obj() const
{
    return _prop_name;
}

inline const char*
Property::get_key_str() const
{
    return _prop_name->get_key_str();
}

inline int
Property::get_key_str_len() const
{
    return _prop_name->size();
}

inline void
Property::get_type(HXPropType* t) const
{
    *t = _prop_type;
}

inline HXPropType
Property::get_type() const
{
    return _prop_type;
}

inline HX_RESULT
Property::get_buf_val(IHXBuffer** v, HXPropType t) const
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
Property::get_int_val(INT32* v) const
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
Property::get_int_ref_val(INT32* v) const
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
Property::get_db_val(DB_implem** v) const
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
Property::get_int_val() const
{
    if (_prop_type != PT_INTEGER)
	return 0;
    else
	return _prop_val.t_int;
}

inline const IHXBuffer*
Property::get_buf_val(HXPropType t) const
{
    if (_prop_type != t)
	return 0;
    
    if (!_prop_val.t_buf)
	return 0;
	
    _prop_val.t_buf->AddRef();
    return _prop_val.t_buf;
}

inline INT32
Property::get_int_ref_val() const
{
    if (_prop_type != PT_INTREF)
	return 0;
    else
    {
	return *_prop_val.t_iref;
    }
}

inline const DB_implem*
Property::get_db_val() const
{
    if (_prop_type != PT_COMPOSITE)
	return 0;
    else
	return _prop_val.t_imp;
}

inline void
Property::set_deleted(DB_implem* ldb, DB_node* d, UINT32 h)
{
    _owner_db = ldb;
    _owner_node = d;
    _id = h;
    _deleted = TRUE;
}

inline HXBOOL
Property::is_deleted()
{
    return _deleted;
}

inline void
Property::set_read_only(HXBOOL read_only)
{
    _read_only = read_only;
}

inline HXBOOL
Property::is_read_only()
{
    return _read_only;
}

inline void
Property::SetStringAccessAsBufferById()
{
    _alternate_string_access_ok = TRUE;
}

#endif // _PROPERTY_H
