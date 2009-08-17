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

#ifndef _MP4TPYLD_H_
#define _MP4TPYLD_H_

/****************************************************************************
 *  Includes
 */
#include "hxalloc.h"
#include "tsconvrt.h"

class CQTTrack;
class CQT_TrackInfo_Manager;

/****************************************************************************
 *  MP4TPayloadFormat
 */
class MP4TPayloadFormat : public IHXPayloadFormatObject
{
public:
    MP4TPayloadFormat(CQTTrack* pTrack,
		      CQT_TrackInfo_Manager* pTrackInfoMgr,
		      CHXBufferMemoryAllocator* pAllocator = NULL);
    ~MP4TPayloadFormat();

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

    ULONG32 GetBitstreamHeaderSize(void) { return m_ulTextCfgHeaderSize; }
    const UINT8* GetBitstreamHeader(void) { return m_pTextHeader; }

    HX_RESULT SetTimeAnchor(ULONG32 ulTime)
    {
	m_TSConverter.SetOffset(ulTime);

	return HXR_OK;
    }

private:

    // /Disallow default constructor invocations:
    MP4TPayloadFormat() {};

    typedef enum
    {
	PYID_X_HX_3GPP_TEXT
    } PayloadID;

    inline HX_RESULT GetPacketizerPacket(IHXPacket* &pOutPacket);

    inline ULONG32 GetFrameSize(CHXSimpleList* pPacketList);

    inline ULONG32 GetPacketTime(IHXPacket* pPacket);

    void FlushPackets(ULONG32 ulCount);

    LONG32			m_lRefCount;
    IHXCommonClassFactory*	m_pClassFactory;

    CHXBufferMemoryAllocator*	m_pAllocator;

    IHXValues*			m_pStreamHeader;
    UINT8*			m_pTextHeader;
    ULONG32			m_ulTextCfgHeaderSize;

    ULONG32			m_ulSamplesPerSecond;

    CHXSimpleList		m_InputPackets;
    CHXSimpleList		m_OutputPackets;
    HXBOOL			m_bPictureStarted;

    HXBOOL			m_bFlushed;
    HXBOOL			m_bFirstPacket;
    HXBOOL			m_bUsesRTPPackets;
    HXBOOL			m_bRTPPacketTested;
    HXBOOL			m_bPacketize;

    PayloadID			m_PayloadID;

    CTSConverter		m_TSConverter;

    CQTTrack*			m_pTrack;

    CQT_TrackInfo_Manager*	m_pTrackInfoMgr;

    RuleToFlagMap*		m_pRuleToFlagMap;

    UINT32			m_ulPriorPacketContentBeginTime;
};

#endif	// _MP4TPYLD_H_
