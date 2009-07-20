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

/****************************************************************************
 *  Includes
 */
#include "qtpacketizerfct.h"
#include "qttrkmgr.h"
#include "qtatmmgs.h"

#include "hxerror.h"

#include "mp4vpyld.h"
#include "mp4apyld.h"
#include "rule2flg.h" // /For RuleToFlagMap (ASM-rule-to-flag map)
#include "mp4tpyld.h"
#include "concatpyld.h"
#include "pcmpyld.h"
#include "mp3pyld.h"
#include "h264packetizer.h"

#include "safestring.h"
#include "qtpacketizerfct.h"

#ifdef QTCONFIG_PACKETIZE_LATM
#include "latmpacketizer.h"
#endif /* QTCONFIG_PACKETIZE_LATM */
#if defined QTCONFIG_PACKETIZE_AMR_NB || defined QTCONFIG_PACKETIZE_AMR_WB
#include "tsconvrt.h"
#include "amrpacketizer.h"
#endif /* QTCONFIG_PACKETIZE_AMR_NB || QTCONFIG_PACKETIZE_AMR_WB */
#ifdef QTCONFIG_PACKETIZE_H263
#include "h263pkt.h"	// H263-2000 pktizer
#endif /* QTCONFIG_PACKETIZE_H263 */
#ifdef QTCONFIG_PACKETIZE_MP4V
#include "mp4vpacketizer.h"	// MP4V-ES pktizer
#endif /* QTCONFIG_PACKETIZE_MP4V */


/****************************************************************************
 *  Class CQTPacketizerFactory
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CQTPacketizerFactory::CQTPacketizerFactory()
{
    ;
}

CQTPacketizerFactory::~CQTPacketizerFactory()
{
    ;
}

/****************************************************************************
 *  Main Interface
 */
/****************************************************************************
 *  Construct
 */
HX_RESULT CQTPacketizerFactory::Construct(IHXPayloadFormatObject* &pPacketizer,
					  const char* pProtocol,
					  CQT_TrackInfo_Manager* pTrackInfo,
					  CQT_MovieInfo_Manager* pMovieInfo,
					  CQTTrackManager* pTrackManager,
					  CQTTrack* pTrack,
					  IUnknown* pContext)
{
    HX_RESULT retVal = HXR_OK;

#ifdef QTCONFIG_SERVER
    const char *pMimeType = NULL;

    if (pTrackInfo)
    {
	pMimeType = pTrackInfo->GetMimeType();
    }

    if (pTrackManager->GetEType() != QT_ETYPE_CLIENT && pMimeType && 
	!pTrackManager->IsHintsEnabled())
    {
	// Construct the packetizer selected based on mime type or
	// other track, movie or protocol properties here
#ifdef QTCONFIG_PACKETIZE_LATM
        if(strcasecmp(pMimeType, "audio/X-RN-MP4-RAWAU") == 0)
        {
            pPacketizer = (IHXPayloadFormatObject*)
                (new LATMPacketizer(pTrackInfo->GetNumChannels(),
                pTrackInfo->GetMediaTimeScale()));
            if(!pPacketizer)
            {
                return HXR_OUTOFMEMORY;
            }
            
            pPacketizer->AddRef();
            return HXR_OK;
        }
#endif /* QTCONFIG_PACKETIZE_LATM */
#ifdef QTCONFIG_PACKETIZE_AMR_NB
        if(strcasecmp(pMimeType, "audio/X-RN-3GPP-AMR") == 0)
        {
            pPacketizer = (IHXPayloadFormatObject*)
                (new AMRPacketizer(NarrowBand));
            if(!pPacketizer)
            {
                return HXR_OUTOFMEMORY;
            }

            pPacketizer->AddRef();
            return HXR_OK;
        }
#endif /* QTCONFIG_PACKETIZE_AMR_NB */
#ifdef QTCONFIG_PACKETIZE_AMR_WB
        if(strcasecmp(pMimeType, "audio/X-RN-3GPP-AMR-WB") == 0)
        {
            pPacketizer = (IHXPayloadFormatObject*)
                (new AMRPacketizer(WideBand));
            if(!pPacketizer)
            {
                return HXR_OUTOFMEMORY;
            }

            pPacketizer->AddRef();
            return HXR_OK;
        }
#endif /* QTCONFIG_PACKETIZE_AMR_WB */
#ifdef QTCONFIG_PACKETIZE_H263
        if(strcasecmp(pMimeType, "video/X-RN-3GPP-H263") == 0)
        {
            pPacketizer = (IHXPayloadFormatObject*)
                (new CH263Packetizer(pTrackInfo));
            if(!pPacketizer)
            {
                return HXR_OUTOFMEMORY;
            }
            
            pPacketizer->AddRef();
            return HXR_OK;
        }
#endif /* QTCONFIG_PACKETIZE_H263 */
#ifdef QTCONFIG_PACKETIZE_MP4V
        if(strcasecmp(pMimeType, "video/X-RN-MP4") == 0)
        {
            pPacketizer = (IHXPayloadFormatObject*)(new CMP4VPacketizer());
            if(!pPacketizer)
            {
                return HXR_OUTOFMEMORY;
            }
            
            pPacketizer->AddRef();
            return HXR_OK;
        }
#endif /* QTCONFIG_PACKETIZE_MP4V */
		if(strcasecmp(pMimeType, "video/X-HX-AVC1") == 0)
        {
            UINT32 ulBrand = 0;
            pTrackManager->GetMajorBrand(&ulBrand);

		    switch (ulBrand)
		    {
		        // check whether major brand is a rel6 brand
		        case QT_3gg6:
		        case QT_3gp6:
		        case QT_3gr6:
		        case QT_3gs6:
                    IHXErrorMessages *pErrorMessages = NULL;
                    if(SUCCEEDED(pContext->QueryInterface(IID_IHXErrorMessages, 
                                            (void**) &pErrorMessages)))
                    {
                        pErrorMessages->Report(HXLOG_WARNING, HXR_OK, 0, 
                            "Warning: Unhinted rel6 file - may not be playable by third party players \n", NULL);
                        HX_RELEASE(pErrorMessages);
                    }
                    break;
            }            
        }
    }
#endif /* QTCONFIG_SERVER */

    switch (pTrackManager->GetFType())
    {
    case QT_FTYPE_QT:
	switch (pTrackInfo->GetTrackType())
	{
        case QT_soun:
            if ((HXR_OK == retVal) && !pPacketizer)
            {
                const char* pMimeType = pTrackInfo->GetMimeType();
                if (pMimeType &&
                    (!strcasecmp(pMimeType, "audio/X-HX-TWOS") ||
                    !strcasecmp(pMimeType, "audio/X-HX-SOWT")||
                    !strcasecmp(pMimeType, "audio/PCMA")||
                    !strcasecmp(pMimeType, "audio/PCMU")))
                {
                    retVal = 
                        HXConcatenatePCMPayloadFormat::CreateInstance(pPacketizer);
                }
                else if(!strcasecmp(pMimeType, "audio/MPEG-ELEMENTARY"))
                {
                    retVal = 
                        HXConcatenateMP3PayloadFormat::CreateInstance(pPacketizer);                
                }
                else if (pMimeType &&
		    (!strcasecmp(pMimeType, "audio/X-RN-3GPP-AMR") ||
		     !strcasecmp(pMimeType, "audio/X-RN-3GPP-AMR-WB")))
                {
                    retVal = 
		        HXConcatenatePayloadFormat::CreateInstance(pPacketizer);
                }
                else
                {
                    retVal = HXR_NO_DATA;
                }
            }
	    break;
	}
	break;
    case QT_FTYPE_MP4:
	switch (pTrackInfo->GetTrackType())
	{
	case QT_vide:
#ifdef QTCONFIG_VIDEO_PACKETIZER
	    if (pProtocol == NULL)
	    {
		MP4VPayloadFormat* pMP4VPacketizer = new MP4VPayloadFormat;
		
		retVal = HXR_OUTOFMEMORY;
		if (pMP4VPacketizer)
		{
		    pMP4VPacketizer->AddRef();
		    pPacketizer = (IHXPayloadFormatObject*) pMP4VPacketizer;
		    retVal = HXR_OK;
		}
	    }   
#else 
#ifdef QTCONFIG_H264_PACKETIZER
        if (pTrackManager->GetEType() == QT_ETYPE_SERVER)
        {
            const char* pMimeType = pTrackInfo->GetMimeType();
            if (pMimeType && (!strcasecmp(pMimeType, "video/X-HX-AVC1")))
            {
                retVal = H264Packetizer::CreateInstance(pPacketizer);
            }
        }
#else
	    retVal = HXR_NO_DATA;
#endif	// QTCONFIG_H264_PACKETIZER
#endif	// QTCONFIG_VIDEO_PACKETIZER
	    break;
	case QT_soun:
#ifdef QTCONFIG_AUDIO_PACKETIZER
	    if (pProtocol == NULL)
	    {
		MP4APayloadFormat* pMP4APacketizer = new MP4APayloadFormat;
		
		retVal = HXR_OUTOFMEMORY;
		if (pMP4APacketizer)
		{
		    pMP4APacketizer->AddRef();
		    pPacketizer = (IHXPayloadFormatObject*) pMP4APacketizer;
		    retVal = HXR_OK;
		}
	    }   
#endif	// QTCONFIG_AUDIO_PACKETIZER

	    if ((HXR_OK == retVal) && !pPacketizer)
	    {
		const char* pMimeType = pTrackInfo->GetMimeType();
		if (pMimeType &&
		    (!strcasecmp(pMimeType, "audio/X-RN-3GPP-AMR") ||
		     !strcasecmp(pMimeType, "audio/X-RN-3GPP-AMR-WB")))
		{
		    retVal = 
			HXConcatenatePayloadFormat::CreateInstance(pPacketizer);
		}
		else
		{
		    retVal = HXR_NO_DATA;
		}
	    }

	    break;
#ifdef QTCONFIG_TIMEDTEXT_PACKETIZER
	case QT_text:
	    if (pProtocol == NULL)
	    {
		MP4TPayloadFormat* pMP4TPacketizer = new MP4TPayloadFormat(
			pTrack, pTrackInfo);
		
		retVal = HXR_OUTOFMEMORY;
		if (pMP4TPacketizer)
		{
		    pMP4TPacketizer->AddRef();
		    pPacketizer = (IHXPayloadFormatObject*) pMP4TPacketizer;
		    retVal = HXR_OK;
		}
	    }
	    break;
#endif	// QTCONFIG_TIMEDTEXT_PACKETIZER
	}
	break;
	
	case QT_FTYPE_EMC:
	    retVal = HXR_NOTIMPL;
	    break;
	default:
	    retVal = HXR_FAIL;
	    break;
    }

    return retVal;
}
