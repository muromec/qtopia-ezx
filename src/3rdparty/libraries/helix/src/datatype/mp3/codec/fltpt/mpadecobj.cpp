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

#include "statname.h"

#include "hlxclib/string.h"
#include "mpadecl1.h"
#include "mpadecl2.h"
#include "mpadecl3.h"
#include "mpadecobjfltpt.h"

CMpaDecObj::CMpaDecObj()
 :  m_pDec(NULL),
    m_pDecL1(NULL),
    m_pDecL2(NULL),
    m_pDecL3(NULL),
    m_bTrustPackets(0),
    m_bUseFrameSize(0)
{
}

CMpaDecObj::~CMpaDecObj()
{
    if (m_pDecL1)
    {
#if defined (HELIX_FEATURE_AUDIO_MPA_LAYER1)
        delete m_pDecL1;
#endif //HELIX_FEATURE_AUDIO_MPA_LAYER1
        m_pDecL1 = NULL;
    }

    if (m_pDecL2)
    {
#if defined (HELIX_FEATURE_AUDIO_MPA_LAYER2)
        delete m_pDecL2;
#endif //HELIX_FEATURE_AUDIO_MPA_LAYER2
        m_pDecL2 = NULL;
    }

    if (m_pDecL3)
    {
#if defined (HELIX_FEATURE_AUDIO_MPA_LAYER3)
        delete m_pDecL3;
#endif //HELIX_FEATURE_AUDIO_MPA_LAYER3
        m_pDecL3 = NULL;
    }
}

int CMpaDecObj::Init_n(unsigned char *pSync,
                       unsigned long ulSize,
                       unsigned char bUseSize)
{
    MPEG_HEAD h;

    memset(&h, 0, sizeof(h));
    int nFrameSize = head_info(pSync, ulSize, &h, m_bTrustPackets);

    if (!nFrameSize)
        return nFrameSize;

    switch (h.option)
    {
        case 3:
#if defined (HELIX_FEATURE_AUDIO_MPA_LAYER1)
            m_pDecL1 = new CMpaDecoderL1();
#endif //HELIX_FEATURE_AUDIO_MPA_LAYER1

            m_pDec = m_pDecL1;
            break;
    
        case 2:
#if defined (HELIX_FEATURE_AUDIO_MPA_LAYER2)
            m_pDecL2 = new CMpaDecoderL2();
#endif //HELIX_FEATURE_AUDIO_MPA_LAYER2

            m_pDec = m_pDecL2;
            break;
    
        case 1:
#if defined (HELIX_FEATURE_AUDIO_MPA_LAYER3)
            m_pDecL3 = new CMpaDecoderL3();
#endif //HELIX_FEATURE_AUDIO_MPA_LAYER3

            m_pDec = m_pDecL3;
            break;
    }
    
    if (!m_pDec)
        return 0;

    m_bUseFrameSize = bUseSize;
    return m_pDec->audio_decode_init(&h, nFrameSize, 0, 0, 0, 24000, (int)m_bUseFrameSize);
}

void CMpaDecObj::DecodeFrame_v(unsigned char *pSource,
              	               unsigned long *pulSize,
                   	           unsigned char *pPCM,
                       	       unsigned long *pulPCMSize)
{
    IN_OUT io = m_pDec->audio_decode(pSource, pPCM, m_bUseFrameSize ? *pulSize : 0);

    *pulSize = io.in_bytes;
    *pulPCMSize = io.out_bytes;
}

void CMpaDecObj::GetPCMInfo_v(unsigned long &ulSampRate,
                              int &nChannels,
                              int &nBitsPerSample)
{
    m_pDec->GetPCMInfo_v(ulSampRate, nChannels, nBitsPerSample);
}

int CMpaDecObj::GetSamplesPerFrame_n()
{
    return m_pDec->GetSamplesPerFrame_n();
}



