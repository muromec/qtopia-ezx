/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: timebuff.cpp,v 1.7 2007/07/06 20:34:58 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxtbuf.h"
#include "chxpckts.h"
#include "timebuff.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP CHXTimeStampedBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXBuffer), (IHXBuffer*) this },
		{ GET_IIDHANDLE(IID_IHXTimeStampedBuffer), (IHXTimeStampedBuffer*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) CHXTimeStampedBuffer::AddRef()
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
STDMETHODIMP_(ULONG32) CHXTimeStampedBuffer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP_(UINT32) CHXTimeStampedBuffer::GetTimeStamp()
{
    return m_ulTimeStamp;
}

STDMETHODIMP
CHXTimeStampedBuffer::SetTimeStamp(UINT32 ulTimeStamp)
{
    m_ulTimeStamp = ulTimeStamp;

    return HXR_OK;
}



CHXTimeStampedBuffer::~CHXTimeStampedBuffer()
{
    if (m_pData)
    {
	delete [] m_pData;
    }
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
STDMETHODIMP CHXTimeStampedBuffer::Get
(
    REF(UCHAR*)		pData, 
    REF(ULONG32)	ulLength
)
{
    // Whenever a COM object is returned out of an
    // interface, it should be AddRef()'d. But this is
    // not a COM object, it is only a buffer...
    pData    = m_pData;
    ulLength = m_ulLength;
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *		IHXBuffer::Set
 *	Purpose:
 *		TBD
 */
STDMETHODIMP CHXTimeStampedBuffer::Set
(
    const UCHAR*	pData, 
    ULONG32		ulLength
)
{
    /* We allow changing the packet info when it is owned
     * by atmost one user.
     */
    if (m_lRefCount > 1)
    {
	return HXR_UNEXPECTED;
    }

    if (m_pData)
    {
	delete [] m_pData;
    }

    m_pData = new UCHAR[ulLength];

    if (!m_pData)
    {
	return HXR_OUTOFMEMORY;
    }

    memcpy(m_pData,pData,HX_SAFESIZE_T(ulLength)); /* Flawfinder: ignore */
    m_ulLength = ulLength;
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *		IHXBuffer::SetSize
 *	Purpose:
 *		TBD
 */
STDMETHODIMP CHXTimeStampedBuffer::SetSize(ULONG32 ulLength)
{
    /* We allow changing the packet info when it is owned
     * by atmost one user.
     */
    if (m_lRefCount > 1)
    {
	return HXR_UNEXPECTED;
    }

    if (ulLength > m_ulLength)
    {
	UCHAR* pTemp = NULL;
	if (m_pData)
	{
	    pTemp = m_pData;
	}

	m_pData = new UCHAR[ulLength];

	if (!m_pData)
	{
	    m_pData = pTemp;
	    return HXR_OUTOFMEMORY;
	}

	if (pTemp)
	{
	    memcpy(m_pData,pTemp,HX_SAFESIZE_T(m_ulLength)); /* Flawfinder: ignore */
	    delete [] pTemp;
	}
    }
    m_ulLength = ulLength;
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *		IHXBuffer::GetSize
 *	Purpose:
 *		TBD
 */
STDMETHODIMP_(ULONG32) CHXTimeStampedBuffer::GetSize()
{
    return m_ulLength;
}

/************************************************************************
 *	Method:
 *		IHXBuffer::GetBuffer
 *	Purpose:
 *		TBD
 */
STDMETHODIMP_(UCHAR*) CHXTimeStampedBuffer::GetBuffer()
{
    // Whenever a COM object is returned out of an
    // interface, it should be AddRef()'d. But this is
    // not a COM object, it is only a buffer...
    return m_pData;
}

void
CHXTimeStampedBuffer::Pack(IHXTimeStampedBuffer* pTimeStampedBuffer, char* pData,
                           UINT32 ulDataBufSize, UINT32& ulSize)
{
    UINT32	ulValue = 0;
    UINT32	ulBufferSize = 0;
    IHXBuffer*	pBuffer = NULL;

    if (!pTimeStampedBuffer)
    {
	goto cleanup;
    }

    if (HXR_OK != pTimeStampedBuffer->QueryInterface(IID_IHXBuffer, (void**)&pBuffer))
    {
	goto cleanup;
    }

    ulValue = pTimeStampedBuffer->GetTimeStamp();
    ulBufferSize = pBuffer->GetSize();

    // figure out the size 
    if (!pData)
    {
	ulSize = sizeof(UINT32) + ulBufferSize;
    }
    // pack the data
    else
    {
        if (ulDataBufSize >= 4)
        {
            *pData++ = (BYTE) ulValue;
            *pData++ = (BYTE)(ulValue >> 8);
	    *pData++ = (BYTE)(ulValue >> 16);
            *pData++ = (BYTE)(ulValue >> 24);
            ulSize  += 4;
        }

        if (ulBufferSize <= ulDataBufSize - 4)
        {
            memcpy(pData, (char*)pBuffer->GetBuffer(), ulBufferSize); /* Flawfinder: ignore */
            pData += ulBufferSize;
            ulSize += ulBufferSize;
        }
    }
   		
cleanup:

    HX_RELEASE(pBuffer);

    return;
}   
   
void
CHXTimeStampedBuffer::UnPack(IHXTimeStampedBuffer*& pTimeStampedBuffer, char* pData, UINT32 ulSize)
{
    UINT32	    ulValue = 0;
    IHXBuffer*	    pBuffer = NULL;

    pTimeStampedBuffer = NULL;

    if (!pData || !ulSize)
    {
	goto cleanup;
    }

    ulValue = (BYTE)*pData++; ulValue |= (((BYTE)*pData++) << 8);
    ulValue |= (((BYTE)*pData++) << 16); ulValue |= (((BYTE)*pData++) << 24);
    ulSize -= 4;

    if (ulSize)
    {
	pTimeStampedBuffer = new CHXTimeStampedBuffer();
	pTimeStampedBuffer->AddRef();
	
	pTimeStampedBuffer->SetTimeStamp(ulValue);

	pTimeStampedBuffer->QueryInterface(IID_IHXBuffer, (void**)&pBuffer);
	pBuffer->Set((const UCHAR*)pData, ulSize);
    }
	
cleanup:

    HX_RELEASE(pBuffer);

    return;
}
