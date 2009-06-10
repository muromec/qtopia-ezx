/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h263video.cpp,v 1.11 2007/07/06 22:00:34 jfinnecy Exp $
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

/****************************************************************************
 *  Defines
 */
// #define ASYNC_RESIZE_OK
// #define ENABLE_TRACE

#define H263_VIDEO_RENDERER_NAME	"H263 Video"
#define H263_CODEC_FOURCC		"H263"


/****************************************************************************
 *  Includes
 */

#include "h263rend.ver"

#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxevent.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxcore.h"
#include "hxrendr.h"
// #include "hxhyper.h"
#include "hxplugn.h"
#include "hxasm.h"
#include "hxupgrd.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxerror.h"
#include "hxwin.h"
#include "hxvsurf.h"
// #include "hxvctrl.h"
#include "hxsite2.h"
#include "hxthread.h"
#include "hxmon.h"
#include "hxformt.h"

#include "h263video.h"
#include "h263vidfmt.h"

#include "hxheap.h"

#ifndef WIDTHBYTES
#define WIDTHBYTES(i)	((ULONG32)((i+31)&(~31))/8)	/* ULONG aligned ! */
#endif

/************************************************************************
 *  CH263VideoRenderer class
 */
/************************************************************************
 *  Constants
 */
const char* const CH263VideoRenderer::zm_pDescription    = "RealNetworks H263 Video Renderer Plugin";

const char* const CH263VideoRenderer::zm_pStreamMimeTypes[] =
{
    "video/H263-2000",
    "video/h263-1998",
    "video/h263",
    "video/X-RN-3GPP-H263",
    NULL
};


/************************************************************************
 *  Constructor/Destructor
 */
CH263VideoRenderer::CH263VideoRenderer(void)
 : CVideoRenderer(),
   CHXBaseCountingObject()
{
    ;
}

CH263VideoRenderer::~CH263VideoRenderer()
{    
    ;
}

HX_RESULT STDAPICALLTYPE CH263VideoRenderer::HXCreateInstance(IUnknown** ppIUnknown)
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    *ppIUnknown = (IUnknown*)(IHXPlugin*) new CH263VideoRenderer();
    if (*ppIUnknown)
    {
        (*ppIUnknown)->AddRef();
        retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT STDAPICALLTYPE CH263VideoRenderer::CanUnload(void)
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}

HX_RESULT STDAPICALLTYPE CH263VideoRenderer::CanUnload2(void)
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}


/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP CH263VideoRenderer::GetPluginInfo
(
   REF(HXBOOL)        /*OUT*/ bLoadMultiple,
   REF(const char*) /*OUT*/ pDescription,
   REF(const char*) /*OUT*/ pCopyright,
   REF(const char*) /*OUT*/ pMoreInfoURL,
   REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    HX_RESULT retVal;

    retVal = CVideoRenderer::GetPluginInfo(bLoadMultiple,
					   pDescription,
					   pCopyright,
					   pMoreInfoURL,
					   ulVersionNumber);

    if (SUCCEEDED(retVal))
    {
	pDescription    = (const char*) zm_pDescription;
	ulVersionNumber = TARVER_ULONG32_VERSION;
    }
    
    return retVal;
}


/************************************************************************
 *  Method:
 *    IHXRenderer::GetRendererInfo
 *  Purpose:
 *    If this object is a file format object this method returns
 *    information vital to the instantiation of file format plugins.
 *    If this object is not a file format object, it should return
 *    HXR_UNEXPECTED.
 */
STDMETHODIMP CH263VideoRenderer::GetRendererInfo
(
    REF(const char**) /*OUT*/ pStreamMimeTypes,
    REF(UINT32)       /*OUT*/ unInitialGranularity
)
{
    HX_RESULT retVal;

    retVal = CVideoRenderer::GetRendererInfo(pStreamMimeTypes,
					     unInitialGranularity);

    if (SUCCEEDED(retVal))
    {
	pStreamMimeTypes = (const char**) zm_pStreamMimeTypes;
    }
    
    return retVal;
}


/****************************************************************************
 *  CreateFormatObject
 */
CVideoFormat* CH263VideoRenderer::CreateFormatObject(IHXValues* pHeader)
{
    return new CH263VideoFormat(m_pCommonClassFactory, this);
}


/****************************************************************************
 *  GetRendererName
 */
const char* CH263VideoRenderer::GetRendererName(void)
{
    return H263_VIDEO_RENDERER_NAME;
}


/****************************************************************************
 *  GetCodecFourCC
 */
const char* CH263VideoRenderer::GetCodecFourCC(void)
{
    return H263_CODEC_FOURCC;
}


/****************************************************************************
 *  SetupBitmapDefaults
 */
void CH263VideoRenderer::SetupBitmapDefaults(IHXValues* pHeader,
                                             HXBitmapInfoHeader &bitmapInfoHeader)
{
    // Call the base video renderer bitmap defaults
    CVideoRenderer::SetupBitmapDefaults(pHeader, bitmapInfoHeader);
    // Now update any H.263-specific defaults. We only need
    // to update the bitcount, compression, and image size.
    bitmapInfoHeader.biBitCount    = H263_PIXEL_SIZE;
    bitmapInfoHeader.biCompression = H263_PIXEL_FORMAT;
    bitmapInfoHeader.biSizeImage   = (bitmapInfoHeader.biWidth * 
                                      bitmapInfoHeader.biHeight * 
                                      bitmapInfoHeader.biBitCount + 7) / 8;
}
