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

#ifndef __MP4AUDFMT_H__
#define __MP4AUDFMT_H__

/****************************************************************************
 *  Defines
 */


/****************************************************************************
 *  Includes
 */
#include "audrendf.h"
#include "hxformt.h"
#include "mp4adec.h"

#include "tsconvrt.h"

#include "fmtfact.h"


/****************************************************************************
 *  Globals
 */
class CMP4AudioRenderer;
class IMP4APayloadFormat;

/****************************************************************************
 *  CMP4AudioFormat
 */
class CMP4AudioFormat : public CAudioFormat
{
public:
    /*
     *	Constructor/Destructor
     */
    CMP4AudioFormat(IHXCommonClassFactory* pCommonClassFactory,
		    CMP4AudioRenderer* pMP4AudioRenderer);
    virtual ~CMP4AudioFormat();
    
    /*
     *	Public and Customizable functionality - derived
     */
    virtual HX_RESULT Init(IHXValues* pHeader);

    virtual ULONG32 GetDefaultPreroll(IHXValues* pValues);
    
    virtual void Reset(void);

    virtual HXBOOL CanChangeAudioStream();
protected:
    /*
     *	Protected but Customizable functionality - derived
     */
    virtual CMediaPacket* CreateAssembledPacket(IHXPacket* pCodecData);
    virtual HX_RESULT DecodeAudioData(HXAudioData& audioData,
				      HXBOOL bFlushCodec);
    virtual HX_RESULT CheckDecoderInstance(IHXAudioDecoder* pInst) {return HXR_OK;}
    virtual CMP4ADecoder* CreateDecoder() {return new CMP4ADecoder;}
    virtual void ProcessAssembledFrame(CMediaPacket* pFrame) {}
 
    void RegisterPayloadFormats();
    void _Reset(void);

    HX_RESULT ConfigureRssm(ULONG32 ulAnchorInMs);
    HX_RESULT UpdateAudioFormat(ULONG32& ulAnchorTime, HXBOOL bForceUpdate = FALSE);

    PayloadFormatFactory m_fmtFactory;
    IMP4APayloadFormat* m_pRssm;

    HXBOOL m_bNewAssembledFrame;
    ULONG32 m_ulAUDuration;
    ULONG32 m_ulLastDecodedEndTime;
    ULONG32 m_ulLastFrameTime;
    ULONG32 m_ulCodecDelayMs;

    HXBOOL m_bCanChangeAudioStream;

    ULONG32 m_ulMaxDecoderOutputSamples;
    ULONG32 m_ulMaxDecoderOutputBytes;
    UINT8*  m_pDecoderBuffer;
    UINT32  m_ulDecoderBufferSize;
    IHXAudioDecoder* m_pDecoderInstance;

    IHXAudioDecoderRenderer* m_pDecoderRenderer;

    CMP4AudioRenderer* m_pMP4AudioRenderer;
    CMP4ADecoder* m_pDecoderModule;

    CTSConverter m_InputTSConverter;
    CTSConverter m_TSConverter;
};

#endif	// __MP4AUDFMT_H__
