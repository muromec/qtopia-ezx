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

#ifndef HXAMRPACK_H
#define HXAMRPACK_H

// Forward declarations
typedef _INTERFACE IHXCommonClassFactory IHXCommonClassFactory;
typedef _INTERFACE IHXValues             IHXValues;
typedef _INTERFACE IHXPacket             IHXPacket;
class CHXSimpleList;

#include "hxformt.h"
#include "amr_flavor.h"

#define DEFAULT_MAX_PACKET_SIZE 600
#define DEFAULT_AVG_PACKET_SIZE 500
#define DEFAULT_MIN_PACKET_SIZE 400

class CHXAMRPayloadFormatPacketizer : public IHXPayloadFormatObject
{
public:
    CHXAMRPayloadFormatPacketizer();
    virtual ~CHXAMRPayloadFormatPacketizer();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXPayloadFormatObject methods
    STDMETHOD(Init)             (THIS_ IUnknown* pContext, HXBOOL bPacketize);
    STDMETHOD(Close)            (THIS);
    STDMETHOD(Reset)            (THIS);
    STDMETHOD(SetStreamHeader)  (THIS_ IHXValues* pHeader);
    STDMETHOD(GetStreamHeader)  (THIS_ REF(IHXValues*) pHeader);
    STDMETHOD(SetPacket)        (THIS_ IHXPacket* pPacket);
    STDMETHOD(GetPacket)        (THIS_ REF(IHXPacket*) pOutPacket);
    STDMETHOD(Flush)            (THIS);

    // CHXAMRPayloadFormatPacketizer methods
    UINT32 GetPacketBytesConsumed() { return m_ulPacketBytesConsumed; }
    UINT32 GetDurationConsumed()    { return m_ulDurationConsumed;    }
private:
    INT32                  m_lRefCount;
    IHXCommonClassFactory* m_pCCF;
    CHXSimpleList*         m_pOutQueue;
    UINT32                 m_ulMinPacketSize;
    UINT32                 m_ulAvgPacketSize;
    UINT32                 m_ulMaxPacketSize;
    UINT32                 m_ulPacketBytesConsumed;
    UINT32                 m_ulDurationConsumed;
    AMRFlavor              m_flavor;
    HX_BITFIELD            m_bFlush : 1;

    void   ClearOutputQueue();
    HXBOOL   ProcessInputPacket(IHXPacket* pPacket, HXBOOL bFlush, UINT32 ulMinSize,
                              REF(UINT32) rulBytesConsumed, REF(UINT32) rulTimeConsumed);
    HXBOOL   FindAMRFrameLength(BYTE* pBuf, UINT32 ulLen, REF(UINT32) rulLen, REF(UINT32) rulDur);
    HXBOOL   FindAllAMRFramesLength(BYTE* pBuf, UINT32 ulLen, UINT32 ulMinSize,
                                  REF(UINT32) rulLen, REF(UINT32) rulDur);
    HXBOOL   CreateAndQueuePacket(BYTE* pBuf, UINT32 ulLen, UINT32 ulTimeStamp,
                                UINT16 usStream, UINT8 ucASMFlags, UINT16 usRuleNum);
    UINT32 GetProp(IHXValues* pValues, const char* pszName);
};

#endif  // HXAMRPACK_H
