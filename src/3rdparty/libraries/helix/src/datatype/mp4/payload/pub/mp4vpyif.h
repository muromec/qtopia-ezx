/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mp4vpyif.h,v 1.4 2009/01/15 17:18:51 ehyche Exp $
 * 
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _MP4VPYIF_H_
#define _MP4VPYIF_H_

/****************************************************************************
 *  Includes
 */
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxalloc.h"

/****************************************************************************
 *  MP4VPayloadFormat
 */
class IMP4VPayloadFormat : public IHXPayloadFormatObject
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
     *	IMP4VPayloadFormat methods
     */
    virtual ULONG32      GetBitstreamHeaderSize(void) = 0;
    virtual const UINT8* GetBitstreamHeader(void) = 0;
    virtual UINT8        GetBitstreamType(void) { return 2; };
    virtual HX_RESULT    CreateHXCodecPacket(UINT32* &pHXCodecDataOut) = 0;
    virtual HX_RESULT    SetTimeAnchor(ULONG32 ulTimeMs) = 0;
    virtual const char*  GetCodecId(void) = 0;
    virtual void         SetAllocator(CHXBufferMemoryAllocator* pAllocator) = 0;
    virtual HX_RESULT    SetNextCodecId() = 0;
    virtual void         ResetCodecId() = 0;
};

#endif	// _MP4VPYIF_H_
