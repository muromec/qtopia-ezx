/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: smartptr.h,v 1.7 2007/07/06 20:39:23 jfinnecy Exp $
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

//
//
//	Macro implementation of "Smart Pointers" to COM interfaces
//
//  Description:
//	
//	A SmartPointer is a class that contains a pointer, but acts like it 
//	*is* the contained pointer.  When used it helps enforce correct usage 
//	of the pointer it contains.  In this case it provides the following 
//	safeguards:
//	    HX_ASSERT if the contained pointer is NULL when you attempt to 
//		use it.
//	    HX_ASSERT if you attempt to replace a valid pointer without 
//		releasing it.
//	    Automatically AddRef() a pointer during assignment.
//	    Automatically QI() an incoming pointer to the correct type 
//		during assignment.
//	    Automatically Release() an existing pointer during assignment.
//	    Automatically Release() an existing pointer during destruction.
//
//	SmartPointers also simplify usage. Example:
//
//	    ReadDone(HX_RESULT status, IUnknown* pbufData)
//	    {
//		DECLARE_SMART_POINTER(IHXBuffer) spbufData;
//		
//		spbufData = pbufData;
//
//		if(spbufData.IsValid())
//		{
//		    // Use the Pointer
//		    cout << spbufData->GetBuffer() << endl;
//		}
//	    }
//	This example has no memory leaks.  In the Assignment the IUnknown 
//	pointer is QI'd for the IHXBuffer.  If the QI had failed, then 
//	the IsValid test would fail too.  Even if the IsValid Test was 
//	forgotten, spbufData->GetBuffer() would cause an assertion if 
//	the QI had failed.
//
//  Usage Tips:
//	
//	Should Not use SmartPointers as function parameters. ie:
//	    ReadDone(HX_RESULT status, DECLARE_SMART_POINTER_UNKNOWN spbufData)
//	Cannot use SmartPointers as List elements.
//
//  Macros Defined in this file:
//
//	DECLARE_SMART_POINTER_UNKNOWN
//	    This Macro is used to create a SmartPointer to an IUnknown 
//	    interface.
//		Example:    DECLARE_SMART_POINTER_UNKNOWN  m_spunkContext;
//
//	DEFINE_SMART_POINTER(IACTUAL)
//	    This Macro is used to define a SmartPointer class for any 
//	    interface other than IUnknown.  It should be used in the header 
//	    file where the interface is defined.
//		Example:    DEFINE_SMART_POINTER(IHXBuffer);
//
//	IMPLEMENT_SMART_POINTER(IACTUAL)
//	    This Macro is used to Implement a SmartPointer class for any 
//	    interface other than IUnknown.  It should be used in a cpp 
//	    file that is linked into each binary.
//		Example:    IMPLEMENT_SMART_POINTER(IHXBuffer)
//
//	DECLARE_SMART_POINTER(IACTUAL)
//	    This Macro is used to create a SmartPointer to any interface other 
//	    than IUnknown.  The header file that contains the call to
//	    IMPLEMENT_SMART_POINTER(IACTUAL) must be included before this macro
//	    can be used.
//		Example:    DECLARE_SMART_POINTER(IHXBuffer)  m_spbufData;
//
//


#ifndef _SMARTPTR_H_
#define _SMARTPTR_H_

#include "hxassert.h"

class _CIUnknown_SP
{
public:
    _CIUnknown_SP() : m_pUnk(NULL)
    {}
    _CIUnknown_SP(_CIUnknown_SP& rspunk) : m_pUnk(NULL)
    {*this = rspunk.m_pUnk;}
    _CIUnknown_SP(IUnknown* punk) : m_pUnk(NULL)
    {*this = punk;}
    ~_CIUnknown_SP()
    {Release();}

    operator IUnknown* ()
    {return m_pUnk;}
    operator const IUnknown* () const
    {return m_pUnk;}

    IUnknown* operator -> () const
    {HX_ASSERT(m_pUnk);return m_pUnk;}
    IUnknown& operator * () const
    {HX_ASSERT(m_pUnk);return *m_pUnk;}
    IUnknown** operator & ()
    {HX_ASSERT(!m_pUnk);return &m_pUnk;}

    _CIUnknown_SP& operator = (IUnknown* punkNew)
    {
	Release();
	if(punkNew)
	{
	    m_pUnk = punkNew;
	    m_pUnk->AddRef();
	}
	return *this;
    }

    HXBOOL operator ! () const
    {return !IsValid();}
    HXBOOL IsValid() const
    {return (m_pUnk!=NULL);}

    IUnknown*& ptr_reference()
    {HX_ASSERT(!m_pUnk);return m_pUnk;}

    IUnknown* AsUnknown() const
    {
	HX_ASSERT(m_pUnk);
	IUnknown* punkOut = m_pUnk;
	punkOut->AddRef();
	return punkOut;		
    }
    void Release()
    {HX_RELEASE(m_pUnk);}
private:
    IUnknown*	    m_pUnk;
};

#define DECLARE_SMART_POINTER_UNKNOWN \
    _CIUnknown_SP

#define DEFINE_SMART_POINTER(IACTUAL)			\
class _C##IACTUAL##_SP					\
{							\
public:							\
    _C##IACTUAL##_SP();					\
    _C##IACTUAL##_SP(_C##IACTUAL##_SP& rspact);		\
    _C##IACTUAL##_SP(IACTUAL* pact);			\
    _C##IACTUAL##_SP(IUnknown* punk);			\
    ~_C##IACTUAL##_SP();				\
							\
    operator IACTUAL* ();				\
    operator const IACTUAL* () const;			\
							\
    IACTUAL* operator -> () const;				\
    IACTUAL& operator * () const;				\
    IACTUAL** operator & ();				\
							\
    _C##IACTUAL##_SP& operator = (IACTUAL* pNew);	\
    _C##IACTUAL##_SP& operator = (IUnknown* punkNew);	\
							\
    HXBOOL operator ! () const;					\
    HXBOOL IsValid() const;					\
							\
    IACTUAL*& ptr_reference();				\
    IUnknown* AsUnknown() const;				\
    void Release();					\
private:						\
    IACTUAL*	    pActual;				\
							\
};


#define IMPLEMENT_SMART_POINTER(IACTUAL)		\
    DEFINE_SMART_POINTER(IACTUAL)			\
							\
    _C##IACTUAL##_SP::_C##IACTUAL##_SP()		\
	: pActual(NULL)					\
    {}							\
							\
    _C##IACTUAL##_SP::_C##IACTUAL##_SP			\
    (							\
	_C##IACTUAL##_SP& rspact			\
    )							\
	: pActual(NULL)					\
    {*this = rspact.pActual;}				\
							\
    _C##IACTUAL##_SP::_C##IACTUAL##_SP(IACTUAL* pact)	\
	: pActual(NULL)					\
    {*this = pact;}					\
							\
    _C##IACTUAL##_SP::_C##IACTUAL##_SP(IUnknown* punk)	\
	: pActual(NULL)					\
    {*this = punk;}					\
							\
    _C##IACTUAL##_SP::~_C##IACTUAL##_SP()		\
    {Release();}					\
							\
    _C##IACTUAL##_SP::operator IACTUAL* ()		\
    {return pActual;}					\
							\
    _C##IACTUAL##_SP::operator const IACTUAL* () const	\
    {return pActual;}					\
							\
    IACTUAL* _C##IACTUAL##_SP::operator -> () const	\
    {HX_ASSERT(pActual);return pActual;}		\
							\
    IACTUAL& _C##IACTUAL##_SP::operator * () const	\
    {HX_ASSERT(pActual);return *pActual;}		\
							\
    IACTUAL** _C##IACTUAL##_SP::operator & ()		\
    {HX_ASSERT(!pActual);return &pActual;}		\
							\
    _C##IACTUAL##_SP& _C##IACTUAL##_SP::operator =	\
    (							\
	IACTUAL* pNew					\
    )							\
    {							\
	Release();					\
	if(pNew)					\
	{						\
	    pActual = pNew;				\
	    pActual->AddRef();				\
	}						\
	return *this;					\
    }							\
							\
    _C##IACTUAL##_SP& _C##IACTUAL##_SP::operator = 	\
    (							\
	IUnknown* punkNew				\
    )							\
    {							\
	Release();					\
	if(punkNew)					\
	{						\
	    punkNew->QueryInterface			\
	    (						\
		IID_##IACTUAL,				\
		(void**)&pActual			\
	    );						\
	}						\
	return *this;					\
    }							\
							\
    HXBOOL _C##IACTUAL##_SP::operator ! () const		\
    {return !IsValid();}				\
							\
    HXBOOL _C##IACTUAL##_SP::IsValid() const		\
    {return pActual!=NULL;}				\
							\
    IACTUAL*& _C##IACTUAL##_SP::ptr_reference()		\
    {HX_ASSERT(!pActual);return pActual;}		\
							\
    IUnknown* _C##IACTUAL##_SP::AsUnknown() const	\
    {							\
	HX_ASSERT(pActual);				\
	IUnknown* punkOut;				\
	pActual->QueryInterface				\
	(						\
		IID_IUnknown,				\
		(void**)&punkOut			\
	);						\
	return punkOut;					\
    }							\
							\
    void _C##IACTUAL##_SP::Release()			\
    {HX_RELEASE(pActual);}

#define DECLARE_SMART_POINTER(IACTUAL) \
    _C##IACTUAL##_SP

#define DEFINE_WRAPPED_POINTER(CACTUAL)			\
class _C##CACTUAL##_WP					\
{							\
public:							\
    _C##CACTUAL##_WP();					\
    _C##CACTUAL##_WP(const _C##CACTUAL##_WP& rspact);	\
    _C##CACTUAL##_WP(const _C##CACTUAL##_SP& rspact);	\
    _C##CACTUAL##_WP(const CACTUAL* pActual);		\
    ~_C##CACTUAL##_WP();				\
							\
    _C##CACTUAL##_WP& operator=				\
    (							\
	const _C##CACTUAL##_WP& rspact			\
    );							\
							\
    void wrapped_ptr(const CACTUAL* pNew);		\
    const CACTUAL* wrapped_ptr() const;			\
    CACTUAL* wrapped_ptr();				\
							\
    HXBOOL IsValid() const;				\
							\
    void Empty();					\
private:						\
    CACTUAL*	    pActual;				\
};

#define IMPLEMENT_WRAPPED_POINTER(CACTUAL)		\
    DEFINE_WRAPPED_POINTER(CACTUAL)			\
							\
    _C##CACTUAL##_WP::_C##CACTUAL##_WP()		\
	: pActual(NULL)					\
    {}							\
    _C##CACTUAL##_WP::_C##CACTUAL##_WP			\
    (							\
	const _C##CACTUAL##_WP& rspact			\
    )							\
	: pActual(NULL)					\
    {wrapped_ptr(rspact.pActual);}			\
    _C##CACTUAL##_WP::_C##CACTUAL##_WP			\
    (							\
	const _C##CACTUAL##_SP& rspact			\
    )							\
	: pActual(NULL)					\
    {wrapped_ptr(rspact);}			\
    _C##CACTUAL##_WP::_C##CACTUAL##_WP			\
    (							\
	const CACTUAL* pActual				\
    )							\
	: pActual(NULL)					\
    {wrapped_ptr(pActual);}				\
    _C##CACTUAL##_WP::~_C##CACTUAL##_WP()		\
    {Empty();}						\
							\
    _C##CACTUAL##_WP& 					\
    _C##CACTUAL##_WP::operator=				\
    (							\
	const _C##CACTUAL##_WP& rspact			\
    )							\
    {wrapped_ptr(rspact.pActual); return *this;}	\
							\
    void _C##CACTUAL##_WP::wrapped_ptr			\
    (							\
	const CACTUAL* pNew				\
    )							\
    {							\
	Empty();					\
	if(pNew)					\
	{						\
	    pActual = (CACTUAL*)pNew;			\
	    pActual->AddRef();				\
	}						\
	return;						\
    }							\
    const CACTUAL* _C##CACTUAL##_WP::wrapped_ptr() const\
    {							\
	HX_ASSERT(pActual);				\
	return pActual;					\
    }							\
    CACTUAL* _C##CACTUAL##_WP::wrapped_ptr()		\
    {							\
	HX_ASSERT(pActual);				\
	return pActual;					\
    }							\
							\
    HXBOOL _C##CACTUAL##_WP::IsValid() const		\
    {return (pActual != NULL);}				\
							\
    void _C##CACTUAL##_WP::Empty()			\
    {HX_RELEASE(pActual);}

#define WRAPPED_POINTER(CACTUAL) \
    _C##CACTUAL##_WP
#endif // _SMARTPTR_H_
