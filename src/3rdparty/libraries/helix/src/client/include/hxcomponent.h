/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxcomponent.h,v 1.2 2007/07/06 21:58:18 jfinnecy Exp $
* 
* Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _HXCOMPONENT_H_
#define _HXCOMPONENT_H_
#include "unkimp.h"

struct IHXPackage_
{
    virtual void OnComponentBirth () = 0;
    virtual void OnComponentDeath () = 0;
};

// XXXHP - note this is unsafe, we shouldn't be defining COM create funcs on 
// managed components, this is here for the sole purpose of easing migration.
#define DECLARE_MANAGED_COMPONENT(ComponentName_)   \
	DECLARE_UNKNOWN_NOCREATE (ComponentName_)   \
	DECLARE_COM_CREATE_FUNCS (ComponentName_)

#define DECLARE_UNMANAGED_COMPONENT(ComponentName_) 			\
	DECLARE_UNKNOWN_NOCREATE (ComponentName_) 				\
	static ComponentName_* CreateUnmanagedObject ();					\
	static HX_RESULT CreateUnmanagedObject(ComponentName_** ppObj);	\
	static HX_RESULT CreateUnmanagedInstance(IUnknown** ppvObj);			\
	static HX_RESULT CreateUnmanagedInstance(IUnknown* pvOuter, IUnknown** ppvObj);

#define DECLARE_UNMANAGED_COMPONENT_NOCREATE(ComponentName_) \
	DECLARE_UNKNOWN_NOCREATE (ComponentName_) 
	
#define DECLARE_ABSTRACT_COMPONENT(ComponentName_) \
	DECLARE_UNKNOWN_NOCREATE (ComponentName_)

#define IMPLEMENT_MANAGED_COMPONENT(ComponentName_) \
	IMPLEMENT_COM_CREATE_FUNCS (ComponentName_)

#define IMPLEMENT_UNMANAGED_COMPONENT(ComponentName_) 			   \
   HX_RESULT ComponentName_::CreateUnmanagedObject(ComponentName_** ppObj) \
    {								\
	*ppObj = new ComponentName_;				\
	if (*ppObj)						\
	{							\
	    InterlockedIncrement(&((*ppObj)->m_lCount));	\
	    HX_RESULT pnrRes = (*ppObj)->FinalConstruct();	\
	    InterlockedDecrement(&((*ppObj)->m_lCount));	\
	    if (FAILED(pnrRes))					\
	    {							\
		delete (*ppObj);				\
		(*ppObj) = NULL;				\
		return pnrRes;					\
	    }							\
	    return HXR_OK;					\
	}							\
	return HXR_OUTOFMEMORY;		   			\
    }								\
    ComponentName_* ComponentName_::CreateUnmanagedObject()	\
    {								\
	ComponentName_* pNew = NULL;				\
	if (SUCCEEDED(CreateUnmanagedObject(&pNew)))		\
	{							\
	    return pNew;					\
	}							\
	return NULL;						\
    }								\
    HX_RESULT ComponentName_::CreateUnmanagedInstance		\
    (								\
	IUnknown* pvOuterObj,					\
	IUnknown** ppvObj					\
    )								\
    {								\
	if (!ppvObj)						\
	    return HXR_POINTER;					\
	*ppvObj = NULL;						\
	ComponentName_* pNew = NULL;				\
	HX_RESULT pnrRes = CreateUnmanagedObject(&pNew);	\
	if (SUCCEEDED(pnrRes) && pNew)				\
	{							\
	    pnrRes = pNew->SetupAggregation( pvOuterObj, ppvObj ); \
	}							\
	return pnrRes;						\
    }								\
    HX_RESULT ComponentName_::CreateUnmanagedInstance(IUnknown** ppvObj) \
    {								\
	return CreateUnmanagedInstance(NULL, ppvObj);		\
    }								

#define IMPLEMENT_UNMANAGED_COMPONENT_NOCREATE(ComponentName_) 

#define IMPLEMENT_ABSTRACT_COMPONENT(ComponentName_)

#define BEGIN_COMPONENT_INTERFACE_LIST(ComponentName_) \
	BEGIN_INTERFACE_LIST_NOCREATE (ComponentName_)

#define HXCOMPONENT_MANAGED_COMPONENT_MANGLE_(ComponentName_) \
	CHXManagedComponent_##ComponentName_##_

#ifdef HX_REGISTER_ALL_COMPONENTS_
	#define REGISTER_MANAGED_COMPONENT(ComponentName_)		\
	class HXCOMPONENT_MANAGED_COMPONENT_MANGLE_(ComponentName_) : public ComponentName_		\
	{															\
	    typedef HXCOMPONENT_MANAGED_COMPONENT_MANGLE_(ComponentName_) CSelf;			\
	    IHXPackage_& m_package;						\
	public:														\
	    HXCOMPONENT_MANAGED_COMPONENT_MANGLE_(ComponentName_) (IHXPackage_& package) : m_package (package) { m_package.OnComponentBirth (); }	\
	    virtual ~HXCOMPONENT_MANAGED_COMPONENT_MANGLE_(ComponentName_) () { m_package.OnComponentDeath (); }			\
	    static HX_RESULT CreateManagedInstance (IHXPackage_& package, IUnknown* pIUnk, IUnknown** ppIUnk)	\
	    {															\
	        if (!ppIUnk) return HXR_POINTER;							\
	        *ppIUnk = NULL;											\
	        CSelf* pNew = new CSelf (package);						\
	        if (!pNew) return HXR_OUTOFMEMORY;						\
																\
	        InterlockedIncrement (&(pNew->m_lCount));				\
	        HX_RESULT res = pNew->FinalConstruct ();					\
	        InterlockedDecrement (&(pNew->m_lCount));				\
	        if (FAILED (res))											\
	        {														\
	           delete pNew;											\
	           return res;											\
	        }														\
	        return pNew->SetupAggregation (pIUnk, ppIUnk);				\
	    }															\
	};
#else
	#define REGISTER_MANAGED_COMPONENT(ComponentName_) 
#endif

#endif // _HXCOMPONENT_H_
