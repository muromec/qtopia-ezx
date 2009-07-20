/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdrm.h,v 1.6 2008/03/21 06:26:22 gbajaj Exp $
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

#ifndef _HXDRM_H_
#define _HXDRM_H_

/* plugin properties to identify a source handler */

#define PLUGIN_SOURCEHANDLER_TYPE	"PLUGIN_SOURCE_HANDLER"
#define PLUGIN_SOURCEHANDLER_GUID	"SOURCE_HANDLER_GUID"

#define SOURCEHANDLER_TYPE "SourceHandlerType"

//source handler type of DRM plugin
#define SOURCEHANDLER_DRM_TYPE	"SOURCE_HANDLER_DRM"

//file header DRM name values
#define HXDRM_NAME_ISPROTECTED	"IsLicensed"
#define HXDRM_NAME_DRMID	"DRMId"
#define HXDRM_NAME_DRMGUID	"DRMGUID"

//mime type suffix of encrypted content
#define ENCRYPTED_MIME_TYPE_SUFFIX "-encrypted"

//DRMId short names

#define HXDRM_ID_REALDRM "RNBA" /* private DRM ID */
#define HXDRM_ID_WMDRM   "WMRM"


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMLicenseStore
 * 
 *  Purpose:
 * 
 *	Interface supplied by DRM plugin to delete all unusable licenses from 
 *	the license store and performs maintenance functions on the data store file.
 * 
 *	{CA914140-3B63-4f69-BE5A-91432CB3BA45}
 * 
 */
DEFINE_GUID(IID_IHXDRMLicenseStore, 
0xca914140, 0x3b63, 0x4f69, 0xbe, 0x5a, 0x91, 0x43, 0x2c, 0xb3, 0xba, 0x45);
#undef  INTERFACE
#define INTERFACE   IHXDRMLicenseStore

DECLARE_INTERFACE_(IHXDRMLicenseStore , IUnknown)
{
    STDMETHOD(PurgeStaleLicenses)	(THIS_ 
				void* pUserData) PURE;	
};
	
/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMStatus
 * 
 *  Purpose:
 * 
 *	Interface supplied by DRM plugin to manage playback of content on local machine
 * 
 *	{3985BB95-42CB-4c30-BA52-A2BF7FD85724}
 * 
 */

DEFINE_GUID(IID_IHXDRMStatus, 0x3985bb95, 0x42cb, 0x4c30, 0xba, 0x52, 0xa2, 
			0xbf, 0x7f, 0xd8, 0x57, 0x24);


#undef  INTERFACE
#define INTERFACE   IHXDRMStatus

DECLARE_INTERFACE_(IHXDRMStatus, IUnknown)
{

    /************************************************************************
     *	Method:
     *	    IHXDRMStatus::CanPlay
     *	Purpose:
     *	    This method is called to determine if a valid license resides upon 
     *		this machine for the current content.
     */
    STDMETHOD(CanPlay)			(THIS) PURE;


    /************************************************************************
     *	Method:
     *	    IHXDRMStatus::GetContentProperties
     *	Purpose:
     *	    request content Info and DRM specific extra information
     *      ContentInfo     Buffer
     */
    STDMETHOD(GetContentProperties)  (THIS_
			              REF(IHXValues*) pProperties) PURE;


    /************************************************************************
     *	Method:
     *	    IHXDRMStatus::IsComponentRevoked
     *	Purpose:
     *	    This method is called to verify if a given component has been revoked
     *		by the DRM system
     *	    
     *	Return values:
     *	    TRUE if the given component is revoked
     *	    FALSE if the component is not revoked.
     */
    STDMETHOD_(HXBOOL, IsComponentRevoked) (THIS_ 
			                   IHXBuffer* /*IN*/ pComponentID) PURE;
};



 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMQueryResponse
 * 
 *  Purpose:
 * 
 *	Interface to receive DRM and license query result
 * 
 * 
 *	{0E416CEA-07A2-4f75-A235-9F0DDB283C7B}
 * 
 */

DEFINE_GUID(IID_IHXDRMQueryResponse, 0xe416cea, 0x7a2, 0x4f75, 0xa2, 0x35, 0x9f, 
			0xd, 0xdb, 0x28, 0x3c, 0x7b);

#undef  INTERFACE
#define INTERFACE   IHXDRMQueryResponse

DECLARE_INTERFACE_(IHXDRMQueryResponse, IUnknown)
{

    /************************************************************************
     *	Method:
     *	    IHXDRMQueryResponse::SetDRMInfo
     *	Purpose:
     *	    response to IHXDRMQuery::GetDRMInfo
     *	    
     */
    STDMETHOD(DRMInfoReady)	(THIS_ 
				IHXBuffer* pDRMInfo) PURE;


    /************************************************************************
     *	Method:
     *	    IHXDRMQueryResponse::SetLicenseInfo
     *	Purpose:
     *	    response to IHXDRMQuery::GetLicenseInfo and IHXDRMQuery::GetALLLicenseInfo
     *			
     *	        pContentInfo:  ContentInfo buffer from GetLicenseInfo
     *		pLicenseInfo:  the license info
     *		bLastResponse: whether this is the last license response call
     *
     */

    STDMETHOD(LicenseInfoReady)	    (THIS_
				    IHXBuffer*	pContentInfo, 
				    IHXValues*	pLicenseInfo,
				    HXBOOL	bLastResponse) PURE;		    
};


 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMQuery
 * 
 *  Purpose:
 * 
 *	Interface to query DRM and license information
 * 
 * 
 *	{6986F713-017B-45eb-8AAE-5CE93C8F615C}
 * 
 */

DEFINE_GUID(IID_IHXDRMQuery, 0x6986f713, 0x17b, 0x45eb, 0x8a, 0xae, 0x5c, 
			0xe9, 0x3c, 0x8f, 0x61, 0x5c);

#undef  INTERFACE
#define INTERFACE   IHXDRMQuery

DECLARE_INTERFACE_(IHXDRMQuery, IUnknown)
{

    /************************************************************************
     *	Method:
     *	    IHXDRMQuery::GetDRMInfo
     *	Purpose:
     *	    Called to obtain a DRM-specific machine/instance ID string
	 *	
	 *  pQueryResponse: to receive the query result
     *	    
     */
    STDMETHOD(GetDRMInfo)	(THIS_ 
				IHXDRMQueryResponse* /*IN*/ pQueryResponse) PURE;



    /************************************************************************
     *	Method:
     *	    IHXDRMQuery::GetLicenseInfo
     *	Purpose:
     *	    This method is called to obtain a description of the rights for 
     *	    a particular piece of content.
	 *		returned rights name/value pairs are DRM-specific
	 *			
     *	    pContentInfo:  DRM-specific content Info. If NULL, will return all
	 *					   license info.
	 *		pQueryResponse: to receive the query result
     *
     */

    STDMETHOD(GetLicenseInfo)	    (THIS_
				    IHXBuffer* 		 /*IN*/	pContentInfo, 
				    IHXDRMQueryResponse* /*IN*/ pQueryResponse) PURE;
};


 
 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMResponse
 * 
 *  Purpose:
 * 
 *	General response interface for App-DRM interaction
 * 
 * 
 *	{638D3166-FD8A-42f0-B939-6A831BA5005D}
 * 
 */
DEFINE_GUID(IID_IHXDRMResponse, 0x638d3166, 0xfd8a, 0x42f0, 0xb9, 0x39, 
            0x6a, 0x83, 0x1b, 0xa5, 0x0, 0x5d);


#undef  INTERFACE
#define INTERFACE   IHXDRMResponse

DECLARE_INTERFACE_(IHXDRMResponse, IUnknown)
{

    /************************************************************************
     *	Method:
     *	    IHXDRMResponse::Complete
     *	Purpose:
     *	    response completion
     *	    
     */
    STDMETHOD(Complete)	(THIS_ 
                         HX_RESULT hResult,
                         IHXValues* pResponse) PURE;
};

 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMTrigger
 * 
 *  Purpose:
 * 
 *	Interface to send licensing/metering/etc triggers to the DRM plugin
 * 
 * 
 *	{25CE164E-3156-4a17-9C3B-AE8EFB108FF0}
 * 
 */
DEFINE_GUID(IID_IHXDRMTrigger, 0x25ce164e, 0x3156, 0x4a17, 0x9c, 0x3b, 0xae, 
			0x8e, 0xfb, 0x10, 0x8f, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXDRMTrigger

DECLARE_INTERFACE_(IHXDRMTrigger, IUnknown)
{

    /************************************************************************
     *	Method:
     *	    IHXDRMTrigger::ProcessTrigger
     *	Purpose:
     *	    Send the trigger to the DRM plugin. Supported trigger type/format is DRM-specific
     *		Trigger processing result is presented on the optional IHXDRMTriggerResponse 
     *		implemented by the client.
     *
     *	    pTrigger :		DRM-specific trigger object
     *	    pResponse:	        (optionally) to receive the trigger processing result
     */
    STDMETHOD(ProcessTrigger)	(THIS_ 
				IHXBuffer*	 /*IN*/	pTrigger,
				IHXDRMResponse*  /*IN*/	pResponse) PURE;
};

 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMHTTPMetering
 * 
 *  Purpose: (WMDRM Specific)
 * 
 *	Interface to collect device metering data for a specific metering ID and 
 *	to clear device metering data and reset the metering store for the metering IDs
 *	specified in the metering response
 * 
 * 
 *	{337034B0-BBD4-4990-B499-C0E57F87AB8B}
 * 
 */
DEFINE_GUID(IID_IHXDRMHTTPMetering, 
0x337034b0, 0xbbd4, 0x4990, 0xb4, 0x99, 0xc0, 0xe5, 0x7f, 0x87, 0xab, 0x8b);

#undef  INTERFACE
#define INTERFACE   IHXDRMHTTPMetering

DECLARE_INTERFACE_(IHXDRMHTTPMetering, IUnknown)
{

    STDMETHOD(GetMeteringReport)	(THIS_ 
				IHXBuffer*	 /*IN*/	pMeteringCert,
				REF(IHXValues*) /*OUT*/ pMeteringReport) PURE;

    STDMETHOD(SetMeteringReportAck)	(THIS_ 
				IHXBuffer*	 /*IN*/	pMeterReportACK,
				REF(INT32) fFlagsOut) PURE;	
};

 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMHTTPSecureClock
 * 
 *  Purpose:(WMDRM Specific)
 * 
 *	Interface to create secure clock challenge, which is sent over the Internet directly from a device or by a proxy application on a computer and 
 *	to processes the response received from the clock service, verifying and storing the clock packet, and to set the clock time
 * 
 * 
 *	{A4B6B36E-3E27-4d25-B79A-2DBBC239642C}
 * 
 */
 
DEFINE_GUID(IID_IHXDRMHTTPSecureClock, 
0xa4b6b36e, 0x3e27, 0x4d25, 0xb7, 0x9a, 0x2d, 0xbb, 0xc2, 0x39, 0x64, 0x2c);

#undef  INTERFACE
#define INTERFACE   IHXDRMHTTPSecureClock

DECLARE_INTERFACE_(IHXDRMHTTPSecureClock, IUnknown)
{

    STDMETHOD(GetSecureClockRequest)	(THIS_ 
				IHXValues* pSecureClock) PURE;

    STDMETHOD(SetSecureClockResponse)	(THIS_ 
				IHXBuffer* pClockResponse,
				REF(INT32) fFlagsOut) PURE;	
};

 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMHTTPLicensing
 * 
 *  Purpose:
 * 
 *	Interface for client managed licensing through HTTP
 * 
 * 
 *	{644234EA-CE3C-4060-B0F7-6986E5142490}
 * 
 */
DEFINE_GUID(IID_IHXDRMHTTPLicensing, 0x644234ea, 0xce3c, 0x4060, 0xb0, 0xf7, 0x69, 
                        0x86, 0xe5, 0x14, 0x24, 0x90);

#undef  INTERFACE
#define INTERFACE   IHXDRMHTTPLicensing

DECLARE_INTERFACE_(IHXDRMHTTPLicensing, IUnknown)
{

    /************************************************************************
     *	Method:
     *	    IHXDRMHTTPLicensing::GetLicenseRequest
     *	Purpose:
     *	    ask DRM to create a license request for a particular content
     *
     *	    pContentInfo :	DRM specific content information
     *      pResponse:   :      to receive the license request:
     *                          LicenseRequestURL          CString     URL of the license request
     *                          SilentLicenseRequestURL    CString     URL of the silent license request
     *                          UseHTTPPost                ULONG32     1 = use HTTP POST, instead of GET
     *                          MimeType                   CString     Mime type used for HTTP POST
     *                          LicenseRequestData         Buffer      license request data for HTTP POST
     */
    STDMETHOD(GetLicenseRequest)    (THIS_ 
                                     IHXBuffer* pContentInfo,
				     IHXDRMResponse* pResponse) PURE;

    /************************************************************************
     *	Method:
     *	    IHXDRMHTTPLicensing::SetLicenseResponse
     *	Purpose:
     *	    send the license response from the server to DRM
     *
     *	    pszMimeType      :	license response data mime type
     *      pLicenseResponse :  license response data
     *      pResponse        :  to receive the license insertion result
     */
    STDMETHOD(SetLicenseResponse)    (THIS_ 
                                      const char* pszMimeType,
                                      IHXBuffer* pLicenseResponse,
				      IHXDRMResponse* pResponse) PURE;
};


 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMAdviseSink
 * 
 *  Purpose:
 * 
 *	Interface for notifying client of DRM events
 * 
 * 
 *	{3F3BCFCA-CC10-4f84-943E-B03D284AF515}
 * 
 */
DEFINE_GUID(IID_IHXDRMAdviseSink, 0x3f3bcfca, 0xcc10, 0x4f84, 0x94, 0x3e, 0xb0, 
            0x3d, 0x28, 0x4a, 0xf5, 0x15);


#undef  INTERFACE
#define INTERFACE   IHXDRMAdviseSink

DECLARE_INTERFACE_(IHXDRMAdviseSink, IUnknown)
{

    /************************************************************************
     *	Method:
     *	    IHXDRMAdviseSink::ContentInfoReady
     *	Purpose:
     *	    inform client that the content information collected from content header is
     *      ready to be queried.
     *
     *	    pDRM :	the DRM object 
     *  
     *  Note: 
     *      the pDRM object is only guaranteed to be valid within the context of
     *      ContentInfoReady call.
     *      
     */
    STDMETHOD(ContentInfoReady)    (THIS_ IUnknown* pDRM) PURE;
};

 /****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXDRMPreRecordingHook
 * 
 *  Purpose:
 * 
 *	Interface for DRM plugin to modify headers before recording
 * 
 * 
 *	{A0AE7A03-855E-4ce5-9445-D885C6E962AD}
 * 
 */

// 
DEFINE_GUID(IID_IHXDRMPreRecordingHook,  0xa0ae7a03, 0x855e, 0x4ce5, 0x94, 0x45, 0xd8, 
            0x85, 0xc6, 0xe9, 0x62, 0xad);

#undef  INTERFACE
#define INTERFACE   IHXDRMPreRecordingHook

DECLARE_INTERFACE_(IHXDRMPreRecordingHook, IUnknown)
{

    /************************************************************************
     *	Method:
     *	    IHXDRMPreRecordingHook::FileHeaderHook
     *	Purpose:
     *	    give DRM the chance to modify the file header before recording
     *      
     */
    STDMETHOD(FileHeaderHook)  (THIS_ IHXValues* pFileHeader) PURE;

    /************************************************************************
     *	Method:
     *	    IHXDRMPreRecordingHook::StreamHeaderHook
     *	Purpose:
     *	    give DRM the chance to modify the stream header before recording
     *      
     */
    STDMETHOD(StreamHeaderHook)  (THIS_ IHXValues* pStreamHeader) PURE;
};

#endif /* _HXDRM_H_ */

