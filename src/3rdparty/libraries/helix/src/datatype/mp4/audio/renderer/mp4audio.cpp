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

/****************************************************************************
 *  Defines
 */
#define MPEG4_AUDIO_RENDERER_NAME	"MPEG-4 Audio"
#define MPEG4_AUDIO_CODEC_4CC		"MP4A"
#define LOSSLESS_RENDERER_NAME          "RealAudio"

/****************************************************************************
 *  Includes
 */
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"

#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxresult.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxcore.h"
#include "hxrendr.h"
#include "hxplugn.h"
#include "hxasm.h"
#include "hxupgrd.h"
#include "hxengin.h"
#include "hxprefs.h"
#include "hxerror.h"
#include "hxwin.h"
#include "hxthread.h"
#include "hxmon.h"
#include "hxformt.h"
#include "hxacodec.h"

#include "mp4arender.ver"
#include "mp4audio.h"
#include "mp4afmt.h"

#include "dllpath.h"
#include "hxheap.h"

/************************************************************************
 *  CMP4AudioRenderer class
 */
/************************************************************************
 *  Constants
 */
const char* const CMP4AudioRenderer::zm_pDescription    = "RealNetworks MPEG-4 Audio Renderer Plugin";

const char* const CMP4AudioRenderer::zm_pStreamMimeTypes[] =
{
#if defined(HELIX_FEATURE_AUDIO_CODEC_QCELP)
    "audio/X-RN-3GPP-QCELP",
    "audio/QCELP",
#endif  // HELIX_FEATURE_AUDIO_CODEC_QCELP
#if defined(HELIX_FEATURE_AUDIO_RALF)
    "audio/x-ralf-mpeg4-generic",
#endif /* #if defined(HELIX_FEATURE_AUDIO_RALF) */
#if defined(HELIX_FEATURE_AUDIO_CODEC_AAC) || defined(HELIX_FEATURE_AUDIO_CODEC_RAAC)
    "audio/X-RN-MP4-RAWAU",
    "audio/MP4A-LATM",
    "audio/mpeg4-simple-A2",
    "audio/mpeg4-generic",
    "audio/X-HX-AAC-GENERIC",
#endif /* #if defined(HELIX_FEATURE_AUDIO_CODEC_AAC) || defined(HELIX_FEATURE_AUDIO_CODEC_RAAC) */
#if defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB)
    "audio/X-RN-3GPP-AMR",
    "audio/AMR",
#endif /* #if defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB) */
#if defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB)
    "audio/X-RN-3GPP-AMR-WB",
    "audio/AMR-WB",
#endif /* #if defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB) */
#if defined(HELIX_FEATURE_AUDIO_CODEC_MP3)
    "audio/X-MP3-draft-00",
    "audio/X-MP3-draft-00-RN",
    "audio/MPEG-ELEMENTARY",
#endif /* #if defined(HELIX_FEATURE_AUDIO_CODEC_MP3) */
    NULL
};


/************************************************************************
 *  Constructor/Destructor
 */
CMP4AudioRenderer::CMP4AudioRenderer(void)
{
}

CMP4AudioRenderer::~CMP4AudioRenderer()
{    
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
STDMETHODIMP CMP4AudioRenderer::GetPluginInfo
(
   REF(HXBOOL)        /*OUT*/ bLoadMultiple,
   REF(const char*) /*OUT*/ pDescription,
   REF(const char*) /*OUT*/ pCopyright,
   REF(const char*) /*OUT*/ pMoreInfoURL,
   REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    HX_RESULT retVal;

    retVal = CAudioRenderer::GetPluginInfo(bLoadMultiple,
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
STDMETHODIMP CMP4AudioRenderer::GetRendererInfo
(
    REF(const char**) /*OUT*/ pStreamMimeTypes,
    REF(UINT32)       /*OUT*/ unInitialGranularity
)
{
    HX_RESULT retVal;

    retVal = CAudioRenderer::GetRendererInfo(pStreamMimeTypes,
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
CAudioFormat* CMP4AudioRenderer::CreateFormatObject(IHXValues* pHeader)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_AUDIO_RALF)
    if (pHeader)
    {
        IHXBuffer* pMimeTypeStr = NULL;
        pHeader->GetPropertyCString("MimeType", pMimeTypeStr);
        if (pMimeTypeStr)
        {
            const char* pszMimeType = (const char*) pMimeTypeStr->GetBuffer();
            if (pszMimeType &&
                !strcmp(pszMimeType, "audio/x-ralf-mpeg4-generic"))
            {
                // We need to update the stats
                if (m_pAudioStats)
                {
                    m_pAudioStats->ReportStat(AS_REND_NAME, LOSSLESS_RENDERER_NAME);
                }
            }
        }
        HX_RELEASE(pMimeTypeStr);
    }
#endif /* #if defined(HELIX_FEATURE_STATS) */
    return new CMP4AudioFormat(m_pCommonClassFactory, this);
}


/****************************************************************************
 *  GetRendererName
 */
const char* CMP4AudioRenderer::GetRendererName(void)
{
    return MPEG4_AUDIO_RENDERER_NAME;
}


/****************************************************************************
 *  GetCodecFourCC
 */
const char* CMP4AudioRenderer::GetCodecFourCC(void)
{
    return MPEG4_AUDIO_CODEC_4CC;
}


/****************************************************************************
 *  GetCodecName
 */
const char* CMP4AudioRenderer::GetCodecName(void)
{
    return MPEG4_AUDIO_RENDERER_NAME;
}

HX_RESULT STDAPICALLTYPE CMP4AudioRenderer::HXCreateInstance(IUnknown** ppIUnknown)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppIUnknown)
    {
        // Set default
        *ppIUnknown = NULL;
        // Create the object
        CMP4AudioRenderer *pObj = new CMP4AudioRenderer();
        if (pObj)
        {
            // QI for IUnknown
            retVal = pObj->QueryInterface(IID_IUnknown, (void**) ppIUnknown);
        }
        if (FAILED(retVal))
        {
            HX_DELETE(pObj);
        }
    }

    return retVal;
}

HX_RESULT STDAPICALLTYPE CMP4AudioRenderer::CanUnload2()
{
    return (CHXBaseCountingObject::ObjectsActive() > 0 ? HXR_FAIL : HXR_OK);
}
