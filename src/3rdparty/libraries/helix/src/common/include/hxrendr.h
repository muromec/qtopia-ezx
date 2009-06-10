/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrendr.h,v 1.14 2008/08/20 21:05:49 ehyche Exp $
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

#ifndef _HXRENDR_H_
#define _HXRENDR_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXRenderer		    IHXRenderer;
typedef _INTERFACE	IHXStream		    IHXStream;
typedef _INTERFACE	IHXStreamSource	    IHXStreamSource;
typedef _INTERFACE	IHXPlayer		    IHXPlayer;
typedef _INTERFACE	IHXClientEngine	    IHXClientEngine;
// $Private:
typedef _INTERFACE	IHXPersistentRenderer	    IHXPersistentRenderer;
// $EndPrivate.
typedef _INTERFACE	IHXUntimedRenderer	    IHXUntimedRenderer;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRenderer
 *
 *  Purpose:
 *
 *	Interface implemented by all renderers. Parts of this interface are
 *	called by the client engine to provide data packets to the 
 *	individual renderers.
 *
 *  IID_IHXRenderer:
 *
 *	{00000300-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRenderer, 0x00000300, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRenderer

typedef ULONG32	HX_DISPLAY_TYPE;

#define HX_DISPLAY_NONE                    0x00000000
#define HX_DISPLAY_WINDOW                  0x00000001
#define HX_DISPLAY_SUPPORTS_RESIZE         0x00000002
#define HX_DISPLAY_SUPPORTS_FULLSCREEN     0x00000004
#define HX_DISPLAY_SUPPORTS_VIDCONTROLS    0x00000008


DECLARE_INTERFACE_(IHXRenderer, IUnknown)
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
     * IHXRenderer methods
     */

    /************************************************************************
     *	Method:
     *	    IHXRenderer::GetRendererInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of rendering 
     *	    plugins.
     */
    STDMETHOD(GetRendererInfo)	(THIS_
				REF(const char**)/*OUT*/ pStreamMimeTypes,
				REF(UINT32)	 /*OUT*/ unInitialGranularity
				) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::StartStream
     *	Purpose:
     *	    Called by client engine to inform the renderer of the stream it
     *	    will be rendering. The stream interface can provide access to
     *	    its source or player. This method also provides access to the 
     *	    primary client controller interface.
     *
     */
    STDMETHOD(StartStream)	(THIS_
				IHXStream*	    pStream,
				IHXPlayer*	    pPlayer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::EndStream
     *	Purpose:
     *	    Called by client engine to inform the renderer that the stream
     *	    is was rendering is closed.
     *
     */
    STDMETHOD(EndStream)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *		IHXRenderer::OnHeader
     *	Purpose:
     *		Called by client engine when a header for this renderer is 
     *		available. The header will arrive before any packets.
     *
     */
    STDMETHOD(OnHeader)		(THIS_
				IHXValues*	    pHeader) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnPacket
     *	Purpose:
     *	    Called by client engine when a packet for this renderer is 
     *	    due.
     *
     */
    STDMETHOD(OnPacket)		(THIS_
				IHXPacket*	    pPacket,
				LONG32		    lTimeOffset) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnTimeSync
     *	Purpose:
     *	    Called by client engine to inform the renderer of the current
     *	    time relative to the streams synchronized time-line. The 
     *	    renderer should use this time value to update its display or
     *	    render it's stream data accordingly.
     *
     */
    STDMETHOD(OnTimeSync)	(THIS_
				ULONG32		    ulTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnPreSeek
     *	Purpose:
     *	    Called by client engine to inform the renderer that a seek is
     *	    about to occur. The render is informed the last time for the 
     *	    stream's time line before the seek, as well as the first new
     *	    time for the stream's time line after the seek will be completed.
     *
     */
    STDMETHOD(OnPreSeek)	(THIS_
				ULONG32		    ulOldTime,
				ULONG32		    ulNewTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnPostSeek
     *	Purpose:
     *	    Called by client engine to inform the renderer that a seek has
     *	    just occurred. The render is informed the last time for the 
     *	    stream's time line before the seek, as well as the first new
     *	    time for the stream's time line after the seek.
     *
     */
    STDMETHOD(OnPostSeek)	(THIS_
				ULONG32		    ulOldTime,
				ULONG32		    ulNewTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnPause
     *	Purpose:
     *	    Called by client engine to inform the renderer that a pause has
     *	    just occurred. The render is informed the last time for the 
     *	    stream's time line before the pause.
     *
     */
    STDMETHOD(OnPause)		(THIS_
				ULONG32		    ulTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnBegin
     *	Purpose:
     *	    Called by client engine to inform the renderer that a begin or
     *	    resume has just occurred. The render is informed the first time 
     *	    for the stream's time line after the resume.
     *
     */
    STDMETHOD(OnBegin)		(THIS_
				ULONG32		    ulTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnBuffering
     *	Purpose:
     *	    Called by client engine to inform the renderer that buffering
     *	    of data is occuring. The render is informed of the reason for
     *	    the buffering (start-up of stream, seek has occurred, network
     *	    congestion, etc.), as well as percentage complete of the 
     *	    buffering process.
     *
     */
    STDMETHOD(OnBuffering)	(THIS_
				ULONG32		    ulFlags,
				UINT16		    unPercentComplete) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::GetDisplayType
     *	Purpose:
     *	    Called by client engine to ask the renderer for it's preferred
     *	    display type. When layout information is not present, the 
     *	    renderer will be asked for it's prefered display type. Depending
     *	    on the display type a buffer of additional information may be 
     *	    needed. This buffer could contain information about preferred
     *	    window size.
     *
     */
    STDMETHOD(GetDisplayType)	(THIS_
				REF(HX_DISPLAY_TYPE)	ulFlags,
				REF(IHXBuffer*)	pBuffer) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRenderer::OnEndofPackets
     *	Purpose:
     *	    Called by client engine to inform the renderer that all the
     *	    packets have been delivered. However, if the user seeks before
     *	    EndStream() is called, renderer may start getting packets again
     *	    and the client engine will eventually call this function again.
     */
    STDMETHOD(OnEndofPackets)	(THIS) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPersistentRenderer
 * 
 *  Purpose:
 * 
 *	Interface exposed by a persistent renderer such as RAM and SMIL
 * 
 *  IID_IHXPersistentRenderer:
 * 
 *	{00000301-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPersistentRenderer, 0x00000301, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPersistentRenderer

DECLARE_INTERFACE_(IHXPersistentRenderer, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXPersistentRenderer methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPersistentRenderer::InitPersistent
     *	Purpose:
     *	    Initialize the persistent renderer
     */
    STDMETHOD(InitPersistent)		(THIS_
					UINT32			ulPersistentComponentID,
					UINT16			uPersistentGroupID,
					UINT16			uPersistentTrackID,
					IHXPersistentRenderer*	pPersistentParent) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentRenderer::GetPersistentID
     *	Purpose:
     *	    Get persistent component ID
     */
    STDMETHOD(GetPersistentID)		(THIS_
					REF(UINT32) ulPersistentComponentID) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentRenderer::GetPersistentProperties
     *	Purpose:
     *	    get properties of the persistent renderer such as the type and version#
     */
    STDMETHOD(GetPersistentProperties)	(THIS_
                               		REF(IHXValues*)    pProperties) PURE;				

    /************************************************************************
     *	Method:
     *	    IHXPersistentRenderer::GetElementProperties
     *	Purpose:
     *	    get properties of the element(track) spawned by this persistent renderer
     */
    STDMETHOD(GetElementProperties)	(THIS_
					UINT16		    uGroupID,
					UINT16		    uTrackID,
                               		REF(IHXValues*)    pProperties) PURE;				

    /************************************************************************
    *  Method:
    *      IHXPersistentRenderer::AttachElementLayout
    *  Purpose:
    *	   attach the site created in its persitent parent as the root layout 
    */
    STDMETHOD(AttachElementLayout)	(THIS_
					UINT16		    uGroupID,
					UINT16		    uTrackID,
					IHXRenderer*	    pRenderer,
					IHXStream*	    pStream,
					IHXValues*	    pProps) PURE;

    /************************************************************************
    *  Method:
    *      IHXPersistentRenderer::DetachElementLayout
    *  Purpose:
    *	   Detach the root layout created by its persistent parent
    */
    STDMETHOD(DetachElementLayout)	(THIS_
					IUnknown*	    pLSG) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPersistentRenderer::GetElementShowStatus
     *	Purpose:
     *	    get element's status(such as its layout's show property) 
     *      at current time
     */
    STDMETHOD(GetElementStatus)		(THIS_
					UINT16		    uGroupID,
					UINT16		    uTrackID,
					UINT32		    ulCurrentTime,
					REF(IHXValues*)    pStatus) PURE;
};
// $EndPrivate.

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXMediaPushdown
 * 
 *  Purpose:
 * 
 *	Al's weird hack interface
 * 
 *  IHXMediaPushdown:
 * 
 *	{00000302-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMediaPushdown, 0x00000302, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXMediaPushdown

DECLARE_INTERFACE_(IHXMediaPushdown, IUnknown)
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
     * IHXMediaPushdown methods
     */
    /************************************************************************
     *  Method:
     *      IHXMediaPushdown::GetCurrentPushdown
     *  Purpose:
     *      Retrieves the current queue depth ("pushdown depth") in milliseconds
     *
     *  Notes:
     *      This is the *decoded* pushdown, not the undecoded pushdown.  Returns
     *      HXR_TIMELINE_SUSPENDED if the stream is paused; HXR_FAIL
     *      if the stream is finished, HXR_OK otherwise.
     */
    STDMETHOD (GetCurrentPushdown) (THIS_ /*OUT*/ 
				    REF(UINT32) ulPushdownMS,
				    REF(UINT32) ulNumFrames) PURE;

    /************************************************************************
     *  Method:
     *      IHXMediaPushdown::IsG2Video
     *  Purpose:
     */
    STDMETHOD_(HXBOOL, IsG2Video) (THIS) PURE;
};
// $EndPrivate.

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXUntimedRenderer
 * 
 *  Purpose:
 * 
 *	Interface exposed by a renderer capable of running faster than real time
 * 
 *  IID_IHXUntimedRenderer:
 * 
 *	{00000303-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXUntimedRenderer, 0x00000303, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXUntimedRenderer

DECLARE_INTERFACE_(IHXUntimedRenderer, IUnknown)
{
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    STDMETHOD_(HXBOOL,IsUntimedRendering)(THIS) PURE;
    STDMETHOD_(HX_RESULT,SetUntimedRendering)(THIS_ HXBOOL) PURE;
};



/*
 * IHXFrameInfo. Used by the core to determine of a visual renderer
 * is still waiting to display the first frame after playback
 * begins or after a seek event. Current used by the QuickSeek 
 * interface to find out just 
 */
DEFINE_GUID(IID_IHXFrameInfo, 0x248e4030, 0x2cf1, 0x4b97, 0x82, 0xe1, 0xab,
            0x7b, 0x8f, 0xa5, 0xa, 0xee);


#undef  INTERFACE
#define INTERFACE   IHXFrameInfo
DECLARE_INTERFACE_(IHXFrameInfo, IUnknown)
{
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;
    STDMETHOD_(HXBOOL,FirstFrameDisplayed)(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXRendererDrivenPacketDelivery
 * 
 *  Purpose:
 * 
 *      Interface exposed by a renderer which wants to control the
 *      packet delivery to it.
 * 
 *  IID_IHXRendererDrivenPacketDelivery:
 * 
 *      {3F868F1E-8AF0-4294-B87F-A5BA09041776}
 * 
 */
DEFINE_GUID(IID_IHXRendererDrivenPacketDelivery, 0x3f868f1e, 0x8af0, 0x4294,
            0xb8, 0x7f, 0xa5, 0xba, 0x9, 0x4, 0x17, 0x76);


#undef  INTERFACE
#define INTERFACE IHXRendererDrivenPacketDelivery

DECLARE_INTERFACE_(IHXRendererDrivenPacketDelivery, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXRendererDrivenPacketDelivery methods
    STDMETHOD_(HXBOOL,IsBufferingEnabled)      (THIS) PURE;
    STDMETHOD_(HXBOOL,IsPacketDeliveryEnabled) (THIS) PURE;
    STDMETHOD_(HXBOOL,CanAcceptPackets)        (THIS) PURE;
};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXRenderer)
DEFINE_SMART_PTR(IHXPersistentRenderer)
DEFINE_SMART_PTR(IHXMediaPushdown)
DEFINE_SMART_PTR(IHXUntimedRenderer)
DEFINE_SMART_PTR(IHXFrameInfo)
DEFINE_SMART_PTR(IHXRendererDrivenPacketDelivery)

#endif /* _HXRENDR_H_ */
