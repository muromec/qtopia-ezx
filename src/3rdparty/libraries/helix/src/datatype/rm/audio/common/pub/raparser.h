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

#ifndef _RAPARSER_H_
#define _RAPARSER_H_

// stream parameters class
class CStreamParam
{
    public:
    CStreamParam();
    ~CStreamParam();
    HX_RESULT ReadOneRAHeader (Byte* buf, UINT32 bLength, UINT32* pBytesRead = NULL);

    UINT16	uHeaderBytes;		// size of raheader info
    UINT16	uFlavorIndex;		// compression type
    UINT32	ulGranularity;		// size of one block of encoded data
    UINT32	ulTotalBytes;		// total bytes of ra data
    UINT32	ulBytesPerMin;		// data rate of encoded and interleaved data
    UINT32	ulBytesPerMin2;		// if IsInterleaved, the data rate of non-interleaved data
				    // else if !IsInterleaved, the data rate of the interleaved data
    UINT16	uInterleaveFactor;	// number of blocks per superblock
    UINT16	uInterleaveBlockSize;	// size of each interleave block
    UINT16	uCodecFrameSize;		// size of each audio frame
    UINT8	ScatterType;			// the interleave pattern type 0==cyclic,1==pattern
    UINT16*	interleavePattern;		// the pattern of interleave if not cyclic
    UINT32	ulUserData;		// extra field for user data
    UINT32	ulSampleRate;		// sample rate of decoded audio
    UINT32	ulActualSampleRate;		// sample rate of decoded audio
    UINT16	uSampleSize;		// bits per sample in decoded audio
    UINT16	uChannels;		// number of audio channels in decoded audio
    char*	interleaverID;		// name of interleaver
    char*	codecID;		// name of codec
    Byte	IsInterleaved;		// TRUE if file has been interleaved
    Byte	CopyByte;		// copy enable byte, if 1 allow copies (SelectiveRecord)
    Byte	StreamType;		// i.e. LIVE_STREAM, FILE_STREAM
    UINT32	ulOpaqueDataSize;	// size of the codec specific data
    UINT8*	opaqueData;			// codec specific data
    char*	sTitle;			// title of clip
    char*	sAuthor;		// author of clip
    char*	sCopyright;		// copyright notice
    char*	sAppString;		// app user string
    
    private:
    HX_RESULT ReadFormat5Header (Byte* buf, UINT32 bLength, UINT32* pBytesRead = NULL);
    HX_RESULT ReadFormat4Header (Byte* buf, UINT32 bLength, UINT32* pBytesRead = NULL);
    HX_RESULT ReadFormat3Header (Byte* buf, UINT32 bLength, UINT32* pBytesRead = NULL);
    char*   ra_set_text(Byte* theText, unsigned char length);
};

// parse headers

#endif // _RAPARSER_H_
