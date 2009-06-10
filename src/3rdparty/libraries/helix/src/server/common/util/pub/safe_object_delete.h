/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: safe_object_delete.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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
#include "hxengin.h"

//-----------------DECL-------------------------

#define DECLARE_SAFE_DELETION(CLASS)				\
class _##CLASS##_DeleteCallback					\
    : public IHXCallback					\
{								\
public:								\
    _##CLASS##_DeleteCallback();				\
    void SetObjectToDelete(CLASS* expired);			\
public:								\
    STDMETHOD(QueryInterface) (REFIID riid, void** ppvObj);	\
    STDMETHOD_(ULONG32,AddRef) ();				\
    STDMETHOD_(ULONG32,Release) ();				\
    STDMETHOD(Func) (THIS);					\
private:							\
    ~_##CLASS##_DeleteCallback();				\
private:							\
    LONG32	m_lRefCount;					\
    CLASS*	m_class;					\
};

#define USE_SAFE_DELETION(CLASS)		\
public:						\
    void _DeleteSafely();			\
private:					\
    friend class _##CLASS##_DeleteCallback;	\

//-----------------IMPL-------------------------

#define IMPLEMENT_SAFE_DELETION(CLASS, IHXSCHEDULER_IMP)	\
    void							\
    CLASS::_DeleteSafely()					\
    {								\
	HX_ASSERT(IHXSCHEDULER_IMP);				\
	HX_ASSERT(this);					\
	IHXCallback* pCallbackObj = NULL;			\
	_##CLASS##_DeleteCallback* pDeleteObj = new _##CLASS##_DeleteCallback();\
	pDeleteObj->SetObjectToDelete(this);			\
	pDeleteObj->QueryInterface(IID_IHXCallback, (void**)&pCallbackObj);\
	HX_ASSERT(pCallbackObj);				\
	IHXSCHEDULER_IMP->RelativeEnter(pCallbackObj, 0);	\
	HX_RELEASE(pCallbackObj);				\
    }								\
    _##CLASS##_DeleteCallback::_##CLASS##_DeleteCallback()	\
	: m_lRefCount(0)					\
	, m_class(NULL)						\
    {								\
    }								\
    _##CLASS##_DeleteCallback::~_##CLASS##_DeleteCallback()	\
    {								\
    }								\
    STDMETHODIMP						\
    _##CLASS##_DeleteCallback::QueryInterface(REFIID riid, void** ppvObj)\
    {								\
	if (IsEqualIID(riid, IID_IUnknown))			\
	{							\
	    AddRef();						\
	    *ppvObj = (IUnknown*)this;				\
	    return HXR_OK;					\
	}							\
	else if (IsEqualIID(riid, IID_IHXCallback))		\
	{							\
	    AddRef();						\
	    *ppvObj = (IHXCallback*)this;			\
	    return HXR_OK;					\
	}							\
	*ppvObj = NULL;						\
	return HXR_NOINTERFACE;					\
    }								\
    STDMETHODIMP_(ULONG32)					\
    _##CLASS##_DeleteCallback::AddRef()				\
    {								\
	return InterlockedIncrement(&m_lRefCount);		\
    }								\
    STDMETHODIMP_(ULONG32)					\
    _##CLASS##_DeleteCallback::Release()				\
    {								\
	if (InterlockedDecrement(&m_lRefCount) > 0)		\
	{							\
	    return m_lRefCount;					\
	}							\
	delete this;						\
	return 0;						\
    }								\
    void							\
    _##CLASS##_DeleteCallback::SetObjectToDelete(CLASS* expired)	\
    {								\
	m_class = expired;					\
    }								\
    STDMETHODIMP						\
    _##CLASS##_DeleteCallback::Func()				\
    {								\
	HX_DELETE(m_class);					\
	return HXR_OK;						\
    }								\

#define DELETE_SAFELY()	\
    _DeleteSafely();
