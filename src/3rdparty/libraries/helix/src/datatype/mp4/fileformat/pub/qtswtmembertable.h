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

#ifndef _QTSWTMEMBERTABLE_H_
#define _QTSWTMEMBERTABLE_H_

/****************************************************************************
 *  Defines
 */
#define QTSWT_BAD_TRACK_ID  0xFFFF


/****************************************************************************
 *  Includes
 */
#include "iqttrack.h"
#include "qttrack.h"


/****************************************************************************
 * 
 *  Class:
 *	CQTSwitchTrackMemberTable
 *
 *  Purpose:
 *	Contains a collection of mutally switchable tracks, provides 
 *      ASMRuleBook descirption of the relationship between such streams
 *      and mapping between ASMRuleBook rules and associated tracks.
 */
class CQTSwitchTrackMemberTable : public IUnknown
{
public:
    /*
     *	Constructor/Destructor
     */
    CQTSwitchTrackMemberTable(UINT16 uNumTracks,
			      CQTTrack* pQTTracks[]);

    /*
     *	Main Interface
     */
    HX_RESULT Init(UINT32 ulTrackSelectionMask,
	           IHXCommonClassFactory* pClassFactory);

    IHXBuffer* GetASMRuleBook(void) { return m_pASMRuleBook; }

    UINT16 MapRuleToMemberId(UINT16 uRuleNum)
    {
	return (uRuleNum >> 1);
    }
    CQTTrack* MapRuleToTrack(UINT16 uRuleNum)
    {
	return m_pTrackTable[MapRuleToMemberId(uRuleNum)].m_pQTTrack;
    }
    UINT16 MapMemberIdToBaseRule(UINT16 uMemberId)
    {
	return (uMemberId << 1);
    }
    UINT16 GetNumRules(void)
    { 
	return (m_uNumTrackTableEntries << 1); 
    }
    UINT16 GetNumTracks(void)
    {
	return m_uNumTrackTableEntries;
    }

    CQTTrack* GetTrack(UINT16 uMemberId)
    {
	return m_pTrackTable[uMemberId].m_pQTTrack;
    }
    ULONG32 GetTrackBandwidth(UINT16 uMemberId)
    {
	return m_pTrackTable[uMemberId].m_ulBandwidth;
    }
    UINT16 GetMemberId(CQTTrack* pQTTrack);

    /*
     *	IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ 
				    REFIID ID, 
				    void** ppInterfaceObj);

    STDMETHOD_(UINT32, AddRef )	    (THIS);

    STDMETHOD_(UINT32, Release)	    (THIS);

protected:
    class CTrackEntry
    {
    public:
	CTrackEntry()
	    : m_pQTTrack(NULL)
	    , m_ulBandwidth(0)
	{
	    ;
	}

	~CTrackEntry()
	{
	    HX_RELEASE(m_pQTTrack);
	}

	void Detach(void)
	{
	    m_pQTTrack = NULL;
	}

	CQTTrack* m_pQTTrack;
	UINT32 m_ulBandwidth;
    };

    /*
     *	Protected utility methods
     */
    HX_RESULT ObtainBandwidth(CTrackEntry* pTrackEntry,
			      IHXCommonClassFactory* pClassFactory);
    HX_RESULT SortTrackTable(void);
    HX_RESULT EstablishASMRuleBook(IHXCommonClassFactory* pClassFactory);


    CTrackEntry* m_pTrackTable;
    UINT16 m_uNumTrackTableEntries;

    UINT32 m_ulTrackSelectionMask;

    IHXBuffer* m_pASMRuleBook;

    HXBOOL m_bInitialized;

private:
    virtual ~CQTSwitchTrackMemberTable(void);

    LONG32 m_lRefCount;
};

#endif  // _QTSWTMEMBERTABLE_H_
