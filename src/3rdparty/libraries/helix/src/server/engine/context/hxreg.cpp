/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxreg.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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

#include <stdio.h>
#include <string.h>

#include "debug.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "regdb_misc.h"
#include "hxmon.h"

#include "proc.h"
#include "simple_callback.h"

#include "base_errmsg.h"
#include "servreg.h"
#include "hxmap.h"
#include "hxreg.h"
#include "hxpropw.h"

HX_RESULT
HXCreatePropDBInstance(IHXRegistry** ppHXRegistry)
{
    HXRegistry* pReg = new HXRegistry;
    *ppHXRegistry = (IHXRegistry*)pReg;
    if (*ppHXRegistry)
    {
        (*ppHXRegistry)->AddRef();
        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}

//////////////////////////// HXRegistry methods ///////////////////////////

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
HXRegistry::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXRegistry2))
    {
        AddRef();
        *ppvObj = (IHXRegistry2*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRegistry))
    {
        AddRef();
        *ppvObj = (IHXRegistry*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXActiveRegistry))
    {
	AddRef();
	*ppvObj = (IHXActiveRegistry*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXCopyRegistry))
    {
	AddRef();
	*ppvObj = (IHXCopyRegistry*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXRegistryAltStringHandling))
    {
	AddRef();
	*ppvObj = (IHXRegistryAltStringHandling*)this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXRegistry::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
HXRegistry::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    
    delete this;
    return 0;
}   


HXRegistry::HXRegistry(ServerRegistry* db, Process* p) 
: m_ulRefCount(0)
, m_pPropDB(db)
, proc(p)
{
}

HXRegistry::~HXRegistry()
{
}


/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::Init
 *  Input Params:   	ServerRegistry* db, Process* p
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	initialize with the Monitor's process and the Property database
 *  in shared memory.
 */
STDMETHODIMP
HXRegistry::Init(ServerRegistry* db, Process* p)
{
    m_pPropDB = db;
    proc   = p;
    procnum = p->procnum();

    return HXR_OK;
}

STDMETHODIMP
HXRegistry::CreatePropWatch(IHXPropWatch*& ppObj)
{
    if (!m_pPropDB)
    {
	ERRMSG(proc->pc->error_handler, "registry has not been initialized\n");
	return HXR_FAIL;
    }

    ppObj = new HXPropWatch(m_pPropDB, proc);
    if (ppObj)
    {
        ppObj->AddRef();
        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::AddComp
 *  Input Params:   	char* new_prop
 *  Return Value:   	HX_RESULT 
 *  Description:
 *  	adds a property which will contain other properties.
 *  (PT_COMPOSITE type)
 */
STDMETHODIMP_(UINT32)
HXRegistry::AddComp(const char * new_prop)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddComp(new_prop, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	AddInt
 *  Input Params:   	char* new_prop, const INT32 value
 *  Return Value:   	HX_RESULT 
 *  Description:
 *  	adds user defined properties to the log database. this way
 *  the user defined properties can be shared by all modules accessing
 *  the database.
 */
STDMETHODIMP_(UINT32)
HXRegistry::AddInt(const char* new_prop, const INT32 val)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddInt(new_prop, val, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetIntByName
 *  Input Params:   	char* prop_name, INT32& val
 *  Return Value:   	HX_RESULT 
 *  Description:
 *  	get the value of a particular property. it returns a error if
 *  the property is not found or there is a type mismatch between the
 *  property in the log database and the value_to_be_returned.
 */
STDMETHODIMP
HXRegistry::GetIntByName(const char* prop_name, INT32& val) const
{
    return m_pPropDB->GetInt(prop_name, &val, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetIntById
 *  Input Params:   	UINT32 ulId, INT32& val
 *  Return Value:   	HX_RESULT 
 *  Description:
 *  	get the value of a particular property. it returns a error if
 *  the property is not found or there is a type mismatch between the
 *  property in the log database and the value_to_be_returned.
 */
STDMETHODIMP
HXRegistry::GetIntById(const UINT32 ulId, INT32& val) const
{
    return m_pPropDB->GetInt(ulId, &val, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetIntByName
 *  Input Params:   	const char* prop_name, const INT32& val
 *  Return Value:   	HX_RESULT 
 *  Description:
 *  	set the value of a property in the log database. it returns an
 *  error if the property could not be found or if the user does not
 *  have permission to write to it (this is the case when s/he tries
 *  to overwrite the default properties provided by the server)
 */
STDMETHODIMP
HXRegistry::SetIntByName(const char* prop_name, const INT32 val)
{
    return m_pPropDB->SetInt(prop_name, val, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	SetIntById
 *  Input Params:   	const UINT32 ulId, const INT32& val
 *  Return Value:   	HX_RESULT 
 *  Description:
 *  	set the value of a property in the log database. it returns an
 *  error if the property could not be found or if the user does not
 *  have permission to write to it (this is the case when s/he tries
 *  to overwrite the default properties provided by the server)
 */
STDMETHODIMP
HXRegistry::SetIntById(const UINT32 ulId, const INT32 val)
{
    return m_pPropDB->SetInt(ulId, val, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::AddStr
 *  Input Params:   	const char* new_prop, const char* const val
 *  Return Value:   	STDMETHODIMP_(UINT32)
 *  Description:
 *  	add a Property with a STRING value.
 */
STDMETHODIMP_(UINT32)
HXRegistry::AddStr(const char* new_prop, IHXBuffer* pValue)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddStr(new_prop, pValue, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetStrByName
 *  Input Params:   	const char* prop_name, IHXBuffer*& val
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	get the value of a particular property. it returns a error if
 *  the property is not found or there is a type mismatch between the
 *  property in the log database and the value_to_be_returned.
 */
STDMETHODIMP
HXRegistry::GetStrByName(const char* prop_name, REF(IHXBuffer*) val) const
{
    val = NULL;
    return m_pPropDB->GetStr(prop_name, val, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetStrById
 *  Input Params:   	const UINT32 ulId, IHXBuffer*& val
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	get the value of a particular property. it returns a error if
 *  the property is not found or there is a type mismatch between the
 *  property in the log database and the value_to_be_returned.
 */
STDMETHODIMP
HXRegistry::GetStrById(const UINT32 ulId, REF(IHXBuffer*) val) const
{
    val = NULL;
    return m_pPropDB->GetStr(ulId, val, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::SetStrByName
 *  Input Params:   	const char* prop_name, const char* const val
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	sets the STRING value of the Property (prop_name) to a new
 *  value (val).
 */
STDMETHODIMP
HXRegistry::SetStrByName(const char* prop_name, IHXBuffer* pValue)
{
    return m_pPropDB->SetStr(prop_name, pValue, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::SetStrById
 *  Input Params:   	const UINT32 ulId, const char* const val
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	sets the STRING value of the Property (ulId) to a new
 *  value (val).
 */
STDMETHODIMP
HXRegistry::SetStrById(const UINT32 ulId, IHXBuffer* pValue)
{
    return m_pPropDB->SetStr(ulId, pValue, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::AddBuf
 *  Input Params:   	const char* new_prop, IHXBuffer* p_buf
 *  Return Value:   	STDMETHODIMP_(UINT32)
 *  Description:
 *  	add a new Property having an IHXBuffer value.
 */
STDMETHODIMP_(UINT32)
HXRegistry::AddBuf(const char* new_prop, IHXBuffer* p_buf)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddBuf(new_prop, p_buf, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetBufByName
 *  Input Params:   	const char* prop_name, IHXBuffer*& pp_buf
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	return a pointer by setting the "pp_buf" parameter to the IHXBuffer 
 *  value in the Property whose name is specified in "prop_name".
 */
STDMETHODIMP
HXRegistry::GetBufByName(const char* prop_name, IHXBuffer*& pp_buf) const
{
    // if the return value is HXR_FAIL then the operation failed
    return m_pPropDB->GetBuf(prop_name, &pp_buf, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetBufById
 *  Input Params:   	const UINT32 ulId, IHXBuffer*& pp_buf
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	return a pointer by setting the "pp_buf" parameter to the IHXBuffer 
 *  value in the Property whose hash key is specified in "ulId".
 */
STDMETHODIMP
HXRegistry::GetBufById(const UINT32 ulId, IHXBuffer*& pp_buf) const
{
    return m_pPropDB->GetBuf(ulId, &pp_buf, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::SetBufByName
 *  Input Params:   	const char* prop_name, IHXBuffer* p_buf
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	sets the Property's (prop_name) IHXBuffer value to a new value.
 */
STDMETHODIMP
HXRegistry::SetBufByName(const char* prop_name, IHXBuffer* p_buf)
{
    return m_pPropDB->SetBuf(prop_name, p_buf, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::SetBufById
 *  Input Params:   	const UINT32 ulId, IHXBuffer* p_buf
 *  Return Value:   	STDMETHODIMP
 *  Description:
 *  	sets the Property's (ulId) IHXBuffer value to a new value.
 */
STDMETHODIMP
HXRegistry::SetBufById(const UINT32 ulId, IHXBuffer* p_buf)
{
    return m_pPropDB->SetBuf(ulId, p_buf, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::AddIntRef
 *  Input Params:   	const char* new_prop, INT32* val
 *  Return Value:   	STDMETHODIMP_(UINT32)
 *  Description:
 *  	add a new Property whose value is a reference to an Integer.
 *  the value of this Property is modified thru' dereferencing the
 *  pointer (by the owner only) and it is mainly used for
 *  storing information that changes very rapidly (many times a second.)
 */
STDMETHODIMP_(UINT32)
HXRegistry::AddIntRef(const char* new_prop, INT32* val)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->AddIntRef(new_prop, val, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	DeleteByName
 *  Input Params:   	char* prop_name
 *  Return Value:   	HX_RESULT 
 *  Description:
 *  	delete any user defined property. it returns an error if the
 *  property is not found or if the user tries to delete a server
 *  defined property.
 */
STDMETHODIMP_(UINT32)
HXRegistry::DeleteByName(const char* prop_name)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->Del(prop_name, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	DeleteById
 *  Input Params:   	UINT32 ulId
 *  Return Value:   	HX_RESULT 
 *  Description:
 *  	delete any user defined property. it returns an error if the
 *  property is not found or if the user tries to delete a server
 *  defined property.
 */
STDMETHODIMP_(UINT32)
HXRegistry::DeleteById(const UINT32 ulId)
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->Del(ulId, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::GetTypeByName const
 *  Input Params:   	const char* prop_name
 *  Return Value:   	STDMETHODIMP_(HXPropType)
 *  Description:
 *  	returns the datatype of the Property given its name (prop_name).
 */
STDMETHODIMP_(HXPropType)
HXRegistry::GetTypeByName(const char* prop_name) const
{
    return m_pPropDB->GetType(prop_name, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	HXRegistry::GetTypeById const
 *  Input Params:   	const UINT32 ulId
 *  Return Value:   	STDMETHODIMP_(HXPropType)
 *  Description:
 *  	returns the datatype of the Property given it hash key (ulId).
 */
STDMETHODIMP_(HXPropType)
HXRegistry::GetTypeById(const UINT32 ulId) const
{
    return m_pPropDB->GetType(ulId, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	FindParentIdByName
 *  Input Params:   	const char* prop_name
 *  Return Value:   	UINT32
 *  Description:
 *  	return the hash key of the parent node of "prop_name"
 */
STDMETHODIMP_(UINT32)
HXRegistry::FindParentIdByName(const char* prop_name) const
{
    // if the return value is ZERO then the operation failed
    return m_pPropDB->FindParentKey(prop_name, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	FindParentIdById
 *  Input Params:   	const UINT32 ulId, IHXBuffer*& prop_name
 *  Return Value:   	UINT32
 *  Description:
 *  	return the hash key of the parent node with "ulId"
 */
STDMETHODIMP_(UINT32)
HXRegistry::FindParentIdById(const UINT32 ulId) const
{
    return m_pPropDB->FindParentKey(ulId, proc);
}

STDMETHODIMP
HXRegistry::GetPropName(const UINT32 id, IHXBuffer*& prop_name) const
{
    return m_pPropDB->GetPropName(id, prop_name, proc);
}

STDMETHODIMP_(UINT32)
HXRegistry::GetId(const char* prop_name) const
{
    return m_pPropDB->GetId(prop_name, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetPropListOfRoot
 *  Input Params:
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	it returns a list of properties at the highest level
 *  in the database hierarchy.
 */
STDMETHODIMP
HXRegistry::GetPropListOfRoot(IHXValues*& pValueList) const
{
    return m_pPropDB->GetPropList(pValueList, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetPropListByName
 *  Input Params:   	const char* prop_name
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	get a list of properties under "prop_name". if "prop_name"
 *  is ZERO then it returns a list of properties at the highest level
 *  in the database hierarchy.
 */
STDMETHODIMP
HXRegistry::GetPropListByName(const char* prop_name, 
                              IHXValues*& pValueList) const
{
    return m_pPropDB->GetPropList(prop_name, pValueList, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetPropListById
 *  Input Params:   	const UINT32 ulId
 *  Return Value:   	HX_RESULT
 *  Description:
 *  	get a list of properties under the Property with hash key 
 *  "ulId".
 */
STDMETHODIMP
HXRegistry::GetPropListById(const UINT32 ulId,
                              IHXValues*& pValueList) const
{
    return m_pPropDB->GetPropList(ulId, pValueList, proc);
}

/*
 *  Copyright (c) 1996, 1997 Progressive Networks
 *
 *  Function Name:  	GetNumPropsAtRoot
 *  Input Params:   	
 *  Return Value:   	INT32
 *  Description:
 *  	returns a count of the number of elements in the database.
 */
STDMETHODIMP_(INT32)
HXRegistry::GetNumPropsAtRoot() const
{
    return m_pPropDB->Count(proc);
}

STDMETHODIMP_(INT32)
HXRegistry::GetNumPropsByName(const char* prop_name) const
{
    return m_pPropDB->Count(prop_name, proc);
}

STDMETHODIMP_(INT32)
HXRegistry::GetNumPropsById(const UINT32 ulId) const
{
    return m_pPropDB->Count(ulId, proc);
}

/************************************************************************
* IHXActiveRegistry::SetAsActive
*
*     Method to set prop pName to active and register pUser as
*   the active prop user.
*/
STDMETHODIMP
HXRegistry::SetAsActive(const char* pName,
			IHXActivePropUser* pUser)
{
    return m_pPropDB->SetAsActive(pName, pUser, proc);
}

/************************************************************************
* IHXActiveRegistry::SetAsInactive
*
*	Method to remove an IHXActiveUser from Prop activation.
*/
STDMETHODIMP
HXRegistry::SetAsInactive(const char* pName,
			IHXActivePropUser* pUser)
{
    return m_pPropDB->SetAsInactive(pName, pUser, proc);
}

/************************************************************************
* IHXActiveRegistry::IsActive
*
*     Tells if prop pName has an active user that must be queried to
*   change the value, or if it can just be set.
*/
STDMETHODIMP_(BOOL)
HXRegistry::IsActive(const char* pName)
{
    return m_pPropDB->IsActive(pName, proc);
}

/************************************************************************
* IHXActiveRegistry::SetActiveInt
*
*    Async request to set int pName to ul.
*/
STDMETHODIMP
HXRegistry::SetActiveInt(const char* pName,
			UINT32 ul, 
			IHXActivePropUserResponse* pResponse)
{
    return m_pPropDB->SetActiveInt(pName, ul, pResponse, proc);
}

/************************************************************************
* IHXActiveRegistry::SetStr
*
*    Async request to set string pName to string in pBuffer.
*/
STDMETHODIMP
HXRegistry::SetActiveStr(const char* pName,
			IHXBuffer* pBuffer,
			IHXActivePropUserResponse* pResponse)
{
    return m_pPropDB->SetActiveStr(pName, pBuffer, pResponse, proc);
}

/************************************************************************
* IHXActiveRegistry::SetBuffer
*
*    Async request to set buffer pName to buffer in pBuffer.
*/
STDMETHODIMP
HXRegistry::SetActiveBuf(const char* pName,
			    IHXBuffer* pBuffer,
			    IHXActivePropUserResponse* pResponse)
{
    return m_pPropDB->SetActiveBuf(pName, pBuffer, pResponse, proc);
}

/************************************************************************
* IHXActivePropUser::DeleteActiveProp
*
*	Async request to delete the active property.
*/
STDMETHODIMP
HXRegistry::DeleteActiveProp(const char* pName,
			    IHXActivePropUserResponse* pResponse)
{
    return m_pPropDB->DeleteActiveProp(pName, pResponse, proc);
}

/************************************************************************
* IHXCopyRegistry::Copy
*
*   Here it is! The "Copy" method!
*/
STDMETHODIMP
HXRegistry::CopyByName(const char* pFrom,
			const char* pTo)
{
    return m_pPropDB->Copy(pFrom, pTo, proc);
}

STDMETHODIMP
HXRegistry::SetStringAccessAsBufferById(UINT32 ulId)
{
    return m_pPropDB->SetStringAccessAsBufferById(ulId, proc);
}



/************************************************************************
 * IHXRegistry2::ModifyIntByName
 *
 *      Changes the INTEGER value in the registry given its Property
 *  name "pName" and an amount to change it by.  Modifies the value
 *  of the integer in the registry by the amount specified by "nDelta",
 *  setting nValue equal to the value after modification.  If the
 *  Property is found, it will return HXR_OK, otherwise it
 *  returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nDelta - IN - amount to modify the named property by
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned, after modification
 */
STDMETHODIMP
HXRegistry::ModifyIntByName(const char*     pName,
                            INT32           nDelta,
                            REF(INT32)      nValue)
{
    return m_pPropDB->ModifyInt(pName, nDelta, &nValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::ModifyIntById
 *  Purpose:
 *      Changes the INTEGER value in the registry given its id "ulID"
 *  and an amount to change it by.  Modifies the value of the
 *  integer in the registry by the amount specified by "nDelta",
 *  setting nValue equal to the value after modification.  If the
 *  Property is found, it will return HXR_OK, otherwise it
 *  returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  ulId - IN - unique id of the Property whose value is to be modified
 *  nDelta - IN - amount to modify the specified property by
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned, after modification
 */
STDMETHODIMP
HXRegistry::ModifyIntById(const UINT32    id,
                          INT32           nDelta,
                          REF(INT32)      nValue)
{
    return m_pPropDB->ModifyInt(id, nDelta, &nValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::BoundedModifyIntByName
 *  Purpose:
 *      Changes the INTEGER value in the registry given its Property name
 *  "pName" and an amount to change it by and keeps the modified value 
 *  within the bounds of the nMin and nMax values. Modifies the value 
 *  of the integer in the registry by the amount specified by "nDelta",
 *  setting nValue equal to the value after modification, if the modified 
 *  value is >= nMin and <= nMax. If either of these limits are violated 
 *  the the resulting value stored in the registry is the value of the 
 *  limit just violated. If the Property is found, it will return HXR_OK, 
 *  otherwise it returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nDelta - IN - amount to modify the named property by
 *  nMin - IN - min value that the modified registry prop can have
 *              if the modified registry value < nMin then
 *                  registry value = nMin
 *  nMax - IN - min value that the modified registry prop can have
 *              if the modified registry value > nMax then
 *                  registry value = nMax
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned, after modification
 */
STDMETHODIMP
HXRegistry::BoundedModifyIntByName(const char*     pName,
				   INT32           nDelta,
				   REF(INT32)      nValue,
				   INT32           nMin,
				   INT32           nMax)
{
    return m_pPropDB->BoundedModifyInt(pName, nDelta, &nValue, proc, nMin, 
	nMax);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::BoundedModifyIntById
 *  Purpose:
 *      Changes the INTEGER value in the registry given its id "ulID"
 *  and an amount to change it by and keeps the modified value within
 *  the bounds of the nMin and nMax values. Modifies the value of the
 *  integer in the registry by the amount specified by "nDelta",
 *  setting nValue equal to the value after modification, if the modified
 *  value is >= nMin and <= nMax. If either of these limits are violated
 *  the the resulting value stored in the registry is the value of the
 *  limit just violated. If the Property is found, it will return HXR_OK, 
 *  otherwise it returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  ulId - IN - unique id of the Property whose value is to be modified
 *  nDelta - IN - amount to modify the specified property by
 *  nMin - IN - min value that the modified registry prop can have
 *              if the modified registry value < nMin then
 *                  registry value = nMin
 *  nMax - IN - min value that the modified registry prop can have
 *              if the modified registry value > nMax then
 *                  registry value = nMax
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned, after modification
 */
STDMETHODIMP
HXRegistry::BoundedModifyIntById(const UINT32    id,
				 INT32           nDelta,
				 REF(INT32)      nValue,
				 INT32           nMin,
				 INT32           nMax)
{
    return m_pPropDB->BoundedModifyInt(id, nDelta, &nValue, proc, nMin, nMax);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::SetAndReturnIntByName
 *  Purpose:
 *      Modify a Property's INTEGER value in the registry given the
 *  Property's name "pName". If the Property is found, the previous
 *  value, prior to setting it, will be assigned to nOldValue.
 *  If the Property is found, it will return HXR_OK, otherwise it
 *  returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nValue - IN - the new value of the Property which is going to be set
 *  nOldValue - OUT - parameter into which the previous value of the
 *                    Property is returned
 */
STDMETHODIMP
HXRegistry::SetAndReturnIntByName(const char*     pName,
                                  INT32           nValue,
                                  REF(INT32)      nOldValue)
{
    return m_pPropDB->SetAndReturnInt(pName, nValue, &nOldValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::SetAndReturnIntById
 *  Purpose:
 *      Modify a Property's INTEGER value in the registry given the
 *  Property's id "ulId". If the id is found, the previous
 *  value, prior to setting it, will be assigned to nOldValue.
 *  If the Property is found, it will return HXR_OK, otherwise it
 *  returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nValue - IN - the new value of the Property which is going to be set
 *  nOldValue - OUT - parameter into which the previous value of the
 *                    Property is returned
 */
STDMETHODIMP
HXRegistry::SetAndReturnIntById(const UINT32    id,
                                INT32           nValue,
                                REF(INT32)      nOldValue)
{
    return m_pPropDB->SetAndReturnInt(id, nValue, &nOldValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::GetIntRefByName
 *  Purpose:
 *      Retrieve an INTEGER REFERENCE property from the registry given
 *  its Property name "pName".  If the Property is found it will return
 *  HXR_OK and pValue will be assigned the address of the integer
 *  (not the value of the integer, which can be obtained even for
 *  INTREFs via the GetIntByxxx methods.)  Otherwise, it returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  pValue - OUT - the address of the integer value 
 */
STDMETHODIMP
HXRegistry::GetIntRefByName(const char*     pName,
                            REF(INT32*)     pValue) const
{
    return m_pPropDB->GetIntRef(pName, &pValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::GetIntRefById
 *  Purpose:
 *      Retrieve an INTEGER REFERENCE property from the registry given
 *  its id "ulId".  If the Property is found it will return
 *  HXR_OK and pValue will be assigned the address of the integer
 *  (not the value of the integer, which can be obtained even for
 *  INTREFs via the GetIntByxxx methods.)  Otherwise, it returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  pValue - OUT - the address of the integer value 
 */
STDMETHODIMP
HXRegistry::GetIntRefById(const UINT32    id,
                          REF(INT32*)     pValue) const
{
    return m_pPropDB->GetIntRef(id, &pValue, proc);
}



/************************************************************************
 *  Method:
 *      IHXRegistry2::AddInt64
 *  Purpose:
 *      Add an INTEGER property with name in "pName" and value in
 *  "iValue" to the registry. The return value is the id to
 *  the newly added Property or ZERO if there was an error.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property that is going to be added to
 *               the registry
 *  nValue - IN - integer value of the Property that is going to be
 *                added to the registry
 */
STDMETHODIMP_(UINT32)
HXRegistry::AddInt64(const char*     pName,
                     const INT64     nValue)
{
    return m_pPropDB->AddInt64(pName, nValue, proc);
}


/************************************************************************
 *  Method:
 *      IHXRegistry2::GetInt64ByName
 *  Purpose:
 *      Retrieve a 64-bit INTEGER value from the registry given its
 *  Property name "pName". If the Property is found, it will return
 *  HXR_OK, otherwise it returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned
 */
STDMETHODIMP
HXRegistry::GetInt64ByName(const char*     pName,
                           REF(INT64)      nValue) const
{
    return m_pPropDB->GetInt64(pName, &nValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::GetInt64ById
 *  Purpose:
 *      Retrieve a 64-bit INTEGER value from the registry given its id
 *  "ulId".  If the Property is found, it will return HXR_OK, otherwise
 *  it returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  ulId - IN - unique id of the Property whose value is to be retrieved
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned
 */
STDMETHODIMP
HXRegistry::GetInt64ById(const UINT32    ulId,
                         REF(INT64)      nValue) const
{
    return m_pPropDB->GetInt64(ulId, &nValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::SetInt64ByName
 *  Purpose:
 *      Modify a Property's INTEGER value in the registry given the
 *  Property's name "pName". If the value was set, it will return HXR_OK,
 *  otherwise it returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be set
 *  nValue - IN - the new value of the Property which is going to be set
 */
STDMETHODIMP
HXRegistry::SetInt64ByName(const char*     pName,
                           const INT64     nValue)
{
    return m_pPropDB->SetInt64(pName, nValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::SetInt64ById
 *  Purpose:
 *      Modify a Property's 64-bit INTEGER value in the registry given the
 *  its id "id". If the value was set, it will return HXR_OK, otherwise
 *  it returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  ulId - IN - unique id of the Property whose value is to be set
 *  nValue - IN - the new value of the Property which is going to be set
 */
STDMETHODIMP
HXRegistry::SetInt64ById(const UINT32    id,
                         const INT64     nValue)
{
    return m_pPropDB->SetInt64(id, nValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::ModifyInt64ByName
 *  Purpose:
 *      Changes the 64-bit INTEGER value in the registry given its Property
 *  name "pName" and an amount to change it by.  Modifies the value
 *  of the integer in the registry by the amount specified by "nDelta",
 *  setting nValue equal to the value after modification.  If the
 *  Property is found, it will return HXR_OK, otherwise it
 *  returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nDelta - IN - amount to modify the named property by
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned, after modification
 */
STDMETHODIMP
HXRegistry::ModifyInt64ByName(const char*     pName,
                              INT64           nDelta,
                              REF(INT64)      nValue)
{
    return m_pPropDB->ModifyInt64(pName, nDelta, &nValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::ModifyInt64ById
 *  Purpose:
 *      Changes the 64-bit INTEGER value in the registry given its id
 *  "ulID and an amount to change it by.  Modifies the value of the
 *  integer in the registry by the amount specified by "nDelta",
 *  setting nValue equal to the value after modification.  If the
 *  Property is found, it will return HXR_OK, otherwise it
 *  returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  ulId - IN - unique id of the Property whose value is to be modified
 *  nDelta - IN - amount to modify the specified property by
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned, after modification
 */
STDMETHODIMP
HXRegistry::ModifyInt64ById(const UINT32    id,
                            INT64           nDelta,
                            REF(INT64)      nValue)
{
    return m_pPropDB->ModifyInt64(id, nDelta, &nValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::BoundedModifyInt64ByName
 *  Purpose:
 *      Changes the 64-bit INT val in the registry given its Property name
 *  "pName" and an amount to change it by and keeps the modified value 
 *  within the bounds of the nMin and nMax values. Modifies the value 
 *  of the integer in the registry by the amount specified by "nDelta",
 *  setting nValue equal to the value after modification, if the modified 
 *  value is >= nMin and <= nMax. If either of these limits are violated 
 *  the the resulting value stored in the registry is the value of the 
 *  limit just violated. If the Property is found, it will return HXR_OK, 
 *  otherwise it returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nDelta - IN - amount to modify the named property by
 *  nMin - IN - min value that the modified registry prop can have
 *              if the modified registry value < nMin then
 *                  registry value = nMin
 *  nMax - IN - min value that the modified registry prop can have
 *              if the modified registry value > nMax then
 *                  registry value = nMax
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned, after modification
 */
STDMETHODIMP
HXRegistry::BoundedModifyInt64ByName(const char*     pName,
				     INT64           nDelta,
				     REF(INT64)      nValue,
				     INT64           nMin,
				     INT64           nMax)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::BoundedModifyInt64ById
 *  Purpose:
 *      Changes the 64-bit INT val in the registry given its id "ulID"
 *  and an amount to change it by and keeps the modified value within
 *  the bounds of the nMin and nMax values. Modifies the value of the
 *  integer in the registry by the amount specified by "nDelta",
 *  setting nValue equal to the value after modification, if the modified
 *  value is >= nMin and <= nMax. If either of these limits are violated
 *  the the resulting value stored in the registry is the value of the
 *  limit just violated. If the Property is found, it will return HXR_OK, 
 *  otherwise it returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  ulId - IN - unique id of the Property whose value is to be modified
 *  nDelta - IN - amount to modify the specified property by
 *  nMin - IN - min value that the modified registry prop can have
 *              if the modified registry value < nMin then
 *                  registry value = nMin
 *  nMax - IN - min value that the modified registry prop can have
 *              if the modified registry value > nMax then
 *                  registry value = nMax
 *  nValue - OUT - parameter into which the value of the Property is
 *                 going to be returned, after modification
 *
 *  NOTE:
 *     the default values should b changed from INT_MIN/MAX to their
 *  appropriate 64-bit values
 */
STDMETHODIMP
HXRegistry::BoundedModifyInt64ById(const UINT32    id,
				   INT64           nDelta,
				   REF(INT64)      nValue,
				   INT64           nMin,
				   INT64           nMax)
{
    return HXR_NOTIMPL;
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::SetAndReturnInt64ByName
 *  Purpose:
 *      Modify a Property's 64-bit INTEGER value in the registry given
 *  the Property's name "pName". If the Property is found, the previous
 *  value, prior to setting it, will be assigned to nOldValue.
 *  If the Property is found, it will return HXR_OK, otherwise it
 *  returns HXR_FAIL.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nValue - IN - the new value of the Property which is going to be set
 *  nOldValue - OUT - parameter into which the previous value of the
 *                    Property is returned
 */
STDMETHODIMP
HXRegistry::SetAndReturnInt64ByName(const char*     pName,
                                    INT64           nValue,
                                    REF(INT64)      nOldValue) 
{
    return m_pPropDB->SetAndReturnInt64(pName, nValue, &nOldValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::SetAndReturnInt64ById
 *  Purpose:
 *      Modify a Property's 64-bit INTEGER value in the registry given
 *  the Property's id "ulId". If the id is found, the previous
 *  value, prior to setting it, will be assigned to nOldValue.
 *  If the Property is found, it will return HXR_OK, otherwise it
 *  returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  nValue - IN - the new value of the Property which is going to be set
 *  nOldValue - OUT - parameter into which the previous value of the
 *                    Property is returned
 */
STDMETHODIMP
HXRegistry::SetAndReturnInt64ById(const UINT32    id,
                                  INT64           nValue,
                                  REF(INT64)      nOldValue)
{
    return m_pPropDB->SetAndReturnInt64(id, nValue, &nOldValue, proc);
}


/************************************************************************
 *  Method:
 *      IHXRegistry2::AddInt64Ref
 *  Purpose:
 *      Add a 64-bit INTEGER REFERENCE property with name in "pName"
 *  and value in "iValue" to the registry. This property allows the user
 *  to modify its contents directly, without having to go through the
 *  registry.
 *
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property that is going to be added to
 *               the registry
 *  pValue - IN - the pointer of the integer value is what gets stored
 *                in the registry as the Integer Reference Property's
 *                value
 */
STDMETHODIMP_(UINT32)
HXRegistry::AddInt64Ref(const char*     pName,
                        INT64*          pValue)
{
    return m_pPropDB->AddInt64Ref(pName, pValue, proc);
}

/************************************************************************
 *  Method:
 *      IHXRegistry2::GetInt64RefByName
 *  Purpose:
 *      Retrieve a 64-bit INTEGER REFERENCE property from the registry
 *  given its Property name "pName".  If the Property is found it will
 *  return HXR_OK and pValue will be assigned the address of the integer
 *  (not the value of the integer, which can be obtained even for
 *  INTREFs via the GetIntByxxx methods.)  Otherwise, it returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  pValue - OUT - the address of the integer value 
 */
STDMETHODIMP
HXRegistry::GetInt64RefByName(const char*     pName,
                              REF(INT64*)     pValue) const
{
    return m_pPropDB->GetInt64Ref(pName, &pValue, proc);
}


/************************************************************************
 *  Method:
 *      IHXRegistry2::GetInt64RefById
 *  Purpose:
 *      Retrieve a 64-bit INTEGER REFERENCE property from the registry
 *  given its id "ulId".  If the Property is found it will return
 *  HXR_OK and pValue will be assigned the address of the integer
 *  (not the value of the integer, which can be obtained even for
 *  INTREFs via the GetIntByxxx methods.)  Otherwise, it returns HXR_FAIL.
 *  
 *  This operation occurs atomically, ensuring that multiple users of the
 *  registry do not interfere with each other, even on multi-CPU systems.
 *  
 *  pName - IN - name of the Property whose value is to be retrieved
 *  pValue - OUT - the address of the integer value 
 */
STDMETHODIMP
HXRegistry::GetInt64RefById(const UINT32    id,
                            REF(INT64*)     pValue) const
{
    return m_pPropDB->GetInt64Ref(id, &pValue, proc);
}


/************************************************************************
 *  Method:
 *      IHXRegistry2::GetChildIdListByName
 *  Purpose:
 *      Get a array which enumerates all of the children under a 
 *      property by id. 
 *
 *  pName - IN - name of the Property whose children are to be enumerated.
 *  pValues - OUT - array of unique Property id's.
 *  ulCount - OUT - size of the returned pValues array.
 *
 *  Note: The array must be deleted by the user.
 */
STDMETHODIMP
HXRegistry::GetChildIdListByName(const char*    pName,
                                 REF(UINT32*)   pValues,
                                 REF(UINT32)    ulCount) const
{
    return m_pPropDB->GetChildIdList(pName, pValues, ulCount, proc);
}


/************************************************************************
 *  Method:
 *      IHXRegistry2::GetChildIdListById
 *  Purpose:
 *      Get a array which enumerates all of the children under a 
 *      property by id. 
 *
 *  ulId - IN - unique id of the Property whose children are to be enumerated.
 *  pValues - OUT - array of unique Property id's.
 *  ulCount - OUT - size of the returned pValues array.
 *
 *  Note: The array must be deleted by the user.
 */
STDMETHODIMP
HXRegistry::GetChildIdListById(const UINT32     ulId,
                               REF(UINT32*)     pValues,
                               REF(UINT32)      ulCount) const
{
    return m_pPropDB->GetChildIdList(ulId, pValues, ulCount, proc);
}


/************************************************************************
 *  Method:
 *      IHXRegistry2::GetPropStatusByName
 *  Purpose:
 *      Queries registry for property status.
 *
 *  pName - IN - name of property to get child ids for.
 *
 *  Returns:
 *      HXR_OK if property exists.
 *      HXR_PROP_DELETE_PENDING if property exists, but is delete-pending.
 *      HXR_FAIL if property doesn't exist.
 */

STDMETHODIMP HXRegistry::GetPropStatusByName(const char* pName) const
{
    return m_pPropDB->GetPropStatus(pName, proc);
}


/************************************************************************
 *  Method:
 *      IHXRegistry2::GetPropStatusById
 *  Purpose:
 *      Queries registry for property status.
 *
 *  ulId - IN - id of property to get child ids for.
 *
 *  Returns:
 *      HXR_OK if property exists.
 *      HXR_PROP_DELETE_PENDING if property exists, but is delete-pending.
 *      HXR_FAIL if property doesn't exist.
 */

STDMETHODIMP HXRegistry::GetPropStatusById(const UINT32 ulId) const
{
    return m_pPropDB->GetPropStatus(ulId, proc);
}
