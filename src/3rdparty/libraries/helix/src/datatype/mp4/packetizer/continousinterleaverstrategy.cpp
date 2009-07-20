#include "continousinterleaverstrategy.h"

BEGIN_INTERFACE_LIST(CContinousInterleaverStrategy)
    INTERFACE_LIST_ENTRY(IID_IHXPacketizerStrategy, IHXPacketizerStrategy)
END_INTERFACE_LIST

CContinousInterleaverStrategy::CContinousInterleaverStrategy(int N)
: m_N(N)
{
	Reset() ;
}

UINT32 CContinousInterleaverStrategy::GetNextPacketIndex()
{
	UINT32 r = m_ulSequenceNumber % m_N + m_ulSequenceNumber / m_N ;
	m_ulSequenceNumber++ ;
	return r ;
}

UINT32 CContinousInterleaverStrategy::GetLowMark() const
{
	return (m_ulSequenceNumber + (m_N-1)) / m_N ;
}

UINT32 CContinousInterleaverStrategy::GetDeInterleaveBuffersize() const
{
	return 0 ; // XXX TODO
}

CContinousInterleaverStrategy::~CContinousInterleaverStrategy()
{
}
