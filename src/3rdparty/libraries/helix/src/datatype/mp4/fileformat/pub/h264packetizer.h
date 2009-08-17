/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h264packetizer.h,v 1.3 2006/01/13 20:41:50 bobclark Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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
 * ******************** END LICENSE BLOCK *************************/ 

#ifndef H264PACKETIZER_H
#define H264PACKETIZER_H

#include "hxcom.h"
#include "hxformt.h"
#include "hxccf.h"
#include "ihxpckts.h"
#include "hxslist.h"

struct PreparedBuffer;
struct DONPacket;

class H264Packetizer : public IHXPayloadFormatObject
{
public:
    H264Packetizer();
    ~H264Packetizer();

    static HX_RESULT CreateInstance(REF(IHXPayloadFormatObject*) pPyld);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)          (THIS);

    STDMETHOD_(ULONG32,Release)         (THIS);

    /*
     *  IHXPayloadFormatObject methods
     */
    STDMETHOD(Init)             (THIS_
                                IUnknown* pContext,
                                HXBOOL bPacketize);

    STDMETHOD(Close)            (THIS);

    STDMETHOD(Reset)            (THIS);

    STDMETHOD(SetStreamHeader)  (THIS_
                                IHXValues* pHeader);

    STDMETHOD(GetStreamHeader)  (THIS_
                                REF(IHXValues*) pHeader);

    STDMETHOD(SetPacket)        (THIS_
                                IHXPacket* pPacket);

    STDMETHOD(GetPacket)        (THIS_
                                REF(IHXPacket*) pPacket);

    STDMETHOD(Flush)            (THIS);

    class CNALPacket
    {
    public:
    IHXBuffer* pBuffer;
    UINT8* pData;
    UINT32 ulSize;
    UINT16 usDON;
    UINT32 ulTimeStamp;
    UINT16 usStream;
    UINT8 uASMFlags;
    UINT16 usASMRule;
    HXBOOL packetValid;

    CNALPacket(void)
    : pBuffer(NULL)
    , pData(NULL)
    , ulSize(0)
    , usDON(0)
    , ulTimeStamp(0)
    , usStream(0)
    , uASMFlags(0)
    , usASMRule(0)
    , packetValid(0)
    {
        ;
    }

    ~CNALPacket()
    {
        
        HX_RELEASE(pBuffer);
    }
    };

private:

    HX_RESULT AddHeaderMimeType();
    HX_RESULT AddHeaderSDPData();
    CNALPacket* AllocateNALPacket(IHXPacket* pPacket);
    HX_RESULT CreateSingleNALPacket();
    HX_RESULT ConstructAggregationPacket();
    IHXPacket* CreateFinishedPkt(CHXSimpleList* tmpList, 
                                             UINT32 ulPacketLength, 
                                             UINT32 ulMinRtpTime);
	void InterleaveRTPPacket(IHXPacket* pPacket);
	HX_RESULT ConstructFragmentedPacket(CNALPacket* pNALPacket);
    void FlushQueues(void);
	    
	ULONG32 m_lRefCount;
    IHXCommonClassFactory* m_pCCF;
    IHXValues* m_pHeader;

	CHXSimpleList m_NALPacketList;
	CHXSimpleList m_FinishedPktList;

	UINT32 m_ulByteCount;
	UINT32 m_ulDON;

    UINT8 m_interleaveCount;
	HXBOOL m_bFragmented;
	HXBOOL m_bPendingFlush;
	HXBOOL m_bInterleavingDone;

};
#endif /* H264PACKETIZER_H */

