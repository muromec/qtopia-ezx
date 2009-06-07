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


#ifndef __MP4VDEC_H__
#define __MP4VDEC_H__

/****************************************************************************
 *  Defines
 */
#define MAX_DECODE_QUALITY  100

/****************************************************************************
 *  Includes
 */
#include "mdpkt.h"
#include "codeclib.h"


/****************************************************************************
 *  Globals
 */
class CMP4VideoFormat;


/****************************************************************************
 *  CMP4VDecoder
 */
class CMP4VDecoder
{
public:
    CMP4VDecoder();
    ~CMP4VDecoder();

    HX_RESULT Init(IUnknown* pContext,
		   CMP4VideoFormat* pVideoFormat,
		   HXxSize* pSize,
		   IHX20MemoryAllocator* pInputAllocator,
		   IHX20MemoryAllocator* pOutputAllocator);
    HX_RESULT Decode(CMediaPacket* pFrameToDeocde,
		     ULONG32 ulQuality);
    HX_RESULT DecodeDone(HXCODEC_DATA* pData);
    HX_RESULT GetImageInfo(HX_MOF* &pImageInfo);
    HX_RESULT SetCPUScalability(HXBOOL bVal);
    
    HX_RESULT Close(void);

    static HX_RESULT STDMETHODCALLTYPE OnNewImage(HXSTREAM streamRef, 
						  HXSTREAM fromStreamRef,	
						  HXCODEC_DATA *pData);

protected:
    HX_RESULT OpenCodec(HX_MOFTAG pmofTag);
    HX_RESULT OpenStream(void);
    virtual CRADynamicCodecLibrary* CreateCodecLibrary();

    void SetCodecQuality(void);
    HX_RESULT GetQualityPreference(UINT16 &usQuality);

    IUnknown*			m_pContext;

    CMP4VideoFormat*		m_pVideoFormat;

    IHX20MemoryAllocator*	m_pInputAllocator;
    IHX20MemoryAllocator*	m_pOutputAllocator;

    CRADynamicCodecLibrary*	m_pCodecLib;
    HXCODEC			m_pCodec;
    HXSTREAM			m_pStream;
    const char*			m_pCodecId;
    HX_MOFTAG			m_moftagOut;
    ULONG32*			m_pImageInfoBuffer;

    ULONG32			m_ulLastTimeStamp;
};

#endif // __MP4VDEC_H__

