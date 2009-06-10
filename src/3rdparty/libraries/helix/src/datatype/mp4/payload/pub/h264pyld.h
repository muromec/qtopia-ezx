/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h264pyld.h,v 1.7 2009/01/15 17:18:51 ehyche Exp $
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
#ifndef _H264PYLD_H_
#define _H264PYLD_H_

/****************************************************************************
 *  Includes
 */
#include "mp4vpyif.h"
#include "tsconvrt.h"
#include "hxcodec.h"  // for HXCODEC_DATA* 
#include "hxalloc.h"


/****************************************************************************
 *  H264PayloadFormat
 */
class H264PayloadFormat : public IMP4VPayloadFormat
{
public:
    H264PayloadFormat(CHXBufferMemoryAllocator* pAllocator = NULL);
    ~H264PayloadFormat();

    static HX_RESULT Build(REF(IMP4VPayloadFormat*) pFmt);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
                REFIID riid,
                void** ppvObj);
    STDMETHOD_(UINT32,AddRef)	(THIS);
    STDMETHOD_(UINT32,Release)	(THIS);

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
     *	IMP4VPayloadFormat methods
     */
    virtual UINT32 GetBitstreamHeaderSize(void) { return m_ulBitstreamHeaderSize; }
    virtual const UINT8* GetBitstreamHeader(void) { return m_pBitstreamHeader; }

    virtual HX_RESULT CreateHXCodecPacket(UINT32* &pHXCodecDataOut);
    virtual const char* GetCodecId(void);
    virtual HX_RESULT   SetNextCodecId();
    virtual void        ResetCodecId();
    virtual HX_RESULT SetTimeAnchor(UINT32 ulTime)
    {
        m_TSConverter.SetOffset(ulTime);
        return HXR_OK;
    }
    virtual void      SetAllocator(CHXBufferMemoryAllocator*	pAllocator);

private:

    typedef enum
    {
        PYID_X_HX_AVC1,
        PYID_X_HX_H264
    } PayloadID;
   
    class CNALUPacket
    {
    private:
    UINT8* pData;
    UINT32 ulSize;
    HXBOOL bDataIsOwned;

    public:
    IHXBuffer* pBuffer;
    UINT16 usDON;
    UINT32 ulTimeStamp;
    HXBOOL bLost;
    UINT16 usASMRule;

    CNALUPacket(void)
    : pBuffer(NULL)
    , pData(NULL)
    , ulSize(0)
    , usDON(0)
    , ulTimeStamp(0)
    , bLost(0)
    , usASMRule(0)
    , bDataIsOwned(FALSE)
    {
        ;
    }

    ~CNALUPacket();

    void DeallocateData();

    // Allocate the new data size and transfer ownership, 
    // i.e. responsibility for deallocation, to this object.
    // If sizeTocopy is non-zero, then copy the given data to starting
    // beyond the old size.
    HX_RESULT ResizeData(UINT32 ulNewSize, 
                         UINT32 ulSizeToCopy, 
                         UINT8* pDataToCopy);
 

    inline UINT8* GetDataPtr()
    {
        return pData;
    }

    inline void SetDataPtr(UINT8* pNewData)
    {
        DeallocateData();
        pData = pNewData;
        bDataIsOwned = FALSE;
    }

    inline UINT32 GetDataSize()
    {
        return ulSize;
    }

    inline void SetDataSize(UINT32 ulSizeIn)
    {
        ulSize = ulSizeIn;
    }
    };


    class CNALUnit
    {
    public:
    UINT16 usSize;
    UINT8* pData;
    CNALUnit(void)
    : pData(NULL)
        , usSize(0)
    {
        ;
    }

    ~CNALUnit()
    {
        HX_VECTOR_DELETE(pData);
    }
    };

    HX_RESULT SetPacketizerHeader(IHXValues* pHeader);
    HX_RESULT SetAssemblerHeader(IHXValues* pHeader);

    HX_RESULT SetPacketizerPacket(IHXPacket* pPacket);
    HX_RESULT SetAssemblerPacket(IHXPacket* pPacket);

    HX_RESULT GetPacketizerPacket(IHXPacket* &pOutPacket);
    HX_RESULT GetAssemblerPacket(IHXPacket* &pOutPacket);

    HX_RESULT SetAssemblerConfig(IHXValues* pHeader);

    // Assembler Functions
    HXCODEC_DATA* StripPacket(void);

    HX_RESULT ProducePackets(void);

    H264PayloadFormat::CNALUPacket* H264PayloadFormat::AllocNALUPacket(IHXBuffer* pBuffer);
        HX_RESULT ParsePacket(IHXPacket* pPacket);

    HX_RESULT DeinterleavePackets(void);

    HX_RESULT DeinterleavePacket(CNALUPacket* pAUPacket);
        
    HX_RESULT ReapMediaPacket(void);

    inline UINT32 GetPacketTime(IHXPacket* pPacket);

    HX_RESULT ParseSpropParameterSet(IHXBuffer *pParameterString);

    void FlushQueues(void);
    void FlushInputQueue(UINT32 ulCount);
    void FlushDeinterleaveQueue(UINT32 ulCount);
    void FlushOutputQueue(UINT32 ulCount);

    void FlushArrays(void);
    inline void FlushNALUArray(CNALUPacket** pArray, UINT32 &ulSize);
    inline void FlushPacketArray(IHXPacket** pArray, UINT32 &ulSize);

  
    inline HXBOOL MayNALUPacketPrecede(CNALUPacket* pCandidate,
                                        CNALUPacket* pResident);

    inline HXBOOL IsInitialBufferingDone(void);
    inline HX_RESULT IsInterleavingDepthAchieved(void);
    inline HXBOOL IsAUComplete(void);

    INT32               m_lRefCount;
    IHXCommonClassFactory*	m_pClassFactory;
    CHXBufferMemoryAllocator*	m_pAllocator;

    // Optional Parameters
    UINT32              m_ulPacketizationMode;
    UINT32              m_ulSpropInterleavingDepth;
    UINT32              m_ulSpropDeintBufReq;
    UINT32	            m_ulSpropInitBufTime;
    UINT32              m_ulSpropMaxDonDiff;

    IHXValues*          m_pStreamHeader;
    UINT8*              m_pBitstreamHeader;
    UINT32              m_ulBitstreamHeaderSize;

    CHXSimpleList       m_InputQueue;
    CHXSimpleList       m_DeinterleaveQueue;
    CHXSimpleList       m_OutputQueue;
    CHXSimpleList       m_ParameterSetQueue;
   
    CNALUPacket**       m_pNALUPackets;
    UINT32              m_ulMaxNALUPackets; 
    UINT32              m_ulNumNALUPackets;
 
    UINT32              m_ulSamplesPerSecond;
    UINT32              m_ulRTPSamplesPerSecond;
    UINT32              m_ulLastParsedPktTime;
    UINT32              m_ulPrevDON;
    UINT32              m_ulSequenceNumber;
    UINT32              m_ulCodecDataSeqNumber;
    UINT32              m_ulCodecIDIndex;
    static const char* const  m_ppszCodecId[];

    PayloadID           m_PayloadID;
    CTSConverter        m_TSConverter;

    HX_BITFIELD	        m_bFlushed:1;
    HX_BITFIELD	        m_bUsesRTPPackets:1;
    HX_BITFIELD         m_bRTPPacketTested:1;
    HX_BITFIELD         m_bPacketize:1;
    HX_BITFIELD         m_bInitialBufferingDone:1;
    HX_BITFIELD         m_bStartPacket:1;
    HX_BITFIELD         m_bFirstPacket:1;
    HX_BITFIELD         m_bStartNALFragment:1;
    HX_BITFIELD         m_bFragmentLost:1;
};


inline void H264PayloadFormat::FlushNALUArray(CNALUPacket** pArray,
                                                UINT32 &ulSize)
{
    if (ulSize > 0)
    {
        do
        {
            ulSize--;
            HX_DELETE(pArray[ulSize]);
        } while (ulSize > 0);
    }
}


inline HXBOOL H264PayloadFormat::MayNALUPacketPrecede(CNALUPacket* pCandidate,
                                                        CNALUPacket* pResident)
{
    if(m_ulPacketizationMode == 2)
    {
        INT16 uDon_diff = 0;
        if((pCandidate->usDON < pResident->usDON) && 
           ((pResident->usDON - pCandidate->usDON) < 32768))
        {
            uDon_diff = pResident->usDON - pCandidate->usDON;
        }

        if((pCandidate->usDON > pResident->usDON) && 
           ((pCandidate->usDON - pResident->usDON) >= 32768))
        {
            uDon_diff = 65536 - pCandidate->usDON + pResident->usDON;
        }
        if((pCandidate->usDON < pResident->usDON) && 
           ((pResident->usDON - pCandidate->usDON) >= 32768))
        {
                        uDon_diff = -(pCandidate->usDON + 65536 - pResident->usDON);
        }
        if((pCandidate->usDON > pResident->usDON) && 
           ((pCandidate->usDON - pResident->usDON) < 32768))
        {
                        uDon_diff = -(pCandidate->usDON - pResident->usDON);
        }
        return (uDon_diff > 0);
    }
    else
    {
            return (pCandidate->usDON > pResident->usDON);
    }
}

#endif	// _H264PYLD_H_
