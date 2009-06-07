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

#ifdef _WIN16
#include <windows.h>
#endif

#include "hxcom.h"
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxwin.h"
#include "ihxpckts.h"
#include "hxcomm.h"
#include "fivemmap.h"
#include "hxstring.h" // CHXString
#include "exsitsup.h"
#include "pckunpck.h" // CreateBufferCCF
#include "ciddefs.h"  // CID_RGB32
#if defined(HELIX_FEATURE_PNG)
#include "pxpngenc.h" // PXPNGEncode::EncodeToPNGBuffer
#endif //  HELIX_FEATURE_PNG
#include "print.h"


#include "globals.h"
struct _stGlobals*& GetGlobal();

/************************************************************************
 *  Method:
 *    Constructor
 */
ExampleSiteSupplier::ExampleSiteSupplier(IUnknown* pUnkPlayer)
    : m_lRefCount(0)
    , m_pSiteManager(NULL)
    , m_pSite(NULL)
    , m_pSiteCapture(NULL)
    , m_pCaptureBuffer(NULL)
    , m_pCCF(NULL)
    , m_pUnkPlayer(pUnkPlayer)
{
    if (m_pUnkPlayer)
    {
	m_pUnkPlayer->QueryInterface(IID_IHXSiteManager,
			(void**)&m_pSiteManager);

	m_pUnkPlayer->QueryInterface(IID_IHXCommonClassFactory,
			(void**)&m_pCCF);

	m_pUnkPlayer->AddRef();
    }
};

/************************************************************************
 *  Method:
 *    Destructor
 */
ExampleSiteSupplier::~ExampleSiteSupplier()
{
    HX_RELEASE(m_pSiteManager);
    HX_RELEASE(m_pSite);
    HX_RELEASE(m_pSiteCapture);
    HX_RELEASE(m_pCaptureBuffer);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pUnkPlayer);
}

/************************************************************************
 *  Method:
 *    IUnknown::QueryInterface
 */
STDMETHODIMP 
ExampleSiteSupplier::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXSiteSupplier*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXSiteSupplier))
    {
	AddRef();
	*ppvObj = (IHXSiteSupplier*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/************************************************************************
 *  Method:
 *    IUnknown::AddRef
 */
STDMETHODIMP_(ULONG32) 
ExampleSiteSupplier::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *    IUnknown::Release
 */
STDMETHODIMP_(ULONG32) 
ExampleSiteSupplier::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *    IHXSiteSupplier::SitesNeeded
 *  Purpose:
 *    Called to inform the site supplier that a site with a particular
 *    set of characteristics is needed. If the site supplier can 
 *    fulfill the request it should call the site manager and add one
 *    or more new sites.
 *    Note that the request for sites is associated with a Request ID
 *    the client core will inform the site supplier when this requested
 *    site is no longer needed.
 */
STDMETHODIMP 
ExampleSiteSupplier::SitesNeeded
(
    UINT32	uRequestID,
    IHXValues*	pProps
)
{
    /*
     * Don't create a site if the -NULL_RENDER command line option
     * was given. - jfarr
     */
    if (GetGlobal()->g_bNullRender)
    {
	return (HXR_OK);
    }
    
    /*
     * If there are no properties, then we can't really create a
     * site, because we have no idea what type of site is desired!
     */
    if (!pProps)
    {
	return HXR_INVALID_PARAMETER;
    }

    HRESULT		hres		= HXR_OK;
    IHXValues*		pSiteProps	= NULL;
    IHXSiteWindowed*	pSiteWindowed	= NULL;
    IHXBuffer*		pValue		= NULL;
    UINT32		style		= 0;
    IHXSite*		pSite		= NULL;

    // Just let the RMA client core create a windowed site for us.
    hres = m_pCCF->CreateInstance(CLSID_IHXSiteWindowed,(void**)&pSiteWindowed);
    if (HXR_OK != hres)
    {
	goto exit;
    }

    hres = pSiteWindowed->QueryInterface(IID_IHXSite,(void**)&pSite);
    if (HXR_OK != hres)
    {
	goto exit;
    }
    
    if( !m_pSite )
    {
        m_pSite = pSite;
        m_pSite->AddRef();
    }
    

    hres = pSiteWindowed->QueryInterface(IID_IHXValues,(void**)&pSiteProps);
    if (HXR_OK != hres)
    {
	goto exit;
    }

    /*
     * We need to figure out what type of site we are supposed to
     * to create. We need to "switch" between site user and site
     * properties. So look for the well known site user properties
     * that are mapped onto sites...
     */
    hres = pProps->GetPropertyCString("playto",pValue);
    if (HXR_OK == hres)
    {
	pSiteProps->SetPropertyCString("channel",pValue);
	HX_RELEASE(pValue);
    }
    else
    {
	hres = pProps->GetPropertyCString("name",pValue);
	if (HXR_OK == hres)
	{
	    pSiteProps->SetPropertyCString("LayoutGroup",pValue);
    	    HX_RELEASE(pValue);
	}
    }

#ifdef _WINDOWS
    style = WS_OVERLAPPED | WS_VISIBLE | WS_CLIPCHILDREN;
#endif

    hres = pSiteWindowed->Create(NULL, style);
    if (HXR_OK != hres)
    {
	goto exit;
    }

    /*
     * We need to wait until we have set all the properties before
     * we add the site.
     */
    hres = m_pSiteManager->AddSite(pSite);
    if (HXR_OK != hres)
    {
	goto exit;
    }
#ifdef _WINDOWS
    {
       HXxWindow* pWindow = pSiteWindowed->GetWindow();
       if (pWindow && pWindow->window) ::SetForegroundWindow( (HWND)(pWindow->window) );
    }
#endif
    m_CreatedSites.SetAt((void*)uRequestID,pSite);
    pSite->AddRef();

exit:

    HX_RELEASE(pSiteProps);
    HX_RELEASE(pSiteWindowed);
    HX_RELEASE(pSite);

    return hres;
}

/************************************************************************
 *  Method:
 *    IHXSiteSupplier::SitesNotNeeded
 *  Purpose:
 *    Called to inform the site supplier that all sites from a previos
 *    site request are no longer needed. If the site supplier had 
 *    previously created non-persistant sites (like popup windows)
 *    to fulfill a request for sites it should call the site manager 
 *    and remove those sites.
 */
STDMETHODIMP 
ExampleSiteSupplier::SitesNotNeeded(UINT32 uRequestID)
{
    IHXSite*		pSite = NULL;
    IHXSiteWindowed*	pSiteWindowed = NULL;
    void*		pVoid = NULL;

    if (!m_CreatedSites.Lookup((void*)uRequestID,pVoid))
    {
	return HXR_INVALID_PARAMETER;
    }
    pSite = (IHXSite*)pVoid;

    m_pSiteManager->RemoveSite(pSite);

    // Need to actually do the work on destroying the window
    // and all that jazz.
    pSite->QueryInterface(IID_IHXSiteWindowed,(void**)&pSiteWindowed);

    pSiteWindowed->Destroy();

    // ref count = 2
    pSiteWindowed->Release();

    // ref count = 1; deleted from this object's view!
    pSite->Release();

    m_CreatedSites.RemoveKey((void*)uRequestID);

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSiteSupplier::BeginChangeLayout
 *  Purpose:
 *    Called to inform the site supplier a layout change has beginning
 *    it can expect to recieve SitesNeeded() and SitesNotNeeded() calls
 *    while a layout change is in progress,
 */
STDMETHODIMP 
ExampleSiteSupplier::BeginChangeLayout()
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXSiteSupplier::DoneChangeLayout
 *  Purpose:
 *    Called to inform the site supplier the layout change has been
 *    completed.
 */
STDMETHODIMP 
ExampleSiteSupplier::DoneChangeLayout()
{
    return HXR_OK;
}

HX_RESULT
ExampleSiteSupplier::CaptureImage(CHXString pszFileName, INT32 dWidth, INT32 dHeight)
{
    HX_RESULT res=HXR_FAIL;

    // Get the IHXSiteCapture object from the site
    if (m_pSiteCapture == NULL)
    {
	if (m_pSite && m_pSite->QueryInterface(IID_IHXSiteCapture, (void**)&m_pSiteCapture) == HXR_OK)
	{
	    m_pSiteCapture->AddRef();
	}
    }

    // Create the buffer that will receive the image data
    if (m_pCaptureBuffer == NULL)
    {
    	CreateBufferCCF(m_pCaptureBuffer, m_pCCF);
	if (!m_pCaptureBuffer)
	{
	    return HXR_OUTOFMEMORY;
	}
    }

    HXBOOL bCanCapture = FALSE;

    // Check if the site is available to capture an image
    if (m_pSiteCapture && (res = m_pSiteCapture->CanCapture(bCanCapture)) == HXR_OK && bCanCapture)
    {
	HX_RESULT res;
	HXxSize outputSize;
	outputSize.cx  = dWidth;
	outputSize.cy  = dHeight;
	m_strFileName = pszFileName;

	// Asynchronous call. CaptureDone() will be called with result.
	res = m_pSiteCapture->Capture((IHXSiteCaptureResponse*)this, m_pCaptureBuffer, &outputSize, CID_RGB32);
    }

    return res;

}

//
// IHXSiteCaptureResponse::CaptureDone
//
// Called when the site has captured the next frame.
// bmiOutputFormat points to image format description which
// is valid until the completion of CaptureDone.
// bmiOutputFormat can be different for every capture.
// pCaptureBuffer holds the image if supplied in
// Capture() method.  pCaptureBuffer is automatically
// resized if it has insufficient size to hold the image
// data.
//
// status may be:
//   HXR_FAIL  -- No capture was done. General Error. All data is invalid.
//   HXR_OK    -- Capture was done. Both variables are valid.
STDMETHODIMP
ExampleSiteSupplier::CaptureDone(REF(HX_RESULT) status,
			REF(HXBitmapInfoHeader) bmiOutputFormat,
			REF(IHXBuffer*) pCaptureBuffer)
{
    if (status == HXR_OK)
    {
#if defined(HELIX_FEATURE_PNG)
	// Convert capture buffer into PNG
	IHXBuffer* pConvertedBuffer=NULL;
	HX_RESULT res = HXR_OK;
	res = PXPNGEncode::EncodeToPNGBuffer(pCaptureBuffer, &bmiOutputFormat, m_pCCF, pConvertedBuffer);
	if (res == HXR_OK && !m_strFileName.IsEmpty())
	{
	    CHXString strTail = m_strFileName.Right(4);
	    if (strTail.CompareNoCase(".png") != 0)
	    {
		m_strFileName += ".png";
	    }

	    FILE* fp = fopen(m_strFileName, "wb");
	    if (fp)
	    {
	        fwrite(pConvertedBuffer->GetBuffer(), 1, pConvertedBuffer->GetSize(), fp);
	        fclose(fp);
		STDOUT("Captured image to PNG file: %s\n", (const char*)m_strFileName);
		STDOUT("File size: %ld\n", pConvertedBuffer->GetSize());
	    }
	    HX_RELEASE(pConvertedBuffer);
	}
#endif //  HELIX_FEATURE_PNG
    }

    return status;
}
