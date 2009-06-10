/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: altserv.h,v 1.2 2003/01/23 23:42:59 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _ALTSERV_H_
#define _ALTSERV_H_

struct IHXAlternateServerProxyResponse;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXAlternateServerProxy
 * 
 *  Purpose:
 *  
 *	Deals with alternate server, alternater proxy config entries
 * 
 *  IID_IHXAlternateServerProxy:
 * 
 *  IID_IHXAlternateServerProxy:	{00000403-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXAlternateServerProxy,	0x00000403, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXAlternateServerProxy

DECLARE_INTERFACE_(IHXAlternateServerProxy, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD (QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;


    /************************************************************************
     *	Method:
     *	    IHXAlternateServerProxy::Init
     *	Purpose:
     *	    Set response object
     */
    STDMETHOD(Init)		(THIS_
				 IHXAlternateServerProxyResponse* pResp) PURE;    

    /************************************************************************
     *	Method:
     *	    IHXAlternateServerProxy::ClearResponse
     *	Purpose:
     *	    Remove response object
     */
    STDMETHOD(ClearResponse)	(THIS_
				 IHXAlternateServerProxyResponse* pResp) PURE;    

    /************************************************************************
     *	Method:
     *	    IHXAlternateServerProxy::IsEnabled
     *	Purpose:
     *	    Returns alternate servers/proxies iff they exist
     *	IN: 
     *	    pURL is everying after the protocol part(e.g. "rtsp://")	
     */
    STDMETHOD_(BOOL,IsEnabled)	(THIS_
				 const char* pURL) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXAlternateServerProxy::GetAltServerProxy
     *	Purpose:
     *	    Returns alternate servers/proxies iff they exist
     *	IN: 
     *	    pURL is everying after the protocol part(e.g. "rtsp://")	
     */
    STDMETHOD(GetAltServerProxy)(THIS_
				 const char* pURL,
				 REF(IHXBuffer*) pAltServ,
				 REF(IHXBuffer*) pAltProx) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXAlternateServerProxy::GetAltServers
     *	Purpose:
     *	    Returns alternate servers iff they exist
     *	IN: 
     *	    pURL is everying after the protocol part(e.g. "rtsp://")	
     */
    STDMETHOD(GetAltServers)	(THIS_
				 const char* pURL, 
				 REF(IHXBuffer*) pAlt) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAlternateServerProxy::GetAltProxies
     *	Purpose:
     *	    Returns alternate proxies iff they exist	    
     *	IN: 
     *	    pURL is everying after the protocol part(e.g. "rtsp://")	
     */
    STDMETHOD(GetAltProxies)	(THIS_
				 const char* pURL, 
				 REF(IHXBuffer*) pAlt) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXAlternateServerProxyResponse
 * 
 *  Purpose:
 *  
 *	Response for IHXAlternateServerProxyResponse
 * 
 *  IID_IHXAlternateServerProxyResponse:
 * 
 *  IID_IHXAlternateServerProxyResponse:	{00000404-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXAlternateServerProxyResponse, 0x00000404, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXAlternateServerProxyResponse

#define HX_GET_ALTERNATES_FLAG(x, y) (x & y)
#define HX_SET_ALTERNATES_FLAG(x, y) (x = x | y)

typedef UINT8 HX_ALTERNATES_MOD_FLAG;

typedef enum _HX_ALTERNATE_TYPE
{
    HX_ALT_SERVER = 0x01,
    HX_ALT_PROXY  = 0x02,
    HX_ALT_DPATH  = 0x04
} HX_ALTERNATES_TYPE;


DECLARE_INTERFACE_(IHXAlternateServerProxyResponse, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD (QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXAlternateServerProxyResponse::OnModifiedEntry___
     *	Purpose:
     *	    When config of "type" is modified, this method will be called.
     */
    STDMETHOD(OnModifiedEntry)	    (THIS_
				     HX_ALTERNATES_MOD_FLAG type) PURE;    
};

#endif // _ALTSERV_H_

