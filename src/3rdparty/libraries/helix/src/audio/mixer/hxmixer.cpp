/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmixer.cpp,v 1.8 2005/03/14 19:43:24 bobclark Exp $
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

#include "hlxclib/memory.h"
 
#include "hxresult.h"
#include "hxtypes.h"
#include "hxmixer.h"
#include "hxcom.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


/************************************************************************
 *  Method:
 *              CHXMixer::MixBuffer()
 *      Purpose:
 *	NOTE: This routine may change the Source Buffer contents in order 
 *	      to multiple it by volume
 */
ULONG32 CHXMixer::MixBuffer
( 
	UCHAR*	pSrc
,	UCHAR*	pDst
,	ULONG32 ulNumBytes
,	HXBOOL	bChannelConvert
,	UINT16	uVolume
,	UINT16  uBitsPerSample
,	HXBOOL&	bIsDestBufferDirty
)
{
    if (!(bIsDestBufferDirty || bChannelConvert || uVolume != 100))
    {
	::memcpy(pDst, pSrc, ulNumBytes); /* Flawfinder: ignore */
	bIsDestBufferDirty = TRUE;
	return ulNumBytes;
    }

    ULONG32 nSamples = 0;
    ULONG32 i,j;

    LONG32 lFixedVolume = (uVolume*256)/100;

    if (bIsDestBufferDirty)
    {
	if ( uBitsPerSample == 16 )
	{
	    short   *dst = (short*)pDst;
	    short   *src = (short*)pSrc;
	    LONG32  lTmp = 0;

	    if ( !bChannelConvert )
	    {
			nSamples = ulNumBytes / 2; 
		for(i = 0, j=0; i < nSamples; i++,j++)
		{
		    lTmp = *(dst+j) + (((LONG32)(lFixedVolume * (*(src+i)))) >> 8);
		    //lTmp = 	 *(dst+j) +   (*(src+i));

		    // clip if need to
		    if ( lTmp > 32767 )
			*(dst+j) = 32767;
		    else if ( lTmp < -32768 )
			*(dst+j) = -32768;
		    else
			*(dst+j) = (short)lTmp;
		}
	    }
	    // Add the sample in again if we are converting this from
	    // mono to stereo.
	    else if ( bChannelConvert )
	    {
		short itmp = 0;
		nSamples = ulNumBytes / 2; 
		for(i = 0, j=0; i < nSamples; i++,j++)
		{
		    lTmp = *(dst+j) + (LONG32) (((LONG32)(lFixedVolume * (*(src+i)))) >> 8);
		    //lTmp = *(dst+j) + (*(src+i));

		    // clip if need to
		    if ( lTmp > 32767 )
			(*(dst+j)) = 32767;
		    else if ( lTmp < -32768 )
			(*(dst+j)) = -32768;
		    else
			(*(dst+j)) = (short)lTmp;

		    // Add this sample in twice for stereo
		    itmp = (*(dst+j));
		    j++;
		    (*(dst+j)) = itmp;
		}									   
	    }
	}
	else if ( uBitsPerSample == 8 )
	{
	    UCHAR* dst = pDst;
	    UCHAR* src = pSrc; 
	    LONG32 uTmp = 0;

	    for(i = 0; i < ulNumBytes; i++)
	    {
		uTmp = (LONG32) *(dst+i) + (LONG32) (((LONG32)(lFixedVolume * (*(src+i)))) >> 8);
		//uTmp = *(dst+i) +  (*(src+i)) ;

		// clip if needed
		if ( uTmp > 255 )
		    *(dst+i) = 255;
		else
		    *(dst+i) = (UCHAR)uTmp;
	    }

	}
    }
    else
    {
	if ( uBitsPerSample == 16 )
	{
	    short   *dst = (short*)pDst;
	    short   *src = (short*)pSrc;
	    LONG32  lTmp = 0;

	    if ( !bChannelConvert )
	    {
			nSamples = ulNumBytes / 2; 
		for(i = 0, j=0; i < nSamples; i++,j++)
		{
		    lTmp = (((LONG32)(lFixedVolume * (*(src+i)))) >> 8);

		    // clip if need to
		    if ( lTmp > 32767 )
			*(dst+j) = 32767;
		    else if ( lTmp < -32768 )
			*(dst+j) = -32768;
		    else
			*(dst+j) = (short)lTmp;
		}
	    }
	    // Add the sample in again if we are converting this from
	    // mono to stereo.
	    else if ( bChannelConvert )
	    {
		short itmp = 0;
		nSamples = ulNumBytes / 2; 
		for(i = 0, j=0; i < nSamples; i++,j++)
		{
		    lTmp = (LONG32) (((LONG32)(lFixedVolume * (*(src+i)))) >> 8);
		    //lTmp = *(dst+j) + (*(src+i));

		    // clip if need to
		    if ( lTmp > 32767 )
			(*(dst+j)) = 32767;
		    else if ( lTmp < -32768 )
			(*(dst+j)) = -32768;
		    else
			(*(dst+j)) = (short)lTmp;

		    // Add this sample in twice for stereo
		    itmp = (*(dst+j));
		    j++;
		    (*(dst+j)) = itmp;
		}									   
	    }
	}
	else if ( uBitsPerSample == 8 )
	{
	    UCHAR* dst = pDst;
	    UCHAR* src = pSrc; 
	    LONG32 uTmp = 0;

	    for(i = 0; i < ulNumBytes; i++)
	    {
		uTmp = (LONG32) (((LONG32)(lFixedVolume * (*(src+i)))) >> 8);
		//uTmp = *(dst+i) +  (*(src+i)) ;

		// clip if needed
		if ( uTmp > 255 )
		    *(dst+i) = 255;
		else
		    *(dst+i) = (UCHAR)uTmp;
	    }

	}
    }

    bIsDestBufferDirty = TRUE;
    return bChannelConvert ? ulNumBytes*2 : ulNumBytes;
}


/************************************************************************
 *  Method:
 *              CHXMixer::ApplyVolume()
 *      Purpose:
 *	NOTE: This routine simply multiplies input buffer by volume 
 */
void CHXMixer::ApplyVolume
( 
	UCHAR*	pSrc
,	ULONG32 ulNumBytes
,	UINT16	uVolume
,	UINT16  uBitsPerSample
)
{
    ULONG32 nSamples = 0;
    ULONG32 i;

    LONG32 lFixedVolume = (uVolume*256)/100;

    if ( uBitsPerSample == 16 )
    {
	short   *dst = (short*)pSrc;
	short   *src = (short*)pSrc;
	LONG32  lTmp = 0;

	nSamples = ulNumBytes / 2; 
	for(i = 0; i < nSamples; i++)
	{
	    lTmp = (((LONG32)(lFixedVolume * (*(src+i)))) >> 8);
	    //lTmp = 	 *(dst+j) +   (*(src+i));

	    // clip if need to
	    if ( lTmp > 32767 )
		*(dst+i) = 32767;
	    else if ( lTmp < -32768 )
		*(dst+i) = -32768;
	    else
		*(dst+i) = (short)lTmp;
	}
    }
    else if ( uBitsPerSample == 8 )
    {
	UCHAR* dst = pSrc;
	UCHAR* src = pSrc; 
	LONG32 uTmp = 0;

	for(i = 0; i < ulNumBytes; i++)
        {
	    uTmp = (LONG32) (((LONG32)(lFixedVolume * (*(src+i)))) >> 8);
	    //uTmp = *(dst+i) +  (*(src+i)) ;

	    // clip if needed
	    if ( uTmp > 255 )
		*(dst+i) = 255;
	    else
		*(dst+i) = (UCHAR)uTmp;
        }
    }

    return;
}


