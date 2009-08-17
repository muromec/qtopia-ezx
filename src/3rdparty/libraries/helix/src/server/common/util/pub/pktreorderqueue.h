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
class HX_deque;

class CQueueEntry  : public IUnknown
{
public:
    CQueueEntry(INT32 lInitialRefCount = 0);
    virtual ~CQueueEntry();
    
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    INT32		m_lRefCount;
    Timeval		m_tTime;
    SequenceNumber	m_SequenceNumber;
};

inline
CQueueEntry::CQueueEntry(INT32 lInitialRefCount)
    : m_lRefCount(lInitialRefCount)
    , m_tTime(0.0)
    , m_SequenceNumber(0)
{
    /* empty */
}
inline 
CQueueEntry::~CQueueEntry()
{
    /* empty */
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

    UINT32                  size();
    BOOL                    empty();

    virtual  void           clear();
    virtual CQueueEntry*    peek_front();
    virtual CQueueEntry*    peek(SequenceNumber& SeqNo);
    virtual CQueueEntry*    peek(UINT32 uIndex);
    virtual CQueueEntry*    pop_front();
    virtual HX_RESULT       add(CQueueEntry* pEntry, SequenceNumber& SeqNo);

    UINT32                  GetFirstSequenceNumber() {return m_FirstSequenceNumber;}
    inline UINT32           GetPacketIndex(SequenceNumber& SeqNo);

protected:
    // must be implemented by a drived class
    virtual CQueueEntry*    CreateLostPacket(SequenceNumber& SeqNo, Timeval& tvTime) = 0;


    HX_deque*               m_pPacketDeque;
    SequenceNumber          m_FirstSequenceNumber;
    BOOL                    m_bSetFirstSequenceNumber;
};

inline UINT32
CInorderPacketQueue::GetPacketIndex(SequenceNumber& SeqNo)
{
    return(SeqNo - m_FirstSequenceNumber);
}

#endif /* _PKTREORDERQUEUE_H_ */
