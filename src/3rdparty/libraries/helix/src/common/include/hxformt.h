/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxformt.h,v 1.9 2006/06/05 23:37:41 jzeng Exp $
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

#ifndef _HXFORMT_H_
#define _HXFORMT_H_

#include "hxfiles.h"

/*
 * Forward declarations of some interfaces defined here-in.
 */
typedef _INTERFACE	IHXFileFormatObject	    IHXFileFormatObject;
typedef _INTERFACE      IHXBroadcastFormatObject   IHXBroadcastFormatObject;
typedef _INTERFACE	IHXFormatResponse       IHXFormatResponse;
typedef _INTERFACE	IHXFormatReuse          IHXFormatReuse;
typedef _INTERFACE	IHXFileObject		    IHXFileObject;
typedef _INTERFACE      IHXNetworkServices	    IHXNetworkServices;
typedef _INTERFACE      IHXPacket                  IHXPacket;
typedef _INTERFACE      IHXValues                  IHXValues;
typedef _INTERFACE	IHXPacketTimeOffsetHandler IHXPacketTimeOffsetHandler;
typedef _INTERFACE	IHXPacketTimeOffsetHandlerResponse
						    IHXPacketTimeOffsetHandlerResponse;
typedef _INTERFACE	IHXLiveFileFormatInfo	    IHXLiveFileFormatInfo;
// $Private:
typedef _INTERFACE	IHXBroadcastLatency	    IHXBroadcastLatency;
typedef _INTERFACE	IHXPayloadFormatObject	    IHXPayloadFormatObject;
typedef _INTERFACE	IHXBlockFormatObject	    IHXBlockFormatObject;
typedef _INTERFACE	IHXFileFormatHeaderAdvise  IHXFileFormatHeaderAdvise;
typedef _INTERFACE	IHXFileFormatHeaderAdviseResponse
						    IHXFileFormatHeaderAdviseResponse;
typedef _INTERFACE	IHXSetPlayParam	    IHXSetPlayParam;
typedef _INTERFACE	IHXSetPlayParamResponse    IHXSetPlayParamResponse;
typedef _INTERFACE	IHXSeekByPacket         IHXSeekByPacket;
typedef _INTERFACE	IHXSeekByPacketResponse    IHXSeekByPacketResponse;
// $EndPrivate.


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileFormatObject
 * 
 *  Purpose:
 * 
 *	Object that allows a Controller to communicate with a specific
 *	File Format plug-in session
 * 
 *  IID_IHXFileFormatObject:
 * 
 *	{00000F00-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXFileFormatObject, 0x00000F00, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileFormatObject

DECLARE_INTERFACE_(IHXFileFormatObject, IUnknown)
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
     *	IHXFileFormatObject methods
     */

    /************************************************************************
     *	Method:
     *	    IHXFileFormatObject::GetFileFormatInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of file format 
     *	    plugins.
     */
    STDMETHOD(GetFileFormatInfo)(THIS_
				REF(const char**) /*OUT*/ pFileMimeTypes,
				REF(const char**) /*OUT*/ pFileExtensions,
				REF(const char**) /*OUT*/ pFileOpenNames
				) PURE;

    STDMETHOD(InitFileFormat)	
			(THIS_
		        IHXRequest*		/*IN*/	pRequest, 
			IHXFormatResponse*	/*IN*/	pFormatResponse,
			IHXFileObject*		/*IN*/  pFileObject) PURE;

    STDMETHOD(GetFileHeader)	(THIS) PURE;

    STDMETHOD(GetStreamHeader)	(THIS_
				UINT16 unStreamNumber) PURE;

    STDMETHOD(GetPacket)	(THIS_
				UINT16 unStreamNumber) PURE;

    STDMETHOD(Seek)		(THIS_
				ULONG32 ulOffset) PURE;

    STDMETHOD(Close)		(THIS) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXBroadcastFormatObject
 * 
 *  Purpose:
 * 
 *	Object that allows a Controller to communicate with a specific
 *	Broadcast Format plug-in session
 * 
 *  IID_IHXBroadcastFormatObject:
 * 
 *	{00000F01-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXBroadcastFormatObject, 0x00000F01, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXBroadcastFormatObject

DECLARE_INTERFACE_(IHXBroadcastFormatObject, IUnknown)
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
     *	IHXBroadcastFormatObject methods
     */

    /************************************************************************
     *	Method:
     *	    IHXBroadcastFormatObject::GetBroadcastFormatInfo
     *	Purpose:
     *	    Returns information vital to the instantiation of broadcast format 
     *	    plugins.
     */
    STDMETHOD(GetBroadcastFormatInfo)(THIS_
				REF(const char*) /*OUT*/ pToken) PURE;

    STDMETHOD(InitBroadcastFormat) (THIS_
				 const char*		/*IN*/	pURL, 
				 IHXFormatResponse*	/*IN*/	pFormatResponse
				) PURE;

    STDMETHOD(GetFileHeader)	(THIS) PURE;

    STDMETHOD(GetStreamHeader)	(THIS_
				UINT16 unStreamNumber) PURE;

    STDMETHOD(StartPackets)	(THIS_
				UINT16 unStreamNumber) PURE;

    STDMETHOD(StopPackets)	(THIS_
				UINT16 unStreamNumber) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFormatResponse
 * 
 *  Purpose:
 * 
 *	Object that allows a specific File Format Object to communicate 
 *	with its user
 * 
 *  IID_IHXFormatResponse:
 * 
 *	{00000F02-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXFormatResponse, 0x00000F02, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFormatResponse

DECLARE_INTERFACE_(IHXFormatResponse, IUnknown)
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
     *	IHXFormatResponse methods
     */
    STDMETHOD(InitDone)			(THIS_
					HX_RESULT	status) PURE;

    STDMETHOD(PacketReady)		(THIS_
					HX_RESULT	status,
					IHXPacket*	pPacket) PURE;

    STDMETHOD(SeekDone)			(THIS_
					HX_RESULT	status) PURE;

    STDMETHOD(FileHeaderReady)		(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader) PURE;

    STDMETHOD(StreamHeaderReady)	(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader) PURE;

    STDMETHOD(StreamDone)		(THIS_
					UINT16		unStreamNumber) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFormatReuse
 * 
 *  Purpose:
 * 
 *  The Controller may reuse a File Format object for multiple sessions.
 *  If the File Format object needs notification that it is being reused
 *  (eg. to reset internal state), this interface may be implemented.  If
 *  implemented, the Controller will only call Reinitialize().  It will
 *  not unsubscribe from any ASM rules explicitly.  If not implemented,
 *  the Controller will assume that unsubscribing from all ASM rules and
 *  seeking to zero is sufficient.
 * 
 *  IID_IHXFormatReuse:
 * 
 *  {e55077c4-a299-11d7-864c-0002b3658720}
 * 
 */
DEFINE_GUID(IID_IHXFormatReuse, 0xe55077c4, 0xa299, 0x11d7, 0x86, 0x4c, 0x0, 
			0x2, 0xb3, 0x65, 0x87, 0x20);

#undef  INTERFACE
#define INTERFACE   IHXFormatReuse

DECLARE_INTERFACE_(IHXFormatReuse, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /* IHXFormatReuse methods */
    STDMETHOD_(HXBOOL,CanReuse)       (THIS_ IHXRequest* pRequest) PURE;
    STDMETHOD(Reinitialize)         (THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPacketFormat
 * 
 *  Purpose:
 * 
 *	Interface that modifies the behavior of an IHXFileFormat by defining
 *	the packet format it will be creating.
 * 
 *  IID_IHXPacketFormat:
 * 
 *	{00000F03-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXPacketFormat, 0x00000F03, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPacketFormat

DECLARE_INTERFACE_(IHXPacketFormat, IUnknown)
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
     *	IHXPacketFormat methods
     */
    STDMETHOD(GetSupportedPacketFormats)			
    					(THIS_
					REF(const char**) pFormats) PURE;

    STDMETHOD(SetPacketFormat)		(THIS_
					const char*	pFormat) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPacketTimeOffsetHandler
 * 
 *  Purpose:
 * 
 *	Provides methods for handling the changing of a packets timestamp.
 * 
 *  IID_IHXPacketTimeOffsetHandler:
 * 
 *	{00000F04-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXPacketTimeOffsetHandler, 0x00000F04, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);


DECLARE_INTERFACE_(IHXPacketTimeOffsetHandler, IUnknown)
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
     * IHXPacketTimeOffsetHandler methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPacketTimeOffsetHandler::Init
     *	Purpose:
     *	    Initialize the IHXPacketTimeOffsetHandler and set the response.
     *      Implementors should look up the MimeType.
     */
    STDMETHOD(Init)		(THIS_
				IHXPacketTimeOffsetHandlerResponse* pResponse,
				IHXValues* pHeader,
				IUnknown* pContext) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketTimeOffsetHandler::SetTimeOffset
     *	Purpose:
     *	    Called to set the time offset.  Uses a bool and a UINT32 instead
     *      of and INT32 so that the time offset wraps around after 47 days
     *      instead of 24.  bPlus says whether to add or subtract.
     */
    STDMETHOD(SetTimeOffset)	(THIS_
				UINT32 ulTimeOffset,
				HXBOOL bPlus) PURE;

    /************************************************************************
     *	Method:
     *	    IHXPacketTimeOffsetHandler::HandlePacket
     *	Purpose:
     *	    give the IHXPacketTimeOffsetHandler a packet to modify for the
     *      time offset.
     */
    STDMETHOD(HandlePacket)	(THIS_
				IHXPacket* pPacket) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPacketTimeOffsetHandlerResponse
 * 
 *  Purpose:
 * 
 *	Provides methods for the IHXPacketTimeOffsetHandler to respond to.
 * 
 *  IID_IHXPacketTimeOffsetHandlerResponse:
 * 
 *	{00000F05-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXPacketTimeOffsetHandlerResponse, 0x00000F05, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);


DECLARE_INTERFACE_(IHXPacketTimeOffsetHandlerResponse, IUnknown)
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
     * IHXPacketTimeOffsetHandler methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPacketTimeOffsetHandler::PacketReady
     *	Purpose:
     *	    Called by IHXPacketTimeOffsetHandler to pass back the packet 
     *      when it is done with it.
     */
    STDMETHOD(TimeOffsetPacketReady)	(THIS_
					IHXPacket* pPacket) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXLiveFileFormatInfo
 * 
 *  Purpose:
 * 
 *	Provides miscellaneous information needed to transmit a live stream.
 *	Optionally implemented by the file format object.
 * 
 *  IID_IHXLiveFileFormatInfo:
 * 
 *	{00000F06-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXLiveFileFormatInfo, 0x00000F06, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXLiveFileFormatInfo

DECLARE_INTERFACE_(IHXLiveFileFormatInfo, IUnknown)
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
     *	IHXLiveFileFormatInfo methods
     */

    /************************************************************************
     *	Method:
     *	    IHXLiveFileFormatInfo::VerifyFileCompatibility
     *	Purpose:
     *	    Compares two file headers and returns HXR_OK if these two 
     *	    files can be transmitted sequentially in a single live 
     *	    presentation.
     */
    STDMETHOD(VerifyFileCompatibility)	    (THIS_
					    IHXValues* pFileHeader1,
					    IHXValues* pFileHeader2) PURE;

    /************************************************************************
     *	Method:
     *	    IHXLiveFileFormatInfo::VerifyStreamCompatibility
     *	Purpose:
     *	    Compares two stream headers and returns HXR_OK if these two  
     *	    streams can be transmitted sequentially in a single live 
     *	    presentation.
     */
    STDMETHOD(VerifyStreamCompatibility)    (THIS_
					    IHXValues* pStreamHeader1,
					    IHXValues* pStreamHeader2) PURE;

    /************************************************************************
     *	Method:
     *	    IHXLiveFileFormatInfo::IsLiveResendRequired
     *	Purpose:
     *	    Returns TRUE if this stream requires the latest packet to be
     *	    resent periodically in a live presentation.
     */
    STDMETHOD_(HXBOOL,IsLiveResendRequired)   (THIS_
					    UINT16 unStreamNumber) PURE;

    /************************************************************************
     *	Method:
     *	    IHXLiveFileFormatInfo::GetResendBitrate
     *	Purpose:
     *	    If periodic live resends are required for this stream, this
     *	    method returns the rate at which we should resend packets. The 
     *	    resend rate is measured in bits per second.
     */
    STDMETHOD(GetResendBitrate)		    (THIS_
					    UINT16 unStreamNumber,
					    REF(UINT32) ulBitrate) PURE;

    /************************************************************************
     *	Method:
     *	    IHXLiveFileFormatInfo::GetResendDuration
     *	Purpose:
     *	    If periodic live resends are required for this stream, this
     *	    method returns the number of milliseconds for which this packet 
     *	    should be resent.
     */
    STDMETHOD(GetResendDuration)	    (THIS_
					    IHXPacket* pPacket,
					    REF(UINT32) ulDuration) PURE;

    /************************************************************************
     *	Method:
     *	    IHXLiveFileFormatInfo::FormResendPacket
     *	Purpose:
     *	    Forms a live resend packet based upon the original packet passed
     *	    as the first parameter. This allows the file format plugin to
     *	    make resend packets distinguishable from original packets.
     */
    STDMETHOD(FormResendPacket)		(THIS_
					IHXPacket* pOriginalPacket,
					REF(IHXPacket*) pResendPacket) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSyncFileFormatObject
 * 
 *  Purpose:
 * 
 *	Simple syncronized file format interface
 * 
 *  IID_IHXSyncFileFormatObject:
 * 
 *	{00000F0C-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXSyncFileFormatObject, 0x00000F0C, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXSyncFileFormatObject IID_IHXSyncFileFormatObject

#undef  INTERFACE
#define INTERFACE   IHXSyncFileFormatObject

DECLARE_INTERFACE_(IHXSyncFileFormatObject, IUnknown)
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
     *	IHXSyncFileFormatObject methods
     */

    STDMETHOD(GetFileFormatInfo)	(THIS_
					REF(const char**) /*OUT*/ pFileMimeTypes,
					REF(const char**) /*OUT*/ pFileExtensions,
					REF(const char**) /*OUT*/ pFileOpenNames
					) PURE;

    STDMETHOD(InitFileFormat)	
					(THIS_
					IHXRequest* /*IN*/ pRequest) PURE;

    STDMETHOD(GetFileHeader)		(THIS_ 
					REF(IHXValues*) /*OUT*/ pHeader) PURE;

    STDMETHOD(GetStreamHeader)		(THIS_ 
					REF(IHXValues*) /*OUT*/ pStreamHeader,
					UINT16	         /*IN*/  unStreamNumber) PURE;

    STDMETHOD(GetPacket)		(THIS_ 
					REF(IHXPacket*) /*OUT*/ pPacket) PURE;

    STDMETHOD(Seek)			(THIS_
					ULONG32 /*IN*/ ulSeekTime) PURE;
    
    STDMETHOD(Close)			(THIS) PURE;
};

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXBroadcastLatency
 * 
 *  Purpose:
 * 
 *	Provides information on latency requirements of broadcast streams.
 *	Optionally implemented by the broadcast format objec.
 * 
 *  IID_IHXBroadcastLatency:
 * 
 *	{00000F08-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXBroadcastLatency, 0x00000F08, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXBroadcastLatency

DECLARE_INTERFACE_(IHXBroadcastLatency, IUnknown)
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
     *	IHXBroadcastLatency methods
     */

    /************************************************************************
     *	Method:
     *	    IHXBroadcastLatency::GetLatencyRequirements
     *	Purpose:
     *	    Get latency reqruirements from broadcast format object.
     *
     *	    ulBackOff: the amount of time in mS that packet flow from
     *      the broadcast format object should backoff when no data is available.
     *
     *	    bUsePreBuffer: TRUE means that there will be some realtime latency
     *      between the packets sent to clients at time now, and the live packets
     *      that are available at time now. FALSE means there will be no realtime 
     *      latency.
     */
    STDMETHOD(GetLatencyRequirements)	    (THIS_
					    REF(UINT32) ulBackoff,
					    REF(HXBOOL)   bUsePreBuffer) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXPayloadFormatObject
 * 
 *  Purpose:
 * 
 *	Object that knows how to properly convert data into a particular
 *	payload format
 * 
 *  IID_IHXPayloadFormatObject:
 * 
 *	{00000F07-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXPayloadFormatObject, 0x00000F07, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXPayloadFormatObject

DECLARE_INTERFACE_(IHXPayloadFormatObject, IUnknown)
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
     *	IHXPayloadFormatObject methods
     */
    STDMETHOD(Init)		(THIS_
				IUnknown* pContext,
				HXBOOL bPacketize) PURE;

    STDMETHOD(Close)		(THIS) PURE;

    STDMETHOD(Reset)		(THIS) PURE;

    STDMETHOD(SetStreamHeader)	(THIS_
				IHXValues* pHeader) PURE;

    STDMETHOD(GetStreamHeader)	(THIS_
				REF(IHXValues*) pHeader) PURE;

    STDMETHOD(SetPacket)	(THIS_
				IHXPacket* pPacket) PURE;

    STDMETHOD(GetPacket)	(THIS_
				REF(IHXPacket*) pPacket) PURE;

    STDMETHOD(Flush)		(THIS) PURE;
};
// $EndPrivate.

// $Private:
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXBlockFormatObject
 * 
 *  Purpose:
 * 
 *	Object that knows how to properly convert data into a particular
 *	payload format
 * 
 *  IID_IHXBlockFormatObject:
 * 
 *	{00000F08-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXBlockFormatObject, 0x00000F09, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXBlockFormatObject

DECLARE_INTERFACE_(IHXBlockFormatObject, IUnknown)
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
     *	IHXBlockFormatObject methods
     */
    STDMETHOD(SetByteRange)	(THIS_
                                UINT64    ulFrom,
                                UINT64    ulTo) PURE;
};

typedef enum _HX_CLIENT_MESSAGE_TYPE
{
    CM_RTSP_UNKNOWN,
    CM_RTSP_OPTIONS,
    CM_RTSP_DESCRIBE,
    CM_RTSP_ANNOUNCE,
    CM_RTSP_SETUP,
    CM_RTSP_PLAY,
    CM_RTSP_PAUSE,
    CM_RTSP_TEARDOWN,
    CM_RTSP_GET_PARAMETER,
    CM_RTSP_SET_PARAMETER,
    CM_RTSP_REDIRECT,
    CM_RTSP_RECORD
} HX_CLIENT_MESSAGE_TYPE;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileFormatHeaderAdvise
 * 
 *  Purpose:
 * 
 *	File format wants notification when headers arrive from client.
 * 
 *  IID_IHXFileFormatHeaderAdvise:
 * 
 *	{00000F0A-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXFileFormatHeaderAdvise, 0x00000F0A, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileFormatHeaderAdvise

DECLARE_INTERFACE_(IHXFileFormatHeaderAdvise, IUnknown)
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
     *	IHXFileFormatHeaderAdvise methods
     *
     *  Only implemented for SETUP, other messages may be added later.
     */
    STDMETHOD(OnHeaders)	(THIS_
				 HX_CLIENT_MESSAGE_TYPE pMessageType,
				 IHXValues* pRequestHeaders,
				 IHXFileFormatHeaderAdviseResponse* pResp
				) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileFormatHeaderAdviseResponse
 * 
 *  Purpose:
 * 
 *	Response for RTSP header advise.
 * 
 *  IID_IHXFileFormatHeaderAdviseResponse:
 * 
 *	{00000F0B-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXFileFormatHeaderAdviseResponse, 0x00000F0B, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXFileFormatHeaderAdviseResponse

DECLARE_INTERFACE_(IHXFileFormatHeaderAdviseResponse, IUnknown)
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
     *	IHXFileFormatHeaderAdviseResponse methods
     *
     *  If file format returns error in status, server will disconnect 
     *  from client.  Optional RTSP error code will be returned to client, 
     *  if non-zero; else 461 will be used.
     */
    STDMETHOD(OnHeadersDone)	(THIS_
				 HX_RESULT status,
				 UINT32 ulErrNo) PURE;
};

typedef enum _HX_PLAY_PARAM
{
    HX_PLAYPARAM_SCALE	 // cast ulValues to FIXED32 and use macro
} HX_PLAY_PARAM;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSetPlayParam
 * 
 *  Purpose:
 *	
 *	
 * 
 *  IID_IHXSetPlayParam:
 * 
 *      {0x503c212c-413f-478b-9fc8daa7b145b8a9}
 * 
 */
DEFINE_GUID(IID_IHXSetPlayParam,	    
    0x503c212c, 0x413f, 0x478b, 0x9f, 0xc8, 0xda, 0xa7, 0xb1, 0x45, 0xb8, 0xa9);

#undef  INTERFACE
#define INTERFACE   IHXSetPlayParam

DECLARE_INTERFACE_(IHXSetPlayParam, IUnknown)
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
     *	IHXSetPlayParam methods
     */
    STDMETHOD(SetParam)			(THIS_ 
					HX_PLAY_PARAM param,
		    			UINT32 ulValue,
		    			IHXSetPlayParamResponse* pResp) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSetPlayParamResponse
 * 
 *  Purpose:
 * 
 *	    
 * 
 *  IID_IHXSetPlayParamResponse:
 * 
 *      {0x750008af-5588-4838-85faaa203a32c799}
 * 
 */
DEFINE_GUID(IID_IHXSetPlayParamResponse,   
    0x750008af, 0x5588, 0x4838, 0x85, 0xfa, 0xaa, 0x20, 0x3a, 0x32, 0xc7, 0x99);

#undef  INTERFACE
#define INTERFACE   IHXSetPlayParamResponse

DECLARE_INTERFACE_(IHXSetPlayParamResponse, IUnknown)
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
     *	IHXSetPlayParamResponse methods
     */
    STDMETHOD(SetParamDone)		(THIS_ 
					HX_RESULT status,
					HX_PLAY_PARAM param,					
		    			UINT32 ulValue) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *     IHXSeekByPacket
 * 
 *  Purpose:
 *     
 *     
 * 
 *  IID_IHXSeekByPacket:
 * 
 *      {0x171c3c4e-c4ea-46fd-b47b-c3b82dbb9517}
 * 
 */
DEFINE_GUID(IID_IHXSeekByPacket,          
    0x171c3c4e, 0xc4ea, 0x46fd, 0xb4, 0x7b, 0xc3, 0xb8, 0x2d, 0xbb, 0x95, 0x17);

#undef  INTERFACE
#define INTERFACE   IHXSeekByPacket

DECLARE_INTERFACE_(IHXSeekByPacket, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)          (THIS_
                                       REFIID riid,
                                       void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)         (THIS) PURE;
    STDMETHOD_(ULONG32,Release)                (THIS) PURE;

    /*
     * IHXSeekByPacket methods
     */
    STDMETHOD(SeekToPacket)            (THIS_ 
                                       UINT32 ulPacketNumber,
                                       IHXSeekByPacketResponse* pResp) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *     IHXSeekByPacketResponse
 * 
 *  Purpose:
 * 
 *         
 * 
 *  IID_IHXSeekByPacketResponse:
 * 
 *      {0xe978476d-6c99-4dc6-9279-7525c693dc34}
 * 
 */
DEFINE_GUID(IID_IHXSeekByPacketResponse,   
    0xe978476d, 0x6c99, 0x4dc6, 0x92, 0x79, 0x75, 0x25, 0xc6, 0x93, 0xdc, 0x34);

#undef  INTERFACE
#define INTERFACE   IHXSeekByPacketResponse

DECLARE_INTERFACE_(IHXSeekByPacketResponse, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)          (THIS_
                                       REFIID riid,
                                       void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)         (THIS) PURE;
    STDMETHOD_(ULONG32,Release)                (THIS) PURE;

    /*
     * IHXSeekByPacketResponse methods
     */
    STDMETHOD(SeekToPacketDone)                (THIS_ 
                                       HX_RESULT status,
                                       UINT32 ulStartingTimestamp) PURE;
 };
// $EndPrivate.

/****************************************************************************
 * 
 *  Interface:
 * 
 *     IHXFileFormatFinder
 * 
 *  Purpose: This interface maybe called by a renderer to get
 *           the interface to its file format plugin. Renderers running
 *           inside of the Helix DNA client can currently use a 
 *           IHXStream to get to IHXStreamSource and then QI
 *           IHXStreamSource for IHXFileFormat, but sometimes
 *           renderers are not running inside Helix DNA Client (for
 *           example, sometimes they run in dtdrive). This
 *           interface provides a unified way for them to get to
 *           their fileformat regardless of the environment they
 *           are running in.
 *
 *           Note that this interface will not succeed when
 *           the renderer is playing a stream from a HXNetSource.
 *         
 * 
 *  IID_IHXFileFormatFinder:
 * 
 *      {5D137B92-987B-4faf-9ADF-D5A02E399DEF}
 * 
 */
DEFINE_GUID(IID_IHXFileFormatFinder, 0x5d137b92, 0x987b, 0x4faf, 0x9a, 0xdf,
            0xd5, 0xa0, 0x2e, 0x39, 0x9d, 0xef);

#undef  INTERFACE
#define INTERFACE IHXFileFormatFinder

DECLARE_INTERFACE_(IHXFileFormatFinder, IUnknown)
{
    // IUnknown method
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // IHXFileFormatFinder methods
    STDMETHOD(FindFileFormat) (THIS_ REF(IHXFileFormatObject*) rpFileFormat) PURE;
 };

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXFileFormatObject)
DEFINE_SMART_PTR(IHXBroadcastFormatObject)
DEFINE_SMART_PTR(IHXFormatResponse)
DEFINE_SMART_PTR(IHXFormatReuse)
DEFINE_SMART_PTR(IHXPacketFormat)
DEFINE_SMART_PTR(IHXPacketTimeOffsetHandler)
DEFINE_SMART_PTR(IHXPacketTimeOffsetHandlerResponse)
DEFINE_SMART_PTR(IHXLiveFileFormatInfo)
DEFINE_SMART_PTR(IHXSyncFileFormatObject)
DEFINE_SMART_PTR(IHXBroadcastLatency)
DEFINE_SMART_PTR(IHXPayloadFormatObject)
DEFINE_SMART_PTR(IHXBlockFormatObject)
DEFINE_SMART_PTR(IHXFileFormatHeaderAdvise)
DEFINE_SMART_PTR(IHXFileFormatHeaderAdviseResponse)
DEFINE_SMART_PTR(IHXSetPlayParam)
DEFINE_SMART_PTR(IHXSetPlayParamResponse)
DEFINE_SMART_PTR(IHXSeekByPacket)
DEFINE_SMART_PTR(IHXSeekByPacketResponse)
DEFINE_SMART_PTR(IHXFileFormatFinder)

#endif  /* _HXFORMT_H_ */
