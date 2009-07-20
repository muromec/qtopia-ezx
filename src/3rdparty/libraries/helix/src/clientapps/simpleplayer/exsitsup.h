/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _EXSITSUP_H_
#define _EXSITSUP_H_

/****************************************************************************
 * 
 *  Class:
 *
 *	ExampleSiteSupplier
 *
 *  Purpose:
 *
 *	Implementation for ragui's IHXSiteSupplier
 *
 */
class ExampleSiteSupplier : 
	public IHXSiteSupplier
	, public IHXSiteCaptureResponse
{
private:
    LONG32		    m_lRefCount;
    IHXSiteManager*	    m_pSiteManager;
    IHXSite*		    m_pSite;
    IHXSiteCapture*	    m_pSiteCapture;
    IHXBuffer*		    m_pCaptureBuffer;
    IHXCommonClassFactory*  m_pCCF;
    IUnknown*		    m_pUnkPlayer;
    FiveMinuteMap	    m_CreatedSites;
    CHXString		    m_strFileName;

    ~ExampleSiteSupplier();

public:
    ExampleSiteSupplier(IUnknown* pUnkPlayer);
    
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXSiteSupplier methods
     */

    /************************************************************************
     *	Method:
     *	    IHXSiteSupplier::SitesNeeded
     *	Purpose:
     *	  Called to inform the site supplier that a site with a particular
     *	  set of characteristics is needed. If the site supplier can 
     *	  fulfill the request it should call the site manager and add one
     *	  or more new sites.
     *    Note that the request for sites is associated with a Request ID
     *    the client core will inform the site supplier when this requested
     *    site is no longer needed.
     */
    STDMETHOD(SitesNeeded)	(THIS_
				UINT32			uRequestID,
				IHXValues*		pSiteProps);

    /************************************************************************
     *  Method:
     *    IHXSiteSupplier::SitesNotNeeded
     *  Purpose:
     *    Called to inform the site supplier that all sites from a previos
     *	  site request are no longer needed. If the site supplier had 
     *	  previously created non-persistant sites (like popup windows)
     *    to fulfill a request for sites it should call the site manager 
     *    and remove those sites.
     */
    STDMETHOD(SitesNotNeeded)
				(THIS_
				UINT32			uRequestID);


    /************************************************************************
     *  Method:
     *    IHXSiteSupplier::BeginChangeLayout
     *  Purpose:
     *    Called to inform the site supplier a layout change has beginning
     *	  it can expect to recieve SitesNeeded() and SitesNotNeeded() calls
     *	  while a layout change is in progress,
     */
    STDMETHOD(BeginChangeLayout) (THIS);

    /************************************************************************
     *  Method:
     *    IHXSiteSupplier::DoneChangeLayout
     *  Purpose:
     *    Called to inform the site supplier the layout change has been
     *	  completed.
     */
    STDMETHOD(DoneChangeLayout) (THIS);

    /*
     * IHXSiteCaptureResponse
     */
    STDMETHOD(CaptureDone)	(THIS_
				REF(HX_RESULT) 		 status,
                        	REF(HXBitmapInfoHeader)  bmiOutputFormat,
                        	REF(IHXBuffer*)          pImageDataBuffer
				);

    HX_RESULT CaptureImage(CHXString pFileName, INT32 dWidth, INT32 dHeight);

};

#endif // _EXSITSUP_H_

