/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdppyldinfo.cpp,v 1.5 2005/09/29 20:59:17 ehyche Exp $
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

#include "hxtypes.h"
#include "hxassert.h"
#include "hlxclib/string.h"
#include "rtptypes.h"

/****************************************************************************
 *  Defines
 */
#define RTP_PAYLOAD_MAP_SIZE	35
#define RTP_BAD_PAYLOAD_TYPE	0xFFFFFFFF

/****************************************************************************
 *  Global - Types
 */
struct RTPPayloadInfo 
{
    const char* pEncoding;
    UINT32  ulRTPFactor;
    UINT32  ulHXFactor;
    HXBOOL    bTimestampDeliv;
    UINT32  ulBitRate;
    UINT16  usChannels;
};

struct MimeTypeToClockRateAndChannelsInfo
{
    const char*	pMimeType;
    UINT32	ulSamplesPerSecond;
    UINT32      ulChannels;
};

struct RTPPayloadHeaderPair
{
    ULONG32 ulPayloadType;
    const BYTE* pHeader;
    ULONG32 ulHeaderSize;
};


/* opaque data for known RTP payloads */
const BYTE p_zPCMU_DATA[] = 
{
    0x01, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00,
    0x40, 0x1f, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 
    0x00, 0x00, 0x26, 0x26 
};

const BYTE p_zPCMA_DATA[] =
{
    0x06, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00,
    0x40, 0x1f, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 
    0x00, 0x00, 0x26, 0x26 
};

const BYTE p_zL162CH_DATA[] =
{
    0x00, 0x00, 0x43, 0x50, 0x00, 0x01, 0x00, 0x02, 
    0x00, 0x00, 0xac, 0x44, 0x00, 0x10, 0x00, 0x00
};

const BYTE p_zL161CH_DATA[] =
{
    0x00, 0x00, 0x43, 0x50, 0x00, 0x01, 0x00, 0x01, 
    0x00, 0x00, 0xac, 0x44, 0x00, 0x10, 0x00, 0x00
};

const BYTE p_zGSM_DATA[] =
{
    0x31, 0x00, 0x01, 0x00, 0x40, 0x1f, 0x00, 0x00,
    0x59, 0x06, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x40, 0x01, 0x26, 0x26
};

typedef struct tPCMHEADER
{
    UINT16      usFormatTag;  /* format type */
    UINT16      usChannels;   /* number of channels (i.e. mono, stereo..*/
    UINT32      ulSamplesPerSec;   /* sample rate */
    UINT16      usBitsPerSample;   /* number of bits per sample of mono data*/
    UINT16      usSampleEndianness; /* for 16-bit samples, 0==little, 1==big */
} PCMHEADER;

/****************************************************************************
 *  Payload to header table - must be NULL entry terminated
 */
const RTPPayloadHeaderPair z_PayloadHeaderPairList[] =
{
    { RTP_PAYLOAD_PCMU,	    p_zPCMU_DATA,   sizeof(p_zPCMU_DATA)    },
    { RTP_PAYLOAD_PCMA,	    p_zPCMA_DATA,   sizeof(p_zPCMA_DATA)    },
    { RTP_PAYLOAD_L16_2CH,  p_zL162CH_DATA, sizeof(p_zL162CH_DATA)  },
    { RTP_PAYLOAD_L16_1CH,  p_zL161CH_DATA, sizeof(p_zL161CH_DATA)  },
    { RTP_PAYLOAD_GSM,	    p_zGSM_DATA,    sizeof(p_zGSM_DATA)	    },
    { RTP_BAD_PAYLOAD_TYPE, NULL,	    0			    }
};

/****************************************************************************
 *  Payload to RN MimeType Table
 *  This table identifies payloads RN can render and the mappings from
 *  the payloads to renderer mime types.
 *  With the transition to standard mime types supported by renderers,
 *  this table will lose the function of mapping but will continue to
 *  identify the renderer supported payloads.
 */
const char* const z_PayloadToRNMimeMap[RTP_PAYLOAD_MAP_SIZE] =
{
    "audio/PCMU",		// 0
    NULL,			// 1
    NULL,			// 2
    "audio/x-pn-gsm610",	// 3
    NULL,			// 4
    "audio/x-pn-dvi4",		// 5
    "audio/x-pn-dvi4",		// 6
    NULL,			// 7
    "audio/PCMA",		// 8
    NULL,			// 9
    "audio/L16",		// 10
    "audio/L16",		// 11
    NULL,			// 12
    NULL,			// 13
    "audio/MPA",		// 14
    NULL,			// 15
    NULL,			// 16
    NULL,			// 17
    NULL,			// 18
    NULL,			// 19
    NULL,			// 20
    NULL,			// 21
    NULL,			// 22
    NULL,			// 23
    NULL,			// 24
    NULL,			// 25
    "video/x-pn-jpeg-plugin",	// 26
    NULL,			// 27
    NULL,			// 28
    NULL,			// 29
    NULL,			// 30
    "video/H261",		// 31
    "video/MPV",		// 32
    NULL,			// 33
    "video/H263"		// 34
};

/*
 * Payload timestamp conversion table - contains the RTPTimestamp to
 * HXTimestamp ratio factor as well as whether or not this payload
 * is frame based or not.  Since packet size is not included in the To compute the timestamp conversion ratio
 * use this formula for sample based payloads:
 *
 * ClockRate (sam/sec) * SampleSize (Bytes/Sample * Channels) / 1000 = 
 *	RTPFactor/HXFactor
 *
 * The stream headers RTPTimestampConversionFactor, HXTimestampConversionFactor
 * and TimestampDelivery are added for known and supported payload types.
 *
 * To convert RTP to RMA time multiply the RTP timestamp by the fraction:
 * RTPTimestampConversionFactor / HXTimestampConversionFactor
 * To convert RMA to RTP time multiply the RMA timestamp by the fraction:
 * HXTimestampConversionFactor / RTPTimestampConversionFactor
 *
 * This information should be exposed with IHXRTPTimestampConverter
 */
/* taken from draft-ietf-avt-profile-new-08.txt */ 
const RTPPayloadInfo z_pPayloads[RTP_PAYLOAD_MAP_SIZE] = 
{
/*   encoding   RTP     RMA     TSD     BW	Channels */
    { "PCMU",	8,	1,	0,	64000,	    1	}, // 0   8000/8/1
    { "1016",	8,	1,	1,	0,	    1	}, // 1   8000/N/1
    { "G726-32",8,	1,	0,	32000,	    1	}, // 2   8000/4/1
    { "GSM",	8,	1,	1,	0,	    1	}, // 3   8000/N/1
    { "G723",	8,	1,	1,	0,	    1	}, // 4   8000/N/1
    { "DVI4",	8,	1,	0,	32000,	    1	}, // 5   8000/4/1
    { "DVI4",	16,	1,	0,	64000,	    1	}, // 6  16000/4/1
    { "LPC",	8,	1,	1,	0,	    1	}, // 7   8000/N/1
    { "PCMA",	8,	1,	0,	64000,	    1	}, // 8   8000/8/1
    { "G722",	8,	1,	0,	64000,	    1	}, // 9   8000/8/1
    { "L16",	441,	10,	0,	1411200,    2	}, // 10 44100/16/2
    { "L16",	441,	10,	0,	705600,	    1	}, // 11 44100/16/1
    { "QCELP",	8,	1,	1,	0,	    1	}, // 12  8000/N/1
    { NULL,	0,	0,	0,	0,	    0	}, // 13 reserved
    { "MPA",	90,	1,	1,	0,	    0	}, // 14 90000/N
    { "G728",	8,	1,	1,	0,	    1	}, // 15  8000/N/1
    { "DVI4",	441,	40,	0,	44100,	    1	}, // 16 11025/4/1
    { "DVI4",	441,	20,	0,	88200,	    1	}, // 17 22050/4/1
    { "G729",	8,	1,	1,	0,	    1	}, // 18  8000/N/1
    { NULL,	0,	0,	0,	0,	    0	}, // 19 reserved
    { NULL,	0,	0,	0,	0,	    0	}, // 20 unassigned
    { NULL,	0,	0,	0,	0,	    0	}, // 21 unassigned
    { NULL,	0,	0,	0,	0,	    0	}, // 22 unassigned
    { NULL,	0,	0,	0,	0,	    0	}, // 23 unassigned
    { NULL,	0,	0,	0,	0,	    0	}, // 24 unassigned
    { "CelB",	90,	1,	1,	0,	    0	}, // 25 90000/N
    { "JPEG",	90,	1,	1,	0,	    0	}, // 26 90000/N
    { NULL,	0,	0,	0,	0,	    0	}, // 27 unassigned
    { "nv",	90,	1,	1,	0,	    0	}, // 28 90000/N
    { NULL,	0,	0,	0,	0,	    0	}, // 29 unassigned
    { NULL,	0,	0,	0,	0,	    0	}, // 30 unassigned
    { "H261",	90,	1,	1,	0,	    0	}, // 31 90000/N
    { "MPV",	90,	1,	1,	0,	    0	}, // 32 90000/N  
    { "MP2T",	90,	1,	1,	0,	    0	}, // 33 90000/N
    { "H263",	90,	1,	1,	0,	    0	}, // 34 90000/N
};

const UINT32 z_cPayloads = 
    sizeof(z_pPayloads) / 
    sizeof(z_pPayloads[0]);

/*
 *  yet another table for mimetype to samplespersecond
 *
 * This information may be found in:
 *
 * http://www.iana.org/assignments/rtp-parameters
 *
 * in the section "RTP Payload Format MIME types".
 * Only those mime types which have default clock
 * rates or channels are listed here.
 */
const MimeTypeToClockRateAndChannelsInfo z_pMimeTypeToClockRateAndChannels[] = 
{
    { "audio/AMR",           8000, 0},
    { "audio/AMR-WB",       16000, 0},
    { "audio/EVRC",          8000, 1},
    { "audio/EVRC0",         8000, 1},
    { "audio/G.722.1",      16000, 1},
    { "audio/G726-16",       8000, 1},
    { "audio/G726-24",       8000, 1},
    { "audio/G726-32",       8000, 1},
    { "audio/G726-40",       8000, 1},
    { "audio/G729D",         8000, 1},
    { "audio/G729E",         8000, 1},
    { "audio/GSM-EFR",       8000, 1},
    { "audio/VDVI",             0, 1},
    { "audio/mpa-robust",   90000, 0},
    { "audio/SMV",           8000, 1},
    { "audio/SMV0",          8000, 1},
    { "text/red",            1000, 0},
    { "text/t140",           1000, 0},
    { "video/BMPEG",        90000, 0},
    { "video/BT656",        90000, 0},
    { "video/DV",           90000, 0},
    { "video/H263-1998",    90000, 0},
    { "video/H263-2000",    90000, 0},
    { "video/MP1S",         90000, 0},
    { "video/MP2P",         90000, 0},
    { "video/MP4V-ES",      90000, 0},
    { "video/pointer",      90000, 0},
    { "video/raw",          90000, 0}
};

const UINT32 z_cMimeTypeToClockRateAndChannels = 
    sizeof(z_pMimeTypeToClockRateAndChannels) / 
    sizeof(z_pMimeTypeToClockRateAndChannels[0]);

const UINT8* SDPGetGSMOpaqueData()
{
    return p_zGSM_DATA;
}

ULONG32 SDPGetGSMOpaqueDataSize()
{
    return sizeof(p_zGSM_DATA);
}

/****************************************************************************
 *  SDPIsStaticPayload
 */
HXBOOL SDPIsStaticPayload(ULONG32 ulPayloadType)
{
    return (ulPayloadType < RTP_PAYLOAD_MAP_SIZE);
}

/****************************************************************************
 *  SDPMapPayloadToRTPFactor
 */
UINT32 SDPMapPayloadToRTPFactor(ULONG32 ulPayloadType)
{
    HX_ASSERT(SDPIsStaticPayload(ulPayloadType));
    return z_pPayloads[ulPayloadType].ulRTPFactor;
}

/****************************************************************************
 *  SDPMapPayloadToRMAFactor
 */
UINT32 SDPMapPayloadToRMAFactor(ULONG32 ulPayloadType)
{
    HX_ASSERT(SDPIsStaticPayload(ulPayloadType));
    return z_pPayloads[ulPayloadType].ulHXFactor;
}

/****************************************************************************
 *  SDPMapPayloadToSamplesPerSecond
 */
ULONG32 SDPMapPayloadToSamplesPerSecond(ULONG32 ulPayloadType)
{
    if (SDPIsStaticPayload(ulPayloadType))
    {
        return (ULONG32) (1000.0 *
                          ((double) SDPMapPayloadToRTPFactor(ulPayloadType)) /
                          ((double) SDPMapPayloadToRMAFactor(ulPayloadType)));
    }

    return 0;
}

/****************************************************************************
 *  SDPIsTimestampDeliverable
 */
HXBOOL SDPIsTimestampDeliverable(ULONG32 ulPayloadType)
{
    HX_ASSERT(SDPIsStaticPayload(ulPayloadType));
    return z_pPayloads[ulPayloadType].bTimestampDeliv;
}

/****************************************************************************
 *  SDPMapPayloadToEncodingName
 */
const char* SDPMapPayloadToEncodingName(ULONG32 ulPayloadType)
{
    if (SDPIsStaticPayload(ulPayloadType))
    {
        return z_pPayloads[ulPayloadType].pEncoding;
    }

    return NULL;
}

/****************************************************************************
 *  SDPMapPayloadToEncodingName
 */
const char* SDPMapPayloadToMimeType(ULONG32 ulPayloadType)
{
    if (SDPIsStaticPayload(ulPayloadType))
    {
        return z_PayloadToRNMimeMap[ulPayloadType];
    }

    return NULL;
}

/****************************************************************************
 *  SDPMapPayloadToChannels
 */
UINT16 SDPMapPayloadToChannels(ULONG32 ulPayloadType)
{
    if (SDPIsStaticPayload(ulPayloadType))
    {
        return z_pPayloads[ulPayloadType].usChannels;	  
    }

    return 0;
}

/****************************************************************************
 *  SDPMapPayloadToBitrate
 */
ULONG32 SDPMapPayloadToBitrate(ULONG32 ulPayloadType)
{
    if (SDPIsStaticPayload(ulPayloadType))
    {
        return z_pPayloads[ulPayloadType].ulBitRate;	  
    }

    return 0;
}

/****************************************************************************
 *  SDPMapMimeTypeToSampleRate
 */
ULONG32 SDPMapMimeTypeToSampleRate(const char* pMimeType)
{
    ULONG32 ulSamplesPerSecond = 0;

    for (ULONG32 ulIdx = 0; 
         ulIdx < z_cMimeTypeToClockRateAndChannels; 
         ulIdx++)
    {
        if (z_pMimeTypeToClockRateAndChannels[ulIdx].pMimeType &&
            !strcasecmp(
                z_pMimeTypeToClockRateAndChannels[ulIdx].pMimeType,
                pMimeType))
        {
            ulSamplesPerSecond = 
                z_pMimeTypeToClockRateAndChannels[ulIdx].ulSamplesPerSecond;
            break;
        }
    }

    return ulSamplesPerSecond;
}

/****************************************************************************
 *  SDPMapMimeTypeToChannels
 */
ULONG32 SDPMapMimeTypeToChannels(const char* pMimeType)
{
    ULONG32 ulChannels = 0;

    for (ULONG32 ulIdx = 0; 
         ulIdx < z_cMimeTypeToClockRateAndChannels; 
         ulIdx++)
    {
        if (z_pMimeTypeToClockRateAndChannels[ulIdx].pMimeType &&
            !strcasecmp(
                z_pMimeTypeToClockRateAndChannels[ulIdx].pMimeType,
                pMimeType))
        {
            ulChannels = 
                z_pMimeTypeToClockRateAndChannels[ulIdx].ulChannels;
            break;
        }
    }

    return ulChannels;
}

/****************************************************************************
 *  SDPMapPayloadToHeaderData
 */
const UCHAR* SDPMapPayloadToHeaderData(ULONG32 ulPayloadType,
                                       ULONG32& ulDataSize)
{
    const RTPPayloadHeaderPair* pPair;

    for (pPair = z_PayloadHeaderPairList;
         pPair->pHeader;
         pPair++)
    {
        if (pPair->ulPayloadType == ulPayloadType)
        {
            ulDataSize = pPair->ulHeaderSize;
            return pPair->pHeader;
        }
    }
    
    ulDataSize = 0;
    return NULL;
}
