/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxbuffer.cpp,v 1.17 2006/09/14 21:07:06 gwright Exp $
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxbuffer.h"
#include "hxcppflags.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////
// XXX - We can't use pnheap to debug mory leaks...
// because of defining our own new and delete operations.
//
//  #include "hxheap.h"
//  
//  #ifdef _DEBUG
//  #undef HX_THIS_FILE		
//  static const char HX_THIS_FILE[] = __FILE__;
//  #endif
/////////////////////////////////////////

#ifdef PAULM_HXBUFFERSPACE
extern UINT32* volatile g_pCHXBufferTotalData;
void
MoreBufferData(UINT32 ul)
{
    if (!g_pCHXBufferTotalData)
    {
        return;
    }
    (*g_pCHXBufferTotalData) += ul;
}

void
LessBufferData(UINT32 ul)
{
    if (!g_pCHXBufferTotalData)
    {
        return;
    }
    if (ul > (*g_pCHXBufferTotalData))
    {
        (*g_pCHXBufferTotalData) = 0;
    }
    else
    {   
        (*g_pCHXBufferTotalData) -= ul;
    }
}

UINT32
GetBufferDataSize()
{
    if (!g_pCHXBufferTotalData)
        return 0;
    return (*g_pCHXBufferTotalData);
}

#endif

CHXBuffer::CHXBuffer()
    : m_lRefCount(0)
    , m_ulAllocLength(0)
    , m_bJustPointToExistingData(FALSE)
{
    m_BigData.m_pData = NULL;
    m_BigData.m_ulLength = 0;
    m_BigData.m_FreeWithMallocInterfaceIfAvail = TRUE;

    m_ShortData[MaxPnbufShortDataLen] = 0;
}

CHXBuffer::CHXBuffer(UCHAR* pData, UINT32 ulLength, HXBOOL bOwnBuffer)
    : m_lRefCount(0)
    , m_ulAllocLength(ulLength)
    , m_bJustPointToExistingData(FALSE)
{
    m_ShortData[MaxPnbufShortDataLen] = 0;

    if (bOwnBuffer)
    {
	SetWithoutAlloc(pData, ulLength);
    }
    else
    {
	Set(pData, ulLength);
    }
}

CHXBuffer::~CHXBuffer()
{
    // If this buffer just points to data/memory that is actually 'owned' by
    // another buffer, our destructor must not de-allocate that memory.
    if( m_bJustPointToExistingData == TRUE )
    {
	return;
    }

    if (!IsShort() && m_BigData.m_pData)
    {
#ifdef PAULM_HXBUFFERSPACE
        LessBufferData(m_ulAllocLength);
#endif
        Deallocate(m_BigData.m_pData);
        m_ulAllocLength = 0;
    }
}

HXBOOL CHXBuffer::IsShort() const
{
    return m_ShortData[MaxPnbufShortDataLen] != BigDataTag;
}


#if !defined(HELIX_CONFIG_NOSTATICS)
// static member initalization...
IMalloc*	CHXBuffer::m_zMallocInterface = NULL;
#endif


HXBOOL CHXBuffer::FreeWithMallocInterface() const
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    return !IsShort() && 
        m_BigData.m_FreeWithMallocInterfaceIfAvail && 
        m_zMallocInterface != NULL;
#else
    return FALSE;
#endif
}

void CHXBuffer::SetAllocator(IMalloc* pMalloc)
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    if (pMalloc)
    {	
        HX_RELEASE(m_zMallocInterface);
        m_zMallocInterface = pMalloc;
        m_zMallocInterface->AddRef();
    }
#endif
}

void CHXBuffer::ReleaseAllocator()
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    HX_RELEASE(m_zMallocInterface);
#endif
}


/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP CHXBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
            AddRef();
            *ppvObj = this;
            return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXBuffer))
    {
            AddRef();
            *ppvObj = (IHXBuffer*)this;
            return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXBuffer2))
    {
            AddRef();
            *ppvObj = (IHXBuffer2*)this;
            return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) CHXBuffer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) CHXBuffer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *	IHXBuffer methods
 */

/************************************************************************
 *	Method:
 *		IHXBuffer::Get
 *	Purpose:
 *		TBD
 */
STDMETHODIMP CHXBuffer::Get
(
    REF(UCHAR*)		pData, 
    REF(ULONG32)	ulLength
)
{
    // Whenever a COM object is returned out of an
    // interface, it should be AddRef()'d. But this is
    // not a COM object, it is only a buffer...
    if (IsShort())
    {
        pData    = m_ShortData;
        ulLength = m_ShortData[MaxPnbufShortDataLen];
    }
    else
    {
        pData    = m_BigData.m_pData;
        ulLength = m_BigData.m_ulLength;
    }
    return HXR_OK;
}


/************************************************************************
 *	Method:
 *		IHXBuffer::Set
 *	Purpose:
 *		TBD
 */
STDMETHODIMP CHXBuffer::Set
(
    const UCHAR*	pData, 
    ULONG32		ulLength
)
{
    HX_RESULT res = SetSize(ulLength, FALSE); // don't copy data
    if (FAILED(res)) return res;

    HX_ASSERT(GetSize() == ulLength);
    memcpy(GetBuffer(), pData, (ulLength <= GetSize() ? ulLength : GetSize())); /* Flawfinder: ignore */
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *		SetSize
 *	Purpose:
 *		TBD
 */
HX_RESULT CHXBuffer::SetSize(ULONG32 ulLength, HXBOOL copyExistingData)
{
    /* We allow changing the packet info when it is owned
     * by atmost one user.
     */
    if (m_lRefCount > 1)
    {
        HX_ASSERT(!"CHXBuffer::SetSize() will bail out because refcount > 1");
        return HXR_UNEXPECTED;
    }

    if (ulLength <= GetSize())
    {
        if (IsShort()) 
        {
            m_ShortData[MaxPnbufShortDataLen] = (UCHAR)ulLength;
        }
        else
        {
            m_BigData.m_ulLength = ulLength;
        }
        return HXR_OK;
    }

    if (ulLength <= MaxPnbufShortDataLen)
    {
        if (IsShort())
        {
            // New size short, old size short
            // Nothing to do
        }
        else
        {
            // New size short, old size big
            UCHAR temp[MaxPnbufShortDataLen];
            memcpy(temp, m_BigData.m_pData, ulLength); /* Flawfinder: ignore */
            Deallocate(m_BigData.m_pData);
            m_ulAllocLength = 0;
            memcpy(m_ShortData, temp, ulLength); /* Flawfinder: ignore */
        }
        m_ShortData[MaxPnbufShortDataLen] = (UCHAR)ulLength;
    }
    else
    {
        if (IsShort())
        {
            // New size big, old size short

            UCHAR* pNewData = Allocate(ulLength);
            m_ulAllocLength = ulLength;

            // Allocate new data
            // Treat alloc error
            if (!pNewData)
            {
                return HXR_OUTOFMEMORY;
            }

            // copy memory
            if (copyExistingData) 
            {
                memcpy(pNewData, m_ShortData, m_ShortData[MaxPnbufShortDataLen]); /* Flawfinder: ignore */
            }

            // Assign data
            m_BigData.m_pData = pNewData;

            // Force length
            m_BigData.m_ulLength = ulLength;

            // Init extra field
            m_BigData.m_FreeWithMallocInterfaceIfAvail = TRUE;

            // Mark data as big
            m_ShortData[MaxPnbufShortDataLen] = BigDataTag;
        }
        else
        {
            if( ulLength <= m_ulAllocLength )
            {
               m_BigData.m_ulLength = ulLength;
               m_BigData.m_FreeWithMallocInterfaceIfAvail = TRUE;
               return HXR_OK;
            }
            // New size big, old size big
            // Reallocate the data
            UCHAR* pTemp = copyExistingData 
                ? Reallocate(m_BigData.m_pData, m_BigData.m_ulLength, ulLength) 
                : Allocate(ulLength);
            // Treat alloc error
            if (!pTemp)
            {
                return HXR_OUTOFMEMORY;
            }
            m_ulAllocLength = ulLength;
            // Deallocate old memory
            if (!copyExistingData) 
            {
                Deallocate(m_BigData.m_pData);
            }

            // Assign data
            m_BigData.m_pData = pTemp;

            // Force length
            m_BigData.m_ulLength = ulLength;

            // Set flag
            m_BigData.m_FreeWithMallocInterfaceIfAvail = TRUE;
        }
    }
    
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *		IHXBuffer::SetSize
 *	Purpose:
 *		TBD
 */
UCHAR* CHXBuffer::Allocate(UINT32 size) const
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    if (m_zMallocInterface)
    {
        return (UCHAR*) m_zMallocInterface->Alloc(size);
    }
    else
#endif
    {
#ifdef HX_CPP_MALLOC_SUPPORTED
        return (UCHAR*)malloc(size);
#else
        return new UCHAR[size];
#endif
    }
}

/************************************************************************
 *	Method:
 *		IHXBuffer::SetSize
 *	Purpose:
 *		TBD
 */
UCHAR* CHXBuffer::Reallocate(UCHAR* p, UINT32 oldSize, UINT32 newSize) const
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    if (m_zMallocInterface)
    {
        UCHAR* pNewData = (UCHAR*)m_zMallocInterface->Alloc(newSize);
        if (!pNewData) return 0;
        // copy memory
        memcpy(pNewData, p, HX_MIN(oldSize, newSize)); /* Flawfinder: ignore */
        m_zMallocInterface->Free(p);
        return pNewData;
    }
    else
#endif
    {
#ifdef HX_CPP_MALLOC_SUPPORTED
        return (UCHAR*)realloc(p, newSize);
#else
        UCHAR* pNewData = new UCHAR[newSize];
        if (pNewData)
        {
            memcpy(pNewData, p, HX_MIN(oldSize, newSize)); /* Flawfinder: ignore */
            delete[] p;
        }
        return pNewData;
#endif
    }
}

/************************************************************************
 *	Method:
 *		IHXBuffer::SetSize
 *	Purpose:
 *		TBD
 */
void CHXBuffer::Deallocate(UCHAR* p) const
{
#if !defined(HELIX_CONFIG_NOSTATICS)
    if (FreeWithMallocInterface())
    {
        m_zMallocInterface->Free(p);
    }
    else
#endif
    {
#ifdef HX_CPP_MALLOC_SUPPORTED
        free(p);
        p=NULL;
#else
        delete[] p;
        p=NULL;
#endif
    }
}

/************************************************************************
 *	Method:
 *		IHXBuffer::SetSize
 *	Purpose:
 *		TBD
 */
STDMETHODIMP CHXBuffer::SetSize(ULONG32 ulLength)
{
    return SetSize(ulLength, TRUE);
}


/************************************************************************
 *	Method:
 *		IHXBuffer::GetSize
 *	Purpose:
 *		TBD
 */
STDMETHODIMP_(ULONG32) CHXBuffer::GetSize()
{
    return IsShort() ? m_ShortData[MaxPnbufShortDataLen] : m_BigData.m_ulLength;
}

/************************************************************************
 *	Method:
 *		IHXBuffer::GetBuffer
 *	Purpose:
 *		TBD
 */
STDMETHODIMP_(UCHAR*) CHXBuffer::GetBuffer()
{
    // Whenever a COM object is returned out of an
    // interface, it should be AddRef()'d. But this is
    // not a COM object, it is only a buffer...
    return IsShort() ? m_ShortData : m_BigData.m_pData;
}

STDMETHODIMP
CHXBuffer::SetWithoutAlloc(UCHAR*		pData, 
			   ULONG32		ulLength)
{
    m_BigData.m_pData = pData;
    m_BigData.m_ulLength = ulLength;
    m_BigData.m_FreeWithMallocInterfaceIfAvail = FALSE;

    m_ShortData[MaxPnbufShortDataLen] = BigDataTag;

    // This constructor does not actually allocate memory, so we may want
    // to use it in a way that does not deallocate memory when it is deleted.
    m_bJustPointToExistingData = TRUE;

    return HXR_OK;
}

HX_RESULT 
CHXBuffer::FromCharArray
(
    const char* szIn, 
    IHXBuffer** ppbufOut
)
{
    if (!szIn)
    {
        *ppbufOut = NULL;
        return HXR_FAIL;
    }
    return FromCharArray(szIn, strlen(szIn)+1, ppbufOut);
}

HX_RESULT 
CHXBuffer::FromCharArray
(
    const char* szIn, 
    UINT32 ulLength, 
    IHXBuffer** ppbufOut
)
{
    if (!szIn)
    {
        *ppbufOut = NULL;
        return HXR_FAIL;
    }

    (*ppbufOut) = new CHXBuffer;

    if((*ppbufOut))
    {
        (*ppbufOut)->AddRef();

        (*ppbufOut)->Set((const unsigned char*)szIn, ulLength);

        return HXR_OK;
    }

    return HXR_OUTOFMEMORY;
}

