#ifndef _SEQUENTIALSTRATEGY_H_
#define _SEQUENTIALSTRATEGY_H_

#include "hxcom.h"
#include "hxtypes.h"
#include "unkimp.h"
#include "ihxtbase.h"

#include "ihxpacketizer.h"

/* sequential strategy:
   Put AUs into packets in sequential order; try to fill up all
   packets as much as possible.
   */

class CSequentialStrategy
: public CUnknownIMP
, public IHXPacketizerStrategy
{
    DECLARE_UNKNOWN(CSequentialStrategy)
public:
    // returns the packet number that the next AU goes into and increments AU number
    STDMETHOD_(UINT32, GetNextPacketIndex)(THIS) {return 0;}
    // returns the earliest packet that is still being filled up.
    // packets with earlier indices can be sent away.
    STDMETHOD_(UINT32, GetLowMark)(THIS) CONSTMETHOD {return 0;}

    // not sure if we need these. In theory, the interleaver can detect these
    // by doing a trial run. But since supplying these is easy, and the trial run
    // inconvenient at best, let's have these.

    // as defined in the RFC.
    STDMETHOD_(UINT32, GetMaxDisplacement)(THIS) CONSTMETHOD {return 0;}
    // as defined in the RFC.
    STDMETHOD_(UINT32, GetDeInterleaveBuffersize)(THIS) CONSTMETHOD {return 0;} // XXX TODO
    // Max gap in sequence numbers of consecutive AUs in one packet
    STDMETHOD_(UINT32, GetMaxIndexDelta)(THIS) CONSTMETHOD {return 1;}

    // reset the strategy to use it again.
    STDMETHOD_(void, Reset)(THIS) {}
} ;

#endif /* _SEQUENTIALSTRATEGY_H_ */
