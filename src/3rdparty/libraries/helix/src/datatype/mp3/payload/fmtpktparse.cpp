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

#include "hxtypes.h"
#include "hlxclib/stdio.h"

#include "hxcom.h"              // IUnknown
#include "hxcomm.h"            // IHXCommonClassFactory
#include "ihxpckts.h"           // IHXBuffer, IHXPacket, IHXValues
#include "hxplugn.h"           // IHXPlugin
#include "hxrendr.h"           // IHXRenderer
#include "hxengin.h"           // IHXInterruptSafe
#include "hxcore.h"            // IHXStream
#include "hxausvc.h"           // Audio Services
#include "hxmon.h"             // IHXStatistics
#include "hxupgrd.h"           // IHXUpgradeCollection
#include "hxslist.h"            // CHXSimpleList
#include "carray.h"             // CHXPtrArray

#include "mpadecobj.h"          // MPEG Audio Decoder (selects fixed-pt or floating-pt based on HELIX_CONFIG_FIXEDPOINT)
#include "mp3format.h"          // MP3 formatter

#include "mp3rend.h"            // CRnMp3Ren
#include "pktparse.h"           // CPacketParser
#include "fmtpktparse.h"        // CFmtPacketParser

CFmtPacketParser::CFmtPacketParser() :
    CPacketParser(),
    m_pPacket(NULL)
{
    m_bReformatted = TRUE;
}

CFmtPacketParser::~CFmtPacketParser()
{    
    HX_RELEASE(m_pPacket);
}

HX_RESULT
CFmtPacketParser::AddPacket(IHXPacket* pPacket, INT32 streamOffsetTime)
{
    if(pPacket == NULL)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_RELEASE(m_pPacket);
    m_pPacket = pPacket;
    m_pPacket->AddRef();

    return HXR_OK;
}

HX_RESULT
CFmtPacketParser::RenderAll()
{
    if(!m_pRenderer || !m_pFmt || !m_pPacket)
    {
        return HXR_FAIL;
    }
    
    HXBOOL bPacketLoss = m_pPacket->IsLost();
    double dTime;
    if(!bPacketLoss)
    {
        IHXBuffer* pBufObj = m_pPacket->GetBuffer();
        if(!pBufObj)
        {
            return HXR_FAIL;
        }
        UCHAR* pFrame;
        UINT32 ulSize;

        pBufObj->Get(pFrame, ulSize);
        if(!pFrame || !ulSize)
        {
            HX_RELEASE(pBufObj);
            return HXR_FAIL;
        }
        
        // Copy the data into the decode buffer
        m_ulDecBufBytes = HX_MIN(ulSize, DEC_BUFFER_SIZE);
        memcpy(m_pDecBuffer, pFrame, m_ulDecBufBytes); /* Flawfinder: ignore */
        HX_RELEASE(pBufObj);

        if(!m_pDecoder && !InitDecoder(m_pDecBuffer, m_ulDecBufBytes, FALSE))
        {
            return HXR_FAIL;
        }

        // ClearMainDataBegin() after 
        // the call to InitDecoder(), because the layer needs
        // to be set before we call ClearMainDataBegin().
        m_pFmt->ClearMainDataBegin(m_pDecBuffer);
        
        dTime = m_pPacket->GetTime();
    }
    else
    {
        dTime = m_dNextPts;
    }
    
    if(!m_ulDecBufBytes)
    {
        return HXR_FAIL;
    }

    // If the packet is lost we use the last packet, still in the buffer
    // NOTE: We should probably check the value returned by D&R!
    DecodeAndRender(m_pDecBuffer, m_ulDecBufBytes, dTime, bPacketLoss);
    
    m_dNextPts = dTime + m_dFrameTime;

    return HXR_OK;
}

void
CFmtPacketParser::RestartStream(void)
{
    HX_RELEASE(m_pPacket);    
    m_ulDecBufBytes = 0;
}

