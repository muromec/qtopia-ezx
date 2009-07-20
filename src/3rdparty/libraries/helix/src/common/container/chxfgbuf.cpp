/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxfgbuf.cpp,v 1.8 2007/07/06 20:34:58 jfinnecy Exp $
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
#include "chxfgbuf.h"
#include "pckunpck.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_INTERFACE_ARRAY_ENUMERATOR(IHXEnumFragmentedBuffer)

BEGIN_INTERFACE_LIST(_CBufferFragment)
    INTERFACE_LIST_ENTRY(IID_IHXBuffer, IHXBuffer)
END_INTERFACE_LIST

BEGIN_INTERFACE_LIST(CHXFragmentedBuffer)
    INTERFACE_LIST_ENTRY(IID_IHXFragmentedBuffer,	IHXFragmentedBuffer)
    INTERFACE_LIST_ENTRY(IID_IHXBuffer,		IHXBuffer)
END_INTERFACE_LIST

/* IHXFragmentedBuffer Methods
 */

STDMETHODIMP 
CHXFragmentedBuffer::_FindFragment(UINT32 ulFindIndex, REF(_CFragment*) pfrgCurrent, REF(UINT32) ulCurrentSize, REF(UINT32) ulCurrentStart)
{
    pfrgCurrent = m_frglstThis.First();

    ulCurrentStart = 0;

    while(pfrgCurrent)
    {
	ulCurrentSize = pfrgCurrent->GetData()->GetSize();
	
	if(ulCurrentStart+ulCurrentSize > ulFindIndex)
	{
	    break;
	}

	ulCurrentStart += ulCurrentSize;

	pfrgCurrent = pfrgCurrent->Next();
    }

    return HXR_OK;
}


STDMETHODIMP
CHXFragmentedBuffer::GetEnumerator(IHXEnumFragmentedBuffer** ppefbNewEnum)
{
    if(!ppefbNewEnum)
        return HXR_POINTER;

    *ppefbNewEnum = NULL;

    IHXBuffer** arrpbufData = NULL;

    if(m_frglstThis.GetTotal())
    {
        _CFragment* pfrgCurrent;
        UINT32 ulIndex;

        arrpbufData = (IHXBuffer**)new IHXBuffer*[m_frglstThis.GetTotal()];

        for 
        (
            pfrgCurrent = m_frglstThis.First(), ulIndex = 0;
            pfrgCurrent;
            pfrgCurrent = pfrgCurrent->Next(), ++ulIndex
        )
        {
            arrpbufData[ulIndex] = pfrgCurrent->GetData();
            (arrpbufData[ulIndex])->AddRef();
        }
    }

    CREATE_INTERFACE_ARRAY_ENUMERATOR
    (
        IHXEnumFragmentedBuffer, 
        arrpbufData,
        m_frglstThis.GetTotal(),
        ppefbNewEnum
    )
    
    
    return HXR_OK;
}


/* Use these to scatter the buffers
 */

STDMETHODIMP 
CHXFragmentedBuffer::Prepend(IHXBuffer* pBufferFrom, UINT32 ulStartFrom, UINT32 ulLengthFrom)
{
    if(!pBufferFrom)
    {
	return HXR_UNEXPECTED;
    }

    m_frglstThis.Insert
    (
	(new _CFragment(m_pContext))->SetData
	(
	    pBufferFrom, 
	    ulStartFrom, 
	    ulLengthFrom
	)
    );

    return HXR_OK;
}

STDMETHODIMP 
CHXFragmentedBuffer::Append(IHXBuffer* pBufferFrom, UINT32 ulStartFrom, UINT32 ulLengthFrom)
{
    if(!pBufferFrom)
    {
	return HXR_UNEXPECTED;
    }

    m_frglstThis.Append
    (
	(new _CFragment(m_pContext))->SetData
	(
	    pBufferFrom, 
	    ulStartFrom, 
	    ulLengthFrom
	)
    );

    return HXR_OK;
}

STDMETHODIMP 
CHXFragmentedBuffer::Insert(IHXBuffer* pBufferFrom, UINT32 ulStartFrom, UINT32 ulLengthFrom, UINT32 ulStartTo)
{
    if(!pBufferFrom)
    {
	return HXR_UNEXPECTED;
    }

    _CFragment* pfrgFirst;
    UINT32 ulTotalFirst=0, ulSizeFirst=0;
    UINT32 ulFirstIndx;

    /* Find First affected fragment
     */
    _FindFragment(ulStartTo, pfrgFirst, ulSizeFirst, ulTotalFirst);

    ulFirstIndx = ulStartTo - ulTotalFirst;

    if(pfrgFirst)
    {
	IHXBuffer* phxbufBuffy;

	phxbufBuffy = pfrgFirst->GetData();
	phxbufBuffy->AddRef();

	/* Reset existing fragment to contain first part of phxbufBuffy
	 */
	pfrgFirst->SetData(phxbufBuffy, 0, ulFirstIndx);

	/* Append new fragment to contain last part of phxbufBuffy (if any)
	 */
	if((ulSizeFirst-ulFirstIndx) > 0)
	{
	    m_frglstThis.Append
	    (
		(
		    (_CFragment*) new _CFragment(m_pContext)
		)
		->SetData
		(
		    phxbufBuffy, 
		    ulFirstIndx, 
		    UINT32(-1)
		)
		, pfrgFirst
	    );
	}
	
	HX_RELEASE(phxbufBuffy);
    }
    else
    {
	/* The existing buffers are not big enough
	 * Need to make an empty Buffer sizeof ulFirstIndx-1
	 */
	IHXBuffer* phxbufTemp = NULL;	
	if (HXR_OK == CreateBufferCCF(phxbufTemp, m_pContext))
	{
	    phxbufTemp->SetSize(ulFirstIndx-1);
	
	    pfrgFirst = new _CFragment(m_pContext);
	    pfrgFirst->SetData(phxbufTemp);
	
	    m_frglstThis.Append(pfrgFirst);

	    HX_RELEASE(phxbufTemp);
	}
    }

    _CFragment* pfrgNew;

    /* Make new fragment to hold the new data
     */
    pfrgNew = new _CFragment(m_pContext);

    /* Set the new data into the fragment
     */
    pfrgNew->SetData(pBufferFrom, ulStartFrom, ulLengthFrom);

    /* Append the New fragment
     */
    m_frglstThis.Append(pfrgNew, pfrgFirst);

    return HXR_OK;
}

STDMETHODIMP 
CHXFragmentedBuffer::Replace(IHXBuffer* pBufferFrom, UINT32 ulStartFrom, UINT32 ulLengthFrom, UINT32 ulStartTo)
{
    if(!pBufferFrom)
    {
	return HXR_UNEXPECTED;
    }

    _CFragment* pfrgFirst;
    _CFragment* pfrgLast;
    _CFragment* pfrgTmp;
    UINT32 ulTotalFirst=0, ulSizeFirst=0, ulTotalLast=0, ulSizeLast=0;
    UINT32 ulFirstIndx, ulLastIndx;

    /* Find First affected fragment
     */
    _FindFragment(ulStartTo, pfrgFirst, ulSizeFirst, ulTotalFirst);

    ulFirstIndx = ulStartTo - ulTotalFirst;

    /* Find Last affected fragment
     * search for ulStartTo+ulLengthFrom
     * Release any buffers contained in that byte range.
     */
    pfrgLast = pfrgFirst;
    ulTotalLast = 0;
    while (pfrgLast)
    {
	ulSizeLast = pfrgLast->GetData()->GetSize();

	if(ulLengthFrom <= ulTotalLast+ulSizeLast)
	{
	    break;
	}

	ulTotalLast += ulSizeLast;

	pfrgTmp = pfrgLast;

	pfrgLast = pfrgLast->Next();

	if (pfrgTmp != pfrgFirst)
	{
	    m_frglstThis.Remove(pfrgTmp);
	}
    }

    if(pfrgFirst == pfrgLast)
	ulLastIndx = ulFirstIndx + ulLengthFrom - 1;
    else
	ulLastIndx = ulLengthFrom - ulTotalLast;

    if(pfrgFirst)
    {
	IHXBuffer* phxbufBuffy;

	phxbufBuffy = pfrgFirst->GetData();
	phxbufBuffy->AddRef();

	/* Reset existing fragment to contain first part of phxbufBuffy
	 */
	if(ulFirstIndx)
	{
	    pfrgFirst->SetData(phxbufBuffy, 0, ulFirstIndx);
	}
	else
	{
	    if(pfrgFirst == pfrgLast)
	    {
		pfrgFirst = NULL;
	    }
	    else
	    {
		/* Remove pfrgFirst..
		 */

		pfrgTmp = pfrgFirst->Prev();
		
		m_frglstThis.Remove(pfrgFirst);

		pfrgFirst = pfrgTmp;
	    }
	}

	if(pfrgLast)
	{
	    if (pfrgLast == pfrgFirst)
	    {
		/* Need to create new fragment. 
		 */
		pfrgLast = new _CFragment(m_pContext);
		
		m_frglstThis.Append(pfrgLast, pfrgFirst);
	    }
	    else
	    {
		HX_RELEASE(phxbufBuffy);

		/* Reuse existing fragment.
		 */
		(phxbufBuffy = pfrgLast->GetData())->AddRef();
	    }

	    if(ulLastIndx < phxbufBuffy->GetSize())
	    {
		/* Need to fragment the Last Fragment. :)
		 */
		pfrgLast->SetData(phxbufBuffy, ulLastIndx+1, UINT32(-1));
	    }
	    else
	    {
		/* Need to discard the Last Fragment.
		 */
		pfrgTmp = pfrgLast->Prev();

		m_frglstThis.Remove(pfrgLast);

		pfrgLast = pfrgTmp;
	    }
	}
	
	HX_RELEASE(phxbufBuffy);
    }
    else if (ulFirstIndx)
    {
	/* The existing buffers are not big enough
	 * Need to make an empty Buffer sizeof ulFirstIndx-1
	 */
	IHXBuffer *phxbufTemp = NULL;
	if (HXR_OK == CreateBufferCCF(phxbufTemp, m_pContext))
	{
	    phxbufTemp->SetSize(ulFirstIndx-1);
    	
	    pfrgFirst = new _CFragment(m_pContext);
	    pfrgFirst->SetData(phxbufTemp);
    	
	    m_frglstThis.Append(pfrgFirst);

	    HX_RELEASE(phxbufTemp);
	}
    }

    _CFragment* pfrgNew;

    /* Make new fragment to hold the new data
     */
    pfrgNew = new _CFragment(m_pContext);

    /* Set the new data into the fragment
     */
    pfrgNew->SetData(pBufferFrom, ulStartFrom, ulLengthFrom);

    /* Append the New fragment
     */
    if (pfrgFirst)
    {
	m_frglstThis.Append(pfrgNew, pfrgFirst);
    }
    else
    {
	m_frglstThis.Insert(pfrgNew, pfrgLast);
    }

    return HXR_OK;
}

/* These will gather and return the specified area
 */

STDMETHODIMP 
CHXFragmentedBuffer::Get(UINT32 ulStartFrom, UINT32 ulLengthFrom, REF(UCHAR*) pData, REF(UINT32) ulLength)
{
    /* Gather from start to start+length
     */
    
    pData = NULL;
    ulLength = 0;
    
    _CFragment* pfrgFirst=NULL;
    _CFragment* pfrgCurrent=NULL;
    IHXBuffer* phxbufBuffy=NULL;
    UINT32 ulTotalFirst=0, ulSizeFirst=0;
    UINT32 ulFirstStartIndex;

    if (ulLengthFrom == (UINT32)(-1))
    {
	ulLengthFrom = GetSize() - ulStartFrom;
    }

    /* Find First fragment
     */
    _FindFragment(ulStartFrom, pfrgFirst, ulSizeFirst, ulTotalFirst);

    ulFirstStartIndex = ulStartFrom-ulTotalFirst;
    
    if (pfrgFirst)
    {
	/* If the range is in one fragment, then just return that fragment.
	 */
	if(pfrgFirst->GetData()->GetSize() >= ulFirstStartIndex+ulLengthFrom)
	{
	    (phxbufBuffy = pfrgFirst->GetData())->AddRef();
	}
	else
	{
	    UCHAR* pucBuffy;
	    UINT32 ulStartIndex;
	    UINT32 ulSize;
	    UINT32 ulRemainingLength;

	    CreateBufferCCF(phxbufBuffy, m_pContext);
	    phxbufBuffy->SetSize(ulLengthFrom);
	    pucBuffy = phxbufBuffy->GetBuffer();

	    ulStartIndex = ulFirstStartIndex;
	    ulSize = ulSizeFirst;
	    ulRemainingLength = ulLengthFrom;
	    pfrgCurrent = pfrgFirst;
	    while(pfrgCurrent && ulRemainingLength > 0)
	    {
		_RecursiveBufferCopy
		(
		    pucBuffy+(ulLengthFrom-ulRemainingLength),
		    pfrgCurrent->GetData(),
		    ulStartIndex,
		    ulSize
		);

		ulRemainingLength -= ulSize-ulStartIndex;

		pfrgCurrent = pfrgCurrent->Next();

		if (pfrgCurrent)
		{
		    ulSize = pfrgCurrent->GetData()->GetSize();

		    if (ulRemainingLength < ulSize)
		    {
			ulSize = ulRemainingLength;
		    }
		}

		ulStartIndex = 0;
	    }
	    
	    Replace(phxbufBuffy, 0, ulLengthFrom, ulStartFrom);

	    /* This forces the correct offset below..
	     */
	    ulFirstStartIndex = 0;
	}
    }

    if(phxbufBuffy)
    {
	phxbufBuffy->Get(pData, ulLength);
    
	pData = pData+ulFirstStartIndex;
	ulLength = ulLengthFrom;
    }

    HX_RELEASE(phxbufBuffy);

    return HXR_OK;
}

void 
CHXFragmentedBuffer::_RecursiveBufferCopy
(
    UCHAR* pucDestBuffer,
    IHXBuffer* pbufSource,
    UINT32 ulStartIndex,
    UINT32 ulSize
)
{
    IHXFragmentedBuffer* pfgbufCurrent = NULL;

    /* Optimized by directly enumerating nested 
     * fragmented buffers.
     * (instead of "pfrgCurrent->GetData()->GetBuffer()")
     */
    if (!(pbufSource->QueryInterface(IID_IHXFragmentedBuffer, (void**)&pfgbufCurrent)) || pfgbufCurrent)
    {
	IHXEnumFragmentedBuffer* pefbCurrent = NULL;
	IHXBuffer* pbufCurrent = NULL;
	UINT32 ulSizeCurrent=0;
	UINT32 ulCopied=0;
	UINT32 ulTotal=0;

	pfgbufCurrent->GetEnumerator(&pefbCurrent);

	pefbCurrent->Reset();
	
	while(!(pefbCurrent->Next(1, &pbufCurrent, NULL)) && ulSize>0)
	{
	    ulSizeCurrent =  pbufCurrent->GetSize();
	    
	    if ((ulTotal+ulSizeCurrent) < ulStartIndex)
	    {
		ulTotal += ulSizeCurrent;
	    }
	    else
	    {
		_RecursiveBufferCopy
		(
		    pucDestBuffer+ulCopied,
		    pbufCurrent,
		    ulStartIndex-ulTotal,
		    HX_MIN(ulSize, ulSizeCurrent)
		);

		ulSize -= ulSizeCurrent;
		ulCopied += ulSizeCurrent;
		ulStartIndex = 0;
		ulTotal=0;
	    }

	    HX_RELEASE(pbufCurrent);
	}
	
	HX_RELEASE(pefbCurrent);
	HX_RELEASE(pfgbufCurrent);
    }
    else
    {
	memcpy /* Flawfinder: ignore */
	(
	    pucDestBuffer, 
	    pbufSource->GetBuffer()+ulStartIndex, 
	    HX_SAFESIZE_T(ulSize-ulStartIndex)
	);
    }
}

STDMETHODIMP_(UCHAR*) 
CHXFragmentedBuffer::GetBuffer(UINT32 ulStartFrom, UINT32 ulLengthFrom)
{
    UCHAR* pData;
    UINT32 ulLength;

    /* Gather from start to start+length
     */
    Get(ulStartFrom, ulLengthFrom, pData, ulLength);

    return pData;
}


/* IHXBuffer Methods
 */ 

/* XXX I am not doing memcopies into existing space as I cannot guarantee write access.
 * An acceptable Optimization is to try to memcopy, and fragment only if that fails.
 */
STDMETHODIMP 
CHXFragmentedBuffer::Set(const UCHAR* pData, ULONG32 ulLength)
{
    /* Need to make an empty Buffer sizeof ulLength
     */
    IHXBuffer* phxbufTemp = NULL;    	
    if (HXR_OK == CreateBufferCCF(phxbufTemp, m_pContext))
    {
	phxbufTemp->SetSize(ulLength);
	phxbufTemp->Set(pData, ulLength);
        
	Replace(phxbufTemp, 0, ulLength, 0);

	HX_RELEASE(phxbufTemp);

	return HXR_OK;
    }
    else
    {
	return HXR_OUTOFMEMORY;
    }
}

STDMETHODIMP 
CHXFragmentedBuffer::Get(REF(UCHAR*) pData, REF(ULONG32) ulLength)
{
    return Get(0,UINT32(-1), pData, ulLength);
}

/* adds/removes fragment(s) to the end to set the size to the requested length
 */
STDMETHODIMP 
CHXFragmentedBuffer::SetSize(ULONG32 ulLength)
{
    UINT32 ulSize;

    ulSize = GetSize();
    
    if (ulLength > ulSize)
    {
	/* The existing buffers are not big enough
	 * Need to make an empty Buffer sizeof ulLength-GetSize()
	 */
	IHXBuffer* phxbufTemp = NULL;
	
	if (HXR_OK == CreateBufferCCF(phxbufTemp, m_pContext))
	{
	    _CFragment* pfrgNew;

	    phxbufTemp->SetSize(ulLength-ulSize);
    	
	    pfrgNew = new _CFragment(m_pContext);
	    pfrgNew->SetData(phxbufTemp);
    	
	    m_frglstThis.Append(pfrgNew);

	    HX_RELEASE(phxbufTemp);
	}
    }
    else if (ulLength < ulSize)
    {
	_CFragment* pfrgFirst;
	_CFragment* pfrgCurrent;
	_CFragment* pfrgTmp;
	UINT32 ulTotalFirst=0, ulSizeFirst=0;

	/* Find First affected fragment
	 */
	_FindFragment(ulLength, pfrgFirst, ulSizeFirst, ulTotalFirst);

	if(pfrgFirst)
	{
	    IHXBuffer* phxbufBuffy;

	    pfrgCurrent = pfrgFirst->Next();

	    phxbufBuffy = pfrgFirst->GetData();
	    phxbufBuffy->AddRef();

	    if(ulLength-ulTotalFirst > 0)
	    {
		/* Reset existing fragment to contain first part of phxbufBuffy
		 */
		pfrgFirst->SetData(phxbufBuffy, 0, ulLength-ulTotalFirst);
	    }
	    else
	    {
		m_frglstThis.Remove(pfrgFirst);
	    }

	    HX_RELEASE(phxbufBuffy);

	    /* Remove the truncated items.
	     */
	    while(pfrgCurrent)
	    {
		pfrgTmp = pfrgCurrent;
		pfrgCurrent = pfrgCurrent->Next();
		m_frglstThis.Remove(pfrgTmp);
	    }
	}
    }

    return HXR_OK;
}

/* returns the sum of all the fragments sizes
 */
STDMETHODIMP_(ULONG32) 
CHXFragmentedBuffer::GetSize()
{
    _CFragment* pfrgCurrent;
    UINT32 ulTotal = 0;

    pfrgCurrent = m_frglstThis.First();

    while (pfrgCurrent)
    {
	ulTotal += pfrgCurrent->GetData()->GetSize();

	pfrgCurrent = pfrgCurrent->Next();
    }

    return ulTotal;
}

STDMETHODIMP_(UCHAR*) 
CHXFragmentedBuffer::GetBuffer()
{
    return GetBuffer(0,UINT32(-1));
}

CHXFragmentedBuffer::_CFragment* 
CHXFragmentedBuffer::_CFragment::SetData(IHXBuffer* pData)
{
    HX_RELEASE(m_pData); 
    m_pData=pData; 
    m_pData->AddRef(); 
    return this;
}

CHXFragmentedBuffer::_CFragment* 
CHXFragmentedBuffer::_CFragment::SetData(IHXBuffer* pData, UINT32 ulStartFrom, UINT32 ulLengthFrom)
{
    HX_RELEASE(m_pData); 
    
    if(!ulStartFrom && ulLengthFrom >= pData->GetSize())
    {
	/* Use whole Buffer
	 */
	m_pData=pData;
        m_pData->AddRef();
    }
    else
    {
	/* Use Part of Buffer
	 */
	_CBufferFragment* pBufferFragment = new _CBufferFragment(m_pContext);
	if (pBufferFragment)
	{
	    pBufferFragment->_SetBuffer
	    (
		pData, 
		ulStartFrom, 
		ulLengthFrom
	    )
	    ->QueryInterface
	    (
		IID_IHXBuffer,
		(void**)&m_pData
	    );
	}
    }

    return this;
}

CHXFragmentedBuffer::_CFragment* 
CHXFragmentedBuffer::_CFragment::Insert(_CFragment* pNewPrev)
{
    if(pNewPrev == m_pPrev)
    {
	return this;
    }

    if(m_pPrev)
    {
	m_pPrev->_SetNext(pNewPrev);
    }

    if(pNewPrev)
    {
	pNewPrev->_SetNext(this);
	pNewPrev->_SetPrev(m_pPrev);
    }

    _SetPrev(pNewPrev);

    return pNewPrev;
}

CHXFragmentedBuffer::_CFragment* 
CHXFragmentedBuffer::_CFragment::Append(_CFragment* pNewNext)
{
    if(pNewNext == m_pNext)
    {
	return this;
    }

    if(m_pNext)
    {
	m_pNext->_SetPrev(pNewNext);
    }

    if(pNewNext)
    {
	pNewNext->_SetPrev(this);
	pNewNext->_SetNext(m_pNext);
    }

    _SetNext(pNewNext);
    
    return pNewNext;
}

CHXFragmentedBuffer::_CFragment* 
CHXFragmentedBuffer::_CFragment::Remove()
{
    _CFragment* pfrgRet;
    
    if(m_pNext)
    {
	pfrgRet = m_pNext;
    }
    else 
    {
	pfrgRet = m_pPrev;
    }
    
    delete this;

    return pfrgRet;
}

void 
CHXFragmentedBuffer::_CFragmentList::Remove(_CFragment* pfrgObsolete)
{
    if(pfrgObsolete)
    {
	if (pfrgObsolete == m_pfrgListEnd)
	{
	    m_pfrgListEnd = pfrgObsolete->Prev();
	}
	if (pfrgObsolete == m_pfrgListStart)
	{
	    m_pfrgListStart = pfrgObsolete->Next();
	}

	pfrgObsolete->Remove();
        --m_ulTotal;
    }
}

void 
CHXFragmentedBuffer::_CFragmentList::Insert(_CFragment* pfrgNew, _CFragment* pfrgRelative)
{
    if (!pfrgNew)
    {
	return;
    }

    if(pfrgRelative)
    {
	pfrgRelative->Insert(pfrgNew);

	if (pfrgRelative == m_pfrgListStart)
	{
	    m_pfrgListStart = pfrgNew;
	}
    }
    else if (m_pfrgListStart)
    {
	m_pfrgListStart->Insert(pfrgNew);
	m_pfrgListStart = pfrgNew;
    }
    else
    {
	m_pfrgListStart = m_pfrgListEnd = pfrgNew;
    }

    ++m_ulTotal;
}

void 
CHXFragmentedBuffer::_CFragmentList::Append(_CFragment* pfrgNew, _CFragment* pfrgRelative)
{
    if (!pfrgNew)
    {
	return;
    }

    if(pfrgRelative)
    {
	pfrgRelative->Append(pfrgNew);

	if (pfrgRelative == m_pfrgListEnd)
	{
	    m_pfrgListEnd = pfrgNew;
	}
    }
    else if (m_pfrgListEnd)
    {
	m_pfrgListEnd->Append(pfrgNew);
	m_pfrgListEnd = pfrgNew;
    }
    else
    {
	m_pfrgListStart = m_pfrgListEnd = pfrgNew;
    }

    ++m_ulTotal;
}

_CBufferFragment* 
_CBufferFragment::_SetBuffer(IHXBuffer* pData, UINT32 ulStart, UINT32 ulLength)
{
    HX_RELEASE(m_pData); 

    m_pData = pData; 
    
    if(m_pData)
    {
	m_pData->AddRef();

	m_ulStart=ulStart;

	if(m_pData->GetSize()-ulStart < ulLength)
	{
	    m_ulLength=m_pData->GetSize()-ulStart;
	}
	else
	{
	    m_ulLength=ulLength;
	}
    }
    else
    {
	m_ulStart=0;
	m_ulLength=0;
    }

    return this;
}

STDMETHODIMP 
_CBufferFragment::Set(const UCHAR* pData, UINT32 ulLength)
{
    if(!m_pData)
    {
	return HXR_UNEXPECTED;
    }

    return m_pData->Set(pData+m_ulStart, ulLength);
}

STDMETHODIMP
_CBufferFragment::Get(REF(UCHAR*) pData, REF(UINT32) ulLength)
{
    if(!m_pData)
    {
	return HXR_UNEXPECTED;
    }

    if(ulLength > m_ulLength)
    {
	return HXR_INVALID_PARAMETER;
    }

    pData = GetBuffer();
    ulLength = GetSize();
    
    return HXR_OK;
}

STDMETHODIMP
_CBufferFragment::SetSize(UINT32 ulLength)
{
    HX_RESULT hxrRet = HXR_FAIL;

    if(!m_pData)
    {
	/* Make New Buffer
	 */
	CreateBufferCCF(m_pData, m_pContext);
	m_ulStart = 0;
    }
    
    hxrRet = m_pData->SetSize(ulLength+m_ulStart);
    m_ulLength = ulLength;
    
    return hxrRet;
}

STDMETHODIMP_(UINT32)
_CBufferFragment::GetSize()
{
    if(!m_pData)
    {
	return 0;
    }

    return HX_MIN(m_pData->GetSize()-m_ulStart, m_ulLength);
}

STDMETHODIMP_(UCHAR*)
_CBufferFragment::GetBuffer()
{
    if(!m_pData)
    {
	return NULL;
    }

    return m_pData->GetBuffer()?m_pData->GetBuffer()+m_ulStart:NULL;
}


