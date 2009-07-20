/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdtcvt.h,v 1.5 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _HXDTCVT_H
#define _HXDTCVT_H

typedef _INTERFACE	IHXDataConvertSystemObject
						IHXDataConvertSystemObject;
typedef _INTERFACE	IHXPacket               IHXPacket;
typedef _INTERFACE	IHXDataConvert               IHXDataConvert;
typedef _INTERFACE	IHXDataConvertResponse       IHXDataConvertResponse;
typedef _INTERFACE	IHXDataRevert                IHXDataRevert;
typedef _INTERFACE	IHXDataRevertResponse        IHXDataRevertResponse;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDataConvertSystemObject
 *
 *  Purpose:
 *
 *	Object that allows Controller to communicate with a specific
 *	Data Convert plugin session (similar to IHXFileSystemObject)
 *
 *  Implemented by:
 *
 *	Server side plugin.
 * 
 *  IID_IMADataConvertSystemObject:
 * 
 *	{00003900-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXDataConvertSystemObject,
    0x00003900, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDataConvertSystemObject

DECLARE_INTERFACE_(IHXDataConvertSystemObject, IUnknown)
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
     * IHXDataConvertSystemObject
     */
     
    /***********************************************************************
     * Method: GetDataConvertInfo
     *
     * Purpose:
     *	Returns information needed for proper instantiation of data
     *  convert plugin.  pShortName should be a short, human readable
     *  name in the form of "company-dcname".  For example:
     *  pShortName = "rn-dataconvert"
     */
    STDMETHOD(GetDataConvertInfo) (THIS_ REF(const char*) pShortName) PURE;
    
    /***********************************************************************
     * Method: InitDataConvertSystem
     *
     * Purpose:
     *	Pass in options from config file from under DataConvertMount
     *  for this specific plugin.
     */
    STDMETHOD(InitDataConvertSystem) (THIS_ IHXValues* pOptions) PURE;
    
    /***********************************************************************
     * Method: CreateDataConvert
     *
     * Purpose:
     *	Purpose:
     *	 System is requesting an IHXDataConvert object for this mount
     *   point.
     */
    STDMETHOD(CreateDataConvert) (THIS_ IUnknown** /*OUT*/ ppConvObj) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDataConvert
 *
 *  Purpose:
 *
 *	Per connection object to handle the actual data and header
 *	conversions.
 *
 *  Implemented by:
 *
 *	Server side plugin.
 * 
 *  IID_IMADataConvert:
 * 
 *	{00003901-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXDataConvert,
    0x00003901, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDataConvert

DECLARE_INTERFACE_(IHXDataConvert, IUnknown)
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
     *  IHXDataConvert
     */
    
    /*
     * NOTE: for each of ConvertFileHeader, ConvertStreamHeader,
     *    ConvertData, you can call the appropriate done method on
     *    the response object with a result of HXR_OK and a NULL buffer
     *    and the system will use the original header/packet.  Do this
     *    if you aren't going to change things in the header/packet.
     */
    /************************************************************************
     * Method: DataConvertInit
     *
     *Purpose:
     *	Basic initialization, mainly just to pass in response object.
     */
    STDMETHOD(DataConvertInit) (THIS_ IHXDataConvertResponse* pResponse) PURE;

    /************************************************************************
     * Method: ConvertFileHeader
     *
     * Purpose:
     *	Pass in file headers for data conversion.
     */
    STDMETHOD(ConvertFileHeader) (THIS_ IHXValues* pFileHeader) PURE;
    
    /************************************************************************
     * Method: ConvertStreamHeader
     *	
     * Purpose:
     *	Pass in stream headers for data conversion.
     */
    STDMETHOD(ConvertStreamHeader) (THIS_ IHXValues* pStreamHeader) PURE;
    
    /************************************************************************
     * Method: GetConversionMimeType
     *
     * Purpose:
     *	Tell the server what converstion type you are using for the
     *	session.
     */
    STDMETHOD(GetConversionMimeType)
	(THIS_ REF(const char*) pConversionType) PURE;

    /************************************************************************
     * Method: ConvertData
     *
     * Purpose:
     *	Pass in data to be converted.
     */
    STDMETHOD(ConvertData) (THIS_ IHXPacket* pPacket) PURE;
    
    /************************************************************************
     * Method: ControlBufferReady
     *
     * Purpose:
     *  Pass in a control channel buffer sent from the IHXDataRevert
     *  on the other side (player).
     */
    STDMETHOD(ControlBufferReady) (THIS_ IHXBuffer* pBuffer) PURE;
    
    /************************************************************************
     * Method: SetMulticastTransportConverter
     *
     * Purpose:
     *  In this case the IHXDataConvert is only handling the header
     *  conversions per player and this call is handing you the data
     *  converter which is doing the data.  This will a different
     *  instance of the same object.
     */
    STDMETHOD(SetMulticastTransportConverter) (THIS_
	    IHXDataConvert* pConverter) PURE;
    
    
    /************************************************************************
     * Method: AddMulticastControlConverter
     *
     * Purpose:
     *  In this case the IHXDataConvert is only handling the data 
     *  conversions for all of the players (but only once because it's
     *  multicast).  This call is handing you one of a possible many
     *  IHXDataConvert objects which will be handling the header
     *  conversions.
     */
    STDMETHOD(AddMulticastControlConverter) (THIS_
	    IHXDataConvert* pConverter) PURE;
    
    /************************************************************************
     * Method: Done
     *
     * Purpose:
     *  Let IHXDataConvert know that it is done. This is mainly to clear
     *  circular refs between multicast transport and controllers.
     */
    STDMETHOD(Done)			(THIS) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDataConvertResponse
 *
 *  Purpose:
 *
 *	Response object for IHXDataConvert.
 *
 *  Implemented by:
 *
 *	Server Core.
 * 
 *  IID_IMADataConvertResponse:
 * 
 *	{00003902-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXDataConvertResponse,
    0x00003902, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDataConvertResponse

DECLARE_INTERFACE_(IHXDataConvertResponse, IUnknown)
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
     *  IHXDataConvertResponse
     */

    /*
     * NOTE: for each of ConvertFileHeader, ConvertStreamHeader,
     *    ConvertData, you can call the appropriate done method on
     *    the response object with a result of HXR_OK and a NULL buffer
     *    and the system will use the original header/packet.  Do this
     *    if you aren't going to change things in the header/packet.
     */
    /************************************************************************
     * Method: DataConvertInitDone
     *
     * Purpose:
     * 	Async notification that the IHXDataConvert is done with
     *  intialization.
     */
    STDMETHOD(DataConvertInitDone) (THIS_ HX_RESULT status) PURE;
    
    /************************************************************************
     * Method: ConvertedFileHeaderReady
     *
     * Purpose:
     *	Async notification that the IHXDataCovert is done converting the
     *  file header.
     */
    STDMETHOD(ConvertedFileHeaderReady) (THIS_
			    HX_RESULT status, IHXValues* pFileHeader) PURE;
    
    /************************************************************************
     * Method: ConvertedStreamHeaderReady
     *
     * Purpose:
     *	Async notification that the IHXDataConvert is done converting the
     *	stream header.
     */
    STDMETHOD(ConvertedStreamHeaderReady) (THIS_
			    HX_RESULT status, IHXValues* pStreamHeader) PURE;

    /************************************************************************
     * Method: ConvertedDataReady
     *
     * Purpose:
     *	Async notification that the IHXDataConvert is done converting
     *  the stream data packet.
     */
    STDMETHOD(ConvertedDataReady) (THIS_ HX_RESULT status,
	    				IHXPacket* pPacket) PURE;

    /************************************************************************
     * Method: SendControlBuffer
     *
     * Purpose:
     *	Provided to allow IHXDataConvert to send an arbitrary buffer
     *  to the IHXDataRevert on the other side (player).
     */
    STDMETHOD(SendControlBuffer) (THIS_ IHXBuffer* pBuffer) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDataRevert
 * 
 *  Purpose:
 *
 *	Revert data coming from coresponding IHXDataConvert on server.
 *
 *  Implemented by:
 *
 *	Player side plugin.
 *
 *  IID_IMADataRevert:
 * 
 *	{00003903-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXDataRevert,
    0x00003903, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDataRevert

DECLARE_INTERFACE_(IHXDataRevert, IUnknown)
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
     *  IHXDataRevert
     */

    /************************************************************************
     * Method: DataRevertInit
     *
     * Purpose:
     *  Basic inialization of IHXDataRevert.  Mainly just to pass in
     *  response object.
     */
    STDMETHOD(DataRevertInit) (THIS_ IHXDataRevertResponse* pResponse) PURE;

    /************************************************************************
     * Method: GetDataRevertInfo
     *
     * Purpose:
     *	Allow IHXDataRevert to notify player core about which data
     *  conversion mime types it is willing to handle.
     */
    STDMETHOD(GetDataRevertInfo) (THIS_ REF(const char**)
					ppConversionMimeTypes) PURE;
    
    /************************************************************************
     * Method: RevertFileHeader
     *
     * Purpose:
     *	Pass in converted FileHeader to allow IHXDataRevert to revert
     *  the header.
     */
    STDMETHOD(RevertFileHeader)	(THIS_ IHXValues* pFileHeader) PURE;
    
    /************************************************************************
     * Method: RevertStreamHeader
     *
     * Purpose:
     * 	Pass in converted StreamHeader to allow IHXDataRevert to revert
     *  the header.
     */
    STDMETHOD(RevertStreamHeader)(THIS_ IHXValues* pStreamHeader) PURE;
    
    /************************************************************************
     * Method: RevertData
     *
     * Purpose:
     *	Pass in converted stream data to allow IHXDataRevert to 
     *  revert the data.
     */
    STDMETHOD(RevertData) (THIS_ IHXPacket* pPacket) PURE;
    
    /************************************************************************
     * Method: ControlBufferReady
     *
     * Purpose:
     *	Pass in control channel buffer received from corresponding
     *  IHXDataConvert on server side.
     */
    STDMETHOD(ControlBufferReady) (THIS_ IHXBuffer* pBuffer) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDataRevertResponse
 * 
 *  Purpose:
 *
 *	Response ojbect for IHXDataRevert.
 *
 *  Implemented by:
 *
 *	Player core.
 *
 *  IID_IMADataRevertResponse:
 * 
 *	{00003904-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXDataRevertResponse,
    0x00003904, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDataRevertResponse

DECLARE_INTERFACE_(IHXDataRevertResponse, IUnknown)
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
     *  IHXDataRevertResponse
     */

    /************************************************************************
     * Method: DataRevertInitDone
     *
     * Purpose:
     *	Async notification that the IHXDataRevert is done intializing
     *  and can begin processing headers.
     */
    STDMETHOD(DataRevertInitDone) (THIS_ HX_RESULT status) PURE;
    
    /************************************************************************
     * Method: RevertedFileHeaderReady
     *
     * Purpose:
     *	Async notification that the IHXDataRevert is done reverting the
     *  file headers.
     */
    STDMETHOD(RevertedFileHeaderReady)	(THIS_ 
	    			HX_RESULT status, IHXValues* pHeader) PURE;
    
    /************************************************************************
     * Method: RevertedStreamHeaderReady
     *
     * Purpose:
     * 	Async notification that the IHXDataRevert is done reverting the
     *  stream headers.
     */
    STDMETHOD(RevertedStreamHeaderReady) (THIS_
				HX_RESULT status, IHXValues* pHeader) PURE;

    /************************************************************************
     * Method: RevertedDataReady
     *
     * Purpose:
     *	Async notification that the IHXDataRevert is done reverting the
     *  stream data.
     */
    STDMETHOD(RevertedDataReady) (THIS_ HX_RESULT status,
	    				IHXPacket* pPacket) PURE;

    /************************************************************************
     * Method: SendControlBuffer
     *
     * Purpose:
     *	Provided to allow IHXDataRevert to send an arbitrary control
     *  buffer to the IHXDataConvert on the other side (server).
     */
    STDMETHOD(SendControlBuffer) (THIS_ IHXBuffer* pBuffer) PURE;

};


#endif
