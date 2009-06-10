/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: raacentrydrm.cpp,v 1.4 2006/12/08 01:41:19 ping Exp $ 
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

#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "racodec.h"
#include "hxacodec.h"
#include "hxresult.h"
#include "hxassert.h"
#include "racodec.h"
#include "baseobj.h"

#include "aacdecdll.h"
#include "aacdecdrm.h"
#include "aacconstants.h"


STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RACreateDecoderInstance) (const CLSID &clsid, IUnknown** ppUnknown)
{
    HX_RESULT retVal = HXR_FAIL;

    if (ppUnknown)
    {
        CAACDecDRM* pObj = new CAACDecDRM();
        if (pObj)
        {
            retVal = pObj->QueryInterface(clsid, (void**) ppUnknown);
        }
    }

    return retVal;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAOpenCodec2) (RACODEC* pCodecRef, const char* pCodecPath)
{
    return ENTRYPOINT(RACreateDecoderInstance) (IID_IHXAudioDecoder, (IUnknown**)pCodecRef);
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RACloseCodec) (RACODEC codecRef)
{
    if (codecRef)
        ((CAACDecDRM*)codecRef)->Release();
    return HXR_OK ;
}

STDAPI_(VOID_PTR) ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetFlavorProperty)(RACODEC codecRef, UINT16 flvIndex, UINT16 propIndex, UINT16* pSize)
{
    void* pRet = NULL;

    if (codecRef)
        pRet = ((CAACDecDRM*)codecRef)->GetFlavorProperty(flvIndex, propIndex, pSize);

    return pRet;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAInitDecoder) (RACODEC codecRef, void* pParam)
{
    HX_RESULT retVal = HXR_FAIL;

    if (codecRef)
        retVal = ((CAACDecDRM*)codecRef)->_RAInitDecoder((RADECODER_INIT_PARAMS*)pParam);

    return retVal;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RADecode) (RACODEC codecRef, Byte* in, UINT32 inLength, Byte* out, UINT32* pOutLength, UINT32 userData)
{
    HX_RESULT retVal = HXR_FAIL;

    if (codecRef)
        retVal = ((CAACDecDRM*)codecRef)->_RADecode((const UCHAR*)in, inLength, (INT16*)out, pOutLength, userData);

    return retVal;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAFlush) (RACODEC codecRef, Byte* outBuf, UINT32* pOutLength)
{
    HX_RESULT retVal = HXR_FAIL;

    if (codecRef)
        retVal = ((CAACDecDRM*)codecRef)->Reset();

    return retVal;
}

STDAPI_(void) ENTRYPOINTCALLTYPE ENTRYPOINT(RAFreeDecoder) (RACODEC codecRef)
{
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetBackend) (RACODEC codecRef, void*** pFuncList)
{
    return HXR_NOTIMPL ;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RAGetGUID) (UCHAR* pGUID)
{
    return HXR_FAIL;
}

STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(RASetComMode) (RACODEC codecRef)
{
    HX_RESULT retVal = HXR_FAIL;

    if (codecRef)
        retVal = ((CAACDecDRM*)codecRef)->GoSecure();

    return retVal;
}

