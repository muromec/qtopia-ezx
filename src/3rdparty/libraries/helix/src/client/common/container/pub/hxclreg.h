/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxclreg.h,v 1.5 2007/07/06 21:57:57 jfinnecy Exp $
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

#ifndef _HXCLIENTREG_H_
#define _HXCLIENTREG_H_

#include "hxmon.h"
class  CommonRegistry;

class HXClientRegistry : public IHXRegistry
{
protected:
    virtual ~HXClientRegistry();


    LONG32		m_lRefCount;
    CommonRegistry*	m_pPropDB;
    IUnknown*		m_pContext;

public:

    HXClientRegistry();

    void Init    (IUnknown* pContext);
    void Close   (void);

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
     *      Create a new IUnknown object which can then be queried for the
     *  right kind of IHXPropWatch object.
     */
    STDMETHOD(CreatePropWatch)		(THIS_
					REF(IHXPropWatch*)	pPropWatch);

    /************************************************************************
     *  Method:
     *      HXRegistry::AddComp
     *  Purpose:
     *      Add a COMPOSITE property to the registry and return its hash
     *  key value if successful. It returns ZERO (0) if an error occured
     *  during the operation.
     */
    STDMETHOD_(UINT32, AddComp)		(THIS_
					const char*	pName);

    /************************************************************************
     *  Method:
     *      HXRegistry::AddInt
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
     *      HXRegistry::GetInt
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
     *      HXRegistry::SetIntByXYZ
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
     *      HXRegistry::AddStr
     *  Purpose:
     *      Add an STRING property with name in "pName" and value in 
     *  "pcValue" to the registry.
     */
    STDMETHOD_(UINT32, AddStr)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue);

    /************************************************************************
     *  Method:
     *      HXRegistry::GetStrByXYZ
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
     *      HXRegistry::SetStrByXYZ
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
     *      HXRegistry::AddBuf
     *  Purpose:
     *      Add an BUFFER property with name in "pName" and value in 
     *  "pValue" to the registry.
     */
    STDMETHOD_(UINT32, AddBuf)		(THIS_
					const char*	pName, 
					IHXBuffer*	pValue);

    /************************************************************************
     *  Method:
     *      HXRegistry::GetStrByXYZ
     *  Purpose:
     *      Retreive the BUFFER from the registry given its Property
     *  name "pName" or its id "id". If the Property 
     *  is found, it will return HXR_OK, otherwise it returns HXR_FAIL.
     */
    STDMETHOD(GetBufByName)		(THIS_
					const char*	pName,
					REF(IHXBuffer*)	ppValue) const;
    STDMETHOD(GetBufById)		(THIS_
					const UINT32	id,
					REF(IHXBuffer*)	ppValue) const;

    /************************************************************************
     *  Method:
     *      HXRegistry::SetBufByXYZ
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
     *      HXRegistry::AddIntRef
     *  Purpose:
     *      Add an INTEGER REFERENCE property with name in "pName" and 
     *  value in "iValue" to the registry. This property allows the user
     *  to modify its contents directly, without having to go through the
     *  registry.
     */
    STDMETHOD_(UINT32, AddIntRef)	(THIS_
					const char*	pName, 
					INT32*	 	piValue);

    /************************************************************************
     *  Method:
     *      HXRegistry::DeleteByXYZ
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
     *      HXRegistry::GetTypeByXYZ
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
     *      HXRegistry::FindParentIdByXYZ
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
     *      HXRegistry::GetPropName
     *  Purpose:
     *      Returns the Property name in the pName char buffer passed
     *  as a parameter, given the Property's id "ulId".
     */
    STDMETHOD(GetPropName)			(THIS_
						const UINT32 ulId,
						REF(IHXBuffer*) pName) const;

    /************************************************************************
     *  Method:
     *      HXRegistry::GetId
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
     */
    STDMETHOD(GetPropListOfRoot) 	(THIS_
					REF(IHXValues*) pValues) const;

    /************************************************************************
     *  Method:
     *      IHXRegistry::GetPropListByXYZ
     *  Purpose:
     *      Returns an array of Properties immediately under the one with
     *  name "pName" or id "id".
     */
    STDMETHOD(GetPropListByName) 	(THIS_
					 const char* pName,
					 REF(IHXValues*) pValues) const;
    STDMETHOD(GetPropListById) 	 	(THIS_
					 const UINT32 id,
					 REF(IHXValues*) pValues) const;

    /************************************************************************
     *  Method:
     *      HXRegistry::GetNumProps
     *  Purpose:
     *      Returns the count of the number of Properties within the
     *  registry. If a property name of id is specified, then it
     *  returns the number of Properties under it.
     */
    STDMETHOD_(INT32, GetNumPropsAtRoot)	(THIS) const;
    STDMETHOD_(INT32, GetNumPropsByName)(THIS_
					const char*	pName) const;
    STDMETHOD_(INT32, GetNumPropsById)	(THIS_
					const UINT32	id) const;
};

#endif /*_HXCLIENTREG_H_*/
