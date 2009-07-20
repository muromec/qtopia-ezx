/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxphook.h,v 1.4 2005/03/14 19:27:09 bobclark Exp $
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

#ifndef _HXPHOOK_H_
#define _HXPHOOK_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXPacket			IHXPacket;
typedef _INTERFACE	IHXPacketHook			IHXPacketHook;
typedef _INTERFACE	IHXPacketHookManager		IHXPacketHookManager;
typedef _INTERFACE	IHXPacketHookHelper		IHXPacketHookHelper;
typedef _INTERFACE	IHXPacketHookHelperResponse    IHXPacketHookHelperResponse;
typedef _INTERFACE	IHXPacketHookSink		IHXPacketHookSink;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPacketHook
 *
 *  Purpose:
 *
 *	Interface implemented by the top level client to support selective
 *	record
 *
 *  IID_IHXPacketHook:
 *
 *	{00002000-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXPacketHook, 0x00002000, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

DECLARE_INTERFACE_(IHXPacketHook, IUnknown)
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
     * IHXPacketHook methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPacketHook::OnStart
     *	Purpose:
     *	    Called by the core to notify the start of this packet hook session
     */
    STDMETHOD(OnStart)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketHook::OnEnd
     *	Purpose:
     *	    Called by the core to notify the end of this packet hook session
     */
    STDMETHOD(OnEnd)		(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketHook::OnFileHeader
     *	Purpose:
     *	    Called by the core to send file header information
     *
     */
    STDMETHOD(OnFileHeader)	(THIS_
				IHXValues* pValues) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketHook::OnStreamHeader
     *	Purpose:
     *	    Called by the core to send stream header information	
     *
     */
    STDMETHOD(OnStreamHeader)	(THIS_
				IHXValues* pValues) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketHook:OnPacket
     *	Purpose:
     *	    Called by the core to send packet information.
     *
     */
    STDMETHOD(OnPacket)		(THIS_
				IHXPacket* pPacket) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPacketHookManager
 *
 *  Purpose:
 *
 *	Interface to the selective record
 *
 *  IID_IHXPacketHookManager
 *
 *	{00002001-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXPacketHookManager, 0x00002001, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPacketHookManager

DECLARE_INTERFACE_(IHXPacketHookManager, IUnknown)
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
     * IHXPacketHookManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPacketHookManager::InitHook
     *	Purpose:
     *	    called by the top level client to pass the IHXPacketHook object
     */
    STDMETHOD(InitHook)		(THIS_
				IHXPacketHook* pPacketHook) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketHookManager::CloseHook
     *	Purpose:
     *	    called by the top level client to close the hook connection
     */
    STDMETHOD(CloseHook)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketHookManager::StartHook
     *	Purpose:
     *	    called by the top level client to start recording
     */
    STDMETHOD(StartHook)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketHookManager::StopHook
     *	Purpose:
     *	    called by the top level client to stop recording
     */
    STDMETHOD(StopHook)		(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPacketHookHelper
 * 
 *  Purpose:
 * 
 *	provide methods to prepare the packet for recording and send back the core
 * 
 *  IID_IHXPacketHookHelper:
 * 
 *	{00002002-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPacketHookHelper, 0x00002002, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
		0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPacketHookHelper

DECLARE_INTERFACE_(IHXPacketHookHelper, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     * IHXPacketHookHelper methods
     */

    /******************************************************************
     * Method:
     *     IHXPacketHookHelper::StartHook
     *
     * Purpose:
     *	   tell the renderer to start sending the record packets
     *
     */
    STDMETHOD(StartHook)		(THIS_
					ULONG32	ulStreamNumber,
					ULONG32	ulTimeOffset,
					IHXPacketHookHelperResponse* pPacketHookHelperResponse) PURE;


    /******************************************************************
     * Method:
     *    IHXPacketHookHelper::StopHook
     *
     * Purpose:
     *    tell the renderer to stop sending the record packets
     */
    STDMETHOD(StopHook)			(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPacketHookHelperResponse
 *
 *  Purpose:
 *
 *	Response interface to the IHXPacketHookHelper at renderer
 *
 *  IID_IHXPacketHookHelperResponse
 *
 *	{00002003-0901-11d1-8B06-00A024406D59}
 */
DEFINE_GUID(IID_IHXPacketHookHelperResponse, 0x00002003, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPacketHookHelperResponse

DECLARE_INTERFACE_(IHXPacketHookHelperResponse, IUnknown)
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
     * IHXPacketHookHelperResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPacketHookHelperResponse::OnPacket
     *	Purpose:
     *	    called by the renderer to pass the packet for recording
     */
    STDMETHOD(OnPacket)		(THIS_
				IHXPacket* pPacket) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketHookManager::OnEndOfPackets
     *	Purpose:
     *	    called by the renderer to notify the end of this stream
     */
    STDMETHOD(OnEndOfPackets)	(THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPacketHookSink
 * 
 *  Purpose:
 * 
 *	provide a sink to catch pre-hook packets
 * 
 *  IID_IHXPacketHookSink:
 * 
 *	{00002004-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXPacketHookSink, 0x00002004, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPacketHookSink

DECLARE_INTERFACE_(IHXPacketHookSink, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     * IHXPacketHookSink methods
     */

    /******************************************************************
     * Method:
     *     IHXPacketHookSink::StartSink
     *
     * Purpose:
     *	   tell the renderer to start collection pre-record packets
     *
     */
    STDMETHOD(StartSink)		(THIS) PURE;

    /******************************************************************
     * Method:
     *    IHXPacketHookHelper::StopHook
     *
     * Purpose:
     *    tell the renderer to stop collection pre-record packets
     */
    STDMETHOD(StopSink)			(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRecordTimeline
 * 
 *  Purpose:
 * 
 *	provide a sink to catch pre-hook packets
 * 
 *  IID_IHXRecordTimeline:
 * 
 *	{00002005-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXRecordTimeline, 0x00002005, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRecordTimeline

DECLARE_INTERFACE_(IHXRecordTimeline, IUnknown)
{
    /******************************************************************
     * Method:
     *     IHXRecordTimeline::GetRecordPos
     *
     * Purpose:
     *	   translates playback position into current recording position.
     *
     */
    STDMETHOD_(UINT32, GetRecordPos)	(THIS_ UINT32 nPlayPos) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordTimeline::SetRecordStartPos
     *
     * Purpose:
     *    requests to start recording from a nPlayPos playback position.
     *	  Renderer might use this method when key-frame rules require
     *    to start recoding earlier then requested by user.
     */
    STDMETHOD_(void, SetRecordStartPos) (THIS_ UINT32 nPlayPos) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordTimeline::OnPlaybackPos
     *
     * Purpose:
     *    sends current playback position to recording timeline.
     */
    STDMETHOD_(void, OnPlaybackPos) (THIS_ UINT32 nPlayPos) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordTimeline::IsRecordStartPosSet
     *
     * Purpose:
     *    returns TRUE if recording start position is set and not going to 
     *	  be changed by any renderer.
     */
    STDMETHOD_(HXBOOL, RecordStartPosFixed) (THIS) PURE;

    /******************************************************************
     * Method:
     *    IHXRecordTimeline::AdjustRecordTimeline
     *
     * Purpose:
     *    allows to adjust recording timeline when seeking while 
     *	  recording.
     */
    STDMETHOD_(void, AdjustRecordTimeline) (THIS_ INT32 nAdjustment) PURE;
};



#endif /* _HXPHOOK_H_ */
