/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: basepkt.h,v 1.10 2006/02/07 19:21:32 ping Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _BASEPKT_H_
#define _BASEPKT_H_

#include "hxcom.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxtime.h"
#include "timeval.h"
#include "hxassert.h"
#include "hxupgrd.h"

class BasePacket : public IHXClientPacket
{
public:
    BasePacket();
    ~BasePacket();

    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    virtual void		SetPacket(IHXPacket* pPacket);
    virtual IHXPacket*		GetPacket();
    virtual IHXPacket*		PeekPacket(); /* This function violates COM Reference Rules */
    virtual UINT16		GetSequenceNumber();
    virtual UINT16		GetReliableSeqNo();
    STDMETHOD_(ULONG32,GetTime)         (THIS);
    STDMETHOD_(UINT16,GetStreamNumber)  (THIS);
    virtual UINT32		GetSize();
    virtual HXBOOL		IsReliable();
    HXBOOL			IsResendRequested();
    void			SetResendRequested();

    /*
     * Leave these public because it's just easier
     */

    UINT16			m_uSequenceNumber;
    UINT16			m_uReliableSeqNo;
    UINT32			m_uPriority;
    HXBOOL			m_bIsReliable;
    HXBOOL			m_bBackToBack;
    HXBOOL			m_bIsResendRequested;

protected:
    LONG32			m_lRefCount;
    IHXPacket*			m_pPacket;
};

inline
BasePacket::BasePacket()
{
    m_lRefCount			= 0;
    m_pPacket			= 0;
    m_uSequenceNumber		= 0;
    m_uReliableSeqNo		= 0;
    m_uPriority			= 0;
    m_bIsReliable		= FALSE;
    m_bBackToBack		= FALSE;
    m_bIsResendRequested	= FALSE;
}

inline
BasePacket::~BasePacket()
{
    if (m_pPacket)
    {
	m_pPacket->Release();
    }
}

inline void
BasePacket::SetPacket(IHXPacket* pPacket)
{
    if (m_pPacket)
    {
        m_pPacket->Release();
    }

    m_pPacket = pPacket;

    if (m_pPacket)
    {
	m_pPacket->AddRef();
    }
   
}

inline IHXPacket*
BasePacket::GetPacket()
{
    if (m_pPacket)
    {
	m_pPacket->AddRef();
    }

    return m_pPacket;
}

inline IHXPacket*
BasePacket::PeekPacket()
{
    /* This function violates COM Reference Rules */

    return m_pPacket;
}

inline STDMETHODIMP
BasePacket::QueryInterface(REFIID riid,
                             void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*) this },
		{ GET_IIDHANDLE(IID_IHXClientPacket), (IHXClientPacket*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj); 
}

inline ULONG32
BasePacket::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

inline ULONG32
BasePacket::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

inline UINT16
BasePacket::GetSequenceNumber()
{
    return m_uSequenceNumber;
}

inline UINT16
BasePacket::GetReliableSeqNo()
{
    return m_uReliableSeqNo;
}

inline STDMETHODIMP_(UINT16)
BasePacket::GetStreamNumber()
{
    return m_pPacket->GetStreamNumber();
}

inline STDMETHODIMP_(ULONG32)
BasePacket::GetTime()
{
    return m_pPacket->GetTime();
}

inline UINT32
BasePacket::GetSize()
{
    if (m_pPacket->IsLost())
    {
	return 0;
    }

    UINT32 size;
    IHXBuffer* pBuffer = m_pPacket->GetBuffer();
    HX_ASSERT(pBuffer);
    size = pBuffer->GetSize();
    pBuffer->Release();
    return size;
}

inline HXBOOL
BasePacket::IsReliable()
{
    return m_bIsReliable;
}

inline HXBOOL
BasePacket::IsResendRequested()
{
    return m_bIsResendRequested;
}

inline void
BasePacket::SetResendRequested()
{
    m_bIsResendRequested = TRUE;
}

struct HXClientPacketInfo
{
    UINT8       reliable;
    UINT8       sanitize;
    UINT16      sequenceNumber;
    UINT16	reliableSeqNo;
    Timeval     startTime;
};

class ClientPacket : public BasePacket
{
public:
    ClientPacket(UINT16      uSequenceNumber,
                 UINT16	     uReliableSeqNo,
                 UINT32	     uTimestamp,
                 UINT32	     uNumBytes,
                 HXBOOL        bIsReliable,
                 IHXPacket* pPacket,
                 Timeval     StartTime,
		 HXBOOL        bSanitize,
		 HXBOOL        bDropped = FALSE);

    STDMETHOD_(ULONG32,GetTime)         (THIS);
    UINT32			GetByteCount();
    HXBOOL			IsLostPacket();
    HXBOOL			IsDroppedPacket();
    HXBOOL			IsSanitizePacket();
    Timeval			GetStartTime();
    void                        SetStartTime(Timeval startTime);

    // serialization method
    static void	Pack	(IHXClientPacket* pPacket, char* pData, UINT32& ulSize, IUnknown* pContext);
    static void	UnPack	(IHXClientPacket*& pPacket, char* pData, UINT32 ulSize, IUnknown* pContext);

private:
    UINT32			m_uTimestamp;
    UINT32			m_uByteCount;
    Timeval			m_StartTime;
    HXBOOL			m_bSanitize;
    HXBOOL                      m_bDropped;
};

inline
ClientPacket::ClientPacket
(
    UINT16      uSequenceNumber,
    UINT16      uReliableSeqNo,
    UINT32      uTimestamp,
    UINT32      uByteCount,
    HXBOOL        bIsReliable,
    IHXPacket* pPacket,
    Timeval     StartTime,
    HXBOOL	bSanitize,
    HXBOOL        bDropped
)
{
    m_uSequenceNumber = uSequenceNumber;
    m_uReliableSeqNo = uReliableSeqNo;
    m_uTimestamp = uTimestamp;
    m_uByteCount = uByteCount;
    m_bIsReliable = bIsReliable;
    SetPacket(pPacket);
    m_StartTime = StartTime;
    m_bSanitize = bSanitize;
    m_bDropped = bDropped;
}

inline STDMETHODIMP_(ULONG32)
ClientPacket::GetTime()
{
    return m_uTimestamp;
}

inline UINT32
ClientPacket::GetByteCount()
{
    return m_uByteCount;
}

inline HXBOOL
ClientPacket::IsLostPacket()
{
    return (m_pPacket || IsDroppedPacket()) ? FALSE : TRUE;
}

inline HXBOOL
ClientPacket::IsDroppedPacket()
{
    return m_bDropped;
}

inline HXBOOL
ClientPacket::IsSanitizePacket()
{
    return m_bSanitize;
}

inline Timeval
ClientPacket::GetStartTime()
{
    return m_StartTime;
}

inline void
ClientPacket::SetStartTime(Timeval startTime)
{
    m_StartTime = startTime;
}

// serialization method
inline void
ClientPacket::Pack(IHXClientPacket* pHXClientPacket, char* pData, UINT32& ulSize, IUnknown* pContext)
{
    UINT16	    uValue = 0;
    UINT32	    ulValue = 0;
    UINT32	    ulPacketSize = 0;
    Timeval	    startTime = 0;
    IHXPacket*	    pPacket = NULL;
    ClientPacket*   pClientPacket = NULL;

    if (!pHXClientPacket)
    {
	goto cleanup;
    }

    pClientPacket = (ClientPacket*)pHXClientPacket;

    pPacket = pClientPacket->GetPacket();
    if (pPacket)
    {
	CHXPacket::Pack(pPacket, NULL, ulPacketSize, pContext);
    }

    // figure out the size 
    if (!pData)
    {
	ulSize = sizeof(HXClientPacketInfo) + ulPacketSize;	
    }
    // pack the data
    else
    {
	pClientPacket = (ClientPacket*)pClientPacket;

	*pData++ = (BYTE)pClientPacket->IsReliable();				ulSize++;
	*pData++ = (BYTE)pClientPacket->IsSanitizePacket();			ulSize++;

	uValue = pClientPacket->GetSequenceNumber();
	*pData++ = (BYTE)uValue; *pData++ = (BYTE)(uValue >> 8); 		ulSize += 2;

	uValue = pClientPacket->GetReliableSeqNo();
	*pData++ = (BYTE)uValue; *pData++ = (BYTE)(uValue >> 8); 		ulSize += 2;
	
	startTime = pClientPacket->GetStartTime();
	ulValue = startTime.tv_sec;
	*pData++ = (BYTE)ulValue; *pData++ = (BYTE)(ulValue >> 8);
	*pData++ = (BYTE)(ulValue >> 16); *pData++ = (BYTE)(ulValue >> 24);	ulSize += 4;

	ulValue = startTime.tv_usec;
	*pData++ = (BYTE)ulValue; *pData++ = (BYTE)(ulValue >> 8);
	*pData++ = (BYTE)(ulValue >> 16); *pData++ = (BYTE)(ulValue >> 24);	ulSize += 4;
    
	if (pPacket)
	{
	    CHXPacket::Pack(pPacket, pData, ulSize, pContext);
	}
    }
   		
cleanup:

    HX_RELEASE(pPacket);

    return;
}   

inline void	
ClientPacket::UnPack(IHXClientPacket*& pClientPacket, char* pData, UINT32 ulSize, IUnknown* pContext)
{
    UINT16	uValue = 0;
    UINT32	ulValue = 0;
    IHXPacket*	pPacket = NULL;
    IHXBuffer*	pBuffer = NULL;
    HXClientPacketInfo	clientPacketInfo;

    pClientPacket = NULL;

    if (!pData || !ulSize)
    {
	goto cleanup;
    }

    HX_ASSERT(ulSize >= sizeof(HXClientPacketInfo));

    clientPacketInfo.reliable = (BYTE)*pData++;	    ulSize--;
    clientPacketInfo.sanitize = (BYTE)*pData++;	    ulSize--;
    
    uValue = (BYTE)*pData++; uValue |= (((BYTE)*pData++) << 8);
    clientPacketInfo.sequenceNumber = uValue;	    ulSize -= 2;

    uValue = (BYTE)*pData++; uValue |= (((BYTE)*pData++) << 8);
    clientPacketInfo.reliableSeqNo = uValue;	    ulSize -= 2;

    ulValue = (BYTE)*pData++; ulValue |= (((BYTE)*pData++) << 8);
    ulValue |= (((BYTE)*pData++) << 16); ulValue |= (((BYTE)*pData++) << 24);
    clientPacketInfo.startTime.tv_sec = ulValue;    ulSize -= 4;

    ulValue = (BYTE)*pData++; ulValue |= (((BYTE)*pData++) << 8);
    ulValue |= (((BYTE)*pData++) << 16); ulValue |= (((BYTE)*pData++) << 24);
    clientPacketInfo.startTime.tv_usec = ulValue;   ulSize -= 4;

    if (ulSize)
    {
	CHXPacket::UnPack(pPacket, pData, ulSize, pContext);
    }

    if (pPacket)
    {
	pBuffer = pPacket->GetBuffer();

	pClientPacket = new ClientPacket(clientPacketInfo.sequenceNumber,
					 clientPacketInfo.reliableSeqNo,
					 pPacket->GetTime(),
					 pBuffer->GetSize(),
					 clientPacketInfo.reliable,
					 pPacket,
					 clientPacketInfo.startTime,
					 FALSE);
    }
    else
    {
	pClientPacket = new ClientPacket(clientPacketInfo.sequenceNumber,
					 clientPacketInfo.reliableSeqNo,
					 0,
					 0,
					 0,
					 0,
					 clientPacketInfo.startTime,
					 FALSE);
    }

    pClientPacket->AddRef();

cleanup:

    HX_RELEASE(pBuffer);
    HX_RELEASE(pPacket);
   
    return;
}

#endif
