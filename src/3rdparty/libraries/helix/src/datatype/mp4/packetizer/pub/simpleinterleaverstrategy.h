#ifndef _SIMPLEINTERLEAVER_STRATEGY_H_
#define _SIMPLEINTERLEAVER_STRATEGY_H_

#include "hxcom.h"
#include "hxtypes.h"
#include "unkimp.h"
#include "ihxtbase.h"

#include "ihxpacketizer.h"

/* Simple (NxM) interleaving (N=3, M=4)

  P  1  4  7 10
  a  2  5  8 11
  c  3  6  9 12
  k  
  e 13 16 19 22
  t 14 17 20 23
    15 18 21 24
  I
  d  .  .  .  .
  x

  |
  |
  V
*/

class CSimpleInterleaverStrategy
: public CUnknownIMP
, public IHXPacketizerStrategy
{
    DECLARE_UNKNOWN(CSimpleInterleaverStrategy)
public:
    // use N==M==1 to get a "null" interleaver, with one AU per frame
    CSimpleInterleaverStrategy(UINT32 N = 1, UINT32 M = 1) ;

    // returns the packet number that the next AU goes into and increments AU number
    STDMETHOD_(UINT32, GetNextPacketIndex)(THIS) ;
    // returns the earliest packet that is still being filled up.
    // packets with earlier indices can be sent away.
    STDMETHOD_(UINT32, GetLowMark)(THIS) CONSTMETHOD ;

    // not sure if we need these. In theory, the interleaver can detect these
    // by doing a trial run. But since supplying these is easy, and the trial run
    // inconvenient at best, let's have these.

    // as defined in the RFC
    STDMETHOD_(UINT32, GetMaxDisplacement)(THIS) CONSTMETHOD {return (m_M-1)*m_N - 1;}
    // as defined in the RFC
    STDMETHOD_(UINT32, GetDeInterleaveBuffersize)(THIS) CONSTMETHOD ;
    // Max gap in sequence numbers of consecutive AUs in one packet
    STDMETHOD_(UINT32, GetMaxIndexDelta)(THIS) CONSTMETHOD {return m_N;}

    // reset the strategy to use it again.
    STDMETHOD_(void, Reset)(THIS) ;

private:
    UINT32 m_N, m_M ;
    UINT32 m_ulSequenceNumber ;
    UINT32 m_ulLowMark ;

    ~CSimpleInterleaverStrategy() ; // destructor will be called through base class
} ;

#endif /* _SIMPLEINTERLEAVER_STRATEGY_H_ */
