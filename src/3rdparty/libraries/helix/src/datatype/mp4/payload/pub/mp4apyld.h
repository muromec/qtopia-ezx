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

#ifndef _MP4APYLD_H_
#define _MP4APYLD_H_

/****************************************************************************
 *  Includes
 */
#include "mp4apyif.h"
#include "tsconvrt.h"

class MP4LATMDepack;

/****************************************************************************
 *  MP4APayloadFormat
 */
class MP4APayloadFormat : public IMP4APayloadFormat
{
public:
    MP4APayloadFormat();
    ~MP4APayloadFormat();

    static HX_RESULT Build(REF(IMP4APayloadFormat*) pFmt);

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
				REF(IHXPacket*) pOutPacket);
    STDMETHOD(Flush)		(THIS);

    /*
     *	IMP4APayloadFormat methods
     */
    virtual UINT8 GetBitstreamType(void) { return m_unAudioConfigType; }
    virtual ULONG32 GetBitstreamHeaderSize(void) { return m_ulAudioConfigSize; }
    virtual const UINT8* GetBitstreamHeader(void) { return m_pAudioConfig; }
    virtual UINT8 GetObjectProfileIndication(void) { return m_uObjectProfileIndication; }

    virtual HX_RESULT CreateMediaPacket(CMediaPacket* &pOutMediaPacket);

    virtual HX_RESULT SetSamplingRate(ULONG32 ulSamplesPerSecond);
    virtual HX_RESULT SetAUDuration(ULONG32 ulAUDuration);
    virtual HX_RESULT SetTimeAnchor(ULONG32 ulTimeMs);
    virtual ULONG32 GetTimeBase(void);

private:
    typedef enum
    {
	PYID_X_HX_MP4_RAWAU,
	PYID_X_HX_QT_RAWAU,		
	PYID_MPEG4_SIMPLE_A2,
	PYID_MP4A_LATM,
	PYID_X_HX_AAC_GENERIC,
	PYID_X_HX_3GPP_QCELP
    } PayloadID;

    inline HX_RESULT CreateRawAUMediaPacket(CMediaPacket* &pOutMediaPacket);

    inline HX_RESULT SetPacketizerHeader(IHXValues* pHeader);
    inline HX_RESULT SetAssemblerHeader(IHXValues* pHeader);

    HX_RESULT SetAssemblerHXHeader(IHXValues* pHeader);
    HX_RESULT SetAssemblerLATMHeader(IHXValues* pHeader);
    HX_RESULT SetAssemblerAACGenericHeader(IHXValues* pHeader);
    HX_RESULT SetAssemblerQCELPHeader(IHXValues* pHeader);

    HX_RESULT SetAssemblerLATMConfig(IHXValues* pHeader);
    HX_RESULT OnFrame(ULONG32 ulTime, 
		      const UINT8* pData, 
		      ULONG32 ulSize,
		      HXBOOL bPreviousLoss);
    static void OnFrameCallback(void* pUserData, 
				ULONG32 ulTime, 
				const UINT8* pData, 
				ULONG32 ulSize,
				HXBOOL bPreviousLoss);

    HX_RESULT GetPacketizerPacket(IHXPacket* &pOutPacket);
    HX_RESULT GetAssemblerPacket(IHXPacket* &pOutPacket);

    inline CMediaPacket* BuildMediaPacket(IHXBuffer* pBuffer,
					  ULONG32 ulTime,
					  ULONG32 ulFlags);
    inline ULONG32 GetPacketTime(IHXPacket* pPacket);

    void FlushPackets(ULONG32 ulCount);

    LONG32			m_lRefCount;
    IHXCommonClassFactory*	m_pClassFactory;

    IHXValues*			m_pStreamHeader;
    CHXSimpleList		m_InputPackets;
    CHXSimpleList		m_OutMediaPacketQueue;
    HXBOOL			m_bFlushed;
    HXBOOL			m_bUsesRTPPackets;
    HXBOOL			m_bRTPPacketTested;
    HXBOOL			m_bPacketize;
    HXBOOL			m_bPriorLoss;

#if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM)
    MP4LATMDepack*		m_pLATMDepack;
#endif /* #if defined(HELIX_FEATURE_AUDIO_MPEG4_DEPACK_LATM) */

    UINT8*			m_pAudioConfig;
    ULONG32			m_ulAudioConfigSize;
    UINT8                       m_unAudioConfigType;
    UINT8                       m_uObjectProfileIndication;

    ULONG32			m_ulSamplesPerSecond;
    ULONG32			m_ulRTPSamplesPerSecond;

    PayloadID			m_PayloadID;

    CTSConverter		m_TSConverter;
};

#endif // ndef _SMPPAYLD_H_
