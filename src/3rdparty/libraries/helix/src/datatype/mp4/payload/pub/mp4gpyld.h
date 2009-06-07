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

#ifndef _MP4GPYLD_H_
#define _MP4GPYLD_H_

/****************************************************************************
 *  Includes
 */
#include "mp4apyif.h"
#include "tsconvrt.h"


/****************************************************************************
 *  MP4GPayloadFormat
 */
class MP4GPayloadFormat : public IMP4APayloadFormat
{
public:
    MP4GPayloadFormat();
    ~MP4GPayloadFormat();

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
    virtual ULONG32 GetBitstreamHeaderSize(void) { return m_Config.m_pConfig->GetSize(); }
    virtual const UINT8* GetBitstreamHeader(void) { return m_Config.m_pConfig->GetBuffer(); }

    virtual HX_RESULT CreateMediaPacket(CMediaPacket* &pOutMediaPacket);

    virtual HX_RESULT SetSamplingRate(ULONG32 ulSamplesPerSecond);
    virtual HX_RESULT SetAUDuration(ULONG32 ulAUDuration);
    virtual HX_RESULT SetTimeAnchor(ULONG32 ulTimeMs);
    virtual ULONG32 GetTimeBase(void);

private:
    class CFormatConfig
    {
    public:
	typedef enum
	{
	    MODE_GENERIC,
	    MODE_A0,
	    MODE_A1,
	    MODE_A2
	} FormatMode;

	CFormatConfig(void)
	    : m_ulStreamType(0)
	    , m_ulProfileLevelID(0)
	    , m_pConfig(NULL)
	    , m_Mode(MODE_GENERIC)
	    , m_ulConstantSize(0)
	    , m_ulConstantDuration(0)
	    , m_ulSizeLength(0)
	    , m_ulIndexLength(0)
	    , m_ulIndexDeltaLength(0)
	    , m_ulCTSDeltaLength(0)
	    , m_ulDTSDeltaLength(0)
	    , m_ulAuxDataSizeLength(0)
	    , m_ulProfile(0)
	{
	    ;
	}

	~CFormatConfig()
	{
	    HX_RELEASE(m_pConfig);
	}

	HX_RESULT Init(IHXValues* pFMTParams,
		       IHXCommonClassFactory* pClassFactory,
               IHXValues* pStreamHeader);

	// Required Parameters
	ULONG32 m_ulStreamType;
	ULONG32 m_ulProfileLevelID;
	IHXBuffer* m_pConfig;

	// Optional Parameters
	FormatMode m_Mode;
	ULONG32 m_ulConstantSize;
	ULONG32 m_ulConstantDuration;
	ULONG32 m_ulSizeLength;
	ULONG32 m_ulIndexLength;
	ULONG32 m_ulIndexDeltaLength;
	ULONG32 m_ulCTSDeltaLength;
	ULONG32 m_ulDTSDeltaLength;
	ULONG32 m_ulAuxDataSizeLength;
	ULONG32 m_ulProfile;

	// Derived Parameters
	HXBOOL	m_bHasAUHeaders;
    };

    class CAUPacket
    {
    public:
	CAUPacket(void)
	: m_pBuffer(NULL)
	, m_ulSize(0)
	, m_bMinTime(FALSE)
	{
	    ;
	}

	~CAUPacket()
	{
	    HX_RELEASE(m_pBuffer);
	}

	ULONG32 m_ulCTS;
	ULONG32 m_ulDTS;
	ULONG32 m_ulIdx;
	IHXBuffer* m_pBuffer;
	UINT8* m_pData;
	ULONG32 m_ulSize;
	HXBOOL m_bMinTime;
    };

    inline HX_RESULT SetPacketizerHeader(IHXValues* pHeader);
    inline HX_RESULT SetAssemblerHeader(IHXValues* pHeader);

    HX_RESULT SetPacketizerPacket(IHXPacket* pPacket);
    HX_RESULT SetAssemblerPacket(IHXPacket* pPacket);

    HX_RESULT GetPacketizerPacket(IHXPacket* &pOutPacket);
    HX_RESULT GetAssemblerPacket(IHXPacket* &pOutPacket);

    HX_RESULT SetAssemblerConfig(IHXValues* pHeader);

    // Assembler Functions
    CMediaPacket* StripPacket(void);

    HX_RESULT ProducePackets(void);

    HX_RESULT CollectPacketFragments(IHXPacket** &pPacketFragments,
				     ULONG32 &ulNumPacketFragments,
				     ULONG32 &ulMaxPacketFragments);

    HX_RESULT ParsePacketFragments(CAUPacket** &pAUFragments,
				   ULONG32 &ulNumAUFragments,
				   ULONG32 &ulMaxAUFragments,
				   IHXPacket** pPacketFragments,
				   ULONG32& ulNumPacketFragments);

    inline HX_RESULT ParsePacketFragment(CAUPacket** &pAUFragments,
					 ULONG32 &ulNumAUFragments,
					 ULONG32 &ulMaxAUFragments,
					 IHXPacket* pPacketFragment);

    HX_RESULT AggregateAUFragments(CAUPacket** &pAUPackets,
				   ULONG32 &ulNumAUPackets,
				   ULONG32 &ulMaxAUPackets,
				   CAUPacket** pAUFragments,
				   ULONG32 &ulNumAUFragments);

    inline HXBOOL IsFragmentCollection(CAUPacket** pAUFragments,
				     ULONG32 ulNumAUFragments);

    HX_RESULT DeinterleaveAUPackets(CAUPacket** pAUPackets,
				    ULONG32 &ulNumAUPackets);

    inline HX_RESULT DeinterleaveAUPacket(CAUPacket* pAUPacket);

    HX_RESULT ReapMediaPacket(ULONG32 ulLastPacketTime);

    inline ULONG32 GetPacketTime(IHXPacket* pPacket);

    void FlushQueues(void);
    void FlushInputQueue(ULONG32 ulCount);
    void FlushDeinterleaveQueue(ULONG32 ulCount);
    void FlushOutputQueue(ULONG32 ulCount);

    inline HXBOOL FindEarliestKnownDeintTime(ULONG32 &ulEarliestKnownDeintTime);

    void FlushArrays(void);
    inline void FlushAUArray(CAUPacket** pArray, ULONG32 &ulSize);
    inline void FlushPacketArray(IHXPacket** pArray, ULONG32 &ulSize);

    inline ULONG32 SignBitField(ULONG32 ulVal, ULONG32 ulNumBits);
    inline HXBOOL TestIndexGreater(ULONG32 ulIdx1, ULONG32 ulIdx2);
    inline HXBOOL TestTimeGreater(ULONG32 ulTime1, ULONG32 ulTime2);
    inline LONG32 IndexDiff(ULONG32 ulIdx1, ULONG32 ulIdx2);
    inline HXBOOL MayAUPacketPrecede(CAUPacket* pCandidate, 
				   CAUPacket* pResident);

    inline ULONG32 ConvertToSomplingTime(ULONG32 ulRTPTime);

    LONG32			m_lRefCount;
    IHXCommonClassFactory*	m_pClassFactory;

    IHXValues*			m_pStreamHeader;
    CHXSimpleList		m_InputQueue;
    CHXSimpleList		m_HoldingQueue;
    CHXSimpleList		m_DeinterleaveQueue;
    CHXSimpleList		m_OutputQueue;
    HXBOOL			m_bFlushed;
    HXBOOL			m_bUsesRTPPackets;
    HXBOOL			m_bRTPPacketTested;
    HXBOOL			m_bPacketize;
    HXBOOL			m_bPriorLoss;

    HXBOOL			m_bStartPacket;
    ULONG32			m_ulLastPacketTime;
    ULONG32			m_ulFrameCount;

    IHXPacket**			m_pPacketFragments;
    CAUPacket**			m_pAUFragments;
    CAUPacket**			m_pAUPackets;

    ULONG32			m_ulNumPacketFragments;
    ULONG32			m_ulNumAUFragments;
    ULONG32			m_ulNumAUPackets;

    ULONG32			m_ulMaxPacketFragments;
    ULONG32			m_ulMaxAUFragments;
    ULONG32			m_ulMaxAUPackets;

    HXBOOL			m_bEarliestDeintTimeKnown;
    ULONG32			m_ulEarliestDeintTimeKnown;

    HXBOOL			m_bLastReapedSet;
    HXBOOL			m_bLastReapedMinTime;
    ULONG32			m_ulLastReapedDTS;
    ULONG32			m_ulLastReapedIdx;
    
    ULONG32			m_ulSamplesPerSecond;
    ULONG32			m_ulRTPSamplesPerSecond;

    ULONG32			m_ulAUDuration;
    CFormatConfig		m_Config;

    CTSConverter		m_TSConverter;
};


inline void MP4GPayloadFormat::FlushAUArray(CAUPacket** pArray, 
					    ULONG32 &ulSize)
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

inline void MP4GPayloadFormat::FlushPacketArray(IHXPacket** pArray, 
						ULONG32 &ulSize)
{
    if (ulSize > 0)
    {
	do
	{
	    ulSize--;
	    HX_RELEASE(pArray[ulSize]);
	} while (ulSize > 0);
    }
}

inline HXBOOL MP4GPayloadFormat::MayAUPacketPrecede(CAUPacket* pCandidate, 
						  CAUPacket* pResident)
{
    return ((!pResident->m_bMinTime &&
	     (TestTimeGreater(pResident->m_ulDTS, pCandidate->m_ulDTS) ||
	      ((pResident->m_ulDTS == pCandidate->m_ulDTS) &&
	       TestIndexGreater(pResident->m_ulIdx, pCandidate->m_ulIdx))))
	    ||
	    (pResident->m_bMinTime &&
	     TestIndexGreater(pResident->m_ulIdx, pCandidate->m_ulIdx))
	   );
}


#endif	// _MP4GPYLD_H_
