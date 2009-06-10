/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxreg.h,v 1.4 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _HXREG_H_
#define _HXREG_H_

#include <limits.h>

#include "hxtypes.h"
#include "hxcom.h"

#include "simple_callback.h"

#include "regdb_misc.h"
#include "hxmon.h"

class ServerRegistry;
class Process;


class HXRegistry : public IHXRegistry2,
                   public IHXRegistry,
		   public IHXActiveRegistry,
		   public IHXCopyRegistry,
		   public IHXRegistryAltStringHandling
{
protected:
				virtual ~HXRegistry();


    ULONG32				m_ulRefCount;
    ServerRegistry*			m_pPropDB;
    Process*				proc;
    int					procnum;


public:
    /*
     * HXServerRegistry specific methods. to be used by the instantiator of
     * this class. NOT VISIBLE to the Plugin.
     */
    HXRegistry		(ServerRegistry*	db = 0,
			Process*		proc = 0);
    STDMETHOD(Init)	(THIS_
			ServerRegistry*		db,
			Process*		proc);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXRegistry methods
     */

    /************************************************************************
     *  Method:
     *      IHXRegistry::CreatePropWatch
     *  Purpose:
     *      Create a new IHXPropWatch object which can then be queried for 
     *  the right kind of IHXPropWatch object.
     */
    STDMETHOD(CreatePropWatch)		(THIS_
					REF(IHXPropWatch*) pPropWatch);

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddComp
     *  Purpose:
     *      Add a COMPOSITE property to the registry and return its id
     *  if successful. It returns ZERO (0) if an error occured during
     *  the operation.
     */
    STDMETHOD_(UINT32, AddComp)		(THIS_
					const char*	pName);

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddInt
     *  Purpose:
     *      Add an INTEGER property with name in "pName" and value in 
     *  "iValue" to the registry. The return value is the id to
     *  the newly added Property or ZERO if there was an error.
     */
    STDMETHOD_(UINT32, AddInt)		(THIS_
					const char*	pName, 
					const INT32	iValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetInt
     *  Purpose:
     *      Retreive an INTEGER value from the registry given its Property
     *  name "pName" or by its id "id". If the Property 
     *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
     */
    STDMETHOD(GetIntByName)		(THIS_
					const char*	pName,
					REF(INT32)	nValue) const;
    STDMETHOD(GetIntById)		(THIS_
					const UINT32	id,
					REF(INT32)	nValue) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetIntByXYZ
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  Property's name "pName" or its id "id". If the value 
     *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
     */
    STDMETHOD(SetIntByName)		(THIS_
					const char*	pName, 
					const INT32	iValue);
    STDMETHOD(SetIntById)		(THIS_
					const UINT32	id,
					const INT32	iValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddStr
     *  Purpose:
     *      Add an STRING property with name in "pName" and value in 
     *  "pValue" to the registry and return its ID if successful.
     *  It returns ZERO (0) if an error occurred during the operation.

     */
    STDMETHOD_(UINT32, AddStr)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetStrByXYZ
     *  Purpose:
     *      Retreive an STRING value from the registry given its Property
     *  name "pName" or by its id "id". If the Property 
     *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
     */
    STDMETHOD(GetStrByName)		(THIS_
					const char*	    pName,
					REF(IHXBuffer*)    pValue) const;
    STDMETHOD(GetStrById)		(THIS_
					const UINT32	    id,
					REF(IHXBuffer*)    pValue) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetStrByXYZ
     *  Purpose:
     *      Modify a Property's STRING value in the registry given the
     *  Property's name "pName" or its id "id". If the value 
     *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
     */
    STDMETHOD(SetStrByName)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue);
    STDMETHOD(SetStrById)		(THIS_
					const UINT32	id,
					IHXBuffer*	pValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddBuf
     *  Purpose:
     *      Add an BUFFER property with name in "pName" and value in 
     *  "pValue" to the registry and return its ID if successful.
     *  It returns ZERO (0) if an error occurred during the operation.

     */
    STDMETHOD_(UINT32, AddBuf)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetStrByXYZ
     *  Purpose:
     *      Retreive the BUFFER from the registry given its Property
     *  name "pName" or its id "id". If the Property 
     *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
     */
    STDMETHOD(GetBufByName)		(THIS_
					const char*	pName,
					REF(IHXBuffer*) ppValue) const;
    STDMETHOD(GetBufById)		(THIS_
					const UINT32	id,
					REF(IHXBuffer*) ppValue) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::SetBufByXYZ
     *  Purpose:
     *      Modify a Property's BUFFER in the registry given the
     *  Property's name "pName" or its id "id". If the value 
     *  was set, it will return HXR_OK, otherwise it returns HXR_FAIL.
     */
    STDMETHOD(SetBufByName)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue);
    STDMETHOD(SetBufById)		(THIS_
					const UINT32	id,
					IHXBuffer*	pValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry::AddIntRef
     *  Purpose:
     *      Add an INTEGER REFERENCE property with name in "pName" and 
     *  value in "iValue" to the registry. This property allows the user
     *  to modify its contents directly, without having to go through the
     *  registry.  The Property's id is returned if successful.
     *  It returns ZERO (0) if an error occurred during the operation.
     */
    STDMETHOD_(UINT32, AddIntRef)	(THIS_
					const char*	pName, 
					INT32*	piValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry::DeleteByXYZ
     *  Purpose:
     *      Delete a Property from the registry using its name "pName"
     *  or id "id".
     */
    STDMETHOD_(UINT32, DeleteByName)	(THIS_
					const char*	pName);
    STDMETHOD_(UINT32, DeleteById)	(THIS_
					const UINT32	id);

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetTypeByXYZ
     *  Purpose:
     *      Returns the datatype of the Property given its name "pName"
     *  or its id "id".
     */
    STDMETHOD_(HXPropType, GetTypeByName)	(THIS_
						const char* pName) const;
    STDMETHOD_(HXPropType, GetTypeById)	(THIS_
						const UINT32 id) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::FindParentIdByXYZ
     *  Purpose:
     *      Returns the id value of the parent node if it exists.
     *  If it fails, a ZERO value is returned.
     */
    STDMETHOD_(UINT32, FindParentIdByName)	(THIS_
						const char* pName) const;
    STDMETHOD_(UINT32, FindParentIdById)	(THIS_
						const UINT32 id) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetPropName
     *  Purpose:
     *      Returns the Property name in the pName char buffer passed
     *  as a parameter, given the Property's id "ulId".
     */
    STDMETHOD(GetPropName)			(THIS_
						const UINT32 ulId,
						REF(IHXBuffer*) pName) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetId
     *  Purpose:
     *      Returns the Property's id given the Property name.
     */
    STDMETHOD_(UINT32, GetId)			(THIS_
						const char* pName) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetPropListOfRoot
     *  Purpose:
     *      Returns an array of a Properties under the root level of the 
     *  registry's hierarchy.
     *      The array is an argv style array of pointers to the 
     *  HXPropList structures, with the last element of the array being
     *  a ZERO pointer.
     */
    STDMETHOD(GetPropListOfRoot) 	(THIS_
					REF(IHXValues*) pValues) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetPropListByXYZ
     *  Purpose:
     *      Returns an array of Properties immediately under the one with
     *  name "pName" or id "id".
     *      The array is an argv style array of pointers to the 
     *  HXPropList structures, with the last element of the array being
     *  a ZERO pointer.
     */
    STDMETHOD(GetPropListByName) 	(THIS_
					 const char* pName,
					 REF(IHXValues*) pValues) const;
    STDMETHOD(GetPropListById) 	 	(THIS_
					 const UINT32 id,
					 REF(IHXValues*) pValues) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetNumPropsAtRoot
     *  Purpose:
     *      Returns the number of Properties at the root of the registry. 
     */
    STDMETHOD_(INT32, GetNumPropsAtRoot)(THIS) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetNumPropsByXYZ
     *  Purpose:
     *      Returns the count of the number of Properties within the
     *  registry. If a property name of id is specified, then it
     *  returns the number of Properties under it.
     */
    STDMETHOD_(INT32, GetNumPropsByName)(THIS_
					const char*	pName) const;

    STDMETHOD_(INT32, GetNumPropsById)	(THIS_
					const UINT32	id) const;

    /*
     * IHXActiveRegistry methods.
     */

    /************************************************************************
    * IHXActiveRegistry::SetAsActive
    *
    *     Method to set prop pName to active and register pUser as
    *   the active prop user.
    */
    STDMETHOD(SetAsActive)    (THIS_
				const char* pName,
				IHXActivePropUser* pUser);

    /************************************************************************
    * IHXActiveRegistry::SetAsInactive
    *
    *	Method to remove an IHXActiveUser from Prop activation.
    */
    STDMETHOD(SetAsInactive)  (THIS_
			    const char* pName,
			    IHXActivePropUser* pUser);

    /************************************************************************
    * IHXActiveRegistry::IsActive
    *
    *     Tells if prop pName has an active user that must be queried to
    *   change the value, or if it can just be set.
    */
    STDMETHOD_(BOOL, IsActive)	(THIS_
				const char* pName);

    /************************************************************************
    * IHXActiveRegistry::SetActiveInt
    *
    *    Async request to set int pName to ul.
    */
    STDMETHOD(SetActiveInt) (THIS_
			    const char* pName,
			    UINT32 ul,
			    IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActiveRegistry::SetActiveStr
    *
    *    Async request to set string pName to string in pBuffer.
    */
    STDMETHOD(SetActiveStr) (THIS_
			    const char* pName,
			    IHXBuffer* pBuffer,
			    IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActiveRegistry::SetActiveBuffer
    *
    *    Async request to set buffer pName to buffer in pBuffer.
    */
    STDMETHOD(SetActiveBuf)	(THIS_
				const char* pName,
				IHXBuffer* pBuffer,
				IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActivePropUser::DeleteActiveProp
    *
    *	Async request to delete the active property.
    */
    STDMETHOD(DeleteActiveProp)	(THIS_
				const char* pName,
				IHXActivePropUserResponse* pResponse);


    /* 
     * IHXCopyRegistry
     */
    /************************************************************************
    * IHXCopyRegistry::Copy
    *
    *   Here it is! The "Copy" method!
    */
    STDMETHOD (CopyByName)  (THIS_
			    const char* pFrom,
			    const char* pTo);

    STDMETHOD (SetStringAccessAsBufferById)  (THIS_ UINT32 ulId);



    /*
     * IHXRegistry2
     */

    /************************************************************************
     *  Method:
     *      IHXRegistry2::ModifyIntByName
     *  Purpose:
     *      Changes the INTEGER value in the registry given its Property
     *  name "pName" and an amount to change it by.  Modifies the value
     *  of the integer in the registry by the amount specified by "nDelta",
     *  setting nValue equal to the value after modification.  If the
     *  Property is found, it will return HXR_OK, otherwise it
     *  returns HXR_FAIL.
     *  
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nDelta - IN - amount to modify the named property by
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(ModifyIntByName)         (THIS_
                                        const char*     pName,
					INT32           nDelta,
                                        REF(INT32)      nValue);

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
     *  ulId - IN - unique id of the Property whose value is to be modified
     *  nDelta - IN - amount to modify the specified property by
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(ModifyIntById)           (THIS_
                                        const UINT32    id,
					INT32           nDelta,
                                        REF(INT32)      nValue);

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
    STDMETHOD(BoundedModifyIntByName)   (THIS_
                                        const char*     pName,
					INT32           nDelta,
                                        REF(INT32)      nValue,
					INT32           nMin=INT_MIN,
					INT32           nMax=INT_MAX);

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
    STDMETHOD(BoundedModifyIntById)     (THIS_
                                        const UINT32    id,
					INT32           nDelta,
                                        REF(INT32)      nValue,
					INT32           nMin=INT_MIN,
					INT32           nMax=INT_MAX);

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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - IN - the new value of the Property which is going to be set
     *  nOldValue - OUT - parameter into which the previous value of the
     *                    Property is returned
     */
    STDMETHOD(SetAndReturnIntByName)   (THIS_
                                        const char*     pName,
					INT32           nValue,
                                        REF(INT32)      nOldValue);

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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - IN - the new value of the Property which is going to be set
     *  nOldValue - OUT - parameter into which the previous value of the
     *                    Property is returned
     */
    STDMETHOD(SetAndReturnIntById)     (THIS_
                                        const UINT32    id,
					INT32           nValue,
                                        REF(INT32)      nOldValue);

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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - the address of the integer value 
     */
    STDMETHOD(GetIntRefByName)         (THIS_
                                        const char*     pName,
                                        REF(INT32*)     pValue) const;

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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - the address of the integer value 
     */
    STDMETHOD(GetIntRefById)           (THIS_
                                        const UINT32    id,
                                        REF(INT32*)     pValue) const;



    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddInt64
     *  Purpose:
     *      Add an INTEGER property with name in "pName" and value in
     *  "iValue" to the registry. The return value is the id to
     *  the newly added Property or ZERO if there was an error.
     *
     *  pName - IN - name of the Property that is going to be added to
     *               the registry
     *  nValue - IN - integer value of the Property that is going to be
     *                added to the registry
     */
    STDMETHOD_(UINT32, AddInt64)       (THIS_
                                        const char*     pName,
                                        const INT64     nValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetInt64ByName
     *  Purpose:
     *      Retrieve a 64-bit INTEGER value from the registry given its
     *  Property name "pName". If the Property is found, it will return
     *  HXR_OK, otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned
     */
    STDMETHOD(GetInt64ByName)          (THIS_
                                        const char*     pName,
                                        REF(INT64)      nValue) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetInt64ById
     *  Purpose:
     *      Retrieve a 64-bit INTEGER value from the registry given its id
     *  "ulId".  If the Property is found, it will return HXR_OK, otherwise
     *  it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be retrieved
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned
     */
    STDMETHOD(GetInt64ById)            (THIS_
                                        const UINT32    ulId,
                                        REF(INT64)      nValue) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetInt64ByName
     *  Purpose:
     *      Modify a Property's INTEGER value in the registry given the
     *  Property's name "pName". If the value was set, it will return HXR_OK,
     *  otherwise it returns HXR_FAIL.
     *
     *  pName - IN - name of the Property whose value is to be set
     *  nValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetInt64ByName)          (THIS_
                                        const char*     pName,
                                        const INT64     nValue);

    /************************************************************************
     *  Method:
     *      IHXRegistry2::SetInt64ById
     *  Purpose:
     *      Modify a Property's 64-bit INTEGER value in the registry given the
     *  its id "id". If the value was set, it will return HXR_OK, otherwise
     *  it returns HXR_FAIL.
     *
     *  ulId - IN - unique id of the Property whose value is to be set
     *  nValue - IN - the new value of the Property which is going to be set
     */
    STDMETHOD(SetInt64ById)            (THIS_
                                        const UINT32    id,
                                        const INT64     nValue);

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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nDelta - IN - amount to modify the named property by
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(ModifyInt64ByName)       (THIS_
                                        const char*     pName,
					INT64           nDelta,
                                        REF(INT64)      nValue);

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
     *  ulId - IN - unique id of the Property whose value is to be modified
     *  nDelta - IN - amount to modify the specified property by
     *  nValue - OUT - parameter into which the value of the Property is
     *                 going to be returned, after modification
     */
    STDMETHOD(ModifyInt64ById)         (THIS_
                                        const UINT32    id,
					INT64           nDelta,
                                        REF(INT64)      nValue);

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
    STDMETHOD(BoundedModifyInt64ByName) (THIS_
                                        const char*     pName,
					INT64           nDelta,
                                        REF(INT64)      nValue,
					INT64           nMin=INT_MIN,
					INT64           nMax=INT_MAX);

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
    STDMETHOD(BoundedModifyInt64ById)   (THIS_
                                        const UINT32    id,
					INT64           nDelta,
                                        REF(INT64)      nValue,
					INT64           nMin=INT_MIN,
					INT64           nMax=INT_MAX);

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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - IN - the new value of the Property which is going to be set
     *  nOldValue - OUT - parameter into which the previous value of the
     *                    Property is returned
     */
    STDMETHOD(SetAndReturnInt64ByName) (THIS_
                                        const char*     pName,
					INT64           nValue,
                                        REF(INT64)      nOldValue);

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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  nValue - IN - the new value of the Property which is going to be set
     *  nOldValue - OUT - parameter into which the previous value of the
     *                    Property is returned
     */
    STDMETHOD(SetAndReturnInt64ById)   (THIS_
                                        const UINT32    id,
					INT64           nValue,
                                        REF(INT64)      nOldValue);


    /************************************************************************
     *  Method:
     *      IHXRegistry2::AddInt64Ref
     *  Purpose:
     *      Add a 64-bit INTEGER REFERENCE property with name in "pName"
     *  and value in "iValue" to the registry. This property allows the user
     *  to modify its contents directly, without having to go through the
     *  registry.
     *
     *  pName - IN - name of the Property that is going to be added to
     *               the registry
     *  pValue - IN - the pointer of the integer value is what gets stored
     *                in the registry as the Integer Reference Property's
     *                value
     */
    STDMETHOD_(UINT32, AddInt64Ref)     (THIS_
                                        const char*     pName,
                                        INT64*          pValue);

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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - the address of the integer value 
     */
    STDMETHOD(GetInt64RefByName)       (THIS_
                                        const char*     pName,
                                        REF(INT64*)     pValue) const;


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
     *  pName - IN - name of the Property whose value is to be retrieved
     *  pValue - OUT - the address of the integer value 
     */
    STDMETHOD(GetInt64RefById)         (THIS_
                                        const UINT32    id,
                                        REF(INT64*)     pValue) const;

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
    STDMETHOD(GetChildIdListByName) (THIS_
                                     const char*        pName,
                                     REF(UINT32*)       pValues,
                                     REF(UINT32)        ulCount) const;


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
    STDMETHOD(GetChildIdListById)       (THIS_
				         const UINT32   ulId,
				         REF(UINT32*)   pValues,
				         REF(UINT32)    ulCount) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetPropStatusById
     *  Purpose:
     *      Gets status of a property by id.
     *
     *  ulId - IN - id of property to get child ids for.
     *
     *  Returns:
     *    HXR_OK if property exists.
     *    HXR_PROP_DELETE_PENDING if property exists, but is delete-pending.
     *    HXR_FAIL if property does not exist.
     */

    STDMETHOD(GetPropStatusById) (THIS_
				  const UINT32 ulId) const;


    /************************************************************************
     *  Method:
     *      IHXRegistry2::GetPropStatusByName
     *  Purpose:
     *      Gets status of a property by name.
     *
     *  szPropName - IN - name of property to get child ids for.
     *
     *  Returns:
     *    HXR_OK if property exists.
     *    HXR_PROP_DELETE_PENDING if property exists, but is delete-pending.
     *    HXR_FAIL if property does not exist.
     */

    STDMETHOD(GetPropStatusByName) (THIS_
				    const char* pName) const;
};

#endif // _HXREG_H_
