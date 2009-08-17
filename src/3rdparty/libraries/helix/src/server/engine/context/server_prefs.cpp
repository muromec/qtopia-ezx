/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_prefs.cpp,v 1.4 2005/06/30 00:07:00 dcollins Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxprefs.h"

#include "proc.h"
#include "servreg.h"
#include "hxassert.h"
#include "server_prefs.h"
#include "base_errmsg.h"
#include "hxmon.h"	// for the property type enums
#include "servbuffer.h"

const char* const 	PREF_PROP_NAME_BASE 	= "config.";
const UINT32 		PREF_PROP_NAME_BASE_LEN = 7;
const UINT32 		PREF_PROP_DEFAULT_LEN 	= 255;

ServerPreferences::ServerPreferences(Process* proc)
                 : m_proc(proc), m_registry(m_proc->pc->registry),
		   m_lRefCount(0)
{
    m_prop_name_buf = new char[PREF_PROP_DEFAULT_LEN];
    m_prop_name_buf_len = PREF_PROP_DEFAULT_LEN;
    strcpy(m_prop_name_buf, PREF_PROP_NAME_BASE);
}

ServerPreferences::~ServerPreferences()
{
    delete[] m_prop_name_buf;
}

STDMETHODIMP
ServerPreferences::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPreferences))
    {
	AddRef();
	*ppvObj = (IHXPreferences*) this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ServerPreferences::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
ServerPreferences::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    //ASSERT(FALSE);
    delete this;
    return 0;
}

const char*
ServerPreferences::MakePrefName(const char* pref_key)
{
    if (strlen(pref_key) + PREF_PROP_NAME_BASE_LEN + 1 > m_prop_name_buf_len)
    {
	delete[] m_prop_name_buf;
	m_prop_name_buf_len = PREF_PROP_NAME_BASE_LEN + strlen(pref_key) + 1;
	m_prop_name_buf = new char[m_prop_name_buf_len];
	strcpy(m_prop_name_buf, PREF_PROP_NAME_BASE);
    }
    strcpy(m_prop_name_buf + PREF_PROP_NAME_BASE_LEN, pref_key);
    return m_prop_name_buf;
}

STDMETHODIMP
ServerPreferences::ReadPref(const char* pref_key, IHXBuffer*& buffer)
{
    HX_RESULT	result;

    HXPropType	val_type;
    const char* actual_name;

    actual_name = MakePrefName(pref_key);

    val_type = m_registry->GetType(actual_name, m_proc);
   
    switch(val_type)
    {
	case PT_STRING:
	    result = m_registry->GetStr(actual_name, buffer, m_proc);
	    break;

	case PT_BUFFER:
	    result = m_registry->GetBuf(actual_name, &buffer, m_proc);
	    break;
	    
	case PT_INTEGER:
	    INT32 num;
	    result = m_registry->GetInt(actual_name, &num, m_proc);
	    if (result == HXR_OK)
	    {
		buffer = new ServerBuffer(TRUE);
		buffer->SetSize(50);
		sprintf((char*)buffer->GetBuffer(), "%lu", num);
	    }
	    break;

	default:
	    return HXR_FAIL;
    }

    return result;
}

STDMETHODIMP
ServerPreferences::WritePref(const char* pref_key, IHXBuffer* buffer)
{
    /*
     * Currently we don't allow anyone to write prefs in the server.
     */
    //return HXR_ACCESSDENIED;

//#if 0
    UINT32	result;
    HXPropType	val_type;

    const char* actual_name;
    actual_name = MakePrefName(pref_key);

    val_type = m_registry->GetType(actual_name, m_proc);
  
    if (val_type == PT_UNKNOWN)
    {
    	result = m_registry->AddBuf(actual_name, buffer, m_proc); 
    }
    else
    {
	result = m_registry->SetBuf(actual_name, buffer, m_proc);
    }

    if (result == 0) 
    { 
	return HXR_FAIL; 
    }
    return HXR_OK;
//#endif /* 0 */
}
