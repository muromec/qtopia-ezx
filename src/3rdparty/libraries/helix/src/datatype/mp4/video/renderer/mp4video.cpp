/****************************************************************************
* 
*  QuickTime Renderer for RealMedia Architecture.
*
*/

/****************************************************************************
 *  Defines
 */
// #define ASYNC_RESIZE_OK
// #define ENABLE_TRACE

#define MP4_VIDEO_RENDERER_NAME	"MPEG-4 Video"


/****************************************************************************
 *  Includes
 */
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "mp4xrend.ver"

#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxevent.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxcore.h"
#include "hxrendr.h"
#include "hxhyper.h"
#include "hxplugn.h"
#include "hxasm.h"
#include "hxupgrd.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxerror.h"
#include "hxwin.h"
#include "hxvsurf.h"
#include "hxvctrl.h"
#include "hxsite2.h"
#include "hxthread.h"
#include "hxmon.h"
#include "hxformt.h"

#include "mp4video.h"
#include "mp4vdfmt.h"

#include "dllpath.h"
#include "hxheap.h"

#ifndef WIDTHBYTES
#define WIDTHBYTES(i)	((ULONG32)((i+31)&(~31))/8)	/* ULONG aligned ! */
#endif

/************************************************************************
 *  CMP4VideoRenderer class
 */
/************************************************************************
 *  Constants
 */
const char* const CMP4VideoRenderer::zm_pDescription    = "RealNetworks MPEG-4 Video Renderer Plugin";

const char* const CMP4VideoRenderer::zm_pStreamMimeTypes[] =
{
    "video/MP4V-ES",
    "video/X-RN-MP4",
    "video/X-HX-AVC1",
    "video/X-HX-DIVX",
    "video/H264",
	#ifdef HELIX_FEATURE_VIDEO_CODEC_VP6
	"video/x-hx-flv",
	#endif
    NULL
};

/************************************************************************
 *  Constructor/Destructor
 */
CMP4VideoRenderer::CMP4VideoRenderer(void)
    : m_pOutputAllocator(NULL)
    , m_pMP4VideoFormat(NULL)
{
    ;
}

CMP4VideoRenderer::~CMP4VideoRenderer()
{
    if (m_pActiveVideoPacket)
    {
	m_pActiveVideoPacket->Clear();
	delete m_pActiveVideoPacket;
	m_pActiveVideoPacket = NULL;
    }

    HX_DELETE(m_pOutputAllocator);

    HX_RELEASE(m_pMP4VideoFormat);
}

STDMETHODIMP CMP4VideoRenderer::EndStream()
{
    if (m_pActiveVideoPacket)
    {
        m_pActiveVideoPacket->Clear();
        delete m_pActiveVideoPacket;
        m_pActiveVideoPacket = NULL;
    }
    return CVideoRenderer::EndStream();
}

HX_RESULT STDAPICALLTYPE CMP4VideoRenderer::HXCreateInstance(IUnknown** ppIUnknown)
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    *ppIUnknown = (IUnknown*)(IHXPlugin*) new CMP4VideoRenderer();
    if (*ppIUnknown)
    {
        (*ppIUnknown)->AddRef();
        retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT STDAPICALLTYPE CMP4VideoRenderer::CanUnload(void)
{
    return CanUnload2();
}

HX_RESULT STDAPICALLTYPE CMP4VideoRenderer::CanUnload2(void)
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}


/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0 and Exhibits. 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM 
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. 
 * All Rights Reserved. 
 * 
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Community Source 
 * License Version 1.0 (the "RCSL"), including Attachments A though H, 
 * all available at http://www.helixcommunity.org/content/rcsl. 
 * You may also obtain the license terms directly from RealNetworks. 
 * You may not use this file except in compliance with the RCSL and 
 * its Attachments. There are no redistribution rights for the source 
 * code of this file. Please see the applicable RCSL for the rights, 
 * obligations and limitations governing use of the contents of the file. 
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
 * https://rarvcode-tck.helixcommunity.org 
 * 
 * Contributor(s): 
 * 
 * ***** END LICENSE BLOCK ***** */ 


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
STDMETHODIMP CMP4VideoRenderer::GetPluginInfo
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
STDMETHODIMP CMP4VideoRenderer::GetRendererInfo
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
CVideoFormat* CMP4VideoRenderer::CreateFormatObject(IHXValues* pHeader)
{
    m_pMP4VideoFormat = new CMP4VideoFormat(m_pCommonClassFactory, this);
    HX_ADDREF(m_pMP4VideoFormat);

    return m_pMP4VideoFormat;
}


/****************************************************************************
 *  GetRendererName
 */
const char* CMP4VideoRenderer::GetRendererName(void)
{
    return MP4_VIDEO_RENDERER_NAME;
}


/****************************************************************************
 *  SetupBitmapDefaults
 */
void CMP4VideoRenderer::SetupBitmapDefaults(IHXValues* pHeader,
                                            HXBitmapInfoHeader &bitmapInfoHeader)
{
    // Call the base video renderer bitmap defaults
    CVideoRenderer::SetupBitmapDefaults(pHeader, bitmapInfoHeader);
    // Now update any MP4V-specific defaults. We only need
    // to update the bitcount, compression, and image size.
    bitmapInfoHeader.biBitCount    = MP4V_PIXEL_SIZE;
    bitmapInfoHeader.biCompression = MP4V_PIXEL_FORMAT;
    bitmapInfoHeader.biSizeImage   = (bitmapInfoHeader.biWidth * 
				      bitmapInfoHeader.biHeight * 
				      bitmapInfoHeader.biBitCount + 7) / 8;
}

HX_RESULT CMP4VideoRenderer::UntimedModeNotice(HXBOOL bUntimedRendering)
{
    if (m_pMP4VideoFormat)
    {
       // Propagate CPUScalability down to the decoder
       m_pMP4VideoFormat->SetCPUScalability(!bUntimedRendering);
    }

    return HXR_OK;
}

