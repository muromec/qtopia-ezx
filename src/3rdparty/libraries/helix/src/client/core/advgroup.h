/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: advgroup.h,v 1.6 2005/03/14 20:31:01 bobclark Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef _ADVANCEDGROUP_H_
#define _ADVANCEDGROUP_H_

#include "basgroup.h"

struct	IHXValues;
struct	IHXGroup;
struct	IHXGroup2;
struct	IHXGroupManager;
struct	IHXGroupSink;
struct	IHXTrack;
struct	IHXStreamSource;
struct	IHXAudioHook;
struct	IHXPrefetch;
struct	IHXPrefetchSink;

class	HXAdvancedTrack;
class	HXAdvancedGroup;
class	HXAdvancedGroupManager;
class	CHXSimpleList;
class   HXPlayer;
class	HXMasterTAC;
class	CHXMapStringToOb;

class HXAdvancedTrack : public HXBasicTrack
#if defined(HELIX_FEATURE_AUDIOHOOK)
	              , public IHXAudioHook
#endif /* HELIX_FEATURE_AUDIOHOOK */
{
protected:
    CHXSimpleList*	    m_pRepeatList;
    HXBOOL		    m_bInSoundLevelAnimation;
    UINT16		    m_uSoundLevel;
    UINT32		    m_ulSoundLevelAnimationTime;   
    UINT32		    m_ulGranularity;

    virtual		    ~HXAdvancedTrack();

    friend class HXAdvancedGroup;

public:
			    HXAdvancedTrack(HXAdvancedGroup* pHXGroup);

#if defined(HELIX_FEATURE_AUDIOHOOK)
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);

    /*
     *  IHXAudioHook methods
     */
    /************************************************************************
     *  Method:
     *      IHXAudioHook::OnInit
     *  Purpose:
     *      Audio Services calls OnInit() with the audio data format of the
     *	    audio data that will be provided in the OnBuffer() method.
     */
    STDMETHOD(OnInit)		(THIS_
                    		HXAudioFormat*	/*IN*/ pFormat);

    /************************************************************************
     *  Method:
     *      IHXAudioHook::OnBuffer
     *  Purpose:
     *      Audio Services calls OnBuffer() with audio data packets. The
     *	    renderer should not modify the data in the IHXBuffer part of
     *	    pAudioInData.  If the renderer wants to write a modified
     *	    version of the data back to Audio Services, then it should 
     *	    create its own IHXBuffer, modify the data and then associate 
     *	    this buffer with the pAudioOutData->pData member.
     */
    STDMETHOD(OnBuffer)		(THIS_
                    		HXAudioData*	/*IN*/   pAudioInData,
                    		HXAudioData*	/*OUT*/  pAudioOutData);
#endif /* HELIX_FEATURE_AUDIOHOOK */

    STDMETHOD(Begin)	            (THIS);
    STDMETHOD(Pause)	            (THIS);
    STDMETHOD(Seek)	            (THIS_
			            UINT32 ulSeekTime);
    STDMETHOD(Stop)	            (THIS);
    STDMETHOD(AddRepeat)            (THIS_
			            IHXValues* pValues);
    STDMETHOD(GetSource)	    (THIS_
				    REF(IHXStreamSource*) pStreamSource);
    STDMETHOD(SetSoundLevel)        (THIS_
			            UINT16 uSoundLevel);
    STDMETHOD_(UINT16, GetSoundLevel)(THIS);
    STDMETHOD(BeginSoundLevelAnimation)(THIS_
					UINT16 uSoundLevelBeginWith);
    STDMETHOD(EndSoundLevelAnimation)(THIS_
				      UINT16 uSoundLevelEndWith);
    virtual void    Close(void);
};

class HXAdvancedGroup : public HXBasicGroup
		      , public IHXGroup2
#if defined(HELIX_FEATURE_PREFETCH)
		      , public IHXPrefetch
		      , public IHXPrefetchSink
#endif /* HELIX_FEATURE_PREFETCH */
{
protected:
    IHXValues*		    m_pGroupProperties;
    HXBOOL		    m_bPrefetchSinkAdded;
    UINT16		    m_uPrefetchTrackCount;
    CHXMapLongToObj*	    m_pPrefetchTrackMap;
    CHXMapLongToObj*	    m_pPersistentComponentPropertyMap;
    CHXSimpleList*	    m_pTrackSinkList;
    CHXSimpleList*	    m_pPrefetchSinkList;

    virtual		    ~HXAdvancedGroup();
    
    friend class HXAdvancedTrack;
    friend class HXAdvancedGroupManager;
    friend class HXPlayer;

public:

                            HXAdvancedGroup(HXAdvancedGroupManager* pManager);
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD(SetGroupProperties)   (THIS_
				    IHXValues*  /*IN*/ pProperties);
    STDMETHOD_(IHXValues*, GetGroupProperties)   (THIS);
    STDMETHOD(AddTrack)	            (THIS_
			            IHXValues*	/*IN*/ pTrack);
    STDMETHOD(RemoveTrack)          (THIS_
			            UINT16	/*IN*/ uTrackIndex);

    /************************************************************************
    *  Method:
    *      IHXGroup2::GetIHXTrack
    *  Purpose:
    *		Get ith track in this group
    */
    STDMETHOD(GetIHXTrack)	(THIS_
				UINT16 		/*IN*/	uTrackIndex,
				REF(IHXTrack*)	/*OUT*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroup2::AddTrackSink
    *  Purpose:
    *		add advise sink on track
    */
    STDMETHOD(AddTrackSink)	(THIS_
				IHXTrackSink*   /*IN*/  pSink);

    /************************************************************************
    *  Method:
    *      IHXGroup2::RemoveTrackSink
    *  Purpose:
    *		remove advise sink on track
    */
    STDMETHOD(RemoveTrackSink)	(THIS_
				IHXTrackSink*   /*IN*/  pSink);

    /************************************************************************
    *  Method:
    *      IHXGroup2::AddTrack2
    *  Purpose:
    *		Add Tracks to the group, including the props set in RequestHeader
    */
    STDMETHOD(AddTrack2)	    (THIS_
				    IHXValues*	/*IN*/ pTrack,
				    IHXValues* /*IN*/ pRequestProp);

    /************************************************************************
    *  Method:
    *      IHXGroup2::GetTrack2
    *  Purpose:
    *		Get ith track in this group
    */
    STDMETHOD(GetTrack2)	    (THIS_
				    UINT16 		/*IN*/	uTrackIndex,
				    REF(IHXValues*)	/*OUT*/ pTrack,
				    REF(IHXValues*)	/*OUT*/	pTrackPropInRequest);

    /************************************************************************
    *  Method:
    *      IHXGroup2::SetPersistentComponentProperties
    *  Purpose:
    *		Set persistent component properties associated with this group
    *		One group may contain multiple persistent components
    */
    STDMETHOD(SetPersistentComponentProperties)   (THIS_
						   UINT32	/*IN*/ ulPersistentComponentID,
						   IHXValues*  /*IN*/ pProperties);

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
						   REF(IHXValues*) /*OUT*/ pProperties);

#if defined(HELIX_FEATURE_PREFETCH)
    /************************************************************************
    *  Method:
    *      IHXPrefetch::AddPrefetchTrack
    *  Purpose:
    *      adds prefetch track by specifying "PrefetchType" and "PrefetchValue"
    *      in pTrack's IHXValues
    */
    STDMETHOD(AddPrefetchTrack)	(THIS_
				 IHXValues* /*IN*/  pTrack);

    /************************************************************************
    *  Method:
    *      IHXPrefetch::RemovePrefetchTrack
    *  Purpose:
    *      removes prefetched track
    */
    STDMETHOD(RemovePrefetchTrack)  (THIS_
				     UINT16 /*IN*/ uTrackIndex);

    /************************************************************************
    *  Method:
    *      IHXPrefetch::GetPrefetchTrackCount
    *  Purpose:
    *      get number of prefetch tracks added
    */
    STDMETHOD_(UINT16,GetPrefetchTrackCount)    (THIS);

    /************************************************************************
    *  Method:
    *      IHXPrefetch::GetPrefetchTrack
    *  Purpose:
    *      get prefetch track based on the index
    */
    STDMETHOD(GetPrefetchTrack) (THIS_
				 UINT16           /*IN*/  uTrackIndex,
				 REF(IHXValues*) /*OUT*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXPrefetch::AddPrefetchSink
    *  Purpose:
    *      add prefetch sink
    */
    STDMETHOD(AddPrefetchSink) (THIS_
				IHXPrefetchSink* /*IN*/ pSink);

    /************************************************************************
    *  Method:
    *      IHXPrefetch::RemovePrefetchSink
    *  Purpose:
    *      remove prefetch sink
    */
    STDMETHOD(RemovePrefetchSink) (THIS_
				   IHXPrefetchSink* /*IN*/ pSink);

    /************************************************************************
    *  Method:
    *	    IHXPrefetchSink::PrefetchTrackAdded()
    *  Purpose:
    *	    prefetch track is added
    */
    STDMETHOD(PrefetchTrackAdded)   (THIS_
				     UINT16	 uGroupIndex,
				     UINT16      uTrackIndex,
				     IHXValues* pTrack);

    /************************************************************************
    *  Method:
    *	    IHXPrefetchSink::PrefetchTrackRemoved()
    *  Purpose:
    *	    prefetch track is removed
    */
    STDMETHOD(PrefetchTrackRemoved) (THIS_
				     UINT16	 uGroupIndex,
				     UINT16      uTrackIndex,
				     IHXValues* pTrack);

    /************************************************************************
    *  Method:
    *	    IHXPrefetchSink::PrefetchTrackDone()
    *  Purpose:
    *	    prefetch track is done
    */
    STDMETHOD(PrefetchTrackDone) (THIS_
				  UINT16    uGroupIndex,
				  UINT16    uTrackIndex,
				  HX_RESULT status);
#endif /* HELIX_FEATURE_PREFETCH */

    /* Other public fuctions */
    virtual HX_RESULT	CurrentGroupSet(void);
            HX_RESULT	RepeatTrackAdded(UINT16 uTrackIndex, IHXValues* pValues);

            HX_RESULT	BeginTrack(UINT16 uTrackIndex, IHXValues* pTrack);
            HX_RESULT	PauseTrack(UINT16 uTrackIndex, IHXValues* pTrack);
            HX_RESULT	SeekTrack(UINT16 uTrackIndex, IHXValues* pTrack, UINT32 ulSeekTime);
            HX_RESULT	StopTrack(UINT16 uTrackIndex, IHXValues* pTrack);
            HX_RESULT	GetSource(UINT16 uTrackIndex, IHXStreamSource*& pStreamSource);
            HX_RESULT	SetSoundLevel(UINT16 uTrackIndex, UINT16 uSoundLevel, HXBOOL bReflushAudioDevice);
            HX_RESULT	BeginSoundLevelAnimation(UINT16 uTrackIndex, UINT16 uSoundLevelBeginWith);	
            HX_RESULT	EndSoundLevelAnimation(UINT16 uTrackIndex, UINT16 uSoundLevelEndWith);
            HX_RESULT	OnSoundLevelAnimation(UINT16 uTrackIndex, UINT16 uSoundLevel, UINT32 ulSoundLevelAnimationTime);

            void	PersistentComponentAdded(UINT16 uTrackIndex);
            void        PersistentComponentRemoved(UINT16 uTrackIndex);

    virtual void        Close(void);
};

class HXAdvancedGroupManager : public HXBasicGroupManager			
{
protected:
    HXMasterTAC*    m_pMasterTAC;
    
    virtual	    ~HXAdvancedGroupManager();

    HX_RESULT	    RepeatTrackAdded(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pTrack);

    friend class HXAdvancedGroup;
    friend class HXPlayer;

public:
		        HXAdvancedGroupManager(HXPlayer* pPlayer = NULL);

    STDMETHOD(CreateGroup)      (REF(IHXGroup*) pGroup);
    STDMETHOD(AddGroup)	        (THIS_
			        IHXGroup*   /*IN*/ pGroup);
    STDMETHOD(RemoveGroup)      (THIS_
			        UINT16      /*IN*/ uGroupIndex);
    STDMETHOD(SetCurrentGroup)  (THIS_
                                UINT16      /*IN*/ uGroupIndex);

            void        SetMasterTAC(HXMasterTAC* pMasterTAC);

    virtual void        PersistentComponentAdded(UINT16 uGroupIndex, UINT16 uTrackIndex);
    virtual void        PersistentComponentRemoved(UINT16 uGroupIndex, UINT16 uTrackIndex);
    virtual void        Close(void);
};

#endif /* _ADVANCEDGROUP_H_ */
