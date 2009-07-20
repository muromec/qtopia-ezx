/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: nestbuff.cpp,v 1.7 2007/04/07 21:13:05 atewari Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

// include
#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"

// pnmisc
#include "baseobj.h"

// pxcomlib
#include "nestbuff.h"

// pndebug
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXNestedBuffer::CHXNestedBuffer()
{
    m_lRefCount = 0;
    m_pBuffer   = NULL;
    m_ulOffset  = 0;
    m_ulSize    = 0;
}

CHXNestedBuffer::~CHXNestedBuffer()
{
    HX_RELEASE(m_pBuffer);
}

STDMETHODIMP CHXNestedBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXBuffer), (IUnknown*)(IHXBuffer*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(UINT32) CHXNestedBuffer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32) CHXNestedBuffer::Release()
{
    
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;

    return 0;
}

STDMETHODIMP CHXNestedBuffer::Get(REF(UCHAR*) pData, REF(ULONG32) ulLength)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pBuffer)
    {
        pData    = m_pBuffer->GetBuffer() + m_ulOffset;
        ulLength = m_ulSize;
        retVal   = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXNestedBuffer::Set(const UCHAR* pData, ULONG32 ulLength)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pBuffer && pData && ulLength &&
        m_ulOffset + ulLength <= m_pBuffer->GetSize())
    {
        // Copy the data
        memcpy(m_pBuffer->GetBuffer() + m_ulOffset, /* Flawfinder: ignore */
               pData,
               ulLength);
        // Set the size
        m_ulSize = ulLength;
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXNestedBuffer::SetSize(ULONG32 ulLength)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pBuffer && ulLength &&
        m_ulOffset + ulLength <= m_pBuffer->GetSize())
    {
        m_ulSize = ulLength;
        retVal   = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXNestedBuffer::GetSize()
{
    return m_ulSize;
}

STDMETHODIMP_(UCHAR*) CHXNestedBuffer::GetBuffer()
{
    UCHAR* pRet = NULL;

    if (m_pBuffer)
    {
        pRet = m_pBuffer->GetBuffer() + m_ulOffset;
    }

    return pRet;
}

STDMETHODIMP CHXNestedBuffer::Init(IHXBuffer* pBuffer, UINT32 ulOffset, UINT32 ulSize)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pBuffer && ulSize)
    {
        if (ulOffset + ulSize <= pBuffer->GetSize())
        {
            // Init the members
            HX_RELEASE(m_pBuffer);
            m_pBuffer  = pBuffer;
            m_pBuffer->AddRef();
            m_ulOffset = ulOffset;
            m_ulSize   = ulSize;
            // Clear the return value
            retVal = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT CHXNestedBuffer::CreateObject(CHXNestedBuffer** ppObj)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppObj)
    {
        CHXNestedBuffer* pObj = new CHXNestedBuffer();
        if (pObj)
        {
            *ppObj = pObj;
            retVal = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT CHXNestedBuffer::CreateNestedBuffer(IHXBuffer* pBuffer, UINT32 ulOffset,
                                              UINT32 ulSize, REF(IHXBuffer*) rpNestedBuffer)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pBuffer && ulSize && ulOffset + ulSize <= pBuffer->GetSize())
    {
        CHXNestedBuffer* pNest = new CHXNestedBuffer();
        if (pNest)
        {
            // AddRef the object
            HX_ADDREF(pNest);
            // Init the object
            retVal = pNest->Init(pBuffer, ulOffset, ulSize);
            if (SUCCEEDED(retVal))
            {
                HX_RELEASE(rpNestedBuffer);
                retVal = pNest->QueryInterface(IID_IHXBuffer, (void**) &rpNestedBuffer);
            }
        }
        HX_RELEASE(pNest);
    }

    return retVal;
}
