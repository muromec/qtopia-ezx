/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmisus.h,v 1.5 2007/07/06 21:58:50 jfinnecy Exp $
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

#ifndef _HXMISUS_H_
#define _HXMISUS_H_

/****************************************************************************
 * 
 *  Class:
 *
 *	CHXMultiInstanceSiteUserSupplier
 *
 *  Purpose:
 *
 *	Implementation for IHXMultiInstanceSiteUserSupplier
 *
 */
class CHXMultiInstanceSiteUserSupplier : 
	public IHXSite,
	public IHXSite2,
        public IHXSiteEnumerator,
	public IHXSiteUserSupplier,
	public IHXMultiInstanceSiteUserSupplier,
	public IHXVideoSurface,
        public IHXVideoSurface2,
	public IHXInterruptSafe
{
private:
    LONG32		    m_lRefCount;
    IHXSiteUser*	    m_pSingleUser;
    CHXSimpleList	    m_SiteUsers;
    HXxSize		    m_size;
    HXxPoint		    m_position;
    INT32		    m_zorder;
    HX_BITFIELD		    m_bIsAttached : 1;
    HX_BITFIELD		    m_bSetSizeHasBeenCalled : 1;
    HX_BITFIELD		    m_bSetPositionHasBeenCalled : 1;
    HX_BITFIELD		    m_bSetZOrderHasBeenCalled : 1;
    HX_BITFIELD		    m_bIsInterrupSafe : 1;
    CHXSimpleList	    m_PassiveSiteWatchers;
    HXBitmapInfoHeader*    m_pSurfaceBitmapInfo;

    friend class CHXMultiInstanceSiteUser;

    ~CHXMultiInstanceSiteUserSupplier();

public:
    CHXMultiInstanceSiteUserSupplier();
    HXBitmapInfoHeader* GetBitmapInfoHeader() {return m_pSurfaceBitmapInfo;}
    
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXMultiInstanceSiteUserSupplier methods called by site users.
     */
    STDMETHOD(SetSingleSiteUser)    (THIS_ 
				    IUnknown*	pUnknown);

    STDMETHOD(ReleaseSingleSiteUser)(THIS);

    /*
     * IHXSiteUserSupplier methods usually called by the 
     * "context" to ask for additional or to release previously
     * created site users.
     */
    STDMETHOD(CreateSiteUser)	(THIS_
				REF(IHXSiteUser*)/*OUT*/ pSiteUser);

    STDMETHOD(DestroySiteUser)	(THIS_
				IHXSiteUser*	  /*IN*/ pSiteUser);

    STDMETHOD_(HXBOOL,NeedsWindowedSites)	(THIS);

    /*
     * IHXSite methods usually called by the "context" to 
     * associate users with the site, and to create child sites
     * as appropriate.
     */
    STDMETHOD(AttachUser)	(THIS_
				IHXSiteUser*	/*IN*/	pUser);

    STDMETHOD(DetachUser)	(THIS);


    STDMETHOD(GetUser)		(THIS_
				REF(IHXSiteUser*) /*OUT*/ pUser);

    STDMETHOD(CreateChild)	(THIS_
				REF(IHXSite*)	/*OUT*/ pChildSite);

    STDMETHOD(DestroyChild)	(THIS_
				IHXSite*	/*IN*/	pChildSite);


    /*
     * IHXSite methods called by the the "context" in which the site
     * is displayed in order to manage its position. Site users should
     * not generally call these methods.
     */
    STDMETHOD(AttachWatcher)	(THIS_
				IHXSiteWatcher* /*IN*/	pWatcher);

    STDMETHOD(DetachWatcher)	(THIS);

    STDMETHOD(SetPosition)	(THIS_
				HXxPoint		position);

    STDMETHOD(GetPosition)	(THIS_
				REF(HXxPoint)		position);

    /*
     * IHXSite methods called by the user of the site to get
     * information about the site, and to manipulate the site.
     */
    STDMETHOD(SetSize)		(THIS_
				HXxSize			size);

    STDMETHOD(GetSize)		(THIS_
				REF(HXxSize)		size);

    STDMETHOD(DamageRect)	(THIS_
				HXxRect			rect);

    STDMETHOD(DamageRegion)	(THIS_
				HXxRegion		region);

    STDMETHOD(ForceRedraw)	(THIS);

    /*
     * IHXSite2 methods called by the the "context" in which the site
     * is displayed in order to manage its position. Site users should
     * not generally call these methods.
     */
    STDMETHOD(UpdateSiteWindow)		(THIS_
					HXxWindow* /*IN*/   pWindow);

    STDMETHOD(ShowSite)			(THIS_
					HXBOOL		    bShow);
                                 
    STDMETHOD_(HXBOOL, IsSiteVisible)     (THIS);

    STDMETHOD(SetZOrder)		(THIS_
					INT32		    lZOrder);

    /*
     * IHXSite2 methods called by the user of the site to get
     * information about the site, and to manipulate the site.
     */
    STDMETHOD(GetZOrder)		(THIS_
					REF(INT32)	    lZOrder);

    STDMETHOD(MoveSiteToTop)		(THIS);

    STDMETHOD(GetVideoSurface)		(THIS_ 
					REF(IHXVideoSurface*) pSurface);
    STDMETHOD_(UINT32,GetNumberOfChildSites) (THIS);

    /*
     * IHXSite2 methods used to add/remove passive site watchers
     */
    STDMETHOD(AddPassiveSiteWatcher)	(THIS_
    					IHXPassiveSiteWatcher* pWatcher);

    STDMETHOD(RemovePassiveSiteWatcher)	(THIS_
    					IHXPassiveSiteWatcher* pWatcher);

    /*
     * IHXSite2 methods used to do cursor management
     */
    STDMETHOD(SetCursor)		(THIS_
    					HXxCursor cursor,
					REF(HXxCursor) oldCursor);

    /*
     * IHXSiteEnumerator methods 
     */
    STDMETHOD(GetFirstSite) (THIS_
                             REF(IHXSite*) /* OUT */ pFirstSite,
                             REF(IHXSiteEnumerator::SitePosition) /* OUT */ nextPosition);

    STDMETHOD(GetNextSite)  (THIS_
                             REF(IHXSite*) pNextSite,
                             REF(IHXSiteEnumerator::SitePosition) /* IN/OUT */ nextPosition);

    /*
     * IHXVideoSurface methods 
     */
    STDMETHOD(Blt)		(THIS_
				UCHAR*			/*IN*/	pImageBits, 
				HXBitmapInfoHeader*    /*IN*/	pBitmapInfo,			
				REF(HXxRect)		/*IN*/	rDestRect, 
				REF(HXxRect)		/*IN*/	rSrcRect) ;

    STDMETHOD(BeginOptimizedBlt)(THIS_ 
				HXBitmapInfoHeader*    /*IN*/	pBitmapInfo) ;

    STDMETHOD(OptimizedBlt)	(THIS_
				UCHAR*			/*IN*/	pImageBits,			
				REF(HXxRect)		/*IN*/	rDestRect, 
				REF(HXxRect)		/*IN*/	rSrcRect);
    
    STDMETHOD(EndOptimizedBlt)	(THIS);

    STDMETHOD(GetOptimizedFormat)(THIS_
				REF(HX_COMPRESSION_TYPE) /*OUT*/ ulType);

    STDMETHOD(GetPreferredFormat)(THIS_
				REF(HX_COMPRESSION_TYPE) /*OUT*/ ulType);

    /*
     * IHXVideoSurface2 methods 
     */
    STDMETHOD(SetProperties)    (THIS_ 
                                 HXBitmapInfoHeader *bmi, 
                                 REF(UINT32) ulNumBuffers, 
                                 IHXRenderTimeLine *pClock);

    STDMETHOD_(void, Flush)     (THIS);
    
    STDMETHOD(ReleaseVideoMem)  (THIS_ VideoMemStruct* pVidMem);
    
    STDMETHOD(ColorConvert)     (THIS_
                                 INT32 cidIn, 
                                 HXxSize *pSrcSize,
                                 HXxRect *prSrcRect,
                                 SourceInputStruct *pInput,
                                 INT32 cidOut,
                                 UCHAR *pDestBuffer, 
                                 HXxSize *pDestSize, 
                                 HXxRect *prDestRect, 
                                 int nDestPitch);
    
    STDMETHOD(GetVideoMem)      (THIS_
                                 VideoMemStruct* pVidMem,
                                 UINT32 ulFlags
                                 );
                                 
    STDMETHOD(Present)          (THIS_
                                 VideoMemStruct* pVidMem,
                                 INT32 lTime, 
                                 UINT32 ulFlags,
                                 HXxRect *prDestRect, 
                                 HXxRect *prSrcRect);
                                 
    STDMETHOD(PresentIfReady)   (THIS);

    /*
     *  IHXInterruptSafe methods
     */

    /************************************************************************
     *	Method:
     *	    IHXInterruptSafe::IsInterruptSafe
     *	Purpose:
     *	    This is the function that will be called to determine if
     *	    interrupt time execution is supported.
     */
    STDMETHOD_(HXBOOL,IsInterruptSafe)		(THIS);
};


/****************************************************************************
 * 
 *  Class:
 *
 *	CHXMultiInstanceSiteUser
 *
 */
class CHXMultiInstanceSiteUser : 
	public IHXSiteUser
{
private:
    LONG32				    m_lRefCount;
    CHXMultiInstanceSiteUserSupplier*	    m_pMISUS;
    IHXSite*				    m_pSite;
    IHXSite2*				    m_pSite2;

    friend class CHXMultiInstanceSiteUserSupplier;

    ~CHXMultiInstanceSiteUser();

public:
    CHXMultiInstanceSiteUser(CHXMultiInstanceSiteUserSupplier* pMISUS);
    
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXSiteUser methods usually called by the "context" to 
     * associate users with the site.
     */
    STDMETHOD(AttachSite)	(THIS_
				IHXSite*	/*IN*/ pSite);

    STDMETHOD(DetachSite)	(THIS);

    /*
     * IHXSiteUser methods called to inform user of an event.
     */
    STDMETHOD(HandleEvent)	(THIS_
				HXxEvent*	/*IN*/ pEvent);

    STDMETHOD_(HXBOOL,NeedsWindowedSites)	(THIS);
};

#endif // _HXMISUS_H_

