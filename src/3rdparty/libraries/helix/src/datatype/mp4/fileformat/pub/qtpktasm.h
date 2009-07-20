/****************************************************************************
 * 
 *  $Id: qtpktasm.h,v 1.3 2005/03/14 19:17:44 bobclark Exp $
 *
 *  Copyright (C) 1995-1999 RealNetworks, Inc. All rights reserved.
 *  
 *  http://www.real.com/devzone
 *
 *  This program contains proprietary 
 *  information of Progressive Networks, Inc, and is licensed
 *  subject to restrictions on use and distribution.
 *
 *
 *  Packet Assembler
 *
 */

#ifndef _QTPKTASM_H_
#define _QTPKTASM_H_

/****************************************************************************
 *  Defines
 */

/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxtypes.h"
#include "hxcomm.h"
#include "qttrack.h"

class CQTTrack;
class CQTRTPSample;
class CQTHintTrack;


/****************************************************************************
 * 
 *  Class:
 *	CQTPacketAssembler
 *
 *  Purpose:
 *	Implements Quick Time Hint Track Packet Assembler
 */
class CQTPacketAssembler : public IUnknown
{
public:
    /*
     *	Constructor/Destructor
     */
    CQTPacketAssembler(void);

    ~CQTPacketAssembler();

    /*
     *	Main Interface
     */
    HX_RESULT Init(CQTFileFormat* pFileFormat);

    HX_RESULT AssemblePacket(	CQTRTPSample* pSample,
				CQTHintTrack* pHintTrack);

    HX_RESULT SegmentReady( HX_RESULT status,
			    IHXBuffer* pBuffer);

    HX_RESULT SegmentReady( HX_RESULT status,
			    IHXBuffer* pBuffer,
			    ULONG32 ulOffset,
			    ULONG32 ulSize);

    void DisablePacketTimeOffset(HXBOOL bDoDisable = TRUE);

    /*
     *	IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ 
				    REFIID ID, 
				    void** ppInterfaceObj);

    STDMETHOD_(UINT32, AddRef )	    (THIS);

    STDMETHOD_(UINT32, Release)	    (THIS);

private:
    HX_RESULT MakePacket(IHXBuffer* pBuffer = NULL);
    inline HX_RESULT LoadPacket(void);
    inline CQTTrack* FindTrackByRefIdx(UINT8 uRefIdx);
    inline HXBOOL NeedToSkipPacket(void);
    HX_RESULT AddSegmentToPacket(HX_RESULT status,
				 IHXBuffer* pBuffer,
				 UINT8* pData,
				 ULONG32 ulOffset,
    				 ULONG32 ulSize);
    HX_RESULT HandleAsyncFailure(HX_RESULT status);

    CQTRTPSample* m_pSample;
    CQTHintTrack* m_pHintTrack;
    CQTTrack* m_pDataTrack;
    IHXCommonClassFactory* m_pClassFactory;
    CQTFileFormat* m_pFileFormat;

    IHXPacket* m_pPacket;
    UINT8* m_pCurrentSegmentStart;
    ULONG32 m_ulCurrentSegmentSize;
    UINT16 m_ulCurrentSegmentIdx;

    UINT16 m_ulSegmentFragments;

    HXBOOL m_bUtilzePacketTimeOffset;

    LONG32  m_lRefCount;
};

#endif  // _QTPKTASM_H_
