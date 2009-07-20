%{
////
// $Log: legacy.pm,v $
// Revision 1.1  2003/07/31 21:57:09  ehyche
// Initial import
// Issue number:
// Obtained from:
// Submitted by:
// Reviewed by:
//
// Revision 1.1  2003/01/16 01:10:15  ehyche
// Added copyright notice
//
// Revision 1.1  2003/01/15 18:48:06  ehyche
// helixized wvffplin
//
// Revision 1.1  1998/06/04 18:49:44  ehodge
// PMC stuff for legacy datatypes to solve big/little endian header problems.
//
////
%}
#ifndef _LEGACY_
#define _LEGACY_

#include "pntypes.h"

#define int8    char
#define int16   INT16
#define int32   LONG32
#define u_int8  UCHAR
#define u_int16 UINT16
#define u_int32 ULONG32

#define AUDIO_PCMHEADER_MAGIC_NUMBER    0x4350
#define AUDIO_WAVHEADER_MAGIC_NUMBER    0x7677


%{
//This is the structure expected in the stream header for
// the x-pn-wav renderer (and now in aurendr, too, to which
// RTP-payload PCM audio gets sent):
//
%}
struct AudioPCMHEADER
{
	u_int16 usVersion;
	u_int16 usMagicNumberTag;
    u_int16	usFormatTag;
    u_int16	usChannels;
    u_int32	ulSamplesPerSec;
    u_int16	usBitsPerSample;
    u_int16	usSampleEndianness;
}

%{
//This is essentially a windows WAVEFORMATEX structure.  It is
// expected by the x-pn-windows-acm renderer.
//
%}
struct AudioWAVEHEADER
{
	u_int16 usVersion;
	u_int16 usMagicNumberTag;
    u_int16 usFormatTag;
    u_int16 usChannels;
    u_int32 ulSamplesPerSec;
    u_int32 ulAvgBytesPerSec;
    u_int16 usBlockAlign;
    u_int16 usBitsPerSample;
    u_int16 usCbSize;
}

#endif

