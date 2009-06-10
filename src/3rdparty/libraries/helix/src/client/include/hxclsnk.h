/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxclsnk.h,v 1.5 2007/07/06 21:58:18 jfinnecy Exp $
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

#ifndef _HXCLSNK_H_
#define _HXCLSNK_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE   IHXClientAdviseSink       IHXClientAdviseSink;
typedef _INTERFACE   IHXRequest		IHXRequest;
typedef _INTERFACE   IHXValues		IHXValues;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientAdviseSink
 * 
 *  Purpose:
 * 
 *	Interface supplied by client to core to receive notifications of
 *	status changes.
 * 
 *  IID_IHXClientAdviseSink:
 * 
 *	{00000B00-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXClientAdviseSink, 0x00000B00, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientAdviseSink

DECLARE_INTERFACE_(IHXClientAdviseSink, IUnknown)
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
     *	IHXClientAdviseSink methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPosLength
     *	Purpose:
     *	    Called to advise the client that the position or length of the
     *	    current playback context has changed.
     */
    STDMETHOD(OnPosLength)		(THIS_
					UINT32	  ulPosition,
					UINT32	  ulLength) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPresentationOpened
     *	Purpose:
     *	    Called to advise the client a presentation has been opened.
     */
    STDMETHOD(OnPresentationOpened)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPresentationClosed
     *	Purpose:
     *	    Called to advise the client a presentation has been closed.
     */
    STDMETHOD(OnPresentationClosed)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnStatisticsChanged
     *	Purpose:
     *	    Called to advise the client that the presentation statistics
     *	    have changed. 
     */
    STDMETHOD(OnStatisticsChanged)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPreSeek
     *	Purpose:
     *	    Called by client engine to inform the client that a seek is
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
     *	    IHXClientAdviseSink::OnPostSeek
     *	Purpose:
     *	    Called by client engine to inform the client that a seek has
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
     *	    IHXClientAdviseSink::OnStop
     *	Purpose:
     *	    Called by client engine to inform the client that a stop has
     *	    just occurred. 
     *
     */
    STDMETHOD(OnStop)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnPause
     *	Purpose:
     *	    Called by client engine to inform the client that a pause has
     *	    just occurred. The render is informed the last time for the 
     *	    stream's time line before the pause.
     *
     */
    STDMETHOD(OnPause)		(THIS_
				ULONG32		    ulTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnBegin
     *	Purpose:
     *	    Called by client engine to inform the client that a begin or
     *	    resume has just occurred. The render is informed the first time 
     *	    for the stream's time line after the resume.
     *
     */
    STDMETHOD(OnBegin)		(THIS_
				ULONG32		    ulTime) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAdviseSink::OnBuffering
     *	Purpose:
     *	    Called by client engine to inform the client that buffering
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
     *	    IHXClientAdviseSink::OnContacting
     *	Purpose:
     *	    Called by client engine to inform the client is contacting
     *	    hosts(s).
     *
     */
    STDMETHOD(OnContacting)	(THIS_
				 const char*	pHostName) PURE;
};


// $Private:

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXClientRequestSink
 *
 *  Purpose:
 *
 *	Enables top level clients to get notified of new URLs
 *	
 *
 *      IID_IHXClientRequestSink
 *
 *	{00000B01-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXClientRequestSink, 0x00000B01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientRequestSink

DECLARE_INTERFACE_(IHXClientRequestSink, IUnknown)
{
    /************************************************************************
     *	Method:
     *	    IHXClientRequestSink::OnNewRequest
     *	Purpose:
     *	    Inform TLC of the new request. The TLC may choose to 
     *	    modify RequestHeaders at this time.
     */

    STDMETHOD(OnNewRequest)  (THIS_ IHXRequest* pNewRequest) PURE;
};

// $EndPrivate.

// 
// State of the client engine, used in IHXClientState and IHXClientStateAdviseSink interfaces
//
typedef enum _EHXClientState
{
	HX_CLIENT_STATE_READY = 0,	// uninitialized
	HX_CLIENT_STATE_CONNECTING,	// attempting to connect to sources
	HX_CLIENT_STATE_CONNECTED,	// connected to sources (realized)
	HX_CLIENT_STATE_OPENING,	// opening sources
	HX_CLIENT_STATE_OPENED,		// opened sources
	HX_CLIENT_STATE_PREFETCHING,	// obtaining resources, buffering, etc.
	HX_CLIENT_STATE_PREFETCHED,	// ready for playback at time zero
	HX_CLIENT_STATE_PLAYING,	// currently playing
	HX_CLIENT_STATE_PAUSED,		// paused
	HX_CLIENT_STATE_SEEKING		// seeking
} EHXClientState;

//
// Client state status, indicates if the client is halted and waiting for IHXClientState::Resume
// Used internally in HXPlayer
//
typedef enum _EHXClientStateStatus
{
	HX_CLIENT_STATE_STATUS_ACTIVE = 0,	// Client state status is active
	HX_CLIENT_STATE_STATUS_HALTED		// Client currently halted
} EHXClientStateStatus;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXClientState
 *
 *  Purpose:
 *
 *	Interface to controlling the client state
 *
 *  IID_IHXClientState:	{E4272CEE-0B51-4ae9-901B-FFA28057E9AD}
 */
DEFINE_GUID(IID_IHXClientState, 0xe4272cee, 0xb51, 0x4ae9, 0x90, 0x1b, 0xff, 0xa2, 0x80, 0x57, 0xe9, 0xad);

#undef  INTERFACE
#define INTERFACE   IHXClientState

DECLARE_INTERFACE_(IHXClientState, IUnknown)
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
     * IHXClientState methods
     */

    /************************************************************************
     *	Method:
     *		IHXClientState::SetConfig
     *	Purpose:
     *		Set config values to modify the behavior of the client engine.
     *		("HaltInConnected", TRUE) : Halt in the CONNECTED state, do not
     *			automatically transition to the PREFETCHED state.
     *		("RestartToPrefetched", TRUE) : At end of media, do not close the engine.
     *			Pause and seek to zero at end of media, so the client state is
     *			PREFETCHED with a current media time of zero, ready for playback.
     *
     *		Refer to EHXClientState and EHXClientStateSTatus for further information.
     */
    STDMETHOD(SetConfig)	(THIS_
		    		IHXValues* pValues) PURE;

    /************************************************************************
     *	Method:
     *		IHXClientState::Resume
     *	Purpose:
     *		Resume player state transition
     *		Used when the "HaltInConnected" config value has been set to true in SetConfig,
     *		and the client has halted in the connected state. Resuming will transition
     *		the client from the CONNECTED to the PREFETCHED state.
     *		Refer to EHXClientState	for further information.
     */
    STDMETHOD(Resume)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientEngine::GetState
     *	Purpose:
     *	    Returns the current state of the player, as defined in EHXClientState
     */
    STDMETHOD_(UINT16, GetState)(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientStateAdviseSink
 * 
 *  Purpose:
 * 
 *	Interface supplied by client to core to receive notifications of
 *	status changes.
 * 
 *  IID_IHXClientStateAdviseSink:
 * 
 *	{D05F1B45-5B3E-4904-8690-95DEF862090E}
 */

DEFINE_GUID(IID_IHXClientStateAdviseSink, 0xd05f1b45, 0x5b3e, 0x4904, 0x86, 0x90, 0x95, 0xde, 0xf8, 0x62, 0x9, 0xe);


#undef  INTERFACE
#define INTERFACE   IHXClientStateAdviseSink

DECLARE_INTERFACE_(IHXClientStateAdviseSink, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientStateAdviseSink::OnStateChange
     *	Purpose:
     *	    Called by client engine to inform the client that the state has changed.
     *	    States are defined in the enum EHXClientState, defined in this file.
     *
     */
    STDMETHOD(OnStateChange)	(THIS_
				UINT16 uOldState,
				UINT16 uNewState
				) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientStateAdviseSinkControl
 * 
 *  Purpose:
 * 
 *	Interface supplied by client to core to receive notifications of
 *	status changes.
 * 
 *  IID_IHXClientStateAdviseSinkControl:
 * 
 *	{8E14B1C5-A931-47b0-BFEA-EA8250C98BF8}
 */

DEFINE_GUID(IID_IHXClientStateAdviseSinkControl, 
		0x8e14b1c5, 0xa931, 0x47b0, 0xbf, 0xea, 0xea, 0x82, 0x50, 0xc9, 0x8b, 0xf8);


#undef  INTERFACE
#define INTERFACE   IHXClientStateAdviseSinkControl

DECLARE_INTERFACE_(IHXClientStateAdviseSinkControl, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientStateAdviseSinkControl::AddClientStateAdviseSink
     *	Purpose:
     */
    STDMETHOD(AddClientStateAdviseSink) (THIS_
		    		IHXClientStateAdviseSink* pClientStateAdviseSink) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientStateAdviseSinkControl::RemoveClientStateAdviseSink
     *	Purpose:
     */
    STDMETHOD(RemoveClientStateAdviseSink) (THIS_
		    		IHXClientStateAdviseSink* pClientStateAdviseSink) PURE;

};

#endif /* _HXCLSNK_H_ */
