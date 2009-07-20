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

#ifndef _MPAPAYLD_H_
#define _MPAPAYLD_H_


class MPAPayloadFormat : public IHXPayloadFormatObject
{
public:
    MPAPayloadFormat();
    ~MPAPayloadFormat();

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

private:
    LONG32			m_lRefCount;
    IUnknown*			m_pContext;
    IHXCommonClassFactory*	m_pClassFactory;
    IHXValues*			m_pStreamHeader;
    UINT32			m_ulStreamNum;
    HXBOOL			m_bPacketizing;
    UINT32			m_ulFrameSize;
    HXBOOL			m_bFragmentFrames;
    UINT32			m_ulFramesPerPacket;
    UINT32			m_ulPacketsPerFrame;
    UINT32			m_ulFrameOffset;
    HXBOOL			m_bFlushed;
    CHXSimpleList*		m_pInputPackets;
    UINT32			m_ulInputSize;
    UINT32			m_ulInputUsed;
    IHXPacket*			m_pPartialPacket;
    IHXBuffer*			m_pPartialBuffer;
    UINT32			m_ulMaxPacketSize;

    UINT32	GetFrameSize		(UCHAR* pBuffer, UINT32 ulSize,
					REF(UINT32) ulBytesSkipped);
    UINT32	GetFrameOffset		(IHXBuffer* pBuffer);
    HX_RESULT	CreateNormalPacket	(REF(IHXPacket*) pPacket);
    HX_RESULT	CreateFragmentedPacket  (REF(IHXPacket*) pPacket);
    HX_RESULT	UnformatRTPPacket	(REF(IHXPacket*) pPacket);
    HX_RESULT	UnformatFirstPacket	(REF(IHXPacket*) pPacket, 
					REF(HXBOOL) bDone);
    HX_RESULT	UnformatNextPacket	(REF(IHXPacket*) pPacket, 
					REF(HXBOOL) bDone);
    HX_RESULT	CreateLostPacket	(IHXPacket* pOldPacket, 
					REF(IHXPacket*) pNewPacket);
    UINT32	GetNextTimestamp	();
    void	CopyInput		(UCHAR* pDest, UINT32 ulSize);
};

#endif // ndef _MPAPAYLD_H_
