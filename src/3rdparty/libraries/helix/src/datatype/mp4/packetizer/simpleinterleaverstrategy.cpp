#include "simpleinterleaverstrategy.h"

BEGIN_INTERFACE_LIST_NOCREATE(CSimpleInterleaverStrategy)
    INTERFACE_LIST_ENTRY(IID_IHXPacketizerStrategy, IHXPacketizerStrategy)
END_INTERFACE_LIST

CSimpleInterleaverStrategy::CSimpleInterleaverStrategy(UINT32 N, UINT32 M)
: m_N(N), m_M(M)
{
    Reset() ;
}

UINT32 CSimpleInterleaverStrategy::GetNextPacketIndex()
{
    UINT32 t = m_ulSequenceNumber / m_N ;
    UINT32 t2 = t / m_M ;
    UINT32 r = m_ulSequenceNumber + (t2 - t)  * m_N ;
    m_ulLowMark += (t - t2 * m_M == (m_M-1)) ;

    m_ulSequenceNumber++ ;
    return r ;
}

UINT32 CSimpleInterleaverStrategy::GetLowMark() const 
{
    return m_ulLowMark ;
}

UINT32 CSimpleInterleaverStrategy::GetDeInterleaveBuffersize() const
{
    return 0 ; // XXX TODO
}

CSimpleInterleaverStrategy::~CSimpleInterleaverStrategy()
{
}

void CSimpleInterleaverStrategy::Reset()
{
    m_ulSequenceNumber = 0 ;
	m_ulLowMark = 0 ;
}
