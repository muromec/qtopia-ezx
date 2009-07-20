/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0 and Exhibits. 
 * REALNETWORKS CONFIDENTIAL--NOT FOR DISTRIBUTION IN SOURCE CODE FORM 
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. 
 * All Rights Reserved. 
 * 
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Community Source 
 * License Version 1.0 (the "RCSL"), including Attachments A though H, 
 * all available at http://www.helixcommunity.org/content/rcsl. 
 * You may also obtain the license terms directly from RealNetworks. 
 * You may not use this file except in compliance with the RCSL and 
 * its Attachments. There are no redistribution rights for the source 
 * code of this file. Please see the applicable RCSL for the rights, 
 * obligations and limitations governing use of the contents of the file. 
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
 * https://rarvcode-tck.helixcommunity.org 
 * 
 * Contributor(s): 
 * 
 * ***** END LICENSE BLOCK ***** */ 

#ifndef __MP4VDFMT_H__
#define __MP4VDFMT_H__

/****************************************************************************
 *  Defines
 */
#define MP4V_PIXEL_SIZE		12
#define MP4V_PIXEL_FORMAT  HX_I420

/****************************************************************************
 *  Includes
 */
#include "vidrendf.h"
#include "hxformt.h"
#include "mp4vpyld.h"
#include "mp4vdec.h"
#include "fmtfact.h"

/****************************************************************************
 *  Globals
 */
class CMP4VideoRenderer;

/****************************************************************************
 *  CMP4VideoFormat
 */
class CMP4VideoFormat : public CVideoFormat
{
public:
    /*
     *	Constructor/Destructor
     */
    CMP4VideoFormat(IHXCommonClassFactory* pCommonClassFactory,
		    CMP4VideoRenderer* pMP4VideoRenderer,
		    HXBOOL bSecure = FALSE);
    virtual ~CMP4VideoFormat();
    
    /*
     *	Public and Customizable functionality - derived
     */
    virtual HX_RESULT Init(IHXValues* pHeader);

    virtual void OnDecodedPacketRelease(CMediaPacket* &pPacket);
    
    virtual void Reset(void);

    virtual void Close(void);

    virtual HX_RESULT InitBitmapInfoHeader(HXBitmapInfoHeader &bitmapInfoHeader,
					   CMediaPacket* pVideoPacket);

    virtual HXBOOL IsBitmapFormatChanged(HXBitmapInfoHeader &BitmapInfoHeader,
				       CMediaPacket* pVideoPacket);

    virtual ULONG32 GetDefaultPreroll(IHXValues* pValues);

    virtual HX_RESULT SetCPUScalability(HXBOOL bVal);

    /*
     *	Public, Custom functionality
     */
    HX_RESULT DecodeDone(HXCODEC_DATA* pData);

    virtual const char* GetCodecId(void);
    ULONG32 GetBitstreamHeaderSize(void);
    const UINT8* GetBitstreamHeader(void);

    ULONG32 m_ulMaxDecodedFrames;
    CMP4VideoRenderer* m_pMP4VideoRenderer;
    
protected:
    /*
     *	Protected but Customizable functionality - derived
     */
    virtual CMediaPacket* CreateAssembledPacket(IHXPacket* pCodecData);
    virtual CMediaPacket* CreateDecodedPacket(CMediaPacket* pFrameToDecode);
    virtual CMP4VDecoder* CreateDecoder();
    virtual void ProcessAssembledFrame(CMediaPacket* pAssembledFrame) {};

    virtual ULONG32 GetMaxDecodedFrames(void);
    virtual ULONG32 GetMaxDecodedFramesInStep(void);
    void RegisterPayloadFormats();

private:
    static void KillMP4VSampleDesc(void* pSampleDesc, void* pUserData);
    static void KillInputBuffer(void* pBuffer, void* pUserData);
    static void KillOutputBuffer(void* pBuffer, void* pUserData);

    HX_RESULT SetupOutputFormat(HX_MOF* pMof);
    HX_RESULT CreateAllocators(void);

    void FlushDecodedRngBuf(void);

    void _Reset(void);
    inline void ReleaseDecodedPacket(HXCODEC_DATA* &pDecodedData);

    VideoPayloadFormatFactory m_fmtFactory;
    IMP4VPayloadFormat* m_pRssm;

    CHXBufferMemoryAllocator* m_pInputAllocator;

    CRingBuffer* m_pDecodedRngBuf;

    HXxSize m_sMediaSize;
    ULONG32 m_ulWidthContainedInSegment;
    ULONG32 m_ulHeightContainedInSegment;
    HXBitmapInfoHeader* m_pCodecOutputBIH;

    HXBOOL m_bFirstDecode;

    // MBO - to delete
    HXCODEC_DATA m_DecodedPacket;
    HXxSize m_DecoderDims;

protected:
    CMP4VDecoder* m_pDecoder;
};

#endif	// __MP4VDFMT_H__
