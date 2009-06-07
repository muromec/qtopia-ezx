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

#ifndef _HXAMRPYLD_H_
#define _HXAMRPYLD_H_


// Forward declarations
typedef _INTERFACE IHXValues IHXValues;
typedef _INTERFACE IHXPacket IHXPacket;
class CMediaPacket;

/****************************************************************************
 *  Includes
 */
#include "mp4apyif.h"
#include "amr_flavor.h"

#include "tsconvrt.h"

/****************************************************************************
 *  HXAMRPayloadFormat
 */
class CHXAMRPayloadFormat : public IMP4APayloadFormat
{
public:
    CHXAMRPayloadFormat();
    ~CHXAMRPayloadFormat();

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
    STDMETHOD(Close)		(THIS);
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
    virtual ULONG32      GetBitstreamHeaderSize(void);
    virtual const UINT8* GetBitstreamHeader(void);
    virtual HX_RESULT    CreateMediaPacket(CMediaPacket* &pOutMediaPacket);
    virtual HX_RESULT    SetSamplingRate(ULONG32 ulSamplesPerSecond);
    virtual HX_RESULT    SetAUDuration(ULONG32 ulAUDuration);
    virtual HX_RESULT    SetTimeAnchor(ULONG32 ulTimeMs);
    virtual ULONG32	 GetTimeBase(void);
private:
    INT32                  m_lRefCount;
    UINT8                  m_bitstreamHdr[1];
    ULONG32                m_ulSampleRate;
    ULONG32                m_ulAUDuration;
    IHXPacket*             m_pCurrentPkt;
    UINT8*                 m_pCurBufPos;
    ULONG32                m_ulBytesLeft;
    ULONG32                m_ulTimestamp;
    AMRFlavor              m_flavor;
    CTSConverter	   m_TSConverter;
    HX_BITFIELD            m_bNeedTimestamp : 1;
    HX_BITFIELD            m_bLoss : 1;
};

#endif	// _HXAMRPYLD_H_
