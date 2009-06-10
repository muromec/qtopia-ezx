/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxclreg.cpp,v 1.7 2007/07/06 21:57:56 jfinnecy Exp $
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

#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxmon.h"

#include "watchlst.h"
#include "property.h"
#include "db_dict.h"
#include "commreg.h"
#include "hxpropwclnt.h"
#include "hxclreg.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXClientRegistry::HXClientRegistry() : 
     m_lRefCount(0)
    ,m_pPropDB(NULL)
    ,m_pContext(NULL)
{
     m_pPropDB = new CommonRegistry;
}

HXClientRegistry::~HXClientRegistry()
{
    Close();
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
HXClientRegistry::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXRegistry), (IHXRegistry*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXRegistry*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXClientRegistry::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXClientRegistry::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

/* 
 * Users of this class should call Init and the context should
 * expose IHXInterruptState and IHXScheduler if they want to
 * ensure that watches are fired ONLY at non-interrupt time.
 *
 * Currently, this functionality is ONLY used by the client core
 * since it is multi-threaded and needs to deal with
 * top level clients which are not thread-safe.
 */
void
HXClientRegistry::Init(IUnknown* pContext)
{
    m_pContext = pContext;
    m_pContext->AddRef();

    m_pPropDB->Init(pContext);
}

void
HXClientRegistry::Close(void)
{
    HX_DELETE(m_pPropDB);
    HX_RELEASE(m_pContext);
}


/************************************************************************
 *  Method:
 *      IHXRegistry::CreatePropWatch
 *  Purpose:
 *      Create a new IHXPropWatch object which can then be queried for the
 *  right kind of IHXPropWatch object.
 */
STDMETHODIMP
HXClientRegistry::CreatePropWatch(IHXPropWatch*& ppObj)
{
    if (!m_pPropDB)
    {
	return HXR_FAIL;
    }

    ppObj = (IHXPropWatch *)new HXClientPropWatch(m_pPropDB, m_pContext);
    if (ppObj)
    {
        ppObj->AddRef();
        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}

/************************************************************************
 *  Method:
 *      IHXRegistry::AddComp
 *  Purpose:
 *      Add a COMPOSITE property to the registry and return its hash
 *  key value if successful. It returns ZERO (0) if an error occured
 *  during the operation.
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::AddComp(const char * new_prop)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddComp(new_prop);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::AddInt
 *  Purpose:
 *      Add an INTEGER property with name in "pName" and value in 
 *  "iValue" to the registry. The return value is the id to
 *  the newly added Property or ZERO if there was an error.
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::AddInt(const char* new_prop, const INT32 val)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddInt(new_prop, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetInt
 *  Purpose:
 *      Retreive an INTEGER value from the registry given its Property
 *  name "pcName" or by its id "id". If the Property 
 *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::GetIntByName(const char* prop_name, INT32& val) const
{
    return m_pPropDB->GetInt(prop_name, &val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetInt
 *  Purpose:
 *      Retreive an INTEGER value from the registry given its Property
 *  name "pcName" or by its id "id". If the Property 
 *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::GetIntById(const UINT32 hash_key, INT32& val) const
{
    return m_pPropDB->GetInt(hash_key, &val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::SetIntByXYZ
 *  Purpose:
 *      Modify a Property's INTEGER value in the registry given the
 *  Property's name "pcName" or its id "id". If the value 
 *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::SetIntByName(const char* prop_name, const INT32 val)
{
    return m_pPropDB->SetInt(prop_name, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::SetIntByXYZ
 *  Purpose:
 *      Modify a Property's INTEGER value in the registry given the
 *  Property's name "pcName" or its id "id". If the value 
 *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::SetIntById(const UINT32 hash_key, const INT32 val)
{
    return m_pPropDB->SetInt(hash_key, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::AddStr
 *  Purpose:
 *      Add an STRING property with name in "pcName" and value in 
 *  "pcValue" to the registry.
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::AddStr(const char* new_prop, IHXBuffer* val)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddStr(new_prop, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetStrByXYZ
 *  Purpose:
 *      Retreive an STRING value from the registry given its Property
 *  name "pcName" or by its id "id". If the Property 
 *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::GetStrByName(const char* prop_name, REF(IHXBuffer*) val) const
{
    val = NULL;

    return m_pPropDB->GetStr(prop_name, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetStrByXYZ
 *  Purpose:
 *      Retreive an STRING value from the registry given its Property
 *  name "pcName" or by its id "id". If the Property 
 *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::GetStrById(const UINT32 hash_key, REF(IHXBuffer*) val) const
{
    val = NULL;

    return m_pPropDB->GetStr(hash_key, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::SetStrByXYZ
 *  Purpose:
 *      Modify a Property's STRING value in the registry given the
 *  Property's name "pcName" or its id "id". If the value 
 *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::SetStrByName(const char* prop_name, IHXBuffer* val)
{
    return m_pPropDB->SetStr(prop_name, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::SetStrByXYZ
 *  Purpose:
 *      Modify a Property's STRING value in the registry given the
 *  Property's name "pcName" or its id "id". If the value 
 *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::SetStrById(const UINT32 hash_key, IHXBuffer* val)
{
    return m_pPropDB->SetStr(hash_key, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::AddBuf
 *  Purpose:
 *      Add an BUFFER property with name in "pcName" and value in 
 *  "pValue" to the registry.
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::AddBuf(const char* new_prop, IHXBuffer* p_buf)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddBuf(new_prop, p_buf);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetBufByName
 *  Purpose:
 *      Retreive the BUFFER from the registry given its Property
 *  name "pcName" or its id "id". If the Property 
 *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::GetBufByName(const char* prop_name, IHXBuffer*& pp_buf) const
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->GetBuf(prop_name, &pp_buf);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetBufById
 *  Purpose:
 *      Retreive the BUFFER from the registry given its Property
 *  name "pcName" or its id "id". If the Property 
 *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::GetBufById(const UINT32 hash_key, IHXBuffer*& pp_buf) const
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->GetBuf(hash_key, &pp_buf);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::SetBufByXYZ
 *  Purpose:
 *      Modify a Property's BUFFER in the registry given the
 *  Property's name "pcName" or its id "id". If the value 
 *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::SetBufByName(const char* prop_name, IHXBuffer* p_buf)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->SetBuf(prop_name, p_buf);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::SetBufByXYZ
 *  Purpose:
 *      Modify a Property's BUFFER in the registry given the
 *  Property's name "pcName" or its id "id". If the value 
 *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
 */
STDMETHODIMP
HXClientRegistry::SetBufById(const UINT32 hash_key, IHXBuffer* p_buf)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->SetBuf(hash_key, p_buf);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::AddIntRef
 *  Purpose:
 *      Add an INTEGER REFERENCE property with name in "pcName" and 
 *  value in "iValue" to the registry. This property allows the user
 *  to modify its contents directly, without having to go through the
 *  registry.
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::AddIntRef(const char* new_prop, INT32* val)
{
    return m_pPropDB->AddIntRef(new_prop, val);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::DeleteByXYZ
 *  Purpose:
 *      Delete a Property from the registry using its name "pcName"
 *  or id "id".
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::DeleteByName(const char* prop_name)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->Del(prop_name);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::DeleteByXYZ
 *  Purpose:
 *      Delete a Property from the registry using its name "pcName"
 *  or id "id".
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::DeleteById(const UINT32 hash_key)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->Del(hash_key);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetType
 *  Purpose:
 *      Returns the datatype of the Property given its name "pcName"
 *  or its id "id".
 */
STDMETHODIMP_(HXPropType)
HXClientRegistry::GetTypeByName(const char* prop_name) const
{
    return m_pPropDB->GetType(prop_name);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetType
 *  Purpose:
 *      Returns the datatype of the Property given its name "pcName"
 *  or its id "id".
 */
STDMETHODIMP_(HXPropType)
HXClientRegistry::GetTypeById(const UINT32 hash_key) const
{
    return m_pPropDB->GetType(hash_key);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::FindParentIdByName
 *  Purpose:
 *      Returns the id value of the parent node of the Property
 *  whose name (prop_name) or id (id) has been specified.
 *  If it fails, a ZERO value is returned.
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::FindParentIdByName(const char* prop_name) const
{
    return m_pPropDB->FindParentKey(prop_name);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::FindParentIdById
 *  Purpose:
 *      Returns the id value of the parent node of the Property
 *  whose name (prop_name) or id (id) has been specified.
 *  If it fails, a ZERO value is returned.
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::FindParentIdById(const UINT32 hash_key) const
{
    return m_pPropDB->FindParentKey(hash_key);
}


/************************************************************************
 *  Method:
 *      HXRegistry::GetPropName
 *  Purpose:
 *      Returns the Property name in the ppcName char buffer passed
 *  as a parameter, given the Property's id "ulId".
 */
STDMETHODIMP
HXClientRegistry::GetPropName(const UINT32 id, IHXBuffer*& prop_name) const
{
    return m_pPropDB->GetPropName(id, prop_name);
}


/************************************************************************
 *  Method:
 *      HXRegistry::GetId
 *  Purpose:
 *      Returns the Property's id given the Property name.
 */
STDMETHODIMP_(UINT32)
HXClientRegistry::GetId(const char* prop_name) const
{
    return m_pPropDB->GetId(prop_name);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetPropListOfRoot
 *  Purpose:
 *      It returns back a list of Properties as an IHXValues (prop_name
 *  and its id pair) at the root level of the registry's hierarchy.
 */
STDMETHODIMP
HXClientRegistry::GetPropListOfRoot(IHXValues*& pValues) const
{
    return m_pPropDB->GetPropList(pValues);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetPropListByName
 *  Purpose:
 *      Returns a list of Properties immediately under the one with
 *  name "pcName" or id "id".
 */
STDMETHODIMP
HXClientRegistry::GetPropListByName(const char* prop_name, 
                                    IHXValues*& pValues) const
{
    return m_pPropDB->GetPropList(prop_name, pValues);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetPropListById
 *  Purpose:
 *      Returns a list of Properties immediately under the one with
 *  name "pcName" or id "id".
 */
STDMETHODIMP
HXClientRegistry::GetPropListById(const UINT32 hash_key,
                                  IHXValues*& pValues) const
{
    return m_pPropDB->GetPropList(hash_key, pValues);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetNumPropsAtRoot
 *  Purpose:
 *      Returns the count of the number of Properties within the
 *  registry. If a property name of id is specified, then it
 *  returns the number of Properties under it.
 */
STDMETHODIMP_(INT32)
HXClientRegistry::GetNumPropsAtRoot() const
{
    return m_pPropDB->Count();
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetNumPropsByName
 *  Purpose:
 *      Returns the count of the number of Properties within the
 *  registry. If a property name of id is specified, then it
 *  returns the number of Properties under it.
 */
STDMETHODIMP_(INT32)
HXClientRegistry::GetNumPropsByName(const char* prop_name) const
{
    return m_pPropDB->Count(prop_name);
}

/************************************************************************
 *  Method:
 *      IHXRegistry::GetNumPropsById
 *  Purpose:
 *      Returns the count of the number of Properties within the
 *  registry. If a property name of id is specified, then it
 *  returns the number of Properties under it.
 */
STDMETHODIMP_(INT32)
HXClientRegistry::GetNumPropsById(const UINT32 hash_key) const
{
    return m_pPropDB->Count(hash_key);
}
