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

#ifndef _AUDFMT_H_
#define _AUDFMT_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxslist.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxthread.h"
#include "hxausvc.h"
#include "mdpkt.h"


enum AUDIO_STATE
{
    AUDIO_NORMAL,
    AUDIO_DRYNOTIFICATION,
    AUDIO_END_OF_PACKETS
};

#ifndef NO_TIME_SET
#define NO_TIME_SET 0xFFFFFFFF
#endif

#define MAX_AUSTR_SIZE 64

_INTERFACE IHXCommonClassFactory;


/****************************************************************************
 *  Globals
 */
class CAudioRenderer;
class CHXSimpleList;


/////////////////////////////////////////////////////////////////////////////
// 
//  Class:
//
//	CAudioFormat
//
//  Purpose:
//
//	Implementation of a RealAudio File Format reader.
//
class CAudioFormat
{
public:
    /*
     *	Constructor/Destructor
     */
    CAudioFormat(IHXCommonClassFactory* pCommonClassFactory, 
		 CAudioRenderer* pAudioRenderer);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	Public but Fixed Core functionality - not deriveable
     */
    void OverrideFactory(IHXCommonClassFactory* pCommonClassFactory);

    HXBOOL Enqueue(IHXPacket* pCodecData);
    HX_RESULT CreateAudioFrame(HXAudioData& audioData, AUDIO_STATE audioState);

    HX_RESULT GetAudioFormat(HXAudioFormat& audioFmt);

    HXBOOL ClipAudioBuffer(HXAudioData* pAudioData, 
			 UINT32 ulAudioTime, 
			 HXBOOL bFromStart);
    void DiscardAudioUntil(UINT32 ulTimestamp);

    void SetStartTime(ULONG32 ulTime)	{ m_ulStartTime = ulTime; }
    ULONG32 GetStartTime(void)		{ return m_ulStartTime; }

    // Assembled Audio Packet Queue 
    HX_RESULT PutAudioPacket(CMediaPacket* pMediaPacket);
    CMediaPacket* PeekAudioPacket(void);
    CMediaPacket* GetAudioPacket(void);
    UINT32        GetNumQueuedAudioPackets();

    // Byte <-> Ms conversion functions
    UINT32 ConvertMsToBytes(UINT32 ulMs);
    UINT32 ConvertBytesToMs(UINT32 ulBytes);

    // Sample <-> Ms conversion functions
    UINT32 ConvertMsToSamples(UINT32 ulMs);
    UINT32 ConvertSamplesToMs(UINT32 ulSamples);

    // Sample <-> Bytes conversion functions
    UINT32 ConvertBytesToSamples(UINT32 ulBytes);
    UINT32 ConvertSamplesToBytes(UINT32 ulSamples);

    // Compressed Byte <-> Ms conversion functions 
    // (compression ration is needed)
    void SetCompressionRation(double fCompressionRatio);

    UINT32 ConvertCompressedBytesToMs(UINT32 ulBytes);
    UINT32 ConvertMsToCompressedBytes(UINT32 ulMs);

    // Sample <-> Time conversion functions
    UINT32 ConvertSamplesToTime(UINT32 ulSamples)
    {    	
	return (ulSamples / m_pAudioFmt->uChannels);
    }

    UINT32 ConvertTimeToSamples(UINT32 ulSamples)
    {    	
	return (ulSamples * m_pAudioFmt->uChannels);
    }

    // Time <-> Ms conversion routines
    UINT32 ConvertMsToTime(UINT32 ulMs);
    UINT32 ConvertTimeToMs(UINT32 ulTime);

    // Return an AU String
    const char* GetAutoUpgradeString() { return (const char*) &m_szAUStr[0]; }

    /*
     *	Public and Customizable functionality - derive to customize
     */
    virtual HX_RESULT Init(IHXValues* pHeader);
    virtual void Reset(void);

    virtual ULONG32 GetDefaultPreroll(IHXValues* pValues);
    virtual ULONG32 GetMaximumPreroll(IHXValues* pValues);

    // This returns flag saying whether or not this format
    // will change the audio stream parameters on the fly
    // or not (i.e. - successive packets can result in 
    // audio stream parameter changes). If this flag returns
    // FALSE, then the base audio renderer doesn't need to
    // check before every write to audio services
    virtual HXBOOL CanChangeAudioStream() { return FALSE; }

    
protected:
    virtual ~CAudioFormat();

    /*
     *	Renderer's customizable fuctions
     */
    virtual CMediaPacket* CreateAssembledPacket(IHXPacket* pPayloadPacket);
    virtual HX_RESULT DecodeAudioData(HXAudioData& audioData, 
				      HXBOOL bFlushCodec = FALSE);
    UINT32  GetULONG32Property(IHXValues*  pValues,
                               const char* pszName,
                               UINT32      ulDefault = 0);

    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pCommonClassFactory;
    HXAudioFormat* m_pAudioFmt;
    char           m_szAUStr[MAX_AUSTR_SIZE];
    
private:
    void FlushQueues(void);

    inline HXBOOL AdjustAudioData(REF(HXAudioData) audioData);
    
    CHXSimpleList   *m_pPendingPacketQueue;

    HXBOOL	    m_bPostStartTime;
    UINT32	    m_ulTrackStartTime;
    UINT32	    m_ulTrackEndTime;
    UINT32	    m_ulForceDiscardUntilTime;
    UINT32	    m_ulStartTime;

    double	    m_fCompressionRatio;

    LONG32	    m_lRefCount;
};


#endif // ndef _AUDFMT_H_

