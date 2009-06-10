/* ***** BEGIN LICENSE BLOCK ***** 
 * Source last modified: $Id:$ 
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

#include "aviffpln.h"
#include "aviindx.h"
#include "riff.h"
#include "ihxpckts.h"

#include "hxheap.h"
#include "hxtlogutil.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef NET_ENDIAN
#define LE32_TO_HOST(x)  ((x << 24)              | \
                          (x << 8  & 0x00FF0000) | \
                          (x >> 8  & 0x0000FF00) | \
                          (x >> 24 & 0xFF))
#define LE16_TO_HOST(x)  ((x << 8) |
                          (x & 0xFF))
#else
#define LE32_TO_HOST(x) (x)
#define LE16_TO_HOST(x) (x)
#endif // NET_ENDIAN

#define AVI_INDEX_CHUNK     0x69647831 /* 'idx1' */

// Flags for index (some unused):
#define AVI_IFLAG_LIST          0x00000001 // chunk is a 'LIST'
#define AVI_IFLAG_KEYCHUNK      0x00000010 // this chunk is a key chunk.
#define AVI_IFLAG_NOTIME	0x00000100 // this chunk doesn't take any time
#define AVI_IFLAG_COMPUSE       0x0FFF0000 // these bits are for compressor use

#define LOAD_GRANULARITY 8192
#define CHUNK_HEADER_SIZE 8

CAVIIndex::CAVIIndex()
    : m_pOuter(NULL)
    , m_pReader(NULL)
    , m_ulIndexOffset(0)
    , m_ulIndexLength(0)
    , m_bRead(FALSE)
    , m_bDiscardPendingIO(FALSE)
    , m_usStreamCount(0)
    , m_ulFirstMOVIOffset(0)
    , m_ulFirstRelativeMOVIOffset(0)
    , m_bRelativeIndexDetermined(FALSE)
    , m_ulFirstUnknownChunk(0)
    , m_state(eReaderOpen)
    , m_scanState(eInitial)
	, m_lRefCount(0)
{
    HXLOGL2(HXLOG_AVIX,"CAVIIndex[%p]::CAVIIndex() CTOR", this);
}

CAVIIndex::~CAVIIndex()
{
    HXLOGL2(HXLOG_AVIX,"CAVIIndex[%p]::~CAVIIndex() DES", this);
/*    HX_RELEASE(m_pReader);
	HX_RELEASE(m_pOuter);

    for (UINT16 i = 0; i < m_usStreamCount; ++i)
    {
        HX_DELETE(m_sliceArray[i]);
    }
*/
}


void CAVIIndex::Init(CAVIFileFormat* pOuter, IHXFileObject* pFile,
                IUnknown* pContext, UINT32 ulFirstMOVIOffset,
                UINT16 usStreamCount)
{
    HXLOGL2(HXLOG_AVIX,"CAVIIndex[%p]::Init()", this);

    HX_ASSERT_VALID_PTR(pOuter);
    m_pOuter = pOuter;

	//AddRef m_pOuter

	m_pOuter->AddRef();

	m_pFile = pFile;
    HX_ASSERT_VALID_PTR(m_pFile);
	HX_ADDREF(m_pFile);

    HX_ASSERT_VALID_PTR(pContext);

    m_ulFirstMOVIOffset = m_ulFirstRelativeMOVIOffset = ulFirstMOVIOffset;
    m_ulFirstUnknownChunk = ulFirstMOVIOffset;
    HX_ASSERT(m_ulFirstMOVIOffset);

    HX_ASSERT(usStreamCount);
    m_usStreamCount = usStreamCount;

    m_sliceArray.SetSize(m_usStreamCount);
    for (UINT16 i = 0; i < m_usStreamCount; ++i)
    {
        m_sliceArray[i] = new StreamSlice;
    }

    // See if we're playing from a filesystem that wants to be linear
    // (eg, http). If so, abort the index read, as the index is at
    // the end of an avi file.

    HXBOOL bPreferLinear = FALSE;

    if(m_pFile)
    {
        HX_RESULT res;
        res = m_pFile->Advise(HX_FILEADVISE_RANDOMACCESS);
        if(res == HXR_ADVISE_PREFER_LINEAR)
        {
            // As seen in CAVIIndex::RIFFFindChunkDone when
            // we fail to find an index chunk -- this is
            // basically short circuiting the search
            // for the index.
            m_state = eReady;
            m_pOuter->IOEvent();
            bPreferLinear = TRUE;
        }
    }

    if (!bPreferLinear && m_pFile && pContext)
    {
        m_pReader = new CRIFFReader(pContext, this, m_pFile);
        HX_ADDREF(m_pReader);
        m_pReader->Open("");
    }
}

// Index methods:
void CAVIIndex::AddToIndex(UINT16 usStream, UINT32 ulChunk, UINT32 ulOffset,
                           UINT32 ulSize, BOOL bKeyChunk)   // offset includes header
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::AddToIndex()", this);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    // Do we have an index, or is this chunk already indexed?
    if (m_pReader || ulChunk < pStream->ulSliceEndChunk)
    {
        return;
    }

    // In bounds?
    else if (ulChunk == pStream->ulSliceEndChunk &&
             pStream->ulSliceEndChunk - pStream->ulSliceStartChunk < MAX_SLICE_SIZE)
    {
        IndexEntry& indexEntry = pStream->entryArray[pStream->ulSliceEndChunk % MAX_SLICE_SIZE];
        indexEntry.bKeyChunk = bKeyChunk;
        indexEntry.ulOffset = ulOffset;
		indexEntry.ulLength = ulSize;
        m_ulFirstUnknownChunk = ulOffset + ulSize + CHUNK_HEADER_SIZE;
        pStream->ulSliceEndChunk++;

        pStream->ulTotalChunks++;
        pStream->ulSliceEndOffset = ulOffset + ulSize + 8;
        pStream->ulTotalBytes += ulSize;
    }
}

void CAVIIndex::FileRead(BOOL bRead)   // All chunks have been indexed; reset on seek
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::FileRead()", this);
    m_bRead = bRead;
}

void CAVIIndex::SetMinimumChunkInterest(UINT16 usStream, UINT32 ulChunk)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::SetMinimumChunkInterest()", this);
    HX_ASSERT(usStream < m_usStreamCount);

    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];
    pStream->ulNextChunkRequired = ulChunk;

    // Clip stale data:
    if (pStream->ulSliceStartChunk > ulChunk ||
        pStream->ulSliceEndChunk < ulChunk)
    {
        if (m_state != eReady)
        {
            m_bDiscardPendingIO = TRUE;
        }
    }
}

BOOL CAVIIndex::IsKeyChunk(UINT16 usStream, UINT32 ulChunk)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::IsKeyChunk()", this);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    // in bounds?
    if (ulChunk >= pStream->ulSliceStartChunk && ulChunk < pStream->ulSliceEndChunk)
    {
        return pStream->entryArray[ulChunk % MAX_SLICE_SIZE].bKeyChunk;
    }

    HX_ASSERT(!"Unindexed key chunk query.");
    return TRUE;
}

// The following return HXR_CHUNK_MISSING if it is known the referenced chunk
// is not the file or HXR_NOT_INDEXABLE if it's outside our body of knowledge;
// the closest chunk is still returned, however.
HX_RESULT CAVIIndex::FindClosestChunk(UINT16 usStream, UINT32 ulChunk,
                                      /* out */ UINT32& ulClosestOffset,
                                      /* out */ UINT32& ulClosestStreamChunk)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::FindClosestChunk()", this);

    if (ulChunk == 0)
	{
        ulClosestOffset = m_ulFirstMOVIOffset + 4;
        ulClosestStreamChunk = 0;
        return HXR_OK;
	}

    SetMinimumChunkInterest(usStream, ulChunk);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    if (pStream->ulTotalChunks != 0)
    {
        if (ulChunk >= pStream->ulSliceEndChunk)
        {
            ulClosestOffset = pStream->ulSliceEndOffset;
            ulClosestStreamChunk = pStream->ulSliceEndChunk;

            if (ulChunk >= pStream->ulTotalChunks)
            {
                if (m_bRead)
                {
                    return HXR_CHUNK_MISSING;
                }
            }

            //HX_ASSERT(!"Unindexed chunk query.");
            return HXR_NOT_INDEXABLE;
        }

        if (ulChunk >= pStream->ulSliceStartChunk)
        {
            HX_ASSERT(pStream->ulSliceStartChunk != pStream->ulSliceEndChunk);
            ulClosestOffset = pStream->entryArray[ulChunk % MAX_SLICE_SIZE].ulOffset;
            ulClosestStreamChunk = ulChunk;
            return HXR_OK;
        }
    }

    ulClosestOffset = m_ulFirstMOVIOffset + 4;
    ulClosestStreamChunk = 0;

    //HX_ASSERT(!"Unindexed chunk query.");
    return HXR_NOT_INDEXABLE;
}

HX_RESULT CAVIIndex::FindClosestKeyChunk(UINT16 usStream, UINT32 ulChunk,
                               /* out */ UINT32& ulClosestOffset,
                               /* out */ UINT32& ulClosestStreamChunk)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::FindClosestKeyChunk()", this);

    if (ulChunk == 0)
    {    
        ulClosestOffset = m_ulFirstMOVIOffset + 4;
        ulClosestStreamChunk = 0;
        return HXR_OK;
    }

    SetMinimumChunkInterest(usStream, ulChunk);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    if (pStream->ulTotalChunks != 0)
    {
        if (ulChunk >= pStream->ulLastKeyChunk)
        {
            ulClosestStreamChunk = min(ulChunk, pStream->ulSliceEndChunk - 1);

            while (ulClosestStreamChunk > pStream->ulSliceStartChunk &&
                   !pStream->entryArray[ulClosestStreamChunk % MAX_SLICE_SIZE].bKeyChunk)
            {
                --ulClosestStreamChunk;
            }

            if (pStream->entryArray[ulClosestStreamChunk % MAX_SLICE_SIZE].bKeyChunk)
            {
                ulClosestOffset = pStream->entryArray[ulClosestStreamChunk % MAX_SLICE_SIZE].ulOffset;
            }
            else
            {
                ulClosestOffset = pStream->ulLastKeyChunkOffset;
                ulClosestStreamChunk = pStream->ulLastKeyChunk;
                SetMinimumChunkInterest(usStream, pStream->ulLastKeyChunk);
            }

            return ulChunk >= pStream->ulSliceEndChunk ? HXR_NOT_INDEXABLE : HXR_OK;
        }
    }

    ulClosestOffset = m_ulFirstMOVIOffset + 4;
    ulClosestStreamChunk = 0;

    return HXR_NOT_INDEXABLE;
}

// The following return zero if no information is available:
// in bytes, not including header:
UINT32 CAVIIndex::GetMaxChunkSize(UINT16 usStream)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::GetMaxChunkSize()", this);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];
    return pStream->ulMaxChunkSize;
}

// in bytes, not including header:
double CAVIIndex::GetAverageChunkSize(UINT16 usStream)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::GetAverageChunkSize()", this);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    return pStream->ulTotalBytes / (double) pStream->ulTotalChunks;
}

// Returns zero if we have no index; assumes transmission at average rate
UINT32 CAVIIndex::GetMaxByteDeflict(UINT16 usStream)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::GetMaxByteDeflict()", this);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    return pStream->ulPrerollBytes;
}

UINT32 CAVIIndex::GetByteTotal(UINT16 usStream)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::GetByteTotal()", this);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    return pStream->ulTotalBytes;
}

UINT32 CAVIIndex::GetChunkTotal(UINT16 usStream)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::GetChunkTotal()", this);
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    return pStream->ulTotalChunks;
}

BOOL CAVIIndex::CanLoadSlice()
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::CanLoadSlice()", this);
    // We want serialized access:
    HX_ASSERT(m_state == eReady);

    for (UINT16 i = 0; i < m_usStreamCount; ++i)
    {
        StreamSlice* pStream = (StreamSlice*) m_sliceArray[i];

        if (m_pReader &&
            ((pStream->ulNextChunkRequired >= pStream->ulSliceEndChunk &&
              pStream->ulSliceEndOffset < m_ulIndexOffset + m_ulIndexLength - sizeof(idx1Entry)) ||
            pStream->ulNextChunkRequired < pStream->ulSliceStartChunk))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CAVIIndex::CanPreloadSlice()
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::CanPreloadSlice()", this);
    // We want serialized access:
    HX_ASSERT(m_state == eReady);

    for (UINT16 i = 0; i < m_usStreamCount; ++i)
    {
        StreamSlice* pStream = (StreamSlice*) m_sliceArray[i];

        if (m_pReader &&
            pStream->ulSliceEndOffset < m_ulIndexOffset + m_ulIndexLength - sizeof(idx1Entry) &&
            ((pStream->ulNextChunkRequired >= pStream->ulSliceEndChunk ||
            pStream->ulNextChunkRequired < pStream->ulSliceStartChunk) ||
            ((INT64) MAX_SLICE_SIZE - (pStream->ulSliceEndChunk - pStream->ulNextChunkRequired) >= LOAD_GRANULARITY)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

void CAVIIndex::GetNextSlice()
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::GetNextSlice()", this);
    HX_ASSERT(CanLoadSlice() || CanPreloadSlice());

    // We want serialized access:
    HX_ASSERT(m_state == eReady);

    // We support bizzarely ordered indexes
    UINT64 llMinFillLevel = MAX_UINT64;
    INT32 ulLeastFullStream = -1;

    for (UINT16 i = 0; m_pReader && i < m_usStreamCount; ++i)
    {
        StreamSlice* pStream = (StreamSlice*) m_sliceArray[i];

        if (pStream->ulNextChunkRequired >= pStream->ulSliceEndChunk)
        {
            UINT32 ulIndexRemaining = m_ulIndexOffset + m_ulIndexLength - pStream->ulSliceEndOffset;
            ulIndexRemaining -= ulIndexRemaining % sizeof(idx1Entry);

            if (!ulIndexRemaining)
            {
                continue;
            }

            if (m_pReader->GetOffset() != pStream->ulSliceEndOffset)
            {
                m_state = eSeekToRecord;
                m_pReader->FileSeek(pStream->ulSliceEndOffset);
            }
            else
            {
                m_state = eReadSlice;
                m_pReader->Read((ulIndexRemaining > LOAD_GRANULARITY*sizeof(idx1Entry)) ?
                                LOAD_GRANULARITY*sizeof(idx1Entry) : ulIndexRemaining);
            }

            return;
        }
        else if (pStream->ulNextChunkRequired < pStream->ulSliceStartChunk)
        {
            pStream->ulSliceStartChunk = pStream->ulSliceEndChunk = 0;
            pStream->ulSliceEndOffset = 0;
            pStream->ulLastKeyChunk = 0;
            pStream->ulLastKeyChunkOffset = m_ulFirstMOVIOffset;

            if (m_pReader->GetOffset() != m_ulIndexOffset)
            {
                m_state = eSeekToRecord;
                m_pReader->FileSeek(m_ulIndexOffset);
            }
            else
            {
                UINT32 ulIndexRemaining = m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset();
                ulIndexRemaining -= ulIndexRemaining % sizeof(idx1Entry);

                if (!ulIndexRemaining)
                {
                    continue;
                }

                m_state = eReadSlice;
                m_pReader->Read((ulIndexRemaining > LOAD_GRANULARITY*sizeof(idx1Entry)) ?
                                LOAD_GRANULARITY*sizeof(idx1Entry) : ulIndexRemaining);
            }

            return;
        }

        if (pStream->ulSliceEndOffset <= m_ulIndexOffset + m_ulIndexLength - sizeof(idx1Entry) &&
            pStream->ulSliceEndChunk - pStream->ulNextChunkRequired < llMinFillLevel)
        {
            llMinFillLevel = (UINT64) pStream->ulSliceEndChunk - (UINT64) pStream->ulNextChunkRequired;
            ulLeastFullStream = i;
        }
    }

    if (MAX_SLICE_SIZE - llMinFillLevel >= LOAD_GRANULARITY && ulLeastFullStream >= 0)
    {
        StreamSlice* pStream = (StreamSlice*) m_sliceArray[ulLeastFullStream];

        if (m_pReader->GetOffset() != pStream->ulSliceEndOffset)
        {
            m_state = eSeekToRecord;
            m_pReader->FileSeek(pStream->ulSliceEndOffset);
        }
        else
        {
            UINT32 ulIndexRemaining = m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset();
            ulIndexRemaining -= ulIndexRemaining % sizeof(idx1Entry);

            if (ulIndexRemaining >= sizeof(idx1Entry))
            {
                m_state = eReadSlice;
                m_pReader->Read((ulIndexRemaining > LOAD_GRANULARITY*sizeof(idx1Entry)) ?
                                LOAD_GRANULARITY*sizeof(idx1Entry) : ulIndexRemaining);
            }
            else
            {
                m_pOuter->IOEvent();
            }
        }

        return;
    }

    m_pOuter->IOEvent();
}

STDMETHODIMP CAVIIndex::QueryInterface(REFIID riid, void** ppvObj)
{
    //return m_pOuter->QueryInterface(riid, ppvObj);

	HX_RESULT retVal = HXR_NOINTERFACE;
	*ppvObj = NULL;

	if ( IsEqualIID(riid, IID_IUnknown) )
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
	else if(m_pOuter)
	{
		retVal = m_pOuter->QueryInterface(riid, ppvObj);
	}

	return retVal;

}

// CRIFFResponse:
STDMETHODIMP_(ULONG32) CAVIIndex::AddRef()
{
    //HX_TRACE("CAVIFileFormat::CAVIIndex::AddRef()\n");
    //return m_pOuter->AddRef();

	return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CAVIIndex::Release()
{
    //HX_TRACE("CAVIFileFormat::CAVIIndex::Release()\n");
    //return m_pOuter->Release();

    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;

}

STDMETHODIMP CAVIIndex::RIFFOpenDone(HX_RESULT status)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFOpenDone()", this);
    HX_ASSERT(m_state == eReaderOpen);

    m_state = eInitialDescend;
    m_pReader->Descend();
    return HXR_OK;
}

STDMETHODIMP CAVIIndex::RIFFCloseDone(HX_RESULT status)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFCloseDone()", this);
    return HXR_NOTIMPL;
}

/* RIFFFindChunkDone is called when a FindChunk completes.
 * len is the length of the data in the chunk (i.e. Read(len)
 * would read the whole chunk)
 */
STDMETHODIMP CAVIIndex::RIFFFindChunkDone(HX_RESULT status, UINT32 len)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFFindChunkDone() status=%x", this, status);
    HX_ASSERT(m_state == eIdx1Find);

    if (FAILED(status) || len == 0)
    {
        // No index; release reader:
        HX_DELETE(m_pReader);
        m_state = eReady;
        m_pOuter->IOEvent();
        return HXR_OK;
    }

    if (m_pReader->GetChunkType() == AVI_INDEX_CHUNK)
    {
        m_state = eIdx1Descend;
        m_ulIndexOffset = m_pReader->GetOffset();
        m_ulIndexLength = len;

        for (UINT16 i = 0; i < m_usStreamCount; ++i)
        {
            StreamSlice* pStream = (StreamSlice*) m_sliceArray[i];
            pStream->ulSliceEndOffset = m_ulIndexOffset;
        }

        // Scan index for preroll, bitrate values:
        HX_ASSERT(m_scanState == eInitial);
        m_pReader->Descend();
    }

    return HXR_OK;
}

/* Called after a Descend completes */
STDMETHODIMP CAVIIndex::RIFFDescendDone(HX_RESULT status)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFDescendDone()", this);
    HX_ASSERT(m_state == eInitialDescend || m_state == eIdx1Descend);

    switch (m_state)
    {
        case eInitialDescend:
            m_state = eInitialSeek;
            m_pReader->FileSeek(m_ulFirstUnknownChunk);
            break;
        case eIdx1Descend:
            switch (m_scanState)
            {
                case eInitial:
                {
                    HX_ASSERT((INT64) m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset() >= 0);
                    UINT32 ulIndexRemaining = m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset();
                    ulIndexRemaining -= ulIndexRemaining % sizeof(idx1Entry);

                    if (ulIndexRemaining >= sizeof(idx1Entry))
                    {
                        m_state = eReadSlice;
                        m_pReader->Read((ulIndexRemaining > LOAD_GRANULARITY*sizeof(idx1Entry)) ?
                                        LOAD_GRANULARITY*sizeof(idx1Entry) : ulIndexRemaining);
                    }
                    else
                    {
                        m_state = eReady;
                    }
                }
                break;
                default:
                    m_state = eReady;
                    HX_ASSERT(!"Unexpected CAVIIndex state");
            }
            break;
        default:
            m_state = eReady;
            HX_ASSERT(!"Unexpected CAVIIndex state");
    }

    return HXR_OK;
}

/* Called after an Ascend completes */
STDMETHODIMP CAVIIndex::RIFFAscendDone(HX_RESULT status)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFAscendDone()", this);
    return HXR_NOTIMPL;
}

/* Called when a read completes.  IHXBuffer contains the data read
 */
STDMETHODIMP CAVIIndex::RIFFReadDone(HX_RESULT status, IHXBuffer *pBuffer)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFReadDone()", this);
    HX_ASSERT(m_state == eReadSlice);

    if (m_bDiscardPendingIO)
    {
        // We've seeked or the stream has ended since the I/O request;
        // let the the outer object reschedule disk reads.
        m_bDiscardPendingIO = FALSE;
        m_state = eReady;
        m_pOuter->IOEvent();
        return HXR_OK;
    }

    // We should never have a failed read unless the file is truncated:
    if (FAILED(status) || !(pBuffer->GetSize() / sizeof(idx1Entry)))
    {
        // Adjust index size:
        m_ulIndexLength = m_pReader->GetOffset() - m_ulIndexOffset;
        switch (m_scanState)
        {
            case eInitial:
                m_scanState = ePreroll;
                m_state = eSeekToRecord;
                m_pReader->FileSeek(m_ulIndexOffset);
                break;
            case ePreroll:
                m_scanState = eComplete;
                break;
            case eComplete:
            default:
                break;
                // TODO: Add correct event callback
        }
    }
    else
    {
        idx1Entry* pEntry = (idx1Entry*) pBuffer->GetBuffer();
        HX_ASSERT(pEntry);

        switch (m_scanState)
        {
            case eInitial:
            {
                HX_ASSERT(pBuffer->GetSize() / sizeof(idx1Entry));
                for (UINT32 i = 0; i < pBuffer->GetSize() / sizeof(idx1Entry); ++i)
                {
                    if (CAVIFileFormat::IsAVChunk(pEntry->ulChunkId))
                    {
                        StreamSlice* pStream = (StreamSlice*) m_sliceArray[CAVIFileFormat::GetStream(pEntry->ulChunkId)];

                        pStream->ulTotalBytes += LE32_TO_HOST(pEntry->ulChunkLength);
                        if (pEntry->ulChunkLength > pStream->ulMaxChunkSize)
                        {
                            pStream->ulMaxChunkSize = LE32_TO_HOST(pEntry->ulChunkLength);
                        }
                        pStream->ulTotalChunks++;
                    }

                    pEntry++;
                }

                HX_ASSERT((INT64) m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset() >= 0);
                UINT32 ulIndexRemaining = m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset();
                ulIndexRemaining -= ulIndexRemaining % sizeof(idx1Entry);

                if (ulIndexRemaining >= sizeof(idx1Entry))
                {
                    m_state = eReadSlice;
                    m_pReader->Read((ulIndexRemaining > LOAD_GRANULARITY*sizeof(idx1Entry)) ?
                                    LOAD_GRANULARITY*sizeof(idx1Entry) : ulIndexRemaining);
                }
                else
                {
                    // Truncated files not supported, for now:
                    HX_ASSERT(m_pReader->GetOffset() == m_ulIndexOffset + m_ulIndexLength);
                    m_state = eSeekToRecord;
                    m_scanState = ePreroll;
                    m_pReader->FileSeek(m_ulIndexOffset);
                }
                break;
            }

            case ePreroll:
            {
                HX_ASSERT(pBuffer->GetSize() / sizeof(idx1Entry));
                for (UINT32 i = 0; i < pBuffer->GetSize() / sizeof(idx1Entry); ++i)
                {
                    if (CAVIFileFormat::IsAVChunk(pEntry->ulChunkId))
                    {
                        StreamSlice* pStream = (StreamSlice*) m_sliceArray[CAVIFileFormat::GetStream(pEntry->ulChunkId)];

                        UINT32 ulAverageBytesPerChunk = (UINT32) (pStream->ulTotalBytes / (double) pStream->ulTotalChunks);
                        pStream->llByteDeflict -= ((INT64) ulAverageBytesPerChunk - LE32_TO_HOST(pEntry->ulChunkLength));

                        // Check max:
                        if (pStream->llByteDeflict > pStream->ulPrerollBytes)
                        {
                            HX_ASSERT(pStream->llByteDeflict < MAX_UINT32);
                            pStream->ulPrerollBytes = (UINT32) pStream->llByteDeflict;
                        }
                    }

                    pEntry++;
                }

                HX_ASSERT((INT64) m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset() >= 0);
                UINT32 ulIndexRemaining = m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset();
                ulIndexRemaining -= ulIndexRemaining % sizeof(idx1Entry);

                if (ulIndexRemaining >= sizeof(idx1Entry))
                {
                    m_state = eReadSlice;
                    m_pReader->Read((ulIndexRemaining > LOAD_GRANULARITY*sizeof(idx1Entry)) ?
                                    LOAD_GRANULARITY*sizeof(idx1Entry) : ulIndexRemaining);
                }
                else
                {
                    HX_ASSERT(m_pReader->GetOffset() == m_ulIndexOffset + m_ulIndexLength);
                    //m_state = eReady;
					m_state = eSeekToRecord;
                    m_scanState = eComplete;
                    //m_pOuter->IOEvent();
					m_pReader->FileSeek(m_ulIndexOffset);
                }
                break;      // In the future, we may consider reading the index data, too;
                            // the current case is optimized for large files (MAX_SLICE_SIZE < chunks in file)
            }

            default:
            case eComplete:
            {
                for (UINT32 i = pBuffer->GetSize() / sizeof(idx1Entry); i > 0; --i)
                {
                    if (!m_bRelativeIndexDetermined)
                    {
                        if (LE32_TO_HOST(pEntry->ulChunkOffset) == m_ulFirstMOVIOffset + 4)
                        {
                            // Offsets are not relative to the movi chunk:
                            m_ulFirstRelativeMOVIOffset = 0;
                        }

                        m_bRelativeIndexDetermined = TRUE;
                    }

                    if (CAVIFileFormat::IsAVChunk(pEntry->ulChunkId))
                    {
                        StreamSlice* pStream = (StreamSlice*) m_sliceArray[CAVIFileFormat::GetStream(pEntry->ulChunkId)];

                        if ((INT64) pStream->ulSliceEndChunk - pStream->ulNextChunkRequired < MAX_SLICE_SIZE)
                        {
                            pStream->bSliceOffsetCapped = FALSE;

                            if (pStream->ulSliceEndChunk - pStream->ulSliceStartChunk == MAX_SLICE_SIZE)
                            {
                                if (pStream->entryArray[pStream->ulSliceStartChunk % MAX_SLICE_SIZE].bKeyChunk)
                                {
                                    pStream->ulLastKeyChunk = pStream->ulSliceStartChunk;
                                    pStream->ulLastKeyChunkOffset = pStream->entryArray[pStream->ulSliceStartChunk % MAX_SLICE_SIZE].ulOffset;
                                }
                                pStream->ulSliceStartChunk++;
                                HX_ASSERT(pStream->ulSliceStartChunk <= pStream->ulNextChunkRequired);
                            }

                            if(LE32_TO_HOST(pEntry->dwFlags) & AVI_IFLAG_KEYCHUNK)
                            {
                                pStream->entryArray[pStream->ulSliceEndChunk % MAX_SLICE_SIZE].bKeyChunk = TRUE;
                            }
                            else
                            {
                                pStream->entryArray[pStream->ulSliceEndChunk % MAX_SLICE_SIZE].bKeyChunk = FALSE;
                            }
                            pStream->entryArray[pStream->ulSliceEndChunk % MAX_SLICE_SIZE].ulOffset =
                                LE32_TO_HOST(pEntry->ulChunkOffset) + m_ulFirstRelativeMOVIOffset;
						    pStream->entryArray[pStream->ulSliceEndChunk % MAX_SLICE_SIZE].ulLength =
		 					    LE32_TO_HOST(pEntry->ulChunkLength);
                          //  HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFReadDone stream: %d\toffset: %lu", this,CAVIFileFormat::GetStream(pEntry->ulChunkId),
                          //           pStream->entryArray[pStream->ulSliceEndChunk % MAX_SLICE_SIZE].ulOffset);

#ifdef _DEBUG_KEYCHUNKS
                            if (pStream->entryArray[pStream->ulSliceEndChunk % MAX_SLICE_SIZE].bKeyChunk)
                            {
                                HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFReadDone keytype: %c%c%c%c",this,
                                pEntry->ulChunkId, pEntry->ulChunkId >> 8,
                                pEntry->ulChunkId >> 16, pEntry->ulChunkId >> 24);
                            }
#endif
                            pStream->ulSliceEndChunk++;
                        }
                        else if (!pStream->bSliceOffsetCapped)
                        {
                            pStream->bSliceOffsetCapped = TRUE;
                            pStream->ulSliceEndOffset = m_pReader->GetOffset() - i*sizeof(idx1Entry);
                        }

                        for (UINT16 j = 0; j < m_usStreamCount; j++)
                        {
                            StreamSlice* pStream = (StreamSlice*) m_sliceArray[j];
                            if (!pStream->bSliceOffsetCapped)
                            {
                                pStream->ulSliceEndOffset = m_pReader->GetOffset() - (i-1)*sizeof(idx1Entry);
                            }
                        }
                    }

                    pEntry++;
                }

                m_state = eReady;
                if (CanLoadSlice())
                {
                    GetNextSlice();
		    return HXR_OK;
                }

		m_pOuter->IOEvent();
            }
        }
    }

    return HXR_OK;
}

/* Called when a seek completes */
STDMETHODIMP CAVIIndex::RIFFSeekDone(HX_RESULT status)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFSeekDone()", this);

    if (m_bDiscardPendingIO)
    {
        // We've seeked or the stream has ended since the I/O request;
        // let the the outer object reschedule disk reads.
        m_bDiscardPendingIO = FALSE;
        m_state = eReady;
        m_pOuter->IOEvent();
        return HXR_OK;
    }

    switch (m_state)
    {
        case eInitialSeek:
            m_state = eIdx1Find;
            m_pReader->FindChunk(AVI_INDEX_CHUNK, TRUE);
            break;
        case eSeekToRecord:
            {
                HX_ASSERT((INT64) m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset() >= 0);
                UINT32 ulIndexRemaining = m_ulIndexOffset + m_ulIndexLength - m_pReader->GetOffset();
                ulIndexRemaining -= ulIndexRemaining % sizeof(idx1Entry);

                if (ulIndexRemaining >= sizeof(idx1Entry))
                {
                    m_state = eReadSlice;
                    m_pReader->Read((ulIndexRemaining > LOAD_GRANULARITY*sizeof(idx1Entry)) ?
                                    LOAD_GRANULARITY*sizeof(idx1Entry) : ulIndexRemaining);
                }
                else
                {
                    m_state = eReady;
                    m_pOuter->IOEvent();
                }
            }
            break;
    }

    return HXR_OK;
}

/* Called with the data from a GetChunk request */
STDMETHODIMP CAVIIndex::RIFFGetChunkDone(HX_RESULT status, UINT32 chunkType,
                                         IHXBuffer* pBuffer)
{
    HXLOGL4(HXLOG_AVIX,"CAVIIndex[%p]::RIFFGetChunkDone()", this);
    return HXR_NOTIMPL;
}

void CAVIIndex::Close()
{
	HXLOGL2(HXLOG_AVIX,"CAVIIndex[%p]::Close()", this);
	if(m_pFile)
	{
		m_pFile->Close();
		HX_RELEASE(m_pFile);
	}

	HX_RELEASE(m_pReader);
	HX_RELEASE(m_pOuter);

    for (UINT16 i = 0; i < m_usStreamCount; ++i)
    {
        HX_DELETE(m_sliceArray[i]);
    }
}

INT32 CAVIIndex::GetChunkLength (UINT16 usStream, UINT32 ulChunkNumber)
{
    StreamSlice* pStream = (StreamSlice*) m_sliceArray[usStream];

    if (ulChunkNumber < pStream->ulSliceEndChunk)
        return pStream->entryArray [ulChunkNumber].ulLength;
    else
        return -1;
}
