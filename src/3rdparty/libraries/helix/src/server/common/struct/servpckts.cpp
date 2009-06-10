/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servpckts.cpp,v 1.18 2009/03/17 22:54:57 ckarusala Exp $ 
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

/************************************************************************
 *  Defines
 */
#define EXTREME_RTP_TIME    0xFFFFFFFF
#define EXTREME_MICRO_SEC_TIME ((UINT64) - 1) 


/************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "ihxpckts.h"
#include "servpckts.h"
#include "hxstrutl.h"
#include "hxpiids.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static char HX_THIS_FILE[] = __FILE__;
#endif


/************************************************************************
 *  ServerPacket
 */
ServerPacket::ServerPacket()
    : m_lRefCount(0)
    , m_pBuffer(NULL)
    , m_ulTime(0)
    , m_uStreamNumber(0)
    , m_unASMFlags(0)
    , m_unASMRuleNumber(0)
    , m_bIsLost(FALSE)
    , m_uASMRuleNumber(0)
    , m_bSlowData(0)
    , m_ulSize(0)
    , m_bLostRelaying(FALSE)
    , m_ulPacketSequenceNumber(0)
    , m_ulPacketStreamSequenceNumber(0)
    , m_bIsTSD(0)
    , m_bRateMgrBlocked(FALSE)
    , m_bTransportBlocked(FALSE)
    , m_ulMediaTimeMs(0)
    , m_bEnableIHXRTPPacketInfo(FALSE)
    , m_uRuleSeqNoArraySize(0)
    , m_pRuleSeqNoArray(NULL)
    , m_bIsRTP(FALSE)
{
}

ServerPacket::ServerPacket(BOOL bAlreadyHasOneRef)
    : m_lRefCount(bAlreadyHasOneRef ? 1 : 0)
    , m_pBuffer(NULL)
    , m_ulTime(0)
    , m_uStreamNumber(0)
    , m_unASMFlags(0)
    , m_unASMRuleNumber(0)
    , m_bIsLost(FALSE)
    , m_uASMRuleNumber(0)
    , m_ulSize(0)
    , m_bLostRelaying(FALSE)
    , m_ulPacketSequenceNumber(0)
    , m_ulPacketStreamSequenceNumber(0)
    , m_bIsTSD(0)
    , m_bRateMgrBlocked(FALSE)
    , m_bTransportBlocked(FALSE)
    , m_ulMediaTimeMs(0)
    , m_bEnableIHXRTPPacketInfo(FALSE)
    , m_bMBit(FALSE)
    , m_uRuleSeqNoArraySize(0)
    , m_pRuleSeqNoArray(NULL)
    , m_bIsRTP(FALSE)
{
}

ServerPacket::~ServerPacket()
{
    HX_RELEASE(m_pBuffer);

    HX_DELETE(m_pRuleSeqNoArray);
}


STDMETHODIMP
ServerPacket::QueryInterface(REFIID riid, void** ppvObj)
{
    /* This is a travesty of justice, but it's fast */
    if (ppvObj == (void **)0xffffd00d)
    {
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXPacket))
    {
        AddRef();
        *ppvObj = (IHXPacket*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerPacketExt))
    {
        AddRef();
        *ppvObj = (IHXServerPacketExt*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTPPacketInfo))
    {
        if (m_bEnableIHXRTPPacketInfo)
        {
            AddRef();
            *ppvObj = (IHXRTPPacketInfo*)this;
            return HXR_OK;
        }
        else
        {
            *ppvObj = NULL;
            return HXR_NOINTERFACE;         
        }
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IHXPacket*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ServerPacket::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) ServerPacket::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *      Method:
 *              IHXPacket::Get
 *      Purpose:
 *              TBD
 */
STDMETHODIMP ServerPacket::Get
(
    REF(IHXBuffer*)     pBuffer, 
    REF(ULONG32)        ulTime,
    REF(UINT16)         uStreamNumber,
    REF(UINT8)          unASMFlags,
    REF(UINT16)         unASMRuleNumber
)
{
    return _Get(pBuffer,
                ulTime,
                uStreamNumber,
                unASMFlags,
                unASMRuleNumber);
}

/************************************************************************
 *      Method:
 *              IHXPacket::GetBuffer
 *      Purpose:
 *              TBD
 */
STDMETHODIMP_(IHXBuffer*) ServerPacket::GetBuffer()
{
    // Whenever a COM object is returned out of an
    // interface, it should be AddRef()'d.
    if (m_pBuffer) m_pBuffer->AddRef();
    return m_pBuffer;
}

/************************************************************************
 *      Method:
 *              IHXPacket::GetTime
 *      Purpose:
 *              TBD
 */
STDMETHODIMP_(ULONG32) ServerPacket::GetTime()
{
    return m_ulTime;
}

/************************************************************************
 *      Method:
 *              IHXPacket::GetFlags
 *      Purpose:
 *              TBD
 */
STDMETHODIMP_(UINT8) ServerPacket::GetASMFlags()
{
    return m_unASMFlags;
}

/************************************************************************
 *      Method:
 *              IHXPacket::GetStreamNumber
 *      Purpose:
 *              TBD
 */
STDMETHODIMP_(UINT16) ServerPacket::GetStreamNumber()
{
    return m_uStreamNumber;
}

/************************************************************************
 *      Method:
 *              IHXPacket::GetReliability
 *      Purpose:
 *              TBD
 */
STDMETHODIMP_(UINT16) ServerPacket::GetASMRuleNumber()
{
    return m_unASMRuleNumber;
}

STDMETHODIMP_(BOOL)
ServerPacket::IsLost()
{ 
    return m_bIsLost;
}

STDMETHODIMP 
ServerPacket::SetAsLost()
{
    HX_ASSERT(m_pBuffer == NULL);
    if (m_pBuffer) return HXR_UNEXPECTED;
    m_bIsLost = TRUE;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXPacket::Set
 *      Purpose:
 *              TBD
 */
STDMETHODIMP ServerPacket::Set
(
    IHXBuffer*          pBuffer, 
    ULONG32             ulTime,
    UINT16              uStreamNumber,
    UINT8               unASMFlags,
    UINT16              unASMRuleNumber
)
{
    
    
    return _Set(pBuffer, 
                ulTime,
                uStreamNumber,
                unASMFlags,
                unASMRuleNumber);
}

/************************************************************************
 *      Method:
 *              IHXRTPPacket::Set
 */
STDMETHODIMP ServerPacket::GetRTP                               
(
    REF(IHXBuffer*)     pBuffer, 
    REF(ULONG32)        ulTime,
    REF(ULONG32)        ulRTPTime,
    REF(UINT16)         uStreamNumber,
    REF(UINT8)          unASMFlags,
    REF(UINT16)         unASMRuleNumber
)
{
    HX_ASSERT(HXR_UNEXPECTED == HXR_OK);
    return HXR_UNEXPECTED;
}

/*************************************************************************
 *	    Method:
 *              IHXRTPPacket::GetRTPTime
 */
STDMETHODIMP_(ULONG32) ServerPacket::GetRTPTime()
{
    HX_ASSERT(HXR_UNEXPECTED == HXR_OK);
    return EXTREME_RTP_TIME;
}

/************************************************************************
 *      Method:
 *              IHXRTPPacket::SetRTP
 */
STDMETHODIMP ServerPacket::SetRTP
(
    IHXBuffer*      pBuffer, 
    ULONG32         ulTime,
    ULONG32         ulRTPTime,
    UINT16          uStreamNumber,
    UINT8           unASMFlags,
    UINT16          unASMRuleNumber
)
{
    HX_ASSERT(HXR_UNEXPECTED == HXR_OK);
    return HXR_UNEXPECTED;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetDeliveryTime
 */
STDMETHODIMP ServerPacket::SetDeliveryTime(UINT32 ulTime)
{
    m_ulTime = ulTime;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetRTPTime
 */
STDMETHODIMP ServerPacket::SetRTPTime(UINT32 ulRTPTime)
{
    HX_ASSERT(HXR_UNEXPECTED == HXR_OK);
    return HXR_UNEXPECTED;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetBuffer
 */
STDMETHODIMP ServerPacket::SetBuffer(IHXBuffer* pBuffer)
{
    /* We allow changing the packet info when it is owned
     * by atmost one user.
     */
    if (m_lRefCount > 1)
    {
        HX_ASSERT(FALSE);
        return HXR_UNEXPECTED;
    }

    HX_RELEASE(m_pBuffer);
    m_pBuffer = pBuffer;
    m_ulSize = 0;
    if(m_pBuffer)
    {
        m_pBuffer->AddRef();
        m_ulSize = pBuffer->GetSize();
    }
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetStreamNumber
 */
STDMETHODIMP ServerPacket::SetStreamNumber(UINT16 unStreamNum)
{
    m_uStreamNumber = unStreamNum;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetASMFlags
 */
STDMETHODIMP ServerPacket::SetASMFlags(UINT8 uASMFlags)
{
    m_unASMFlags = uASMFlags;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetASMRuleNumber
 */
STDMETHODIMP ServerPacket::SetASMRuleNumber(UINT16 unRuleNum)
{
    m_unASMRuleNumber = unRuleNum;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::GetMilliSecondTS
 */
UINT32 ServerPacket::GetMilliSecondTS()
{
    return m_ulMediaTimeMs;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetMilliSecondTS
 */
STDMETHODIMP ServerPacket::SetMilliSecondTS(UINT32 ulTimeStamp)
{
    m_ulMediaTimeMs = ulTimeStamp;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::GetMicroSecondTS
 */
UINT64 ServerPacket::GetMicroSecondTS()
{
    HX_ASSERT(HXR_UNEXPECTED == HXR_OK);
    return EXTREME_MICRO_SEC_TIME;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetMicroSecondTS
 */
STDMETHODIMP ServerPacket::SetMicroSecondTS(UINT64 ulTimeStamp)
{
    HX_ASSERT(HXR_UNEXPECTED == HXR_OK);
    return HXR_UNEXPECTED;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::IsRTP
 */
BOOL ServerPacket::IsRTP()
{
    return m_bIsRTP;
}


void
ServerPacket::SetPacket(IHXPacket* pPacket)
{
    if (this == pPacket)
    {
        return;
    }

    IHXBuffer*      pBuffer;
    UINT32          ulTime;
    UINT16          unStreamNumber;
    UINT8           unASMFlags;
    UINT16          unASMRuleNumber;

    pPacket->Get(pBuffer, ulTime, unStreamNumber, unASMFlags,
        unASMRuleNumber);
    _Set(pBuffer, ulTime, unStreamNumber, unASMFlags, unASMRuleNumber);
  
    if (pPacket->IsLost())
    {
        this->SetAsLost();
    }
  
    HX_RELEASE(pBuffer);
}

IHXPacket*
ServerPacket::GetPacket()
{
    AddRef();

    return this;
}

IHXPacket*
ServerPacket::PeekPacket()
{
    /* This function violates COM Reference Rules */

    return this;
}

UINT32
ServerPacket::GetSize()
{
    return m_ulSize;
}

inline HX_RESULT ServerPacket::_Get(IHXBuffer*  &pBuffer,
                                    ULONG32     &ulTime,
                                    UINT16      &uStreamNumber,
                                    UINT8       &unASMFlags,
                                    UINT16      &unASMRuleNumber)
{
    pBuffer         = m_pBuffer;
    ulTime          = m_ulTime;
    uStreamNumber   = m_uStreamNumber;
    unASMFlags      = m_unASMFlags;
    unASMRuleNumber = m_unASMRuleNumber;

    // Whenever a COM object is returned out of an
    // interface, it should be AddRef()'d.
    if (m_pBuffer) m_pBuffer->AddRef();

    return HXR_OK;
}

inline HX_RESULT ServerPacket::_Set(IHXBuffer*  pBuffer, 
                                    ULONG32     ulTime,
                                    UINT16      uStreamNumber,
                                    UINT8       unASMFlags,
                                    UINT16      unASMRuleNumber)
{
    /* We allow changing the packet info when it is owned
     * by atmost one user.
     */
    if (m_lRefCount > 1)
    {
        HX_ASSERT(FALSE);
        return HXR_UNEXPECTED;
    }

    if (m_pBuffer) m_pBuffer->Release();
    m_pBuffer = pBuffer;
    m_ulSize = 0;
    if (m_pBuffer)
    {
        m_pBuffer->AddRef();
        m_ulSize = pBuffer->GetSize();
    }

    m_ulTime            = ulTime;
    m_uStreamNumber     = uStreamNumber;
    m_unASMFlags        = unASMFlags;
    m_unASMRuleNumber   = unASMRuleNumber;    

    return HXR_OK;
}


STDMETHODIMP_(UINT8)ServerPacket::GetVersion() {return 0x02;}
STDMETHODIMP ServerPacket::GetPaddingBit(REF(BOOL)bPadding) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetPaddingBit(BOOL bPadding){return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetExtensionBit(REF(BOOL)bExtension){return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetExtensionBit(BOOL bExtension){return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetCSRCCount(REF(UINT8)unCSRCCount){return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetCSRCCount(UINT8 unCSRCCount){return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetMarkerBit(REF(BOOL)bMarker) {bMarker = m_bMBit; return HXR_OK;}
STDMETHODIMP ServerPacket::SetMarkerBit(BOOL bMarker) {m_bMBit = bMarker; return HXR_OK;}
STDMETHODIMP ServerPacket::GetPayloadType(REF(UINT8)unPayloadType){return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetPayloadType(UINT8 unPayloadType){return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetSequenceNumber(REF(UINT16)unSeqNo){return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetSequenceNumber(UINT16 unSeqNo) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetTimeStamp(REF(UINT32)ulTS) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetTimeStamp(UINT32 ulTS) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetSSRC(REF(UINT32)ulSSRC) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetSSRC(UINT32 ulSSRC) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetCSRCList(REF(const char*) pulCSRC) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetCSRCList(const char* pCSRCList, UINT32 ulSize) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetPadding(REF(const char*) pPadding) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetPadding(const char* pPadding, UINT32 ulSize) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::GetExtension(REF(const char*) pExtension) {return HXR_NOTIMPL;}
STDMETHODIMP ServerPacket::SetExtension(const char* pExtension, UINT32 ulSize) {return HXR_NOTIMPL;}


/////////////////////////////////////////////////////////////////////////
//      ServerRTPPacket
//
/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your 
//              object.
//
STDMETHODIMP ServerRTPPacket::QueryInterface(REFIID riid, void** ppvObj)
{
    /* This is a travesty of justice, but it's fast */
    if (ppvObj == (void **)0xffffd00d)
    {
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTPPacket))
    {
        AddRef();
        *ppvObj = (IHXRTPPacket*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPacket))
    {
        AddRef();
        *ppvObj = (IHXPacket*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRTPPacketInfo))
    {
        if (m_bEnableIHXRTPPacketInfo)
        {
            AddRef();
            *ppvObj = (IHXRTPPacketInfo*)this;
            return HXR_OK;
        }
        else
        {
            *ppvObj = NULL;
            return HXR_NOINTERFACE;         
        }
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    return ServerPacket::QueryInterface(riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) ServerRTPPacket::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *      Method:
 *              IHXPacket::Set
 */
STDMETHODIMP ServerRTPPacket::Set
(
    IHXBuffer*          pBuffer, 
    ULONG32             ulTime,
    UINT16              uStreamNumber,
    UINT8               unASMFlags,
    UINT16              unASMRuleNumber
)
{
    m_ulRTPTime = ulTime;

    return _Set(pBuffer, 
                ulTime,
                uStreamNumber,
                unASMFlags,
                unASMRuleNumber);
}

/************************************************************************
 *      Method:
 *              IHXRTPPacket::Set
 */
STDMETHODIMP ServerRTPPacket::GetRTP                            
(
    REF(IHXBuffer*)     pBuffer, 
    REF(ULONG32)        ulTime,
    REF(ULONG32)        ulRTPTime,
    REF(UINT16)         uStreamNumber,
    REF(UINT8)          unASMFlags,
    REF(UINT16)         unASMRuleNumber
)
{
    ulRTPTime = m_ulRTPTime;

    return _Get(pBuffer,
                ulTime,
                uStreamNumber,
                unASMFlags,
                unASMRuleNumber);
}

/************************************************************************
 *      Method:
 *              IHXRTPPacket::GetRTPTime
 */
STDMETHODIMP_(ULONG32) ServerRTPPacket::GetRTPTime()
{
    return m_ulRTPTime;
}

/************************************************************************
 *      Method:
 *              IHXRTPPacket::SetRTP
 */
STDMETHODIMP ServerRTPPacket::SetRTP
(
    IHXBuffer*      pBuffer, 
    ULONG32         ulTime,
    ULONG32         ulRTPTime,
    UINT16          uStreamNumber,
    UINT8           unASMFlags,
    UINT16          unASMRuleNumber
)
{
    m_ulRTPTime = ulRTPTime;

    return _Set(pBuffer, 
                ulTime,
                uStreamNumber,
                unASMFlags,
                unASMRuleNumber);
}

void ServerRTPPacket::SetPacket(IHXPacket* pPacket)
{
    if (this == pPacket)
    {
        return;
    }

    IHXBuffer*      pBuffer;
    UINT32          ulTime;
    UINT32          ulRTPTime;
    UINT16          unStreamNumber;
    UINT8           unASMFlags;
    UINT16          unASMRuleNumber;

    IHXRTPPacket*  pRTPPacket;

    if (pPacket->QueryInterface(IID_IHXRTPPacket, 
                                (void**) &pRTPPacket) == HXR_OK)
    {
        pRTPPacket->GetRTP(pBuffer, ulTime, ulRTPTime, unStreamNumber, 
                           unASMFlags, unASMRuleNumber);
        pRTPPacket->Release();
        SetRTP(pBuffer, ulTime, ulRTPTime, unStreamNumber,
               unASMFlags, unASMRuleNumber);
    }
    else
    {
        pPacket->Get(pBuffer, ulTime, unStreamNumber, unASMFlags,
                     unASMRuleNumber);
        _Set(pBuffer, ulTime, unStreamNumber, unASMFlags, unASMRuleNumber);
    }

    if (pPacket->IsLost())
    {
        this->SetAsLost();
    }

    HX_RELEASE(pBuffer);
}

// copies data from pServerPacket packet to itself
// Needs to be updated while adding any new member

HX_RESULT
ServerRTPPacket::SetData(ServerRTPPacket* pRTPPacket)
{
    if (!pRTPPacket)
    {
        return HXR_UNEXPECTED;
    }

    ServerPacket::SetData((ServerPacket*)pRTPPacket);
    m_ulRTPTime = pRTPPacket->m_ulRTPTime;

    return HXR_OK;
}

STDMETHODIMP_(UINT32)
ServerPacket::GetSeqNo()
{
    return m_ulPacketSequenceNumber;
}

STDMETHODIMP_(UINT32)
ServerPacket::GetStreamSeqNo()
{
    return m_ulPacketStreamSequenceNumber;
}

STDMETHODIMP_(BOOL)
ServerPacket::GetIsLostRelaying()
{
    return m_bLostRelaying;
}

STDMETHODIMP
ServerPacket::SetSeqNo(UINT32 ulSeqNo)
{
    m_ulPacketSequenceNumber = ulSeqNo;

    return HXR_OK;
}

STDMETHODIMP
ServerPacket::SetStreamSeqNo(UINT32 ulStreamSeqNo)
{
    m_ulPacketStreamSequenceNumber = ulStreamSeqNo;

    return HXR_OK;
}

STDMETHODIMP
ServerPacket::SetIsLostRelaying(BOOL bLostRelay)
{
    m_bLostRelaying = bLostRelay;

    return HXR_OK;
}

STDMETHODIMP_(BOOL)
ServerPacket::SupportsLowLatency()
{
    return (m_pRuleSeqNoArray ? TRUE : FALSE);
}

STDMETHODIMP_(UINT16)
ServerPacket::GetRuleSeqNoArraySize()
{
    return (m_uRuleSeqNoArraySize);
}

STDMETHODIMP_(UINT16*)
ServerPacket::GetRuleSeqNoArray()
{
    return (m_pRuleSeqNoArray);
}

STDMETHODIMP
ServerPacket::SetRuleSeqNoArray(UINT16* pRuleSeqNoArray, UINT16 uSize)
{
    if (m_pRuleSeqNoArray)
    {
        HX_DELETE(m_pRuleSeqNoArray);
    }

    m_pRuleSeqNoArray = new UINT16[uSize];
 
    if (m_pRuleSeqNoArray)
    {
        m_uRuleSeqNoArraySize = uSize;

        for (UINT16 i=0; i < m_uRuleSeqNoArraySize; i++)
        {
            m_pRuleSeqNoArray[i] = pRuleSeqNoArray[i];
        }
    }

    return HXR_OK;
}

// copies data from pServerPacket packet to itself
// Needs to be updated while adding any new member

HX_RESULT
ServerPacket::SetData(ServerPacket* pServerPacket)
{
    if (m_lRefCount > 1 || !pServerPacket)
    {
        HX_ASSERT(FALSE);
        return HXR_UNEXPECTED;
    }
    
       
    if (m_pBuffer)
    {
        m_pBuffer->Release();
    }
    if (m_pRuleSeqNoArray)
    {
        HX_DELETE(m_pRuleSeqNoArray);
    }
    *this = *pServerPacket;
    m_lRefCount = 1;
    if(m_pBuffer)
    {
        m_pBuffer->AddRef();
    }
    if (m_pRuleSeqNoArray)
    {
        m_pRuleSeqNoArray = new UINT16[m_uRuleSeqNoArraySize];

        for (UINT16 i=0; i < m_uRuleSeqNoArraySize; i++)
        {
            m_pRuleSeqNoArray[i] = pServerPacket->m_pRuleSeqNoArray[i];
        }
    }
    return HXR_OK;
}
//RTPPacketInfo Methods

STDMETHODIMP 
ServerRTPPacket::GetPaddingBit(REF(BOOL)bPadding)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::SetPaddingBit(BOOL bPadding)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::GetExtensionBit(REF(BOOL)bExtension)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::SetExtensionBit(BOOL bExtension)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::GetCSRCCount(REF(UINT8)unCSRCCount)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::SetCSRCCount(UINT8 unCSRCCount)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::GetCSRCList(REF(const char*) pulCSRC)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::SetCSRCList(const char* pCSRCList, UINT32 ulSize)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::GetPadding(REF(const char*) pPadding)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::SetPadding(const char* pPadding, UINT32 ulSize)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::GetExtension(REF(const char*) pExtension)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::SetExtension(const char* pExtension, UINT32 ulSize)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
ServerRTPPacket::GetPayloadType(REF(UINT8)unPayloadType)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP 
ServerRTPPacket::SetPayloadType(UINT8 unPayloadType)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP_(UINT8)
ServerRTPPacket::GetVersion() 
{
    return 0x02;
}

STDMETHODIMP
ServerRTPPacket::GetMarkerBit(REF(BOOL)bMarker)
{
    bMarker = m_bMBit; return HXR_OK;
}

STDMETHODIMP
ServerRTPPacket::SetMarkerBit(BOOL bMarker)
{
    m_bMBit = bMarker; return HXR_OK;
}

STDMETHODIMP
ServerRTPPacket::GetSequenceNumber(REF(UINT16)unSeqNo)
{
    unSeqNo = m_ulPacketSequenceNumber;
    return HXR_OK;
}

STDMETHODIMP
ServerRTPPacket::SetSequenceNumber(UINT16 unSeqNo) 
{
    m_ulPacketSequenceNumber = unSeqNo;
    return HXR_OK;
}

STDMETHODIMP
ServerRTPPacket::GetTimeStamp(REF(UINT32)ulTS)
{
    ulTS = m_ulRTPTime;
    return HXR_OK;
}

STDMETHODIMP
ServerRTPPacket::SetTimeStamp(UINT32 ulTS) 
{
    m_ulRTPTime = ulTS;
    return HXR_OK;
}

STDMETHODIMP ServerRTPPacket::GetSSRC(REF(UINT32)ulSSRC)
{
    ulSSRC = m_ulSSRC;
    return HXR_OK;
}

STDMETHODIMP
ServerRTPPacket::SetSSRC(UINT32 ulSSRC)
{
    m_ulSSRC = ulSSRC;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *              IHXServerPacketExt::SetRTPTime
 */
STDMETHODIMP ServerRTPPacket::SetRTPTime(UINT32 ulRTPTime)
{
    m_ulRTPTime = ulRTPTime;
    return HXR_OK;
}

