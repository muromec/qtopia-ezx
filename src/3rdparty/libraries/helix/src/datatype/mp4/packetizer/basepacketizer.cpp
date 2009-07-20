#include "basepacketizer.h"

#include "mp4gpacketizer.h"

BEGIN_INTERFACE_LIST_NOCREATE(CBasePacketizer)
    INTERFACE_LIST_ENTRY(IID_IHXPacketizer, IHXPacketizer)
END_INTERFACE_LIST

CBasePacketizer::CBasePacketizer(IHXPacketizerStrategy* pStrategy)
: m_spStrategy(pStrategy)
{
    Reset() ;
}

CBasePacketizer::~CBasePacketizer()
{
}

void
CBasePacketizer::Reset()
{
    m_ulLowMark = 0 ;
    m_ulHighMark = 0 ;
    m_ulSequenceNumber = 0 ;
    m_q.clear() ;
}

/*
   push another frame into this packet.

   We maintain a low water mark and a high water mark. Packets below the low
   water mark are ready for send-off. If the strategy wants to access a packet
   that is at or beyond the high water mark, we'll create its packetizer and all preceding.
 */
HX_RESULT CBasePacketizer::AddSample(IHXTMediaSample* pSample)
{
    unsigned long nextPkt = m_spStrategy->GetNextPacketIndex() ;

#if 0
    {
	HXT_TIME t1,t2 ;
	pSample->GetTime(&t1, &t2) ;
	printf("%d: %d (%d - %d)\n", m_ulSequenceNumber, nextPkt, (int)t1, (int)t2) ;
    }
#endif

    pSample->SetSampleField(HXT_FIELD_SEQUENCE_NUMBER, m_ulSequenceNumber++) ;

    // "late" packets go into the first packetizer available.
    if (nextPkt < m_ulLowMark)
	nextPkt = m_ulLowMark ;

    // future packets grow the queue.
    for (; m_ulHighMark <= nextPkt; m_ulHighMark++)
	m_q.push_back(newPacket()) ;

    return m_q[nextPkt-m_ulLowMark]->AddSample(pSample) ;
}

// do we have enough data to spit out one or more packets?
BOOL CBasePacketizer::IsPacketReady() const
{
    // XXX TODO: Do we need the test against empty queue here?
    return !m_q.empty() && (m_q.front()->IsPacketReady() || m_ulLowMark < m_spStrategy->GetLowMark()) ;
}

// assemble a packet. If not enough data has been collected, return
// HXR_NO_DATA

/* Just take the frontmost packet, assemble it and send it off. It's
   packetizer is recycled and appended at the high water mark end of
   the queue. */

HX_RESULT CBasePacketizer::AssemblePacket(IHXTMediaSample* pSample)
{
    HX_RESULT res = HXR_OK ;

    if (m_q.empty())
	return HXR_NO_DATA ;

    if (SUCCEEDED(res))
	res = m_q.front()->AssemblePacket(pSample) ;

    if (res == HXR_NO_DATA || m_q.front()->GetPacketSize() == 0)
    {
	IHXPacketizerPtr p = m_q.front() ;
	m_q.pop_front() ;
	m_ulLowMark++ ;
	m_q.push_back(p) ; // recycle packet
	m_ulHighMark++ ;
    }

    return res ;
}

// while we are fragmenting AUs, this returns TRUE
BOOL CBasePacketizer::HasFragments() const
{
    return !m_q.empty() && m_q.front()->HasFragments() ;
}

UINT32 CBasePacketizer::GetPacketSize() const
{
    return m_q.empty() ? 0 : m_q.front()->GetPacketSize() ;
}
