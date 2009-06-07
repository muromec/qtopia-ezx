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
#ifdef QTCONFIG_BFRAG_FACTORY
#include "bfragfct.h"
#endif	// QTCONFIG_BFRAG_FACTORY
#include "bfrag.h"
#include "hxassert.h"


/****************************************************************************
 *  Globals
 */

/****************************************************************************
 *  Local Defines
 */

/****************************************************************************
 *  Class CBufferFragment
 */
/****************************************************************************
 *  Destructor
 */
CBufferFragment::~CBufferFragment()
{
#ifdef QTCONFIG_BFRAG_FACTORY
    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pBufferFragmentFactory);
#else	// QTCONFIG_BFRAG_FACTORY
    m_pBuffer->Release();
#endif	// QTCONFIG_BFRAG_FACTORY
    
}


/****************************************************************************
 *  Public methods
 */
#ifdef QTCONFIG_BFRAG_FACTORY
void CBufferFragment::Link(CBufferFragmentFactory* pBufferFragmentFactory)
{
    if (m_pBufferFragmentFactory)
    {
	m_pBufferFragmentFactory->Release();
    }
    m_pBufferFragmentFactory = pBufferFragmentFactory;
    if (pBufferFragmentFactory)
    {
	pBufferFragmentFactory->AddRef();
    }
}

void CBufferFragment::Unlink(void)
{
    HX_RELEASE(m_pBufferFragmentFactory);
}
#endif	// QTCONFIG_BFRAG_FACTORY


/****************************************************************************
 *  IHXBuffer methods
 */
/****************************************************************************
 *  Get
 */
STDMETHODIMP CBufferFragment::Get(REF(UCHAR*)	pData, 
				  REF(ULONG32)	ulLength)
{
    pData = m_pData;
    ulLength = m_ulSize;

    return HXR_OK;
}

/****************************************************************************
 *  Set
 */
STDMETHODIMP CBufferFragment::Set(const UCHAR*	pData, 
				  ULONG32 ulLength)
{
    HX_ASSERT(FALSE);

    return HXR_FAIL;
}

/****************************************************************************
 *  SetSize
 */
STDMETHODIMP CBufferFragment::SetSize(ULONG32 ulLength)
{
    if (m_pBuffer)
    {
	if (m_pData - m_pBuffer->GetBuffer() + ulLength <= m_ulSize)
	{
	    m_ulSize = ulLength;
	    return HXR_OK;
	}
    }

    HX_ASSERT(FALSE);

    return HXR_FAIL;
}

/****************************************************************************
 *  GetSize
 */
STDMETHODIMP_(ULONG32) CBufferFragment::GetSize()
{
    return m_ulSize;
}

/****************************************************************************
 *  GetBuffer
 */
STDMETHODIMP_(UCHAR*) CBufferFragment::GetBuffer()
{
    return m_pData;
}


/****************************************************************************
 *  IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//
STDMETHODIMP CBufferFragment::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXBuffer))
    {
	AddRef();
	*ppvObj = (IHXBuffer*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }

    *ppvObj = NULL;

    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CBufferFragment::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//
STDMETHODIMP_(ULONG32) CBufferFragment::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

#ifdef QTCONFIG_BFRAG_FACTORY
    if (m_pBufferFragmentFactory)
    {
	HX_RELEASE(m_pBuffer);
	m_pBufferFragmentFactory->Put(this);
	m_pBufferFragmentFactory->Release();
	m_pBufferFragmentFactory = NULL;
    }
    
    if (m_lRefCount == 0)
#endif	// QTCONFIG_BFRAG_FACTORY
    {
	delete this;
    }

    return 0;
}
