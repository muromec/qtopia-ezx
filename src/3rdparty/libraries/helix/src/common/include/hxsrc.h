/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsrc.h,v 1.6 2004/11/08 19:43:26 hwatson Exp $
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

#ifndef _HXSRC_H_
#define _HXSRC_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IHXRawSourceObject		IHXRawSourceObject;
typedef _INTERFACE	IHXRawSinkObject		IHXRawSinkObject;
typedef _INTERFACE	IHXSourceFinderObject		IHXSourceFinderObject;
typedef _INTERFACE	IHXSourceFinderResponse	IHXSourceFinderResponse;
typedef _INTERFACE	IHXRequest			IHXRequest;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRawSourceObject
 * 
 *  Purpose:
 * 
 *	Object that serves packets to sinks
 * 
 *  IID_IHXRawSourceObject:
 * 
 *	{00001000-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXRawSourceObject, 0x00001000, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRawSourceObject

DECLARE_INTERFACE_(IHXRawSourceObject, IUnknown)
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
	 *	IHXRawSourceObject methods
	 */

    /************************************************************************
     *	Method:
     *	    IHXRawSourceObject::Init
     *	Purpose:
     *	    Initializes the connection between the source and the sink
     */
    STDMETHOD(Init)		(THIS_
				IUnknown* pUnknown) PURE;

    STDMETHOD(Done)		(THIS) PURE;

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
 *	IHXRawSinkObject
 * 
 *  Purpose:
 * 
 *	Object that receives raw packets from a source
 * 
 *  IID_IHXRawSinkObject:
 * 
 *	{00001001-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXRawSinkObject, 0x00001001, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRawSinkObject

DECLARE_INTERFACE_(IHXRawSinkObject, IUnknown)
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
	 *	IHXRawSinkObject methods
	 */

    /************************************************************************
     *	Method:
     *	    IHXRawSinkObject::InitDone
     *	Purpose:
     *	    Callback after source object has initialized the connection
     */
    STDMETHOD(InitDone)			(THIS_
					HX_RESULT	status) PURE;

    STDMETHOD(FileHeaderReady)		(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader) PURE;

    STDMETHOD(StreamHeaderReady)	(THIS_
					HX_RESULT	status,
					IHXValues*	pHeader) PURE;

    STDMETHOD(PacketReady)		(THIS_
					HX_RESULT	status,
					IHXPacket*	pPacket) PURE;

    STDMETHOD(StreamDone)		(THIS_
					UINT16		unStreamNumber) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSourceFinderObject
 * 
 *  Purpose:
 * 
 *	Object that allows a sink to search for a raw packet source
 * 
 *  IID_IHXSourceFinderObject:
 * 
 *	{00001002-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXSourceFinderObject, 0x00001002, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#define CLSID_IHXSourceFinderObject	IID_IHXSourceFinderObject

#undef  INTERFACE
#define INTERFACE   IHXSourceFinderObject

#define CLSID_IHXSourceFinderObject IID_IHXSourceFinderObject

DECLARE_INTERFACE_(IHXSourceFinderObject, IUnknown)
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
	 *	IHXSourceFinderObject methods
	 */

    STDMETHOD(Init)		(THIS_ 
				IUnknown*	pUnknown) PURE;

    STDMETHOD(Find)		(THIS_
				IHXRequest*	pRequest) PURE;

    STDMETHOD(Done)		(THIS) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSourceFinderResponse
 * 
 *  Purpose:
 * 
 *	Object that returns a raw packet source to a sink
 * 
 *  IID_IHXSourceFinderResponse:
 * 
 *	{00001003-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXSourceFinderResponse, 0x00001003, 0x901, 0x11d1,
            0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXSourceFinderResponse

DECLARE_INTERFACE_(IHXSourceFinderResponse, IUnknown)
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
	 *	IHXSourceFinderResponse methods
	 */

    STDMETHOD(InitDone)			(THIS_
					HX_RESULT	status) PURE;

    STDMETHOD(FindDone)			(THIS_
					HX_RESULT	status,
					IUnknown*	pUnknown) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSourceFinderFileResponse
 *      A source finder response interface for static content.  Returns the
 *      underlying file object associated with the raw source container. 
 *
 *  IID_IHXSourceFinderFileResponse:
 * 
 *	{00001004-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXSourceFinderFileResponse, 0x00001004, 0x901, 0x11d1,
            0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);


#undef  INTERFACE
#define INTERFACE   IHXSourceFinderFileResponse

DECLARE_INTERFACE_(IHXSourceFinderFileResponse, IUnknown)
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
	 *	IHXSourceFinderFileResponse methods
	 */

    STDMETHOD(FindDone)			(THIS_
					HX_RESULT	status,
					IUnknown*	pSourceContainer,
                    IUnknown*   pFileObject) PURE;

};



#endif /* _HXSRC_H_ */
