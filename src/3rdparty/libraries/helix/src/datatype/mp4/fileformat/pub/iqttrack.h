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

#ifndef _IQTTRACK_H_
#define _IQTTRACK_H_

/****************************************************************************
 *  Defines
 */
#define QTTRK_USE_KEY_FRAMES	FALSE
#define QTTRK_USE_ANY_FRAME	TRUE


/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxresult.h"
#include "hxcomm.h"

#include "ihxpckts.h"


class CQTPacketAssembler;
class CQTFileFormat;
class CQTPacketizerFactory;
class CQTTrackManager;
class CQT_MovieInfo_Manager;
class CQT_ChunkToOffset_Manager;

/****************************************************************************
 * 
 *  Class:
 *	IQTTrackResponse
 *
 *  Purpose:
 *	Implements Quick Time Track Response Interface
 */
class IQTTrackResponse : public IUnknown
{
public:
    /*
     *	IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ 
				    REFIID ID, 
				    void** ppInterfaceObj) PURE;

    STDMETHOD_(UINT32, AddRef )	    (THIS) PURE;

    STDMETHOD_(UINT32, Release)	    (THIS) PURE;

    /*
     *	IQTTrackResponse methods
     */
    virtual HX_RESULT PacketReady(UINT16 uStreamNum, 
				  HX_RESULT status, 
				  IHXPacket* pPacket) = 0;
};

/****************************************************************************
 * 
 *  Class:
 *	IQTTrack
 *
 *  Purpose:
 *	Implements Quick Time Track Interface
 */
class IQTTrack : public IUnknown
{
public:
    /*
     *	IUnknown methods
     */
    STDMETHOD (QueryInterface )	    (THIS_ 
				    REFIID ID, 
				    void** ppInterfaceObj) PURE;

    STDMETHOD_(UINT32, AddRef )	    (THIS) PURE;

    STDMETHOD_(UINT32, Release)	    (THIS) PURE;

    /*
     *	IQTTrack methods
     */
    virtual HX_RESULT Init(IQTTrackResponse* pResponse,
			   CQTFileFormat* pFileFormat,
			   CQTPacketAssembler* pPacketAssembler,
			   CQTPacketizerFactory* pPacketizerFactory,
			   const char* pProtocol = NULL) = 0;

    virtual HX_RESULT SetResponse(IQTTrackResponse* pResponse) = 0;

    virtual void Close(void) = 0;

    virtual HX_RESULT BuildStreamHeader(IHXValues* &pHeader,
					CQT_MovieInfo_Manager* pMovieInfo,
					CQTTrackManager* pTrackManager) = 0;
    virtual HX_RESULT GetPayloadIdentity(IHXValues* pHeader) = 0;

    virtual HX_RESULT Seek(ULONG32 ulTime, HXBOOL bUseSyncPoints = TRUE) = 0;  // Time given in miliseconds
    virtual HX_RESULT GetPacket(UINT16 uStreamNum) = 0;

    virtual HX_RESULT Subscribe(UINT16 uRuleNumber) = 0;
    virtual HX_RESULT Unsubscribe(UINT16 uRuleNumber) = 0;
    virtual HX_RESULT SubscribeDefault(void) = 0;
    virtual HXBOOL IsSubscribed(void) = 0;

    virtual HX_RESULT ComputeTrackSize(ULONG32& ulTrackSizeOut) = 0;    
    virtual ULONG32 GetID(void) = 0;

    virtual ULONG32 GetTrackWidth(void) = 0;
    virtual ULONG32 GetTrackHeight(void) = 0;

    virtual ULONG32 GetSDPLength(void) = 0;
    virtual const UINT8* GetSDP(void) = 0;

    virtual HX_RESULT ObtainTrackBitrate(ULONG32& ulAvgBitrateOut) = 0;

    virtual CQT_ChunkToOffset_Manager* GetChunkToOffsetMgr(void) = 0;
};

#endif  // _IQTTRACK_H_
