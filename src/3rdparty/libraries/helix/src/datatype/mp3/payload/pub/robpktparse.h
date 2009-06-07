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

#ifndef _ROBPKTPARSE_H_
#define _ROBPKTPARSE_H_

// Forward declarations
class CHXTimestampConverter;

const int MAX_FRAMES = 0x100; //max frames per cycle

struct SFrameData
{
    IHXBuffer* pPacketBuf;
    UINT32      ulOffset;    
    HXBOOL        bCont;

    SFrameData* pNext;

    ~SFrameData() { HX_DELETE(pNext); HX_RELEASE(pPacketBuf); }
};

struct SFrameInfo
{    
    int         nIndex;
    UINT32      ulADUSize;
    SFrameData* pFrameData;   
    double      dTime;
    HXBOOL        bComplete;

    ~SFrameInfo() { HX_DELETE(pFrameData); }
};

struct SCycle
{   
    SFrameInfo* pFrames[MAX_FRAMES];
    int         nCycleIndex;
    int         nMaxIndex;
    double      dStartTime;

    SCycle*     pNext;
    SCycle*     pLast;

    SCycle(int nCycle) : nCycleIndex(nCycle), nMaxIndex(0),
                                 dStartTime(-1), pNext(NULL), pLast(NULL)
    {
        memset(pFrames, 0, sizeof(SFrameInfo*)*MAX_FRAMES); 
    }
    ~SCycle() { for(int i=0;i<=nMaxIndex;i++) { HX_DELETE(pFrames[i]); }}
};

/*******************************************************
 * CCycleList: A simple doubly linked list of SCycle *s
 * Only defines the methods we need.
 */
class CCycleList
{
public:
    CCycleList() : m_pHead(NULL), m_pTail(NULL), m_nCount(0) {}
    ~CCycleList() { DeleteAll(); }

    inline SCycle*  GetHead()               { return m_pHead; }
    inline SCycle*  GetTail()               { return m_pTail; }
    inline SCycle*  RemoveHead();
    inline void     AddTail(SCycle* pCycle);
    
    inline void     DeleteAll();    
    
    inline HXBOOL     IsEmpty()               { return m_pHead == NULL; }
    inline int      GetCount()              { return m_nCount; }
    
    inline SCycle*  GetNext(SCycle* pCycle) { return pCycle->pNext; }

private:
    SCycle* m_pHead;
    SCycle* m_pTail;

    int     m_nCount;
};

/*
 * CRobustPacketParser: RFC 3119 (audio/mpa-robust)
 */
class CRobustPacketParser : public CPacketParser
{
public:
    CRobustPacketParser();
    virtual ~CRobustPacketParser();

    HX_RESULT   AddPacket   (IHXPacket* pPacket, INT32 streamOffsetTime);
    HX_RESULT   RenderAll   (void);

    void PreSeek            (void);
    void PostSeek	    (UINT32 time);
    
    void EndOfPackets       (void);
protected:    
    UINT32 GetFrameInfo(IHXBuffer* pPacket, UINT32 ulOffset, 
                        double dTime = -1);
    UINT32 GetFrame(UCHAR*& pFrameBuffer, double& dTime, HXBOOL& bLost);  
    
    CHXTimestampConverter* m_pTsConvert;

private:    
    CCycleList      m_Cycles;           // List of cycles.
    UINT32          m_ulIndex;          // Index of next outgoing frame
    HXBOOL            m_bFirstFrame;      // Waiting for first frame
};

inline SCycle* 
CCycleList::RemoveHead() 
{ 
    if(m_pHead == NULL)
    {
        return NULL;
    }
    if(m_pTail == m_pHead)
    {
        m_pTail = NULL;
    }
    
    SCycle* pHead = m_pHead; 
    m_pHead = m_pHead->pNext;
    if(m_pHead != NULL)
    {
        m_pHead->pLast = NULL;
    }
    m_nCount--;
    return pHead;
}

inline void 
CCycleList::AddTail(SCycle* pCycle) 
{ 
    pCycle->pNext = NULL;
    pCycle->pLast = m_pTail;
    if(m_pTail)
    {
        m_pTail->pNext = pCycle;
    }
    else
    {
        m_pHead = pCycle;
    }
    m_pTail = pCycle;
    m_nCount++;
}

inline void 
CCycleList::DeleteAll() 
{ 
    if(m_pHead == NULL)
    {
        m_nCount = 0;
        m_pTail = NULL;
    }
    else
    { 
        SCycle* pCycle = m_pHead;
        m_pHead = pCycle->pNext;
        delete pCycle;
        DeleteAll();
    }
}

#endif //_ROBPKTPARSE_H_
