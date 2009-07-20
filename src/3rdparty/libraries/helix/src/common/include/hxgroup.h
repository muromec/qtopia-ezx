/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxgroup.h,v 1.5 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _IHXGROUP_H_
#define _IHXGROUP_H_

/****************************************************************************
 *
 * Forward declarations of some interfaces defined/used here-in.
 */
typedef _INTERFACE   IHXStreamSource		    IHXStreamSource;
typedef _INTERFACE   IHXGroup			    IHXGroup;
typedef _INTERFACE   IHXGroup2			    IHXGroup2;
typedef _INTERFACE   IHXGroupManager		    IHXGroupManager;
typedef _INTERFACE   IHXGroupSink		    IHXGroupSink;
typedef _INTERFACE   IHXGroupSink2		    IHXGroupSink2;
typedef _INTERFACE   IHXValues			    IHXValues;
typedef _INTERFACE   IHXTrack			    IHXTrack;
typedef _INTERFACE   IHXTrackSink		    IHXTrackSink;
typedef _INTERFACE   IHXPrefetch		    IHXPrefetch;
typedef _INTERFACE   IHXPrefetchSink		    IHXPrefetchSink;

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXGroup
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXGroup:
 * 
 *  {0x00002400-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXGroup, 0x00002400, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXGroup

DECLARE_INTERFACE_(IHXGroup, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

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
				     IHXValues*  /*IN*/ pProperties) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup::GetGroupProperties
    *  Purpose:
    *		Get any group specific information. May return NULL.
    */
    STDMETHOD_(IHXValues*, GetGroupProperties)   (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup::GetTrackCount
    *  Purpose:
    *		Get the number of tracks within this group.
    */
    STDMETHOD_(UINT16,GetTrackCount)    (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup::GetTrack
    *  Purpose:
    *		Get ith track in this group
    */
    STDMETHOD(GetTrack)	(THIS_
			UINT16 	    /*IN*/ uTrackIndex,
			REF(IHXValues*)  /*OUT*/ pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup::AddTrack
    *  Purpose:
    *		Add Tracks to the group.
    */
    STDMETHOD(AddTrack)	(THIS_
			IHXValues*	/*IN*/ pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup::RemoveTrack
    *  Purpose:
    *		Remove an already added track
    */
    STDMETHOD(RemoveTrack)  (THIS_
			    UINT16	/*IN*/ uTrackIndex) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXGroup2
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXGroup2:
 * 
 *  {0x00002403-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXGroup2, 0x00002403, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXGroup2

DECLARE_INTERFACE_(IHXGroup2, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXGroup2 methods
     */
    /************************************************************************
    *  Method:
    *      IHXGroup2::GetIHXTrack
    *  Purpose:
    *		Get ith track in this group
    */
    STDMETHOD(GetIHXTrack)	(THIS_
				UINT16 		/*IN*/	uTrackIndex,
				REF(IHXTrack*)	/*OUT*/ pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup2::AddTrackSink
    *  Purpose:
    *		add advise sink on track
    */
    STDMETHOD(AddTrackSink)	(THIS_
				IHXTrackSink*   /*IN*/  pSink) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup2::RemoveTrackSink
    *  Purpose:
    *		remove advise sink on track
    */
    STDMETHOD(RemoveTrackSink)	(THIS_
				IHXTrackSink*   /*IN*/  pSink) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup2::AddTrack2
    *  Purpose:
    *		Add Tracks to the group, including the props set in RequestHeader
    */
    STDMETHOD(AddTrack2)	    (THIS_
				    IHXValues*	/*IN*/ pTrack,
				    IHXValues* /*IN*/ pRequestProp) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup2::GetTrack2
    *  Purpose:
    *		Get ith track in this group
    */
    STDMETHOD(GetTrack2)	    (THIS_
				    UINT16 		/*IN*/	uTrackIndex,
				    REF(IHXValues*)	/*OUT*/ pTrack,
				    REF(IHXValues*)	/*OUT*/	pTrackPropInRequest) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup2::SetPersistentComponentProperties
    *  Purpose:
    *		Set persistent component properties associated with this group
    *		One group may contain multiple persistent components
    */
    STDMETHOD(SetPersistentComponentProperties)   (THIS_
						   UINT32	/*IN*/ ulPersistentComponentID,
						   IHXValues*  /*IN*/ pProperties) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup2::GetPersistentComponentProperties
    *  Purpose:
    *		Get any persistent component specific information associated with 
    *		the group.
    *		One group may contain multiple persistent components
    */
    STDMETHOD(GetPersistentComponentProperties)   (THIS_
						   UINT32	    /*IN*/  ulPersistentComponentID,
						   REF(IHXValues*) /*OUT*/ pProperties) PURE;
};
// $EndPrivate.

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXGroupManager
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXGroupManager:
 * 
 *  {0x00002401-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXGroupManager, 0x00002401, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXGroupManager

DECLARE_INTERFACE_(IHXGroupManager, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXGroupManager methods
     */

    /************************************************************************
    *  Method:
    *      IHXGroupManager::CreateGroup
    *  Purpose:
    *		Create a group
    */
    STDMETHOD(CreateGroup)    (REF(IHXGroup*) pGroup) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupManager::GetGroupCount
    *  Purpose:
    *		Get the number of groups within the presentation.
    */
    STDMETHOD_(UINT16,GetGroupCount)    (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupManager::GetGroup
    *  Purpose:
    *		Get ith group in the presentation
    */
    STDMETHOD(GetGroup)	(THIS_
			UINT16 		  /*IN*/  uGroupIndex,
			REF(IHXGroup*)  /*OUT*/ pGroup) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupManager::SetCurrentGroup
    *  Purpose:
    *		Play this group in the presentation.
    */
    STDMETHOD(SetCurrentGroup)	(THIS_
				UINT16 	    /*IN*/ uGroupIndex) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupMgr::GetCurrentGroup
    *  Purpose:
    *		Get the current group index
    */
    STDMETHOD(GetCurrentGroup)	(THIS_
				REF(UINT16) /*OUT*/ uGroupIndex) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupManager::AddGroup
    *  Purpose:
    *		Add a group to the presentation.
    */
    STDMETHOD(AddGroup)	(THIS_
			IHXGroup*	    /*IN*/ pGroup) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupManager::RemoveGroup
    *  Purpose:
    *		Remove an already added group
    */
    STDMETHOD(RemoveGroup)  (THIS_
			    UINT16 	/*IN*/ uGroupIndex) PURE;


    /************************************************************************
    *  Method:
    *      IHXGroupManager::AddSink
    *  Purpose:
    *		Add a sink to get notifications about any tracks or groups
    *		being added to the presentation.
    */
    STDMETHOD(AddSink)	(THIS_
			IHXGroupSink* /*IN*/ pGroupSink) PURE;


    /************************************************************************
    *  Method:
    *      IHXGroupManager::RemoveSink
    *  Purpose:
    *		Remove Sink
    */
    STDMETHOD(RemoveSink)   (THIS_
			    IHXGroupSink* /*IN*/ pGroupSink) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup::GetPresentationProperties
    *  Purpose:
    *		Get any presentation information. May return NULL.
    */
    STDMETHOD_(IHXValues*, GetPresentationProperties)   (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroup::SetPresentationProperties
    *  Purpose:
    *		Set any presentation information like Title Author 
    *		Copyright etc. 
    */
    STDMETHOD(SetPresentationProperties)   (THIS_
					    IHXValues*  /*IN*/ pProperties) PURE;
   
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXGroupSink
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXGroupSink:
 * 
 *  {0x00002402-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXGroupSink, 0x00002402, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXGroupSink

DECLARE_INTERFACE_(IHXGroupSink, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXGroupSink methods
     */
    /************************************************************************
    *  Method:
    *      IHXGroupSink::GroupAdded
    *  Purpose:
    *		Notification of a new group being added to the presentation.
    */
    STDMETHOD(GroupAdded)    (THIS_
			    UINT16 	    /*IN*/ uGroupIndex,
			    IHXGroup*	    /*IN*/ pGroup) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink::GroupRemoved
    *  Purpose:
    *		Notification of a group being removed from the presentation.
    */
    STDMETHOD(GroupRemoved)    (THIS_
				UINT16 	    /*IN*/ uGroupIndex,
				IHXGroup*  /*IN*/ pGroup) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink::AllGroupsRemoved
    *  Purpose:
    *		Notification that all groups have been removed from the 
    *		current presentation.
    */
    STDMETHOD(AllGroupsRemoved)  (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackAdded
    *  Purpose:
    *		Notification of a new track being added to a group.
    */
    STDMETHOD(TrackAdded)  (THIS_
			    UINT16 	    /*IN*/ uGroupIndex,
			    UINT16 	    /*IN*/ uTrackIndex,
			    IHXValues*	    /*IN*/ pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackRemoved
    *  Purpose:
    *		Notification of a track being removed from a group.
    */
    STDMETHOD(TrackRemoved)    (THIS_
				UINT16 		/*IN*/ uGroupIndex,
				UINT16 		/*IN*/ uTrackIndex,
				IHXValues*	/*IN*/ pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackStarted
    *  Purpose:
    *		Notification of a track being started (to get duration, for
    *		instance...)
    */
    STDMETHOD(TrackStarted)	(THIS_
				UINT16	    /*IN*/ uGroupIndex,
				UINT16	    /*IN*/ uTrackIndex,
				IHXValues* /*IN*/ pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackStopped
    *  Purpose:
    *		Notification of a track being stopped
    *
    */
    STDMETHOD(TrackStopped)	(THIS_
				UINT16	    /*IN*/ uGroupIndex,
				UINT16	    /*IN*/ uTrackIndex,
				IHXValues* /*IN*/ pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink::CurrentGroupSet
    *  Purpose:
    *		This group is being currently played in the presentation.
    */
    STDMETHOD(CurrentGroupSet)	(THIS_
				UINT16 	    /*IN*/ uGroupIndex,
				IHXGroup*  /*IN*/ pGroup) PURE;
};


// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXPreCacheGroupMgr
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXPreCacheGroupMgr:
 * 
 *  {00000E00-b4c8-11d0-9995-00a0248da5f0}
 * 
 */
DEFINE_GUID(IID_IHXPreCacheGroupMgr,    0x00000E00, 0xb4c8, 0x11d0, 
			0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);


#undef  INTERFACE
#define INTERFACE   IHXPreCacheGroupMgr

DECLARE_INTERFACE_(IHXPreCacheGroupMgr, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXPreCacheGroupMgr::SetNextGroup
    *  Purpose:
    *		Play this as the next group in the presentation.
    */
    STDMETHOD(SetNextGroup)	(THIS_
				UINT16 	    /*IN*/ uGroupIndex) PURE;
                
    /************************************************************************
    *  Method:
    *      IHXPreCacheGroupMgr::GetNextGroup
    *  Purpose:
    *		Get the next group to be played in the presentation.
    */
    STDMETHOD(GetNextGroup)	(THIS_ REF(UINT16) uGroupIndex) PURE;

    /************************************************************************
    *  Method:
    *      IHXPreCacheGroupMgr::DefaultNextGroup
    *  Purpose:
    *		Reset to the default next group to be played in the presentation.
    */
    STDMETHOD(DefaultNextGroup)	(THIS) PURE;
};
// $EndPrivate.

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXTrack
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXTrack:
 * 
 *  {0x00002404-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXTrack, 0x00002404, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXTrack

DECLARE_INTERFACE_(IHXTrack, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXTrack methods
     */
    /************************************************************************
    *  Method:
    *	    IHXTrack::Begin()
    *  Purpose:
    *	    start the track
    */
    STDMETHOD(Begin)	(THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::Pause()
    *  Purpose:
    *	    pause the track
    */
    STDMETHOD(Pause)	(THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::Seek()
    *  Purpose:
    *	    seek the track
    */
    STDMETHOD(Seek)	(THIS_
			UINT32 ulSeekTime) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::Stop()
    *  Purpose:
    *	    stop the track
    */
    STDMETHOD(Stop)	(THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::AddRepeat()
    *  Purpose:
    *	    add repeat tracks
    */
    STDMETHOD(AddRepeat)(THIS_
			IHXValues* pValues) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::GetTrackProperties()
    *  Purpose:
    *	    get track properties
    */
    STDMETHOD(GetTrackProperties)(THIS_
				 REF(IHXValues*) pValues,
				 REF(IHXValues*) pValuesInRequest) PURE;

    /************************************************************************
     *	Method:
     *	    IHXTrack::GetSource
     *	Purpose:
     *	    Returns the Nth source instance supported by this player.
     */
    STDMETHOD(GetSource)	(THIS_
				REF(IHXStreamSource*)	pStreamSource) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::SetSoundLevel()
    *  Purpose:
    *	    Set Audio Level
    */
    STDMETHOD(SetSoundLevel)(THIS_
			    UINT16 uSoundLevel) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::GetSoundLevel()
    *  Purpose:
    *	    Get Audio Level
    */
    STDMETHOD_(UINT16, GetSoundLevel)(THIS) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::BeginSoundLevelAnimation()
    *  Purpose:
    *	    notify the start of soundlevel animation
    */
    STDMETHOD(BeginSoundLevelAnimation)(THIS_
					UINT16 uSoundLevelBeginWith) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrack::EndSoundLevelAnimation()
    *  Purpose:
    *	    notify the stop of soundlevel animation
    */
    STDMETHOD(EndSoundLevelAnimation)(THIS_
				      UINT16 uSoundLevelEndWith) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXTrackSink
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXTrackSink:
 * 
 *  {0x00002405-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXTrackSink, 0x00002405, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXTrackSink

DECLARE_INTERFACE_(IHXTrackSink, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXTrackSink methods
     */
    /************************************************************************
    *  Method:
    *	    IHXTrackSink::BeginDone()
    *  Purpose:
    *	    done with begin
    */
    STDMETHOD(BeginDone)	(THIS_
				UINT16	uGroupIndex,
				UINT16	uTrackIndex) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrackSink::SeekDone()
    *  Purpose:
    *	    done with seek
    */
    STDMETHOD(SeekDone)		(THIS_
				UINT16	uGroupIndex,
				UINT16	uTrackIndex,
				UINT32	ulSeekTime) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrackSink::PauseDone()
    *  Purpose:
    *	    done with pause
    */
    STDMETHOD(PauseDone)	(THIS_
				UINT16	uGroupIndex,
				UINT16	uTrackIndex) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrackSink::StopDone()
    *  Purpose:
    *	    done with stop
    */
    STDMETHOD(StopDone)		(THIS_
				UINT16	uGroupIndex,
				UINT16	uTrackIndex) PURE;

    /************************************************************************
    *  Method:
    *	    IHXTrackSink::OnSoundLevelAnimation()
    *  Purpose:
    *	    sound level animation is in process
    */
    STDMETHOD(OnSoundLevelAnimation)	(THIS_
					UINT16	uGroupIndex,
					UINT16	uTrackIndex,
					UINT32	ulSoundLevelAnimationTime) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXGroupSink2
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXGroupSink2:
 * 
 *  {0x00002407-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXGroupSink2, 0x00002407, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
            0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXGroupSink2

DECLARE_INTERFACE_(IHXGroupSink2, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
                    REFIID riid,
                    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXGroupSink2 methods
     */
    /************************************************************************
    *  Method:
    *      IHXGroupSink2::GroupInsertedBefore
    *  Purpose:
    *		Notification of a new group being inserted before the group
    */
    STDMETHOD(GroupInsertedBefore)    (THIS_
				       UINT16 	    /*IN*/ uBeforeGroupIndex,
				       IHXGroup*   /*IN*/ pGroup) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink2::GroupInsertedAfter
    *  Purpose:
    *		Notification of a new group being inserted after the group
    */
    STDMETHOD(GroupInsertedAfter)    (THIS_
				      UINT16 	    /*IN*/ uAfterGroupIndex,
				      IHXGroup*    /*IN*/ pGroup) PURE;

    /************************************************************************
    *  Method:
    *      IHXGroupSink2::GroupReplaced
    *  Purpose:
    *		Notification of a new group replace the current group
    *		current presentation.
    */
    STDMETHOD(GroupReplaced)	    (THIS_
				     UINT16	    /*IN*/ uGroupIndex,
				     IHXGroup*	    /*IN*/ pGroup) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPrefetch
 *
 *  Purpose:
 *
 *	Interface to manage prefetch
 *
 *  IID_IHXPrefetch:
 *
 *	{00002408-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPrefetch, 0x00002408, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			     0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXPrefetch, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXPrefetch methods
     */

    /************************************************************************
    *  Method:
    *      IHXPrefetch::AddPrefetchTrack
    *  Purpose:
    *      adds prefetch track by specifying "PrefetchType" and "PrefetchValue"
    *      in pTrack's IHXValues
    */
    STDMETHOD(AddPrefetchTrack)	(THIS_
				 IHXValues* /*IN*/  pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXPrefetch::RemovePrefetchTrack
    *  Purpose:
    *      removes prefetched track
    */
    STDMETHOD(RemovePrefetchTrack)  (THIS_
				     UINT16 /*IN*/ uTrackIndex) PURE;

    /************************************************************************
    *  Method:
    *      IHXPrefetch::GetPrefetchTrackCount
    *  Purpose:
    *      get number of prefetch tracks added
    */
    STDMETHOD_(UINT16,GetPrefetchTrackCount)    (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXPrefetch::GetPrefetchTrack
    *  Purpose:
    *      get prefetch track based on the index
    */
    STDMETHOD(GetPrefetchTrack) (THIS_
				 UINT16           /*IN*/  uTrackIndex,
				 REF(IHXValues*) /*OUT*/ pTrack) PURE;

    /************************************************************************
    *  Method:
    *      IHXPrefetch::AddPrefetchSink
    *  Purpose:
    *      add prefetch sink
    */
    STDMETHOD(AddPrefetchSink) (THIS_
				IHXPrefetchSink* /*IN*/ pSink) PURE;

    /************************************************************************
    *  Method:
    *      IHXPrefetch::RemovePrefetchSink
    *  Purpose:
    *      remove prefetch sink
    */
    STDMETHOD(RemovePrefetchSink) (THIS_
				   IHXPrefetchSink* /*IN*/ pSink) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPrefetchSink
 *
 *  Purpose:
 *
 *	Sink Interface to IHXPrefetch
 *
 *  IID_IHXPrefetchSink
 *
 *	{00002406-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXPrefetchSink, 0x00002406, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
				    0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXPrefetchSink, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXPrefetchSink methods
     */

    /************************************************************************
    *  Method:
    *	    IHXPrefetchSink::PrefetchTrackAdded()
    *  Purpose:
    *	    prefetch track is added
    */
    STDMETHOD(PrefetchTrackAdded)   (THIS_
				     UINT16	 uGroupIndex,
				     UINT16      uTrackIndex,
				     IHXValues* pTrack) PURE;

    /************************************************************************
    *  Method:
    *	    IHXPrefetchSink::PrefetchTrackRemoved()
    *  Purpose:
    *	    prefetch track is removed
    */
    STDMETHOD(PrefetchTrackRemoved) (THIS_
				     UINT16	 uGroupIndex,
				     UINT16      uTrackIndex,
				     IHXValues* pTrack) PURE;

    /************************************************************************
    *  Method:
    *	    IHXPrefetchSink::PrefetchTrackDone()
    *  Purpose:
    *	    prefetch track is done
    */
    STDMETHOD(PrefetchTrackDone) (THIS_
				  UINT16    uGroupIndex,
				  UINT16    uTrackIndex,
				  HX_RESULT status) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXGroup)
DEFINE_SMART_PTR(IHXGroup2)
DEFINE_SMART_PTR(IHXGroupManager)
DEFINE_SMART_PTR(IHXGroupSink)
DEFINE_SMART_PTR(IHXPreCacheGroupMgr)
DEFINE_SMART_PTR(IHXTrack)
DEFINE_SMART_PTR(IHXTrackSink)
DEFINE_SMART_PTR(IHXGroupSink2)
DEFINE_SMART_PTR(IHXPrefetch)
DEFINE_SMART_PTR(IHXPrefetchSink)

#endif /*_IHXGROUP_H_*/
