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

#include "hxtypes.h"
#include "hxcom.h"
#include "hlxclib/string.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxresult.h"

#include "ringbuf.h"

CIHXRingBuffer::CIHXRingBuffer(IHXCommonClassFactory *pClassFactory,
                                 UINT32 ulBufSize,
                                 UINT32 ulWrapSize)
    : m_lRefCount(0),
      m_pBuffer(NULL),
      m_pVBufBegin(NULL),
      m_ulBufBegin(0),
      m_ulBufEnd(0),
      m_ulWrite(0),
      m_ulRead(0),
      m_ulBytesWritten(0),
      m_ulBytesRead(0),
      m_ulBufSize(ulBufSize),
      m_ulGuardSize(ulWrapSize),
      m_pIhxBuffer(NULL),
      m_ulError(0)
{
    // Create an IHXBuffer
    pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&m_pIhxBuffer);

    if (m_pIhxBuffer)
    {
        // 32-byte allign buffer
        m_pIhxBuffer->SetSize(ulBufSize + ulWrapSize + 31);
        if (m_pIhxBuffer->GetSize() != ulBufSize + ulWrapSize + 31)
        {
            HX_RELEASE(m_pIhxBuffer);
            m_ulError = HXR_OUTOFMEMORY;
            return;
        }

        m_pBuffer = m_pIhxBuffer->GetBuffer();
        m_pBuffer = (UCHAR*) (((PTR_INT) m_pBuffer + 31) & ~31);
    
        // Init members
        m_pBufBegin = m_pBuffer + m_ulGuardSize;
        m_ulBufEnd = m_ulBufBegin + m_ulBufSize;

        m_pVBufBegin = m_pBufBegin;

        m_pWrite =
        m_pRead = m_pBufBegin;
    }
    else
    {
        m_ulError = HXR_OUTOFMEMORY;
    }
}

CIHXRingBuffer::~CIHXRingBuffer()
{
    // Release the buffer
    HX_RELEASE(m_pIhxBuffer);
}

///////////////////////////////////////////////////////////////////////////////
//	IUnknown methods
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIHXRingBuffer::QueryInterface(REFIID riid,
								             void** ppvObj)
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

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
CIHXRingBuffer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
CIHXRingBuffer::Release()
{
    return InterlockedIncrement(&m_lRefCount);
}
	
///////////////////////////////////////////////////////////////////////////
//	IHXBuffer methods
///////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIHXRingBuffer::Get(REF(UCHAR*) pData, 
                     REF(ULONG32) ulLength)
{
    return m_pIhxBuffer->Get(pData, ulLength);
}

STDMETHODIMP
CIHXRingBuffer::Set(const UCHAR *pData, 
                     ULONG32 ulLength)
{
    return m_pIhxBuffer->Set(pData, ulLength);
}

STDMETHODIMP
CIHXRingBuffer::SetSize(ULONG32 ulLength)
{
    return m_pIhxBuffer->SetSize(ulLength);
}

STDMETHODIMP_(ULONG32) 
CIHXRingBuffer::GetSize()
{
    return m_pIhxBuffer->GetSize();
}

STDMETHODIMP_(UCHAR*)
CIHXRingBuffer::GetBuffer()
{
    return m_pIhxBuffer->GetBuffer();
}

void CIHXRingBuffer::Reset()
{
    m_pVBufBegin = m_pBufBegin;

    m_pWrite =
    m_pRead = m_pBufBegin;

    m_ulBytesWritten =
    m_ulBytesRead = 0;
}

UINT32 CIHXRingBuffer::CopyData(UCHAR *pData, UINT32 ulBytes)
{
    UINT32 ulCopy = HX_MIN(ulBytes, GetFreeBufferSpace());
    UINT32 ulWrap = 0;

    // Handle buffer wrap
    if (m_ulWrite + ulCopy >= m_ulBufEnd)
    {
        ulWrap = m_ulBufEnd - m_ulWrite;

        memcpy(m_pWrite, pData, ulWrap); /* Flawfinder: ignore */

        pData += ulWrap;
        ulCopy -= ulWrap;
        m_ulBytesWritten += ulWrap;
        m_ulWrite = m_ulBufBegin;
    }

    memcpy(m_pWrite, pData, ulCopy); /* Flawfinder: ignore */
    m_ulBytesWritten += ulCopy;
    m_ulWrite += ulCopy;

    // Number of bytes copied to the buffer
    return ulCopy + ulWrap;
}

void CIHXRingBuffer::AdvanceRead(UINT32 ulBytes)
{
    m_ulRead += ulBytes;

    if (m_ulRead >= m_ulBufEnd)
        m_ulRead -= m_ulBufSize;

    m_ulBytesRead += ulBytes;
}

void CIHXRingBuffer::DecrementRead(UINT32 ulBytes)
{
    m_ulRead -= ulBytes;

    if (m_ulRead < m_ulBufBegin)
        m_ulRead += m_ulBufSize;

    m_ulBytesRead -= ulBytes;
}

UCHAR* CIHXRingBuffer::GetReadPointer(UINT32 &ulBytes)
{
    ulBytes = m_ulBufEnd - m_ulRead;
    ulBytes = HX_MIN((UINT32)ulBytes, GetBytesInBuffer());

    return m_pRead;
}

void CIHXRingBuffer::Wrap(UINT32 ulOldBytes)
{
    // Ensures that distance from end of buffer
    // to read from is less that our guard.
    UINT32   ulSize = m_ulBufEnd - m_ulRead + ulOldBytes;

    if (ulSize <= m_ulGuardSize)
    {
        memcpy(m_pBufBegin-ulSize, m_pRead-ulOldBytes, ulSize); /* Flawfinder: ignore */
        m_pRead = m_pBufBegin-ulSize+ulOldBytes;

        m_pVBufBegin = m_pBufBegin-ulSize;
    }
}




