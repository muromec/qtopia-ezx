/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: basgroup.h,v 1.6 2007/07/06 21:58:11 jfinnecy Exp $
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

#ifndef _BASICGROUP_H_
#define _BASICGROUP_H_

struct	IHXValues;
struct	IHXGroup;
struct	IHXGroupManager;
struct	IHXGroupSink;
struct	IHXTrack;
struct	IHXStreamSource;

class	CHXSimpleList;
class	HXBasicGroup;
class	HXBasicGroupManager;
class   HXPlayer;
class	CHXMapStringToOb;
class	HXBasicTrack;
class   HXMasterTAC;

class HXBasicTrack :  public IHXTrack
{
protected:
    LONG32		    m_lRefCount;
    HXBasicGroup*           m_pHXGroup;
    IHXValues*		    m_pValues;
    IHXValues*		    m_pValuesInRequest;
    HXBOOL		    m_bActive;

    virtual		    ~HXBasicTrack();

    friend class HXBasicGroup;

public:
			    HXBasicTrack(HXBasicGroup*	pHXGroup);

    UINT16		    m_uTrackIndex;

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);

    /*
     *  IHXTrack methods
     */
    /************************************************************************
    *  Method:
    *	    IHXTrack::Begin()
    *  Purpose:
    *	    start the track
    */
    STDMETHOD(Begin)	(THIS);

    /************************************************************************
    *  Method:
    *	    IHXTrack::Pause()
    *  Purpose:
    *	    pause the track
    */
    STDMETHOD(Pause)	(THIS);

    /************************************************************************
    *  Method:
    *	    IHXTrack::Seek()
    *  Purpose:
    *	    seek the track
    */
    STDMETHOD(Seek)	(THIS_
			UINT32 ulSeekTime);

    /************************************************************************
    *  Method:
    *	    IHXTrack::Stop()
    *  Purpose:
    *	    stop the track
    */
    STDMETHOD(Stop)	(THIS);

    /************************************************************************
    *  Method:
    *	    IHXTrack::AddRepeat()
    *  Purpose:
    *	    add repeat tracks
    */
    STDMETHOD(AddRepeat)(THIS_
			IHXValues* pValues);

    /************************************************************************
    *  Method:
    *	    IHXTrack::GetTrackProperties()
    *  Purpose:
    *	    get track properties
    */
    STDMETHOD(GetTrackProperties)(THIS_
				 REF(IHXValues*) pValues,
				 REF(IHXValues*) pValuesInRequest);

    /************************************************************************
     *	Method:
     *	    IHXTrack::GetSource
     *	Purpose:
     *	    Returns the Nth source instance supported by this player.
     */
    STDMETHOD(GetSource)	(THIS_
				REF(IHXStreamSource*)	pStreamSource);

    /************************************************************************
    *  Method:
    *	    IHXTrack::SetSoundLevel()
    *  Purpose:
    *	    Set Audio Level
    */
    STDMETHOD(SetSoundLevel)(THIS_
			    UINT16 uSoundLevel);

    /************************************************************************
    *  Method:
    *	    IHXTrack::GetSoundLevel()
    *  Purpose:
    *	    Get Audio Level
    */
    STDMETHOD_(UINT16, GetSoundLevel)(THIS);

    /************************************************************************
    *  Method:
    *	    IHXTrack::BeginSoundLevelAnimation()
    *  Purpose:
    *	    notify the start of soundlevel animation
    */
    STDMETHOD(BeginSoundLevelAnimation)(THIS_
					UINT16 uSoundLevelBeginWith);

    /************************************************************************
    *  Method:
    *	    IHXTrack::EndSoundLevelAnimation()
    *  Purpose:
    *	    notify the stop of soundlevel animation
    */
    STDMETHOD(EndSoundLevelAnimation)(THIS_
				      UINT16 uSoundLevelEndWith);

    virtual HX_RESULT	SetTrackProperties(IHXValues* pValues,
				           IHXValues* pValuesInRequest);    

    virtual void        Close(void);
};

class HXBasicGroup :  public IHXGroup
{
protected:
    LONG32		    m_lRefCount;
    HXBasicGroupManager*    m_pGroupManager;
    HXPlayer*		    m_pPlayer;
    HXBOOL		    m_bToNotifyTrack;    

    UINT16		    m_uTrackCount;
    CHXMapLongToObj*	    m_pTrackMap;

    virtual		    ~HXBasicGroup();

    HX_RESULT       DoGetTrack(UINT16 uTrackIndex, 
                               REF(IHXValues*) pTrack, 
                               REF(IHXValues*) pTrackPropInRequest);

    HX_RESULT       DoAddTrack(IHXValues* pTrack,
                               IHXValues* pTrackPropInRequest,
                               HXBasicTrack* pHXTrack);
        
    void	    StartTrackNotification(void);

    friend class HXBasicTrack;
    friend class HXBasicGroupManager;
    friend class HXPlayer;

#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    friend class HXAdvancedTrack;
    friend class HXAdvancedGroupManager;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */

public:

    UINT16		    m_uGroupIndex;

			    HXBasicGroup(HXBasicGroupManager* pManager);
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);

    /*
     *  IHXGroup methods
     */
    /************************************************************************
    *  Method:
    *      IHXGroup::SetGroupProperties
    *  Purpose:
    *		Set any group specific information like Title Author 
    *		Copyright etc. 
    */
    STDMETHOD(SetGroupProperties)   (THIS_
				     IHXValues*  /*IN*/ pProperties);

    /************************************************************************
    *  Method:
    *      IHXGroup::GetGroupProperties
    *  Purpose:
    *		Get any group specific information. May return NULL.
    */
    STDMETHOD_(IHXValues*, GetGroupProperties)   (THIS);

    /************************************************************************
    *  Method:
    *      IHXGroup::GetTrackCount
    *  Purpose:
    *		Get the number of tracks within this group.
    */
    STDMETHOD_(UINT16,GetTrackCount)    (THIS);

    /************************************************************************
    *  Method:
    *      IHXGroup::GetTrack
    *  Purpose:
    *		Call this to hook audio data after all audio streams in this
    *		have been mixed.
    */
    STDMETHOD(GetTrack)	(THIS_
			UINT16 	    /*IN*/ uTrackIndex,
			REF(IHXValues*)  /*OUT*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroup::AddTrack
    *  Purpose:
    *		Add Tracks to the group.
    */
    STDMETHOD(AddTrack)	(THIS_
			IHXValues*	/*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroup::RemoveTrack
    *  Purpose:
    *		Remove an already added track
    */
    STDMETHOD(RemoveTrack)  (THIS_
			    UINT16	/*IN*/ uTrackIndex);

    /* Other public fuctions */
    virtual HX_RESULT	CurrentGroupSet(void);
    virtual void	Close(void);
};

class HXBasicGroupManager : public IHXGroupManager,
			    public IHXPreCacheGroupMgr
{
protected:
    LONG32		m_lRefCount;
    CHXMapLongToObj*	m_pGroupMap;
    CHXSimpleList*	m_pSinkList;
    IHXValues*		m_pPresentationProperties;
    
    // required for precache support
    UINT16	    m_uGroupCount;
    UINT16	    m_uCurrentGroup;
    UINT16 	    m_uNextGroup;
    HXBOOL 	    m_bDefaultNextGroup;
    HXBOOL	    m_bCurrentGroupInitialized;
    HXPlayer*	    m_pPlayer;

    virtual	    ~HXBasicGroupManager();

    HX_RESULT	    TrackAdded	(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pTrack);
    HX_RESULT	    TrackRemoved(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pTrack);

    void 	    RemoveAllGroup(void);

    friend class HXBasicGroup;
    friend class HXPlayer;

#if defined(HELIX_FEATURE_ADVANCEDGROUPMGR)
    friend class HXAdvancedGroup;
#endif /* HELIX_FEATURE_ADVANCEDGROUPMGR */

public:

		    HXBasicGroupManager(HXPlayer* pPlayer = NULL);
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);

    /*
     *  IHXGroupManager methods
     */

    /************************************************************************
    *  Method:
    *      IHXGroupManager::CreateGroup
    *  Purpose:
    *		Create a group
    */
    STDMETHOD(CreateGroup)    (REF(IHXGroup*) pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupManager::GetGroupCount
    *  Purpose:
    *		Get the number of groups within the presentation.
    */
    STDMETHOD_(UINT16,GetGroupCount)    (THIS);

    /************************************************************************
    *  Method:
    *      IHXGroupManager::GetGroup
    *  Purpose:
    *		Get ith group in the presentation
    */
    STDMETHOD(GetGroup)	(THIS_
			UINT16 		  /*IN*/  uGroupIndex,
			REF(IHXGroup*)  /*OUT*/ pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupManager::SetCurrentGroup
    *  Purpose:
    *		Play this group in the presentation.
    */
    STDMETHOD(SetCurrentGroup)	(THIS_
				UINT16 	    /*IN*/ uGroupIndex);

    /************************************************************************
    *  Method:
    *      IHXGroupManager::GetCurrentGroup
    *  Purpose:
    *		Get current group index
    */
    STDMETHOD(GetCurrentGroup)	(THIS_
				REF(UINT16) /*OUT*/ uGroupIndex);

    /************************************************************************
    *  Method:
    *      IHXGroupManager::AddGroup
    *  Purpose:
    *		Add a group to the presentation.
    */
    STDMETHOD(AddGroup)	(THIS_
			IHXGroup*	    /*IN*/ pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupManager::RemoveGroup
    *  Purpose:
    *		Remove an already added group
    */
    STDMETHOD(RemoveGroup)  (THIS_
			    UINT16 	/*IN*/ uGroupIndex);


    /************************************************************************
    *  Method:
    *      IHXGroupManager::AddSink
    *  Purpose:
    *		Add a sink to get notifications about any tracks or groups
    *		being added to the presentation.
    */
    STDMETHOD(AddSink)	(THIS_
			IHXGroupSink* /*IN*/ pGroupSink);


    /************************************************************************
    *  Method:
    *      IHXGroupManager::RemoveSink
    *  Purpose:
    *		Remove Sink
    */
    STDMETHOD(RemoveSink)   (THIS_
			    IHXGroupSink* /*IN*/ pGroupSink);

    /************************************************************************
    *  Method:
    *      IHXGroupManager::SetPresentationProperties
    *  Purpose:
    *		Set any presentation information like Title Author 
    *		Copyright etc. 
    */
    STDMETHOD(SetPresentationProperties)   (THIS_
					    IHXValues*  /*IN*/ pProperties);

    /************************************************************************
    *  Method:
    *      IHXGroupManager::GetPresentationProperties
    *  Purpose:
    *		Get any presentation information. May return NULL.
    */
    STDMETHOD_(IHXValues*, GetPresentationProperties)   (THIS);

    /************************************************************************
    *  Method:
    *      IHXPreCacheGroupMgr::SetNextGroup
    *  Purpose:
    *		Play this as the next group in the presentation.
    */
    STDMETHOD(SetNextGroup)	(THIS_
				UINT16 	    /*IN*/ uGroupIndex);
                
    /************************************************************************
    *  Method:
    *      IHXPreCacheGroupMgr::GetNextGroup
    *  Purpose:
    *		Get the next group to be played in the presentation.
    */
    STDMETHOD(GetNextGroup)	(THIS_ REF(UINT16) uGroupIndex);

    /************************************************************************
    *  Method:
    *      IHXPreCacheGroupMgr::DefaultNextGroup
    *  Purpose:
    *		Reset to default the next group to play in the presentation.
    */
    STDMETHOD(DefaultNextGroup)	(THIS);

    virtual void	    SetMasterTAC(HXMasterTAC* pMasterTAC);
    virtual HX_RESULT	    TrackStarted(UINT16 uGroupIndex, UINT16 uTrackIndex, HXBOOL bIsRepeating = FALSE);
    virtual HX_RESULT	    TrackStopped(UINT16 uGroupIndex, UINT16 uTrackIndex);
    virtual void	    PersistentComponentAdded(UINT16 uGroupIndex, UINT16 uTrackIndex);
    virtual void	    PersistentComponentRemoved(UINT16 uGroupIndex, UINT16 uTrackIndex);
    virtual HX_RESULT	    InsertGroupAt(UINT16 uGroupIndex, IHXGroup* pGroup);
    virtual void            Close(void);
};

#endif /*_BASICGROUP_H_*/
