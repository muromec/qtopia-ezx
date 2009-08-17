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

#ifndef _MPADEC_H_
#define _MPADEC_H_

#include "l3.h"
#include "mhead.h"

// generate Layer 3 reformatting functions for test
//#define REFORMAT

///////////////////////////////////////////////////////////////////////////////
// CMpaDecoder is the base class for MPEG audio decoding
///////////////////////////////////////////////////////////////////////////////
class CMpaDecoder
{

public:
    CMpaDecoder();

    ~CMpaDecoder() {}
    


    // standard decode
    //virtual IN_OUT  audio_decode(unsigned char *bs,
    //                             unsigned char *pcm) = 0;
    // Layer 3 reformatted frames w/optional concealment
    // Layer 1/2 standard decode
    virtual IN_OUT  audio_decode(unsigned char *bs,
                                 unsigned char *pcm,
                                 int size) = 0;

    // conceal_enable applies to Layer 3 only
    // ignored for layers 1/2
    virtual int     audio_decode_init(MPEG_HEAD *h,
                                      int framebytes_arg,
                                      int reduction_code = 0,
                                      int transform_code = 0,
                                      int convert_code = 0,
                                      int freq_limit = 24000,
                                      int conceal_enable = 0) = 0;
    
#ifdef REFORMAT
    // Layer3 only
    virtual IN_OUT  audio_decode_reformat(unsigned char *bs,
                                 unsigned char *bs_out) = 0;
#endif
    
    ///////////////////////////////////////////////////////////////////////////
    // Function:    GetPCMInfo_v
    // Purpose:     Retrieves info of the PCM output
    // Parameters:  ulSampRate  will contain the PCM sample rate (ex 441000)
    //              nChannels   will contain the number of PCM channels
    //              nBitsPerSample  will contain the nuber of bits per PCM sample
    ///////////////////////////////////////////////////////////////////////////
    void            GetPCMInfo_v(unsigned long &ulSampRate,
                                int &nChannels,
                                int &nBitsPerSample)
                    {
                        ulSampRate = (unsigned long)decinfo.samprate;
                        nChannels = decinfo.channels;
                        nBitsPerSample = decinfo.bits;
                    }

    int             GetSamplesPerFrame_n() {return m_nSampsPerFrame;};
    
protected:

    DEC_INFO        decinfo;                // Decoder output properties
    unsigned char   m_bMpeg1,               // Is this mpeg1 audio
                    m_bUseFrameSize;        // Use input frame size on decode
    int             m_nSampsPerFrame;       // Number of source samples per frame

    //int table_init_flag;
    int iframe;
    int h_id;
    int nsb_limit;
    int sr_index;
    int outvalues;
    int outbytes;
    int framebytes;
    int padframebytes;
    int crcbytes;
    int pad;
    int nchan;
    int ms_mode;
    int is_mode;
    /* circular window buffers */
    int vb_ptr[2];
    float vbuf[2][512];


    //SBT_FUNCTION sbt_function;



};

#endif
