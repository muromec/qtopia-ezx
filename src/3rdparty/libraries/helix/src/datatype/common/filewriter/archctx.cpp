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
 * Defines
 */


/****************************************************************************
 * Includes
 */
#include "archctx.h"


/************************************************************************
 *  Constructor/Destructor
 */
CArchiverContext::CArchiverContext(IUnknown* pContext,
				   LONG32* plStaticRefCount)
    : m_pContext(pContext)
    , m_plStaticRefCount(plStaticRefCount)
    , m_lRefCount(0)
{
    if (m_plStaticRefCount)
    {
	InterlockedIncrement(m_plStaticRefCount);
    }

    if (m_pContext)
    {
	m_pContext->AddRef();
    }
}

CArchiverContext::~CArchiverContext()
{
    HX_RELEASE(m_pContext);

    if (m_plStaticRefCount)
    {
	InterlockedDecrement(m_plStaticRefCount);
    }
}


/************************************************************************
 *	Method:
 *	    IHXErrorMessages::Report
 */
STDMETHODIMP CArchiverContext::Report(const UINT8 unSeverity,  
				      HX_RESULT	ulHXCode,
				      const ULONG32 ulUserCode,
				      const char* pUserString,
				      const char* pMoreInfoURL)
{
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXErrorMessages::GetErrorText
 */
STDMETHODIMP_(IHXBuffer*) CArchiverContext::GetErrorText(HX_RESULT ulHXCode)
{
    return NULL;
}


/************************************************************************
 *  IUnknown methods
 */
STDMETHODIMP CArchiverContext::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*) (IHXErrorMessages*) this;
	return HXR_OK;
    }

    if (m_pContext)
    {
	retVal = m_pContext->QueryInterface(riid, ppvObj);
    }

    if (FAILED(retVal))
    {
	if (IsEqualIID(riid, IID_IHXErrorMessages))
	{
	    AddRef();
	    *ppvObj = (IHXErrorMessages*) this;
	    return HXR_OK;
	}

	*ppvObj = NULL;
    }
 
    return retVal;
}

STDMETHODIMP_(ULONG32) CArchiverContext::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CArchiverContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
