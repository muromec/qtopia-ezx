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

#ifndef _MP4VPYLD_H_
#define _MP4VPYLD_H_

/****************************************************************************
 *  Includes
 */
#include "hxalloc.h"
#include "tsconvrt.h"
#include "mp4vpyif.h"

/****************************************************************************
 *  MP4VPayloadFormat
 */
class MP4VPayloadFormat : public IMP4VPayloadFormat
{
public:
    MP4VPayloadFormat(CHXBufferMemoryAllocator* pAllocator = NULL);
    ~MP4VPayloadFormat();
    static HX_RESULT Build(REF(IMP4VPayloadFormat*) pFmt);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXPayloadFormatObject methods
     */
    STDMETHOD(Init)		(THIS_
				IUnknown* pContext,
				HXBOOL bPacketize);
    STDMETHOD(Close)		(THIS)	{ return HXR_NOTIMPL; }
    STDMETHOD(Reset)		(THIS);
    STDMETHOD(SetStreamHeader)	(THIS_
				IHXValues* pHeader);
    STDMETHOD(GetStreamHeader)	(THIS_
				REF(IHXValues*) pHeader);
    STDMETHOD(SetPacket)	(THIS_
				IHXPacket* pPacket);
    STDMETHOD(GetPacket)	(THIS_
				REF(IHXPacket*) pPacket);
    STDMETHOD(Flush)		(THIS);

    /*
     *	Custom public interface methods
     */
    HX_RESULT CreateHXCodecPacket(ULONG32* &pHXCodecDataOut);

    const char* GetCodecId(void) { return m_pCodecId; }
    ULONG32 GetBitstreamHeaderSize(void) { return m_ulBitstreamHeaderSize; }
    const UINT8* GetBitstreamHeader(void) { return m_pBitstreamHeader; }
    void SetAllocator(CHXBufferMemoryAllocator* pAllocator);

    HX_RESULT SetTimeAnchor(ULONG32 ulTime)
    {
	m_TSConverter.SetOffset(ulTime);

	return HXR_OK;
    }

    HX_RESULT SetMaxPacketSize(UINT32 ulMax)
    {
	m_ulMaxPacketDataSize = ulMax;
	return HXR_OK;
    }

private:
    typedef enum
    {
	PYID_UNKNOWN,
	PYID_X_HX_MP4,
	PYID_MP4V_ES,
	PYID_X_HX_3GPP_H263,
	PYID_X_HX_AVC1,
        PYID_X_HX_DIVX
    } PayloadID;

    inline HX_RESULT SetPacketizerHeader(IHXValues* pHeader);
    inline HX_RESULT SetAssemblerHeader(IHXValues* pHeader);
    HX_RESULT SetAssemblerHXHeader(IHXValues* pHeader);
    HX_RESULT SetAssembler3016Header(IHXValues* pHeader);
    HX_RESULT SetAssemblerHX3GPPH263Header(IHXValues* pHeader);
    HX_RESULT SetAssemblerHX3GPPH264Header(IHXValues* pHeader);
    HX_RESULT SetAssemblerHXAVIHeader(IHXValues* pHeader);
    inline HX_RESULT GetPacketizerPacket(IHXPacket* &pOutPacket);
    inline HX_RESULT GetAssemblerPacket(IHXPacket* &pOutPacket);
    inline HX_RESULT SetPacketizerPacket(IHXPacket* pPacket);
    inline HX_RESULT SetAssemblerPacket(IHXPacket* pPacket);

    inline HX_RESULT GetRawPacketizerPacket(IHXPacket* &pOutPacket);
    inline HX_RESULT FragmentPacket(IHXPacket* pPacket);

    inline ULONG32 GetFrameSize(CHXSimpleList* pPacketList);

    inline ULONG32 GetPacketTime(IHXPacket* pPacket);

    void FlushPackets(ULONG32 ulCount);
    void FlushOutputPackets(ULONG32 ulCount);

    ULONG32 CountValidPackets(ULONG32 ulCount);
    ULONG32 SumPacketSizes(ULONG32 ulCount);

    inline HXBOOL IsPictureStart(IHXPacket* pPacket);
    inline HXBOOL IsValidPacket(IHXPacket* pPacket);

    LONG32			m_lRefCount;
    IHXCommonClassFactory*	m_pClassFactory;

    CHXBufferMemoryAllocator*	m_pAllocator;

    IHXValues*			m_pStreamHeader;
    UINT8*			m_pBitstreamHeader;
    ULONG32			m_ulBitstreamHeaderSize;
    const char*			m_pCodecId;

    ULONG32			m_ulSamplesPerSecond;

    CHXSimpleList		m_InputPackets;
    CHXSimpleList		m_OutputPackets;
    ULONG32			m_ulFrameCount;
    ULONG32			m_ulFrameTime;
    UINT16			m_uSeqNumber;
    HXBOOL			m_bPictureStarted;

    ULONG32			m_ulMaxPacketDataSize;
    HXBOOL			m_bFlushed;
    HXBOOL			m_bFirstPacket;
    HXBOOL			m_bFirstFrame;
    HXBOOL			m_bUsesRTPPackets;
    HXBOOL			m_bRTPPacketTested;
    HXBOOL			m_bPacketize;

    PayloadID			m_PayloadID;

    CTSConverter		m_TSConverter;
};

#endif	// _MP4VPYLD_H_
