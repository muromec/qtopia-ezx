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

#ifndef _QTTRKMGR_H_
#define _QTTRKMGR_H_

/****************************************************************************
 *  Includes
 */
#include "iqttrack.h"
#include "qtatoms.h"
#include "qttrack.h"

class CQTFileFormat;
class CQTPacketAssembler;
class CQTTrack;
class CQTSwitchTrack;
class CQTPacketizerFactory;

/****************************************************************************
 *  Globals
 */
typedef enum
{
    QT_FTYPE_UNKNOWN,
    QT_FTYPE_QT,
    QT_FTYPE_MP4,
    QT_FTYPE_EMC
} QT_FTYPE;

typedef enum
{
    QT_ETYPE_UNKNOWN,
    QT_ETYPE_SERVER,
    QT_ETYPE_CLIENT
} QT_ETYPE;

typedef enum
{
    QTGROUP_NONE,	// Not groupped
    QTGROUP_LOOSE,	// Loose groups are not bound by same StreamGroupNumber (e.g. old QuickTime alternate groups)
    QTGROUP_TIGHT	// Tight groups are bound by same StreamGroupNumber (e.g. 3GPP-Rel6 alternate groups)
} QTTrackGroupType;

/****************************************************************************
 * 
 *  Class:
 *	CTrackManager
 *
 *  Purpose:
 *	Provides mapping of tracks to streams and maintains
 *  collective information about the tracks including the
 * tracks themselves.
 */
class CQTTrackManager
{
public:
    /*
     *	Constructor/Destructor
     */
    CQTTrackManager(void);

    ~CQTTrackManager();

    /*
     *	Main Interface
     */
    HX_RESULT ManageTracks(CQTAtom *pRootAtom);
    HX_RESULT ReadyTracks(HXBOOL bIgnoreHintTracks = FALSE,
			  HXBOOL bFallbackToTracks = FALSE);
    HX_RESULT InitTracks(CQTFileFormat *pFileFormat,
			 CQTPacketAssembler *pPacketAssembler,
			 CQTPacketizerFactory* pPacketizerFactory,
			 const char* pProtocol = NULL);
    HX_RESULT AddStreamToGroup(UINT16 uStreamNumber,
			       UINT16 uAltGroupId, 
			       ULONG32 ulBandwidth,
			       QTTrackGroupType eGroupType = QTGROUP_LOOSE);
    HX_RESULT InitStreamGroups(CQTFileFormat *pFileFormat,
			       CQTPacketAssembler* pPacketAssembler);
    HX_RESULT SubscribeDefault(void);
    HX_RESULT Subscribe(UINT16 uStreamNum, UINT16 uRuleNumber);
    HX_RESULT Unsubscribe(UINT16 uStreamNum, UINT16 uRuleNumber);
    HX_RESULT RemoveInactiveStreams(HXBOOL bLeaveReferences = FALSE);

    void CloseTracks(void);
    void ResetTracks(void);

    UINT16  GetNumStreams(void)         { return m_uNumStreams; }
    UINT16  GetNumStreamGroups(void)    { return m_uNumStreamGroups; }
    UINT16  GetNumTracks(void)          { return m_uNumTracks; }
    UINT16  GetNumActiveStreams(void);
    
    CQTTrack* GetTrack(UINT16 uTrackIndex)
    {
	return m_pTrackTable[uTrackIndex].m_pTrack;
    }
    IQTTrack* GetStreamTrack(UINT16 uStreamNumber)
    {
	return m_pStreamToTrackMap[uStreamNumber].m_pIQTTrack;
    }
    CQTTrack* GetTrackById(ULONG32 ulTrackID);
    CQTAtom* GetTrackAtomById(ULONG32 ulTrackID);
    HXBOOL IsStreamTrack(IQTTrack* pTrack);

    UINT16 GetStreamNumberByTrackId(ULONG32 ulTrackId);

    HXBOOL IsStreamTrackActive(UINT16 uStreamNumber)
    {
	return m_pStreamToTrackMap[uStreamNumber].m_bActive;
    }

    HXBOOL AreStreamGroupsPresent(void);
    UINT16 GetStreamGroupNumber(UINT16 uStreamNumber)
    {
	return m_pStreamToTrackMap[uStreamNumber].m_uStreamGroupNumber;
    }
    HXBOOL IsStreamDefaultAlternate(UINT16 uStreamNumber)
    {
	return m_pStreamToTrackMap[uStreamNumber].m_bDefaultAlternate;
    }

    QT_FTYPE GetFType(void)	{ return m_FType; }
    HX_RESULT GetMajorBrand(UINT32* pulMajorBrand);
    QT_ETYPE GetEType(void)	{ return m_EType; }
    void SetEType(QT_ETYPE eEType)  { m_EType = eEType; }
    HXBOOL IsHintsEnabled(void)	{ return m_bHintTracksActive; }
    HXBOOL IsHinted(void)		{ return m_bHinted; }

    static CQT_hdlr_Atom* GetTrackAtomHdlr(CQT_trak_Atom* pTrakAtom);
    static CQT_stbl_Atom* GetTrackAtomStbl(CQT_trak_Atom* pTrakAtom);
    static HXBOOL IsHintTrackAtom(CQT_trak_Atom* pTrakAtom);
    static HXBOOL IsNonEmptyTrackAtom(CQT_trak_Atom* pTrakAtom);
    
private:
    class CQTTrackTable
    {
    public:
	CQTTrackTable(void)
	    : m_pTrack(NULL)
	    , m_pTrackAtom(NULL)
	    , m_uRefCount(0)
	{
	    ;
	}

	~CQTTrackTable();

	void Clear(void);

	CQTTrack* m_pTrack;
	CQTAtom* m_pTrackAtom;
	UINT16 m_uRefCount;
    };

    class CQTStream
    {
    public:
	CQTStream(void)
	    : m_pIQTTrack(NULL)
	    , m_pQTSwitchTrack(NULL)
	    , m_bActive(FALSE)
	    , m_eGroupType(QTGROUP_NONE)
	    , m_uAltGroupId(0)
	    , m_uStreamGroupNumber(0)
	    , m_ulSwitchGroupId(0)
	    , m_ulStreamSelectionMask(0)
	    , m_ulBandwidth(0)
	    , m_pLanguage(NULL)
	    , m_bDefaultAlternate(FALSE)
	{
	    ;
	}

	~CQTStream();

	void Clear(void);
	void Detach(void);

	IQTTrack*	    m_pIQTTrack;
	CQTSwitchTrack*	    m_pQTSwitchTrack;
	HXBOOL		    m_bActive;
	QTTrackGroupType    m_eGroupType;
	UINT16		    m_uAltGroupId;
	UINT16		    m_uStreamGroupNumber;
	ULONG32		    m_ulSwitchGroupId;
	ULONG32		    m_ulStreamSelectionMask;
	ULONG32		    m_ulBandwidth;
	IHXBuffer*	    m_pLanguage;
	HXBOOL		    m_bDefaultAlternate;
    };

    void DecideHintTrackActivation(HXBOOL bFallbackToTracks);
    HX_RESULT CreateHintTrack(CQTAtom* pTrackAtom, 
			      UINT16 uTrackIdx,
			      UINT16& uStreamTrackCount);

    HX_RESULT SequentializeStreamGroupNumbers(void);
    void ReleaseStreamTrack(IQTTrack* pQTTrack);
    void AddTracks(CQTAtom* pRootAtom);
    void DeleteTrackAtomList(void);
    void Clear(void);    

    UINT16 GetTrackStreamNum(IQTTrack* pIQTTrack);

    UINT16 m_uNumTracks;
    UINT16 m_uNumStreams;
    UINT16 m_uNumStreamGroups;

    HXBOOL m_bHinted;
    HXBOOL m_bHintTracksActive;
    QT_FTYPE m_FType;
    QT_ETYPE m_EType;

    HXBOOL m_bInitialSubscriptionWindowClosed;

    CQTTrackTable* m_pTrackTable;
    CQTStream* m_pStreamToTrackMap;

    CHXSimpleList* m_pTrackAtomList;
    CHXSimpleList* m_pHintTrackIdxList;

    CQT_iods_Atom* m_pIodsAtom;
    CQT_ftyp_Atom* m_pFtypAtom;
};

#endif  // _QTTRKMGR_H_
