/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ramrendr.h,v 1.7 2007/07/06 22:01:31 jfinnecy Exp $
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

#ifndef _RAMRENDR_H_
#define _RAMRENDR_H_

struct RAMPlayToAssoc
{
    UINT16	m_uGroupIndex;
    UINT16	m_uTrackIndex;
    UINT32	m_ulDelay;
    UINT32	m_ulDuration;
    CHXString	m_id;
};

class CRAMRenderer : public IHXPlugin,
		     public IHXRenderer
#if defined(HELIX_FEATURE_NESTEDMETA)
		     , public IHXPersistentRenderer
		     , public IHXGroupSink
		     , public IHXRendererAdviseSink
#endif /* HELIX_FEATURE_NESTEDMETA */
                     , public IHXPlaybackVelocity
{
public:
    CRAMRenderer();
    virtual ~CRAMRenderer();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXPlugin methods
    STDMETHOD(GetPluginInfo) (THIS_ REF(HXBOOL)        bLoadMultiple,
                                    REF(const char*) pDescription,
                                    REF(const char*) pCopyright,
                                    REF(const char*) pMoreInfoURL,
                                    REF(ULONG32)     ulVersionNumber);
    STDMETHOD(InitPlugin)    (THIS_ IUnknown* pContext);

    // IHXRenderer methods
    STDMETHOD(GetRendererInfo) (THIS_ REF(const char**) pStreamMimeTypes,
                                      REF(UINT32)       unInitialGranularity);
    STDMETHOD(StartStream)     (THIS_ IHXStream* pStream, IHXPlayer* pPlayer);
    STDMETHOD(EndStream)       (THIS);
    STDMETHOD(OnHeader)        (THIS_ IHXValues* pHeader);
    STDMETHOD(OnPacket)        (THIS_ IHXPacket* pPacket, LONG32 lTimeOffset);
    STDMETHOD(OnTimeSync)      (THIS_ ULONG32 ulTime);
    STDMETHOD(OnPreSeek)       (THIS_ ULONG32 ulOldTime, ULONG32 ulNewTime);
    STDMETHOD(OnPostSeek)      (THIS_ ULONG32 ulOldTime, ULONG32 ulNewTime);
    STDMETHOD(OnPause)         (THIS_ ULONG32 ulTime);
    STDMETHOD(OnBegin)         (THIS_ ULONG32 ulTime);
    STDMETHOD(OnBuffering)     (THIS_ ULONG32 ulFlags, UINT16 unPercentComplete);
    STDMETHOD(GetDisplayType)  (THIS_ REF(HX_DISPLAY_TYPE) ulFlags,
                                      REF(IHXBuffer*)      pBuffer);
    STDMETHOD(OnEndofPackets)  (THIS);

#if defined(HELIX_FEATURE_NESTEDMETA)
    // IHXGroupSink methods
    STDMETHOD(GroupAdded)       (THIS_ UINT16 uGroupIndex, IHXGroup* pGroup);
    STDMETHOD(GroupRemoved)     (THIS_ UINT16 uGroupIndex, IHXGroup* pGroup);
    STDMETHOD(AllGroupsRemoved) (THIS);
    STDMETHOD(TrackAdded)       (THIS_ UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pTrack);
    STDMETHOD(TrackRemoved)     (THIS_ UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pTrack);
    STDMETHOD(TrackStarted)     (THIS_ UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pTrack);
    STDMETHOD(TrackStopped)     (THIS_ UINT16 uGroupIndex, UINT16 uTrackIndex, IHXValues* pTrack);
    STDMETHOD(CurrentGroupSet)  (THIS_ UINT16 uGroupIndex, IHXGroup* pGroup);

    // IHXRendererAdviseSink methods
    STDMETHOD(TrackDurationSet)         (THIS_ UINT32 ulGroupIndex, UINT32 ulTrackIndex,
                                               UINT32 ulDuration,   UINT32 ulDelay,
                                               HXBOOL   bIsLive);
    STDMETHOD(RepeatedTrackDurationSet) (THIS_ const char* pID, UINT32 ulDuration,
                                               HXBOOL bIsLive);
    STDMETHOD(TrackUpdated)             (THIS_ UINT32 ulGroupIndex, UINT32 ulTrackIndex,
                                               IHXValues* pValues);
    STDMETHOD(RendererInitialized)      (THIS_ IHXRenderer* pRend, IUnknown* pStream,
                                               IHXValues* pInfo);
    STDMETHOD(RendererClosed)           (THIS_ IHXRenderer* pRend, IHXValues* pInfo);

    // IHXPersistentRenderer methods
    STDMETHOD(InitPersistent)		(THIS_
					UINT32			ulPersistentComponentID,
					UINT16			uPersistentGroupID,
					UINT16			uPersistentTrackID,
					IHXPersistentRenderer*	pPersistentParent);

    STDMETHOD(GetPersistentID)		(THIS_
					REF(UINT32) ulPersistentComponentID);

    STDMETHOD(GetPersistentProperties)	(THIS_
                               		REF(IHXValues*)    pProperties);				

    STDMETHOD(GetElementProperties)	(THIS_
					UINT16		    uGroupID,
					UINT16		    uTrackID,
                               		REF(IHXValues*)    pProperties);

    STDMETHOD(AttachElementLayout)	(THIS_
					UINT16		    uGroupID,
					UINT16		    uTrackID,
					IHXRenderer*	    pRenderer,
					IHXStream*	    pStream,
					IHXValues*	    pProps);

    STDMETHOD(DetachElementLayout)	(THIS_
					IUnknown*	    pLSG);

    STDMETHOD(GetElementStatus)		(THIS_
					UINT16		    uGroupID,
					UINT16		    uTrackID,
					UINT32		    ulCurrentTime,
					REF(IHXValues*)    pStatus);
#endif /* HELIX_FEATURE_NESTEDMETA */

    // IHXPlaybackVelocity methods
    STDMETHOD(InitVelocityControl)         (THIS_ IHXPlaybackVelocityResponse* pResponse) { return HXR_OK; }
    STDMETHOD(QueryVelocityCaps)           (THIS_ REF(IHXPlaybackVelocityCaps*) rpCaps) { return HXR_NOTIMPL; }
    STDMETHOD(SetVelocity)                 (THIS_ INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch) { return HXR_OK; }
    STDMETHOD_(INT32,GetVelocity)          (THIS) { return 0; }
    STDMETHOD(SetKeyFrameMode)             (THIS_ HXBOOL bKeyFrameMode) { return HXR_OK; }
    STDMETHOD_(HXBOOL,GetKeyFrameMode)     (THIS) { return FALSE; }
    STDMETHOD_(UINT32,GetKeyFrameInterval) (THIS) { return 0; }
    STDMETHOD(CloseVelocityControl)        (THIS) { return HXR_OK; }
protected:

    LONG32                  m_lRefCount;
    HXBOOL		    m_bRAMProcessed;
    HXBOOL		    m_bFirstTrack;
    char*                   m_pURLFragment;
    UINT32		    m_ulGroupIndex;
    CHXString               m_urlPrefix;
    CHXString               m_urlRoot;
    IUnknown*               m_pContext;
    IHXStream*             m_pStream;
    IHXPlayer*             m_pPlayer;

    UINT32		    m_ulPersistentComponentDelay;
    UINT32		    m_ulPersistentComponentDuration;
    UINT32		    m_ulPersistentComponentID;
    UINT16		    m_uPersistentGroupID;
    UINT16		    m_uPersistentTrackID;
    UINT16		    m_uGroupIndexWithin;
    UINT32		    m_ulPersistentVersion;
    PersistentType	    m_persistentType;
    ElementWithinTag	    m_elementWithinTag;
    CHXMapLongToObj*	    m_pTrackMap;
    CHXSimpleList*	    m_pPlayToAssocList;
    IHXValues*		    m_pPersistentProperties;
    IHXValues*		    m_pStreamProperties;
    IHXPersistentRenderer* m_pPersistentParentRenderer;
    IHXPersistentComponentManager* m_pPersistentComponentManager;

    IHXBuffer*              m_pOriginalURL;

    static const char*      zm_pName;
    static const char*      zm_pDescription;
    static const char*      zm_pCopyright;
    static const char*      zm_pMoreInfoURL;
    static const char*      zm_pStreamMimeTypes[];

    HX_RESULT		    ProcessRAM(IHXBuffer* pBuffer);
    virtual HX_RESULT       ProcessURL(const char* pURL) { return HXR_NOTIMPL; }
    HX_RESULT		    PrepareGroup(REF(IHXValues*) pGroupProperties);
    HX_RESULT		    PrepareTrack(char* pszURL, REF(IHXValues*) pTrackProperties);
    HX_RESULT		    ConvertURL(const char* pURL, CHXString& newURL);    
    ElementWithinTag	    AdjustElementWithinTag(ElementWithinTag elementWithinTag);
    HXBOOL		    IsNestedMetaSupported(void);
    void                    GeneratePreFix(void);

    RAMPlayToAssoc*	    GetPlayToAssoc(UINT16 uGroupIndex, UINT16 uTrackIndex);
    RAMPlayToAssoc*	    GetPlayToAssocByMedia(const char* pszMediaID);

    void		    RemoveTracks();
    void		    RemoveAllPlayToAssoc();    

    void		    Cleanup(void);
};

#endif // _RAMRENDR_H_

