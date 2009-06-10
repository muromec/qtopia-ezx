/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: perscmgr.h,v 1.6 2007/07/06 21:58:11 jfinnecy Exp $
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

#ifndef _PERSISTENTCOMPONENTMANAGER_
#define _PERSISTENTCOMPONENTMANAGER_

class	HXPlayer;
class	SourceInfo;
class	HXPersistentComponent;
class	HXPersistentComponentManager;

class HXPersistentComponent : public IHXPersistentComponent
{
protected:
    LONG32			    m_lRefCount;
    HXBOOL			    m_bInitialized;
    HXBOOL			    m_bToBeClosed;
    HXBOOL			    m_bCleanupLayoutCalled;
    UINT16			    m_uGroups;
    UINT16			    m_uTracks;
    UINT32			    m_ulComponentID;
    UINT32			    m_ulPersistentType;
    SourceInfo*			    m_pSourceInfo;
    HXPersistentComponent*	    m_pPersistentParent;
    CHXSimpleList*		    m_pPersistentChildList;		    
    IHXValues*			    m_pProperties;
    IHXPersistentRenderer*	    m_pPersistentRenderer;
    IHXRendererAdviseSink*	    m_pRendererAdviseSink;
    IHXGroupSink*		    m_pGroupSink;
    HXPersistentComponentManager*  m_pComponentManager;

    virtual	    ~HXPersistentComponent();

    friend class HXPlayer;
    friend class SourceInfo;
    friend class HXSource;
    friend class HXNetSource;
    friend class HXAdvancedGroupManager;
    friend class HXPersistentComponentManager;

public:

    HXPersistentComponent(HXPersistentComponentManager* pManager);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::Init
     *	Purpose:
     *	    initialize persistent component
     */
    STDMETHOD(Init)			(THIS_
                               		IHXPersistentRenderer* pPersistentRenderer);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::AddRendererAdviseSink
     *	Purpose:
     *	    add renderer advise sink
     */
    STDMETHOD(AddRendererAdviseSink)	(THIS_
                               		IHXRendererAdviseSink* pSink);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::RemoveRendererAdviseSink
     *	Purpose:
     *	    remove renderer advise sink
     */
    STDMETHOD(RemoveRendererAdviseSink)	(THIS_
                               		IHXRendererAdviseSink* pSink);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::AddGroupSink
     *	Purpose:
     *	    add renderer advise sink
     */
    STDMETHOD(AddGroupSink)		(THIS_
                               		IHXGroupSink* pSink);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::RemoveGroupSink
     *	Purpose:
     *	    remove renderer advise sink
     */
    STDMETHOD(RemoveGroupSink)		(THIS_
                               		IHXGroupSink* pSink);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::GetPersistentRenderer
     *	Purpose:
     *	    get persistent renderer
     */
    STDMETHOD(GetPersistentRenderer)	(THIS_
                               		REF(IHXPersistentRenderer*) pPersistentRenderer);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponent::GetPersistentProperties
     *	Purpose:
     *	    get persistent component properties
     */
    STDMETHOD(GetPersistentProperties)	(THIS_
                               		REF(IHXValues*) pProperties);

    HX_RESULT	GetPersistentComponent	(UINT32	ulComponentID, REF(IHXPersistentComponent*) pComponent);
    HX_RESULT	CurrentGroupSet		(UINT16 uGroupIndex, IHXGroup* pGroup);
    HX_RESULT	OnTimeSync		(UINT32 ulCurrentTime);
    UINT32	GetPersistentComponentCount(void);
    void	TrackUpdated		(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues);
    void	AllRenderersClosed	(void);
    void	Reset			(void);
    void	Remove			(void);
};

class HXPersistentComponentManager : public IHXPersistentComponentManager,
				      public IHXGroupSink
{
protected:
    LONG32		    m_lRefCount;
    UINT32		    m_ulComponentIndex;
    INT32		    m_nCurrentGroup;
    HXPlayer*		    m_pPlayer;    
    HXPersistentComponent* m_pRootPersistentComponent;

    virtual	    ~HXPersistentComponentManager();

    friend class HXPlayer;
    friend class HXPersistentComponent;

public:

    HXPersistentComponentManager(HXPlayer* pPlayer);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)      (THIS);

    STDMETHOD_(ULONG32,Release)     (THIS);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::CreatePersistentComponent
     *	Purpose:
     *	    create persistent component
     */
    STDMETHOD(CreatePersistentComponent)    (THIS_
					    REF(IHXPersistentComponent*)   pPersistentComponent);


    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::AddPersistentComponent
     *	Purpose:
     *	    add persistent component
     */
    STDMETHOD(AddPersistentComponent)	    (THIS_
					    IHXPersistentComponent*	pPersistentComponent);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::RemovePersistentComponent
     *	Purpose:
     *	    remove persistent component
     */
    STDMETHOD(RemovePersistentComponent)    (THIS_
					    UINT32 ulPersistentComponentID);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::GetPersistentComponent
     *	Purpose:
     *	    get persistent component information
     */
    STDMETHOD(GetPersistentComponent)	    (THIS_
					    UINT32			    ulPersistentComponentID,
					    REF(IHXPersistentComponent*)   pPersistentComponent);

    /************************************************************************
     *	Method:
     *	    IHXPersistentComponentManager::AttachPersistentComponentLayout
     *	Purpose:
     *	    get persistent component information
     */
    STDMETHOD(AttachPersistentComponentLayout)	(IUnknown*  pLSG,
						IHXValues* pProps);	
    /*
     *  IHXGroupSink methods
     */
    /************************************************************************
    *  Method:
    *      IHXGroupSink::GroupAdded
    *  Purpose:
    *		Notification of a new group being added to the presentation.
    */
    STDMETHOD(GroupAdded)		    (THIS_
					    UINT16	/*IN*/ uGroupIndex,
					    IHXGroup*	/*IN*/ pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::GroupRemoved
    *  Purpose:
    *		Notification of a group being removed from the presentation.
    */
    STDMETHOD(GroupRemoved)		    (THIS_
					    UINT16	/*IN*/ uGroupIndex,
					    IHXGroup*  /*IN*/ pGroup);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::AllGroupsRemoved
    *  Purpose:
    *		Notification that all groups have been removed from the 
    *		current presentation.
    */
    STDMETHOD(AllGroupsRemoved)		    (THIS);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackAdded
    *  Purpose:
    *		Notification of a new track being added to a group.
    */
    STDMETHOD(TrackAdded)		    (THIS_
					    UINT16 	    /*IN*/ uGroupIndex,
					    UINT16 	    /*IN*/ uTrackIndex,
					    IHXValues*	    /*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackRemoved
    *  Purpose:
    *		Notification of a track being removed from a group.
    */
    STDMETHOD(TrackRemoved)		    (THIS_
					    UINT16	    /*IN*/ uGroupIndex,
					    UINT16 	    /*IN*/ uTrackIndex,
					    IHXValues*	    /*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackStarted
    *  Purpose:
    *		Notification of a track being started (to get duration, for
    *		instance...)
    */
    STDMETHOD(TrackStarted)		    (THIS_
					    UINT16	    /*IN*/ uGroupIndex,
					    UINT16	    /*IN*/ uTrackIndex,
					    IHXValues*	    /*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::TrackStopped
    *  Purpose:
    *		Notification of a track being stopped
    *
    */
    STDMETHOD(TrackStopped)		    (THIS_
					    UINT16	    /*IN*/ uGroupIndex,
					    UINT16	    /*IN*/ uTrackIndex,
					    IHXValues*	    /*IN*/ pTrack);

    /************************************************************************
    *  Method:
    *      IHXGroupSink::CurrentGroupSet
    *  Purpose:
    *		This group is being currently played in the presentation.
    */
    STDMETHOD(CurrentGroupSet)		    (THIS_
					    UINT16 	    /*IN*/ uGroupIndex,
					    IHXGroup*	    /*IN*/ pGroup);

    HX_RESULT		OnTimeSync(ULONG32 ulCurrentTime);
    HXBOOL		IsCleanupLayoutNeeded(INT32 nCurrentGroup, INT32 nGroupSwitchTo);
    UINT32		GetPersistentComponentCount(void);
    void		TrackUpdated(UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pValues);
    void		CloseAllRenderers(INT32 nGroupSwitchTo);
    void		Reset();
    void		Close();
};
#endif /*_PERSISTENTCOMPONENTMANAGER_*/
