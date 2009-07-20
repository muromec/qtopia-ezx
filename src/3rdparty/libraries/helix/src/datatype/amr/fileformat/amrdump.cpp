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
#include "hlxclib/memory.h"
#include "hxtypes.h"
#include "amr_frame_info.h"
#include "amr_frame_hdr.h"

#ifdef _AIX
#include "dllpath.h"
ENABLE_MULTILOAD_DLLACCESS_PATHS(amrdump)
#endif

#define AMR_HEADER_READ_SIZE    32
#define AMR_MAX_FRAME_SIZE      60
#define AMR_NUMTYPES            16

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <amrfile>\n", argv[0]);
        return 0;
    }

    FILE* fp = fopen(argv[1], "rb");
    if (fp)
    {
        // Read the file header
        unsigned char ucHdr[AMR_HEADER_READ_SIZE];
        fread(&ucHdr[0], 1, AMR_HEADER_READ_SIZE, fp);
        // Check what kind of header it is
        unsigned char ucMagicSingle[6]     = {0x23,0x21,0x41,0x4d,0x52,0x0a};
        unsigned char ucMagicSingleWB[9]   = {0x23,0x21,0x41,0x4d,0x52,0x2d,0x57,0x42,0x0a};
        unsigned char ucMagicMulti[12]     = {0x23,0x21,0x41,0x4d,0x52,0x5F,0x4D,0x43,0x31,0x2E,0x30,0x0a};
        unsigned char ucMagicMultiWB[15]   = {0x23,0x21,0x41,0x4d,0x52,0x2d,0x57,0x42,0x5F,0x4D,0x43,0x31,0x2E,0x30,0x0a};
        unsigned char ucNumChannelsMap[16] = {0,2,3,4,4,5,6,0,0,0,0,0,0,0,0,0};
        unsigned long ulHeaderSize         = 0;
        unsigned long ulNumChannels        = 0;
        unsigned long ulNextFileOffset     = 0;
        bool          bWideBand            = false;
        bool          bOK                  = true;

        if (!memcmp(ucHdr, ucMagicSingle, sizeof(ucMagicSingle)))
        {
            bWideBand     = false;
            ulNumChannels = 1;
            ulHeaderSize  = sizeof(ucMagicSingle);
        }
        else if (!memcmp(ucHdr, ucMagicSingleWB, sizeof(ucMagicSingleWB)))
        {
            bWideBand     = true;
            ulNumChannels = 1;
            ulHeaderSize  = sizeof(ucMagicSingleWB);
        }
        else if (!memcmp(ucHdr, ucMagicMulti, sizeof(ucMagicMulti)))
        {
            bWideBand     = false;
            ulNumChannels = ucNumChannelsMap[ucHdr[sizeof(ucMagicMulti) + 3] & 0x0F];
            ulHeaderSize  = sizeof(ucMagicMulti) + 4;
        }
        else if (!memcmp(ucHdr, ucMagicMultiWB, sizeof(ucMagicMultiWB)))
        {
            bWideBand     = true;
            ulNumChannels = ucNumChannelsMap[ucHdr[sizeof(ucMagicMultiWB) + 3] & 0x0F];
            ulHeaderSize  = sizeof(ucMagicMultiWB) + 4;
        }
        else
        {
            // This is not an AMR file
            fprintf(stderr, "%s is not an AMR file.\n", argv[1]);
            bOK = false;
        }
        if (bOK)
        {
            // Print out information about the file
            fprintf(stdout, "AMR file %s\n", argv[1]);
            fprintf(stdout, "\tType:       %s\n", (bWideBand ? "Wide-band" : "Narrow-band"));
            fprintf(stdout, "\tChannels:   %lu\n", ulNumChannels);
            fprintf(stdout, "\tHeaderSize: %lu\n", ulHeaderSize);
            // Set the next file offset
            ulNextFileOffset = ulHeaderSize;
            // Loop through, seeking to the next offset, and
            // reading a frame
            unsigned char ucFrame[AMR_MAX_FRAME_SIZE];
            // Set up some strings
            const char* pszTypeStr[AMR_NUMTYPES] = {"4.75kbps", "5.15kbps",  "5.90kbps",  "6.70kbps",
                                                    "7.40kbps", "7.95kbps", "10.20kbps", "12.20kbps",
                                                    "AMR Silence Detection",
                                                    "GSM-EFR Silence Detection",
                                                    "TDMA-EFR Silence Detection",
                                                    "PDC-EFR Silence Detection",
                                                    "Future Use", "Future Use", "Future Use",
                                                    "No data (no transmission/reception)"};
            // Initialize a CAMRFrameHdr object
            CAMRFrameHdr cHdr(bWideBand ? WideBand : NarrowBand);
            // Initialize frame counters
            unsigned long ulTime      = 0;
            unsigned long ulNumFrames = 0;
            // Loop to the end of the file
            while (!feof(fp))
            {
                // Seek to the beginning of speech frames
                fseek(fp, ulNextFileOffset, SEEK_SET);
                // Read enough to hold a frame
                int lNumBytes = fread(&ucFrame[0], 1, AMR_MAX_FRAME_SIZE, fp);
                if (lNumBytes > 0)
                {
                    // Unpack the header
                    unsigned char* pBuf = &ucFrame[0];
                    cHdr.Unpack(pBuf);
                    // Print out the information
                    fprintf(stdout, "Frame %lu at offset %lu:\n", ulNumFrames, ulNextFileOffset);
                    fprintf(stdout, "\tType:     %lu (%s)\n", cHdr.Type(),
                            (cHdr.Type() < AMR_NUMTYPES ? pszTypeStr[cHdr.Type()] : "Unknown"));
                    fprintf(stdout, "\tQuality:  %lu\n", cHdr.Quality());
                    fprintf(stdout, "\tBytes:    %lu\n", cHdr.HdrBytes() + cHdr.DataBytes());
                    fprintf(stdout, "\tTime(ms): %lu\n", ulTime);
                    // Advance the next file offset
                    ulNextFileOffset += cHdr.HdrBytes() + cHdr.DataBytes();
                    // Increment the number of frames
                    ulNumFrames++;
                    // Increment the duration (all AMR
                    // frames are 20ms)
                    ulTime += 20;
                }
            }
        }
        // Close the file
        fclose(fp);
    }

    return 0;
}
