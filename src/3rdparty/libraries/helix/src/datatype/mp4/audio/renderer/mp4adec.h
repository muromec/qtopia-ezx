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

#ifndef __MP4ADEC_H__
#define __MP4ADEC_H__

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxacodec.h"
#include "mdpkt.h"
#include "decinfostore.h"

class DLLAccessBridge;
class IMP4APayloadFormat;

#define MAX_LIBNAME_SIZE 16

/****************************************************************************
 *  CMP4ADecoder
 */
class CMP4ADecoder
{
public:
    CMP4ADecoder();
    ~CMP4ADecoder();

    HX_RESULT   Open(IHXValues* pHeader, 
                     IMP4APayloadFormat* pRssm, 
		     IHXAudioDecoder** pAudioDecoder, 
		     IUnknown* pContext);
    HX_RESULT   OpenNext(IHXValues* pHeader, 
                         IMP4APayloadFormat* pRssm, 
		         IHXAudioDecoder** pAudioDecoder, 
		         IUnknown* pContext);
    HX_RESULT   BuildDLLAccessBridge(IHXAudioDecoder** pAudioDecoder, 
                                     IHXBuffer* pMimeType,
                                     IMP4APayloadFormat* pRssm, 
		                     IUnknown* pContext);
    HX_RESULT   Close(void);
    const char* GetAutoUpgradeString() { return m_szAUStr; }
    const char* GetCodecName(void) { return m_pDecoderInfo ? m_pDecoderInfo->GetCodecName() : NULL; }
    const char* GetCodecFourCC(void) { return m_pDecoderInfo ? m_pDecoderInfo->GetCodecFourCC() : NULL; } 
    HXBOOL      CanChangeAudioStream(void) { return m_pDecoderInfo ? m_pDecoderInfo->CanChangeAudioStream() : FALSE; }
    HXBOOL      IsMDFHWDecInfoEnabled() { return m_bMDFHWDecInfoEnabled; }
    HXBOOL      IsMDFSWDecInfoEnabled() { return m_bMDFSWDecInfoEnabled; }

protected:
    virtual HX_RESULT CheckDecoder(IHXAudioDecoder* pAudioDecoder) {return HXR_OK;}

private:
    void RegisterDecoderInfo();
    void InitializeDynamicConfiguration(IUnknown* pContext);
    HXBOOL OS_BuildLibName(const char* pMimeType, char *pLibName, UINT32 ulBufLen);

    DLLAccessBridge* m_pCodecAccess;
    DecoderInfoStore m_decInfoStore;
    CDecoderInfo*    m_pDecoderInfo;
    char             m_szAUStr[MAX_LIBNAME_SIZE]; /* Flawfinder: ignore */
    HXBOOL           m_bMDFHWDecInfoEnabled;
    HXBOOL           m_bMDFSWDecInfoEnabled;
};

#endif // __MP4ADEC_H__
