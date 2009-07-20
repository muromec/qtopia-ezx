/* ***** Begin LICENSE BLOCK *****
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

#include "hxcom.h"
#include "hxassert.h"
#include "ihxpckts.h"
#include "nettypes.h"
#include "hxnet.h"
#include "writequeue.h"

#define SIZE_TO_LEN(n) (HX_IOV_MAX*(((n)+MAX_IP_PACKET-1)/MAX_IP_PACKET))

#define ADVANCE(pos,n) { pos = (pos+n)%m_uQueueLen; }

CHXWriteQueue::CHXWriteQueue(void) :
    m_uHeadPos(0),
    m_uTailPos(0),
    m_uQueuedBytes(0),
    m_uQueueUsed(0),
    m_uQueueLen(0),
    m_ppBufQueue(NULL)
{
    // Empty
}

CHXWriteQueue::~CHXWriteQueue(void)
{
    while (m_uQueueUsed > 0)
    {
        HX_RELEASE(m_ppBufQueue[m_uHeadPos]);
        ADVANCE(m_uHeadPos, 1);
        m_uQueueUsed--;
    }
    delete[] m_ppBufQueue;
}

HX_RESULT
CHXWriteQueue::SetSize(UINT32 uSize)
{
    UINT32 uLen = SIZE_TO_LEN(uSize);
    IHXBuffer** ppNewBufQueue;

    //XXXTDM: Support resizing with queued data?
    if (m_uQueueUsed != 0)
    {
        return HXR_UNEXPECTED;
    }

    ppNewBufQueue = new IHXBuffer*[uLen];
    if (ppNewBufQueue == NULL)
    {
        return HXR_OUTOFMEMORY;
    }
    delete[] m_ppBufQueue;
    m_uHeadPos = 0;
    m_uTailPos = 0;
    m_uQueuedBytes = 0;
    m_uQueueUsed = 0;
    m_uQueueLen = uLen;
    m_ppBufQueue = ppNewBufQueue;
    return HXR_OK;
}

void
CHXWriteQueue::Discard(void)
{
    while (m_uQueueUsed > 0)
    {
        HX_RELEASE(m_ppBufQueue[m_uHeadPos]);
        ADVANCE(m_uHeadPos, 1);
        m_uQueueUsed--;
    }
}

CHXStreamWriteQueue::CHXStreamWriteQueue(void) :
    CHXWriteQueue(),
    m_uOffset(0)
{
    // Empty
}

CHXStreamWriteQueue::~CHXStreamWriteQueue(void)
{
    // Empty
}

HX_RESULT
CHXStreamWriteQueue::Enqueue(UINT32 nVecLen, IHXBuffer** ppBufVec,
                IHXSockAddr* pAddr)
{
    HX_ASSERT(pAddr == NULL);
    if (m_uQueueLen-m_uQueueUsed < nVecLen)
    {
        return HXR_SOCK_WOULDBLOCK;
    }
    while (nVecLen)
    {
        m_ppBufQueue[m_uTailPos] = *ppBufVec;
        m_ppBufQueue[m_uTailPos]->AddRef();
        m_uQueuedBytes += (*ppBufVec)->GetSize();
        ADVANCE(m_uTailPos,1)
        m_uQueueUsed++;
        nVecLen--;
        ppBufVec++;
    }
    return HXR_OK;
}

void
CHXStreamWriteQueue::FillVector(UINT32* pvlen, hx_iov* piov, IHXSockAddr** ppAddr)
{
    UINT32 len = 0;
    UINT32 pos = m_uHeadPos;
    memset(piov, 0, HX_IOV_MAX*sizeof(hx_iov));
    piov->set_len(m_ppBufQueue[pos]->GetSize() - m_uOffset);
    piov->set_buf(m_ppBufQueue[pos]->GetBuffer() + m_uOffset);
    ADVANCE(pos, 1)
    len++;
    piov++;
    while (len < HX_IOV_MAX && pos != m_uTailPos)
    {
        piov->set_len(m_ppBufQueue[pos]->GetSize());
        piov->set_buf(m_ppBufQueue[pos]->GetBuffer());
        ADVANCE(pos, 1)
        len++;
        piov++;
    }
    *ppAddr = NULL;
    *pvlen = len;
}

HX_RESULT
CHXStreamWriteQueue::Dequeue(UINT32 uSize)
{
    HX_ASSERT(m_uQueueUsed >= 1);
    HX_ASSERT(m_uQueuedBytes >= uSize);
    m_uQueuedBytes -= uSize;

    IHXBuffer* pBuf = m_ppBufQueue[m_uHeadPos];
    UINT32 uBufLen = pBuf->GetSize() - m_uOffset;
    if (uBufLen > uSize)
    {
        m_uOffset += uSize;
        return HXR_OK;
    }
    pBuf->Release();
    uSize -= uBufLen;
    ADVANCE(m_uHeadPos, 1)
    m_uQueueUsed--;

    while (uSize)
    {
        HX_ASSERT(m_uQueueUsed > 0);
        pBuf = m_ppBufQueue[m_uHeadPos];
        uBufLen = pBuf->GetSize();
        if (uBufLen > uSize)
        {
            break;
        }
        pBuf->Release();
        uSize -= uBufLen;
        ADVANCE(m_uHeadPos, 1)
        m_uQueueUsed--;
    }
    m_uOffset = uSize;
    return HXR_OK;
}

CHXDatagramWriteQueue::CHXDatagramWriteQueue(void) :
    CHXWriteQueue(),
    m_ppAddrQueue(NULL)
{
    // Empty
}

CHXDatagramWriteQueue::~CHXDatagramWriteQueue(void)
{
    if (m_uQueueUsed > 0)
    {
        UINT32 pos = m_uHeadPos;
        while (pos != m_uTailPos)
        {
            HX_RELEASE(m_ppAddrQueue[pos]);
            ADVANCE(pos, 1);
        }
    }
    delete[] m_ppAddrQueue;
}

HX_RESULT
CHXDatagramWriteQueue::SetSize(UINT32 uSize)
{
    UINT32 uLen = SIZE_TO_LEN(uSize);
    IHXBuffer** ppNewBufQueue;
    IHXSockAddr** ppNewAddrQueue;

    //XXXTDM: Support resizing with queued data?
    if (m_uQueueUsed != 0)
    {
        return HXR_UNEXPECTED;
    }

    ppNewBufQueue = new IHXBuffer*[uLen];
    ppNewAddrQueue = new IHXSockAddr*[uLen/HX_IOV_MAX];
    if (ppNewBufQueue == NULL || ppNewAddrQueue == NULL)
    {
        delete[] ppNewAddrQueue;
        delete[] ppNewBufQueue;
        return HXR_OUTOFMEMORY;
    }
    delete[] m_ppBufQueue;
    delete[] m_ppAddrQueue;
    m_uHeadPos = 0;
    m_uTailPos = 0;
    m_uQueuedBytes = 0;
    m_uQueueUsed = 0;
    m_uQueueLen = uLen;
    m_ppBufQueue = ppNewBufQueue;
    m_ppAddrQueue = ppNewAddrQueue;
    return HXR_OK;
}

HX_RESULT
CHXDatagramWriteQueue::Enqueue(UINT32 nVecLen, IHXBuffer** ppBufVec,
                IHXSockAddr* pAddr)
{
    if (m_uQueueLen-m_uQueueUsed < nVecLen)
    {
        return HXR_SOCK_WOULDBLOCK;
    }
    IHXBuffer** ppCur = &m_ppBufQueue[m_uTailPos];
    memset(ppCur, 0, HX_IOV_MAX*sizeof(IHXBuffer*));
    while (nVecLen)
    {
        *ppCur = *ppBufVec;
        (*ppCur)->AddRef();
        m_uQueuedBytes += (*ppBufVec)->GetSize();
        ppCur++;
        nVecLen--;
        ppBufVec++;
    }
    m_ppAddrQueue[m_uTailPos/HX_IOV_MAX] = pAddr;
    HX_ADDREF(m_ppAddrQueue[m_uTailPos/HX_IOV_MAX]);
    ADVANCE(m_uTailPos, HX_IOV_MAX)
    m_uQueueUsed += HX_IOV_MAX;
    return HXR_OK;
}

void
CHXDatagramWriteQueue::FillVector(UINT32* pvlen, hx_iov* piov, IHXSockAddr** ppAddr)
{
    UINT32 len = 0;
    UINT32 pos = m_uHeadPos;
    memset(piov, 0, HX_IOV_MAX*sizeof(hx_iov));
    while (len < HX_IOV_MAX && m_ppBufQueue[pos] != NULL)
    {
        piov->set_len(m_ppBufQueue[pos]->GetSize());
        piov->set_buf(m_ppBufQueue[pos]->GetBuffer());
        pos++;
        len++;
        piov++;
    }
    *ppAddr = m_ppAddrQueue[m_uHeadPos/HX_IOV_MAX];
    *pvlen = len;
}

HX_RESULT
CHXDatagramWriteQueue::Dequeue(UINT32 uSize)
{
    HX_ASSERT(m_uQueueUsed >= HX_IOV_MAX);
    HX_ASSERT(m_uQueuedBytes >= uSize);
    HX_RELEASE(m_ppAddrQueue[m_uHeadPos/HX_IOV_MAX]);
    UINT32 pos = 0;
    IHXBuffer** ppCur = &m_ppBufQueue[m_uHeadPos];
    while (pos < HX_IOV_MAX && *ppCur != NULL)
    {
        m_uQueuedBytes -= (*ppCur)->GetSize();
        HX_RELEASE(*ppCur);
        ppCur++;
        pos++;
    }
    ADVANCE(m_uHeadPos, HX_IOV_MAX)
    m_uQueueUsed -= HX_IOV_MAX;
    return HXR_OK;
}
