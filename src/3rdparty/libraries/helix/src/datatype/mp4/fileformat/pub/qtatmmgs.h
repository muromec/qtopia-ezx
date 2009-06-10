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

#ifndef _QTATMMGS_H_
#define _QTATMMGS_H_

/****************************************************************************
 *  Defines
 */
#define _STCO_ZERO_BASED_IQ
#define _STSD_ZERO_BASED_IQ
#define _STSS_ZERO_BASED_IQ
#define _TINF_NO_MEDIA_SCALE_IQ

// Enable identification of SYNC packets with ASM_SWITCH_ON/OFF flags
#define _STSS_TRACK_SYNC    

#define QT_BAD_IDX	0xFFFFFFFF
#define QT_BAD_PAYLOAD	0xFFFFFFFF

#ifdef QTCONFIG_SPEED_OVER_SIZE
#define QTATMMGS_INLINE inline
#else	// QTCONFIG_SPEED_OVER_SIZE
#define QTATMMGS_INLINE /**/
#endif	// QTCONFIG_SPEED_OVER_SIZE

// Track Selection Mask Values
#define QT_TSEL_LANGUAGE	0x01
#define QT_TSEL_BANDWIDTH	0x02
#define QT_TSEL_CODEC		0x04
#define QT_TSEL_SCREEN_SIZE	0x08
#define QT_TSEL_MAX_PACKET_SIZE	0x10
#define QT_TSEL_MEDIA_TYPE	0x20


/****************************************************************************
 *  Includes
 */
#include "qtatoms.h"
#include "hxcomm.h"

class CQTTrackManager;
class CQT_MovieInfo_Manager;


/****************************************************************************
 *  Track Edit Manager
 *  Note: All locally stored times are in media units
 */
class CQT_TrackEdit_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_TrackEdit_Manager(void);
    ~CQT_TrackEdit_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init( CQTAtom* pAtom, 
		    ULONG32 ulMovieTimeScale,
		    ULONG32 ulMediaTimeScale);

    HXBOOL EstablishByMediaTime(ULONG32 ulTime);

    HXBOOL EstablishByTime(ULONG32 ulTime) // Given in miliseconds
    {
	return EstablishByMediaTime(
	    (ULONG32) (((double) ulTime) / 1000.0 * m_ulMediaTimeScale + 0.5));
    }

    HXBOOL AdvanceByMediaTime(ULONG32 ulMediaTime, HXBOOL &bDone)  // media units
    {
	m_ulCurrentInEditTime += ulMediaTime;
	bDone = FALSE; // Assume Not Done

	if (m_ulCurrentInEditTime < m_ulCurrentEditDuration)
	{
	    return FALSE;  // No Edit Advancement
	}
	else
	{
	    // We have to move onto the next edit
	    if (!SequenceToEdit())
	    {
		bDone = TRUE;
		m_ulCurrentInEditTime -= ulMediaTime;
	    }

	    return TRUE;  // Edit Advancement (possibly failed)
	}
    }

    ULONG32 GetMediaTime(void)  // in media units
    {
	return m_ulCurrentMediaStartTime + m_ulCurrentInEditTime;
    }

    ULONG32 GetRealMediaTime(void)  // in media units
    {
	return m_ulCurrentEditTime + m_ulCurrentInEditTime;
    }
    double ConvertMediaToRealTime(ULONG32 ulMediaTime) // in miliseconds
    {
	return (((double) ulMediaTime) *
	        1000.0 /
		((double) m_ulMediaTimeScale));
    }

    double GetRealTime(void)  // in miliseconds
    {
	return ConvertMediaToRealTime(m_ulCurrentEditTime + m_ulCurrentInEditTime);
    }

private:
    HXBOOL SequenceToEdit(void);

    ULONG32 MovieToMediaUnits(ULONG32 ulMovieTime)
    {
	return (ULONG32) ((((double) ulMovieTime) / m_ulMovieTimeScale) * 
			  m_ulMediaTimeScale + 0.5);
    }

    CQT_elst_Atom* m_pEditListAtom;

    ULONG32 m_ulMovieTimeScale;
    ULONG32 m_ulMediaTimeScale;

    ULONG32 m_ulNumEdits;
    ULONG32 m_ulCurrentEditIdx;
    ULONG32 m_ulCurrentEditTime;
    ULONG32 m_ulCurrentInEditTime;
    ULONG32 m_ulCurrentEditDuration;
    ULONG32 m_ulCurrentMediaStartTime;  
};

/****************************************************************************
 *  Sample To Chunk Manager
 */
class CQT_SampleToChunk_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_SampleToChunk_Manager(void);
    ~CQT_SampleToChunk_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTAtom* pAtom);

    HXBOOL EstablishBySample(ULONG32 ulSampleNum);

    HXBOOL AdvanceBySample(void)
    {
	m_ulSampleNumber++;
	m_ulSampleInChunkNum++;

	return SequenceToChunk();
    }

    HXBOOL AdvanceToNextChunk(void)
    {
	m_ulSampleNumber += (m_ulSamplesPerChunk - m_ulSampleInChunkNum);
	m_ulSampleInChunkNum = m_ulSamplesPerChunk;

	return AdvanceBySample();
    }

    ULONG32 GetChunkNum(void)		{ return m_ulCurrentChunk; }
    ULONG32 GetChunkSampleNum(void)	{ return m_ulSampleInChunkNum; }
    ULONG32 GetChunkSampleCount(void)	{ return m_ulSamplesPerChunk; }
    ULONG32 GetSampleDescIdx(void)	{ return m_ulSampleDescIdx; }
    ULONG32 GetSampleNum(void)		{ return m_ulSampleNumber; }
    
private:
    HXBOOL SequenceToChunk(void);
    inline HXBOOL SequenceReverseToChunk(void);
    ULONG32       GetFirstChunk(ULONG32 i);
    ULONG32       GetSamplesPerChunk(ULONG32 i);
    ULONG32       GetSampleDescID(ULONG32 i);
    ULONG32       GetNextFirstChunk(ULONG32 i, ULONG32& rulNextIdx);
    ULONG32       GetPrevFirstChunk(ULONG32 i, ULONG32& rulPrevIdx);

    CQT_stsc_Atom* m_pSampleToChunkAtom;

    ULONG32 m_ulNumEntries;
    ULONG32 m_ulSampleInChunkNum;
    ULONG32 m_ulCurrentChunk;
    ULONG32 m_ulNextEntryChunk;
    ULONG32 m_ulCurrentEntryIdx;
    ULONG32 m_ulNextEntryIdx;
    ULONG32 m_ulSamplesPerChunk;
    ULONG32 m_ulSampleNumber;
    ULONG32 m_ulSampleDescIdx;
#ifdef _STCO_ZERO_BASED_IQ
    ULONG32 m_ulChunkNumOffset;
#endif	// _STCO_ZERO_BASED_IQ
};

/****************************************************************************
 *  Time To Sample Manager
 *  Note: All time locals are in media time units
 */
class CQT_TimeToSample_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_TimeToSample_Manager(void);
    ~CQT_TimeToSample_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTAtom* pAtom);

    HXBOOL EstablishByMediaTime(ULONG32 ulMediaTime);
    HXBOOL EstablishAtKeyByMediaTime(ULONG32 ulMediaTime);

    HXBOOL GetLastPreTargetKeyMediaTime(ULONG32 &ulMediaTime);

    QTATMMGS_INLINE HXBOOL AdvanceSyncSampleNumber(void);
    QTATMMGS_INLINE HXBOOL AdvanceCompBySample(void);
    QTATMMGS_INLINE HXBOOL AdvanceBySample(void);

    HXBOOL EstablishCompBySample(ULONG32 ulSampleNum);

    ULONG32 GetSampleNumber(void)	{ return m_ulSampleNumber; }
    ULONG32 GetSampleDuration(void)	{ return m_ulSampleDuration; }
    ULONG32 GetCompositionOffset(void)	{ return m_ulCompOffset; }
    ULONG32 GetLastMediaSyncTime(void)	{ return m_ulLastSyncTime; }
    ULONG32 GetSyncSampleNumber(void)	{ return m_ulSyncSampleNumber; }
    HXBOOL    IsOnSyncSample(void)	{ return (m_ulSyncSampleNumber == 
						  m_ulSampleNumber); }

private:
    CQT_stts_Atom* m_pTimeToSampleAtom;
    CQT_ctts_Atom* m_pCompOffsetAtom;
    CQT_stss_Atom* m_pSyncSampleAtom;

    ULONG32 m_ulNumEntries;
    ULONG32 m_ulCurrentEntryIdx;
    ULONG32 m_ulSampleNumber;
    ULONG32 m_ulSamplesLeftInEntry;
    ULONG32 m_ulSampleDuration;
    ULONG32 m_ulLastSyncTime;

    ULONG32 m_ulNumCompEntries;
    ULONG32 m_ulCurrentCompEntryIdx;
    ULONG32 m_ulCompSampleNumber;
    ULONG32 m_ulSamplesLeftInCompEntry;
    ULONG32 m_ulCompOffset;

    ULONG32 m_ulNumSyncEntries;
    ULONG32 m_ulCurrentSyncEntryIdx;
    ULONG32 m_ulSyncSampleNumber;
    ULONG32 m_ulLastPreTargetSyncMediaTime;

#ifdef _STSS_ZERO_BASED_IQ
    ULONG32 m_ulKeyFrameNumOffset;
#endif	// _STSS_ZERO_BASED_IQ
};

/****************************************************************************
 *  Sample Size Manager
 */
class CQT_SampleSize_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_SampleSize_Manager(void);
    ~CQT_SampleSize_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTAtom* pAtom);

    HXBOOL EstablishBySample(ULONG32 ulSampleNum, 
			   ULONG32 ulChunkSampleNum,
			   ULONG32 ulSamplesPerChunk = 0);

    ULONG32 GetSampleSize(void)		{ return m_ulSampleSize; }
    ULONG32 GetChunkSampleOffset(void)	{ return m_ulChunkSampleOffset; }
    ULONG32 GetChunkSize(void)		{ return m_ulChunkSize; }
    ULONG32 Get_NumEntries(void)		{ return m_ulNumEntries; }

private:
    CQT_stsz_Atom* m_pSampleSizeAtom;

    ULONG32 m_ulGenericSize;
    ULONG32 m_ulSampleSize;
    ULONG32 m_ulChunkSampleOffset;
    ULONG32 m_ulChunkSize;
    ULONG32 m_ulNumEntries;
    ULONG32 m_ulChunkStartSampleNum;
    ULONG32 m_ulSampleNum;
};

/****************************************************************************
 *  Chunk To Offset Manager
 */
class CQT_ChunkToOffset_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_ChunkToOffset_Manager(void);
    ~CQT_ChunkToOffset_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTAtom* pAtom);

    HXBOOL EstablishByChunk(ULONG32 ulChunkNum)
    {
	if ((ulChunkNum > 0) &&
	    (ulChunkNum <= m_ulNumEntries))
	{
	    m_ulChunkOffset = m_pChunkToOffsetAtom->Get_ChunkOffset(ulChunkNum - 1);
	    return TRUE;
	}

	return FALSE;
    }

    ULONG32 GetChunkOffset(void)    { return m_ulChunkOffset; }

    HXBOOL GetChunkOffsetByChunk(UINT32 ulChunkNum, UINT32& ulChunkOffset)
    {
	if ((ulChunkNum > 0) &&
	    (ulChunkNum <= m_ulNumEntries))
	{
	    ulChunkOffset = m_pChunkToOffsetAtom->Get_ChunkOffset(ulChunkNum - 1);
            return TRUE;
	}

	return FALSE;
    }

    UINT32 GetNumChunks(void)
    {
	return m_ulNumEntries;
    }

private:
    CQT_stco_Atom* m_pChunkToOffsetAtom;

    ULONG32 m_ulChunkOffset;
    ULONG32 m_ulNumEntries;
};

/****************************************************************************
 *  Sample Description Manager
 */
class CQT_SampleDescription_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_SampleDescription_Manager(void);
    ~CQT_SampleDescription_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTAtom* pAtom);

    HXBOOL EstablishByIdx(ULONG32 ulSampleDescIdx)
    {
	if (ulSampleDescIdx == m_ulSampleDescIdx)
	{
	    return TRUE;
	}

	if (ulSampleDescIdx < m_ulNumEntries)
	{
	    m_ulSampleDescIdx = ulSampleDescIdx;
	    return ParseSampleDescription();
	}

	return FALSE;
    }

    ULONG32 GetDataRefIdx(void)		{ return m_ulDataRefIdx; }
    ULONG32 GetDataFormat(void)		{ return m_ulDataFormat; }
    ULONG32 GetNumEntries(void)		{ return m_ulNumEntries; }
    ULONG32 GetRTPTimeScale(void)	{ return m_ulRTPTimeScale; }
    LONG32  GetTimeStampOffset(void)	{ return m_lTimeStampOffset; }
    LONG32  GetSequenceNumOffset(void)	{ return m_lSequenceNumOffset; }
    CQT_stsd_Atom::ArrayEntry* GetSampleDescEntry(void)	{ return m_pSampleDesc; }
    ULONG32 GetSampleDescBufferOffset(void)	
    {
	HX_ASSERT(m_pSampleDescriptionAtom);
	return (ULONG32) (((UINT8*) m_pSampleDesc) - 
			  m_pSampleDescriptionAtom->GetData());
    }
    IHXBuffer* GetSampleDescBuffer(void)
    {
	HX_ASSERT(m_pSampleDescriptionAtom);
	return m_pSampleDescriptionAtom->GetBuffer();
    }
    CQT_stsd_Atom* GetSampleDescriptionAtom() { return m_pSampleDescriptionAtom; }

private:
    HXBOOL ParseSampleDescription(void);

    CQT_stsd_Atom* m_pSampleDescriptionAtom;

    ULONG32 m_ulSampleDescIdx;
    CQT_stsd_Atom::ArrayEntry* m_pSampleDesc;
    ULONG32 m_ulDataRefIdx;
    ULONG32 m_ulDataFormat;
    ULONG32 m_ulRTPTimeScale;
    LONG32  m_lTimeStampOffset;
    LONG32  m_lSequenceNumOffset;
    ULONG32 m_ulNumEntries;
    ULONG32 m_ulDataRefIdxOffset;
};

/****************************************************************************
 *  Data Reference Manager
 */
class CQT_DataReference_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_DataReference_Manager(void);
    ~CQT_DataReference_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTAtom* pAtom, IHXCommonClassFactory* pClassFactory);

    HXBOOL EstablishByIdx(ULONG32 ulDataRefIdx)
    {
	if (ulDataRefIdx == m_ulDataRefIdx)
	{
	    return TRUE;
	}

	if (ulDataRefIdx < m_ulNumEntries)
	{
	    m_ulDataRefIdx = ulDataRefIdx;
	    return ParseDataReference();
	}

	return FALSE;
    }

    IHXBuffer* GetDataRefName(void)	{ return m_pDataRefName; }

private:
    HXBOOL ParseDataReference(void);
    HXBOOL FindRelPath(UINT8* pData,
		     ULONG32 ulDataLength, 
		     UINT8* &pRelPath, 
		     ULONG32 &ulPathLength);

    CQT_dref_Atom* m_pDataReferenceAtom;

    ULONG32 m_ulDataRefIdx;
    IHXBuffer* m_pDataRefName;
    IHXCommonClassFactory* m_pClassFactory;
    ULONG32 m_ulNumEntries;
};

/****************************************************************************
 *  Hint Reference Manager
 */
class CQT_HintReference_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_HintReference_Manager(void);
    ~CQT_HintReference_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTAtom* pAtom);

    HXBOOL EstablishByIdx(ULONG32 ulTrackRefIdx)
    {
	if (ulTrackRefIdx == m_ulTrackRefIdx)
	{
	    return TRUE;
	}

	if (ulTrackRefIdx < m_ulNumEntries)
	{
	    m_ulTrackRefIdx = ulTrackRefIdx;
	    return ParseTrackReference();
	}

	return FALSE;
    }

    ULONG32 GetTrackID(void)	{ return m_ulTrackID; }

private:
    HXBOOL ParseTrackReference(void)
    {
	m_ulTrackID = m_pHintAtom->Get_TrackID(m_ulTrackRefIdx);
	
	return TRUE;
    }

    CQT_hint_Atom* m_pHintAtom;

    ULONG32 m_ulTrackRefIdx;
    ULONG32 m_ulTrackID;
    ULONG32 m_ulNumEntries;
};

/****************************************************************************
 *  Track Info Manager
 */
class CQT_TrackInfo_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_TrackInfo_Manager(void);
    ~CQT_TrackInfo_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(IUnknown* pContext,
		   CQTAtom* pAtom,
		   CQT_SampleDescription_Manager* pSampleDescManager,
		   CQTTrackManager* pTrackManager,
		   CQT_MovieInfo_Manager* pMovieInfo = NULL);
		   
    HX_RESULT CheckForcePacketization(CQT_sdp_Atom* pSDPAtom, IUnknown* pContext);

    QTAtomType GetTrackType(void)	{ return m_TrackType; }

    ULONG32 GetMediaTimeScale(void)	{ return m_ulMediaTimeScale; }
    // Track duration is given in movie units
    ULONG32 GetTrackDuration(void)	{ return m_ulTrackDuration; }

    UINT16 GetAltGroupId(void)		{ return m_uAltGroupId; }
    ULONG32 GetSwitchGroupId(void)	{ return m_ulSwitchGroupId; }
    ULONG32 GetTrackSelectionMask(void)	{ return m_ulTrackSelectionMask; }

    ULONG32 GetNameLength(void);
    const UINT8* GetName(void);

    ULONG32 GetSDPLength(void);
    const UINT8* GetSDP(void);

    ULONG32 GetOpaqueDataLength(void);
    const UINT8* GetOpaqueData(void);

    const UINT32 GetNumSamplesInOpaqueData(void);

    const char* GetMimeType(void)   { return m_pMimeType; }
    ULONG32 GetAvgPacketSize(void)  { return m_ulAvgPacketSize; }


    ULONG32 GetTrackSize(void)	    { return m_ulTrackSize; }
    void SetTrackSize(ULONG32 ulTrackSize)
    {
	m_ulTrackSize = ulTrackSize;
    }

    ULONG32 GetAvgBitrate(void)	    { return m_ulAvgBitrate; }

    ULONG32 GetMaxBitrate(void)	    { return m_ulMaxBitrate; }
    ULONG32 GetMaxRawBitrate(void)	    { return m_ulMaxRawBitrate; }    	

    ULONG32 GetNeededBandwidth(void){ return m_ulBandwidth; }
    void SetNeededBandwidth(ULONG32 ulBandwidth)
    {
	m_ulBandwidth = ulBandwidth;
    }

    ULONG32 GetPreroll(void)	    { return m_ulPreroll; }

    ULONG32 GetPredata(void)	    { return m_ulPredata; }
    
    ULONG32 GetRefTrackId(void)	    { return m_ulRefTrackID; }

    ULONG32 GetTrackWidth(void)	    { return m_ulTrackWidth; }

    ULONG32 GetTrackHeight(void)    { return m_ulTrackHeight; }

    ULONG32 GetFrameWidth(void)     { return m_ulFrameWidth; }
    ULONG32 GetFrameHeight(void)    { return m_ulFrameHeight; }

    ULONG32 GetNumChannels(void)    { return m_ulChannels; }

    UINT32 GetTrackTransformX(void) { return m_ulTrackMatrixTransformX; }
    UINT32 GetTrackTransformY(void) { return m_ulTrackMatrixTransformY; }

    ULONG32 GetPayloadType(void);
    ULONG32 GetPayloadNameLength(void);
    UINT8* GetPayloadName(void);

    HX_RESULT GetHeader(IHXValues* &pHeader);
    ULONG32 m_ulAvgPacketSize;
    void SetHeader(IHXValues* pHeader);

private:
    void Clear(void);

    HX_RESULT InitHinted(CQTAtom* pAtom,
			 CQT_SampleDescription_Manager* pSampleDescManager,
			 CQTTrackManager* pTrackManager,
			 CQT_MovieInfo_Manager* pMovieInfo);
    HX_RESULT InitNonHinted(CQTAtom* pAtom,
			    CQT_SampleDescription_Manager* pSampleDescManager,
			    CQTTrackManager* pTrackManager,
			    CQT_MovieInfo_Manager* pMovieInfo);
    HX_RESULT SkipToAvcC(UINT8*& pData, ULONG32 ulSize);
    ULONG32 m_ulMediaTimeScale;
    ULONG32 m_ulPayloadType;
    ULONG32 m_ulTrackDuration;
    UINT16  m_uAltGroupId;
    ULONG32 m_ulSwitchGroupId;
    ULONG32 m_ulTrackSize;
    ULONG32 m_ulTrackSelectionMask;
    ULONG32 m_ulAvgBitrate;
    ULONG32 m_ulMaxBitrate;
    ULONG32 m_ulMaxRawBitrate;
    ULONG32 m_ulBandwidth;
    ULONG32 m_ulPreroll;
    ULONG32 m_ulPredata;
    ULONG32 m_ulRefTrackID;
    ULONG32 m_ulTrackWidth;
    ULONG32 m_ulTrackHeight;
    ULONG32 m_ulFrameWidth;
    ULONG32 m_ulFrameHeight;
    ULONG32 m_ulChannels;

    QTAtomType m_TrackType;
    UINT32 m_ulTrackMatrixTransformX;
    UINT32 m_ulTrackMatrixTransformY;
    char* m_pName;
    char* m_pSDP;
    char* m_pMimeType;
    UINT8* m_pOpaqueData;
    ULONG32 m_ulOpaqueDataSize;
    HXBOOL m_bOpaqueDataShouldBeDeleted;
    UINT32 m_ulNumSamplesInOpaqueData;
    IHXValues* m_pHeader;
    
    CQT_name_Atom* m_pNameAtom;
    CQT_sdp_Atom* m_pSDPAtom;
    CQT_payt_Atom* m_pPayloadTypeAtom;
};

/****************************************************************************
 *  Movie Info Manager
 */
class CQT_MovieInfo_Manager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_MovieInfo_Manager(void);
    ~CQT_MovieInfo_Manager();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTAtom* pAtom, 
		   CQTTrackManager* pTrackManager);

    void      Clear(void);

    ULONG32 GetMovieTimeScale(void)	{ return m_ulMovieTimeScale; }
    ULONG32 GetMovieDuration(void)	{ return m_ulMovieDuration; }

    //*** Name Atom **********************************************

    ULONG32 GetNameLength(void)
    {
        return m_pNameAtom ? m_pNameAtom->GetDataSize() : 0;
    }

    UINT8* GetName(void)
    {
        return m_pNameAtom ? m_pNameAtom->GetData() : NULL;
    }

#if defined(HELIX_FEATURE_3GPP_METAINFO) || defined(HELIX_FEATURE_SERVER)
    //*** Title Atom *********************************************

    ULONG32 GetTitleLength(void)
    {
        return m_pTitlAtom ? m_pTitlAtom->GetTitleLength() : 0;
    }

    UINT8* GetTitle(void)
    {
        return m_pTitlAtom ? m_pTitlAtom->GetTitle() : NULL;
    }

    //*** Author Atom ********************************************

    ULONG32 GetAuthorLength(void)
    {
        return m_pAuthAtom ? m_pAuthAtom->GetAuthorLength() : 0;
    }

    UINT8* GetAuthor(void)
    {
        return m_pAuthAtom ? m_pAuthAtom->GetAuthor() : NULL;
    }

    //*** Copyright Atom *****************************************

    ULONG32 GetCopyrightLength(void)
    {
        return m_pCprtAtom ? m_pCprtAtom->GetCopyrightLength() : 0;
    }

    UINT8* GetCopyright(void)
    {
        return m_pCprtAtom ? m_pCprtAtom->GetCopyright() : NULL;
    }

#ifdef HELIX_FEATURE_3GPP_METAINFO
    //*** Description Atom ***************************************

    ULONG32 GetDescriptionLength(void)
    {
        return m_pDscpAtom ? m_pDscpAtom->GetDescriptionLength() : 0;
    }

    UINT8* GetDescription(void)
    {
        return m_pDscpAtom ? m_pDscpAtom->GetDescription() : NULL;
    }

    //*** Performer Atom *****************************************

    ULONG32 GetPerformerLength(void)
    {
        return m_pPerfAtom ? m_pPerfAtom->GetPerformerLength() : 0;
    }

    UINT8* GetPerformer(void)
    {
        return m_pPerfAtom ? m_pPerfAtom->GetPerformer() : NULL;
    }

    //*** Genre Atom *********************************************

    ULONG32 GetGenreLength(void)
    {
        return m_pGnreAtom ? m_pGnreAtom->GetGenreLength() : 0;
    }

    UINT8* GetGenre(void)
    {
        return m_pGnreAtom ? m_pGnreAtom->GetGenre() : NULL;
    }

    //*** Rating Atom ********************************************

    ULONG32 GetRatingCriteriaLength(void)
    {
        return m_pRtngAtom ? m_pRtngAtom->GetRatingCriteriaLength() : 0;
    }

    UINT8* GetRatingCriteria(void)
    {
        return m_pRtngAtom ? m_pRtngAtom->GetRatingCriteria() : NULL;
    }

    ULONG32 GetRatingEntityLength(void)
    {
        return m_pRtngAtom ? m_pRtngAtom->GetRatingEntityLength() : 0;
    }

    UINT8* GetRatingEntity(void)
    {
        return m_pRtngAtom ? m_pRtngAtom->GetRatingEntity() : NULL;
    }

    ULONG32 GetRatingInfoLength(void)
    {
        return m_pRtngAtom ? m_pRtngAtom->GetRatingInfoLength() : 0;
    }

    UINT8* GetRatingInfo(void)
    {
        return m_pRtngAtom ? m_pRtngAtom->GetRatingInfo() : NULL;
    }

    //*** Classification Atom ************************************

    UINT16 GetClassTable(void)
    {
        return m_pClsfAtom ? m_pClsfAtom->GetClassTable() : 0;
    }

    ULONG32 GetClassEntityLength(void)
    {
        return m_pClsfAtom ? m_pClsfAtom->GetClassEntityLength() : 0;
    }

    UINT8* GetClassEntity(void)
    {
        return m_pClsfAtom ? m_pClsfAtom->GetClassEntity() : NULL;
    }

    ULONG32 GetClassInfoLength(void)
    {
        return m_pClsfAtom ? m_pClsfAtom->GetClassInfoLength() : 0;
    }

    UINT8* GetClassInfo(void)
    {
        return m_pClsfAtom ? m_pClsfAtom->GetClassInfo() : NULL;
    }

    //*** Keywords Atom ******************************************

    UINT8 GetKeywordCount(void)
    {
        return m_pKywdAtom ? m_pKywdAtom->Get_KeywordCnt() : 0;
    }

    UINT8 GetKeywordLength(ULONG32 ulKeywordIdx)
    {
        if (ulKeywordIdx >= (ULONG32) GetKeywordCount())
            return 0;
        return m_pKywdAtom ? m_pKywdAtom->Get_KeywordSize(ulKeywordIdx) : 0;
    }

    UINT8* GetKeyword(ULONG32 ulKeywordIdx)
    {
        if (ulKeywordIdx >= (ULONG32) GetKeywordCount())
            return NULL;
        return m_pKywdAtom ? m_pKywdAtom->Get_KeywordEntry(ulKeywordIdx) : NULL;
    }

    //*** Location Info Atom *************************************

    ULONG32 GetLocationNameLength(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetNameLength() : 0;
    }

    UINT8* GetLocationName(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetName() : NULL;
    }

    ULONG32 GetLocationAstronomicalBodyLength(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetAstronomicalBodyLength() : 0;
    }

    UINT8* GetLocationAstronomicalBody(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetAstronomicalBody() : NULL;
    }

    ULONG32 GetLocationAdditionalNotesLength(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetAdditionalNotesLength() : 0;
    }

    UINT8* GetLocationAdditionalNotes(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetAdditionalNotes() : NULL;
    }

    INT16 GetLocationRole(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetRole() : -1;
    }

    // valid longitude values are -180 <= x <= +180
    LONG32 GetLongitude_WholePart(void)
    {
        LONG32 value = m_pLociAtom ? m_pLociAtom->GetLongitude_WholePart() : -99999;
        if (value < -180 || value > 180)
            value = -99999;
        return value;
    }

    // fractional 16-bits is assumed to be positive 0 <= x <= +65535
    LONG32 GetLongitude_FractionalPart(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetLongitude_FractionalPart() : -99999;
    }

    // valid latitude values are -90 <= x <= +90
    LONG32 GetLatitude_WholePart(void)
    {
        LONG32 value = m_pLociAtom ? m_pLociAtom->GetLatitude_WholePart() : -99999;
        if (value < -90 || value > 90)
            value = -99999;
        return value;
    }

    // fractional 16-bits is assumed to be positive 0 <= x <= +65535
    LONG32 GetLatitude_FractionalPart(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetLatitude_FractionalPart() : -99999;
    }

    // 16-bit altitude is allowed to be -32768 <= x <= +32767
    LONG32 GetAltitude_WholePart(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetAltitude_WholePart() : -99999;
    }

    // fractional 16-bits is assumed to be positive 0 <= x <= +65535
    LONG32 GetAltitude_FractionalPart(void)
    {
        return m_pLociAtom ? m_pLociAtom->GetAltitude_FractionalPart() : -99999;
    }

    //*** Album Atom *********************************************

    UINT32 GetAlbumTitleLength(void)
    {
        return m_pAlbmAtom ? m_pAlbmAtom->GetAlbumTitleLength() : 0;
    }

    UINT8* GetAlbumTitle(void)
    {
        return m_pAlbmAtom ? m_pAlbmAtom->GetAlbumTitle() : NULL;
    }

    HXBOOL HasTrackNumber(void)
    {
        return m_pAlbmAtom ? m_pAlbmAtom->HasTrackNumber() : FALSE;
    }

    UINT8 GetTrackNumber(void)
    {
        return m_pAlbmAtom ? m_pAlbmAtom->GetTrackNumber() : 0;
    }

    //*** Recording Year Atom ************************************

    HXBOOL HasRecordingYear()
    {
        return (m_pYrrcAtom ? TRUE : FALSE);
    }

    UINT16 GetRecordingYear(void)
    {
        return m_pYrrcAtom ? m_pYrrcAtom->GetRecordingYear() : 0;
    }

    //*** Language Encoding **************************************

    // If the encodings of all atoms are equal, <out> is filled with unambiguous encoding and TRUE is returned.
    // If the encodings of atoms differ, <out> is filled with 'und' (for undetermined) and TRUE is returned.
    // If no encodings are found, <out> is filled with 'und' (for undetermined) and FALSE is returned.
    HXBOOL GetGlobalLanguageEncoding(char out[3]);

#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER

    char* GetRefURL(void)
    {
        return m_pRefURL;
    }

    ULONG32 GetSDPLength(void);
    UINT8* GetSDP(void);

private:
    HX_RESULT ParseMovieHintInfo(CQTAtom* pAtom);
    char *m_pRefURL;
    ULONG32 m_ulMovieTimeScale;
    ULONG32 m_ulMovieDuration;

    CQT_name_Atom*    m_pNameAtom;

#if defined(HELIX_FEATURE_3GPP_METAINFO) || defined(HELIX_FEATURE_SERVER)
    CQT_titl_Atom*    m_pTitlAtom;
    CQT_auth_Atom*    m_pAuthAtom;
    CQT_cprt_Atom*    m_pCprtAtom;
#ifdef HELIX_FEATURE_3GPP_METAINFO
    CQT_dscp_Atom*    m_pDscpAtom;
    CQT_perf_Atom*    m_pPerfAtom;
    CQT_gnre_Atom*    m_pGnreAtom;
    CQT_rtng_Atom*    m_pRtngAtom;
    CQT_clsf_Atom*    m_pClsfAtom;
    CQT_kywd_Atom*    m_pKywdAtom;
    CQT_loci_Atom*    m_pLociAtom;
    CQT_albm_Atom*    m_pAlbmAtom;
    CQT_yrrc_Atom*    m_pYrrcAtom;
#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER

    CQT_rtp_Atom*     m_pRTPSDPAtom;
};

#ifdef QTCONFIG_SPEED_OVER_SIZE
#include "qtatmmgs_inline.h"
#endif	// QTCONFIG_SPEED_OVER_SIZE

#endif  // _QTATMMGS_H_
