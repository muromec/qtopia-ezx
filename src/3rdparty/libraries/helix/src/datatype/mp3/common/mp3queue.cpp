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

#include "hlxclib/string.h"
#include "mp3format.h"
#include "mp3queue.h"

CMp3Queue::CMp3Queue(CMp3Format* pFmt, int nEntries)
 :  m_ulDataBytes(0),
    m_ulQueueSize(nEntries),
    m_ulEntries(0),
    m_ulHead(0),
    m_ulAddIndex(0),
    m_ulHeadDataRemnant(0),
    m_ulNext(0),
    m_pQueue(NULL),
    m_pFmt(pFmt)
{
    m_pQueue = new tFrameInfo[m_ulQueueSize];
    memset(m_pQueue, 0, sizeof(tFrameInfo) * m_ulQueueSize);

    m_ulNext = m_ulHead + 1;
}

CMp3Queue::~CMp3Queue()
{
    RemoveAll();

    if (m_pQueue)
        delete [] m_pQueue;
}

int CMp3Queue::AddEntry(IHXPacket* pPacket)
{
    if (!pPacket)
        return -1;

    IHXBuffer *pBuf = pPacket->GetBuffer();
    
    m_pQueue[m_ulAddIndex].pPacket = pPacket;
    m_pQueue[m_ulAddIndex].pPacket->AddRef();
    m_pQueue[m_ulAddIndex].ulSize = pBuf->GetSize();
    m_pQueue[m_ulAddIndex].pBuffer = pBuf->GetBuffer();
    //m_pQueue[m_ulAddIndex].pBuffer = new UCHAR[pBuf->GetSize()];
    //memcpy(m_pQueue[m_ulAddIndex].pBuffer, pBuf->GetBuffer(), pBuf->GetSize());
    
    m_pQueue[m_ulAddIndex].ulTime = pPacket->GetTime();

    if (GenerateFrameInfo(&m_pQueue[m_ulAddIndex]))
    {
        int nOffset = m_ulAddIndex;

        if (++m_ulAddIndex >= m_ulQueueSize)
            m_ulAddIndex -= m_ulQueueSize;

        ++m_ulEntries;

        m_ulDataBytes += m_pQueue[nOffset].ulDataSize;

        return nOffset;
    }
    else
        return -1;
}

tFrameInfo* CMp3Queue::GetHead()
{
    m_ulNext = m_ulHead + 1;

    if (m_ulNext >= m_ulQueueSize)
        m_ulNext -= m_ulQueueSize;

    return &m_pQueue[m_ulHead];
}

tFrameInfo* CMp3Queue::GetIndex(int nIndex)
{
    return &m_pQueue[nIndex];
}

tFrameInfo* CMp3Queue::Next()
{
    int nIndex = m_ulNext;

    if (++m_ulNext >= m_ulQueueSize)
        m_ulNext -= m_ulQueueSize;

    return &m_pQueue[nIndex];
}

void CMp3Queue::RemoveAll()
{
    for (UINT32 i=0; i<m_ulQueueSize; i++)
    {
        HX_RELEASE(m_pQueue[i].pPacket);
        //if (m_pQueue[i].pBuffer)
        //{
        //    delete [] m_pQueue[i].pBuffer;
        //    m_pQueue[i].pBuffer = 0;
        //}
    }

    m_ulHead = 0;
    m_ulNext = m_ulHead + 1;
    m_ulAddIndex = m_ulHead;
    m_ulEntries = 0;
    m_ulDataBytes = 0;
}

void CMp3Queue::RemoveHead()
{
    m_ulNext = m_ulHead + 1;

    HX_RELEASE(m_pQueue[m_ulHead].pPacket);

    //if (m_pQueue[m_ulHead].pBuffer)
    //    delete [] m_pQueue[m_ulHead].pBuffer;

    memset(&m_pQueue[m_ulHead], 0, sizeof(m_pQueue[m_ulHead]));

    m_ulHead = m_ulNext;
    if (m_ulHead >= m_ulQueueSize)
        m_ulHead -= m_ulQueueSize;

    if (++m_ulNext >= m_ulQueueSize)
        m_ulNext -= m_ulQueueSize;

    --m_ulEntries;
}


UCHAR CMp3Queue::GenerateFrameInfo(tFrameInfo* pInfo)
{
    UCHAR   *pBuffer = pInfo->pBuffer;
    UINT32  ulSize = pInfo->ulSize;

    if (!pBuffer || !ulSize)
        return 0;
    
    if (m_ulHead == m_ulAddIndex)
        m_pFmt->Init(pBuffer, ulSize);

    int     nFrameSize,
            nHdrSize,
            nDataOffset;

    m_pFmt->GetDataOffset(pBuffer,
                          ulSize,
                          nFrameSize,
                          nHdrSize,
                          nDataOffset);
    pInfo->pHdr = pBuffer;
    pInfo->ulHdrSize = nHdrSize;

    pInfo->pData = pBuffer + nHdrSize;
    pInfo->ulDataSize = ulSize - nHdrSize;
    
    // If main_data_begin on the head entry was greater than
    // its data, skip the remaining data in future frames.
    if (m_ulHeadDataRemnant)
    {
        int nCopy = HX_MIN(m_ulHeadDataRemnant, pInfo->ulDataSize);
        
        pInfo->pData += nCopy;
        pInfo->ulDataSize -= nCopy;

        m_ulHeadDataRemnant -= nCopy;
    }

    // If this is the first frame in the queue, ensure pData points the
    // the first byte of data for this syncword.
    if (m_ulHead == m_ulAddIndex)
    {
        if ((UINT32)nDataOffset > pInfo->ulDataSize)
        {
            m_ulHeadDataRemnant = nDataOffset - pInfo->ulDataSize;

            pInfo->pData += pInfo->ulDataSize;
            pInfo->ulDataSize = 0;
        }
        else
        {
            pInfo->pData += nDataOffset;
            pInfo->ulDataSize -= nDataOffset;
        }
    }

    pInfo->ulFrameSize = nFrameSize;
    pInfo->ulOffset = nDataOffset;


    return 1;
}
