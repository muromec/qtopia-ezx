/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servpckts.h,v 1.9 2004/10/21 23:35:00 jc Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  

#ifndef _SERVPCKTS_H_
#define _SERVPCKTS_H_

#include "hxmap.h"
#include "ihxpckts.h"
#include "basepkt.h"
#include "mem_cache.h"

/////////////////////////////////////////////////////////////////////////
//  ServerPacket
//  Note: We derive ServerPacket from IHXRTPPacket in order to eliminate
//  the multiple inheritance in ServerPacket derived ServerRTPPacket.
//  The multiple inheritance of ServerRTPPacket would make the IHXPacket
//  cast to ServerPacket (aka "travesty of justice") invalid.
//
class ServerPacket : public IHXRTPPacket
                   , public BasePacket
                   , public IHXBroadcastDistPktExt
                   , public IHXRTPPacketInfo
{
public:
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID riid,
                                 void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXPacket methods
     */
    STDMETHOD(Get)              (THIS_
                                 REF(IHXBuffer*)       pBuffer, 
                                 REF(ULONG32)           ulTime,
                                 REF(UINT16)            uStreamNumber,
                                 REF(UINT8)             unASMFlags,
                                 REF(UINT16)            unASMRuleNumber);

    STDMETHOD_(IHXBuffer*,GetBuffer)   (THIS);
    STDMETHOD_(ULONG32,GetTime)         (THIS);
    STDMETHOD_(UINT16,GetStreamNumber)  (THIS);
    STDMETHOD_(UINT8,GetASMFlags)       (THIS);
    STDMETHOD_(UINT16,GetASMRuleNumber) (THIS);
    STDMETHOD_(BOOL,IsLost)             (THIS);
    STDMETHOD(SetAsLost)                (THIS);

    STDMETHOD(Set)              (THIS_
                                 IHXBuffer*    pBuffer, 
                                 ULONG32        ulTime,
                                 UINT16         uStreamNumber,
                                 UINT8          unASMFlags,
                                 UINT16         unASMRuleNumber);

    /*
     *  IHXRTPPacket methods
     */
    STDMETHOD(GetRTP)           (THIS_
                                 REF(IHXBuffer*)       pBuffer, 
                                 REF(ULONG32)           ulTime,
                                 REF(ULONG32)           ulRTPTime,
                                 REF(UINT16)            uStreamNumber,
                                 REF(UINT8)             unASMFlags,
                                 REF(UINT16)            unASMRuleNumber);

   STDMETHOD_(ULONG32,GetRTPTime)       (THIS);

   STDMETHOD(SetRTP)            (THIS_
                                 IHXBuffer*            pBuffer, 
                                 ULONG32                ulTime,
                                 ULONG32                ulRTPTime,
                                 UINT16                 uStreamNumber,
                                 UINT8                  unASMFlags,
                                 UINT16                 unASMRuleNumber);

    /*
     * IHXBroadcastDistPktExt methods
     */
    STDMETHOD_(UINT32,GetSeqNo)         (THIS);
    STDMETHOD_(UINT32,GetStreamSeqNo)   (THIS);
    STDMETHOD_(BOOL,GetIsLostRelaying)  (THIS);
    STDMETHOD_(BOOL,SupportsLowLatency)    (THIS);
    STDMETHOD_(UINT16,GetRuleSeqNoArraySize) (THIS);
    STDMETHOD_(UINT16*,GetRuleSeqNoArray) (THIS);

    STDMETHOD(SetSeqNo)                 (THIS_ UINT32 ulSeqNo);
    STDMETHOD(SetStreamSeqNo)           (THIS_ UINT32 ulStreamSeqNo);
    STDMETHOD(SetIsLostRelaying)        (THIS_ BOOL   bLostRelay);
    STDMETHOD(SetRuleSeqNoArray)        (THIS_ UINT16* pRuleSeqNoArray,
                                               UINT16 uSize);

    /*
     * IHXRTPPacketInfo
     */
    STDMETHOD_(UINT8, GetVersion)   (THIS); 
    STDMETHOD(GetPaddingBit)	    (THIS_ REF(BOOL)bPadding); 
    STDMETHOD(SetPaddingBit)	    (THIS_ BOOL bPadding);
    STDMETHOD(GetExtensionBit)	    (THIS_ REF(BOOL)bExtension);
    STDMETHOD(SetExtensionBit)	    (THIS_ BOOL bExtension);
    STDMETHOD(GetCSRCCount)	    (THIS_ REF(UINT8)unCSRCCount);
    STDMETHOD(SetCSRCCount)	    (THIS_ UINT8 unCSRCCount);
    STDMETHOD(GetMarkerBit)	    (THIS_ REF(BOOL)bMarker); 
    STDMETHOD(SetMarkerBit)	    (THIS_ BOOL bMarker); 
    STDMETHOD(GetPayloadType)	    (THIS_ REF(UINT8)unPayloadType);
    STDMETHOD(SetPayloadType)	    (THIS_ UINT8 unPayloadType);
    STDMETHOD(GetSequenceNumber)    (THIS_ REF(UINT16)unSeqNo);
    STDMETHOD(SetSequenceNumber)    (THIS_ UINT16 unSeqNo); 
    STDMETHOD(GetTimeStamp)	    (THIS_ REF(UINT32)ulTS); 
    STDMETHOD(SetTimeStamp)	    (THIS_ UINT32 ulTS); 
    STDMETHOD(GetSSRC)		    (THIS_ REF(UINT32)ulSSRC); 
    STDMETHOD(SetSSRC)		    (THIS_ UINT32 ulSSRC); 
    STDMETHOD(GetCSRCList)	    (THIS_ REF(const char*) pulCSRC);
    STDMETHOD(SetCSRCList)	    (THIS_ const char* pCSRCList, UINT32 ulSize);     
    STDMETHOD(GetPadding)	    (THIS_ REF(const char*) pPadding); 
    STDMETHOD(SetPadding)	    (THIS_ const char* pPadding, UINT32 ulSize); 
    STDMETHOD(GetExtension)	    (THIS_ REF(const char*) pExtension); 
    STDMETHOD(SetExtension)	    (THIS_ const char* pExtension, UINT32 ulSize); 
     
    /*
     *  Misc. public methods
     */
    ServerPacket();
    ServerPacket(BOOL bAlreadyHasOneRef);

    MEM_CACHE_MEM

    void			SetMediaTimeInMs(UINT32 ulTS) {m_ulMediaTimeMs = ulTS;}
    UINT32			GetMediaTimeInMs(void) { return m_ulMediaTimeMs; }

    void			SetRuleNumber(UINT16 unRuleNumber) {m_unASMRuleNumber = unRuleNumber;}
    void			EnableIHXRTPPacketInfo(void) {m_bEnableIHXRTPPacketInfo = TRUE;}
    
    // Things Not Related to the IHXPacket or IHXRTPPacket Part
    virtual void                SetPacket(IHXPacket* pPacket);
    virtual IHXPacket*		GetPacket();
    virtual IHXPacket*		PeekPacket();
    virtual UINT32              GetSize();
    virtual BOOL                IsTSD() { return m_bIsTSD; }
    virtual void                SetTSD(BOOL bTSD) { m_bIsTSD = bTSD; }

    UINT32                      m_uASMRuleNumber;
    Timeval                     m_tSendTime;
    BOOL                        m_bSlowData;
    BOOL                        m_bTransportBlocked;
    BOOL                        m_bRateMgrBlocked;

    // Things Related to Broadcast Distribution
    UINT32                      m_ulPacketSequenceNumber;
    UINT32                      m_ulPacketStreamSequenceNumber;
    BOOL                        m_bLostRelaying;


protected:
    /*
     *  Misc. protected methods
     */
    ~ServerPacket();

    inline HX_RESULT _Get(
                       IHXBuffer*  &pBuffer, 
                       ULONG32      &ulTime,
                       UINT16       &uStreamNumber,
                       UINT8        &unASMFlags,
                       UINT16       &unASMRuleNumber);

    inline HX_RESULT _Set(
                       IHXBuffer*  pBuffer, 
                       ULONG32      ulTime,
                       UINT16       uStreamNumber,
                       UINT8        unASMFlags,
                       UINT16       unASMRuleNumber);

    /*
     *  private member vars
     */
    LONG32                      m_lRefCount;
    IHXBuffer*                  m_pBuffer;
    ULONG32                     m_ulTime;
    UINT16                      m_uStreamNumber;
    UINT8                       m_unASMFlags;
    UINT16                      m_unASMRuleNumber;
    BOOL                        m_bIsLost;
    UINT32                      m_ulSize;
    BOOL                        m_bIsTSD;

    UINT32			m_ulMediaTimeMs;

    /*
     * IHXRTPPacketInfo
     */
    BOOL			m_bEnableIHXRTPPacketInfo;
    BOOL			m_bMBit;

    UINT16                      m_uRuleSeqNoArraySize;
    UINT16*                     m_pRuleSeqNoArray;
};


/////////////////////////////////////////////////////////////////////////
//  ServerRTPPacket
//
class ServerRTPPacket : public ServerPacket
{
public:
    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                 REFIID         riid,
                                 void**         ppvObj);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXPacket methods
     */
    STDMETHOD(Set)              (THIS_
                                 IHXBuffer*    pBuffer, 
                                 ULONG32        ulTime,
                                 UINT16         uStreamNumber,
                                 UINT8          unASMFlags,
                                 UINT16         unASMRuleNumber);

    /*
     *  IHXRTPPacket methods
     */
    STDMETHOD(GetRTP)           (THIS_
                                 REF(IHXBuffer*)       pBuffer, 
                                 REF(ULONG32)           ulTime,
                                 REF(ULONG32)           ulRTPTime,
                                 REF(UINT16)            uStreamNumber,
                                 REF(UINT8)             unASMFlags,
                                 REF(UINT16)            unASMRuleNumber);

   STDMETHOD_(ULONG32,GetRTPTime)       (THIS);

   STDMETHOD(SetRTP)            (THIS_
                                 IHXBuffer*            pBuffer, 
                                 ULONG32                ulTime,
                                 ULONG32                ulRTPTime,
                                 UINT16                 uStreamNumber,
                                 UINT8                  unASMFlags,
                                 UINT16                 unASMRuleNumber);
    
    /*
     * Misc. public methods
     */
    ServerRTPPacket()
        : m_ulRTPTime(0)
    {
    }

    ServerRTPPacket(BOOL bAlreadyHasOneRef)
        : ServerPacket(bAlreadyHasOneRef)
        , m_ulRTPTime(0)
    {
    }

    virtual void SetPacket(IHXPacket* pPacket);

protected:
    ~ServerRTPPacket()
    {
    }

    ULONG32                     m_ulRTPTime;
};

#endif /* _SERVPCKTS_H_ */
