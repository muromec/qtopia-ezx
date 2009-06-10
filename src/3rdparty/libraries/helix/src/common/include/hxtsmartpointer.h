/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxtsmartpointer.h,v 1.7 2008/08/20 21:04:01 ehyche Exp $
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

#ifndef SMART_POINTER_H
#define SMART_POINTER_H

#include "hxcom.h"

#if defined(HELIX_FEATURE_ALLOW_DEPRECATED_SMARTPOINTERS)
//
// Smart Pointer class that works with any pointer derived from IUnknown

template<class T>
class CHXTSmartPtr
{
public:
	// contruction
	CHXTSmartPtr( T *ptr = 0 );
	CHXTSmartPtr( const CHXTSmartPtr<T> &spCopy );
	~CHXTSmartPtr();

	// operators
	CHXTSmartPtr<T>& operator=( CHXTSmartPtr<T> &sp );
	CHXTSmartPtr<T>& operator=( T *ptr );

	bool operator==( const CHXTSmartPtr<T> &sp ) const;
	bool operator==( const T *ptr ) const;

	bool operator!=( const CHXTSmartPtr<T>& sp ) const;
	bool operator!=( const T * ptr ) const;

	bool operator<( const CHXTSmartPtr<T> &sp ) const;
	bool operator<( const T *ptr ) const;

	bool operator>( const CHXTSmartPtr<T> &sp ) const;
	bool operator>( const T *ptr ) const;
	
	T* operator->() const;
	T& operator*() const;
	operator T*() const;
	T* RawPointer() const;	// allows explicit access to pointer without having to cast


	///////////////////////////////////////////////////////////////////////////
	// additional accessors	
	//

	//	The Adopt method assumes a pointer without AddRef'ing it.
	//	This is usefull when calling COM methods that do an Automatic AddRef.
	//
	//	example:
	//
	//	Foo::GetObject( IUnknown **pUnk )
	//	{
	//		*pUnk = new MyObject();
	//		pUnk->AddRef();
	//	}
	//	...
	//	pFoo->GetObject( spMySmartPointer->Adopt() );
		
	T** Adopt();
	IUnknown** AdoptUnknown();
	void** AdoptVoid();
	
protected:
	T *m_ptr;
};


// inline implementation of CHXTSmartPtr<T>

template<class T>
inline CHXTSmartPtr<T>::CHXTSmartPtr( T *ptr ) :
	m_ptr( ptr )
{	
	if ( m_ptr )
	{
		m_ptr->AddRef();
	}
}

template<class T>
inline CHXTSmartPtr<T>::CHXTSmartPtr( const CHXTSmartPtr<T> &spCopy ) :
	m_ptr( spCopy.m_ptr ) 
{
	if ( m_ptr )
	{
		m_ptr->AddRef();
	}
}

template<class T>
inline CHXTSmartPtr<T>::~CHXTSmartPtr()
{
	if ( m_ptr )
	{
		m_ptr->Release();
		m_ptr = 0;
	}
}


//
// operators
//
template<class T>
inline CHXTSmartPtr<T>& CHXTSmartPtr<T>::operator=( T *ptr )
{
	// release current pointer if any
	if ( m_ptr )
	{
		m_ptr->Release();
	}

	// copy pointer
	m_ptr = ptr;
	if ( m_ptr )
	{
		// add reference
		m_ptr->AddRef();
	}

	return *this;
}

template<class T>
inline CHXTSmartPtr<T>& CHXTSmartPtr<T>::operator=( CHXTSmartPtr<T> &sp )
{ 
	return operator=( sp.m_ptr );
}


template<class T>
inline bool CHXTSmartPtr<T>::operator==( const CHXTSmartPtr<T> &sp ) const
{
	return m_ptr == sp.m_ptr;
}

template<class T>
inline bool CHXTSmartPtr<T>::operator==( const T *ptr ) const
{
	return m_ptr == ptr;
}


template<class T>
inline bool CHXTSmartPtr<T>::operator!=( const CHXTSmartPtr<T>& sp ) const
{
	return m_ptr != sp.m_ptr;
}


template<class T>
inline bool CHXTSmartPtr<T>::operator!=( const T* ptr ) const
{
	return m_ptr != ptr;
}


template<class T>
inline bool CHXTSmartPtr<T>::operator>( const CHXTSmartPtr<T> &sp ) const
{
	return m_ptr > sp.m_ptr;
}

template<class T>
inline bool CHXTSmartPtr<T>::operator>( const T *ptr ) const
{
	return m_ptr > ptr;
}


template<class T>
inline bool CHXTSmartPtr<T>::operator<( const CHXTSmartPtr<T> &sp ) const
{
	return m_ptr < sp.m_ptr;
}

template<class T>
inline bool CHXTSmartPtr<T>::operator<( const T *ptr ) const
{
	return m_ptr < ptr;
}

template<class T>
inline T* CHXTSmartPtr<T>::RawPointer() const
{
	return m_ptr;
}

template<class T>
inline T* CHXTSmartPtr<T>::operator->() const
{
	return m_ptr;
}

template<class T>
inline T& CHXTSmartPtr<T>::operator*() const
{
	return *m_ptr;
}

template<class T>
inline CHXTSmartPtr<T>::operator T*() const
{
	return m_ptr;
}

template<class T>
inline T** CHXTSmartPtr<T>::Adopt()
{
	HX_RELEASE( m_ptr );
	return &m_ptr;
}

template<class T>
inline IUnknown** CHXTSmartPtr<T>::AdoptUnknown()
{
	HX_RELEASE( m_ptr );
	return (IUnknown**) &m_ptr;
}

template<class T>
inline void** CHXTSmartPtr<T>::AdoptVoid()
{
	HX_RELEASE( m_ptr );
	return (void**) &m_ptr;
}

#define HXT_MAKE_CLASS_SMART_PTR_BY_NAME( ClassName, PointerName ) typedef CHXTSmartPtr<ClassName> PointerName;
#define HXT_MAKE_CLASS_SMART_PTR( ClassName ) HXT_MAKE_CLASS_SMART_PTR_BY_NAME( ClassName, ClassName ## Ptr )

//  Query does a QI on the argument pointer and stores the result internally.
//  eg. if ( spMyObject.Query( pCompositeObjectWhichImplementsMyObject ) )
//	{ do stuff ... }
	
//#define HXT_MAKE_SMART_QUERY_PTR_WITH_GUID( ClassName, THIS_CLSIID )			
#define HXT_MAKE_SMART_QUERY_PTR_BY_NAME_WITH_GUID( ClassName, PointerName, THIS_CLSIID )			\
	class PointerName : public CHXTSmartPtr<ClassName>							\
	{																			\
	public:																		\
		HX_RESULT Query( IUnknown *pIUnknown )									\
		{																		\
			HX_RELEASE( m_ptr );												\
			return pIUnknown->QueryInterface( THIS_CLSIID, (void**) &m_ptr );	\
		}																		\
																				\
		HX_RESULT QuerySelf()													\
		{																		\
			ClassName *pResult;													\
			HX_RESULT res = m_ptr->QueryInterface( THIS_CLSIID,					\
												   (void**) &pResult );			\
			if ( SUCCEEDED( res ) )												\
			{																	\
				m_ptr = pResult;												\
				pResult->Release(); /* release additional reference from QI */	\
			}																	\
																				\
			return res;															\
		}																		\
                                                                                \
        template <class TAFactory>                                              \
        HX_RESULT CreateFromFactory( TAFactory pFactory )                       \
        {                                                                       \
            return pFactory->CreateInstance( THIS_CLSIID, AdoptVoid() );        \
        }                                                                       \
                                                                                \
		ClassName ## Ptr() :													\
			CHXTSmartPtr<ClassName>() {}									    \
																				\
		ClassName ## Ptr( ClassName *ptr ) :									\
			CHXTSmartPtr<ClassName>( ptr ) {}									\
																				\
		ClassName ## Ptr( const ClassName ## Ptr &spCopy ):						\
			CHXTSmartPtr<ClassName>( spCopy.m_ptr ) {}							\
																				\
		ClassName ## Ptr& operator=( ClassName *ptr )							\
			{ CHXTSmartPtr<ClassName>::operator=( ptr ); return *this; }		\
																				\
		ClassName ## Ptr& operator=( ClassName ## Ptr &sp )						\
			{ return ClassName ## Ptr::operator=( sp.m_ptr ); }					\
																				\
		bool operator==( const ClassName *ptr ) const							\
			{ return m_ptr == ptr; }											\
																				\
		bool operator==( const ClassName ## Ptr &sp ) const						\
			{ return operator==( sp.m_ptr ); }									\
																				\
		bool operator!=( const ClassName * ptr ) const							\
			{ return m_ptr != ptr; }											\
																				\
		bool operator!=( const ClassName ## Ptr& sp ) const						\
			{ return operator!=( sp.m_ptr ); }									\
	};

#define HXT_MAKE_SMART_QUERY_PTR_BY_NAME( ClassName, PointerName ) HXT_MAKE_SMART_QUERY_PTR_BY_NAME_WITH_GUID( ClassName, PointerName, IID_ ## ClassName )
#define HXT_MAKE_SMART_QUERY_PTR_WITH_GUID( ClassName, THIS_CLSIID ) HXT_MAKE_SMART_QUERY_PTR_BY_NAME_WITH_GUID( ClassName, ClassName ## Ptr, THIS_CLSIID )
#define HXT_MAKE_SMART_QUERY_PTR( ClassName ) HXT_MAKE_SMART_QUERY_PTR_WITH_GUID( ClassName, IID_ ## ClassName )

#define HXT_MAKE_SMART_PTR HXT_MAKE_SMART_QUERY_PTR
#define HXT_MAKE_SMART_PTR_WITH_GUID HXT_MAKE_SMART_QUERY_PTR_WITH_GUID 
#define HXT_DEFINE_SMART_PTR( ClassName ) struct ClassName; typedef CHXTSmartPtr<ClassName> ClassName ## Ptr;



// if hxassert.h has been included
#ifdef _HXASSERT_H_

template <class TSmartPointer>
void AssertValidInterface( TSmartPointer spTest )
{
	TSmartPointer spCheckBefore = spTest;
	spTest.QuerySelf();
	HX_ASSERT( spCheckBefore == spTest );
}

#endif

#if defined( DEBUG ) || defined( _DEBUG )
#define ASSERT_VALID_INTERFACE AssertValidInterface

#else
#define ASSERT_VALID_INTERFACE
#endif

#else /* #if defined(HELIX_FEATURE_ALLOW_DEPRECATED_SMARTPOINTERS) */

/* Implement deprecated smart pointers with HXCOMPtr-based smart pointers */
#include "hxcomptr.h"

/*
 * HXT_MAKE_CLASS_SMART_PTR is used in Producer SDK code when
 * a smart pointer is needed for a class. The class must implement
 * IUnknown, but does not necessarily have to implement any interfaces
 * which are derived from IUnknown.
 */
#define HXT_MAKE_CLASS_SMART_PTR(ClassName) HX_MAKE_CLASS_SMART_PTR_BY_NAME(ClassName, ClassName##Ptr)

/*
 * HXT_MAKE_SMART_PTR is the commonly-used macro in Producer SDK code when
 * a smart pointer is needed for an IUnknown-derived interface.
 * 
 */
#define HXT_MAKE_SMART_PTR(Interface) HX_MAKE_SMART_QUERY_PTR_BY_NAME_WITH_GUID(Interface, Interface##Ptr, IID_##Interface)


#endif /* #if defined(HELIX_FEATURE_ALLOW_DEPRECATED_SMARTPOINTERS) #else */


#endif // SMART_POINTER_H
