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

#ifndef _QTTRACK_H_
#define _QTTRACK_H_

/****************************************************************************
 *  Defines
 */
#define QTTRK_USE_KEY_FRAMES	FALSE
#define QTTRK_USE_ANY_FRAME	TRUE

#define QT_WIDTH_METANAME       "Width"
#define QT_HEIGHT_METANAME      "Height"
#define QT_FRAMEWIDTH_METANAME  "FrameWidth"
#define QT_FRAMEHEIGHT_METANAME "FrameHeight"

#define QTASM_MARKER_ON_RULE	1
#define QTASM_MARKER_OFF_RULE	0


/****************************************************************************
 *  Includes
 */
#include "hxfiles.h"
#include "hxfswch.h"
#include "iqttrack.h"
#include "qtatmmgs.h"

#include "hxformt.h"

#include "qtmcache.h"


class CQTPacketAssembler;
class CQTFileFormat;
class CQTPacketizerFactory;

/****************************************************************************
 * 
 *  Class:
 *	CQTTrack
 *
 *  Purpose:
 *	Implements Quick Time Track.
 */
class CQTTrack : public IQTTrack,
		 public IHXFileResponse
{
public:
    /*
     *	Constructor/Destructor
     */
    CQTTrack(CQTAtom* pTrackAtom);

    virtual ~CQTTrack();

    /*
     *	Main (IQTTrack) Interface
     */
    virtual HX_RESULT Init(IQTTrackResponse* pResponse,
			   CQTFileFormat* pFileFormat,
			   CQTPacketAssembler* pPacketAssembler,
			   CQTPacketizerFactory* pPacketizerFactory,
			   const char* pProtocol = NULL);

    virtual HX_RESULT SetResponse(IQTTrackResponse* pResponse);

    virtual void Close(void);

    virtual HX_RESULT BuildStreamHeader(IHXValues* &pHeader,
					CQT_MovieInfo_Manager* pMovieInfo,
					CQTTrackManager* pTrackManager);
    virtual HX_RESULT GetPayloadIdentity(IHXValues* pHeader);

    virtual HX_RESULT Seek(ULONG32 ulTime, HXBOOL bUseSyncPoints = TRUE);  // Time given in miliseconds
    virtual HX_RESULT GetPacket(UINT16 uStreamNum);

    virtual HX_RESULT Subscribe(UINT16 uRuleNumber);
    virtual HX_RESULT Unsubscribe(UINT16 uRuleNumber);
    virtual HX_RESULT SubscribeDefault(void);
    virtual HXBOOL IsSubscribed(void);

    virtual HX_RESULT ComputeTrackSize(ULONG32& ulTrackSizeOut);
    virtual HX_RESULT ObtainTrackBandwidth(ULONG32& ulBandwidthOut);
    virtual HX_RESULT ObtainTrackBitrate(ULONG32& ulAvgBitrateOut);

    virtual ULONG32 GetID(void)	{ return m_ulTrackID; }

    virtual ULONG32 GetTrackWidth(void)	
    { 
	return m_TrackInfo.GetTrackWidth(); 
    }
    virtual ULONG32 GetTrackHeight(void)
    {
	return m_TrackInfo.GetTrackHeight();
    }

    virtual ULONG32 GetSDPLength(void)
    {
	return m_TrackInfo.GetSDPLength(); 
    }
    virtual const UINT8* GetSDP(void)
    {
	return m_TrackInfo.GetSDP(); 
    }

    virtual CQT_ChunkToOffset_Manager* GetChunkToOffsetMgr(void)
    {
	return &m_ChunkToOffset;
    }

    /*
     *	Back-end interface
     */
    HX_RESULT GetDataSegment(ULONG32 ulSampleNum,
			     ULONG32 ulSampleOffset,
			     ULONG32 ulSize);

    HX_RESULT GetCompressedDataSegment(ULONG32 ulSampleNum,
				       ULONG32 ulSampleOffset,
				       ULONG32 ulSize,
				       UINT16  uBytesPerCBlock,
				       UINT16  uSamplesPerCBlock);

    HX_RESULT GetSampleDescDataSegment(ULONG32 ulSampleDescIdx,
				       ULONG32 ulSampleDescOffset,
				       ULONG32 ulSize);

    HX_RESULT GetNextCompressedDataSegment(ULONG32 ulSize);

    ULONG32 GetLastSampleDescIdx(void) {return m_ulLastSampleDescIdx;}
    UINT16 GetBaseRuleNumber(void)     {return m_uBaseRuleNumber;}

    void SetBaseRuleNumber(UINT16 uRuleNumber)
    {
	m_uBaseRuleNumber = uRuleNumber;
    }

    /*
     *	IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ 
				    REFIID ID, 
				    void** ppInterfaceObj);

    STDMETHOD_(UINT32, AddRef )	    (THIS);

    STDMETHOD_(UINT32, Release)	    (THIS);

    /*
     *  IHXFileResponse methods
     */
    STDMETHOD(InitDone)	    (THIS_
			    HX_RESULT	status)
    {
	HX_ASSERT(HXR_NOTIMPL == HXR_OK);
	return HXR_NOTIMPL;
    }

    STDMETHOD(CloseDone)    (THIS_
			    HX_RESULT	status)
    {
	HX_ASSERT(HXR_NOTIMPL == HXR_OK);
	return HXR_NOTIMPL;
    }

    STDMETHOD(ReadDone)	    (THIS_ 
			    HX_RESULT	status,
			    IHXBuffer*	pBuffer);

    STDMETHOD(WriteDone)    (THIS_ 
			    HX_RESULT	status)
    {
	HX_ASSERT(HXR_NOTIMPL == HXR_OK);
	return HXR_NOTIMPL;
    }

    STDMETHOD(SeekDone)	    (THIS_ 
			    HX_RESULT	status);
 
    /*
     *	Public Atom Managers
     */
    CQT_TrackInfo_Manager m_TrackInfo;
    CQT_SampleDescription_Manager m_SampleDesc;

protected:
    /*
     *	Protected functions
     */
    HX_RESULT InitPacketizer(IHXPayloadFormatObject* &pPacketizer,
			     CQTPacketizerFactory* pPacketizerFactory,
			     const char* pProtocol,
			     CQT_TrackInfo_Manager* pTrackInfo,
			     CQT_MovieInfo_Manager* pMovieInfo,
			     CQTTrackManager* pTrackManager,
			     CQTTrack* pTrack,
			     IUnknown* pContext);

    HX_RESULT LoadData(IHXBuffer* pFileName,
		       ULONG32 ulOffset,
		       ULONG32 ulSize);

    HX_RESULT DataReady(HX_RESULT status,
			IHXBuffer *pBuffer);

    HXBOOL SequenceToTime(CQT_TrackEdit_Manager &TrackEdit,
			CQT_TimeToSample_Manager &TimeToSample,
			CQT_SampleToChunk_Manager &SampleToChunk,
			HXBOOL bUseNonKeyFrames,
			HXBOOL bUseBestPickHeuristic = FALSE);

    HXBOOL AdvanceSample(CQT_TrackEdit_Manager &TrackEdit,
		       CQT_TimeToSample_Manager &TimeToSample,
		       CQT_SampleToChunk_Manager &SampleToChunk);

    HX_RESULT ReturnPacket(HX_RESULT status, 
			   IHXBuffer* pBuffer);

    HX_RESULT ReturnPacket(HX_RESULT status, 
				  IHXBuffer* pBuffer,
				  ULONG32 ulOffset,
				  ULONG32 ulSize);

    UINT32 GetFramesPerMSecond(CQT_MovieInfo_Manager* pMovieInfo);

    /*
     *	Protected Atom Managers
     */
    CQT_DataReference_Manager m_DataRef;
    CQT_SampleSize_Manager m_SampleSize;
    
    CQT_trak_Atom* m_pTrackAtom;
    IQTTrackResponse* m_pResponse;
    CQTFileFormat* m_pFileFormat;
    CQTPacketAssembler* m_pPacketAssembler;

    IHXCommonClassFactory* m_pClassFactory;

    ULONG32 m_ulTrackID;

    ULONG32 m_ulReadSize;
    ULONG32 m_ulReadPageSize;
    ULONG32 m_ulReadPageOffset;
    IHXBuffer* m_pReadFileNameBuffer;
    char* m_pReadFileName;

#ifdef QTCONFIG_TRACK_CACHE
    CQTMemCache m_TrackCache;
#endif	// QTCONFIG_TRACK_CACHE

private:
    typedef enum
    {
	QTT_SampleRead,
	QTT_SegmentRead,
	QTT_Offline
    } QTTPendingState;

    QTTPendingState m_PendingState;
    HXBOOL m_bTrackDone;

    /*
     *	Private Atom Managers
     */
    CQT_TrackEdit_Manager m_TrackEdit;
    CQT_TimeToSample_Manager m_TimeToSample;
    CQT_SampleToChunk_Manager m_SampleToChunk;
    CQT_ChunkToOffset_Manager m_ChunkToOffset;

    UINT16 m_uBytesPerCBlock;
    UINT16 m_uSamplesPerCBlock;
    UINT16 m_uStreamNumber;
    UINT16 m_uBaseRuleNumber;

    IHXPayloadFormatObject* m_pPacketizer;
    IHXFileSwitcher* m_pFileSwitcher;
    
    HXBOOL m_bIsSubscribed;

    LONG32 m_lRefCount;

    ULONG32 m_ulLastSampleDescIdx;
};

#endif  // _QTTRACK_H_
