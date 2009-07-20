/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: prxallow.h,v 1.2 2003/01/23 23:42:59 damonlan Exp $ 
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

#ifndef _PRXALLOW_H_
#define _PRXALLOW_H_

enum DestType
{
    URLForOrigin,
    URLForCache,
    URLForSplitter
};

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IHXValues			    IHXValues;
typedef _INTERFACE  IHXBuffer			    IHXBuffer;
typedef _INTERFACE  IHXRequest			    IHXRequest;
typedef _INTERFACE  IHXProxyConnectionAdviseSink   IHXProxyConnectionAdviseSink;
typedef _INTERFACE  IHXProxyConnectionResponse     IHXProxyConnectionResponse;
typedef _INTERFACE  IHXProxyController             IHXProxyController;

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXProxyConnectionAdviseSink
 * 
 *  Purpose:
 *
 *      Advise Sink which receives notification whenever a new client 
 *      connects to the proxy.
 * 
 *  IID_IHXProxyConnectionAdviseSink:
 * 
 *      {00002600-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXProxyConnectionAdviseSink, 0x00002604, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXProxyConnectionAdviseSink

DECLARE_INTERFACE_(IHXProxyConnectionAdviseSink, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    /* OnConnection is called when a new player has connected to the
     * proxy.  If the result is HXR_OK, then the plugin will be notified
     * when certain events occur in the player's life cycle.
     */
    STDMETHOD(OnConnection) (THIS_ 
				IHXProxyConnectionResponse* pResponse) PURE;


    /* SetPlayerController is called by the proxy to provide us with
     * an interface which can stop, alert, redirect or otherwise control
     * the player we are receiving notifications about.
     */
    STDMETHOD(SetProxyController)  (THIS_
				IHXProxyController* pProxyController) PURE;


    /* SetRegistryID is called by the proxy to provide us with the
     * ID for this Player in the server registry. The plugin can use this
     * registry ID to find out various information about the connected player.
     */
    STDMETHOD(SetRegistryID)        (THIS_ UINT32 ulProxyRegistryID) PURE;


    /* OnPartialURL is called when the RTSP proxy receives an OPTIONS request
     * with a partial URL like "rtsp://host:port". The proxy does not have a
     * full URL and it does not know the actual origin server host:port until
     * The allowance plugin calls OnURLDone(). 
     */
	    
    STDMETHOD(OnPartialURL)                (THIS_ IHXRequest* pRequest) PURE;

    /* OnURL is called when the proxy begins processing a URL. The destType
     * argument indicates what URL this is. If DestType is :
     *
     *          URLForOrigin   -> the client just requested a new URL
     *
     *          URLForCache    -> the content is on-demand and the proxy is 
     *                            about to ask the cache for this URL
     *
     *          URLForSplitter -> the content is live and the proxy is 
     *                            about to ask the splitter for this URL
     */
	    
    STDMETHOD(OnURL)                (THIS_ IHXRequest* pRequest,
                                            DestType    type) PURE;

    STDMETHOD(OnRedirect)           (THIS_ IHXBuffer* pLocation) PURE;
    STDMETHOD(OnBegin)              (THIS) PURE;
    STDMETHOD(OnStop)               (THIS) PURE;
    STDMETHOD(OnPause)              (THIS) PURE;
    STDMETHOD(OnDone)               (THIS) PURE;
};



/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXProxyConnectionResponse
 * 
 *  Purpose:
 *
 *      Response object for the ProxyConnectionAdviseSink.
 * 
 *  IID_IHXPlayerConnectionResponse:
 * 
 *      {00002601-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXProxyConnectionResponse, 0x00002605, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXProxyConnectionResponse

DECLARE_INTERFACE_(IHXProxyConnectionResponse, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    STDMETHOD(OnConnectionDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnPartialURLDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnURLDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnBeginDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnStopDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnPauseDone)		(THIS_ HX_RESULT status) PURE;
    STDMETHOD(OnRedirectDone)		(THIS_ HX_RESULT status) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXProxyController
 * 
 *  Purpose:
 *
 *      Object created by the server core and given to the 
 *      IHXPlayerConnectionResponse object so that the response object
 *      can control the connected player.
 * 
 *  IID_IHXProxyController:
 * 
 *      {00002602-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXProxyController, 0x00002606, 0x901, 0x11d1, 
	    0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE IHXProxyController

DECLARE_INTERFACE_(IHXProxyController, IUnknown)
{
    STDMETHOD(QueryInterface)           (THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;

    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    STDMETHOD(Disconnect)               (THIS) PURE;
    STDMETHOD(AlertAndDisconnect)       (THIS_ IHXBuffer* pAlert) PURE;

    /* ProxyRedirect is called by a ProxyConnectionAdviseSink to redirect
     * this client to another proxy. Note: This method is only available 
     * when the connected client supports the 305 Use Proxy.
     */
    STDMETHOD(ProxyRedirect)           (THIS_ IHXBuffer* pLocation,
					UINT32 ulSecsFromNow) PURE;

    STDMETHOD(SetParentProxy)          (THIS_ IHXBuffer* pLocation) PURE;
};



#endif /* _PRXALLOW_H_ */
