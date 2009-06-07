/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpac.h,v 1.3 2004/07/09 18:20:48 hubbe Exp $
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

#ifndef _HXPAC_H_
#define _HXPAC_H_

typedef _INTERFACE   IHXProxyAutoConfig	    IHXProxyAutoConfig;
typedef _INTERFACE   IHXProxyAutoConfigCallback    IHXProxyAutoConfigCallback;
typedef _INTERFACE   IHXProxyAutoConfigAdviseSink  IHXProxyAutoConfigAdviseSink;

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXProxyAutoConfig
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXProxyAutoConfig:
 * 
 *  {0x00004800-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXProxyAutoConfig,	    0x00004800, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXProxyAutoConfig

DECLARE_INTERFACE_(IHXProxyAutoConfig, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXProxyAutoConfig methods
     */
    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfig::Init
    *  Purpose:
    */
    STDMETHOD(Init)		    (THIS_
				    IUnknown* pContext) PURE; 

    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfig::GetHTTPProxyInfo
    *  Purpose:
    */
    STDMETHOD(GetHTTPProxyInfo)	    (THIS_
				    IHXProxyAutoConfigCallback* pCallback,					  
				    const char* pszURL,
				    const char* pszHost) PURE;

    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfig::GetRTSPPNMProxyInfo
    *  Purpose:
    */
    STDMETHOD(GetRTSPPNMProxyInfo)  (THIS_
				    IHXProxyAutoConfigCallback* pCallback,					  
				    const char* pszURL,
				    const char* pszHost) PURE;

    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfig::AbortProxyInfo
    *  Purpose:
    */
    STDMETHOD(AbortProxyInfo)	    (THIS_
				    IHXProxyAutoConfigCallback* pCallback) PURE;

    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfig::AddAdviseSink
    *  Purpose:
    */
    STDMETHOD(AddAdviseSink)	    (THIS_
				    IHXProxyAutoConfigAdviseSink* pSink) PURE;

    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfig::RemoveAdviseSink
    *  Purpose:
    */
    STDMETHOD(RemoveAdviseSink)	    (THIS_
				    IHXProxyAutoConfigAdviseSink* pSink) PURE;

    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfig::Close
    *  Purpose:
    */
    STDMETHOD(Close)		    (THIS) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXProxyAutoConfigCallback
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXProxyAutoConfigCallback:
 * 
 *  {0x00004801-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXProxyAutoConfigCallback,    0x00004801, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXProxyAutoConfigCallback

DECLARE_INTERFACE_(IHXProxyAutoConfigCallback, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXProxyAutoConfigCallback methods
     */
    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfigCallback::GetProxyInfoDone
    *  Purpose:
    */
    STDMETHOD(GetProxyInfoDone)	    (THIS_
				    HX_RESULT	status,
				    char*	pszProxyInfo) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *  IHXProxyAutoConfigAdviseSink
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXProxyAutoConfigAdviseSink:
 * 
 *  {0x00004802-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXProxyAutoConfigAdviseSink,    0x00004802, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXProxyAutoConfigAdviseSink

DECLARE_INTERFACE_(IHXProxyAutoConfigAdviseSink, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)       (THIS_
				    REFIID riid,
				    void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;

    STDMETHOD_(ULONG32,Release)     (THIS) PURE;

    /*
     *  IHXProxyAutoConfigAdviseSink methods
     */
    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfigAdviseSink::OnWPADBegin
    *  Purpose:
    */
    STDMETHOD(OnWPADBegin)	    (THIS) PURE;

    /************************************************************************
    *  Method:
    *      IHXProxyAutoConfigAdviseSink::OnWPADEnd
    *  Purpose:
    */
    STDMETHOD(OnWPADEnd)	    (THIS_
				    HX_RESULT ulStatus) PURE;
};
#endif /* _HXPAC_H_ */
