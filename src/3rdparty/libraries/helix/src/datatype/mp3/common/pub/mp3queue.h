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

#ifndef _MP3QUEUE_H_
#define  _MP3QUEUE_H_

#include "hxcom.h"
#include "ihxpckts.h"

typedef struct
{
    IHXPacket* pPacket;    // Origonal packet

    UCHAR*  pHdr;           // Pointer to syncword, side info etc.
    UINT32  ulHdrSize;      // Size of pHdr

    UCHAR*  pData;          // Pointer to mp3 data
    UINT32  ulDataSize;     // Size of mp3 pData

    UINT32  ulFrameSize;    // Size of mp3 frame
    UINT32  ulOffset;       // main_data_begin

    UCHAR*  pBuffer;        // mp3 data
    UINT32  ulSize;         // mp3 frame size
    UINT32  ulTime;         // Time stamp of the frame
} tFrameInfo;

class CMp3Format;

class CMp3Queue
{
public:
    CMp3Queue(CMp3Format* pFmt, int nEntries);
    ~CMp3Queue();

    int             AddEntry(IHXPacket* pPacket);
    tFrameInfo*     GetHead();
    tFrameInfo*     GetIndex(int nIndex);
    tFrameInfo*     Next();
    void            RemoveHead();
    void            RemoveAll();
    
    void            RemoveDataBytes(UINT32 ulBytes) {m_ulDataBytes -= ulBytes;}
        
    UINT32  GetDataBytes()  {return m_ulDataBytes;}
    UINT32  GetEntries()    {return m_ulEntries;}

private:
    UCHAR           GenerateFrameInfo(tFrameInfo* pInfo);


    UINT32      m_ulDataBytes,          // How much mp3 data is in queue
                m_ulQueueSize,          // How many entries can the queue hold
                m_ulEntries,            // How many entries are in the queue
                m_ulHead,               // Index of the head
                m_ulAddIndex,           // Index of the next added entry
                m_ulHeadDataRemnant,    // main_data_begin bytes left over from head
                m_ulNext;               // Index of entry for Next call

    tFrameInfo  *m_pQueue;              // The queue itself

    CMp3Format  *m_pFmt;                // Used in AddEntry
};

#endif
