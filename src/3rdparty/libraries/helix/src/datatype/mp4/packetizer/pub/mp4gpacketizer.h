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

#ifndef _LATMPACKET_H_
#define _LATMPACKET_H_

#include "hxcom.h"
#include "hxtypes.h"
#include "unkimp.h"
#include "ihxtbase.h"
#include "bitpack.h"

#include "ihxpacketizer.h"
#include "basepacketizer.h"

#include <deque>
using namespace std ;

class CMP4GPacket: public CUnknownIMP
, public IHXPacketizer
{
    DECLARE_UNKNOWN(CMP4GPacket)
public:

    typedef struct {
	UINT32 ulMTU ;
	UINT32 ulMaxAUDuration ;
	HXT_TIME tMaxPacketDuration ;
	UINT32 ulMaxAUSize ;
	UINT32 ulSizeLength ;
	UINT32 ulCTSDeltaLength, ulCTSDeltaPresentLength ;
	UINT32 ulIndexLength, ulIndexDeltaLength ;
	BOOL bAllowFragmentation;
    } tConfigParms ;

    CMP4GPacket(const tConfigParms &config) ;

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
    STDMETHOD_(BOOL, HasFragments)(THIS) CONSTMETHOD { return m_bFragmentsPending; };

    // reset the packetizer to start state.
    STDMETHOD_(void, Reset)(THIS) {}

private:
    // methods
    virtual ~CMP4GPacket();
    
    // the duration of packets in the queue
    HXT_TIME GetQueueDuration() const ;
    
    static UINT32 BITS_TO_BYTES(UINT32 bits) {return ((bits + 7) >> 3) ;}
    UINT32 headerSize(UINT32 n) const ;
    
    UINT32 FitPacket(UINT32 &size) const ;

    // MPEG-4 generic packetizer properties
    
    UINT32 GetSizeLength() const {return m_config.ulSizeLength;}
    UINT32 GetIndexLength() const {return m_config.ulIndexLength;}
    UINT32 GetIndexDeltaLength() const {return m_config.ulIndexDeltaLength;}
    UINT32 GetCTSDeltaLength() const {return m_config.ulCTSDeltaPresentLength ? m_config.ulCTSDeltaLength : 0 ;}
    UINT32 GetMTU() const {return m_config.ulMTU;}
    HXT_TIME GetMaxPacketDuration() const {return m_config.tMaxPacketDuration;}
    BOOL GetAllowFragmentation() const { return m_config.bAllowFragmentation; }

    // data
    UINT32 m_ulBytesInQueue ;
    UINT32 m_ulAUOffset ;
    BOOL m_bFragmentsPending ;
    BOOL m_bPacketFull ;

    // configuration parameters
    tConfigParms m_config ;

    // defaults
    enum {
	headerLengthBits = 16
    } ;
    
    HXT_MAKE_SMART_PTR(IHXTMediaSample) ;
    
    deque<IHXTMediaSamplePtr> m_sampleQueue ;
};

HXT_MAKE_CLASS_SMART_PTR(CMP4GPacket) ;

class CMP4GPacketizer
: public CBasePacketizer
{
    DECLARE_UNKNOWN(CMP4GPacketizer)
public:
    CMP4GPacketizer(IHXPacketizerStrategy *pPacketizerStrategy) ;

    // MPEG-4 generic packetizer properties

    UINT32 GetSizeLength() const {return m_config.ulSizeLength;}
    UINT32 GetIndexLength() const {return m_config.ulIndexLength;}
    UINT32 GetIndexDeltaLength() const {return m_config.ulIndexDeltaLength;}
    UINT32 GetCTSDeltaLength() const {return m_config.ulCTSDeltaPresentLength ? m_config.ulCTSDeltaLength : 0 ;}
    UINT32 GetMTU() const {return m_config.ulMTU;}
    BOOL GetAllowFragmentation() const { return m_config.bAllowFragmentation; }

    // return duration if constant, zero otherwise
    UINT32 GetConstantDuration() const ;
    // return size in bytes if constant, zero otherwise
    UINT32 GetConstantSize() const ;

    HX_RESULT SetMTU(UINT32 mtu) ;
    HX_RESULT SetAUSize(UINT32 maxAUSize, BOOL bConstantSize = TRUE) ;
    HX_RESULT SetAUDuration(UINT32 ulMaxDuration, BOOL bConstantDuration = TRUE) ;
    HX_RESULT SetMaxPacketDuration(HXT_TIME tMaxDuration) ;
    HX_RESULT SetAllowFragmentation(BOOL bAllowFragmentation);

private:
    virtual ~CMP4GPacketizer() {}

    // configuration parameters
    CMP4GPacket::tConfigParms m_config ;

    IHXPacketizer *newPacket() const ;
};


#endif /* _LATMPACKET_H_ */
