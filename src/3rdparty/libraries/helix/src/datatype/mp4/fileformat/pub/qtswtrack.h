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

#ifndef _QTSWTRACK_H_
#define _QTSWTRACK_H_

/****************************************************************************
 *  Includes
 */
#include "iqttrack.h"
#include "qttrack.h"
#include "qtatmmgs.h"
#include "qtswtmembertable.h"


/****************************************************************************
 * 
 *  Class:
 *	CQTSwitchTrack
 *
 *  Purpose:
 *	Provides a Switch Track abstraction.  Switch track is a collection
 *      of alternate, dynamically switchable tracks.  The collection is
 *      described by ASMRuleBook (a collection of rules).  A user of this
 *      class can select (subscribe) rules that describe conditions for
 *      the desired media stream. As a result, this class will produce 
 *      (switch to) the appropriate stream in an optimal manner.
 */
class CQTSwitchTrack : public IQTTrack,
		       public IQTTrackResponse,
		       public IHXCallback
{
public:
    /*
     *	Constructor/Destructor
     */
    CQTSwitchTrack(UINT16 uNumSiwtchableTracks,
		   CQTTrack* pSwitchableTracks[],
		   CQTTrack* pPrimaryTrack,
		   UINT32 ulTrackSelectionMask,
		   CQTSwitchTrackMemberTable* &pSwitchTrackMemberTable);

    CQTSwitchTrack(CQTSwitchTrackMemberTable* pSwitchTrackMemberTable,
		   CQTTrack* pPrimaryTrack,
		   UINT32 ulTrackSelectionMask);

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
    virtual ULONG32 GetID(void);

    virtual ULONG32 GetTrackWidth(void);
    virtual ULONG32 GetTrackHeight(void);

    virtual ULONG32 GetSDPLength(void);
    virtual const UINT8* GetSDP(void);

    virtual HX_RESULT ObtainTrackBitrate(ULONG32& ulAvgBitrateOut);

    virtual CQT_ChunkToOffset_Manager* GetChunkToOffsetMgr(void);

    /*
     *	IQTTrackResponse Interface
     */
    virtual HX_RESULT PacketReady(UINT16 uStreamNum, 
				  HX_RESULT status, 
				  IHXPacket* pPacket);

    /*
     *  IHXCallback methods
     */
    STDMETHOD(Func)		    (THIS);

    /*
     *	IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ 
				    REFIID ID, 
				    void** ppInterfaceObj);

    STDMETHOD_(UINT32, AddRef )	    (THIS);

    STDMETHOD_(UINT32, Release)	    (THIS);

protected:
    typedef enum
    {
	QTSWT_Offline,
	QTSWT_Ready,
	QTSWT_Priming
    } QTSwitchTrackState;

    class CSwitchBoardEntry
    {
    public:
	CSwitchBoardEntry()
	    : m_bIsTrackSubscribed(FALSE)
	    , m_bIsSubscriptionPending(FALSE)
	    , m_pNextPacket(NULL)
	{
	    ;
	}

	~CSwitchBoardEntry()
	{
	    HX_RELEASE(m_pNextPacket);
	}

	HXBOOL m_bIsTrackSubscribed;
	HXBOOL m_bIsSubscriptionPending;
	IHXPacket* m_pNextPacket;
    };

    class CRunBoardEntry
    {
    public:
	CRunBoardEntry()
	    : m_uMemberId(QTSWT_BAD_TRACK_ID)
	    , m_pQTTrack(NULL)
	    , m_bTrackDone(FALSE)
	    , m_pNextPacket(NULL)
	{
	    ;
	}

	~CRunBoardEntry()
	{
	    HX_RELEASE(m_pNextPacket);
	    HX_RELEASE(m_pQTTrack);
	}

	void Clear()
	{
	    m_uMemberId = QTSWT_BAD_TRACK_ID;
	    m_bTrackDone = FALSE;
	    HX_RELEASE(m_pNextPacket);
	    HX_RELEASE(m_pQTTrack);
	}

	UINT16 m_uMemberId;
	CQTTrack* m_pQTTrack;
	HXBOOL m_bTrackDone;
	IHXPacket* m_pNextPacket;
    };

    void _InitOnConstruct(CQTTrack* pPrimaryTrack,
			  UINT32 ulTrackSelectionMask,
			  CQTSwitchTrackMemberTable* pSwitchTrackMemberTable);

    HX_RESULT _Subscribe(UINT16 uRuleNumber, HXBOOL bDoSubscribe);
    
    HX_RESULT ProcessPacket(UINT16 uStreamNum, 
			    HX_RESULT status, 
			    IHXPacket* pPacket,
			    UINT16 uMemberId);
    HX_RESULT Switch(HXBOOL bOnSeek = FALSE,
		     HXBOOL bUseSyncPoints = FALSE);

    IHXCommonClassFactory* m_pClassFactory;
    IHXScheduler* m_pScheduler;

    CQTSwitchTrackMemberTable* m_pMemberTable;
    UINT16 m_uNumTracks;
    CQTTrack* m_pPrimaryTrack;
    UINT16 m_uPrimaryMemberId;
    UINT32 m_ulTrackSelectionMask;

    CQTFileFormat* m_pFileFormat;
    IQTTrackResponse* m_pResponse;

    CSwitchBoardEntry* m_pSwitchBoard;
    CRunBoardEntry* m_pRunBoard;
    CRunBoardEntry* m_pRunBoardEntryPending;

    HXBOOL m_bSwitchBoardDirty;

    UINT32 m_ulLastReturnedPacketTime;
    HXBOOL m_bFirstPacket;
    HXBOOL m_bFirstPacketAfterSwitch;

    UINT16 m_uStreamNum;
    UINT32 m_ulRecursionCount;

    QTSwitchTrackState m_State;

private:
    virtual ~CQTSwitchTrack();

    LONG32 m_lRefCount;
};

#endif  // _QTSWTRACK_H_
