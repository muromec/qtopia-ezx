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

#ifndef AUDHEAD_H
#define AUDHEAD_H

// Some Commonly used Headers


typedef struct tAuHeader
{
    UINT16      usFormatTag;         /* format type */
    UINT16      usChannels;          /* number of channels (i.e. mono, stereo..*/
    UINT32      ulSamplesPerSec;     /* sample rate */
} AuHeader;

//
// this is the structure expected in the stream header for
// the x-pn-wav renderer.
//
typedef struct tPCMHEADER
{
    UINT16      usFormatTag;         /* format type */
    UINT16      usChannels;          /* number of channels (i.e. mono, stereo..*/
    UINT32      ulSamplesPerSec;     /* sample rate */
    UINT16      usBitsPerSample;     /* number of bits per sample of mono data*/
    UINT16	usSampleEndianness;  /* for 16-bit samples, 0==little, 1==big */
} PCMHEADER;

//
// this is essentially a windows WAVEFORMATEX structure.  It is
// expected by the x-pn-windows-acm renderer.
//

typedef struct tWAVEHEADER
{
    UINT16      usFormatTag;         /* format type */
    UINT16      usChannels;          /* number of channels (i.e. mono, stereo..*/
    UINT32      ulSamplesPerSec;     /* sample rate */
    UINT32      ulAvgBytesPerSec;    /* for buffer estimation */
    UINT16      usBlockAlign;        /* block size of data */
    UINT16      usBitsPerSample;     /* number of bits per sample of mono data*/
    UINT16	usCbSize;	     /* number of bits of additional format info */
} WAVEHEADER;
//We need this because byte-packing on some operating systems may cause
// sizeof(WAVEHEADER) to return a larger number than 18 (20 or 24):
#define TRUE_SIZEOF_WAVEHEADER	18 

#endif /* #ifndef AUDHEAD_H */

