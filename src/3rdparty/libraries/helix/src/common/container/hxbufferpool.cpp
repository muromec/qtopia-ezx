/* ***** BEGIN LICENSE BLOCK ***** 
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
 * 
 * Patent Notices: This file may contain technology protected by one or 
 * more of the patents listed at www.helixcommunity.org
 * 
 * 1.   The contents of this file, and the files included with this file,
 * are protected by copyright controlled by RealNetworks and its 
 * licensors, and made available by RealNetworks subject to the current 
 * version of the RealNetworks Public Source License (the "RPSL") 
 * available at  * http://www.helixcommunity.org/content/rpsl unless 
 * you have licensed the file under the current version of the 
 * RealNetworks Community Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply.  You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * 2.  Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above.  Please note that RealNetworks and its 
 * licensors disclaim any implied patent license under the GPL.  
 * If you wish to allow use of your version of this file only under 
 * the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting Paragraph 1 above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete Paragraph 1 above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology.  RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.   Copying, including reproducing, storing, 
 * adapting or translating, any or all of this material other than 
 * pursuant to the license terms referred to above requires the prior 
 * written consent of RealNetworks and its licensors
 * 
 * This file, and the files included with this file, is distributed
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT 
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS 
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING 
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
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
#include "pckunpck.h"
#include "hxslist.h"
#define HELIX_FEATURE_LOGLEVEL_NONE // Uncomment to disable logging
#include "hxtlogutil.h"
#include "hxbufferpool.h"

STDMETHODIMP CHXBufferFromPool::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown),  (IUnknown*) (IHXBuffer*) this  },
        { GET_IIDHANDLE(IID_IHXBuffer),             (IHXBuffer*) this  }
    };

    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXBufferFromPool::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXBufferFromPool::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    // Before we destruct, put our contained buffer back into the pool
    if (m_pBufferPoolManager)
    {
        m_pBufferPoolManager->PutBufferInPool(m_pBuffer);
    }

    delete this;
    return 0;
}

STDMETHODIMP CHXBufferFromPool::Get(REF(UCHAR*) rpData, REF(ULONG32) rulLength)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pBuffer && m_ulSizeUsed)
    {
        // Get the buffer from the contained buffer
        rpData = m_pBuffer->GetBuffer();
        // Get the size used from our m_ulSizeUsed member
        rulLength = m_ulSizeUsed;
        // Make sure we have a valid buffer
        if (rpData)
        {
            // Clear the return value
            retVal = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP CHXBufferFromPool::Set(const UCHAR* pData, ULONG32 ulLength)
{
    // We don't expectt that IHXBuffer::Set() will be called on
    // buffers created from the buffer pool.
    HX_ASSERT(FALSE && "IHXBuffer::Set() should not be called on buffers created from CHXBufferFromPool");
    return HXR_UNEXPECTED;
}

STDMETHODIMP CHXBufferFromPool::SetSize(ULONG32 ulLength)
{
    HXLOGL4(HXLOG_GENE, "CHXBufferFromPool[%p]::SetSize(%lu) contained buffer size=%lu",
            this, ulLength, (m_pBuffer ? m_pBuffer->GetSize() : 0));
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pBuffer)
    {
        // Set the return value
        retVal = HXR_FAIL;
        // We can only set the size of the buffer to be less
        // than or equal to the contained buffer's actual size.
        if (ulLength <= m_pBuffer->GetSize())
        {
            // Set the member variable
            m_ulSizeUsed = ulLength;
            // Clear the return value
            retVal = HXR_OK;
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXBufferFromPool::GetSize()
{
    return m_ulSizeUsed;
}

STDMETHODIMP_(UCHAR*) CHXBufferFromPool::GetBuffer()
{
    UCHAR* pRet = NULL;

    if (m_pBuffer)
    {
        pRet = m_pBuffer->GetBuffer();
    }

    return pRet;
}

CHXBufferFromPool::CHXBufferFromPool(CHXBufferPoolManager* pMgr, IHXBuffer* pBuffer, UINT32 ulSizeUsed)
    : m_lRefCount(0)
    , m_pBufferPoolManager(pMgr)
    , m_pBuffer(pBuffer)
    , m_ulSizeUsed(ulSizeUsed)
{
    HXLOGL4(HXLOG_GENE, "CON CHXBufferFromPool(pMgr=%p,pBuffer=%p,ulSizeUsed=%lu) this=%p",
            pMgr, pBuffer, ulSizeUsed, this);
    HX_ASSERT(m_pBufferPoolManager && m_pBuffer && m_ulSizeUsed);
    HX_ADDREF(m_pBufferPoolManager);
    HX_ADDREF(m_pBuffer);
}

CHXBufferFromPool::~CHXBufferFromPool()
{
    HXLOGL4(HXLOG_GENE, "DES CHXBufferFromPool() this=%p", this);
    HX_RELEASE(m_pBufferPoolManager);
    HX_RELEASE(m_pBuffer);
    m_ulSizeUsed = 0;
}

CHXBufferPoolManager::CHXBufferPoolManager()
    : m_lRefCount(0)
    , m_pContext(NULL)
    , m_pFreeList(NULL)
    , m_pFreeListMutex(NULL)
{
    HXLOGL4(HXLOG_GENE, "CON CHXBufferPoolManager this=%p", this);
}

STDMETHODIMP CHXBufferPoolManager::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        {GET_IIDHANDLE(IID_IUnknown), (IUnknown*) this}
    };

    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXBufferPoolManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXBufferPoolManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT CHXBufferPoolManager::Init(IUnknown* pContext)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pContext)
    {
        // Save the context
        HX_RELEASE(m_pContext);
        m_pContext = pContext;
        m_pContext->AddRef();
        // Create the mutex
        HX_RELEASE(m_pFreeListMutex);
        retVal = CreateInstanceCCF(CLSID_IHXMutex, (void**) &m_pFreeListMutex, m_pContext);
        if (SUCCEEDED(retVal))
        {
            // Clear any existing free list
            ClearFreeList();
            // Set the return value
            retVal = HXR_OUTOFMEMORY;
            // Create the free list
            HX_DELETE(m_pFreeList);
            m_pFreeList = new CHXSimpleList();
            if (m_pFreeList)
            {
                // Clear the return value
                retVal = HXR_OK;
            }
        }
    }

    return retVal;
}

HX_RESULT CHXBufferPoolManager::GetBufferFromPool(UINT32 ulSize, REF(IHXBuffer*) rpBuffer)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    HX_ASSERT(ulSize && rpBuffer == NULL);
    if (ulSize)
    {
        // First try and find a buffer from the free list
        IHXBuffer* pBuffer = NULL;
        retVal = FindBufferInPool(ulSize, pBuffer);
        if (FAILED(retVal))
        {
            // We could not find a buffer from the pool, so
            // we will need to create one.
            retVal = CreateSizedBufferCCF(pBuffer, m_pContext, ulSize);
            if (SUCCEEDED(retVal))
            {
                HXLOGL4(HXLOG_GENE, "CHXBufferPoolManager[%p]::GetBufferFromPool(ulSize=%lu,) - CREATED buffer %p",
                        this, ulSize, pBuffer);
            }
        }
        else
        {
            HXLOGL4(HXLOG_GENE, "CHXBufferPoolManager[%p]::GetBufferFromPool(ulSize=%lu,) - FOUND pool buffer %p of size %lu",
                    this, ulSize, pBuffer, pBuffer->GetSize());
        }
        if (SUCCEEDED(retVal))
        {
            // Set the return value
            retVal = HXR_OUTOFMEMORY;
            // Now we need to create a CHXBufferFromPool wrapper object
            // so that when the wrapper is released, we can return
            // it to the pool.
            CHXBufferFromPool* pPoolBuf = new CHXBufferFromPool(this, pBuffer, ulSize);
            if (pPoolBuf)
            {
                // QI the CHXBufferFromPool object for IHXBuffer
                retVal = pPoolBuf->QueryInterface(IID_IHXBuffer, (void**) &rpBuffer);
            }
        }
        HX_RELEASE(pBuffer);
    }

    return retVal;
}

void CHXBufferPoolManager::Close()
{
    // Clear the free list
    ClearFreeList();
    // Delete the free list
    HX_DELETE(m_pFreeList);
    // Release the members
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pFreeListMutex);
}

CHXBufferPoolManager::~CHXBufferPoolManager()
{
    HXLOGL4(HXLOG_GENE, "DES CHXBufferPoolManager this=%p", this);
    Close();
}

HX_RESULT CHXBufferPoolManager::FindBufferInPool(UINT32 ulSize, REF(IHXBuffer*) rpBuffer)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    HX_ASSERT(rpBuffer == NULL);
    if (ulSize)
    {
        // Set the return value
        retVal = HXR_FAIL;
        // Do we he have a free list?
        if (m_pFreeList)
        {
            // Lock the mutex
            if (m_pFreeListMutex)
            {
                m_pFreeListMutex->Lock();
            }
            // Search through the list and find the best match
            UINT32       ulBest  = 0xFFFFFFFF;
            LISTPOSITION posBest = NULL;
            LISTPOSITION posList = m_pFreeList->GetHeadPosition();
            while (posList)
            {
                // Get the next free buffer
                IHXBuffer* pBuffer = (IHXBuffer*) m_pFreeList->GetAt(posList);
                if (pBuffer)
                {
                    // Get the size of this free buffer
                    UINT32 ulBufSize = pBuffer->GetSize();
                    // Is it large enough to meet the request?
                    if (ulBufSize >= ulSize)
                    {
                        // Compute the excess size of this buffer
                        UINT32 ulDiff = ulBufSize - ulSize;
                        // We want to minimize the excess size
                        if (ulDiff < ulBest)
                        {
                            // This is the current best buffer, so save it
                            ulBest  = ulDiff;
                            posBest = posList;
                        }
                    }
                }
                m_pFreeList->GetNext(posList);
            }
            // Did we find any buffers which would meet the size request?
            if (ulBest < 0xFFFFFFFF)
            {
                // Get the best buffer. We don't need to 
                // AddRef() since the list already had a
                // ref on the buffer
                rpBuffer = (IHXBuffer*) m_pFreeList->GetAt(posBest);
                // Remove this list entry
                m_pFreeList->RemoveAt(posBest);
                // Clear the return value
                retVal = HXR_OK;
            }
            // Unlock the mutex
            if (m_pFreeListMutex)
            {
                m_pFreeListMutex->Unlock();
            }
        }
    }

    return retVal;
}

void CHXBufferPoolManager::PutBufferInPool(IHXBuffer* pBuffer)
{
    HXLOGL4(HXLOG_GENE, "CHXBufferPoolManager[%p]::PutBufferInPool(pBuffer=%p) buffer size=%lu",
            this, pBuffer, (pBuffer ? pBuffer->GetSize() : 0));
    if (pBuffer)
    {
        // Lock the mutex
        if (m_pFreeListMutex)
        {
            m_pFreeListMutex->Lock();
        }
        if (m_pFreeList)
        {
            // AddRef the buffer before going on the list
            pBuffer->AddRef();
            // Put the buffer on the list
            m_pFreeList->AddTail((void*) pBuffer);
        }
        // Unlock the mutex
        if (m_pFreeListMutex)
        {
            m_pFreeListMutex->Unlock();
        }
    }
}

void CHXBufferPoolManager::ClearFreeList()
{
    if (m_pFreeList)
    {
        // Lock the mutex
        if (m_pFreeListMutex)
        {
            m_pFreeListMutex->Lock();
        }
        // Remove all the buffers from the free list
        while (m_pFreeList->GetCount() > 0)
        {
            IHXBuffer* pBuffer = (IHXBuffer*) m_pFreeList->RemoveHead();
            HX_RELEASE(pBuffer);
        }
        // Unlock the mutex
        if (m_pFreeListMutex)
        {
            m_pFreeListMutex->Unlock();
        }
    }
}
