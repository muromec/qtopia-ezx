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
// #define _IGNORE_MP4_AUDIO

#define TRACK_SDP_CHUNK_SIZE	512

/****************************************************************************
 *  Includes
 */
#include "qtatmmgs.h"
#include "qttrkmgr.h"
#include "mp4desc.h"
#include "hxstrutl.h"

#include "rtptypes.h"
#include "rtsputil.h"

#ifndef QTCONFIG_SPEED_OVER_SIZE
#include "qtatmmgs_inline.h"
#endif	// QTCONFIG_SPEED_OVER_SIZE

/****************************************************************************
 *  Track Edit Manager
 */
#define QT_MAX_MOVIE_DURATION	0xFFFFFFFF

/****************************************************************************
 *  Constructor/Destructor
 */
CQT_TrackEdit_Manager::CQT_TrackEdit_Manager(void)
    : m_pEditListAtom(NULL)
    , m_ulMovieTimeScale(1)
    , m_ulMediaTimeScale(1)
    , m_ulNumEdits(0)
    , m_ulCurrentEditIdx(0)
    , m_ulCurrentEditTime(0)
    , m_ulCurrentInEditTime(0)
    , m_ulCurrentEditDuration(QT_MAX_MOVIE_DURATION)
    , m_ulCurrentMediaStartTime(0)
{
    ;
}

CQT_TrackEdit_Manager::~CQT_TrackEdit_Manager(void)
{
    HX_RELEASE(m_pEditListAtom);
}

/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CQT_TrackEdit_Manager::Init(CQTAtom* pAtom,
				      ULONG32 ulMovieTimeScale,
				      ULONG32 ulMediaTimeScale)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(ulMovieTimeScale);
    HX_ASSERT(ulMediaTimeScale);
    
    m_ulMovieTimeScale = ulMovieTimeScale;
    m_ulMediaTimeScale = ulMediaTimeScale;
    m_ulNumEdits = 0;
    
    HX_RELEASE(m_pEditListAtom);

    if (pAtom)
    {
	if (pAtom->GetType() == QT_elst)
	{
	    m_pEditListAtom = (CQT_elst_Atom*) pAtom;
	}
	else
	{
	    m_pEditListAtom = (CQT_elst_Atom*) pAtom->FindPresentChild(QT_elst);
	}

	if (m_pEditListAtom)
	{
	    m_pEditListAtom->AddRef();
	    m_ulNumEdits = m_pEditListAtom->Get_NumEntries();
	}
    }

    if (!EstablishByTime(0))
    {
	retVal = HXR_FAIL;

	// Reset State
	HX_RELEASE(m_pEditListAtom);
	m_ulNumEdits = 0;
	EstablishByTime(0);
    }

    return retVal;
}


/****************************************************************************
 *  EstablishByMovieTime
 */
HXBOOL CQT_TrackEdit_Manager::EstablishByMediaTime(ULONG32 ulMediaTime)
{
    m_ulCurrentEditIdx = 0;
    m_ulCurrentEditTime = 0;
    m_ulCurrentInEditTime = ulMediaTime;

    if (m_ulNumEdits > 0)
    {
	m_ulCurrentEditDuration = MovieToMediaUnits(
				    m_pEditListAtom->Get_TrackDuration(0));
	m_ulCurrentMediaStartTime = m_pEditListAtom->Get_MediaTime(0);
	return SequenceToEdit();
    }
    else
    {
	m_ulCurrentEditDuration = QT_MAX_MOVIE_DURATION;
	m_ulCurrentMediaStartTime = 0;
    }

    return TRUE;
}

/****************************************************************************
 *  Private Methods
 */
/****************************************************************************
 *  SequenceToEdit
 */
HXBOOL CQT_TrackEdit_Manager::SequenceToEdit(void)
{
    while ((m_ulCurrentMediaStartTime == QT_EMPTY_EDIT) ||
	   (m_ulCurrentInEditTime >= m_ulCurrentEditDuration))
    {
	m_ulCurrentEditIdx++;

	if (m_ulCurrentEditIdx < m_ulNumEdits)
	{
	    if (m_ulCurrentMediaStartTime != QT_EMPTY_EDIT)
	    {
		m_ulCurrentInEditTime -= m_ulCurrentEditDuration;
	    }
	    m_ulCurrentEditTime += m_ulCurrentEditDuration;
	    m_ulCurrentEditDuration = MovieToMediaUnits(
					m_pEditListAtom->Get_TrackDuration(
					    m_ulCurrentEditIdx));
	    m_ulCurrentMediaStartTime = m_pEditListAtom->Get_MediaTime(
					    m_ulCurrentEditIdx);
	}
	else
	{
	    m_ulCurrentEditIdx--;
	    return FALSE;
	}
    }

    return TRUE;
}


/****************************************************************************
 *  Sample To Chunk Manager
 */
#define QT_MAX_SAMPLES_PER_CHUNK	0xFFFFFFFF
#define QT_NULL_CHUNK_NUM		0

/****************************************************************************
 *  Constructor/Destructor
 */
CQT_SampleToChunk_Manager::CQT_SampleToChunk_Manager(void)
    : m_pSampleToChunkAtom(NULL)
    , m_ulNumEntries(0)
    , m_ulSampleInChunkNum(0)
    , m_ulCurrentChunk(0)
    , m_ulNextEntryChunk(QT_NULL_CHUNK_NUM)
    , m_ulCurrentEntryIdx(0)
    , m_ulSamplesPerChunk(QT_MAX_SAMPLES_PER_CHUNK)
    , m_ulSampleNumber(0)
    , m_ulSampleDescIdx(QT_BAD_IDX)
#ifdef _STCO_ZERO_BASED_IQ
    , m_ulChunkNumOffset(0)
#endif	// _STCO_ZERO_BASED_IQ
{
    ;
}

CQT_SampleToChunk_Manager::~CQT_SampleToChunk_Manager()
{
    HX_RELEASE(m_pSampleToChunkAtom);
}

/****************************************************************************
 *  Init
 */
HX_RESULT CQT_SampleToChunk_Manager::Init(CQTAtom* pAtom)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pSampleToChunkAtom);
    m_ulNumEntries = 0;
    m_ulSampleNumber = 0;

    if (pAtom)
    {
	if (pAtom->GetType() == QT_stsc)
	{
	    m_pSampleToChunkAtom = (CQT_stsc_Atom*) pAtom;
	}
	else
	{
	    m_pSampleToChunkAtom = (CQT_stsc_Atom*) pAtom->FindPresentChild(QT_stsc);
	}

	if (m_pSampleToChunkAtom)
	{
	    m_pSampleToChunkAtom->AddRef();
	    m_ulNumEntries = m_pSampleToChunkAtom->Get_NumEntries();
	}
    }

    if (m_ulNumEntries == 0)
    {
	EstablishBySample(0);
	retVal = HXR_NO_DATA;
    }
    else
    {
	m_ulSampleNumber = 2;  // Force full initialization
	retVal = EstablishBySample(1) ? HXR_OK : HXR_FAIL;

#ifdef _STCO_ZERO_BASED_IQ
	m_ulChunkNumOffset = 0;

	if (retVal == HXR_OK)
	{
	    if ((m_ulCurrentChunk == 0) ||
		(m_ulSampleDescIdx == ((ULONG32) -1)))
	    {
		m_ulCurrentChunk = 1;
		m_ulNextEntryChunk++;
		m_ulSampleDescIdx++;
		m_ulChunkNumOffset = 1;
	    }
	}
#endif	// _STCO_ZERO_BASED_IQ
    }

    return retVal;
}

/****************************************************************************
 *  EstablishBySample
 */
HXBOOL CQT_SampleToChunk_Manager::EstablishBySample(ULONG32 ulSampleNum)
{
    if (m_ulNumEntries > 0)
    {
	HX_ASSERT(ulSampleNum);

	if (ulSampleNum >= m_ulSampleNumber)
	{
	    // Search Forward
	    m_ulSampleInChunkNum += (ulSampleNum - m_ulSampleNumber);
	    m_ulSampleNumber = ulSampleNum;

	    return SequenceToChunk();
	}
	else if (ulSampleNum > (m_ulSampleNumber >> 1))
	{
	    // Search in Reverse
	    m_ulSampleInChunkNum = m_ulSamplesPerChunk -
				   m_ulSampleInChunkNum +
				   m_ulSampleNumber -
				   ulSampleNum +
				   1;
	    m_ulSampleNumber = ulSampleNum;

	    if (SequenceReverseToChunk())
	    {
		m_ulSampleInChunkNum = m_ulSamplesPerChunk -
				       m_ulSampleInChunkNum +
				       1;

		return TRUE;
	    }
	}
	else
	{
	    // Search Forward from the beginning
	    m_ulSampleInChunkNum = ulSampleNum;
	    m_ulCurrentEntryIdx = 0;
	    m_ulCurrentChunk = m_pSampleToChunkAtom->Get_FirstChunk(0)
#ifdef _STCO_ZERO_BASED_IQ
			       + m_ulChunkNumOffset
#endif	// _STCO_ZERO_BASED_IQ
			       ;
	    m_ulSamplesPerChunk = m_pSampleToChunkAtom->Get_SamplesPerChunk(0);
	    m_ulSampleDescIdx = m_pSampleToChunkAtom->Get_SampleDescID(0) - 1
#ifdef _STCO_ZERO_BASED_IQ
			        + m_ulChunkNumOffset
#endif	// _STCO_ZERO_BASED_IQ
			       ;

	    if (m_ulNumEntries > 1)
	    {
		m_ulNextEntryChunk = m_pSampleToChunkAtom->
		    Get_FirstChunk(1)
#ifdef _STCO_ZERO_BASED_IQ
		    + m_ulChunkNumOffset
#endif	// _STCO_ZERO_BASED_IQ
		    ;
	    }
	    else
	    {
		// No more chunk entries
		m_ulNextEntryChunk = QT_NULL_CHUNK_NUM;
	    }

	    m_ulSampleNumber = ulSampleNum;

	    return SequenceToChunk();
	}
    }	
    else
    {
	m_ulSampleNumber = 0;
	m_ulSampleInChunkNum = 0;
	m_ulCurrentChunk = 0;
	m_ulSamplesPerChunk = QT_MAX_SAMPLES_PER_CHUNK;
	m_ulSampleDescIdx = QT_BAD_IDX;
#ifdef _STCO_ZERO_BASED_IQ
	m_ulChunkNumOffset = 0;
#endif	// _STCO_ZERO_BASED_IQ
    }

    return FALSE;
}

/****************************************************************************
 *  Private Methods
 */
/****************************************************************************
 *  SequenceToChunk
 */
#ifdef STCMGR_USE_MODULUS_SEQUENCING

// This sequencing algorithm has better efficiency (than non-modulus 
// sequencing) on files containing many chunks per table entry and/or those 
// whose progression through the table is irratic (has skipps).
HXBOOL CQT_SampleToChunk_Manager::SequenceToChunk(void)
{
    ULONG32 ulTargetChunk;
    HXBOOL bEntryIdxChanged;

    if (m_ulSampleInChunkNum > m_ulSamplesPerChunk)
    {
	bEntryIdxChanged = FALSE;

	do
	{
	    if (m_ulSamplesPerChunk == 0)
	    {
		ulTargetChunk = m_ulCurrentChunk + 1;
	    }
	    else
	    {
		ulTargetChunk = m_ulCurrentChunk +
		    		(m_ulSampleInChunkNum - 1) /
				m_ulSamplesPerChunk;
	    }

	    if ((ulTargetChunk >= m_ulNextEntryChunk) &&
		(m_ulNextEntryChunk != QT_NULL_CHUNK_NUM))
	    {
		m_ulSampleInChunkNum -= (m_ulNextEntryChunk -
					 m_ulCurrentChunk) *
					m_ulSamplesPerChunk;
		m_ulCurrentChunk = m_ulNextEntryChunk;
		m_ulCurrentEntryIdx++;
		bEntryIdxChanged = TRUE;

		m_ulSamplesPerChunk = m_pSampleToChunkAtom->
		    Get_SamplesPerChunk(m_ulCurrentEntryIdx);
		
		if ((m_ulCurrentEntryIdx + 1) < m_ulNumEntries)
		{
		    m_ulNextEntryChunk = m_pSampleToChunkAtom->
			Get_FirstChunk(m_ulCurrentEntryIdx + 1);
		}
		else
		{
		    // No more chunk entries
		    m_ulNextEntryChunk = QT_NULL_CHUNK_NUM;
		}
	    }
	    else
	    {
		m_ulCurrentChunk = ulTargetChunk;
		if (m_ulSamplesPerChunk != 0)
		{  
		    m_ulSampleInChunkNum = (m_ulSampleInChunkNum - 1) %
					    m_ulSamplesPerChunk +
					    1;
		}
		else
		{
		    // encountered empty chunk at the end of table
		    return FALSE;
		}
	    }
	} while (m_ulSampleInChunkNum > m_ulSamplesPerChunk);

	if (bEntryIdxChanged)
	{
	    m_ulSampleDescIdx = m_pSampleToChunkAtom->
		Get_SampleDescID(m_ulCurrentEntryIdx) - 1
#ifdef _STCO_ZERO_BASED_IQ
		+ m_ulChunkNumOffset
#endif	// _STCO_ZERO_BASED_IQ
			       ;
	}
    }

    return TRUE;
}

#else	// STCMGR_USE_MODULUS_SEQUENCING

// This sequencing algorithm is more efficient for files
// containing fewer chunks per table entry and/or those
// whose progression through the chunks involves fewer
// skipps.
HXBOOL CQT_SampleToChunk_Manager::SequenceToChunk(void)
{
    HXBOOL bEntryIdxChanged;

    if (m_ulSampleInChunkNum > m_ulSamplesPerChunk)
    {
	bEntryIdxChanged = FALSE;

	do
	{
	    m_ulSampleInChunkNum = m_ulSampleInChunkNum - m_ulSamplesPerChunk;
	    m_ulCurrentChunk++;
	    
	    if (m_ulCurrentChunk == m_ulNextEntryChunk)
	    {
		m_ulCurrentEntryIdx++;
		bEntryIdxChanged = TRUE;
		
		m_ulSamplesPerChunk = m_pSampleToChunkAtom->
		    Get_SamplesPerChunk(m_ulCurrentEntryIdx);
		
		if ((m_ulCurrentEntryIdx + 1) < m_ulNumEntries)
		{
		    m_ulNextEntryChunk = m_pSampleToChunkAtom->
			Get_FirstChunk(m_ulCurrentEntryIdx + 1)
#ifdef _STCO_ZERO_BASED_IQ
			+ m_ulChunkNumOffset
#endif	// _STCO_ZERO_BASED_IQ
			;
		}
		else
		{
		    // No more chunk entries
		    m_ulNextEntryChunk = QT_NULL_CHUNK_NUM;
		}
	    }
	    else if ((m_ulSamplesPerChunk == 0) && 
		(m_ulNextEntryChunk == QT_NULL_CHUNK_NUM))
	    {
		return FALSE;
	    }
	} while (m_ulSampleInChunkNum > m_ulSamplesPerChunk);

	if (bEntryIdxChanged)
	{
	    m_ulSampleDescIdx = m_pSampleToChunkAtom->
		Get_SampleDescID(m_ulCurrentEntryIdx) - 1
#ifdef _STCO_ZERO_BASED_IQ
		+ m_ulChunkNumOffset
#endif	// _STCO_ZERO_BASED_IQ
		;
	}
    }

    return TRUE;
}

#endif	// STCMGR_USE_MODULUS_SEQUENCING

/****************************************************************************
 *  SequenceReverseToChunk
 */
HXBOOL CQT_SampleToChunk_Manager::SequenceReverseToChunk(void)
{
    ULONG32 ulEntryFirstChunk;
    HXBOOL bEntryIdxChanged;

    if (m_ulSampleInChunkNum > m_ulSamplesPerChunk)
    {
	bEntryIdxChanged = FALSE;

	ulEntryFirstChunk = m_pSampleToChunkAtom->
	    Get_FirstChunk(m_ulCurrentEntryIdx);

	do
	{
	    m_ulSampleInChunkNum = m_ulSampleInChunkNum - m_ulSamplesPerChunk;
	    m_ulCurrentChunk--;
	    
	    if (m_ulCurrentChunk < ulEntryFirstChunk)
	    {
		if (m_ulCurrentEntryIdx == 0)
		{
		    return FALSE;
		}

		m_ulCurrentEntryIdx--;
		bEntryIdxChanged = TRUE;
		
		m_ulSamplesPerChunk = m_pSampleToChunkAtom->
		    Get_SamplesPerChunk(m_ulCurrentEntryIdx);
		ulEntryFirstChunk = m_pSampleToChunkAtom->
		    Get_FirstChunk(m_ulCurrentEntryIdx);
	    }
	} while (m_ulSampleInChunkNum > m_ulSamplesPerChunk);

	if (bEntryIdxChanged)
	{
	    m_ulSampleDescIdx = m_pSampleToChunkAtom->
		Get_SampleDescID(m_ulCurrentEntryIdx) - 1
#ifdef _STCO_ZERO_BASED_IQ
		+ m_ulChunkNumOffset
#endif	// _STCO_ZERO_BASED_IQ
		;
	}

	if ((m_ulCurrentEntryIdx + 1) < m_ulNumEntries)
	{
	    m_ulNextEntryChunk = m_pSampleToChunkAtom->
		Get_FirstChunk(m_ulCurrentEntryIdx + 1)
#ifdef _STCO_ZERO_BASED_IQ
		+ m_ulChunkNumOffset
#endif	// _STCO_ZERO_BASED_IQ
		;
	}
	else
	{
	    // No more chunk entries
	    m_ulNextEntryChunk = QT_NULL_CHUNK_NUM;
	}
    }

    return TRUE;
}


#define INVALID_COMP_SAMPLE_NUM	    0xFFFFFFFF
#define QT_INVALID_KEY_MEDIA_TIME   0xFFFFFFFF

/****************************************************************************
 *  Time To Sample Manager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQT_TimeToSample_Manager::CQT_TimeToSample_Manager(void)
    : m_pTimeToSampleAtom(NULL)
    , m_pCompOffsetAtom(NULL)
    , m_pSyncSampleAtom(NULL)
    , m_ulNumEntries(0)
    , m_ulCurrentEntryIdx(0)
    , m_ulSampleNumber(0)
    , m_ulSamplesLeftInEntry(0)
    , m_ulSampleDuration(0)
    , m_ulLastSyncTime(0)
    , m_ulNumCompEntries(0)
    , m_ulCurrentCompEntryIdx(0)
    , m_ulCompSampleNumber(INVALID_COMP_SAMPLE_NUM)
    , m_ulSamplesLeftInCompEntry(0)
    , m_ulCompOffset(0)
    , m_ulNumSyncEntries(0)
    , m_ulCurrentSyncEntryIdx(0)
    , m_ulSyncSampleNumber(0)
    , m_ulLastPreTargetSyncMediaTime(QT_INVALID_KEY_MEDIA_TIME)
#ifdef _STSS_ZERO_BASED_IQ
    , m_ulKeyFrameNumOffset(0)
#endif	// _STSS_ZERO_BASED_IQ
{
    ;
}

CQT_TimeToSample_Manager::~CQT_TimeToSample_Manager()
{
    HX_RELEASE(m_pTimeToSampleAtom);
    HX_RELEASE(m_pCompOffsetAtom);
    HX_RELEASE(m_pSyncSampleAtom);
}

/****************************************************************************
 *  Init
 */
HX_RESULT CQT_TimeToSample_Manager::Init(CQTAtom* pAtom)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pTimeToSampleAtom);
    HX_RELEASE(m_pCompOffsetAtom);
    HX_RELEASE(m_pSyncSampleAtom);
    
    m_ulNumEntries = 0;
    m_ulCompSampleNumber = INVALID_COMP_SAMPLE_NUM;

    if (pAtom)
    {
	if (pAtom->GetType() == QT_stts)
	{
	    m_pTimeToSampleAtom = (CQT_stts_Atom*) pAtom;
	}
	else
	{
	    m_pTimeToSampleAtom = (CQT_stts_Atom*) pAtom->
				    FindPresentChild(QT_stts);
	    m_pCompOffsetAtom = (CQT_ctts_Atom*) pAtom->
				    FindPresentChild(QT_ctts);
	}

	if (m_pTimeToSampleAtom)
	{
	    m_pTimeToSampleAtom->AddRef();
	    m_ulNumEntries = m_pTimeToSampleAtom->Get_NumEntries();
	}

	if (m_pCompOffsetAtom)
	{
	    m_pCompOffsetAtom->AddRef();
	    m_ulNumCompEntries = m_pCompOffsetAtom->Get_NumEntries();
	}
    }

    if (m_ulNumEntries > 0)
    {
	if (pAtom != m_pTimeToSampleAtom)
	{
	    m_pSyncSampleAtom = (CQT_stss_Atom*) pAtom->
				    FindPresentChild(QT_stss);
	    if (m_pSyncSampleAtom)
	    {
		ULONG32 ulNumSyncEntries = 0;

		ulNumSyncEntries = m_pSyncSampleAtom->Get_NumEntries();

#ifdef _STSS_ZERO_BASED_IQ
		if (ulNumSyncEntries > 0)
		{
		    ULONG32 ulKeyFrameSample;

		    ulKeyFrameSample = m_pSyncSampleAtom->
					    Get_SampleNum(0);
		    if (ulKeyFrameSample == 0)
		    {
			m_ulKeyFrameNumOffset = 1;
		    }
		}
#endif	// _STSS_ZERO_BASED_IQ

#ifdef _STSS_TRACK_SYNC
		m_ulNumSyncEntries = ulNumSyncEntries;
#endif // _STSS_TRACK_SYNC

		m_pSyncSampleAtom->AddRef();
	    }
	}
    }

    retVal = EstablishByMediaTime(0) ? HXR_OK : HXR_NO_DATA;

    return retVal;
}

/****************************************************************************
 *  EstablishAtKeyByMediaTime
 */
HXBOOL CQT_TimeToSample_Manager::EstablishAtKeyByMediaTime(ULONG32 ulMediaTime)
{
    ULONG32 ulSyncEntryIdx;
    ULONG32 ulKeyFrameSample;
    ULONG32 ulNumSyncEntries = 0;
    HXBOOL bEstablished = FALSE;

    if (EstablishByMediaTime(ulMediaTime))
    {
	ulNumSyncEntries = m_ulNumSyncEntries;
	if ((ulNumSyncEntries == 0) && m_pSyncSampleAtom)
	{
	    ulNumSyncEntries = m_pSyncSampleAtom->Get_NumEntries();
	}

	bEstablished = (ulNumSyncEntries == 0);

	for (ulSyncEntryIdx = m_ulCurrentSyncEntryIdx; 
	     ulSyncEntryIdx < ulNumSyncEntries;
	     ulSyncEntryIdx++)
	{
	    ulKeyFrameSample = m_pSyncSampleAtom->
		Get_SampleNum(ulSyncEntryIdx) + m_ulKeyFrameNumOffset;
	    while (ulKeyFrameSample >= m_ulSampleNumber)
	    {
		if (ulKeyFrameSample == m_ulSampleNumber)
		{
		    // Key Sample lined up with the Sample
		    return TRUE;
		}

		// We'll try to line up the Sample with the key sample
		if (!AdvanceBySample())
		{
		    return FALSE;
		}

		// Keep track of how for beyond initally requested time
		// the key frame was found.
		m_ulLastSyncTime += m_ulSampleDuration;
	    }
	}
    }

    return bEstablished;
}

/****************************************************************************
 *  EstablishByMediaTime
 */
HXBOOL CQT_TimeToSample_Manager::EstablishByMediaTime(ULONG32 ulMediaTime)
{
    HXBOOL bIsEstablished;
    ULONG32 ulTargetMediaTime = ulMediaTime;

    m_ulCurrentEntryIdx = 0;
    m_ulLastSyncTime = 0;
    m_ulCurrentSyncEntryIdx = 0;
    m_ulLastPreTargetSyncMediaTime = QT_INVALID_KEY_MEDIA_TIME;
    
    if (m_ulNumEntries > 0)
    {
	m_ulSampleNumber = 0;
	m_ulSamplesLeftInEntry = m_pTimeToSampleAtom->Get_SampleCount(0);
	m_ulSampleDuration = m_pTimeToSampleAtom->Get_SampleDuration(0);
    }
    else
    {
	m_ulSampleNumber = 0;
	m_ulSamplesLeftInEntry = 0;
	m_ulSampleDuration = 0;
    }

    if (m_ulNumSyncEntries > 0)
    {
	m_ulSyncSampleNumber = m_pSyncSampleAtom->Get_SampleNum(0) +
			       m_ulKeyFrameNumOffset;
    }
    else
    {
	m_ulSyncSampleNumber = 0;
    }

    bIsEstablished = EstablishCompBySample(0);

    if (bIsEstablished)
    {
	bIsEstablished = AdvanceBySample();
    }

    if (bIsEstablished)
    {
	if (IsOnSyncSample())
	{
	    m_ulLastPreTargetSyncMediaTime = ulTargetMediaTime - ulMediaTime;
	}

	while (ulMediaTime > 0)
	{
	    if (IsOnSyncSample())
	    {
		m_ulLastPreTargetSyncMediaTime = ulTargetMediaTime - ulMediaTime;
	    }

	    if (ulMediaTime >= m_ulSampleDuration)
	    {
		ulMediaTime -= m_ulSampleDuration;
	    }
	    else
	    {
		m_ulLastSyncTime = m_ulSampleDuration - ulMediaTime;
		ulMediaTime = 0;
	    }

	    if (!AdvanceBySample())
	    {
		bIsEstablished = FALSE;
		break;
	    }
	}
    }

    return bIsEstablished;
}

/****************************************************************************
 *  GetLastPreTargetKeyMediaTime
 */
HXBOOL CQT_TimeToSample_Manager::GetLastPreTargetKeyMediaTime(ULONG32 &ulMediaTime)
{
    if (m_ulLastPreTargetSyncMediaTime != QT_INVALID_KEY_MEDIA_TIME)
    {
	ulMediaTime = m_ulLastPreTargetSyncMediaTime;
	return TRUE;
    }

    return FALSE;
}

/****************************************************************************
 *  EstablishCompByOffset
 */
HXBOOL CQT_TimeToSample_Manager::EstablishCompBySample(ULONG32 ulSampleNum)
{
    HXBOOL bIsEstablished = TRUE;

    if (m_ulNumCompEntries == 0)
    {
	return bIsEstablished;
    }

    if (m_ulCompSampleNumber >= ulSampleNum)
    {
	m_ulCompSampleNumber = 0;
	m_ulCurrentCompEntryIdx = 0;
	m_ulSamplesLeftInCompEntry = m_pCompOffsetAtom->Get_SampleCount(0);
	m_ulCompOffset = m_pCompOffsetAtom->Get_SampleOffset(0);
    }

    while (m_ulCompSampleNumber < ulSampleNum)
    {
	if (!(bIsEstablished = AdvanceCompBySample()))
	{
	    break;
	}
    }
	
    return bIsEstablished;
}


/****************************************************************************
 *  Sample Size Manager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQT_SampleSize_Manager::CQT_SampleSize_Manager(void)
    : m_pSampleSizeAtom(NULL)
    , m_ulGenericSize(0)
    , m_ulSampleSize(0)
    , m_ulChunkSampleOffset(0)
    , m_ulChunkSize(0)
    , m_ulNumEntries(0)
    , m_ulChunkStartSampleNum(QT_BAD_IDX)
    , m_ulSampleNum(QT_BAD_IDX)
{
    ;
}

CQT_SampleSize_Manager::~CQT_SampleSize_Manager()
{
    HX_RELEASE(m_pSampleSizeAtom);
}

/****************************************************************************
 *  Init
 */
HX_RESULT CQT_SampleSize_Manager::Init(CQTAtom *pAtom)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pSampleSizeAtom);
    m_ulGenericSize = 0;
    m_ulSampleSize = 0;
    m_ulChunkSampleOffset = 0;
    m_ulChunkSize = 0;
    m_ulNumEntries = 0;
    m_ulChunkStartSampleNum = QT_BAD_IDX;
    m_ulSampleNum = QT_BAD_IDX;

    if (pAtom)
    {
	if ((pAtom->GetType() == QT_stsz) ||
	    (pAtom->GetType() == QT_stz2))
	{
	    m_pSampleSizeAtom = (CQT_stsz_Atom*) pAtom;
	}
	else
	{
	    m_pSampleSizeAtom = (CQT_stsz_Atom*) pAtom->FindPresentChild(QT_stsz);
	    if (!m_pSampleSizeAtom)
	    {
		m_pSampleSizeAtom = (CQT_stz2_Atom*) pAtom->FindPresentChild(QT_stz2);
	    }
	}

	if (m_pSampleSizeAtom)
	{
	    m_pSampleSizeAtom->AddRef();
	    m_ulGenericSize = m_pSampleSizeAtom->Get_SampleSize();
	    m_ulNumEntries = m_pSampleSizeAtom->Get_NumEntries();
	}
    }

    if (m_ulGenericSize == 0)
    {
	if (m_ulNumEntries == 0)
	{
	    retVal = HXR_NO_DATA;
	}
    }
    else
    {
	m_ulSampleSize = m_ulGenericSize;
	// All samples have the same size - do not need the table
	HX_RELEASE(m_pSampleSizeAtom);
    }

    return retVal;
}

/****************************************************************************
 *  EstablishBySample
 */
HXBOOL CQT_SampleSize_Manager::EstablishBySample(ULONG32 ulSampleNum, 
					       ULONG32 ulChunkSampleNum,
					       ULONG32 ulSamplesPerChunk)
{
    ULONG32 ulChunkStartSampleNum;
    ULONG32 ulChunkEndSampleNum;

    if ((ulChunkSampleNum > 0) &&
	(ulSampleNum >= ulChunkSampleNum))
    {
	if (ulSamplesPerChunk < ulChunkSampleNum)
	{
	    ulSamplesPerChunk = ulChunkSampleNum;
	}

	if (m_ulGenericSize)
	{
	    m_ulChunkSampleOffset = (ulChunkSampleNum - 1) * m_ulGenericSize;
	    m_ulChunkSize = ulSamplesPerChunk * m_ulGenericSize;
	    return TRUE;
	}
	
	ulChunkStartSampleNum = ulSampleNum - ulChunkSampleNum;
	ulChunkEndSampleNum = ulChunkStartSampleNum + ulSamplesPerChunk;

	if (ulChunkEndSampleNum
	    <= m_ulNumEntries)
	{
	    // Convert to 0 based index for efficiency
	    ulSampleNum--;
	    
	    if (m_ulChunkStartSampleNum == ulChunkStartSampleNum)
	    {
		if (m_ulSampleNum == ulSampleNum)   // m_ulSampleNum is 1 based
		{
		    m_ulChunkSampleOffset += m_ulSampleSize;
		    m_ulSampleSize = m_pSampleSizeAtom->Get_SampleSize(m_ulSampleNum++);
		    return TRUE;
		}
		else if (m_ulSampleNum == (ulSampleNum + 1))
		{
		    // Already computed
		    return TRUE;
		}
	    }

	    // Convert to 0 based index for efficiency
	    ulChunkSampleNum--;
	    
	    m_ulSampleSize = m_pSampleSizeAtom->Get_SampleSize(ulSampleNum);
	    m_ulChunkStartSampleNum = ulChunkStartSampleNum;
	    m_ulSampleNum = ulSampleNum + 1;
	    
	    for (m_ulChunkSampleOffset = 0;
		 ulChunkStartSampleNum < ulSampleNum;
		 ulChunkStartSampleNum++)
	    {
		m_ulChunkSampleOffset += m_pSampleSizeAtom->
		    Get_SampleSize(ulChunkStartSampleNum);
	    }
	
	    /*** Not needed
	    m_ulChunkSize = m_ulChunkSampleOffset;
	    do
	    {
		m_ulChunkSize += m_pSampleSizeAtom->
		    Get_SampleSize(ulChunkStartSampleNum++);
	    } while (ulChunkStartSampleNum < ulChunkEndSampleNum);
	    ***/
	    
	    return TRUE;
	}
    }

    return FALSE;
}


/****************************************************************************
 *  Chunk To Offset Manager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQT_ChunkToOffset_Manager::CQT_ChunkToOffset_Manager(void)
    : m_pChunkToOffsetAtom(NULL)
    , m_ulChunkOffset(0)
    , m_ulNumEntries(0)
{
    ;
}

CQT_ChunkToOffset_Manager::~CQT_ChunkToOffset_Manager()
{
    HX_RELEASE(m_pChunkToOffsetAtom);
}

/****************************************************************************
 *  Main Interface
 */
HX_RESULT CQT_ChunkToOffset_Manager::Init(CQTAtom* pAtom)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pChunkToOffsetAtom);
    m_ulChunkOffset = 0;
    m_ulNumEntries = 0;

    if (pAtom)
    {
	if ((pAtom->GetType() == QT_stco) ||
	    (pAtom->GetType() == QT_co64))
	{
	    m_pChunkToOffsetAtom = (CQT_stco_Atom*) pAtom;
	}
	else
	{
	    m_pChunkToOffsetAtom = (CQT_stco_Atom*) pAtom->FindPresentChild(QT_stco);
	    if (!m_pChunkToOffsetAtom)
	    {
		m_pChunkToOffsetAtom = (CQT_stco_Atom*) pAtom->FindPresentChild(QT_co64);
	    }
	}

	if (m_pChunkToOffsetAtom)
	{
	    m_pChunkToOffsetAtom->AddRef();
	    m_ulNumEntries = m_pChunkToOffsetAtom->Get_NumEntries();
	}
    }

    if (m_ulNumEntries == 0)
    {
	retVal = HXR_FAIL;
	HX_RELEASE(m_pChunkToOffsetAtom);
    }
   
    return retVal;
}


/****************************************************************************
 *  Sample Description Manager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQT_SampleDescription_Manager::CQT_SampleDescription_Manager(void)
    : m_pSampleDescriptionAtom(NULL)
    , m_ulSampleDescIdx(QT_BAD_IDX)
    , m_pSampleDesc(NULL)
    , m_ulDataRefIdx(QT_BAD_IDX)
    , m_ulDataFormat(0)
    , m_ulRTPTimeScale(0)
    , m_lTimeStampOffset(0)
    , m_lSequenceNumOffset(0)
    , m_ulNumEntries(0)
    , m_ulDataRefIdxOffset(1)
{
    ;
}

CQT_SampleDescription_Manager::~CQT_SampleDescription_Manager()
{
    HX_RELEASE(m_pSampleDescriptionAtom);
}

/****************************************************************************
 *  Main Interface Manager
 */
HX_RESULT CQT_SampleDescription_Manager::Init(CQTAtom* pAtom)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pSampleDescriptionAtom);
    m_ulSampleDescIdx = QT_BAD_IDX;
    m_pSampleDesc = NULL;
    m_ulDataRefIdx = QT_BAD_IDX;
    m_ulDataFormat = 0;
    m_ulRTPTimeScale = 0;
    m_lTimeStampOffset = 0;
    m_lSequenceNumOffset = 0;
    m_ulNumEntries = 0;

    if (pAtom)
    {
	if (pAtom->GetType() == QT_stsd)
	{
	    m_pSampleDescriptionAtom = (CQT_stsd_Atom*) pAtom;
	}
	else
	{
	    m_pSampleDescriptionAtom = (CQT_stsd_Atom*) pAtom->FindPresentChild(QT_stsd);
	}

	if (m_pSampleDescriptionAtom)
	{
	    m_pSampleDescriptionAtom->AddRef();
	    m_ulNumEntries = m_pSampleDescriptionAtom->Get_NumEntries();
	}
    }

#ifdef _STSD_ZERO_BASED_IQ
    ULONG32 ulIdx;

    for (ulIdx = 0; ulIdx < m_ulNumEntries; ulIdx++)
    {
	if (EstablishByIdx(ulIdx))
	{
	    if (m_ulDataRefIdx == ((ULONG32) -1))
	    {
		m_ulDataRefIdxOffset = 0;
		break;
	    }
	}
    }

    m_ulSampleDescIdx = QT_BAD_IDX;
#endif // _STSD_ZERO_BASED_IQ

    retVal = EstablishByIdx(0) ? HXR_OK : HXR_FAIL;

   
    return retVal;
}

/****************************************************************************
 *  Private Methods
 */
HXBOOL CQT_SampleDescription_Manager::ParseSampleDescription(void)
{
    HXBOOL bSuccess = TRUE;
    ULONG32 ulTaggedEntryIdx = 0;
    CQT_stsd_Atom::TaggedEntry* pTaggedEntry;

    // Find The Needed Sample
    m_pSampleDesc = m_pSampleDescriptionAtom->
	Get_SampleDesc(m_ulSampleDescIdx);

    // Extract Needed Fixed Sample Fields
    m_ulDataRefIdx = m_pSampleDescriptionAtom->
	Get_DataRefIdx(m_pSampleDesc) - m_ulDataRefIdxOffset;
    m_ulDataFormat = m_pSampleDescriptionAtom->
	Get_DataFormat(m_pSampleDesc);

    m_ulRTPTimeScale = 0;
    m_lTimeStampOffset = 0;
    m_lSequenceNumOffset = 0;

    if (m_ulDataFormat == QT_rtp)
    {
	// Extract Needed Tagged Entries
	while ((pTaggedEntry = m_pSampleDescriptionAtom->
	    Get_TaggedEntry((CQT_stsd_Atom::HintArrayEntry *) m_pSampleDesc, 
			    ulTaggedEntryIdx)) != NULL)
	{
	    switch (CQTAtom::GetUL32(pTaggedEntry->pTag))
	    {
	    case QT_tims:
		m_ulRTPTimeScale = CQTAtom::GetUL32(pTaggedEntry->pData);
		break;
	    case QT_tsro:
		m_lTimeStampOffset = (LONG32) CQTAtom::GetUL32(pTaggedEntry->pData);
		break;
	    case QT_snro:
		m_lSequenceNumOffset = (LONG32) CQTAtom::GetUL32(pTaggedEntry->pData);
		break;
	    default:
		// nothing to do
		break;
	    }
	    
	    ulTaggedEntryIdx++;
	}
	bSuccess = (m_ulRTPTimeScale > 0);
    }

    return bSuccess;
}


/****************************************************************************
 *  Data Reference Manager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQT_DataReference_Manager::CQT_DataReference_Manager(void)
    : m_pDataReferenceAtom(NULL)
    , m_ulDataRefIdx(QT_BAD_IDX)
    , m_pDataRefName(NULL)
    , m_pClassFactory(NULL)
    , m_ulNumEntries(0)
{
    ;
}

CQT_DataReference_Manager::~CQT_DataReference_Manager()
{
    HX_RELEASE(m_pDataReferenceAtom);
    HX_RELEASE(m_pDataRefName);
    HX_RELEASE(m_pClassFactory);
}

/****************************************************************************
 *  Main Interface
 */
HX_RESULT CQT_DataReference_Manager::Init
(
    CQTAtom* pAtom,
    IHXCommonClassFactory* pClassFactory
)
{
    HX_RESULT retVal = HXR_OK;

    HX_RELEASE(m_pDataReferenceAtom);
    HX_RELEASE(m_pDataRefName);
    HX_RELEASE(m_pClassFactory);

    m_ulDataRefIdx = QT_BAD_IDX;
    m_ulNumEntries = 0;

    if (pAtom)
    {
	HX_ASSERT(pClassFactory);
	m_pClassFactory = pClassFactory;
	pClassFactory->AddRef();

	if (pAtom->GetType() == QT_dref)
	{
	    m_pDataReferenceAtom = (CQT_dref_Atom*) pAtom;
	}
	else
	{
	    m_pDataReferenceAtom = (CQT_dref_Atom*) pAtom->FindPresentChild(QT_dref);
	}

	if (m_pDataReferenceAtom)
	{
	    m_pDataReferenceAtom->AddRef();
	    m_ulNumEntries = m_pDataReferenceAtom->Get_NumEntries();
	}
    }

    retVal = EstablishByIdx(0) ? HXR_OK : HXR_FAIL;
   
    return retVal;
}

/****************************************************************************
 *  Private methods
 */
HXBOOL CQT_DataReference_Manager::ParseDataReference(void)
{
    CQT_dref_Atom::ArrayEntry* pRefEntry;
    HXBOOL bSuccess = TRUE;

    HX_RELEASE(m_pDataRefName);

    pRefEntry = m_pDataReferenceAtom->GetRefEntry(m_ulDataRefIdx);

    if (!m_pDataReferenceAtom->Get_RefFlags(pRefEntry))
    {
	bSuccess = FALSE;

#ifdef QTCONFIG_ALLOW_EXTERNAL_DATAREFS
	/* handle Mac. Alias and form m_pDataRefName */			       		
	UINT8* pDataRef = (UINT8*) 
			  m_pDataReferenceAtom->Get_RefData(pRefEntry);
	ULONG32 ulCurrentPos = pDataRef - m_pDataReferenceAtom->GetData();
	ULONG32 ulDataRefLength = m_pDataReferenceAtom->GetDataSize();
	UINT8* pRelPath = NULL;
	ULONG32 ulPathLength = 0;

	if (ulDataRefLength > ulCurrentPos)
	{
	    ulDataRefLength -= ulCurrentPos;
	    bSuccess = FindRelPath(pDataRef, 
				   ulDataRefLength, 
				   pRelPath, 
				   ulPathLength);
	}

	if (bSuccess)
	{
	    HX_ASSERT(pRelPath);
	    HX_ASSERT(ulPathLength);

	    bSuccess = FALSE;
	    if (SUCCEEDED(m_pClassFactory->CreateInstance(
				CLSID_IHXBuffer, 
				(void**) &m_pDataRefName)))
	    {
		if (m_pDataRefName->SetSize(ulPathLength + 1) == HXR_OK)
		{
		    UINT8* pData = m_pDataRefName->GetBuffer();

		    bSuccess = TRUE;

		    memcpy(pData, /* Flawfinder: ignore */
			   pRelPath,
			   ulPathLength);
		    pData[ulPathLength] = '\0';

		    while (*pData != '\0')
		    {
			if (*pData == ':')
			{
			    *pData = '/';
			}

			pData++;
		    }
		}
	    }
	}
#endif	// QTCONFIG_ALLOW_EXTERNAL_DATAREFS
    }

    return bSuccess;
}


/****************************************************************************
 *  FindRelPath
 */
#ifdef QTCONFIG_ALLOW_EXTERNAL_DATAREFS
HXBOOL CQT_DataReference_Manager::FindRelPath(UINT8* pData, 
					    ULONG32 ulDataLength, 
					    UINT8* &pRelPath, 
					    ULONG32 &ulPathLength)
{
    UINT8* pCurrentPos;
    UINT8* pPathStart;
    UINT8* pPathEnd;
    HXBOOL bSuccess = FALSE;
    INT16 lNLvlTo;
    INT16 lNLvlFrom;
    CQT_dref_Atom::ItemEntry* pItem;

    pCurrentPos = pData + (sizeof(CQT_dref_Atom::DataRef) - 1);

    bSuccess = (((ULONG32) (pCurrentPos - 
			    pData + 
			    sizeof(CQT_dref_Atom::ItemEntry) - 
			    1)) <= ulDataLength);

    if (bSuccess)
    {
	CQT_dref_Atom::DataRef* pDataRef = (CQT_dref_Atom::DataRef*) pData;

	lNLvlTo = CQTAtom::GetI16(pDataRef->pNLvlTo);
	lNLvlFrom = CQTAtom::GetI16(pDataRef->pNLvlFrom);

	// Make sure the path give is relative
	bSuccess = ((lNLvlTo >= 0) && (lNLvlFrom >= 0));
    }
  
    // Search for the absolute pathname marker
    if (bSuccess)
    {
	ULONG32 ulIdx;

	bSuccess = FALSE;

	for (ulIdx = QT_ALIS_MAXCOUNT; ulIdx != 0; ulIdx--)
	{
	    pItem = (CQT_dref_Atom::ItemEntry*) pCurrentPos;
	    
	    if ((CQTAtom::GetI16(pItem->pType) == QT_ALIS_ABSPATH) ||
		(CQTAtom::GetI16(pItem->pType) == QT_ALIS_END))
	    {
		bSuccess = (CQTAtom::GetI16(pItem->pType) == QT_ALIS_ABSPATH);
		break;
	    }
	    
	    pCurrentPos = pItem->pData + 
			  ((CQTAtom::GetI16(pItem->pSize) + 1) & ~1);
	    
	    bSuccess = (((ULONG32) (pCurrentPos - pData)) <= ulDataLength);
	}
    }
    
    if (bSuccess)
    {
	// Strip off the absolute portions
	pPathStart = pItem->pData;
	pPathEnd = (pPathStart + CQTAtom::GetUI16(pItem->pSize));

	pCurrentPos = pPathEnd;

	bSuccess = (((ULONG32) (pCurrentPos - pData)) <= ulDataLength);
    }

    if (bSuccess)
    {
	while (lNLvlTo && (pCurrentPos >= pPathStart))
	{
	    if (*(pCurrentPos--) == ':')
	    {
		lNLvlTo--;
	    }
	}

	pCurrentPos++;
	if(*pCurrentPos == ':')
	{
	    pCurrentPos++;
	}

	pRelPath = pCurrentPos;
	ulPathLength = pPathEnd + 1 - pCurrentPos;

	bSuccess = (ulPathLength != 0);
    }

    return bSuccess;
}

#else // QTCONFIG_ALLOW_EXTERNAL_DATAREFS

/****************************************************************************
 *  FindRelPath
 */
HXBOOL CQT_DataReference_Manager::FindRelPath(UINT8* pData, 
					    ULONG32 ulDataLength, 
					    UINT8* &pRelPath, 
					    ULONG32 &ulPathLength)
{
    return FALSE;
}

#endif // QTCONFIG_ALLOW_EXTERNAL_DATAREFS


/****************************************************************************
 *  Track Info Manager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQT_TrackInfo_Manager::CQT_TrackInfo_Manager(void)
    : m_ulMediaTimeScale(0)
    , m_ulPayloadType(QT_BAD_PAYLOAD)
    , m_ulTrackDuration(0)
    , m_uAltGroupId(0)
    , m_ulSwitchGroupId(0)
    , m_ulTrackSize(0)
    , m_ulTrackSelectionMask(0)
    , m_ulAvgBitrate(0)
    , m_ulMaxBitrate(0)
    , m_ulBandwidth(0)
    , m_ulPreroll(0)
    , m_ulPredata(0)
    , m_ulRefTrackID(QT_BAD_IDX)
    , m_ulTrackWidth(0)
    , m_ulTrackHeight(0)
    , m_ulFrameWidth(0)
    , m_ulFrameHeight(0)
    , m_ulChannels(0)
    , m_ulTrackMatrixTransformX(0)
    , m_ulTrackMatrixTransformY(0)
    , m_pName(NULL)
    , m_pSDP(NULL)
    , m_pMimeType(NULL)
    , m_pOpaqueData(NULL)
    , m_ulOpaqueDataSize(0)
    , m_bOpaqueDataShouldBeDeleted(FALSE)
    , m_ulNumSamplesInOpaqueData(0)
    , m_pHeader(NULL)
    , m_pNameAtom(NULL)
    , m_pSDPAtom(NULL)
    , m_pPayloadTypeAtom(NULL)
{
    ;
}

CQT_TrackInfo_Manager::~CQT_TrackInfo_Manager()
{
    Clear();
}

/****************************************************************************
 *  Main Interface
 */
HX_RESULT CQT_TrackInfo_Manager::Init(IUnknown* pContext,
				      CQTAtom* pAtom,
				      CQT_SampleDescription_Manager* pSampleDescManager,
				      CQTTrackManager* pTrackManager,
				      CQT_MovieInfo_Manager* pMovieInfo)
{
    HX_RESULT retVal = HXR_OK;
    
    Clear();
    
    if (pAtom->GetType() != QT_trak)
    {
	pAtom = NULL;
    }

    // Extract Track Type
    if (pAtom)
    {
	CQT_hdlr_Atom* pHdlrAtom = CQTTrackManager::GetTrackAtomHdlr(
					(CQT_trak_Atom *) pAtom);

	if (pHdlrAtom)
	{
	    m_TrackType = pHdlrAtom->Get_CompSubtype();
	}
	else
	{
	    // Fail
	    pAtom = NULL;
	}
    }

    // Extract track duration
    if (pAtom)
    {
	CQT_tkhd_Atom* pTrackHeaderAtom = NULL;

	pTrackHeaderAtom = (CQT_tkhd_Atom*) 
			   pAtom->FindPresentChild(QT_tkhd);

	if (pTrackHeaderAtom)
	{
	    m_ulTrackDuration = pTrackHeaderAtom->Get_Duration();
	    m_uAltGroupId = pTrackHeaderAtom->Get_AltGroup();
	    m_ulTrackWidth    = (UINT32) pTrackHeaderAtom->Get_TrackWidth();
	    m_ulTrackHeight   = (UINT32) pTrackHeaderAtom->Get_TrackHeight();
	    // These are the offsets from the origin (video's upper left):
	    m_ulTrackMatrixTransformX = (UINT32) pTrackHeaderAtom->Get_TrackMatrixTx();
	    m_ulTrackMatrixTransformY = (UINT32) pTrackHeaderAtom->Get_TrackMatrixTy();
	}
	else
	{
	    pAtom = NULL;
	}
    }

    // Extract Track Time Scale
    if (pAtom)
    {
	CQTAtom* pChildAtom = pAtom->FindPresentChild(QT_mdia);

	if (pChildAtom)
	{
	    CQT_mdhd_Atom* pMediaHeaderAtom = (CQT_mdhd_Atom*)
					       pChildAtom->FindPresentChild(QT_mdhd);
	    if (pMediaHeaderAtom)
	    {
		m_ulMediaTimeScale = pMediaHeaderAtom->Get_TimeScale();
	    }
	}

	if (m_ulMediaTimeScale == 0)
	{
	    pAtom = NULL;
	}
    }

    if (pAtom)
    {
	CQTAtom* pChildAtom = pAtom->FindPresentChild(QT_udta);

	// Extract Track Selection Mask
	if (pChildAtom)
	{
	    CQT_tsel_Atom* pTrackSelectionAtom = 
		(CQT_tsel_Atom*) pChildAtom->FindPresentChild(QT_tsel);
	    if (pTrackSelectionAtom)
	    {
		ULONG32 ulNumTrackSelAttr = 
		    pTrackSelectionAtom->Get_NumEntries();

		m_ulSwitchGroupId = pTrackSelectionAtom->Get_SwitchGroup();

		while (ulNumTrackSelAttr > 0)
		{
		    ulNumTrackSelAttr--;

		    switch (pTrackSelectionAtom->Get_Attribute(ulNumTrackSelAttr))
		    {
		    case QT_lang:
			m_ulTrackSelectionMask |= QT_TSEL_LANGUAGE;
			break;
		    case QT_bwas:
			m_ulTrackSelectionMask |= QT_TSEL_BANDWIDTH;
			break;
		    case QT_cdec:
			m_ulTrackSelectionMask |= QT_TSEL_CODEC;
			break;
		    case QT_scsz:
			m_ulTrackSelectionMask |= QT_TSEL_SCREEN_SIZE;
			break;
		    case QT_mpsz:
			m_ulTrackSelectionMask |= QT_TSEL_MAX_PACKET_SIZE;
			break;
		    case QT_mtyp:
			m_ulTrackSelectionMask |= QT_TSEL_MEDIA_TYPE;
			break;
		    default:
			// unknown - do nothing
			break;
		    }
		}
	    }
	}
    }

    if (pAtom)
    {
	switch (m_TrackType)
	{
	case QT_hint:
	    retVal = InitHinted(pAtom,
				pSampleDescManager,
				pTrackManager,
				pMovieInfo);


            if (HXR_OK == retVal && m_pSDPAtom)
            {
                retVal = CheckForcePacketization(m_pSDPAtom, pContext);
            }				

#ifdef _IGNORE_MP4_AUDIO
	    if (pTrackManager->GetFType() == QT_FTYPE_MP4)
	    {
		const char* pSDPData = (char*) GetSDP();

		if (pSDPData && (GetSDPLength() >= (sizeof("audio") - 1)))
		{
		    if (strncmp(pSDPData, "m=audio", sizeof("audio") - 1) == 0)
		    {
			retVal = HXR_IGNORE;
		    }
		}
	    }
#endif	// _IGNORE_MP4_AUDIO
	    break;

	case QT_soun:
#ifdef _IGNORE_MP4_AUDIO
	    if (pTrackManager->GetFType() == QT_FTYPE_MP4)
	    {
		retVal = HXR_IGNORE;
		break;
	    }
#endif	// _IGNORE_MP4_AUDIO

	case QT_vide:
	case QT_text:
	    retVal = InitNonHinted(pAtom,
				   pSampleDescManager,
				   pTrackManager,
				   pMovieInfo);
	    break;

	default:
	    // not supported
	    retVal = HXR_IGNORE;
	}

	if (FAILED(retVal))
	{
	    pAtom = NULL;
	}
    }

    if (SUCCEEDED(retVal) && (pAtom == NULL))
    {
	retVal = HXR_FAIL;
    }
   
    return retVal;
}

ULONG32 CQT_TrackInfo_Manager::GetNameLength(void)
{
    ULONG32 ulSize = 0;
    
    if (m_pNameAtom)
    {
	ulSize = m_pNameAtom->GetDataSize();
    }
    else if (m_pName)
    {
	ulSize = strlen(m_pName);
    }
    
    return ulSize;
}

const UINT8* CQT_TrackInfo_Manager::GetName(void)
{
    return m_pNameAtom ? m_pNameAtom->GetData() : ((UINT8*) m_pName);
}

ULONG32 CQT_TrackInfo_Manager::GetSDPLength(void)
{
    ULONG32 ulSize = 0;
    
    if (m_pSDPAtom)
    {
	ulSize = m_pSDPAtom->GetDataSize();
    }
    else if (m_pSDP)
    {
	ulSize = strlen(m_pSDP);
    }
    
    return ulSize;
}

const UINT8* CQT_TrackInfo_Manager::GetSDP(void)
{
    return m_pSDPAtom ? m_pSDPAtom->GetData() : ((UINT8*) m_pSDP);
}

ULONG32 CQT_TrackInfo_Manager::GetOpaqueDataLength(void)
{    
    return m_ulOpaqueDataSize;
}

const UINT8* CQT_TrackInfo_Manager::GetOpaqueData(void)
{
    return m_pOpaqueData;
}

const UINT32 CQT_TrackInfo_Manager::GetNumSamplesInOpaqueData(void)
{
    return m_ulNumSamplesInOpaqueData;
}

HX_RESULT CQT_TrackInfo_Manager::GetHeader(IHXValues* &pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pHeader)
    {
	pHeader = m_pHeader;
	pHeader->AddRef();
	retVal = HXR_OK;
    }

    return retVal;
}

void CQT_TrackInfo_Manager::SetHeader(IHXValues* pHeader)
{
    HX_RELEASE(m_pHeader);

    m_pHeader = pHeader;
    if (m_pHeader)
    {
	m_pHeader->AddRef();
    }
}

/****************************************************************************
 *  Private functions
 */
void CQT_TrackInfo_Manager::Clear(void)
{
    HX_RELEASE(m_pNameAtom);
    HX_RELEASE(m_pSDPAtom);
    HX_RELEASE(m_pPayloadTypeAtom);
    HX_RELEASE(m_pHeader);
    HX_VECTOR_DELETE(m_pSDP);
    HX_VECTOR_DELETE(m_pMimeType);

    m_ulMediaTimeScale = 0;
    m_ulTrackDuration = 0;
    m_uAltGroupId = 0;
    m_ulSwitchGroupId = 0;
    m_ulTrackSize = 0;
    m_ulTrackSelectionMask = 0;
    m_ulRefTrackID = QT_BAD_IDX;
    m_ulTrackWidth = 0;
    m_ulTrackHeight = 0;
    m_ulFrameWidth = 0;
    m_ulFrameHeight = 0;

    m_ulTrackMatrixTransformX = m_ulTrackMatrixTransformY = 0;
    m_ulOpaqueDataSize = 0;
    if (m_bOpaqueDataShouldBeDeleted  &&  m_pOpaqueData)
    {
	delete [] m_pOpaqueData;
	m_pOpaqueData = NULL;
	m_ulOpaqueDataSize = 0;
	m_bOpaqueDataShouldBeDeleted = FALSE;
	m_ulNumSamplesInOpaqueData = 0;
    }
}

HX_RESULT CQT_TrackInfo_Manager::InitNonHinted(CQTAtom* pAtom,
					       CQT_SampleDescription_Manager* pSampleDescManager,
					       CQTTrackManager* pTrackManager,
					       CQT_MovieInfo_Manager* pMovieInfo)
{
    const char* pMediaType = NULL;
    const char* pMediaName = NULL;
#ifndef HELIX_FEATURE_3GPPCLIENT_ONLY
    UINT8 uProfileObjectIndication = MP4OBJ_FORBIDDEN;
    UINT8 uStreamType = MP4STRM_FORBIDDEN;
#endif /* #ifndef HELIX_FEATURE_3GPPCLIENT_ONLY */

    HX_RESULT retVal = HXR_OK;

    // Form Track name
    switch (m_TrackType)
    {
    case QT_vide:
	m_pName = "Video Track";
	break;
    case QT_soun:
	m_pName = "Audio Track";
	break;
    case QT_text:
	m_pName = "Text Track";
	break;
    default:
	retVal = HXR_IGNORE;
	break;
    }

    m_ulOpaqueDataSize = 0;
    if (m_bOpaqueDataShouldBeDeleted  &&  m_pOpaqueData)
    {
	delete [] m_pOpaqueData;
	m_pOpaqueData = NULL;
    }
    m_ulOpaqueDataSize = 0;
    m_bOpaqueDataShouldBeDeleted = FALSE;
    m_ulNumSamplesInOpaqueData = 0;

    // Extract Opaque Data
    if ((retVal == HXR_OK) && pAtom)
    {
	if (pSampleDescManager->GetNumEntries() == 1)
	{
	    CQT_stsd_Atom::ArrayEntry* pSampleDescEntry = 
		pSampleDescManager->GetSampleDescEntry();
	    ULONG32 ulSampleDescEntrySize = 
		CQTAtom::GetUL32(pSampleDescEntry->pSize);

	    if (pTrackManager->GetFType() == QT_FTYPE_MP4)
	    {
		switch (pSampleDescManager->GetDataFormat())
		{
		case QT_mp4v:
		    if (ulSampleDescEntrySize > 
			sizeof(CQT_stsd_Atom::VideoMP4ArrayEntry))
		    {
                        m_ulFrameWidth = CQTAtom::GetUI16((UINT8*)
                           (((CQT_stsd_Atom::VideoMP4ArrayEntry*)
                           pSampleDescEntry)->pWidth));
                        m_ulFrameHeight = CQTAtom::GetUI16((UINT8*)
                           (((CQT_stsd_Atom::VideoMP4ArrayEntry*)
                           pSampleDescEntry)->pHeight));

			m_pOpaqueData = ((CQT_stsd_Atom::VideoMP4ArrayEntry*) 
			    pSampleDescEntry)->pESDescriptor;
		    }
		    break;
		case QT_mp4a:
		    if (ulSampleDescEntrySize > 
			sizeof(CQT_stsd_Atom::AudioMP4ArrayEntry))
                    {
                        m_ulChannels = CQTAtom::GetUI16((UINT8*)
                            (((CQT_stsd_Atom::AudioArrayEntry*) 
			    pSampleDescEntry)->pNumChannels));
			m_pOpaqueData = ((CQT_stsd_Atom::AudioMP4ArrayEntry*) 
			    pSampleDescEntry)->pESDescriptor;
		    }
		    break;
		case QT_s263:
		    if (ulSampleDescEntrySize > 
			sizeof(CQT_stsd_Atom::VideoS263ArrayEntry))
		    {
                        m_ulFrameWidth = CQTAtom::GetUI16((UINT8*)
                           (((CQT_stsd_Atom::VideoS263ArrayEntry*)
                           pSampleDescEntry)->pWidth));
                        m_ulFrameHeight = CQTAtom::GetUI16((UINT8*)
                           (((CQT_stsd_Atom::VideoS263ArrayEntry*)
                           pSampleDescEntry)->pHeight));

			m_pOpaqueData = ((CQT_stsd_Atom::VideoS263ArrayEntry*) 
			    pSampleDescEntry)->pDecoderSpecificInfo;
		    }
		    break;
		case QT_avc1:
		    if (ulSampleDescEntrySize > 
			sizeof(CQT_stsd_Atom::VideoAVCArrayEntry))
		    {
			m_pOpaqueData = ((CQT_stsd_Atom::VideoAVCArrayEntry*) 
			    pSampleDescEntry)->pDecoderSpecificInfo;
		    }
		    break;
		case QT_sqcp:
		    if (ulSampleDescEntrySize > 
			sizeof(CQT_stsd_Atom::AudioQCELPArrayEntry))
		    {
			m_pOpaqueData = ((CQT_stsd_Atom::AudioQCELPArrayEntry*) 
			    pSampleDescEntry)->pDecoderSpecificInfo;
		    }
		    break;
		case QT_samr:
		    if (ulSampleDescEntrySize > 
			sizeof(CQT_stsd_Atom::AudioSAMRArrayEntry))
		    {
			m_pOpaqueData = ((CQT_stsd_Atom::AudioSAMRArrayEntry*) 
			    pSampleDescEntry)->pDecoderSpecificInfo;
		    }
		    break;
		case QT_sawb:
		    if (ulSampleDescEntrySize > 
			sizeof(CQT_stsd_Atom::AudioSAWBArrayEntry))
		    {
			m_pOpaqueData = ((CQT_stsd_Atom::AudioSAWBArrayEntry*) 
			    pSampleDescEntry)->pDecoderSpecificInfo;
		    }
		    break;
#ifdef QTCONFIG_TIMEDTEXT_PACKETIZER
		case QT_tx3g:
		    m_ulNumSamplesInOpaqueData = 1;
		    // /Use entire descEntry as opaque stream data; unpack it
		    // in renderer:
		    if (ulSampleDescEntrySize >= 
			    sizeof(CQT_stsd_Atom::TextSampleEntry))
		    {
			CQT_stsd_Atom* pSampleDescAtom =
				pSampleDescManager->GetSampleDescriptionAtom();
			m_pOpaqueData = pSampleDescAtom->GetData();
			m_ulOpaqueDataSize = pSampleDescAtom->GetDataSize();
		    }
		    else
		    {
			retVal = HXR_UNEXPECTED;
			m_ulNumSamplesInOpaqueData = 0;
		    }
		    break;
#endif	// QTCONFIG_TIMEDTEXT_PACKETIZER
		case QT_alac:
		    if (ulSampleDescEntrySize > 
			sizeof(CQT_stsd_Atom::AudioALACArrayEntry))
		    {
			m_pOpaqueData = ((CQT_stsd_Atom::AudioALACArrayEntry*) 
			    pSampleDescEntry)->pDecoderSpecificInfo;
		    }
		    break;
		default:
		    retVal = HXR_IGNORE;
		    break;
		}
	    }
	    else
	    {
		switch (m_TrackType)
		{
		case QT_vide:
		    if (ulSampleDescEntrySize >=
			sizeof(CQT_stsd_Atom::VideoArrayEntry))
		    {
			m_pOpaqueData = ((CQT_stsd_Atom::VideoArrayEntry*) 
			    pSampleDescEntry)->pVersion;
		    }
		    break;
		case QT_soun:
		    if (ulSampleDescEntrySize >=
			sizeof(CQT_stsd_Atom::AudioArrayEntry))
		    {
			m_pOpaqueData = ((CQT_stsd_Atom::AudioArrayEntry*) 
			    pSampleDescEntry)->pVersion;
		    }
		    break;
		default:
		    retVal = HXR_IGNORE;
		    break;
		}
	    }

	    if (m_pOpaqueData)
	    {
		m_ulOpaqueDataSize = ulSampleDescEntrySize - 
				     (m_pOpaqueData - ((UINT8*) pSampleDescEntry));

#ifndef HELIX_FEATURE_3GPPCLIENT_ONLY
		if ((pSampleDescManager->GetDataFormat() == QT_mp4v) || 
		    (pSampleDescManager->GetDataFormat() == QT_mp4a))
		{
		    ES_Descriptor ESDesc;
		    DecoderConfigDescriptor* pDCDesc = NULL;
		    UINT8* pOpaqueData = m_pOpaqueData;
		    ULONG32 ulOpaqueDataSize = m_ulOpaqueDataSize;
		    
		    retVal = ESDesc.Unpack(pOpaqueData, ulOpaqueDataSize);
		    
		    if (SUCCEEDED(retVal))
		    {
			retVal = HXR_FAIL;
			pDCDesc = ESDesc.m_pDecConfigDescr;
			
			if (pDCDesc)
			{
			    m_ulAvgBitrate = pDCDesc->m_ulAvgBitrate;
			    m_ulMaxBitrate = pDCDesc->m_ulMaxBitrate;
			    uProfileObjectIndication = pDCDesc->m_uObjectProfileIndication;
			    uStreamType = pDCDesc->m_uStreamType;
			    retVal = HXR_OK;
			}
		    }
		    
#ifdef _TINF_NO_MEDIA_SCALE_IQ
		    if (SUCCEEDED(retVal) && pMovieInfo)
		    {
			// PacketVideo format records sample times in movie
			// scale.  That format also has a flaw in ES_Descriptor
			// that forces ES_Descriptor into the alternate 
			// parsing mode which results in setting of the
			// SIZE_HEADER_INCLUSIVE flag. Thus, the media time scale 
			// is corrected based on this condition.
			if (ESDesc.m_ulFlags &
			    MP4BaseDescriptor::SIZE_HEADER_INCLUSIVE)
			{
			    m_ulMediaTimeScale = pMovieInfo->GetMovieTimeScale();
			}
		    }
#endif	// _TINF_NO_MEDIA_SCALE_IQ
		}
#endif	// HELIX_FEATURE_3GPPCLIENT_ONLY
	    }
	    else
	    {
		if (retVal == HXR_OK)
		{
		    retVal = HXR_FAIL;
		}
	    }
	}
#ifdef QTCONFIG_TIMEDTEXT_PACKETIZER
	else if (pSampleDescManager->GetNumEntries() > 1  &&
		QT_tx3g == pSampleDescManager->GetDataFormat())
	{
	    // Cycle through and combine these into one opaque blob for
	    // the stream header:
	    UINT32 ulNumEntries = pSampleDescManager->GetNumEntries();
	    CQT_stsd_Atom* pSampleDescAtom =
		    pSampleDescManager->GetSampleDescriptionAtom();
	    if (pSampleDescAtom  &&  ulNumEntries)
	    {
		m_ulNumSamplesInOpaqueData = ulNumEntries;
		m_ulOpaqueDataSize = pSampleDescAtom->GetDataSize();
		m_pOpaqueData = pSampleDescAtom->GetData();
	    }
	}
#endif	// QTCONFIG_TIMEDTEXT_PACKETIZER
    }

    // Form SDP Segment
    if ((retVal == HXR_OK) && pAtom)
    {
	// Set default media name
	if (pTrackManager->GetFType() == QT_FTYPE_MP4)
	{
	    pMediaName = "X-RN-MP4-RAWAU";
	}
	else
	{
	    pMediaName = "X-RN-QT-RAWAU";
	}
	
	switch (m_TrackType)
	{
	case QT_vide:
	    pMediaType = "video";
	    switch (pSampleDescManager->GetDataFormat())
	    {
	    case QT_mp4v:
	    case QT_mp4a:
		pMediaName = "X-RN-MP4";
		break;
	    case QT_s263:
		pMediaName = "X-RN-3GPP-H263";
		break;
	    case QT_avc1:
		pMediaName = "X-HX-AVC1";
		break;
	    default:
		// nothing to do
		break;
	    }
	    break;
	    
	    case QT_soun:
		pMediaType = "audio";
		switch (pSampleDescManager->GetDataFormat())
		{
#ifndef HELIX_FEATURE_3GPPCLIENT_ONLY
		case QT_mp4v:
		case QT_mp4a:
		    if ((uProfileObjectIndication == MP4OBJ_VISUAL_ISO_IEC_11172_3) &&
			(uStreamType == MP4STRM_AUDIO))
		    {
			pMediaName = "MPEG-ELEMENTARY";
			m_pOpaqueData = NULL;
			m_ulOpaqueDataSize = 0;
			m_ulPreroll = 1000;
		    }
		    break;
#endif	// HELIX_FEATURE_3GPPCLIENT_ONLY
		case QT_samr:
		    pMediaName = "X-RN-3GPP-AMR";
		    break;
		case QT_alac:
		    pMediaName = "X-HX-ALAC";
		    break;
		case QT_sqcp:
		    pMediaName = "X-RN-3GPP-QCELP";
		    break;
		case QT_sawb:
		    pMediaName = "X-RN-3GPP-AMR-WB";
		    break;
		default:
		    // nothing to do
		    break;
		}
		break;
		
#ifdef QTCONFIG_TIMEDTEXT_PACKETIZER
		case QT_text:
		    pMediaType = "video";
		    pMediaName = "X-RN-3GPP-TEXT";
		    break;
#endif	// QTCONFIG_TIMEDTEXT_PACKETIZER
		    
		default:
		    pMediaType = "application";
		    break;
	}
	
	m_ulPayloadType = RTP_PAYLOAD_DYNAMIC;

#ifdef QTCONFIG_SERVER

	// XXXSR We should allocate different dynamic payload type
	// for each stream in the file
	// That's kind of hard to do at this part of the code
	// Walk back up and add the number of active streams

	CQT_tkhd_Atom* pTrackHeaderAtom = NULL;
	pTrackHeaderAtom = (CQT_tkhd_Atom*) pAtom->FindPresentChild(QT_tkhd);

	if (pTrackHeaderAtom)
	{
	    ULONG32 ulTrackID = pTrackHeaderAtom->Get_TrackID();

	    CQTTrack* pTrack = pTrackManager->GetTrackById(ulTrackID);

	    if (pTrack)
	    {
		ULONG32 ulActiveStreams = pTrackManager->GetNumActiveStreams();

		m_ulPayloadType += ulActiveStreams;
	    }
	}
	    

	HX_VECTOR_DELETE(m_pSDP);
	m_pSDP = new char [TRACK_SDP_CHUNK_SIZE];
	retVal = HXR_OUTOFMEMORY;
	
	if (m_pSDP)
	{
	    char* pWriter;
	    char* pWriterEnd = m_pSDP + TRACK_SDP_CHUNK_SIZE;
	    retVal = HXR_OK;
	    
	    pWriter = m_pSDP;
	    
	    pWriter += SafeSprintf(pWriter, pWriterEnd - pWriter,
				   "m=%s 0 RTP/AVP %d\r\n", 
				   pMediaType, 
				   m_ulPayloadType);
	    
	    pWriter += SafeSprintf(pWriter, pWriterEnd - pWriter,
				   "a=rtpmap:%d %s/%d\r\n",
				   m_ulPayloadType,
				   pMediaName,
				   m_ulMediaTimeScale);

	    HX_ASSERT((pWriter - m_pSDP) < TRACK_SDP_CHUNK_SIZE);
	}
#endif	// QTCONFIG_SERVER
    }

    if ((retVal == HXR_OK) && pAtom)
    {
	HX_ASSERT(pMediaType);
	HX_ASSERT(pMediaName);

	ULONG32 ulMediaTypeLen = strlen(pMediaType);
	ULONG32 ulMediaNameLen = strlen(pMediaName);

	m_pMimeType = new char [ulMediaTypeLen + ulMediaNameLen + 2];

	retVal = HXR_OUTOFMEMORY;
	if (m_pMimeType)
	{
	    retVal = HXR_OK;

	    strcpy(m_pMimeType, pMediaType); /* Flawfinder: ignore */
	    strcpy(m_pMimeType + ulMediaTypeLen + 1, pMediaName); /* Flawfinder: ignore */
	    m_pMimeType[ulMediaTypeLen] = '/';
	}
    }

    return retVal;
}

/****************************************************************************
 *  Movie Info Manager
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQT_MovieInfo_Manager::CQT_MovieInfo_Manager(void)
    : m_ulMovieTimeScale(0)
    , m_ulMovieDuration(0)
    , m_pNameAtom(NULL)

#if (defined HELIX_FEATURE_3GPP_METAINFO || defined HELIX_FEATURE_SERVER)
    , m_pTitlAtom(NULL)
    , m_pAuthAtom(NULL)
    , m_pCprtAtom(NULL)
#ifdef HELIX_FEATURE_3GPP_METAINFO
    , m_pDscpAtom(NULL)
    , m_pPerfAtom(NULL)
    , m_pGnreAtom(NULL)
    , m_pRtngAtom(NULL)
    , m_pClsfAtom(NULL)
    , m_pKywdAtom(NULL)
    , m_pLociAtom(NULL)
#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER
	
    , m_pRTPSDPAtom(NULL)
{
    ;
}

CQT_MovieInfo_Manager::~CQT_MovieInfo_Manager()
{
    Clear();
}

void CQT_MovieInfo_Manager::Clear()
{
    HX_RELEASE(m_pNameAtom);

#if (defined HELIX_FEATURE_3GPP_METAINFO || defined HELIX_FEATURE_SERVER)
    HX_RELEASE(m_pTitlAtom);
    HX_RELEASE(m_pAuthAtom);
    HX_RELEASE(m_pCprtAtom);
#ifdef HELIX_FEATURE_3GPP_METAINFO
    HX_RELEASE(m_pDscpAtom);
    HX_RELEASE(m_pPerfAtom);
    HX_RELEASE(m_pGnreAtom);
    HX_RELEASE(m_pRtngAtom);
    HX_RELEASE(m_pClsfAtom);
    HX_RELEASE(m_pKywdAtom);
    HX_RELEASE(m_pLociAtom);
#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER
 
    HX_RELEASE(m_pRTPSDPAtom);
}

/****************************************************************************
 *  Main Interface
 */
HX_RESULT CQT_MovieInfo_Manager::Init(CQTAtom* pAtom, 
				      CQTTrackManager* pTrackManager)
{
    HX_RESULT retVal;

    m_ulMovieTimeScale = 0;
    m_ulMovieDuration = 0;
    HX_RELEASE(m_pNameAtom);

#if (defined HELIX_FEATURE_3GPP_METAINFO || defined HELIX_FEATURE_SERVER)
    HX_RELEASE(m_pTitlAtom);
    HX_RELEASE(m_pAuthAtom);
    HX_RELEASE(m_pCprtAtom);
#ifdef HELIX_FEATURE_3GPP_METAINFO
    HX_RELEASE(m_pDscpAtom);
    HX_RELEASE(m_pPerfAtom);
    HX_RELEASE(m_pGnreAtom);
    HX_RELEASE(m_pRtngAtom);
    HX_RELEASE(m_pClsfAtom);
    HX_RELEASE(m_pKywdAtom);
    HX_RELEASE(m_pLociAtom);
#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER
    
    HX_RELEASE(m_pRTPSDPAtom);

    if (pAtom && (pAtom->GetType() != QT_moov))
    {
	pAtom = NULL;
    }

    if (pAtom)
    {
	CQT_mvhd_Atom *pMovieHeaderAtom = NULL;

	pMovieHeaderAtom = (CQT_mvhd_Atom*) 
			   pAtom->FindPresentChild(QT_mvhd);

	if (pMovieHeaderAtom)
	{
	    m_ulMovieTimeScale = pMovieHeaderAtom->Get_TimeScale();
	    m_ulMovieDuration = pMovieHeaderAtom->Get_Duration();
	}
	else
	{
	    pAtom = NULL;
	}
    }

    if (pAtom)
    {
	pAtom = pAtom->FindPresentChild(QT_udta);
    }

    if (pAtom)
    {
	m_pNameAtom = (CQT_name_Atom*) pAtom->FindPresentChild(QT_name);
	if (m_pNameAtom)
	{
	    m_pNameAtom->AddRef();
	}

#if (defined HELIX_FEATURE_3GPP_METAINFO || defined HELIX_FEATURE_SERVER)
        m_pTitlAtom = (CQT_titl_Atom*) pAtom->FindPresentChild(QT_titl);
        if (m_pTitlAtom)
        {
            m_pTitlAtom->AddRef();
        }

        m_pAuthAtom = (CQT_auth_Atom*) pAtom->FindPresentChild(QT_auth);
        if (m_pAuthAtom)
        {
            m_pAuthAtom->AddRef();
        }

        m_pCprtAtom = (CQT_cprt_Atom*) pAtom->FindPresentChild(QT_cprt);
        if (m_pCprtAtom)
        {
            m_pCprtAtom->AddRef();
        }

#ifdef HELIX_FEATURE_3GPP_METAINFO
        m_pDscpAtom = (CQT_dscp_Atom*) pAtom->FindPresentChild(QT_dscp);
        if (m_pDscpAtom)
        {
            m_pDscpAtom->AddRef();
        }

        m_pPerfAtom = (CQT_perf_Atom*) pAtom->FindPresentChild(QT_perf);
        if (m_pPerfAtom)
        {
            m_pPerfAtom->AddRef();
        }

        m_pGnreAtom = (CQT_gnre_Atom*) pAtom->FindPresentChild(QT_gnre);
        if (m_pGnreAtom)
        {
            m_pGnreAtom->AddRef();
        }

        m_pRtngAtom = (CQT_rtng_Atom*) pAtom->FindPresentChild(QT_rtng);
        if (m_pRtngAtom)
        {
            m_pRtngAtom->AddRef();
        }

        m_pClsfAtom = (CQT_clsf_Atom*) pAtom->FindPresentChild(QT_clsf);
        if (m_pClsfAtom)
        {
            m_pClsfAtom->AddRef();
        }

        m_pKywdAtom = (CQT_kywd_Atom*) pAtom->FindPresentChild(QT_kywd);
        if (m_pKywdAtom)
        {
            m_pKywdAtom->AddRef();
        }

        m_pLociAtom = (CQT_loci_Atom*) pAtom->FindPresentChild(QT_loci);
        if (m_pLociAtom)
        {
            m_pLociAtom->AddRef();
        }
#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER

	pAtom = pAtom->FindPresentChild(QT_hnti);
    }

    ParseMovieHintInfo(pAtom);
    
    retVal = m_ulMovieTimeScale ? HXR_OK : HXR_FAIL;
   
    return retVal;
}
