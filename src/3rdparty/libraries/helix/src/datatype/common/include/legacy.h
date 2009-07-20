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

/*
 * This is generated code, do not modify. Look in legacy.pm to
 * make modifications
 */
#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

typedef char*	pmc_string;

struct buffer {
	 UINT32	len;
	 INT8*	data;
};
#endif/*PMC_PREDEFINED_TYPES*/


#ifndef _LEGACY_
#define _LEGACY_

#include "hxtypes.h"

#if (!defined(_BEOS))
#define int8    char
#define int16   INT16
#define int32   LONG32
#endif
#define u_int8  UCHAR
#define u_int16 UINT16
#define u_int32 ULONG32

#define AUDIO_PCMHEADER_MAGIC_NUMBER    0x4350
#define AUDIO_WAVHEADER_MAGIC_NUMBER    0x7677


//This is the structure expected in the stream header for
// the x-pn-wav renderer (and now in aurendr, too, to which
// RTP-payload PCM audio gets sent):
//
class AudioPCMHEADER
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 16;}

    UINT16	usVersion;
    UINT16	usMagicNumberTag;
    UINT16	usFormatTag;
    UINT16	usChannels;
    UINT32	ulSamplesPerSec;
    UINT16	usBitsPerSample;
    UINT16	usSampleEndianness;
};

inline UINT8*
AudioPCMHEADER::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off++ = (UINT8) (usVersion>>8); *off++ = (UINT8) (usVersion);}
    {*off++ = (UINT8) (usMagicNumberTag>>8); *off++ = (UINT8) (usMagicNumberTag);}
    {*off++ = (UINT8) (usFormatTag>>8); *off++ = (UINT8) (usFormatTag);}
    {*off++ = (UINT8) (usChannels>>8); *off++ = (UINT8) (usChannels);}
    {
	*off++ = (UINT8) (ulSamplesPerSec>>24); *off++ = (UINT8) (ulSamplesPerSec>>16);
	*off++ = (UINT8) (ulSamplesPerSec>>8); *off++ = (UINT8) (ulSamplesPerSec);
    }
    {*off++ = (UINT8) (usBitsPerSample>>8); *off++ = (UINT8) (usBitsPerSample);}
    {*off++ = (UINT8) (usSampleEndianness>>8); *off++ = (UINT8) (usSampleEndianness);}
    len = off-buf;
    return off;
}

inline UINT8*
AudioPCMHEADER::unpack(UINT8* buf, UINT32 len)
{
    if (len <= 0)
	return 0;
    UINT8* off = buf;

    {usVersion = *off++<<8; usVersion |= *off++;}
    {usMagicNumberTag = *off++<<8; usMagicNumberTag |= *off++;}
    {usFormatTag = *off++<<8; usFormatTag |= *off++;}
    {usChannels = *off++<<8; usChannels |= *off++;}
    {
	ulSamplesPerSec = ((UINT32)*off++)<<24; ulSamplesPerSec |= ((UINT32)*off++)<<16;
	ulSamplesPerSec |= ((UINT32)*off++)<<8; ulSamplesPerSec |= ((UINT32)*off++);
    }
    {usBitsPerSample = *off++<<8; usBitsPerSample |= *off++;}
    {usSampleEndianness = *off++<<8; usSampleEndianness |= *off++;}
    return off;
}

//This is essentially a windows WAVEFORMATEX structure.  It is
// expected by the x-pn-windows-acm renderer.
//
class AudioWAVEHEADER
{
public:
    UINT8*	pack(UINT8* buf, UINT32 &len);
    UINT8*	unpack(UINT8* buf, UINT32 len);
    const UINT32	static_size() {return 22;}

    UINT16	usVersion;
    UINT16	usMagicNumberTag;
    UINT16	usFormatTag;
    UINT16	usChannels;
    UINT32	ulSamplesPerSec;
    UINT32	ulAvgBytesPerSec;
    UINT16	usBlockAlign;
    UINT16	usBitsPerSample;
    UINT16	usCbSize;
};

inline UINT8*
AudioWAVEHEADER::pack(UINT8* buf, UINT32 &len)
{
    UINT8* off = buf;

    {*off++ = (UINT8) (usVersion>>8); *off++ = (UINT8) (usVersion);}
    {*off++ = (UINT8) (usMagicNumberTag>>8); *off++ = (UINT8) (usMagicNumberTag);}
    {*off++ = (UINT8) (usFormatTag>>8); *off++ = (UINT8) (usFormatTag);}
    {*off++ = (UINT8) (usChannels>>8); *off++ = (UINT8) (usChannels);}
    {
	*off++ = (UINT8) (ulSamplesPerSec>>24); *off++ = (UINT8) (ulSamplesPerSec>>16);
	*off++ = (UINT8) (ulSamplesPerSec>>8); *off++ = (UINT8) (ulSamplesPerSec);
    }
    {
	*off++ = (UINT8) (ulAvgBytesPerSec>>24); *off++ = (UINT8) (ulAvgBytesPerSec>>16);
	*off++ = (UINT8) (ulAvgBytesPerSec>>8); *off++ = (UINT8) (ulAvgBytesPerSec);
    }
    {*off++ = (UINT8) (usBlockAlign>>8); *off++ = (UINT8) (usBlockAlign);}
    {*off++ = (UINT8) (usBitsPerSample>>8); *off++ = (UINT8) (usBitsPerSample);}
    {*off++ = (UINT8) (usCbSize>>8); *off++ = (UINT8) (usCbSize);}
    len = off-buf;
    return off;
}

inline UINT8*
AudioWAVEHEADER::unpack(UINT8* buf, UINT32 len)
{
    if (len <= 0)
	return 0;
    UINT8* off = buf;

    {usVersion = *off++<<8; usVersion |= *off++;}
    {usMagicNumberTag = *off++<<8; usMagicNumberTag |= *off++;}
    {usFormatTag = *off++<<8; usFormatTag |= *off++;}
    {usChannels = *off++<<8; usChannels |= *off++;}
    {
	ulSamplesPerSec = ((UINT32)*off++)<<24; ulSamplesPerSec |= ((UINT32)*off++)<<16;
	ulSamplesPerSec |= ((UINT32)*off++)<<8; ulSamplesPerSec |= ((UINT32)*off++);
    }
    {
	ulAvgBytesPerSec = ((UINT32)*off++)<<24; ulAvgBytesPerSec |= ((UINT32)*off++)<<16;
	ulAvgBytesPerSec |= ((UINT32)*off++)<<8; ulAvgBytesPerSec |= ((UINT32)*off++);
    }
    {usBlockAlign = *off++<<8; usBlockAlign |= *off++;}
    {usBitsPerSample = *off++<<8; usBitsPerSample |= *off++;}
    {usCbSize = *off++<<8; usCbSize |= *off++;}
    return off;
}

#endif

