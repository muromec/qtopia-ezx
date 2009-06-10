/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: commreg.cpp,v 1.23 2009/05/14 15:02:13 ehyche Exp $
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

#include "hxcom.h"
// #include <stdio.h>
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"

#include "hxtypes.h"
#include "debug.h"
#include "ihxpckts.h"
#include "hxmon.h"
#include "hxstrutl.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "db_misc.h"
#include "key.h"

#include "watchlst.h"
#include "property.h"
#include "db_dict_abs.h"
#include "db_dict.h"
#include "id.h"
#include "commreg.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

CommonRegistry::CommonRegistry()
    : m_LastError(HXR_OK)
    , _count(0)
    , m_pWatchList(NULL)
    , m_lWatchCount(0)
    , m_pContext(NULL)
{
    _logdb_imp = new DB_dict;
    if(!_logdb_imp)
    {
        m_LastError = HXR_OUTOFMEMORY;
    }
    _ids = new CHXID(1000);
    if(!_ids)
    {
        m_LastError = HXR_OUTOFMEMORY;
    }

    DPRINTF(D_REGISTRY, ("CommonRegistry::CommonRegistry -- _logdb_imp(%p), "
                         "_ids(%p)\n", _logdb_imp, _ids));
}

CommonRegistry::~CommonRegistry()
{
    HX_DELETE(_logdb_imp);
    HX_DELETE(_ids);
    HX_RELEASE(m_pContext);
}

void
CommonRegistry::Init(IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
}

/*
 *  Function Name:  	AddComp
 *  Input Params:   	char * prop_name
 *  Return Value:   	UINT32
 *  Description:
 *  	function for adding PT_COMPOSITE nodes in the log database.
 *  PT_COMPOSITE nodes can contain other nodes, this is the structure
 *  demanded by the hierarchical storage.
 */
UINT32
CommonRegistry::AddComp(const char* prop_name)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::AddComp(%s)\n", prop_name));

    HX_RESULT	theErr = HXR_OK;
    DB_node* d = 0;
    Property* p = 0;
    Key* k = new Key(prop_name);
    if(!k)
    {
        return 0;
    }
    int len = k->size();
    char* curr_key_str = new char[len];
    if(!curr_key_str)
    {
        HX_DELETE(k);
        return 0;
    }
    DB_implem* ldb = _logdb_imp;
    DB_node*	new_d = NULL;
    HXBOOL read_only = FALSE;

    /*
     * check if all but the last sub-strings are already present
     * eg.
     *     if foo.bar.moo is to be added then
     *        the following loop checks is "foo" and "bar"
     *        have already been created
     *     after the while loop one final check is made to check
     *     if "moo" NOT already present to avoid adding a DUPLICATE
     */
    *curr_key_str = '\0';
    while(!k->last_sub_str())
    {
        k->append_sub_str(curr_key_str, len);
	if (p && p->get_type() == PT_COMPOSITE)
	    p->get_db_val(&ldb);
	if (!ldb)
	{
	    DPRINTF(D_INFO, ("%s -- %s has NO Properties under it!\n",
		   prop_name, curr_key_str));
	    theErr = HXR_FAILED;
	    goto cleanup;
	}
	d = ldb->find(curr_key_str);
	if (!d)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s was NOT FOUND\n",
	                         prop_name, curr_key_str));
	    theErr = HXR_FAILED;
	    goto cleanup;
	}
	p = d->get_data();
        if (!p)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         prop_name, curr_key_str));
	    theErr = HXR_FAILED;
	    goto cleanup;
	}
	read_only = p->is_read_only();
	if (read_only)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         prop_name, curr_key_str));
	    theErr = HXR_FAILED;
	    goto cleanup;
	}
    }
    if (p && p->get_type() == PT_COMPOSITE)
	p->get_db_val(&ldb);
    k->append_sub_str(curr_key_str, len);
    if (ldb->find(curr_key_str))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", k->get_key_str()));
	theErr = HXR_FAILED;
	goto cleanup;
    }

    // everything is alright add the new property
    new_d = _addComp(k, curr_key_str, ldb);

    AddDone(ldb, new_d, d, p);

cleanup:

    HX_VECTOR_DELETE(curr_key_str);

    if (HXR_OK != theErr)
    {
	if (k)
	{
	    delete k;
	    k = NULL;
	}

	return 0;
    }
    else
    {
	return new_d->get_id();
    }
}

UINT32			
CommonRegistry::_buildSubstructure4Prop(const char* pFailurePoint,
					const char* pProp)
{
    /*
     * A lower composite was not there.
     * Add all of the composites up the to prop.
     */
    UINT32 len = strlen(pProp) + 1;
    Key* lame = new Key(pProp);
    char* temp_key_str = new char[len];
    *temp_key_str = 0;
    while (strlen(temp_key_str) < strlen(pFailurePoint))
    {
	lame->append_sub_str(temp_key_str, len);
    }
    int ret;
    while ((ret = AddComp(temp_key_str)) != 0)
    {
	if (lame->last_sub_str())
	{
	    break;
	}
	lame->append_sub_str(temp_key_str, len);
    }
    delete[] temp_key_str;
    delete lame;
    lame = 0;
    return ret;
}

/*
 *  Function Name:  	AddInt
 *  Input Params:   	const char* prop_name, const INT32 val
 *  Return Value:   	UINT32
 *  Description:
 *  	function to add a property which has an integer value, to
 *  the log database. this will add all the USER DEFINED properties.
 */

UINT32
CommonRegistry::AddInt(const char* prop_name, const INT32 val)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::AddInt(%s, %ld)\n", prop_name, val));
    HX_RESULT theErr = HXR_OK;
    DB_node* d = 0;
    Property* p = 0;
    Key* k = new Key(prop_name);
    if(!k)
    {
        return 0;
    }
    int len = k->size();
    char* curr_key_str = new char[len];
    if(!curr_key_str)
    {
        HX_DELETE(k);
        return 0;
    }
    DB_implem* ldb = _logdb_imp;
    DB_node* new_d = NULL;
    HXBOOL read_only = FALSE;

    *curr_key_str = '\0';
    while(!k->last_sub_str())
    {
        k->append_sub_str(curr_key_str, len);
	if (p && p->get_type() == PT_COMPOSITE)
	    p->get_db_val(&ldb);
	if (!ldb)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	                         prop_name, curr_key_str));
	    theErr = HXR_FAILED;
	    goto cleanup;
	}
	d = ldb->find(curr_key_str);
	if (!d)
	{
	    int ret = _buildSubstructure4Prop(curr_key_str,
		prop_name);

	    if (ret)
	    {
		d = ldb->find(curr_key_str);
	    }
	    if (!ret || !d)
	    {
		theErr = HXR_FAILED;
		goto cleanup;
	    }
	}
	p = d->get_data();
        if (!p)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         prop_name, curr_key_str));
	    theErr = HXR_FAILED;
	    goto cleanup;
	}
	read_only = p->is_read_only();
	if (read_only)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         prop_name, curr_key_str));
	    theErr = HXR_FAILED;
	    goto cleanup;
	}
    }
    if (p && p->get_type() == PT_COMPOSITE)
	p->get_db_val(&ldb);
    k->append_sub_str(curr_key_str, len);
    if (ldb->find(curr_key_str))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", k->get_key_str()));
	theErr = HXR_FAILED;
	goto cleanup;
    }

    // everything is alright add the new property
    new_d = _addInt(k, curr_key_str, val, ldb);

    AddDone(ldb, new_d, d, p);

cleanup:

    HX_VECTOR_DELETE(curr_key_str);

    if (HXR_OK != theErr)
    {
	if (k)
	{
	    delete k;
	    k = NULL;
	}
	return 0;
    }
    else
    {
	return new_d->get_id();
    }
}

/*
 *  Function Name:  	GetInt
 *  Input Params:   	const char* prop_name, INT32* value
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	retrieve the INTEGER property if it exists.
 */
HX_RESULT
CommonRegistry::GetInt(const char* prop_name, INT32* value) const
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return HXR_FAIL;

    if (p)
    {
	switch(p->get_type())
	{
	    case PT_INTEGER:
		return p->get_int_val(value);
		break;

	    case PT_INTREF:
		return p->get_int_ref_val(value);
		break;

	    default:
		DPRINTF(D_REGISTRY, ("%s -- Property<-->Type MISMATCH\n",
		                     prop_name));
		return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

/*
 *  Function Name:  	GetInt
 *  Input Params:   	const UINT32 hash_key, INT32* value
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	retrieve the INTEGER property using the HashTree.
 */
HX_RESULT
CommonRegistry::GetInt(const UINT32 hash_key, INT32* value) const
{
    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("GetInt(%lu) failed\n", hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();
    if (p)
    {
	switch(p->get_type())
	{
	    case PT_INTEGER:
		return p->get_int_val(value);
		break;

	    case PT_INTREF:
		return p->get_int_ref_val(value);
		break;

	    default:
		DPRINTF(D_REGISTRY, ("%lu -- Property<-->Type MISMATCH\n",
		                     hash_key));
		return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

/*
 *  Function Name:  	SetInt
 *  Input Params:   	const char* prop_name, const INT32 value
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	sets the value of an integer property.
 */
HX_RESULT
CommonRegistry::SetInt(const char* prop_name, const INT32 value)
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return HXR_FAIL;

    if (!p)
        return HXR_FAIL;

    if (p->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%s -- Property is READ ONLY\n",
	                     prop_name));
	return HXR_FAIL;
    }

    switch(p->get_type())
    {
        case PT_INTEGER:
            p->int_val(value);
            break;

        case PT_INTREF:
            DPRINTF(D_REGISTRY, ("cannot set INTREF value using SetInt\n"));
            return HXR_FAIL;

        default:
            DPRINTF(D_REGISTRY, ("%s -- Property<-->Type MISMATCH\n",
	                         prop_name));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(d, p);
}

/*
 *  Function Name:  	SetInt
 *  Input Params:   	const UINT32 hash_key, const INT32 value
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set a new value of the INTEGER property using the HashTree.
 */
HX_RESULT
CommonRegistry::SetInt(const UINT32 hash_key, const INT32 value)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::SetInt(%lu, %ld)\n", hash_key,
                         value));
    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("SetInt(%lu) failed\n", hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();
    if (!p)
        return HXR_FAIL;

    if (p->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%s(%lu) -- Property is READ ONLY\n",
	                     p->get_key_str(), hash_key));
	return HXR_FAIL;
    }

    switch(p->get_type())
    {
        case PT_INTEGER:
            p->int_val(value);
            break;

        case PT_INTREF:
            DPRINTF(D_REGISTRY, ("cannot set INTREF value using SetInt\n"));
            return HXR_FAIL;

        default:
            DPRINTF(D_REGISTRY, ("%s(%lu) -- Property<-->Type MISMATCH\n",
	                         p->get_key_str(), hash_key));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(d, p);
}

/*
 *  Function Name:  	CommonRegistry::AddStr
 *  Input Params:   	const char* prop_name, const char* const value,
 *  Return Value:   	UINT32
 *  Description:
 *  	adds a STRING Property ot the registry.
 */
UINT32
CommonRegistry::AddStr(const char* prop_name, IHXBuffer* buf)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::AddStr(%s, buf(%p)\n",
                         prop_name, buf));
    DB_node* d = 0;
    Property* p = 0;
    Key* k = new Key(prop_name);
    if(!k)
    {
        return 0;
    }
    int len = k->size();
    char* curr_key_str = new char[len];
    if(!curr_key_str)
    {
        HX_DELETE(k);
        return 0;
    }
    DB_implem* ldb = _logdb_imp;
    HXBOOL read_only = FALSE;

    *curr_key_str = '\0';
    while(!k->last_sub_str())
    {
        k->append_sub_str(curr_key_str, len);
	if (p && p->get_type() == PT_COMPOSITE)
	    p->get_db_val(&ldb);
	if (!ldb)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	            prop_name, curr_key_str));
	    delete k;
	    HX_VECTOR_DELETE(curr_key_str);
	    return 0;
	}
	d = ldb->find(curr_key_str);
	if (!d)
	{
	    int ret = _buildSubstructure4Prop(curr_key_str,
		prop_name);
	    if (!ret)
	    {
		delete k;
		delete[] curr_key_str;
		return 0;
	    }
	    d = ldb->find(curr_key_str);
	    if (!d)
	    {
		delete k;
		delete[] curr_key_str;
		return 0;
	    }
	}
	p = d->get_data();
        if (!p)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         prop_name, curr_key_str));
	    delete k;
	    HX_VECTOR_DELETE(curr_key_str);
	    return 0;
	}
	read_only = p->is_read_only();
	if (read_only)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         prop_name, curr_key_str));
	    delete k;
	    HX_VECTOR_DELETE(curr_key_str);
	    return 0;
	}
    }
    if (p && p->get_type() == PT_COMPOSITE)
	p->get_db_val(&ldb);
    k->append_sub_str(curr_key_str, len);
    if (ldb->find(curr_key_str))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", k->get_key_str()));
	delete k;
	HX_VECTOR_DELETE(curr_key_str);
	return 0;
    }

    // everything is alright add the new property
    DB_node* new_d = _addBuf(k, curr_key_str, buf, ldb, PT_STRING);

    HX_VECTOR_DELETE(curr_key_str);

    AddDone(ldb, new_d, d, p);
    return new_d->get_id();
}

/*
 *  Function Name:  	GetStr
 *  Input Params:   	const char* prop_name, char*& value
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	retrieves a STRING Property value from the registry given its
 *  property name.
 */
HX_RESULT
CommonRegistry::GetStr(const char* prop_name, REF(IHXBuffer*) p_buf) const
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return HXR_FAIL;

    if (p)
    {
	switch(p->get_type())
	{
	    case PT_STRING:
		return p->get_buf_val(&p_buf, PT_STRING);
		break;

	    default:
		DPRINTF(D_REGISTRY, ("%s -- Property<-->Type MISMATCH\n",
		                     prop_name));
		return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

/*
 *  Function Name:  	GetStr
 *  Input Params:   	const UINT32 hash_key, char*& value
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	retrieves a STRING Property value from the registry given its
 *  hash key.
 */
HX_RESULT
CommonRegistry::GetStr(const UINT32 hash_key, REF(IHXBuffer*) p_buf) const
{
    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("GetStr(%lu) failed\n", hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();

    if (p)
    {
	switch(p->get_type())
	{
	    case PT_STRING:
		return p->get_buf_val(&p_buf, PT_STRING);
		break;

	    default:
		DPRINTF(D_REGISTRY, ("%lu -- Property<-->Type MISMATCH\n",
		                     hash_key));
		return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

/*
 *  Function Name:  	SetStr
 *  Input Params:   	const char* prop_name, const IHXBuffer* value,
 *                      size_t val_len
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set the value of a STRING Property given a new value and its
 *  CORRECT length.
 */
HX_RESULT
CommonRegistry::SetStr(const char* prop_name, IHXBuffer* p_buf)
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return HXR_FAIL;

    if (!p)
        return HXR_FAIL;

    if (p->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%s -- Property is READ ONLY\n",
	                     prop_name));
	return HXR_FAIL;
    }

    switch(p->get_type())
    {
        case PT_STRING:
            p->buf_val(p_buf, PT_STRING);
            break;

        default:
            DPRINTF(D_REGISTRY, ("%s -- Property<-->Type MISMATCH\n",
	                         prop_name));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(d, p);
}

/*
 *  Function Name:  	SetStr
 *  Input Params:   	const UINT32 hash_key, const IHXBuffer* value,
 *                      size_t val_len
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set the value of a STRING Property given a new value and its
 *  CORRECT length.
 */
HX_RESULT
CommonRegistry::SetStr(const UINT32 hash_key, IHXBuffer* p_buf)
{
    DB_node*  d = 0;
    Property* p = 0;
    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("SetStr(%lu) failed\n", hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();

    if (!p)
        return HXR_FAIL;

    if (p->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%lu -- Property is READ ONLY\n",
	                     hash_key));
	return HXR_FAIL;
    }

    switch(p->get_type())
    {
        case PT_STRING:
            p->buf_val(p_buf, PT_STRING);
            break;

        default:
            DPRINTF(D_REGISTRY, ("%lu -- Property<-->Type MISMATCH\n",
	                         hash_key));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(d, p);
}

UINT32
CommonRegistry::AddBuf(const char* prop_name, IHXBuffer* buf)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::AddBuf(%s, buf(%p)\n",
                         prop_name, buf));
    DB_node* d = 0;
    Property* p = 0;
    Key* k = new Key(prop_name);
    if(!k)
    {
        return 0;
    }
    int len = k->size();
    char* curr_key_str = new char[len];
    if(!curr_key_str)
    {
        HX_DELETE(k);
        return 0;
    }
    DB_implem* ldb = _logdb_imp;
    HXBOOL read_only = FALSE;

    *curr_key_str = '\0';
    while(!k->last_sub_str())
    {
        k->append_sub_str(curr_key_str, len);
	if (p && p->get_type() == PT_COMPOSITE)
	    p->get_db_val(&ldb);
	if (!ldb)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	                         prop_name, curr_key_str));
	    HX_VECTOR_DELETE(curr_key_str);
	    delete k;
	    return 0;
	}
	d = ldb->find(curr_key_str);
	if (!d)
	{
	    int ret = _buildSubstructure4Prop(curr_key_str,
		prop_name);
	    if (!ret)
	    {
		delete k;
		delete[] curr_key_str;
		return 0;
	    }
	    d = ldb->find(curr_key_str);
	    if (!d)
	    {
		delete k;
		delete[] curr_key_str;
		return 0;
	    }
	}
	p = d->get_data();
        if (!p)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         prop_name, curr_key_str));
	    HX_VECTOR_DELETE(curr_key_str);
	    delete k;
	    return 0;
	}
	read_only = p->is_read_only();
	if (read_only)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         prop_name, curr_key_str));
	    HX_VECTOR_DELETE(curr_key_str);
	    delete k;
	    return 0;
	}
    }
    if (p && p->get_type() == PT_COMPOSITE)
	p->get_db_val(&ldb);
    k->append_sub_str(curr_key_str, len);
    if (ldb->find(curr_key_str))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", k->get_key_str()));
	HX_VECTOR_DELETE(curr_key_str);
	delete k;
	return 0;
    }

    // everything is alright add the new property
    DB_node* new_d = _addBuf(k, curr_key_str, buf, ldb);

    HX_VECTOR_DELETE(curr_key_str);

    AddDone(ldb, new_d, d, p);
    return new_d->get_id();
}

HX_RESULT
CommonRegistry::GetBuf(const char* prop_name, IHXBuffer** pp_val) const
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return HXR_FAIL;

    if (p)
    {
	if (p->get_type() == PT_BUFFER)
	{
	    return p->get_buf_val(pp_val);
	}
	else if (p->_alternate_string_access_ok)
	{
	    return p->get_buf_val(pp_val, PT_STRING);
	}
	else
	{
	    DPRINTF(D_REGISTRY, ("%s -- Property<-->Type MISMATCH\n",
				 prop_name));
	    return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

HX_RESULT
CommonRegistry::GetBuf(const UINT32 hash_key, IHXBuffer** pp_buf) const
{
    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("GetBuf(%lu) failed\n", hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();

    if (p)
    {
	if (p->get_type() == PT_BUFFER)
	{
	    return p->get_buf_val(pp_buf);
	}
	else if (p->_alternate_string_access_ok)
	{
	    return p->get_buf_val(pp_buf, PT_STRING);
	}
	else
	{
	    DPRINTF(D_REGISTRY, ("%lu -- Property<-->Type MISMATCH\n",
				 hash_key));
	    return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

HX_RESULT
CommonRegistry::SetBuf(const char* prop_name, IHXBuffer* p_buf)
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return HXR_FAIL;

    if (!p)
        return HXR_FAIL;

    if (p->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%s -- Property is READ ONLY\n",
	                     prop_name));
	return HXR_FAIL;
    }

    if (p->get_type() == PT_BUFFER)
    {
	p->buf_val(p_buf);
    }
    else if (p->_alternate_string_access_ok)
    {
	p->buf_val(p_buf, PT_STRING);
    }
    else
    {
	DPRINTF(D_REGISTRY, ("%s -- Property<-->Type MISMATCH\n",
			     prop_name));
	return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(d, p);
}

HX_RESULT
CommonRegistry::SetBuf(const UINT32 hash_key, IHXBuffer* p_buf)
{
    DB_node*  d = 0;
    Property* p = 0;
    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("GetBuf(%lu) failed\n", hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();

    if (!p)
        return HXR_FAIL;

    if (p->is_read_only())
    {
        DPRINTF(D_REGISTRY, ("%lu -- Property is READ ONLY\n",
	                     hash_key));
	return HXR_FAIL;
    }

    switch(p->get_type())
    {
        case PT_BUFFER:
	    p_buf->AddRef();
            p->buf_val(p_buf);
	    p_buf->Release();
            break;

        default:
            DPRINTF(D_REGISTRY, ("%lu -- Property<-->Type MISMATCH\n",
	                         hash_key));
            return HXR_PROP_TYPE_MISMATCH;
    }
    // dispatch the callbacks and then return HXR_OK
    return SetDone(d, p);
}

HX_RESULT
CommonRegistry::SetReadOnly(const char* pName, HXBOOL bValue)
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, pName) != HXR_OK)
	return HXR_FAIL;

    if (!p)
        return HXR_FAIL;

    _setReadOnly(p, bValue);

    return HXR_OK;
}

HX_RESULT
CommonRegistry::SetReadOnly(UINT32 ulRegId, HXBOOL bValue)
{
    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(ulRegId);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("SetReadOnly(%lu) failed\n", ulRegId));
	return HXR_FAIL;
    }
    p = d->get_data();
    if (!p)
        return HXR_FAIL;

    _setReadOnly(p, bValue);

    return HXR_OK;
}

UINT32
CommonRegistry::AddIntRef(const char* prop_name, INT32* val)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::AddIntRef(%s, %ld)\n", prop_name,
                         *val));
    DB_node* d = 0;
    Property* p = 0;
    Key* k = new Key(prop_name);
    if(!k)
    {
        return 0;
    }
    int len = k->size();
    char* curr_key_str = new char[len];
    if(!curr_key_str)
    {
        HX_DELETE(k);
        return 0;
    }
    DB_implem* ldb = _logdb_imp;
    HXBOOL read_only = FALSE;

    *curr_key_str = '\0';
    while(!k->last_sub_str())
    {
        k->append_sub_str(curr_key_str, len);
	if (p && p->get_type() == PT_COMPOSITE)
	    p->get_db_val(&ldb);
	if (!ldb)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	                         prop_name, curr_key_str));
	    HX_VECTOR_DELETE(curr_key_str);
	    return 0;
	}
	d = ldb->find(curr_key_str);
	if (!d)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s was NOT FOUND\n",
	                         prop_name, curr_key_str));
	    HX_VECTOR_DELETE(curr_key_str);
	    return 0;
	}
	p = d->get_data();
        if (!p)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         prop_name, curr_key_str));
	    HX_VECTOR_DELETE(curr_key_str);
	    return 0;
	}
	read_only = p->is_read_only();
	if (read_only)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s is READ ONLY\n",
	                         prop_name, curr_key_str));
	    HX_VECTOR_DELETE(curr_key_str);
	    return 0;
	}
    }
    if (p && p->get_type() == PT_COMPOSITE)
	p->get_db_val(&ldb);
    k->append_sub_str(curr_key_str, len);
    if (ldb->find(curr_key_str))
    {
	DPRINTF(D_REGISTRY, ("%s -- is ALREADY PRESENT!\n", k->get_key_str()));
	HX_VECTOR_DELETE(curr_key_str);
	return 0;
    }

    // everything is alright add the new property
    DB_node* new_d = _addIntRef(k, curr_key_str, val, ldb);

    HX_VECTOR_DELETE(curr_key_str);

    AddDone(ldb, new_d, d, p);
    return new_d->get_id();
}

/*
 *  Function Name:  	Del
 *  Input Params:   	const char* prop_name
 *  Return Value:   	UINT32
 *  Description:
 *  	delete a property (DB_node too) from the database given
 *  the key string.
 */
UINT32
CommonRegistry::Del(const char* prop_name)
{
    DB_node* d = 0;
    Property* p = 0;
    DB_implem* ldb = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return 0;

    if (!p)
    {
	DPRINTF(D_REGISTRY, ("%s -- has NO DATA\n", prop_name));
        return 0;
    }

    // XXXDPS - Currently we do not check if any of the children are read-
    // only before deleting the key. Thus, a user could delete a read-only
    // node by deleting its parent, grandparent, etc. This would slow down
    // the Del() call significantly in the general case because we would
    // have to do a search first, and abort the entire operation if any child
    // is read-only. So it is currently safest to write-protect a key that
    // has no parents (a root key, like "license" or "config").
    if (p->is_read_only())
    {
	DPRINTF(D_REGISTRY, ("%s -- is READ ONLY\n", prop_name));
        return 0;
    }

    // find the DB that contains this node
    ldb = d->get_db();

    if (!ldb)
    {
	DPRINTF(D_REGISTRY, ("%s -- has NO IMPLEMENTATION\n", prop_name));
        return 0;
    }

    // here the response method doesn't exactly end this method
    DeleteDone(ldb, d, p);

    UINT32 h = d->get_id();

    if (p->m_lWatchCount)
    {
	/*
	 * Wait for the clear watches to delete the node
	 */

	p->set_deleted(ldb, d, h);

	return h;
    }

    return _Del(ldb, d, p, h);
}

/*
 *  Function Name:  	Del
 *  Input Params:   	const UINT32 hash_key
 *  Return Value:   	UINT32
 *  Description:
 *  	delete a property (DB_node too) from the database given
 *  the key string.
 */
UINT32
CommonRegistry::Del(const UINT32 hash_key)
{
    DB_node* d = 0;
    Property* p = 0;
    DB_implem* ldb = _logdb_imp;

    d = (DB_node *)_ids->get(hash_key);

    if (!d)
    {
	DPRINTF(D_REGISTRY, ("%lu -- was NOT FOUND\n", hash_key));
	return 0;
    }

    p = d->get_data();

    if (!p)
    {
	DPRINTF(D_REGISTRY, ("%lu -- has NO DATA\n", hash_key));
	return 0;
    }

    if (p->is_read_only())
    {
	DPRINTF(D_REGISTRY, ("%lu -- is READ ONLY\n", hash_key));
        return 0;
    }

    // find the DB that contains this node
    ldb = d->get_db();

    if (!ldb)
    {
	DPRINTF(D_REGISTRY, ("%lu -- has NO IMPLEMENTATION\n", hash_key));
        return 0;
    }

    // here the response method doesn't exactly end this method
    DeleteDone(ldb, d, p);

    if (p->m_lWatchCount)
    {
	/*
	 * Wait for the clear watches to delete the node
	 */

	p->set_deleted(ldb, d, hash_key);

	return hash_key;
    }

    return _Del(ldb, d, p, hash_key);
}

UINT32
CommonRegistry::_Del(DB_implem* ldb, DB_node* d, Property* p, UINT32 h)
{
    if (p->get_type() == PT_COMPOSITE)
    {
	DB_implem* temp_ldb;
	p->get_db_val(&temp_ldb);
	if (_del(temp_ldb) == HXR_FAIL)
	    return 0;
    }

    _ids->destroy(h);
    ldb->del(d);

    _count--;
    return h;
}

UINT32
CommonRegistry::SetWatch(PropWatch* pPropWatch)
{
    /*
     * XXXAAK -- need to do some checking to disallow duplicate watchpoints
     * per process
     */
    WListElem* wle = new WListElem;
    wle->data = pPropWatch;
    m_pWatchList->insert(wle);
    m_lWatchCount++;

    return 1;
}

UINT32
CommonRegistry::SetWatch(const char* prop_name, PropWatch* pPropWatch)
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) == HXR_OK)
    {
	WListElem* wle = new WListElem;
	wle->data = pPropWatch;
	p->m_pWatchList->insert(wle);
	p->m_lWatchCount++;
	return d->get_id();
    }
    return 0;
}

UINT32
CommonRegistry::SetWatch(const UINT32 hash_key, PropWatch* pPropWatch)
{
    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
	return 0;
    p = (Property *)d->get_data();

    if (p)
    {
	WListElem* wle = new WListElem;
	wle->data = pPropWatch;
	p->m_pWatchList->insert(wle);
	p->m_lWatchCount++;
	return hash_key;
    }
    return 0;
}

/*
 *  Function Name:  	SetTrickleWatch
 *  Input Params:   	const char* par_prop_name, const char* target_name,
 *                      PropWatch* cb
 *  Return Value:   	UINT32
 *  Description:
 *  	this method does kinda a watch for a Property that will be added
 *  in the future. when the "target_name" Property gets added somewhere
 *  under the hierarchy of the parent Property "par_prop_name" the watch
 *  will be TRICKLED down the hierarchy until it reaches its target and
 *  set it for the target. if the target never gets added the callback
 *  will NOT be triggered.
 */
HX_RESULT
CommonRegistry::SetTrickleWatch(const char* par_prop_name, 
                                const char* target_name,
			        PropWatch* cb)
{
    // not supported!!!
    return HXR_NOTIMPL; 
}

/*
 *  Function Name:  	CommonRegistry::ClearWatch
 *  Input Params:   	int proc_num
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clear a watchpoint from the global watchlist at the highgest 
 *  level of the database hierarchy.
 */
HX_RESULT
CommonRegistry::ClearWatch(IHXPropWatchResponse* pResonse)
{
    return _clearWatch(pResonse);
}

HX_RESULT
CommonRegistry::ClearWatch(const char* prop_name, IHXPropWatchResponse* pResonse)
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) != HXR_OK)
	return HXR_FAIL;

    return _clearWatch(p, pResonse);
}

HX_RESULT
CommonRegistry::ClearWatch(const UINT32 hash_key, IHXPropWatchResponse* pResonse)
{
    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	return HXR_FAIL;
    }
    p = d->get_data();

    return _clearWatch(p, pResonse);
}

HX_RESULT
CommonRegistry::AddDone(DB_implem* db_level, DB_node* new_node,
	                DB_node* parent_node, Property* parent_prop)
{
    _dispatchParentCallbacks(db_level, new_node, DBE_ADDED);
    
    return HXR_OK;
}

HX_RESULT
CommonRegistry::SetDone(DB_node* new_node, Property* new_prop)
{
    _dispatchCallbacks(new_node, new_prop, DBE_MODIFIED);
    return HXR_OK;
}

HX_RESULT
CommonRegistry::DeleteDone(DB_implem* db_level, DB_node* node,
			   Property* prop)
{
    _dispatchCallbacks(node, prop, DBE_DELETED);
    _dispatchParentCallbacks(db_level, node, DBE_DELETED);
   
    return HXR_OK;
}

DB_node*
CommonRegistry::_addComp(Key* k, char* key_str, DB_implem* ldb)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::_addComp()\n"));
    Property* new_p = new Property(k, PT_COMPOSITE);
    if( !new_p )
    {
        return NULL;
    }

    DB_node* new_d = ldb->add(key_str, new_p);
    if( !new_d )
    {
	delete new_p;
	return NULL;
    }

    DB_dict* pNewDb = new DB_dict(new_d);
    if( !pNewDb )
    {
	delete new_p;
	delete new_d;
	return NULL;
    }
    new_p->db_val(pNewDb);

    UINT32 id = _ids->create((void *)new_d);
    new_d->id(id);
    _count++;

    return new_d;
}

DB_node*
CommonRegistry::_addInt(Key* k, char* key_str, INT32 val, DB_implem* ldb)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::_addInt()\n"));
    Property* new_p = new Property(k, PT_INTEGER);
    if(!new_p)
    {
        return NULL;
    }
    new_p->int_val(val);

    DB_node* new_d = ldb->add(key_str, new_p);
    if (!new_d)
    {
	delete new_p;
	return 0;
    }

    UINT32 id = _ids->create((void *)new_d);
    new_d->id(id);
    _count++;

    return new_d;
}

DB_node*
CommonRegistry::_addBuf(Key* k, char* key_str, IHXBuffer* buf, DB_implem* ldb,
                        HXPropType val_type)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::_addBuf()\n"));
    Property* new_p = new Property(k, val_type);
    if(!new_p)
    {
        return NULL;
    }
    // AddRef gets called within the buf_val() method
    new_p->buf_val(buf, val_type);

    DB_node* new_d = ldb->add(key_str, new_p);
    if (!new_d)
    {
	delete new_p;
	return 0;
    }

    UINT32 id = _ids->create((void *)new_d);
    new_d->id(id);
    _count++;

    return new_d;
}

DB_node*
CommonRegistry::_addIntRef(Key* k, char* key_str, INT32* val, DB_implem* ldb)
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::_addIntRef()\n"));
    Property* new_p = new Property(k, PT_INTREF);
    new_p->int_ref_val(val);

    DB_node* new_d = ldb->add(key_str, new_p);
    if (!new_d)
    {
	delete new_p;
	return 0;
    }

    UINT32 id = _ids->create((void *)new_d);
    new_d->id(id);
    _count++;

    return new_d;
}

/*
 *  Function Name:  	_dispatchParentCallbacks
 *  Input Params:   	DB_implem* db, DB_node* currNode, DB_Event e
 *  Return Value:   	void
 *  Description:
 *      fires off the callbacks of the Parent of the
 *  Property in "currNode". it is used only when a Property gets
 *  added or deleted.
 */
void
CommonRegistry::_dispatchParentCallbacks(DB_implem* db, DB_node* currNode, 
                                         DB_Event e)
{
    DB_node*	    parNode = 0;		// parent's node
    Property*	    parProp = 0;		// parent's property
    UINT32 	    par_hash_key = 0;

    if (_logdb_imp == db)
    {
	if (!m_pWatchList || m_pWatchList->empty())
	    return;

	PropWatch* 	pPropWatch = NULL;
	UINT32 hash_key = currNode->get_id();

	DPRINTF(D_REGISTRY, ("_dPC -- before root loop\n"));
	for (WatchList_iterator wli(m_pWatchList); *wli != 0; ++wli)
	{
	    WListElem* wle = *wli;
	    pPropWatch = (PropWatch *)wle->data;

	    switch (e)
	    {
	    case DBE_ADDED:
		pPropWatch->m_pResponse->AddedProp(hash_key, PT_UNKNOWN, 0);
		break;
	    case DBE_MODIFIED:
		pPropWatch->m_pResponse->ModifiedProp(hash_key, PT_UNKNOWN, 0);
		break;
	    case DBE_DELETED:
		pPropWatch->m_pResponse->DeletedProp(hash_key, 0);
		break;
	    default:
		break;
	    }
	}
    }
    else
    {
	// find the node that owns this DB
	parNode = db->owner_node();
	if (parNode)
	{
	    par_hash_key = parNode->get_id();
	    // get the parent's Property
	    parProp = parNode->get_data();
	    if (!parProp)
	    {
		DPRINTF(D_REGISTRY, ("%s has an INVALID parent Property\n",
		                     currNode->get_data()->get_key_str()));
		return;
	    }
	}
	else
	{
	    DPRINTF(D_REGISTRY, ("%s has an INVALID parent DB_node\n",
	                         currNode->get_data()->get_key_str()));
	    return;
	}

	if (!parProp->m_pWatchList || parProp->m_pWatchList->empty())
	    return;

	UINT32 hash_key = currNode->get_id();
	for (WatchList_iterator wli(parProp->m_pWatchList); *wli != 0; ++wli)
	{
	    WListElem* wle = *wli;
	    PropWatch* pPropWatch = (PropWatch *)wle->data;

	    switch (e)
	    {
	    case DBE_ADDED:
		pPropWatch->m_pResponse->AddedProp(hash_key, PT_UNKNOWN, 
						   par_hash_key);
		break;
	    case DBE_MODIFIED:
		pPropWatch->m_pResponse->ModifiedProp(hash_key, PT_UNKNOWN,
						      par_hash_key);
		break;
	    case DBE_DELETED:
		pPropWatch->m_pResponse->DeletedProp(hash_key, par_hash_key);
		break;
	    default:
		break;
	    }
	}
    }
}

/*
 *  Function Name:  	_dispatchCallbacks
 *  Input Params:   	UINT32 hash_key, Property* p, DB_Event e
 *  Return Value:   	void
 *  Description:
 *  	it fires off the watch callbacks of the Property "p" whenever
 *  it gets modified or deleted.
 */
void
CommonRegistry::_dispatchCallbacks(DB_node* d, Property* p, DB_Event e)
{
    if (p->m_lWatchCount <= 0)
	return;

    UINT32 hash_key = d->get_id();
    UINT32 par_id = 0;

    // find the DB that contains this node
    DB_implem* ldb = d->get_db();
    if (ldb)
    {
	// find the node that owns the DB
	DB_node* par_node = ldb->owner_node();
	if (par_node)
	    par_id = par_node->get_id();
    }

    for (WatchList_iterator wli(p->m_pWatchList); *wli != 0; ++wli)
    {
	WListElem* wle = *wli;
	PropWatch* pPropWatch = (PropWatch *)wle->data;

	switch (e)
	{
	case DBE_ADDED:
	    pPropWatch->m_pResponse->AddedProp(hash_key, PT_UNKNOWN, par_id);
	    break;
	case DBE_MODIFIED:
	    pPropWatch->m_pResponse->ModifiedProp(hash_key, PT_UNKNOWN, par_id);
	    break;
	case DBE_DELETED:
	    pPropWatch->m_pResponse->DeletedProp(hash_key, par_id);
	    break;
	default:
	    break;
	}
    }
}

/*
 *  Function Name:  	CommonRegistry::_clearWatch
 *  Input Params:   	int proc_num
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clear watch at the root level of the registry.
 */
HX_RESULT
CommonRegistry::_clearWatch(IHXPropWatchResponse* pResonse)
{
    for (WatchList_iterator wli(m_pWatchList); *wli != 0; ++wli)
    {
	WListElem* wle = *wli;
	PropWatch* pPropWatch = (PropWatch *)wle->data;

    // Delete the entry that contains pResonse if one was supplied.
    // If pResponse is NULL, blindly delete the first entry in the watch list.
    if ((pPropWatch && pResonse && pPropWatch->m_pResponse == pResonse) ||
        !pResonse)
    {
        m_pWatchList->removeElem(wle);
	    delete wle;
	    delete pPropWatch;
	    m_lWatchCount--;
    }
    }
    
    return HXR_OK;
}

/*
 *  Function Name:  	_clearWatch
 *  Input Params:   	Property* p
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	clears a watch callback (watchpoint) from a Property based on 
 *  the process number. if the Property is not specified, then the
 *  watchpoints at the highgest level are cleared.
 */
HX_RESULT
CommonRegistry::_clearWatch(Property* p, IHXPropWatchResponse* pResonse)
{
    if (p)
    {
	for (WatchList_iterator wli(p->m_pWatchList); *wli != 0; ++wli)
	{
	    WListElem* wle = *wli;
	    PropWatch* pPropWatch = (PropWatch *)wle->data;
	    
        // Delete the entry that contains pResonse if one was supplied.
        // If pResponse is NULL, blindly delete the first entry in the watch list.
        if ((pPropWatch && pResonse && pPropWatch->m_pResponse == pResonse) ||
            !pResonse)
        {
            delete pPropWatch;
	        return DeleteWatch(p, wle);
        }
	}
    }
    return HXR_OK;
}

HX_RESULT
CommonRegistry::DeleteWatch(Property* p, WListElem* wle)
{
    p->m_pWatchList->removeElem(wle);
    delete wle;
    p->m_lWatchCount--;

    if (p->is_deleted() && !p->m_lWatchCount)
    {
	_Del(p->_owner_db, p->_owner_node, p, p->_id);
    }

    return HXR_OK;
}

/*
 *  Function Name:  	GetType
 *  Input Params:   	const char* prop_name
 *  Return Value:   	HXPropType
 *  Description:
 *      returns the Datatype of the Property.
 */
HXPropType
CommonRegistry::GetType(const char* prop_name) const
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) == HXR_OK)
	if (p)
	    return p->get_type();

    return PT_UNKNOWN;
}

/*
 *  Function Name:  	GetType
 *  Input Params:   	const UINT32 hash_key
 *  Return Value:   	HXPropType
 *  Description:
 *      returns the Datatype of the Property.
 */
HXPropType
CommonRegistry::GetType(const UINT32 hash_key) const
{
    DB_node*  d = 0;
    Property* p = 0;
    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("Get Type (%lu) failed\n", hash_key));
	return PT_UNKNOWN;
    }
    p = d->get_data();

    if (p)
	return p->get_type();

    return PT_UNKNOWN;
}

/*
 *  Function Name:  	CommonRegistry::GetPropList const
 *  Input Params:	IHXValues*& pValues
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	it returns back a list of triples one for each property under 
 *  highest level of the database. each of these properties consists of 
 *  name, type and the hash key
 *
 *  XXXAAK -- return a list of hash keys if the list element is PT_COMPOSITE.
 */
HX_RESULT
CommonRegistry::GetPropList(IHXValues*& pValues) const
{
    return _getPropList(_logdb_imp, pValues);
}

/*
 *  Function Name:  	CommonRegistry::GetPropList const
 *  Input Params:   	const char* prop_name, IHXValues*& pValues
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	given a PT_COMPOSITE property, it returns back a list of triples
 *  one for each property under the PT_COMPOSITE. each of which consists 
 *  of property name, type and the hash key
 *
 *  XXXAAK -- return a list of hash keys if the list element is PT_COMPOSITE.
 */
HX_RESULT
CommonRegistry::GetPropList(const char* prop_name, IHXValues*& pValues) const
{
    DB_node*  d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) == HXR_OK)
    {
	if (p)
	{
	    switch(p->get_type())
	    {
		case PT_COMPOSITE:
		{
		    DB_implem* ldb = 0;

		    p->get_db_val(&ldb);
		    return _getPropList(ldb, pValues);
		}
		default:
		    DPRINTF(D_REGISTRY, ("%s -- is NOT a COMPOSITE property\n", 
		                         prop_name));
		    break;
	    }
	}
    }
    return HXR_FAIL;
}

HX_RESULT
CommonRegistry::Copy(const char* pFrom, const char* pTo)
{
    HXPropType type;
    type = GetType(pFrom);
    char buf[256]; /* Flawfinder: ignore */
    INT32 Int;
    UINT32 ul;
    IHXBuffer* pBuffer = 0;
    HX_RESULT res;
    IHXValues* pValues = 0;
    const char* pName;

    switch (type)
    {
    case PT_COMPOSITE:
	/*
	 * Get all props and recurse copy all of them.
	 */
	if (HXR_OK != (res = GetPropList(pFrom, pValues)))
	{
	    break;
	}
	if (HXR_OK == pValues->GetFirstPropertyULONG32(pName, ul))
	{
	    SafeStrCpy(buf, pTo, 256);
	    SafeStrCat(buf, pName + strlen(pFrom), 256);
	    res = Copy(pName, buf);
	    while (HXR_OK == pValues->GetNextPropertyULONG32(pName, ul))
	    {
		SafeStrCpy(buf, pTo, 256);
		SafeStrCat(buf, pName + strlen(pFrom), 256);
		res = Copy(pName, buf);
	    }
	}
	HX_RELEASE(pValues);
	break;

    case PT_INTEGER:
	if (HXR_OK != (res = GetInt(pFrom, &Int)))
	{
	    break;
	}
	if (AddInt(pTo, Int))
	{
	    res = HXR_OK;
	}
	else
	{
	    res = HXR_FAIL;
	}
	break;

    case PT_STRING:
	if (HXR_OK != (res = GetStr(pFrom, pBuffer)))
	{
	    break;
	}
	if (AddStr(pTo, pBuffer))
	{
	    res = HXR_OK;
	}
	else
	{
	    res = HXR_FAIL;
	}
	if (pBuffer)
	{
	    pBuffer->Release();
	}
	pBuffer = 0;
	break;

    case PT_BUFFER:
	if (HXR_OK != (res = GetBuf(pFrom, &pBuffer)))
	{
	    break;
	}
	if (AddBuf(pTo, pBuffer))
	{
	    res = HXR_OK;
	}
	else
	{
	    res = HXR_FAIL;
	}
	if (pBuffer)
	{
	    pBuffer->Release();
	}
	pBuffer = 0;
	break;
    
    default:
	res = HXR_FAIL;
    }

    return res;
}

HX_RESULT
CommonRegistry::SetStringAccessAsBufferById(UINT32 hash_key)
{
    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("SetStringAccessAsBufferById(%lu) failed\n",
	    hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();

    if (p)
    {
	switch(p->get_type())
	{
	    case PT_STRING:
		p->SetStringAccessAsBufferById();
		return HXR_OK;

	    default:
		DPRINTF(D_REGISTRY, ("%lu -- Property<-->Type MISMATCH\n",
		                     hash_key));
		return HXR_PROP_TYPE_MISMATCH;
	}
    }
    return HXR_FAIL;
}

/*
 *  Function Name:  	CommonRegistry::GetPropList const
 *  Input Params:   	const UINT32 hash_key, IHXValues*& pValues
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	given a PT_COMPOSITE property, it returns back a list of triples
 *  one for each property under the PT_COMPOSITE. each of which consists 
 *  of property name, type and the hash key
 *
 *  XXXAAK -- return a list of hash keys if the list element is PT_COMPOSITE.
 */
HX_RESULT
CommonRegistry::GetPropList(const UINT32 hash_key, IHXValues*& pValues) const
{
    if (!hash_key)
	return _getPropList(_logdb_imp, pValues);

    DB_node*  d = 0;
    Property* p = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("could not get PropList for %lu -- no data\n",
	                     hash_key));
	return HXR_FAIL;
    }
    p = d->get_data();

    if (p)
    {
	switch(p->get_type())
	{
	    case PT_COMPOSITE:
	    {
		DB_implem* ldb = 0;

		p->get_db_val(&ldb);
		return _getPropList(ldb, pValues);
	    }
	    default:
		DPRINTF(D_REGISTRY, ("%s(%lu) -- is NOT a COMPOSITE property\n",
		                     p->get_key_str(), hash_key));
		break;
	}
    }
    return HXR_FAIL;
}

/*
 *  Function Name:  	CommonRegistry::GetPropName const
 *  Input Params:   	const UINT32 id, IHXBuffer*& prop_name
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	return a pointer to the key string of the Property whose
 *  id is given.
 */
HX_RESULT
CommonRegistry::GetPropName(const UINT32 id, IHXBuffer*& prop_name) const
{
    DB_node* d = (DB_node *)_ids->get(id);
    if (d)
    {
	Property* p = d->get_data();
	if (p)
	{
	    if (m_pContext)
	    {
		return CreateAndSetBufferCCF(prop_name, (UCHAR*)p->get_key_str(),
					     p->get_key_str_len(), m_pContext);
	    }
#if !defined(HELIX_FEATURE_CLIENT)
	    prop_name = new CHXBuffer;
	    prop_name->Set((const unsigned char *)p->get_key_str(), 
			   p->get_key_str_len());
	    prop_name->AddRef();
	    return HXR_OK;
#endif
	}
    }
    return HXR_FAIL;
}

/*
 *  Function Name:  	GetId
 *  Input Params:   	const char* prop_name
 *  Return Value:   	UINT32
 *  Description:
 *  	returns the ID value for the Property name (prop_name) passed
 *  as a parameter. the main CONDITION is that the Property MUST EXIST,
 *  otherwise it returns a ZERO (0).
 */
UINT32
CommonRegistry::GetId(const char* prop_name) const
{
    DPRINTF(D_REGISTRY&D_ENTRY, ("CommonRegistry::GetId(%s)\n", prop_name));
    DB_node* d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) == HXR_OK)
	if (d)
        return d->get_id();
    return 0;
}

UINT32
CommonRegistry::FindParentKey(const char* prop_name)
{
    DB_node* d = 0;
    Property* p = 0;

    if (_find(&d, &p, prop_name) == HXR_OK)
    {
	if (d)
	{
	    // find the DB that contains this node
	    DB_implem* ldb = d->get_db();
	    if (ldb)
	    {
		// find the node that owns this DB
		d = ldb->owner_node();
		if (d)
		    return d->get_id();
	    }
	}
    }
    return 0;
}

UINT32
CommonRegistry::FindParentKey(const UINT32 hash_key)
{
    DB_node* d = (DB_node *)_ids->get(hash_key);

    // find the node with the hash key
    if (d)
    {
	// find the DB that contains this node
	DB_implem* ldb = d->get_db();
	if (ldb)
	{
	    // find the node that owns this DB
	    d = ldb->owner_node();
	    if (d)
		return d->get_id();
	}
    }
    return 0;
}

/*
 *  Function Name:  	CommonRegistry::Count const
 *  Input Params:   	
 *  Return Value:   	INT32
 *  Description:
 *  	returns the total number of Properties (including COMPOSITE types)
 *  in the registry.
 */
INT32
CommonRegistry::Count() const
{
    return _count;
}

/*
 *  Function Name:  	CommonRegistry::Count
 *  Input Params:   	const char* prop_name
 *  Return Value:   	UINT32
 *  Description:
 *  	returns the number of Properties below the COMPOSITE whose
 *  name has been specified. if the Property is not a COMPOSITE then
 *  ZERO is returned.
 */
INT32
CommonRegistry::Count(const char* prop_name) const
{
    DB_node* d = 0;
    Property* p = 0;
    DB_implem* ldb = 0;
    if (_find(&d, &p, prop_name) != HXR_OK)
	return 0;

    if (p)
    {
	if (p->get_type() == PT_COMPOSITE)
	{
	    p->get_db_val(&ldb);
	    if (ldb)
		return ldb->count();
	}
    }
    return 0;
}

/*
 *  Function Name:  	CommonRegistry::Count
 *  Input Params:   	const UINT32 hash_key
 *  Return Value:   	UINT32
 *  Description:
 *  	returns the number of Properties below the COMPOSITE whose
 *  hash key has been specified. if the Property is not a COMPOSITE
 *  then ZERO is returned.
 */
INT32
CommonRegistry::Count(const UINT32 hash_key) const
{
    DB_node* d = 0;
    Property* p = 0;
    DB_implem* ldb = 0;

    d = (DB_node *)_ids->get(hash_key);
    if (!d)
    {
	DPRINTF(D_REGISTRY, ("%lu -- does not exist\n", hash_key));
	return HXR_FAIL;
    }

    p = d->get_data();
    if (p)
    {
	if (p->get_type() == PT_COMPOSITE)
	{
	    p->get_db_val(&ldb);
	    if (ldb)
		return ldb->count();
	}
    }
    return 0;
}

/*
 *  Function Name:  	CommonRegistry::_del
 *  Input Params:   	DB_implem* db
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	recursive delete down the database, which simultaneously
 *  deletes nodes from the hash tree. this method gets called from
 *  Del() when the user wants to delete a COMPOSITE Property. the
 *  default action is to delete all Properties under that COMPOSITE.
 */
HX_RESULT
CommonRegistry::_del(DB_implem* db)
{
    DPRINTF(D_REGISTRY, ("_del(db(%p)\n", db));
    DB_node* d = db->first();

    while (d)
    {
	Property* pnode = d->get_data();
	DPRINTF(D_REGISTRY, ("_del(db(%p)) -- %s(%lu)\n", db, 
		pnode->get_key_str(), d->get_id()));
	if (pnode)
	{
	    if (pnode->get_type() == PT_COMPOSITE)
	    {
		DB_implem* ldb;
		pnode->get_db_val(&ldb);
		if (!ldb)
		{
		    DPRINTF(D_REGISTRY, ("invalid Property(%lu) not deleted\n",
		                         d->get_id()));
		    return HXR_FAIL;
		}
		_del(ldb);
	    }
	    // fire off the callbacks if someone is watching this prop
	    DeleteDone(db, d, pnode);

	    _ids->destroy(d->get_id());
	    db->del(d);
	    _count--;
	}
	else
	{
	    DPRINTF(D_REGISTRY, ("data corrputed for Property(%lu)\n",
	                         d->get_id()));
	    return HXR_FAIL;
	}
	d = db->first();
    }
    return HXR_OK;
}

/*
 *  Function Name:  	CommonRegistry::_getPropList const
 *  Input Params:   	DB_implem* ldb, IHXValues*& pValues
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	return back a list of triples (name, type, hash) consisting of
 *  all the properties in the database level specified by "ldb". this
 *  method is generally called from GetPropList() when a user requests 
 *  for a list of Properties under some COMPOSITE Property.
 */
HX_RESULT
CommonRegistry::_getPropList(DB_implem* ldb, IHXValues*& pValues) const
{
    DPRINTF(D_REGISTRY, ("_getPropList(ldb(%p))\n", ldb));
    DB_node*  node = 0;

    if (m_pContext)
    {
	CreateValuesCCF(pValues, m_pContext);
    }
    // Client should always provide m_pContext
#if !defined(HELIX_FEATURE_CLIENT)
    else
    {
	pValues = new CHXHeader;
	pValues->AddRef();
    }
#endif

    node = ldb->first();
    while (node)
    {
	Property* p = node->get_data();

	if (!p)
	{
	    DPRINTF(D_REGISTRY, ("%ld is an empty Registry node, skipping it\n",
	                         node->get_id()));
	}
	pValues->SetPropertyULONG32(p->get_key_str(), node->get_id());

	DB_node* n = ldb->next(node);
	node = n;
    }
    return HXR_OK;
}

/*
 *  Copyright (c) 1996, 1997, 1998 RealNetworks
 *
 *  Function Name:  	CommonRegistry::_setReadOnly
 *  Input Params:   	Property* pProp, HXBOOL bValue
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	set the read_only flag of this node, and all nodes beneath
 *	this node (if this node is a composite) to bValue.
 */
HX_RESULT
CommonRegistry::_setReadOnly(Property* pProp, HXBOOL bValue)
{
    DB_implem* ldb  = 0;
    DB_node*   node = 0;

    // Set read_only flag on pProp
    pProp->set_read_only(bValue);

    if (pProp->get_type() == PT_COMPOSITE)
    {
	// Get ldb of pProp
	pProp->get_db_val(&ldb);
	if (ldb)
	{
	    // Set read_only flag on each child
	    node = ldb->first();
	    while (node)
	    {
		Property* p = node->get_data();

		if (!p)
		{
		    DPRINTF(D_REGISTRY, ("%ld is an empty Registry node, "
			"skipping it\n", node->get_id()));
		}
		// Recurse...
		_setReadOnly(p, bValue);

		DB_node* n = ldb->next(node);
		node = n;
	    }
	}
    }

    return HXR_OK;
}

/*
 *  Function Name:  	_find
 *  Input Params:   	DB_node** d, Property** p, 
 *                      UINT32& hash_key, const char* prop_name
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	find a DB_node and its corresponding Property based on the name
 *  or the hash key, whichever is given.
 */
HX_RESULT
CommonRegistry::_find(DB_node** d, Property** p, const char* prop_name) const
{
    DPRINTF(D_REGISTRY, ("CommonRegistry::_find(prop_name(%s))\n", prop_name));

    Key* k = new Key(prop_name);
    if(!k  ||  HXR_OK != k->m_LastError)
    {
        return 0;
    }
    int len = k->size();
    char* curr_key_str = new char[len];
    if(NULL==curr_key_str)
    {
        HX_DELETE(k);
        return 0;
    }
    DB_implem* ldb = _logdb_imp;
    HX_RESULT ret = HXR_OK;

    // find the node that contains the "prop_name"
    *curr_key_str = '\0';
    while(k->append_sub_str(curr_key_str, len))
    {
	if (!ldb)
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO Properties under it!\n",
	                         prop_name, curr_key_str));
	    ret = HXR_FAIL;
	    goto cleanup;
	}
	*d = ldb->find(curr_key_str);
	if (!(*d))
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s was NOT FOUND\n",
	                         prop_name, curr_key_str));
	    ret = HXR_PROP_NOT_FOUND;
	    goto cleanup;
	}

	*p = (*d)->get_data();
        if (!(*p))
	{
	    DPRINTF(D_REGISTRY, ("%s -- %s has NO DATA\n",
	                         prop_name, curr_key_str));
            ret = HXR_FAIL;
	    goto cleanup;
	}
	if ((*p)->get_type() == PT_COMPOSITE)
	    (*p)->get_db_val(&ldb);
    }

    if (*d && *p)
	ret = HXR_OK;
    else
	ret = HXR_FAIL;

cleanup:
    HX_VECTOR_DELETE(curr_key_str);
    delete k;

    return ret;
}
