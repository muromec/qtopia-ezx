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
#ifndef _PKTREORDERQUEUE_H_
#define _PKTREORDERQUEUE_H_


#include "seqno.h"
#include "timeval.h"
#include "hxdeque.h"

class CQueueEntry  : public IUnknown
{
public:
    CQueueEntry(IHXServerPacketExt* pPacket = NULL, INT32 lInitialRefCount = 0);
    virtual ~CQueueEntry();
    
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    INT32		m_lRefCount;
    Timeval             m_tTime;
    IHXServerPacketExt* m_pPacket;
    SequenceNumber	m_SequenceNumber;
};

inline
CQueueEntry::CQueueEntry(IHXServerPacketExt* pPacket, INT32 lInitialRefCount)
    : m_lRefCount(lInitialRefCount)
    , m_pPacket(pPacket)
    , m_tTime(0.0)
    , m_SequenceNumber(0)
{
    HX_ADDREF(m_pPacket);
}

inline 
CQueueEntry::~CQueueEntry()
{
    HX_RELEASE(m_pPacket);
}
 
inline STDMETHODIMP_(ULONG32)
CQueueEntry::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

inline STDMETHODIMP
CQueueEntry::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

inline STDMETHODIMP_(ULONG32)
CQueueEntry::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


class CInorderPacketQueue
{
public:
    CInorderPacketQueue();
    virtual ~CInorderPacketQueue();

    inline UINT32   size();
    inline HXBOOL   empty();

    void clear();
    inline CQueueEntry*     peek_front();
    inline CQueueEntry*     peek_back();
    inline CQueueEntry*     peek(SequenceNumber& SeqNo);
    inline CQueueEntry*     peek(UINT32 uIndex);
    inline CQueueEntry*     pop_front();
    HX_RESULT               add(CQueueEntry* pEntry, SequenceNumber& SeqNo);

    inline UINT32           GetFirstSequenceNumber();
    inline UINT32           GetPacketIndex(SequenceNumber& SeqNo);
    inline void             ResetFirstSequenceNumber();

protected:
    // must be implemented by a drived class
    virtual CQueueEntry*    CreateLostPacket(SequenceNumber& SeqNo, Timeval& tv);

    HX_deque*               m_pPacketDeque;
    SequenceNumber          m_FirstSequenceNumber;
    BOOL                    m_bSetFirstSequenceNumber;
};

inline UINT32
CInorderPacketQueue::GetFirstSequenceNumber()
{
    return m_FirstSequenceNumber;
}

inline UINT32
CInorderPacketQueue::GetPacketIndex(SequenceNumber& SeqNo)
{
    return(SeqNo - m_FirstSequenceNumber);
}

inline void 
CInorderPacketQueue::ResetFirstSequenceNumber()
{
    m_bSetFirstSequenceNumber = FALSE;
}

inline UINT32
CInorderPacketQueue::size()
{
    return (UINT32)m_pPacketDeque->size();
}

inline BOOL
CInorderPacketQueue::empty()
{
    return m_pPacketDeque->empty();
}

inline CQueueEntry*
CInorderPacketQueue::peek_front()
{
    return !m_pPacketDeque->empty() ? (CQueueEntry*)m_pPacketDeque->front() : NULL;
}

inline CQueueEntry*
CInorderPacketQueue::peek_back()
{
    return !m_pPacketDeque->empty() ? (CQueueEntry*)m_pPacketDeque->back() : NULL;
}

inline CQueueEntry*
CInorderPacketQueue::peek(SequenceNumber& SeqNo)
{
    UINT32 index = GetPacketIndex(SeqNo);

    if (index >= (UINT32)m_pPacketDeque->size())
    {
        return NULL;
    }

    return (CQueueEntry*)(*m_pPacketDeque)[index];
}

inline CQueueEntry*
CInorderPacketQueue::peek(UINT32 uIndex)
{
    if (uIndex >= (UINT32)m_pPacketDeque->size())
    {
        return NULL;
    }

    return (CQueueEntry*)(*m_pPacketDeque)[uIndex];
}

inline CQueueEntry*
CInorderPacketQueue::pop_front()
{
    m_FirstSequenceNumber++;
    return !m_pPacketDeque->empty() ? (CQueueEntry*)m_pPacketDeque->pop_front() : NULL;
}

inline CQueueEntry*
CInorderPacketQueue::CreateLostPacket(SequenceNumber& SeqNo, Timeval& tvTime)
{
    // we cheat and initialize the entry with RefCnt set to 1
    CQueueEntry* pTmpEntry = new CQueueEntry(NULL, 1);
    
    pTmpEntry->m_SequenceNumber = SeqNo;
    pTmpEntry->m_tTime = tvTime;

    return pTmpEntry;
}

#endif /* _PKTREORDERQUEUE_H_ */
