/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsdesc.h,v 1.5 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _HXSDESC_
#define _HXSDESC_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE	IUnknown			IUnknown;

struct IHXBuffer;
struct IHXValues;

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXStreamDescription
 *
 *  Purpose:
 *
 *	This interface allows transforms between IHXValues and stream
 *	description formats.
 *
 *
 *  IID_IHXStreamDescription:
 *
 *	{00001900-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXStreamDescription, 0x00001900, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59); 

#undef  INTERFACE
#define INTERFACE   IHXStreamDescription

DECLARE_INTERFACE_(IHXStreamDescription, IUnknown)
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
     * IHXStreamDescription methods
     */
	

    /************************************************************************
     *	Method:
     *	    IHXStreamDescription::GetStreamDescriptionInfo
     *	Purpose:
     *	    Get info to initialize the stream description plugin
     */
    STDMETHOD(GetStreamDescriptionInfo)
				(THIS_
				REF(const char*) /*OUT*/ pMimeTypes) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStreamDescription::GetValues
     *	Purpose:
     *	    Transform a media description string into an IHXValues
     *		object.
     */
    STDMETHOD(GetValues)	(THIS_
    				IHXBuffer* 	    /*IN*/  pDescription,
				REF(UINT16)	    /*OUT*/ nValues,
    				REF(IHXValues**)   /*OUT*/ pValueArray) PURE;

    /************************************************************************
     *	Method:
     *	    IHXStreamDescription::GetDescription
     *	Purpose:
     *	    Transform an IHXValues object into a stream description
     *		string.
     */
    STDMETHOD(GetDescription)	(THIS_
				UINT16		    /*IN*/   nValues,
				IHXValues**	    /*IN*/   pValueArray,
    				REF(IHXBuffer*)    /*OUT*/  pDescription) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXRTPPayloadInfo
 *
 *  Purpose:
 *
 *	This interface exposes the conversion table used to convert RTP
 *      timestamps to RMA timestamps and whether or not the content is
 *      timestamp deliverable.
 *
 *
 *  IID_IHXRTPPayloadInfo:
 *
 *	{00001901-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXRTPPayloadInfo, 0x00001901, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59); 

#undef  INTERFACE
#define INTERFACE   IHXRTPPayloadInfo

DECLARE_INTERFACE_(IHXRTPPayloadInfo, IUnknown)
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
     * IHXRTPPayloadInfo methods
     */
	
    /************************************************************************
     *	Method:
     *	    IHXRTPPayloadInfo::PayloadSupported
     *	Purpose:
     *	    Returns TRUE if this payload type is handled by this interface
     */
    STDMETHOD_(HXBOOL, IsPayloadSupported)	    (THIS_
				UINT32      /*IN*/  ulRTPPayloadType) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRTPPayloadInfo::GetTimestampConversionFactors
     *	Purpose:
     *	    Retrieves the RTP and RMA factors for RTP to RMA timestamp ratio.
     *      RTP->RMA is RTPTimestamp * HXFactor / RTPFactor
     *      RMA->RTP is HXTimestamp * RTPFactor / HXFactor
     *  
     *  Note: 
     *      Does not check if the payload type is supported
     */
    STDMETHOD(GetTimestampConversionFactors)	    (THIS_
    				UINT32      /*IN*/  ulRTPPayloadType,
				REF(UINT32) /*OUT*/ ulRTPFactor,
				REF(UINT32) /*OUT*/ ulHXFactor) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRTPPayloadInfo::IsTimestampDeliverable
     *	Purpose:
     *	    Returns TRUE if this payload type is timestamp deliverable
     */
    STDMETHOD_(HXBOOL, IsTimestampDeliverable)	    (THIS_
				UINT32      /*IN*/  ulRTPPayloadType) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXStreamDescriptionSettings
 *
 *  Purpose:
 *
 *	Manage settings related to stream description
 *
 *
 *  IID_IHXStreamDescriptionOptions:
 *
 *	{00001902-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXStreamDescriptionSettings, 0x00001902, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59); 

#undef  INTERFACE
#define INTERFACE   IHXStreamDescriptionSettings

DECLARE_INTERFACE_(IHXStreamDescriptionSettings, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXStreamDescriptionSettings methods
     */
    STDMETHOD(SetOption)(const char* pKey, IHXBuffer* pVal) PURE;
    STDMETHOD(GetOption)(const char* pKey, REF(IHXBuffer*) pVal) PURE;
	
};

#endif /* _HXSDESC_ */
