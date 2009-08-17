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

#include "mmf/server/mmfcodec.h"
#include "symbianaudiodecoder.h"
#include "symbianswdecoder.h"

HX_RESULT HXEXPORT ENTRYPOINT(RACreateDecoderInstance) (const CLSID &clsid, 
                                                        IUnknown** ppUnknown)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    IHXAudioDecoder* pObj = new HXSymbianSwAudioDecoder;

    if(pObj)
    {
        res = pObj->QueryInterface(clsid, (void**)ppUnknown);

        if(!SUCCEEDED(res))
        {
            HX_DELETE(pObj);
        }
    }

    return res;
}

HX_RESULT HXEXPORT ENTRYPOINT(RAOpenCodec2) (RACODEC* pCodecRef,
                                             const char* pCodecPath)
{
    return ENTRYPOINT(RACreateDecoderInstance) (IID_IHXAudioDecoder,
                                                (IUnknown**)pCodecRef);
}

HX_RESULT HXEXPORT ENTRYPOINT(RACloseCodec) (RACODEC codecRef)
{
    if (codecRef)
    {
        ((HXSymbianSwAudioDecoder*)codecRef)->Release();
    }

    return HXR_OK ;
}

VOID_PTR HXEXPORT ENTRYPOINT(RAGetFlavorProperty)(RACODEC codecRef,
                                                UINT16 flvIndex,
                                                UINT16 propIndex,
                                                UINT16* pSize)
{
    void* pRet = NULL;

    if (codecRef)
    {
        pRet = ((HXSymbianSwAudioDecoder*)codecRef)->GetFlavorProperty(flvIndex,
                                                                   propIndex,
                                                                   pSize);
    }

    return pRet;
}

HX_RESULT HXEXPORT ENTRYPOINT(RAInitDecoder) (RACODEC codecRef, void* pParam)
{
    HX_RESULT retVal = HXR_FAIL;

    if (codecRef)
    {
        retVal = ((HXSymbianSwAudioDecoder*)codecRef)
                         ->_RAInitDecoder((RADECODER_INIT_PARAMS*)pParam);
    }

    return retVal;
}

HX_RESULT HXEXPORT ENTRYPOINT(RADecode) (RACODEC codecRef,
                                         Byte* in,
                                         UINT32 inLength,
                                         Byte* out,
                                         UINT32* pOutLength,
                                         UINT32 userData)
{
    HX_RESULT retVal = HXR_FAIL;

    if (codecRef)
    {
        retVal = ((HXSymbianSwAudioDecoder*)codecRef)->_RADecode((const UCHAR*)in,
                                                             inLength,
                                                             (INT16*)out,
                                                             pOutLength,
                                                             userData);
    }

    return retVal;
}

HX_RESULT HXEXPORT ENTRYPOINT(RAFlush) (RACODEC codecRef,
                    Byte* outBuf,
                    UINT32* pOutLength)
{
    HX_RESULT retVal = HXR_FAIL;

    if (codecRef)
    {
        retVal = ((HXSymbianSwAudioDecoder*)codecRef)->Reset();
    }

    return retVal;
}

void HXEXPORT ENTRYPOINT(RAFreeDecoder) (RACODEC codecRef)
{
}

HX_RESULT HXEXPORT ENTRYPOINT(RAGetBackend) (RACODEC codecRef,
                                             void*** pFuncList)
{
    return HXR_NOTIMPL ;
}

HX_RESULT HXEXPORT ENTRYPOINT(RAGetGUID) (UCHAR* pGUID)
{
    return HXR_FAIL;
}

HX_RESULT HXEXPORT ENTRYPOINT(RASetComMode) (RACODEC codecRef)
{
    return HXR_OK;
}

