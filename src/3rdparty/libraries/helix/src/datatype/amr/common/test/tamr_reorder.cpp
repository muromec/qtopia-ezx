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

#include <stdio.h>

#include "amr_reorder.h"
#include "amr_frame_info.h"

bool Compare(const UINT8* pControl, const UINT8* pTest, int frameType)    
{
    bool ret = true;

    int frameBytes = (CAMRFrameInfo::FrameBits(frameType) + 7) >> 3;

    for (int j = 0; j < frameBytes; j++)
    {
	if (pTest[j] != pControl[j])
	{
	    UINT8 delta = pTest[j] ^ pControl[j];

	    unsigned int bitID = j * 8;

	    for (int k = 0; k < 8; k++)
	    {
		if ((delta & 0x80) &&
		    (bitID < CAMRFrameInfo::FrameBits(frameType)))
		{
		    printf ("%d is wrong\n", bitID);
		    ret = false;
		}

		delta <<= 1;
		bitID++;
	    }
	}
    }

    return ret;
}

int main(int argc, char* argv[])
{

    if (argc < 2)
    {
	printf("Usage: %s <frame type>\n", argv[0]);
	return -1;
    }

    int frameType = atoi(argv[1]);

    int maxBytes = (CAMRFrameInfo::MaxFrameBits() + 7 )  >> 3;
    UINT8* pControl = new UINT8[maxBytes];
    UINT8* pTest = new UINT8[maxBytes];

    int frameBytes = (CAMRFrameInfo::FrameBits(frameType) + 7) >> 3;

    bool ret = true;

    for (int i = 0; (i < 10) && ret; i++)
    {
	printf ("Round %d\n",i);

	for (int j = 0; j < frameBytes; j++)
	    pControl[j] = ::rand() & 0xff;
	
	::memcpy(pTest, pControl, frameBytes); /* Flawfinder: ignore */
	
	CAMRBitReorder reorder;

	reorder.ToNetwork(frameType, pTest);
	
	//Compare(pControl, pTest, frameType);
	
	reorder.ToCodec(frameType, pTest);
	
	ret = Compare(pControl, pTest, frameType);
    }

    delete [] pControl;
    delete [] pTest;

    if (ret)
	printf("Unit test PASSED\n");
    else
	printf("Unit test FAILED\n");

    return 0;
}
