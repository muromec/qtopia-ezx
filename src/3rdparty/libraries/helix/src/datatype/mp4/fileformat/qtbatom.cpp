/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/****************************************************************************
 *  Includes
 */
#include "qtbatom.h"
#include "mempager.h"


/****************************************************************************
 *  Class:
 *	CQTAtom
 */
/****************************************************************************
 *  Constructors/Destructor
 */
CQTAtom::CQTAtom(void)
    : m_pParent(NULL)
    , m_pChildList(NULL)
    , m_ulOffset(0)
    , m_ulSize(0)
    , m_pBuffer(NULL)
    , m_pData(NULL)
    , m_lRefCount(0)
{
    ;
}


CQTAtom::CQTAtom(ULONG32 ulOffset,
		 ULONG32 ulSize, 
		 CQTAtom *pParent)
    : m_pParent(NULL)
    , m_pChildList(NULL)
    , m_ulOffset(ulOffset)
    , m_ulSize(ulSize)
    , m_pBuffer(NULL)
    , m_pData(NULL)
    , m_lRefCount(0)
{
    if (pParent != NULL)
    {
	pParent->AddPresentChild(this);
    }
}


CQTAtom::~CQTAtom()
{
    // Disown All children and release them
    if (m_pChildList)
    {
	CQTAtom *pChild;

	while (!m_pChildList->IsEmpty())
	{
	    pChild = (CQTAtom*) m_pChildList->RemoveHead();
	    pChild->m_pParent = NULL;
	    pChild->Release();
	}

	delete m_pChildList;
    }

    // If there is a parent, detach from it
    if (m_pParent)
    {
	LISTPOSITION SelfEntry;

	HX_ASSERT(m_pParent->m_pChildList);
	HX_ASSERT(!m_pParent->m_pChildList->IsEmpty());

	SelfEntry = m_pParent->m_pChildList->Find(this);

	HX_ASSERT(SelfEntry);

	m_pParent->m_pChildList->RemoveAt(SelfEntry);
    }

    HX_RELEASE(m_pBuffer);
}


/****************************************************************************
 *  Access Functions
 */
void CQTAtom::SetBuffer(IHXBuffer *pBuffer)
{
    HX_RELEASE(m_pBuffer);
    m_pData = NULL;
    m_pBuffer = pBuffer;
    if (pBuffer)
    {
	pBuffer->AddRef();
	m_pData = pBuffer->GetBuffer();
    }
}


HX_RESULT CQTAtom::AddPresentChild(CQTAtom *pChild)
{
    HX_ASSERT(pChild);
    HX_ASSERT(!pChild->m_pParent);

    if (m_pChildList == NULL)
    {
	m_pChildList = new CHXSimpleList;

	if (m_pChildList == NULL)
	{
	    return HXR_OUTOFMEMORY;
	}
    }

    m_pChildList->AddTail(pChild);
    pChild->m_pParent = this;

    pChild->AddRef();

    return HXR_OK;
}


CQTAtom* CQTAtom::GetPresentChild(UINT16 uChildIndex)
{
    if (m_pChildList != NULL)
    {
	LISTPOSITION ListPosition = m_pChildList->FindIndex((int) uChildIndex);

	if (ListPosition != NULL)
	{
	    return (CQTAtom *) m_pChildList->GetAt(ListPosition);
	}
    }

    return NULL;
}


CQTAtom* CQTAtom::FindPresentChild(QTAtomType AtomType)
{
    UINT16 uChildIdx = 0;
    UINT16 uChildCount = GetPresentChildCount();
    LISTPOSITION CurrentPosition;
    CQTAtom* pCurrentChild;

    if (uChildCount > 0)
    {
	CurrentPosition = m_pChildList->GetHeadPosition();

	do
	{
	    pCurrentChild = (CQTAtom*) m_pChildList->GetNext(CurrentPosition);
	    if (pCurrentChild->GetType() == AtomType)
	    {
		return pCurrentChild;
	    }
	} while ((++uChildIdx) < uChildCount);
    }
	
    return NULL;
}


/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP CQTAtom::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown *) this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CQTAtom::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CQTAtom::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/****************************************************************************
 *  Class:
 *	CQTPagingAtom
 */
/****************************************************************************
 *  Destructor
 */
CQTPagingAtom::~CQTPagingAtom()
{
#if !defined(QTCONFIG_NO_PAGING)
    HX_RELEASE(m_pMemPager);
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
}

/****************************************************************************
 *  SetMemPager
 */
void CQTPagingAtom::SetMemPager(CMemPager* pMemPager)
{
    m_pData = NULL;
    HX_RELEASE(m_pBuffer);
    m_lastStatus = HXR_OK;
#if !defined(QTCONFIG_NO_PAGING)
    HX_RELEASE(m_pMemPager);
    m_pMemPager = pMemPager;
    if (pMemPager)
    {
	pMemPager->AddRef();
	m_pData = pMemPager->GetBaseVirtualAddr();
    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
}

/****************************************************************************
 *  SetBuffer
 */
void CQTPagingAtom::SetBuffer(IHXBuffer *pBuffer)
{
#if !defined(QTCONFIG_NO_PAGING)
    HX_RELEASE(m_pMemPager);
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
    m_lastStatus = HXR_OK;
    CQTAtom::SetBuffer(pBuffer);
}


/****************************************************************************
 *  IsFaulted
 */
HXBOOL CQTPagingAtom::IsFaulted(void)
{
#if !defined(QTCONFIG_NO_PAGING)
    return m_pMemPager ? m_pMemPager->IsFaulted() : FALSE;
#else
    return FALSE;
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
}

