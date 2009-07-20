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

/****************************************************************************
 *  Defines
 */

/****************************************************************************
 *  Includes
 */
#include <stdlib.h>

#include "qtoffsetmpr.h"
#include "hxprdnld.h"


/****************************************************************************
 *  Class CQTOffsetToTimeMapper
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQTOffsetToTimeMapper::CQTOffsetToTimeMapper(void)
    : m_ulNumOffsets(0),
      m_pOffsetMap(NULL),
      m_pOffsetMapIndex(NULL),
      m_uNumStreams(0),
      m_pStreamAvgBitrate(NULL),
      m_ulFileDuration(0),
      m_ulOffsetIdx(0)
{
    ;
}


CQTOffsetToTimeMapper::~CQTOffsetToTimeMapper()
{
    Clear();
}


/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CQTOffsetToTimeMapper::Init(CQTTrackManager* pTrackManager,
				      ULONG32 ulFileDuration)
{
    UINT16 uStrmIdx;
    UINT32 ulOffsetIdx;
    UINT16 uNumStreams = 0;
    UINT32 ulNumOffsets = 0;
    UINT32* pStreamBytes = NULL;
    UINT32* pStreamOffsetIdx = NULL;
    UINT32* pStreamLastOffset = NULL;
    UINT32* pOffsetInStrmIdx = NULL;
    UINT32* pOffsetDeferredBytes = NULL;

    UINT32 ulBaseOffset = 0;
    UINT32 ulMaxOffsetGap = 0;
    HXBOOL bSortNeeded = FALSE;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    Clear();

    if (pTrackManager)
    {
	retVal = HXR_OK;
    }

    // Obtain number of streams and allocate per stream info. storage
    if (SUCCEEDED(retVal))
    {
	m_ulFileDuration = ulFileDuration;

	uNumStreams = pTrackManager->GetNumStreams();

	if (uNumStreams != 0)
	{
	    retVal = HXR_OUTOFMEMORY;
	    m_pStreamAvgBitrate = new UINT32 [uNumStreams];
	    pStreamBytes = new UINT32 [uNumStreams];
	    pStreamOffsetIdx = new UINT32 [uNumStreams];
	    pStreamLastOffset = new UINT32 [uNumStreams];
	    if (m_pStreamAvgBitrate && 
		pStreamBytes && 
		pStreamOffsetIdx && 
		pStreamLastOffset)
	    {
		memset(m_pStreamAvgBitrate, 0, uNumStreams * sizeof(UINT32));
		memset(pStreamBytes, 0, uNumStreams * sizeof(UINT32));
		memset(pStreamOffsetIdx, 0, uNumStreams * sizeof(UINT32));
		memset(pStreamLastOffset, 0, uNumStreams * sizeof(UINT32));
		m_uNumStreams = uNumStreams;
		retVal = HXR_OK;
	    }
	}
    }

    // Obtain number of chunk offsets to track and stream bitrates
    for (uStrmIdx = 0; SUCCEEDED(retVal) && (uStrmIdx < m_uNumStreams); uStrmIdx++)
    {
	if (pTrackManager->IsStreamTrackActive(uStrmIdx))
	{
	    // There is an offset for every chunk
	    ulNumOffsets += pTrackManager->
				GetStreamTrack(uStrmIdx)->
				    GetChunkToOffsetMgr()->
					GetNumChunks();
	    
	    // Try to obtain bitrate
	    retVal = pTrackManager->GetStreamTrack(uStrmIdx)->
		ObtainTrackBitrate(m_pStreamAvgBitrate[uStrmIdx]);
	}
    }

    // Create map storage
    if (SUCCEEDED(retVal) && (ulNumOffsets != 0))
    {
	retVal = HXR_OUTOFMEMORY;
	m_pOffsetMap = new QTOffsetMapEntry [ulNumOffsets];
	m_pOffsetMapIndex = new QTOffsetMapEntry* [ulNumOffsets];
	pOffsetInStrmIdx = new UINT32 [ulNumOffsets];
	pOffsetDeferredBytes = new UINT32 [ulNumOffsets];
	
	if (m_pOffsetMap && 
	    m_pOffsetMapIndex &&
	    pOffsetInStrmIdx &&
	    pOffsetDeferredBytes)
	{
	    m_ulNumOffsets = ulNumOffsets;
	    retVal = HXR_OK;
	}
    }

    // Build the map by merge sorting the stream offsets.
    // In order for merge sort to work, offsets of each stream must be stored
    // in increasing order.  If this is not the case, merge sort will fail to
    // produce the sorted master offset list in which case this is noted and
    // quick sort is run on the offsets as keys afterwards.
    if (SUCCEEDED(retVal))
    {
	UINT32 ulOffset;
	UINT32 ulMinOffset;
	UINT32 ulPrevMinOffset = 0;
	UINT16 uMinOffsetStrmIdx;
	HXBOOL bMinFound;
	
	CQT_ChunkToOffset_Manager* pChunkToOffsetMgr;

	for (ulOffsetIdx = 0; ulOffsetIdx < m_ulNumOffsets; ulOffsetIdx++)
	{
	    ulMinOffset = MAX_UINT32;
	    uMinOffsetStrmIdx = 0;
	    bMinFound = FALSE;

	    for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	    {
		if (pTrackManager->IsStreamTrackActive(uStrmIdx))
		{
		    pChunkToOffsetMgr = pTrackManager->
					    GetStreamTrack(uStrmIdx)->
						GetChunkToOffsetMgr();

		    if (pChunkToOffsetMgr->GetChunkOffsetByChunk(
				    pStreamOffsetIdx[uStrmIdx] + 1,
				    ulOffset) &&
			(ulOffset <= ulMinOffset))
		    {
			ulMinOffset = ulOffset;
			uMinOffsetStrmIdx = uStrmIdx;
			bMinFound = TRUE;
		    }
		}
	    }

	    HX_ASSERT(bMinFound);

	    m_pOffsetMapIndex[ulOffsetIdx] = m_pOffsetMap + ulOffsetIdx;

	    if (bMinFound)
	    {
		// Initialize known quantities (offset and stream number) in 
		// the map.
		// Other quantities will be dealt with after sorted order is
		// ensured.
		m_pOffsetMap[ulOffsetIdx].ulOffset = ulMinOffset;
		m_pOffsetMap[ulOffsetIdx].uStream = uMinOffsetStrmIdx;

		// Remeber the in-stream sequence index for time mapping 
		// analysis later.
		pOffsetInStrmIdx[ulOffsetIdx] = pStreamOffsetIdx[uMinOffsetStrmIdx];

		// Advance the index into the stream we just merged from
		pStreamOffsetIdx[uMinOffsetStrmIdx]++;

		// Test if final table is sorted in increasing order by offset.
		// If not, note that sort is needed afterwards.
		if (ulPrevMinOffset > ulMinOffset)
		{
		    bSortNeeded = TRUE;
		}

		// Store the largest offset for each stream.
		// this is needed to establish if a stream should be considered
		// terminated at some offset and thus be excluded from 
		// mapping to playable time.
		// If stream is terminated (ended) it no longer poses 
		// restriction on amount of playable time.
		if (pStreamLastOffset[uMinOffsetStrmIdx] < ulMinOffset)
		{
		    pStreamLastOffset[uMinOffsetStrmIdx] = ulMinOffset;
		}

		ulPrevMinOffset = ulMinOffset;
	    }
	    else
	    {
		// This is unexpected
		retVal = HXR_FAIL;
		break;
	    }
	}
    }

    // Sort the map by offsets if unsorted at this point
    if (SUCCEEDED(retVal) && bSortNeeded && (m_ulNumOffsets != 0))
    {
	qsort(m_pOffsetMapIndex, 
	      m_ulNumOffsets, 
	      sizeof(QTOffsetMapEntry*), 
	      compareOffsetMapEntry);

    }

    // Determine the full span of media data played.
    // The span is used to create sceiling for offset time mapping
    // by interpolating file duration.
    // This sceiling is important to prevent offset to time mapping
    // overestimation in cases of hinted files, multi-rate files, 
    // alternate-track files and files with non-media data embedded 
    // between media data chunks.
    if (SUCCEEDED(retVal))
    {
	if ((m_ulNumOffsets > 1) && 
	    (ulFileDuration != HX_PROGDOWNLD_UNKNOWN_DURATION))
	{
	    ulBaseOffset = m_pOffsetMapIndex[0]->ulOffset;
	    ulMaxOffsetGap = m_pOffsetMapIndex[m_ulNumOffsets - 1]->ulOffset - 
			     ulBaseOffset;

	    // Since we do not have the information about the size of the last 
	    // chunk, approximate by assuming the size of the previous chunk
	    // in the same stream.
	    if (m_ulNumOffsets > 1)
	    {
		// Find out the stream the last chunk belongs to
		uStrmIdx = m_pOffsetMapIndex[m_ulNumOffsets - 1]->uStream;

		// Look for the chunk preceeding the last chunk within the
		// same stream.
		for (ulOffsetIdx = m_ulNumOffsets - 2;
		     ulOffsetIdx != 0;
		     ulOffsetIdx--)
		{
		    if (m_pOffsetMapIndex[ulOffsetIdx]->uStream == uStrmIdx)
		    {
			ulMaxOffsetGap += 
			    m_pOffsetMapIndex[ulOffsetIdx + 1]->ulOffset -
			    m_pOffsetMapIndex[ulOffsetIdx]->ulOffset;
			break;
		    }
		}
	    }
	}
    }

    // Compute time mappings
    if (SUCCEEDED(retVal))
    {
	UINT16 uCurrStrmIdx;
	UINT32 ulOtherStreamsMinTime;
	UINT32 ulBytesForChunk;
	UINT32 ulTimeEstimateLimit = MAX_UINT32;

	// We'll be tracking in stream index we expect to encounter next
	// in the master offset map.  This helps determine temporal gaps
	// in subsequent stream chunks and engage the serach for chunk
	// that downloaded bytes can be attributed to in terms of time.
	memset(pStreamOffsetIdx, 0, uNumStreams * sizeof(UINT32));

	// When a gap in stream chunk sequence is found the bytes of
	// downloaded chunk need to be attributed to a forward chunk that
	// closes the gap.
	// The transfers of chunk bytes to forward chunk are recorded in
	// pOffsetDeferredBytes array.
	// We clear the array to 0 prior to starting scan of offsets.
	memset(pOffsetDeferredBytes, 0, m_ulNumOffsets * sizeof(UINT32));

	for (ulOffsetIdx = 0; ulOffsetIdx < m_ulNumOffsets; ulOffsetIdx++)
	{
	    // Determine the time limit we'll apply to this offset time estiamte
	    if (ulMaxOffsetGap != 0)
	    {
		ulTimeEstimateLimit = 
		    HXMulDiv(m_pOffsetMapIndex[ulOffsetIdx]->ulOffset - ulBaseOffset, 
			     ulFileDuration, 
			     ulMaxOffsetGap);
	    }

	    // Determine which stream we are dealing with at current offset
	    uCurrStrmIdx = m_pOffsetMapIndex[ulOffsetIdx]->uStream;

	    // Compute time mapping for current stream based on bytes
	    // attributed to this stream so far and stream's average bitrate.
	    m_pOffsetMapIndex[ulOffsetIdx]->ulStreamTime =
		HXMulDiv(pStreamBytes[uCurrStrmIdx], 
			 8000, 
			 m_pStreamAvgBitrate[uCurrStrmIdx]);

	    // Limit the stream time based on file offset span estimates
	    if (m_pOffsetMapIndex[ulOffsetIdx]->ulStreamTime > ulTimeEstimateLimit)
	    {
		m_pOffsetMapIndex[ulOffsetIdx]->ulStreamTime = ulTimeEstimateLimit;
	    }

	    // Find the minimum number of stream bytes for other streams
	    ulOtherStreamsMinTime = MAX_UINT32;

	    // Find the minimum time represented by other streams that have not 
	    // yet ended (still have offsets ahead of current offset).
	    for (uStrmIdx = 0; 
		 uStrmIdx < m_uNumStreams; 
		 uStrmIdx++)
	    {
		if (uStrmIdx != uCurrStrmIdx)
		{
		    if (pTrackManager->IsStreamTrackActive(uStrmIdx) &&
			(pStreamLastOffset[uStrmIdx] >= m_pOffsetMapIndex[ulOffsetIdx]->ulOffset))
		    {
			UINT32 ulStreamTime = 
			    HXMulDiv(pStreamBytes[uStrmIdx], 
				     8000, 
				     m_pStreamAvgBitrate[uStrmIdx]);

			if (ulStreamTime < ulOtherStreamsMinTime)
			{
			    ulOtherStreamsMinTime = ulStreamTime;
			}
		    }
		}
	    }
		 
	    m_pOffsetMapIndex[ulOffsetIdx]->ulOtherStreamsMinTime = ulOtherStreamsMinTime;

	    // Limit the stream time based on file offset span estimates
	    if (m_pOffsetMapIndex[ulOffsetIdx]->ulOtherStreamsMinTime > ulTimeEstimateLimit)
	    {
		m_pOffsetMapIndex[ulOffsetIdx]->ulOtherStreamsMinTime = ulTimeEstimateLimit;
	    }

	    // Determine the number of bytes that can be potentially 
	    // moving the playback time forward
	    ulBytesForChunk = 0;
	    if ((ulOffsetIdx + 1) < m_ulNumOffsets)
	    {
		ulBytesForChunk =
		    pOffsetDeferredBytes[ulOffsetIdx] +
		    (m_pOffsetMapIndex[ulOffsetIdx + 1]->ulOffset -
		     m_pOffsetMapIndex[ulOffsetIdx]->ulOffset);
	    }

	    // Below will assert only if there is an error in this algorithm.
	    // Since we expect in-stream idecies in sequence, finding one that 
	    // is smaller than our expectation should not be possible.
	    HX_ASSERT(pOffsetInStrmIdx[ulOffsetIdx] >= pStreamOffsetIdx[uCurrStrmIdx]);

	    // Evaluate if there is a gap in stream chunk sequence.
	    // We conclude there is a gap if expected in-stream index is different
	    // than in-stream index of current offset (chunk).
	    m_pOffsetMapIndex[ulOffsetIdx]->bOutOfSequence = 
		(pStreamOffsetIdx[uCurrStrmIdx] != pOffsetInStrmIdx[ulOffsetIdx]);

	    // If this chunk is out of sequence (creates a sequence gap), 
	    // we'll look for a chunk that preceeds this chunk and
	    // can thus potentially contribute to playback time (bytes) 
	    // at the time of completion of its loading.  In case a gap of 
	    // multiple chunks exists, precedence is given to chunks closer 
	    // temporally to the gap causing chunk.  Not doing so would result
	    // in mapping error.
	    if (m_pOffsetMapIndex[ulOffsetIdx]->bOutOfSequence)
	    {
		UINT32 ulForwardOffsetIdx;
		UINT32 ulMaxPreceedingInStrmIdx = 0;
		UINT32 ulMaxPreceedingOffsetIdx = 0;

		for (ulForwardOffsetIdx = ulOffsetIdx + 1;
		     ulForwardOffsetIdx < m_ulNumOffsets;
		     ulForwardOffsetIdx++)
		{
		    // We must consider only the offsets (chunks) in current 
		    // stream
		    if (m_pOffsetMapIndex[ulForwardOffsetIdx]->uStream == 
			uCurrStrmIdx)
		    {
			if (pOffsetInStrmIdx[ulForwardOffsetIdx] < 
			    pOffsetInStrmIdx[ulOffsetIdx])
			{
			    // We found a chunk that fills the created gap
			    // See if it is closer than the one we have already found
			    if (pOffsetInStrmIdx[ulForwardOffsetIdx] >= 
				ulMaxPreceedingInStrmIdx)
			    {
				ulMaxPreceedingInStrmIdx = 
				    pOffsetInStrmIdx[ulForwardOffsetIdx];
				ulMaxPreceedingOffsetIdx =
				    ulForwardOffsetIdx;

				if ((ulMaxPreceedingInStrmIdx + 1) == 
				    pOffsetInStrmIdx[ulOffsetIdx])
				{
				    // We found the chunk immediately preceeding the
				    // gap causing chunk.
				    // No need to look any further
				    break;
				}
			    }
			}
		    }	
		}

		if (ulMaxPreceedingOffsetIdx != 0)
		{
		    // We found the chunk to attribute the time (bytes) of the
		    // temporal gap causing chunk.
		    pOffsetDeferredBytes[ulMaxPreceedingOffsetIdx] += 
			ulBytesForChunk;

		    // We will not be attributing any time (bytes) to this 
		    // chunk since it is preceeded by a temporal gap and its
		    // contribution to playback time must be deferred until
		    // the gap is closed.
		    ulBytesForChunk = 0;
		}
		else
		{
		    // We cannot find any forward chunks closing the gap.
		    // This must mean we have closed the gap with previous 
		    // chunks.
		    // Restore the stream index to normal sequencing.
		    pStreamOffsetIdx[uCurrStrmIdx] = 
			pOffsetInStrmIdx[ulOffsetIdx] + 1;

		    m_pOffsetMapIndex[ulOffsetIdx]->bOutOfSequence = FALSE;
		}
	    }
	    else
	    {
		// Offset (chunk) is mot out of temporal sequence.
		// Increment the in-stream index to expect next by one.
		pStreamOffsetIdx[uCurrStrmIdx]++;
	    }

	    // Move forward stream time maintained in bytes
	    pStreamBytes[uCurrStrmIdx] += ulBytesForChunk;
	}
    }

    HX_VECTOR_DELETE(pStreamOffsetIdx);
    HX_VECTOR_DELETE(pStreamBytes);
    HX_VECTOR_DELETE(pStreamLastOffset);
    HX_VECTOR_DELETE(pOffsetInStrmIdx);
    HX_VECTOR_DELETE(pOffsetDeferredBytes);

    if (FAILED(retVal))
    {
	Clear();
    }

    return retVal;
}

/****************************************************************************
 *  Map
 */
HX_RESULT CQTOffsetToTimeMapper::Map(UINT32 ulOffset, UINT32& ulTimeOut)
{
    UINT32 ulTime = 0;
    HX_RESULT retVal = HXR_FAIL;

    if (m_pOffsetMapIndex)
    {
	UINT32 ulOffsetIdx = m_ulOffsetIdx;
	UINT32 ulMapOffset;

	// Look for the chunk that contains specified file offset

	// Scan Forward for appropriate offset index
	do
	{
	    m_ulOffsetIdx = ulOffsetIdx;
	    ulOffsetIdx++;
	} while ((ulOffsetIdx < m_ulNumOffsets) &&
	         (m_pOffsetMapIndex[ulOffsetIdx]->ulOffset <= ulOffset));

	ulOffsetIdx = m_ulOffsetIdx;

	// Scan Backwards for appropriate index offset
	while ((m_ulOffsetIdx != 0) &&
	       (m_pOffsetMapIndex[m_ulOffsetIdx]->ulOffset > ulOffset))
	{
	    m_ulOffsetIdx--;
	} 

	// Obtain offset for the found chunk
	ulMapOffset = m_pOffsetMapIndex[m_ulOffsetIdx]->ulOffset;

	if (ulMapOffset <= ulOffset)
	{
	    UINT32 ulChunkOffset = ulOffset - ulMapOffset;
	    // If the chunk is out of sequence it cannot advance the playback 
	    // time-line as it is preceeded by temporal gap which must be
	    // closed first.
	    UINT32 ulInChunkTime = (m_pOffsetMapIndex[m_ulOffsetIdx]->bOutOfSequence ?
				    0 :
				    HXMulDiv(
					ulChunkOffset, 
					8000,
					m_pStreamAvgBitrate[m_pOffsetMapIndex[m_ulOffsetIdx]->uStream]));
	    UINT32 ulLimitTime = m_ulFileDuration;

	    // Compute stream time of the specified file offset based on 
	    // stream bitrate
	    ulTime = m_pOffsetMapIndex[m_ulOffsetIdx]->ulStreamTime + ulInChunkTime;

	    // Set the time estimate limit to be the time for the next offset 
	    // in the map.
	    if ((m_ulOffsetIdx + 1) < m_ulNumOffsets)
	    {
		ulLimitTime = m_pOffsetMapIndex[m_ulOffsetIdx + 1]->ulStreamTime;
	    }

	    // The playable time is the lesser one between
	    // the stream time the specified file offset translates into and 
	    // minimum time for other streams at the specified file offset.
	    if (ulTime > m_pOffsetMapIndex[m_ulOffsetIdx]->ulOtherStreamsMinTime)
	    {
		ulTime = m_pOffsetMapIndex[m_ulOffsetIdx]->ulOtherStreamsMinTime;
	    }

	    if (ulTime > ulLimitTime)
	    {
		ulTime = ulLimitTime;
	    }
	}
	else
	{
	    // File offset is before any media data
	    HX_ASSERT(m_ulOffsetIdx == 0);

	    ulTime = 0;
	}

	ulTimeOut = ulTime;

	retVal = HXR_OK;
    }

    return retVal;
}

/****************************************************************************
 *  Private Methods
 */
/****************************************************************************
 *  Clear
 */
void CQTOffsetToTimeMapper::Clear(void)
{
    m_ulOffsetIdx = 0;

    m_ulNumOffsets = 0;
    HX_VECTOR_DELETE(m_pOffsetMap);
    HX_VECTOR_DELETE(m_pOffsetMapIndex);

    m_uNumStreams = 0;
    HX_VECTOR_DELETE(m_pStreamAvgBitrate);
}

UINT32 CQTOffsetToTimeMapper::HXMulDiv(UINT32 left, UINT32 right, UINT32 bottom)
{
    double res = double(left) * double(right) / double(bottom);
    if (res > MAX_UINT32)
    {
        HX_ASSERT(res <= MAX_UINT32); // overflow
        return MAX_UINT32;
    }
    return UINT32(res);
}

int CQTOffsetToTimeMapper::compareOffsetMapEntry(const void *elem1, const void *elem2)
{
    ULONG32 ulOffset1 = ((QTOffsetMapEntry*) elem1)->ulOffset;
    ULONG32 ulOffset2 = ((QTOffsetMapEntry*) elem2)->ulOffset;

    if (ulOffset1 < ulOffset2)
    {
	return -1;
    }
    else if (ulOffset1 > ulOffset2)
    {
	return 1;
    }
    
    return 0;
}





