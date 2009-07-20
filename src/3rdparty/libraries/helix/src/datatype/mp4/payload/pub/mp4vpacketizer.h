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

#ifndef _MP4VPACKETIZER_H_
#define _MP4VPACKETIZER_H_

#include "mp4vpyld.h"

/*
 * MP4Video packetizer implementing RFC3016
 * Only thing this class does, though, is to put a=fmtp and a correct mimetype
 */
class CMP4VPacketizer	: public MP4VPayloadFormat
{
public:
    CMP4VPacketizer();
    ~CMP4VPacketizer();

    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);
    
    /*
     *	IHXPayloadFormatObject methods
     */
    STDMETHOD(Init)		(THIS_ IUnknown* pContext, HXBOOL bPacketize);
    STDMETHOD(SetStreamHeader)	(THIS_ IHXValues* pHeader);

protected:
    /*
     * Stream Header Routines
     */
    HX_RESULT	HandleMimeType(void);
    void	HandleMaxPacketSize(void);
    HX_RESULT	HandleFMTP(BYTE* pOpaque, UINT32 ulOpaqueSize);
    HX_RESULT	HandleOpaqueData(IHXBuffer* pOpaque);
private:    
    UINT32		    m_ulRefCount;
    IUnknown*		    m_pContext;
    IHXCommonClassFactory*  m_pCCF;
    IHXValues*		    m_pStreamHeader;
};
#endif /* _MP4VPACKETIZER_H_ */

