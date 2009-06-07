/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servbuffer.cpp,v 1.3 2003/09/04 22:35:33 dcollins Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "servbuffer.h"
#include "rtsputil.h"

#ifdef PAULM_HXBUFFERSPACE
extern void MoreBufferData(UINT32 ul);
extern void LessBufferData(UINT32 ul);
extern UINT32 GetBufferDataSize();
#endif

ServerBuffer::ServerBuffer()
    : m_ulRefCount(0)
    , m_pData(NULL)
    , m_ulLength(0)
    , m_ulAllocLength(0)
    , m_bPassesInBuffer(FALSE)
{
}

ServerBuffer::ServerBuffer(BOOL bAlreadyHasOneRef)
    : m_ulRefCount(bAlreadyHasOneRef ? 1 : 0)
    , m_pData(NULL)
    , m_ulLength(0)
    , m_ulAllocLength(0)
    , m_bPassesInBuffer(FALSE)
{
}

ServerBuffer::ServerBuffer(UCHAR* pData, UINT32 ulLength)
    : m_ulRefCount(0)
    , m_pData(pData)
    , m_ulLength(ulLength)
    , m_ulAllocLength(ulLength)
    , m_bPassesInBuffer(TRUE)
{
}

ServerBuffer::~ServerBuffer()
{
    if (m_pData)
    {
#ifdef PAULM_HXBUFFERSPACE
	LessBufferData(m_ulAllocLength);
#endif
	delete[] m_pData;
    }
}

STDMETHODIMP
ServerBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXBuffer))
    {
	    AddRef();
	    *ppvObj = (IHXBuffer*)this;
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


STDMETHODIMP_(ULONG32)
ServerBuffer::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(ULONG32)
ServerBuffer::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
ServerBuffer::Get(REF(UCHAR*)	pData, 
	       REF(ULONG32)	ulLength)
{
    pData    = m_pData;
    ulLength = m_ulLength;

    return HXR_OK;
}


STDMETHODIMP ServerBuffer::Set(const UCHAR*	pData, 
			    ULONG32		ulLength)
{
    /*
     * We allow changing the packet info when it is owned
     * by at most one user.
     */
    if (m_ulRefCount > 1)
    {
	return HXR_UNEXPECTED;
    }

    if (m_pData)
    {
#ifdef PAULM_HXBUFFERSPACE
	LessBufferData(m_ulAllocLength);
#endif
	delete[] m_pData;
	m_pData = NULL;
    }

    if (m_pData == NULL)
    {
#ifdef PAULM_HXBUFFERSPACE
	MoreBufferData(ulLength);
#endif
	m_pData = new UCHAR[ulLength];
	m_ulAllocLength = ulLength;
    }
    
    if (!m_pData)
    {
	m_ulAllocLength = 0;
	return HXR_OUTOFMEMORY;
    }

    memcpy(m_pData, pData, ulLength);
    m_ulLength = ulLength;

    return HXR_OK;
}


STDMETHODIMP
ServerBuffer::SetSize(ULONG32 ulLength)
{
    /*
     * We allow changing the packet info when it is owned
     * by at most one user.
     */
    if (m_ulRefCount > 1)
    {
	return HXR_UNEXPECTED;
    }

    if (ulLength <= m_ulAllocLength)
    {
        m_ulLength = ulLength;

	return HXR_OK;
    }
    
    UCHAR* pTemp = m_pData;
	
    m_pData = new UCHAR[ulLength];

    if (pTemp)
    {
	memcpy(m_pData, pTemp, m_ulLength);
	delete[] pTemp;
    }

#ifdef PAULM_HXBUFFERSPACE
    LessBufferData(m_ulLength);
    MoreBufferData(ulLength);
#endif

    m_ulAllocLength = ulLength;
    m_ulLength = ulLength;

    return HXR_OK;
}


STDMETHODIMP_(ULONG32)
ServerBuffer::GetSize()
{
    return m_ulLength;
}


STDMETHODIMP_(UCHAR*)
ServerBuffer::GetBuffer()
{
    return m_pData;
}

HX_RESULT 
ServerBuffer::FromCharArray(const char* pIn, IHXBuffer** ppbufOut)
{
    if (pIn == NULL)
    {
	*ppbufOut = NULL;

	return HXR_FAIL;
    }
    return FromCharArray(pIn, strlen(pIn)+1, ppbufOut);
}

HX_RESULT 
ServerBuffer::FromCharArray(const char* pIn, UINT32 ulLength, 
    IHXBuffer** ppbufOut)
{
    if (pIn == NULL)
    {
	*ppbufOut = NULL;

	return HXR_FAIL;
    }

    (*ppbufOut) = new ServerBuffer(TRUE);
    (*ppbufOut)->Set((const unsigned char*)pIn, ulLength);

    return HXR_OK;
}
