#ifndef _CONTINOUSINTERLEAVER_STRATEGY_H_
#define _CONTINOUSINTERLEAVER_STRATEGY_H_

#include "hxcom.h"
#include "hxtypes.h"
#include "unkimp.h"
#include "ihxtbase.h"

#include "ihxpacketizer.h"

/* Continuous interleaving:

  .  .  .  0
  .  .  1  4
  .  2  5  8
  3  6  9 12
  7 10 13 16
 11 14 17  .
 15 18  .  .
 19 .   .  .
 */

class CContinousInterleaverStrategy
: public CUnknownIMP
, public IHXPacketizerStrategy
{
    DECLARE_UNKNOWN(CContinousInterleaverStrategy)
public:
    CContinousInterleaverStrategy(int N = 1) ;

    // returns the packet number that the next AU goes into and increments AU number
    STDMETHOD_(UINT32, GetNextPacketIndex)(THIS) ;
    // returns the earliest packet that is still being filled up.
    // packets with earlier indices can be sent away.
    STDMETHOD_(UINT32, GetLowMark)(THIS) CONSTMETHOD ;

    // not sure if we need these. In theory, the interleaver can detect these
    // by doing a trial run. But since supplying these is easy, and the trial run
    // inconvenient at best, let's have these.

    // as defined in the RFC
    STDMETHOD_(UINT32, GetMaxDisplacement)(THIS) CONSTMETHOD ;
    // as defined in the RFC
    STDMETHOD_(UINT32, GetDeInterleaveBuffersize)(THIS) CONSTMETHOD ;
    // Max gap in sequence numbers of consecutive AUs in one packet
    STDMETHOD_(UINT32, GetMaxIndexDelta)(THIS) CONSTMETHOD {return m_N-1;}

    // reset the strategy to use it again.
    STDMETHOD_(void, Reset)(THIS) {m_ulSequenceNumber=0;}

private:
    UINT32 m_ulSequenceNumber ;
    int m_N ;

    ~CContinousInterleaverStrategy() ;
} ;

#endif /* _CONTINOUSINTERLEAVER_STRATEGY_H_ */
