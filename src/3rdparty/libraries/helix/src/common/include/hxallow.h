/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxallow.h,v 1.6 2005/03/14 19:27:09 bobclark Exp $
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

#ifndef _HXALLOW_H_
#define _HXALLOW_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IHXValues			    IHXValues;
typedef _INTERFACE  IHXBuffer			    IHXBuffer;
typedef _INTERFACE  IHXRequest			    IHXRequest;

typedef _INTERFACE  IHXPlayerConnectionAdviseSinkManager    IHXPlayerConnectionAdviseSinkManager;
typedef _INTERFACE  IHXPlayerConnectionAdviseEvents         IHXPlayerConnectionAdviseEvents;
typedef _INTERFACE  IHXPlayerConnectionAdviseSink           IHXPlayerConnectionAdviseSink;
typedef _INTERFACE  IHXPlayerConnectionResponse             IHXPlayerConnectionResponse;
typedef _INTERFACE  IHXPlayerController                     IHXPlayerController;
typedef _INTERFACE  IHXPlayerControllerProxyRedirect        IHXPlayerControllerProxyRedirect;
typedef _INTERFACE  IHXMidBoxNotify                         IHXMidBoxNotify;

// IHXPlayerConnectionAdviseEvents file types
#define ADVISE_FILE_LIVE    0x0001
#define ADVISE_FILE_ALL     0xffff

// IHXPlayerConnectionAdviseEvents event types
#define ADVISE_EVENT_NONE       0x0000
#define ADVISE_EVENT_ONURL      0x0001
#define ADVISE_EVENT_ONBEGIN    0x0002
#define ADVISE_EVENT_ONSTOP     0x0004
#define ADVISE_EVENT_ONPAUSE    0x0008
#define ADVISE_EVENT_ONDONE     0x0010
#define ADVISE_EVENT_ALL        0xffff

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPlayerConnectionAdviseEvents
 * 
 *  Purpose:
 *
 *      Provide negotiation between file type and interesting events.
 * 
 *  IID_IHXPlayerConnectionAdviseEvents:
 * 
 *      {8fe78da6-a828-11d7-939c-00601df0ce4c}
 * 
 */

DEFINE_GUID(IID_IHXPlayerConnectionAdviseEvents, 0x8fe78da6, 0xa828, 0x11d7, 
	    0x93, 0x9c, 0x0, 0x60, 0x1d, 0xf0, 0xce, 0x4c);

#undef  INTERFACE
#define INTERFACE IHXPlayerConnectionAdviseEvents

DECLARE_INTERFACE_(IHXPlayerConnectionAdviseEvents, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    STDMETHOD_(UINT32,EventSelect)      (THIS_ UINT32 uFileType) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPlayerConnectionAdviseSink
 * 
 *  Purpose:
 *
 *      Advise Sink which receives notification whenever a new player 
 *      connects to the server.
 * 
 *  IID_IHXPlayerConnectionAdviseSink:
 * 
 *      {00002600-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPlayerConnectionAdviseSink, 0x00002600, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXPlayerConnectionAdviseSink

DECLARE_INTERFACE_(IHXPlayerConnectionAdviseSink, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /* OnConnection is called when a new player has connected to the
     * server.  If the result is HXR_OK, then the plugin will be notified
     * when certain events occur in the player's life cycle.
     */
    STDMETHOD(OnConnection) (THIS_ 
				IHXPlayerConnectionResponse* pResponse) PURE;

    /* SetPlayerController is called by the server core to provide us with
     * an interface which can stop, alert, redirect or otherwise control
     * the player we are receiving notifications about.
     */
    STDMETHOD(SetPlayerController)  (THIS_
				IHXPlayerController* pPlayerController) PURE;

    /* SetRegistryID is called by the server core to provide us with the
     * ID for this Player in the server registry. The plugin can use this
     * registry ID to find out various information about the connected player.
     */
    STDMETHOD(SetRegistryID)        (THIS_ UINT32 ulPlayerRegistryID) PURE;

    STDMETHOD(OnURL)                (THIS_ IHXRequest* pRequest) PURE;
    STDMETHOD(OnBegin)              (THIS) PURE;
    STDMETHOD(OnStop)               (THIS) PURE;
    STDMETHOD(OnPause)              (THIS) PURE;
    STDMETHOD(OnDone)               (THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPlayerConnectionResponse
 * 
 *  Purpose:
 *
 *      Response object for the PlayerConnectionAdviseSink.
 * 
 *  IID_IHXPlayerConnectionResponse:
 * 
 *      {00002601-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPlayerConnectionResponse, 0x00002601, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXPlayerConnectionResponse

DECLARE_INTERFACE_(IHXPlayerConnectionResponse, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    STDMETHOD(OnConnectionDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnURLDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnBeginDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnStopDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnPauseDone)		(THIS_ HX_RESULT status) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPlayerController
 * 
 *  Purpose:
 *
 *      Object created by the server core and given to the 
 *      IHXPlayerConnectionResponse object so that the response object
 *      can control the connected player.
 * 
 *  IID_IHXPlayerController:
 * 
 *      {00002602-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPlayerController, 0x00002602, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXPlayerController

DECLARE_INTERFACE_(IHXPlayerController, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    STDMETHOD(Pause)		        (THIS) PURE;
    STDMETHOD(Resume)                   (THIS) PURE;
    STDMETHOD(Disconnect)               (THIS) PURE;
    STDMETHOD(AlertAndDisconnect)       (THIS_ IHXBuffer* pAlert) PURE;

    /* HostRedirect is called by a PlayerConnectionAdviseSink to redirect
     * this player to another host and/or port, for the same URL. This
     * method works with both RTSP and PNA protocols.
     */
    STDMETHOD(HostRedirect)             (THIS_ IHXBuffer* pHost, 
					UINT16 nPort) PURE;

    /* NetworkRedirect is called by a PlayerConnectionAdviseSink to redirect
     * this player to another URL. Note: This method is only available for
     * redirecting an RTSP player connection to another RTSP URL.
     */
    STDMETHOD(NetworkRedirect)          (THIS_ IHXBuffer* pURL,
					UINT32 ulSecsFromNow) PURE;

    /* Redirect is called by a PlayerConnectionAdviseSink to redirect
     * this player to another URL on the same server. For example, if 
     * pPartialURL were set to "welcome.rm", the player would be redirected
     * to "current_protocol://current_host:current_port/welcome.rm". This
     * method works with both RTSP and PNA protocols.
     */
    STDMETHOD(Redirect)			(THIS_ IHXBuffer* pPartialURL) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPlayerConnectionAdviseSinkManager
 * 
 *  Purpose:
 *
 *      Manages the creation of IHXPlayerConnectionAdviseSink objects
 * 
 *  IID_IHXPlayerConnectionAdviseSinkManager:
 * 
 *      {00002603-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPlayerConnectionAdviseSinkManager, 0x00002603, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXPlayerConnectionAdviseSinkManager

DECLARE_INTERFACE_(IHXPlayerConnectionAdviseSinkManager, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    STDMETHOD(CreatePlayerConnectionAdviseSink)
		(THIS_
		REF(IHXPlayerConnectionAdviseSink*) pPCAdviseSink) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXPlayerControllerProxyRedirect
 * 
 *  Purpose:
 *
 *      QueryInterfaced from IHXPlayerController.  Allows 305 proxy redirect
 *      to be issued (as per RTSP spec).
 * 
 *  IID_IHXPlayerControllerProxyRedirect:
 * 
 *      {00002607-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPlayerControllerProxyRedirect, 0x00002607, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXPlayerControllerProxyRedirect

DECLARE_INTERFACE_(IHXPlayerControllerProxyRedirect, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /*
     * This URL is just a hostname / port.  It must be formatted like this:
     * "rtsp://audio.real.com:554/".
     *
     * NOTE:  You can *only* call this method between OnURL() and OnURLDone().
     * NOTE:  This method only works on RTSP connections.
     */
    STDMETHOD(NetworkProxyRedirect)          (THIS_ IHXBuffer* pURL) PURE;
};

//$ Private:

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXMidBoxNotify
 * 
 *  Purpose:
 *
 *      Notify allowance plugins that the client is or is not a "midbox".
 *      A midbox is a device between the player and server, such as a
 *      proxy or splitter.
 * 
 *  IID_IHXMidBoxNotify:
 *
 *      {f8c5dcaf-9a5f-4d1b-a061-22fa0d038848}
 * 
 */

DEFINE_GUID(IID_IHXMidBoxNotify, 0xf8c5dcaf, 0x9a5f, 0x4d1b,
	    0xa0, 0x61, 0x22, 0xfa, 0x0d, 0x03, 0x88, 0x48);

#undef  INTERFACE
#define INTERFACE IHXMidBoxNotify

DECLARE_INTERFACE_(IHXMidBoxNotify, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    STDMETHOD(SetMidBox)                (THIS_ HXBOOL bIsMidBox) PURE;
};

// $EndPrivate.

#endif /* _HXALLOW_H_ */
