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

#ifndef _MP4APYIF_H_
#define _MP4APYIF_H_

/****************************************************************************
 *  Includes
 */
#include "ihxpckts.h"
#include "hxformt.h"
#include "mdpkt.h"


/****************************************************************************
 *  MP4GPayloadFormat
 */
class IMP4APayloadFormat : public IHXPayloadFormatObject
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXPayloadFormatObject methods
     */
    STDMETHOD(Init)		(THIS_
				IUnknown* pContext,
				HXBOOL bPacketize) PURE;
    STDMETHOD(Close)		(THIS)	{ return HXR_NOTIMPL; }
    STDMETHOD(Reset)		(THIS) PURE;
    STDMETHOD(SetStreamHeader)	(THIS_
				IHXValues* pHeader) PURE;
    STDMETHOD(GetStreamHeader)	(THIS_
				REF(IHXValues*) pHeader) PURE;
    STDMETHOD(SetPacket)	(THIS_
				IHXPacket* pPacket) PURE;
    STDMETHOD(GetPacket)	(THIS_
				REF(IHXPacket*) pOutPacket) PURE;
    STDMETHOD(Flush)		(THIS) PURE;

    /*
     *	IMP4APayloadFormat methods
     */
    virtual ULONG32 GetBitstreamHeaderSize(void) = 0;
    virtual const UINT8* GetBitstreamHeader(void) = 0;
    virtual UINT8 GetBitstreamType(void) { return 2; };
    virtual HX_RESULT CreateMediaPacket(CMediaPacket* &pOutMediaPacket) = 0;
    virtual HX_RESULT SetSamplingRate(ULONG32 ulSamplesPerSecond) = 0;
    virtual HX_RESULT SetAUDuration(ULONG32 ulAUDuration) = 0;
    virtual HX_RESULT SetTimeAnchor(ULONG32 ulTimeMs) = 0;
    virtual ULONG32 GetTimeBase(void) = 0;
    virtual UINT8 GetObjectProfileIndication() {return 0;}
};

#endif	// _MP4APYIF_H_
