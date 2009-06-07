#ifndef _PACKETIZER_H_
#define _PACKETIZER_H_

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "unkimp.h"
#include "ihxtbase.h"

#include "ihxpacketizer.h"

#include <deque>
using namespace std ;

class CBasePacketizer
: public CUnknownIMP
, public IHXPacketizer
{
    DECLARE_UNKNOWN(CBasePacketizer)

public:
    CBasePacketizer(IHXPacketizerStrategy *strategy) ;

    //	*** IHXPacketizer ***
    // push another frame into this packet.
    STDMETHOD(AddSample)(THIS_ IHXTMediaSample* pSample) ;

    // do we have enough data for one entire packet?
    STDMETHOD_(BOOL, IsPacketReady)(THIS) CONSTMETHOD;

    //	Get the size of the next assembled packet,
    //	provided there is one, otherwise return 0.
    STDMETHOD_(UINT32, GetPacketSize)(THIS) CONSTMETHOD;

    // assemble a packet. If not enough data has been collected, return
    // HXR_NO_DATA...?
    STDMETHOD(AssemblePacket)(THIS_ IHXTMediaSample* pSample) ;

    // while we are fragmenting AUs, this returns 1
    STDMETHOD_(BOOL, HasFragments)(THIS) CONSTMETHOD ;

    STDMETHOD_(void, Reset)(THIS) ;


    // the number of bytes in the queue
//    UINT32 getQueueSize() const {return m_ulBytesInQueue;}

    // the duration of packets in the queue
//    HXT_TIME getQueueDuration() const ;

#if 0
    UINT32 getSizeLength() const {return m_SizeLength;}
    UINT32 getIndexLength() const {return 0;}
    UINT32 getIndexDeltaLength() const {return 0;}
    UINT32 getCTSDeltaLength() const {return m_CTSDeltaLength;}
    UINT32 getMTU() const {return m_ulMTU;}

    HX_RESULT setMaxAUSize(UINT32 maxAUSize) ;
    HX_RESULT setMTU(UINT32 mtu) ;
    HX_RESULT setMaxAUDuration(HXT_TIME maxDuration, BOOL needCTS = FALSE) ;
#endif

protected:
    HXT_MAKE_SMART_PTR(IHXPacketizerStrategy) ;
    
    IHXPacketizerStrategyPtr m_spStrategy ;
    unsigned long m_ulLowMark, m_ulHighMark, m_ulSequenceNumber ;

    // the specific packetizer implements this.
    virtual IHXPacketizer* newPacket(void) const = 0 ;

    HXT_MAKE_SMART_PTR(IHXPacketizer) ;
    deque<IHXPacketizerPtr> m_q ;

    ~CBasePacketizer() ;
} ;

#endif /*_PACKETIZER_H_ */
