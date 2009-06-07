/* ***** BEGIN LICENSE BLOCK ***** 
 * 
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxplugn.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxformt.h"
#include "hxslist.h"
#include "dllaccesbridge.h"
#include "dllpath.h"
#include "hxacodec.h"
#include "hxstrutl.h"
#include "hxplgns.h"
#include "pckunpck.h"
#include "hxprefutil.h"

#include "mp4adec.h"
#if defined(HELIX_FEATURE_AUDIO_CODEC_QCELP)
#include "qcelpdecinfo.h"
#endif
#if defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB)
#include "amrdecinfo.h"
#endif
#if defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB)
#include "amrwbdecinfo.h"
#endif
#if defined(HELIX_FEATURE_AUDIO_CODEC_RAAC)
#include "raacdecinfo.h"
#endif
#if defined(HELIX_FEATURE_AUDIO_CODEC_AAC)
#include "aacdecinfo.h"
#endif
#if defined(HELIX_FEATURE_AUDIO_CODEC_MP3)
#include "mp3decinfo.h"
#endif
#if defined(HELIX_FEATURE_AUDIO_RALF)
#include "losslessdecinfo.h"
#endif
#if defined(HELIX_FEATURE_SYMBIAN_MDF_AUDIO_SW)
#include "mdfswdecinfo.h"
#endif
#if defined(HELIX_FEATURE_SYMBIAN_MDF_AUDIO_DSP)
#include "mdfhwdecinfo.h"
#endif
#define CODEC_FOURCC_CONFIG_PROPERTY    "CodecFourCC"

/****************************************************************************
 *  Method:
 *    CMP4ADecoder::CMP4ADecoder
 *
 */
CMP4ADecoder::CMP4ADecoder()
    : m_pCodecAccess(NULL)
    , m_pDecoderInfo(NULL)
    , m_bMDFHWDecInfoEnabled(TRUE) // enabled by default
    , m_bMDFSWDecInfoEnabled(TRUE) // enabled by default
{
    // Register the decoder info with m_decInfoStore
    RegisterDecoderInfo();
    // Null out lib name string
    memset(&m_szAUStr[0], 0, MAX_LIBNAME_SIZE);
}

/****************************************************************************
 *  Method:
 *    CMP4ADecoder::~CMP4ADecoder
 *
 */
CMP4ADecoder::~CMP4ADecoder()
{
    Close();
}

void CMP4ADecoder::RegisterDecoderInfo()
{
#if defined(HELIX_FEATURE_SYMBIAN_MDF_AUDIO_DSP)
    // This must be defined before ARM SW codecs to give DSP codecs priority.
    m_decInfoStore.RegisterInfo(new CMDFHWDecInfo(this)); 
#endif    

#if defined(HELIX_FEATURE_SYMBIAN_MDF_AUDIO_SW)
    // This must be defined before standard Helix codecs.
    m_decInfoStore.RegisterInfo(new CMDFSWDecInfo(this)); 
#endif

#if defined(HELIX_FEATURE_AUDIO_CODEC_RAAC)
    m_decInfoStore.RegisterInfo(new CRAACDecInfo);
#endif	// defined(HELIX_FEATURE_AUDIO_CODEC_RAAC)

#if defined(HELIX_FEATURE_AUDIO_CODEC_AAC)
    // Register AAC library name function
    m_decInfoStore.RegisterInfo(new CAACDecInfo);
#endif	// defined(HELIX_FEATURE_AUDIO_CODEC_AAC)

#if defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB)
    // Register AMR-NB library name function
    m_decInfoStore.RegisterInfo(new CAMRNBDecInfo);
#endif // defined(HELIX_FEATURE_AUDIO_CODEC_AMRNB)

#if defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB)
    // Register AMR-WB library name function
    m_decInfoStore.RegisterInfo(new CAMRWBDecInfo);
#endif	// defined(HELIX_FEATURE_AUDIO_CODEC_AMRWB)

#if defined(HELIX_FEATURE_AUDIO_CODEC_MP3)
    // Register MP3 library name function
    m_decInfoStore.RegisterInfo(new CMP3DecInfo);
#endif	// defined(HELIX_FEATURE_AUDIO_CODEC_MP3)

    // Register ALSD/lossless library name function
#if defined(HELIX_FEATURE_AUDIO_RALF)
    m_decInfoStore.RegisterInfo(new CLosslessDecInfo);
#endif	// HELIX_FEATURE_AUDIO_RALF

#if defined(HELIX_FEATURE_AUDIO_CODEC_QCELP)
    // Register QCELP library name
    m_decInfoStore.RegisterInfo(new CQCELPDecInfo);
#endif  // HELIX_FEATURE_AUDIO_CODEC_QCELP
}

HX_RESULT CMP4ADecoder::Close(void)
{
    HX_DELETE(m_pCodecAccess);

    return HXR_OK;
}


/****************************************************************************
 *  Method:
 *    CMP4ADecoder::CMP4ADecoder
 *
 */
HX_RESULT CMP4ADecoder::Open(IHXValues* pHeader,
                             IMP4APayloadFormat* pRssm, 
			     IHXAudioDecoder** pOutAudioDecoder,
			     IUnknown* pContext)
{
    HX_RESULT retVal = HXR_FAIL;

    InitializeDynamicConfiguration(pContext);

    if (pHeader && pOutAudioDecoder)
    {
        // Close the decoder
        Close();
        
        // Get the mime type
        IHXBuffer* pMimeType = NULL;
        pHeader->GetPropertyCString("MimeType", pMimeType);
        
        if (pMimeType)
        {
            m_pDecoderInfo = m_decInfoStore.GetInfoFirst((const char*) pMimeType->GetBuffer(), pRssm);
            if (m_pDecoderInfo)
            {
                retVal = BuildDLLAccessBridge(pOutAudioDecoder, pMimeType, pRssm, pContext);		
            }
        }
        HX_RELEASE(pMimeType);
    }

    return retVal;
}

HX_RESULT CMP4ADecoder::OpenNext(IHXValues* pHeader,
                                 IMP4APayloadFormat* pRssm, 
			         IHXAudioDecoder** pOutAudioDecoder,
			         IUnknown* pContext)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pHeader && pOutAudioDecoder)
    {
        // Close the decoder
        Close();
        // Get the mime type
        IHXBuffer* pMimeType = NULL;
        pHeader->GetPropertyCString("MimeType", pMimeType);
        if (pMimeType)
        {
            m_pDecoderInfo = m_decInfoStore.GetInfoNext((const char*) pMimeType->GetBuffer(), pRssm);
            if (m_pDecoderInfo)
            {
                HX_RELEASE(*pOutAudioDecoder);
                retVal = BuildDLLAccessBridge(pOutAudioDecoder, pMimeType, pRssm, pContext);
            }
        }
        HX_RELEASE(pMimeType);
    }

    return retVal;
}

void CMP4ADecoder::InitializeDynamicConfiguration(IUnknown* pContext)
{
    if (pContext)
    {
        HXBOOL bUseHWDeviceCodec = FALSE;

        // store the preference for use by IsMatch() from DecInfo's who can access it
        HX_RESULT result = ReadPrefBOOL(pContext, "UseCMMFHWDeviceAudioCodec", bUseHWDeviceCodec);
        if(SUCCEEDED(result))
        {
            // If the preference exists, only enable one or the other.
            // Otherwise, both are enabled and preference is given to
            // hardware accelerated codecs.
            m_bMDFHWDecInfoEnabled = bUseHWDeviceCodec;
            m_bMDFSWDecInfoEnabled = !bUseHWDeviceCodec;
        }
    }
}

HX_RESULT CMP4ADecoder::BuildDLLAccessBridge(IHXAudioDecoder** pOutAudioDecoder,
                                             IHXBuffer* pMimeType,
                                             IMP4APayloadFormat* pRssm, 
                                             IUnknown* pContext)
{
    HX_RESULT retVal = HXR_OK;
    char pOSLibName[MAX_LIBNAME_SIZE]; /* Flawfinder: ignore */

    while (m_pDecoderInfo)
    {
        if (OS_BuildLibName(m_pDecoderInfo->GetLibName(),
                            pOSLibName,
                            MAX_LIBNAME_SIZE))
        {
            retVal = HXR_OUTOFMEMORY;

            if (!m_pCodecAccess)
            {
                // DLLAccess should be used to provide cross-platform support
                m_pCodecAccess = new DLLAccessBridge(pOSLibName, 
						     DLLTYPE_CODEC, 
						     pContext);
            }
            if (m_pCodecAccess)
            {
                retVal = HXR_REQUEST_UPGRADE;
                if (m_pCodecAccess->isOpen())
                {
                    // we got a module so get the symbols we need
                    FPRACreateDecoderInstance fpRACreateDecoderInstance = NULL;
                    fpRACreateDecoderInstance = (FPRACreateDecoderInstance) 
                        m_pCodecAccess->getSymbol("RACreateDecoderInstance");
                    if (fpRACreateDecoderInstance)
                    {
                        IHXAudioDecoder* pAudioDecoder = NULL;
                        retVal = (*fpRACreateDecoderInstance)(IID_IHXAudioDecoder,
                                                              (IUnknown**) &pAudioDecoder);
                        if (SUCCEEDED(retVal))
                        {
                            retVal = CheckDecoder(pAudioDecoder);
                            if (SUCCEEDED(retVal))
                            {
                                // Set the out parameter
                                pAudioDecoder->AddRef();
                                *pOutAudioDecoder = pAudioDecoder;
                            }
                        }
                        HX_RELEASE(pAudioDecoder);
                    }
                }
            }
        }
	if (SUCCEEDED(retVal))
	{
	    break;
	}

	HX_DELETE(m_pCodecAccess);
	m_pDecoderInfo = m_decInfoStore.GetInfoNext((const char*) pMimeType->GetBuffer(), pRssm);
    }

    // if IHXObjectConiguration is implemented by the Decoder, then pass
    // it configuration parameters, e.g. context, fourCC
    if (SUCCEEDED(retVal))
    {
        IHXObjectConfiguration* pObjConfig = NULL;

        (*pOutAudioDecoder)->QueryInterface(IID_IHXObjectConfiguration, 
                                            (void**)(&pObjConfig));

        if (pObjConfig)
        {
            IHXValues* pConfig = NULL;
            retVal = CreateValuesCCF(pConfig, pContext);

            if (SUCCEEDED(retVal))
            {
                retVal = SetCStringPropertyCCF(pConfig, 
                                               CODEC_FOURCC_CONFIG_PROPERTY,
                                               m_pDecoderInfo->GetCodecFourCC(),
                                               pContext,
                                               FALSE);
            }

            if (SUCCEEDED(retVal))
            {
                retVal = pObjConfig->SetContext(pContext);
            }

            if (SUCCEEDED(retVal))
            {
                retVal = pObjConfig->SetConfiguration(pConfig);
            }

            HX_RELEASE(pConfig);
        }
        
        HX_RELEASE(pObjConfig);
    }

    return retVal;
}

#if defined(WIN32) || defined(_WIN32) || defined(_SYMBIAN)
static const char z_dllSuffix[] = ".DLL";
#elif defined(_MAC_UNIX)
static const char z_dllSuffix[] = ".bundle";
#elif _MACINTOSH 
#if defined(_CARBON)
#ifdef _MAC_MACHO
static const char z_dllSuffix[] = ".bundle";
#else
static const char z_dllSuffix[] = ".shlb";
#endif
#else	// _CARBON
static const char z_dllSuffix[] = ".DLL";
#endif	// _CARBON
#elif defined (_UNIX)
    //
    // codecs are named like this :XXXX.so" where XXXX is the
    // codec id string
    //
static const char z_dllSuffix[] = ".so";
#elif defined (_OPENWAVE)
// currently it doesn't support dynamic loading lib, so N/A
static const char z_dllSuffix[] = "";
#endif

/****************************************************************************
 *  Method:
 *    CMP4ADecoder::OS_BuildLibName
 */
HXBOOL CMP4ADecoder::OS_BuildLibName(const char* pLibName, 
				   char *pOSLibName, 
				   UINT32 ulBufLen)
{
    HXBOOL bRetVal = FALSE;

    if (pOSLibName && pLibName)
    {
#if defined(HELIX_FEATURE_AUTOUPGRADE)
        // Copy the base file name into the AU String (just
	// in case we need it)
	SafeStrCpy(m_szAUStr, pLibName, MAX_LIBNAME_SIZE);
#endif /* #if defined(HELIX_FEATURE_AUTOUPGRADE) */

	SafeStrCpy(pOSLibName, pLibName, MAX_LIBNAME_SIZE);
	// Now append the filename suffix
	SafeStrCat(pOSLibName, z_dllSuffix, ulBufLen);

	bRetVal = TRUE;
    }

    return bRetVal;
}
