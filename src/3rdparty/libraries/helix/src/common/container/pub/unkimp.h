/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unkimp.h,v 1.6 2007/07/06 20:35:02 jfinnecy Exp $
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
//	Macro implementation of IUnknown
//
//  Description:
//	This file defines classes and Macro's to simplify the implementation 
//	of IUnknown in RMA objects.
//	It also implements object creation methods 
//	(CreateObject, CreateInstance)
//
//  Usage Tips:
//	
//	Never use the new operator to create a class instance!!
//	    Instead use CreateObject if you need to use non-interface
//	    methods and CreateInstance otherwise.
//
//  Class(es) Defined in this file:
//
//	CUnknownIMP
//	    The COM Object implementation must inherit from this class.
//
//	CAggregateImpl
//	    Used internally to support Aggregation.
//
//  Macros Defined in this file:
//
//	DECLARE_UNKNOWN(THIS_CLASS)
//	    This must be in the class declaration.
//
//	BEGIN_INTERFACE_LIST(THIS_CLASS)
//	    This must be in the class implementation.
//
//	END_INTERFACE_LIST
//	    This must be in the class implementation.
//
//	INTERFACE_LIST_ENTRY(INTERFACE_IID, INTERFACE_CLASS)
//	    Defines the QI of a locally implemented interface.
//	    Optional: if used must be in the class implementation.
//
//	INTERFACE_LIST_ENTRY2(DERIVED_CLASS, BASE_INTERFACE_IID, BASE_CLASS)
//	    Defines the QI of a locally implemented interface that must be 
//	    disambiguated. DERIVED_CLASS is the intermediate class used to 
//	    achieve the disambiguation.
//
//	INTERFACE_LIST_ENTRY_DELEGATE(INTERFACE_IID, DELEGATE_QI_FUNCTION)
//	    Defines the QI of a single interface supported by an aggregated 
//	    object.
//	    Optional: if used must be in the class implementation.
//
//	INTERFACE_LIST_ENTRY_DELEGATE_BLIND(DELEGATE_QI_FUNCTION)
//	    Defines the QI of all interfaces supported by an aggregated object.
//	    Optional: if used must be in the class implementation.
//
//	INTERFACE_LIST_ENTRY_BASE
//	    Specifies a base class whose query interface function should be called.
//
//  Example:
//
//  rmabuffer.h
//	#include <unkimp.h>
//
//	class CHXBuffer 
//	    : public CUnknownIMP
//	    , public IHXBuffer
//	{
//	    DECLARE_UNKNOWN(CHXBuffer)
//	public:
//	    STDMETHOD(FinalConstruct)();
//	    void FinalRelease();
//
//	    // List of IHXBuffer Methods..
//
//	private:
//	    // List of private members..
//	    DECLARE_SMART_POINTER_UNKNOWN m_spunkOther;
//	    DECLARE_SMART_POINTER_UNKNOWN m_spunkYetAnother;
//	};
//
//  rmabuffer.cpp
//	#include "rmabuffer.h"
//	
//	BEGIN_INTERFACE_LIST(CHXBuffer)
//	    INTERFACE_LIST_ENTRY(IID_IHXBuffer, IHXBuffer)
//	    INTERFACE_LIST_ENTRY_DELEGATE
//	    (
//		IID_IHXOther, 
//		m_spunkOther.QueryInterface
//	    )
//	    INTERFACE_LIST_ENTRY_DELEGATE_BLIND
//	    (
//		m_spunkYetAnother.QueryInterface
//	    )
//	END_INTERFACE_LIST
//
//	STDMETHODIMP CHXBuffer::FinalConstruct()
//	{
//	    // Create the Aggregated objects here..
//	    //
//	    if 
//	    (
//		SUCCEEDED
//		(
//		    CIHXOther::CreateInstance
//		    (
//			GetControllingUnknown(),
//			&m_spunkOther
//		    )
//		)
//		&&
//		SUCCEEDED
//		(
//		    CIHXYetAnother::CreateInstance
//		    (
//			GetControllingUnknown(),
//			&m_spunkYetAnother
//		    )
//		)
//	    )
//	    {
//		return HXR_OK;
//	    }
//	    return HXR_FAIL;
//	}
//
//	void CHXBuffer::FinalRelease()
//	{
//	    // If we were not using SmartPointers, we would 
//	    // release the Aggregated objects here.
//	}
//
//	// Implement IHXBuffer methods..
//  


#ifndef __CUnknownIMP_H__
#define __CUnknownIMP_H__

#include "hxcom.h"
#include "hxassert.h"

#ifdef INCLUDE_DEBUG_LIFETIME
#include <io.h>
#include <fcntl.h>
#include "chxdataf.h"
#include "hxtrace.h"
#include "hxstring.h"
#endif // INCLUDE_DEBUG_LIFETIME

#ifdef ENABLE_UNKIMP_TRACKER
#include "ihxiunknowntracker.h"
#endif

class CUnknownIMP : public IUnknown
{
public:
    CUnknownIMP() : m_lCount(0), m_punkOuter(NULL)
#ifdef INCLUDE_DEBUG_LIFETIME
    ,m_pRaFileDebugLifetime(NULL)
#endif

    {
	m_punkControlling = this;
    }
    virtual ~CUnknownIMP()
    {}

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID riid,
	void** ppvObj
    )
    {
	if(m_punkOuter)
	{
	    return m_punkOuter->QueryInterface(riid, ppvObj);
	}
	return _ActualQI(riid, ppvObj);
    }

    STDMETHOD_(ULONG32,AddRef) (THIS)
    {
	if(m_punkOuter)
	{
	    return m_punkOuter->AddRef();
	}
	return ActualAddRef();
    }

    STDMETHOD_(ULONG32,Release) (THIS)
    {
	if(m_punkOuter)
	{
	    return m_punkOuter->Release();
	}
	return ActualRelease();
    }

    const IUnknown* GetUnknown() const
    {
	return this;
    }

    IUnknown* GetUnknown()
    {
	return this;
    }
    IUnknown* GetControllingUnknown()
    {
	return m_punkControlling;
    }

    STDMETHOD(SetupAggregation)( IUnknown* pvOuterObj, IUnknown** pvResult );
    
#ifdef ENABLE_UNKIMP_TRACKER
    static void SetIUnknownTracker( IHXIUnknownTracker* pIUnknownTracker )
    {
	if (pIUnknownTracker != ms_pIUnknownTracker)
	{
	    if (ms_pIUnknownTracker)
	    {
		IHXIUnknownTracker* pIOldUnknownTracker = ms_pIUnknownTracker;
		ms_pIUnknownTracker = NULL;
		HX_RELEASE(pIOldUnknownTracker);
	    }
	    if (pIUnknownTracker)
	    {
		pIUnknownTracker->AddRef();
		ms_pIUnknownTracker = pIUnknownTracker;
	    }
	}
    }
#endif

protected:
    // This is overriden in the derived object to provide
    // an object specific imp.
    STDMETHOD(_ActualQI)(REFIID riid, void** ppvObj) PURE;

#ifdef INCLUDE_DEBUG_LIFETIME

    CHXDataFile* m_pRaFileDebugLifetime;
    CHXString m_StringFilename;

    void InitFilename(const char* pFilename)
    {
	m_StringFilename = pFilename;
    }

    STDMETHOD_(ULONG32,ActualAddRef) ()
    {
	if (m_pRaFileDebugLifetime)
	{
	    char buf[80]; /* Flawfinder: ignore */
	    sprintf(buf, "*** AddRef %d ***\r\n", m_lCount+1); /* Flawfinder: ignore */
	    m_pRaFileDebugLifetime->Write(buf, strlen(buf));
	    UINT32* pBinaryStackTrace = NULL;
	    if (GetStack(&pBinaryStackTrace))
	    {
		char* pCharStackTrace = NULL;
		if (GetStackSymbols(pBinaryStackTrace, &pCharStackTrace))
		{
		    m_pRaFileDebugLifetime->Write
		    (
			pCharStackTrace,
			strlen(pCharStackTrace)
		    );
		}
		HX_VECTOR_DELETE(pCharStackTrace);
	    }
	    HX_VECTOR_DELETE(pBinaryStackTrace);
	}

	return InterlockedIncrement(&m_lCount);
    }
    STDMETHOD_(ULONG32,ActualRelease) ()
    {
	if (m_pRaFileDebugLifetime)
	{
	    char buf[80]; /* Flawfinder: ignore */
	    sprintf(buf, "*** Release %d ***\r\n", m_lCount-1); /* Flawfinder: ignore */
	    m_pRaFileDebugLifetime->Write(buf, strlen(buf));

	    UINT32* pBinaryStackTrace = NULL;

	    if (GetStack(&pBinaryStackTrace))
	    {
		char* pCharStackTrace = NULL;
	    	if (GetStackSymbols(pBinaryStackTrace, &pCharStackTrace))
		{
		    m_pRaFileDebugLifetime->Write
		    (
			pCharStackTrace,
			strlen(pCharStackTrace)
		    );
		}
		HX_VECTOR_DELETE(pCharStackTrace);
	    }
	    HX_VECTOR_DELETE(pBinaryStackTrace);
	}

	HX_ASSERT(m_lCount>0);

	if (InterlockedDecrement(&m_lCount) > 0)
	{
	    return m_lCount;
	}

	FinalRelease();
	delete this;
	return 0;
    }
    STDMETHOD(FinalConstruct)()
    {
	if (m_StringFilename.IsEmpty())
	{
	    return HXR_OK;
	}

	char szTemp[32]; /* Flawfinder: ignore */
	m_StringFilename.GetBuffer(0)[4] = '\0';
	m_StringFilename.ReleaseBuffer();
	ltoa((UINT32)this, szTemp, 16);
	m_StringFilename += szTemp;
	m_StringFilename.GetBuffer(0)[8] = '.';
	if (m_StringFilename.GetLength() > 12)
	{                           \
	    m_StringFilename.GetBuffer(0)[11] = '\0';
	    m_StringFilename.ReleaseBuffer();
	}                           \
	m_pRaFileDebugLifetime = CHXDataFile::Construct();
	m_pRaFileDebugLifetime->Open(m_StringFilename, _O_CREAT | _O_RDWR);
	m_pRaFileDebugLifetime->Write
	(
	    "*** Created new class instance ***\r\n",
	    sizeof("*** Created new class instance ***\r\n")
	);

	UINT32* pBinaryStackTrace = NULL;

	if (GetStack(&pBinaryStackTrace))
	{
	    char* pCharStackTrace = NULL;
	    if (GetStackSymbols(pBinaryStackTrace, &pCharStackTrace))
	    {
		m_pRaFileDebugLifetime->Write
		(
		    pCharStackTrace,
		    strlen(pCharStackTrace)
		);
	    }
	    HX_VECTOR_DELETE(pCharStackTrace);
	}

	HX_VECTOR_DELETE(pBinaryStackTrace);

	return HXR_OK;
    }
    virtual void FinalRelease()
    {
	if (m_pRaFileDebugLifetime)
	{
	    m_pRaFileDebugLifetime->Write("*** Deleted ***\r\n", 17);
	    UINT32* pBinaryStackTrace = NULL;
	    if (GetStack(&pBinaryStackTrace))
	    {
		char* pCharStackTrace = NULL;
		if (GetStackSymbols(pBinaryStackTrace, &pCharStackTrace))
		{
		    m_pRaFileDebugLifetime->Write
		    (
			pCharStackTrace,
			strlen(pCharStackTrace)
		    );
		}
	        HX_VECTOR_DELETE(pCharStackTrace);
	    }

	    HX_VECTOR_DELETE(pBinaryStackTrace);
	    m_pRaFileDebugLifetime->Close();
	}

	HX_DELETE(m_pRaFileDebugLifetime);
    }
#else
    // These can be overriden in the derived object to provide
    // an object specific imp.
    STDMETHOD_(ULONG32,ActualAddRef) (THIS)
    {
#ifdef ENABLE_UNKIMP_TRACKER
	ULONG32 postAddRefRefCount = InterlockedIncrement(&m_lCount);
	if (ms_pIUnknownTracker)
	{
	    ms_pIUnknownTracker->OnIUnknownAddRef( this, postAddRefRefCount );
	}
	return postAddRefRefCount;
#else
	return InterlockedIncrement(&m_lCount);
#endif
    }
    STDMETHOD_(ULONG32,ActualRelease) (THIS)
    {
	HX_ASSERT(m_lCount>0);
#ifdef ENABLE_UNKIMP_TRACKER
	ULONG32 preReleaseRefCount = m_lCount;
	if (ms_pIUnknownTracker)
	{
	    ms_pIUnknownTracker->OnIUnknownRelease( this, preReleaseRefCount );
	}
#endif
	if (InterlockedDecrement(&m_lCount) > 0)
	{
	    return m_lCount;
	}

	FinalRelease();
	delete this;
	return 0;
    }
    STDMETHOD(FinalConstruct)()
    {
	return HXR_OK;
    }
    virtual void FinalRelease()
    {
    }
#endif
    LONG32 m_lCount;
    IUnknown* m_punkOuter;
    IUnknown* m_punkControlling;

    friend class CAggregateImpl;
private:

    CUnknownIMP(CUnknownIMP&) : IUnknown () {}
    
#ifdef ENABLE_UNKIMP_TRACKER
    static IHXIUnknownTracker* ms_pIUnknownTracker;
#endif
};

#ifdef INCLUDE_DEBUG_LIFETIME
#define INIT_DEBUG_LIFETIME(CLASS)			\
CUnknownIMP::InitFilename(#CLASS);
#else
#define INIT_DEBUG_LIFETIME(CLASS)
#endif

#ifdef ENABLE_UNKIMP_TRACKER
#define INIT_UNKIMP_TRACKER \
IHXIUnknownTracker* CUnknownIMP::ms_pIUnknownTracker = NULL;
#endif

class CAggregateImpl : public IUnknown
{
public:
    CAggregateImpl( CUnknownIMP* pobjAggregated )
	: m_lCount( 0 )
	, m_pobjAggregated( pobjAggregated )
    {}
    virtual ~CAggregateImpl()
    {
	delete m_pobjAggregated;
    }

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)
    (
	THIS_
	REFIID riid,
	void** ppvObj
    )
    {
	return m_pobjAggregated->_ActualQI(riid, ppvObj);
    }

    STDMETHOD_(ULONG32,AddRef) (THIS)
    {
#ifdef ENABLE_UNKIMP_TRACKER
	ULONG32 postAddRefRefCount = InterlockedIncrement(&m_lCount);
	if (CUnknownIMP::ms_pIUnknownTracker)
	{
	    CUnknownIMP::ms_pIUnknownTracker->OnIUnknownAddRef( m_pobjAggregated, postAddRefRefCount );
	}
	return postAddRefRefCount;
#else
	return InterlockedIncrement(&m_lCount);
#endif
    }

    STDMETHOD_(ULONG32,Release) (THIS)
    {
	HX_ASSERT(m_lCount>0);
#ifdef ENABLE_UNKIMP_TRACKER
	ULONG32 preReleaseRefCount = m_lCount;
	if (CUnknownIMP::ms_pIUnknownTracker)
	{
	    CUnknownIMP::ms_pIUnknownTracker->OnIUnknownRelease( m_pobjAggregated, preReleaseRefCount );
	}
#endif
	if (InterlockedDecrement(&m_lCount) > 0)
	{
	    return m_lCount;
	}

	delete this;
	return 0;
    }

private:
    LONG32 m_lCount;
    CUnknownIMP* m_pobjAggregated;

    CAggregateImpl(){}
    CAggregateImpl(CAggregateImpl&) : IUnknown () {}
};





#define DECLARE_UNKNOWN(THIS_CLASS)				\
	DECLARE_COM_CREATE_FUNCS(THIS_CLASS)			\
    public:							\
	STDMETHOD(QueryInterface)				\
	(							\
	    THIS_						\
	    REFIID riid,					\
	    void** ppvObj					\
	);							\
	STDMETHOD_(ULONG32, AddRef)(THIS);			\
	STDMETHOD_(ULONG32, Release)(THIS);			\
    protected:							\
	STDMETHOD(_ActualQI)(REFIID riid, void** ppvObj);\
    public:	

#define DECLARE_UNKNOWN_NOCREATE(THIS_CLASS)				\
    public:							\
	STDMETHOD(QueryInterface)				\
	(							\
	    THIS_						\
	    REFIID riid,					\
	    void** ppvObj					\
	);							\
	STDMETHOD_(ULONG32, AddRef)(THIS);			\
	STDMETHOD_(ULONG32, Release)(THIS);			\
    protected:							\
	STDMETHOD(_ActualQI)(REFIID riid, void** ppvObj);\
    public:

#define BEGIN_INTERFACE_LIST(THIS_CLASS)				\
	IMPLEMENT_COM_CREATE_FUNCS(THIS_CLASS)				\
	STDMETHODIMP THIS_CLASS::QueryInterface				\
	(								\
	    THIS_							\
	    REFIID riid,						\
	    void** ppvObj						\
	)								\
	{return CUnknownIMP::QueryInterface(riid, ppvObj);}		\
	STDMETHODIMP_(ULONG32) THIS_CLASS::AddRef(THIS)			\
	{return CUnknownIMP::AddRef();}					\
	STDMETHODIMP_(ULONG32) THIS_CLASS::Release(THIS)		\
	{return CUnknownIMP::Release();}				\
	STDMETHODIMP THIS_CLASS::_ActualQI(REFIID riid, void** ppvObj)	\
	{								\
	    if (!ppvObj)						\
		return HXR_POINTER;					\
	    if (IsEqualIID(IID_IUnknown, riid))				\
	    {								\
		AddRef();						\
		*ppvObj = CUnknownIMP::GetUnknown();			\
		return HXR_OK;						\
	    }

#define BEGIN_INTERFACE_LIST_NOCREATE(THIS_CLASS)				\
	STDMETHODIMP THIS_CLASS::QueryInterface				\
	(								\
	    THIS_							\
	    REFIID riid,						\
	    void** ppvObj						\
	)								\
	{return CUnknownIMP::QueryInterface(riid, ppvObj);}		\
	STDMETHODIMP_(ULONG32) THIS_CLASS::AddRef(THIS)			\
	{return CUnknownIMP::AddRef();}					\
	STDMETHODIMP_(ULONG32) THIS_CLASS::Release(THIS)		\
	{return CUnknownIMP::Release();}				\
	STDMETHODIMP THIS_CLASS::_ActualQI(REFIID riid, void** ppvObj)	\
	{								\
	    if (!ppvObj)						\
		return HXR_POINTER;					\
	    if (IsEqualIID(IID_IUnknown, riid))				\
	    {								\
		AddRef();						\
		*ppvObj = CUnknownIMP::GetUnknown();			\
		return HXR_OK;						\
	    }

#define INTERFACE_LIST_ENTRY(INTERFACE_IID, INTERFACE_CLASS)	\
	    if (IsEqualIID(INTERFACE_IID, riid))		\
	    {							\
		AddRef();					\
		INTERFACE_CLASS* pThisAsInterfaceClass = this;  \
		*ppvObj = pThisAsInterfaceClass;		\
		return HXR_OK;					\
	    }

/*!
    @defined INTERFACE_LIST_ENTRY_BASE
    @discussion Specifies a base class whose query interface function should be called.
*/
#define INTERFACE_LIST_ENTRY_BASE( BASE_CLASS )				\
	if( SUCCEEDED( BASE_CLASS::_ActualQI( riid, ppvObj ) ) )	\
	{								\
	    return HXR_OK;						\
	}


#define INTERFACE_LIST_ENTRY2(DERIVED_CLASS, BASE_INTERFACE_IID, BASE_CLASS)	\
	    if (IsEqualIID(BASE_INTERFACE_IID, riid))		\
	    {							\
		AddRef();					\
		DERIVED_CLASS* pTemp1 = this;			\
		BASE_CLASS* pTemp2 = pTemp1;			\
		*ppvObj = pTemp2;				\
		return HXR_OK;					\
	    }

#define INTERFACE_LIST_ENTRY_DELEGATE_BLIND(DELEGATE_QI_FUNCTION)\
	    if( SUCCEEDED( DELEGATE_QI_FUNCTION( riid, ppvObj ) ) )\
	    {							\
		return HXR_OK;					\
	    }							\

#define INTERFACE_LIST_ENTRY_DELEGATE(INTERFACE_IID, DELEGATE_QI_FUNCTION)\
	    if(IsEqualIID(INTERFACE_IID, riid) &&		\
		SUCCEEDED( DELEGATE_QI_FUNCTION( riid, ppvObj ) ) )\
	    {							\
		return HXR_OK;					\
	    }

#define END_INTERFACE_LIST					\
	    *ppvObj = NULL;					\
	    return HXR_NOINTERFACE;				\
	}

/*!
    @defined END_INTERFACE_LIST_BASE
    @discussion Ends the list of supported interfaces.  Specifies the base class whose
    query interface function should be called.
*/
#define END_INTERFACE_LIST_BASE( BASE_CLASS )									\
	return BASE_CLASS::_ActualQI( riid, ppvObj );							\
    }                                                       							



#define DECLARE_COM_CREATE_FUNCS(CLASS)				\
    public:							\
	static CLASS* CreateObject();				\
	static HX_RESULT CreateObject(CLASS** ppObj);		\
	static HX_RESULT CreateInstance(IUnknown** ppvObj);	\
	static HX_RESULT CreateInstance				\
	(							\
	    IUnknown* pvOuter,					\
	    IUnknown** ppvObj					\
	);							\

#define IMPLEMENT_COM_CREATE_FUNCS(CLASS)			\
    HX_RESULT CLASS::CreateObject(CLASS** ppObj)		\
    {								\
	*ppObj = new CLASS;					\
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
    CLASS* CLASS::CreateObject()				\
    {								\
	CLASS* pNew = NULL;					\
	if (SUCCEEDED(CreateObject(&pNew)))			\
	{							\
	    return pNew;					\
	}							\
	return NULL;						\
    }								\
    HX_RESULT CLASS::CreateInstance				\
    (								\
	IUnknown* pvOuterObj,					\
	IUnknown** ppvObj					\
    )								\
    {								\
	if (!ppvObj)						\
	    return HXR_POINTER;					\
	*ppvObj = NULL;						\
	CLASS* pNew = NULL;					\
	HX_RESULT pnrRes = CreateObject(&pNew);			\
	if (SUCCEEDED(pnrRes) && pNew)				\
	{							\
	    pnrRes = pNew->SetupAggregation( pvOuterObj, ppvObj );	\
	}							\
	return pnrRes;						\
    }								\
    HX_RESULT CLASS::CreateInstance(IUnknown** ppvObj)		\
    {								\
	return CreateInstance(NULL, ppvObj);			\
    }								
    
    

/*!
    @defined IMPLEMENT_UNKNOWN_NOINTERFACE
    @discussion This fills in implementations for AddRef() and Release()--basically forwards
    calls onto CUnknownIMP.
*/
#define IMPLEMENT_UNKNOWN_NOINTERFACE( THIS_CLASS )										\
    STDMETHODIMP THIS_CLASS::QueryInterface( REFIID riid, void** ppvObj)                              \
    {                                                                                                           \
	return CUnknownIMP::QueryInterface( riid, ppvObj );                                                      \
    }														\
    STDMETHODIMP_(ULONG32) THIS_CLASS::AddRef(THIS) { return CUnknownIMP::AddRef(); }				\
    STDMETHODIMP_(ULONG32) THIS_CLASS::Release(THIS) { return CUnknownIMP::Release(); }      		        


/*!
    @defined INTERFACE_LIST_ENTRY_SIMPLE
    @discussion Simplified version of INTERFACE_LIST_ENTRY.  Only takes the interface
    name.  The IID is assumed to be the interface name with 'IID_' prepended.
*/
#define INTERFACE_LIST_ENTRY_SIMPLE( INTERFACE_CLASS )							\
	INTERFACE_LIST_ENTRY( IID_##INTERFACE_CLASS, INTERFACE_CLASS )                                        

/*!
    @defined INTERFACE_LIST_ENTRY_SIMPLE2
    @discussion This allows an interface to appear in an interface list 
      even when the conversion to it is ambiguous owing to multiple 
      inheritence. By specifying an intermediate interface the conversion 
      is disambiguated.
      This is the simplified version of INTERFACE_LIST_ENTRY2. Only takes 
      the interface names.  The IID are assumed to be the interface names 
      with 'IID_' prepended.
*/
#define INTERFACE_LIST_ENTRY_SIMPLE2( DERIVED_CLASS, BASE_CLASS )				\
	INTERFACE_LIST_ENTRY2( DERIVED_CLASS, IID_##BASE_CLASS, BASE_CLASS )                                        



/*!
    @defined INTERFACE_LIST_ENTRY_TESTPOINTER
*/
#define INTERFACE_LIST_ENTRY_MEMBER( INTERFACE_IID, MEMBER_VARIABLE )					\
	if( NULL != ( MEMBER_VARIABLE ) )								\
	{                                                                                               \
	    if( IsEqualIID( INTERFACE_IID, riid ) )                                                     \
	    {                                                                                           \
		if( SUCCEEDED( MEMBER_VARIABLE->QueryInterface( riid, ppvObj ) ) )                      \
		    return HXR_OK;                                                                      \
	    }                                                                                           \
	}                                                                                               \

/*!
    @defined INTERFACE_LIST_ENTRY_TESTPOINTER
*/
#define INTERFACE_LIST_ENTRY_MEMBER_BLIND( MEMBER_VARIABLE )						\
	if( NULL != ( MEMBER_VARIABLE ) )								\
	{                                                                                               \
	    if( SUCCEEDED( MEMBER_VARIABLE->QueryInterface( riid, ppvObj ) ) )                      	\
		return HXR_OK;                                                                      	\
	}                                                                                               \



#endif //!__CUnknownIMP_H__
