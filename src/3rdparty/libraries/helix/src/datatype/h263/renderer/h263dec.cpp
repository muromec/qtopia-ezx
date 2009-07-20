/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h263dec.cpp,v 1.16 2009/04/16 17:18:25 gahluwalia Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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
#include "hxresult.h"
#include "hxassert.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxplugn.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxformt.h"
#include "hxslist.h"
#include "dllaccesbridge.h"
#include "dllpath.h"
#include "hxvh263.h"
#include "h263vdec.h"
#include "h263dec.h"
#include "h263vidfmt.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
 *  Method:
 *    CH263Decoder::CH263Decoder
 *
 */

CH263Decoder::CH263Decoder(IUnknown* pContext)
    : m_ulPreviousFrameType(0)
    , m_fpH263Init(0)
    , m_fpH263Free(0)
    , m_fpH263Transform(0)
    , m_pContext(pContext)
    , m_pCodecAccess(NULL)
    , m_pDecoderState(NULL)
    , m_bMoreFrames(FALSE)
{
    HX_ADDREF(m_pContext);
}

/****************************************************************************
 *  Method:
 *    CH263Decoder::~CH263Decoder
 *
 */
CH263Decoder::~CH263Decoder()
{
    CloseDecoder();
    HX_RELEASE(m_pContext);
}

HX_RESULT 
CH263Decoder::CloseDecoder()
{
    if (m_pDecoderState)
    {
	m_fpH263Free(m_pDecoderState);
	m_pDecoderState = NULL;
    }
    m_fpH263Init = NULL;
    m_fpH263Free = NULL;
    m_fpH263Transform = NULL;
    HX_DELETE(m_pCodecAccess);

    return HXR_OK;
}

/****************************************************************************
 *  Method:
 *    CH263Decoder::CH263Decoder
 *
 */
HX_RESULT
CH263Decoder::InitDecoder(HXxSize* pSize)
{
    HX_RESULT ret = HXR_OK;
    // Build the module name if none is supplied
    char pLibName[16]; /* Flawfinder: ignore */

	// Try to load a codec until OS_BuildLibName does not find one
    for (INT32 i=0;; i++)
    {
        if (!m_pCodecAccess)
        {
            if (OS_BuildLibName(pLibName, 16, i) == FALSE)
            {
                return HXR_FAIL;
            }

            // DLLAccess should be used to provide cross-platform support
            m_pCodecAccess = new DLLAccessBridge(pLibName, DLLTYPE_CODEC, m_pContext);
        }

        if (LoadCodecFunctions())
        {
            // We found a codec and could get its symbols
            if (m_pDecoderState)
            {
                m_fpH263Free(m_pDecoderState);
            }

            m_InitParams.outtype    = T_YUV420_NOCOPY;
            m_InitParams.pels	    = (UINT16)pSize->cx;
            m_InitParams.lines	    = (UINT16)pSize->cy;
            m_InitParams.nPadWidth    = 0;	/* number of columns of padding on right to get 16 x 16 block*/
            m_InitParams.nPadHeight   = 0;/* number of rows of padding on bottom to get 16 x 16 block*/

            m_InitParams.ulInvariants = 0;
            // ulInvariants specifies the invariant picture header bits
            m_InitParams.packetization = TRUE;
            m_InitParams.ulStreamVersion = HX_ENCODE_PROD_VERSION(4,4,0,0);
            m_InitParams.pContext = m_pContext;
            
            HX_ADDREF(m_InitParams.pContext);
            ret = m_fpH263Init(&m_InitParams, &m_pDecoderState);
            HX_RELEASE(m_InitParams.pContext); 
            if(SUCCEEDED(ret))
            {           
                break;
            }
            else
            {
                if (m_pDecoderState)
                {
	                m_fpH263Free(m_pDecoderState);
	                m_pDecoderState = NULL;
                }
                m_fpH263Init = NULL;
                m_fpH263Free = NULL;
                m_fpH263Transform = NULL;
            }
        }
        
        // We could not get the codec's symbols so look for another one
        HX_DELETE(m_pCodecAccess);
    }

    return ret;
}


/****************************************************************************
 *  Method:
 *    CH263Decoder::DecodeFrame()
 */
HX_RESULT
CH263Decoder::DecodeFrame(CMediaPacket* pFrameToDecode, 
			  UINT8* pDecodedBuf,
			  HXxSize* pFrameDims) 
{
    HX_RESULT ret = HXR_OK;

    if (m_fpH263Transform != NULL)
    {
	H263DecoderOutParams OutputParams;
	H263DecoderInParams InputParams;
	
	InputParams.dataLength = pFrameToDecode->m_ulDataSize;
	InputParams.bInterpolateImage = FALSE;
	InputParams.numDataSegments = 0;
	InputParams.pDataSegments = 0;
	InputParams.timestamp = pFrameToDecode->m_ulTime;
	InputParams.flags = 0;

	if (m_bMoreFrames)
	{
	    m_bMoreFrames = FALSE;
	    InputParams.flags |= RV_DECODE_MORE_FRAMES;
	}
		
	if (pDecodedBuf != NULL)
	{
	    ret = VerifyInput(pFrameToDecode, &InputParams);
        
        if (FAILED(ret))
        {
            return ret;
        }

        ret = m_fpH263Transform(pFrameToDecode->m_pData, 
		pDecodedBuf, &InputParams, 
		&OutputParams, m_pDecoderState);
#ifdef XXX_JHUG_DEBUG
	    FILE* f = fopen("C:\\TEMP\\decodeLog.txt", "a+");
	    fprintf(f, "Transform pBuffer, length=%i %s\n", pBuffer->GetSize(), SUCCEEDED(ret) ? "SUCCEEDED":"FAILED");
	    fclose(f);
#endif // XXX_JHUG_DEBUG
	    
	    if (OutputParams.notes & RV_DECODE_KEY_FRAME)
	    {
		m_ulPreviousFrameType = FRAME_TYPE_I;
	    }
	    else
	    {
		if (OutputParams.notes & RV_DECODE_B_FRAME)
		{
		    m_ulPreviousFrameType = FRAME_TYPE_B;
		}
		else
		{
		    m_ulPreviousFrameType = FRAME_TYPE_P;
		}
	    }

	    if (pFrameDims)
	    {
		pFrameDims->cx = OutputParams.width;
		pFrameDims->cy = OutputParams.height;
	    }
	}
	else 
	{
	    m_ulPreviousFrameType = FRAME_TYPE_I;
	}

	if (OutputParams.notes & RV_DECODE_MORE_FRAMES)
	{
	    m_bMoreFrames = TRUE;
	}
	else
	{
	    m_bMoreFrames = FALSE;
	}
    }
    else
    {
	ret = HXR_FAIL;
    }
	
    return ret;
}


/****************************************************************************
 *  Method:
 *    CH263Decoder::OS_BuildLibName
 */
HXBOOL CH263Decoder::OS_BuildLibName(char *pLibName,
                                   UINT32 ulLibNameBufLen,
                                   INT32 nIndex)
{
    HXBOOL rtn = TRUE;

    if (pLibName == NULL)
        return FALSE;

    // Priority scheme for loading h263 backend
    switch (nIndex)
    {
        //try omx decoder first 
         case 0:
            SafeStrCpy(pLibName, "omxv", ulLibNameBufLen);
            break;
        // mpeg4/h263/rv7 combo decoder
        case 1:
            SafeStrCpy(pLibName, "dmp4", ulLibNameBufLen);
            break;
        
        // g2 decoder
        case 2:
        case 3:
            SafeStrCpy(pLibName, "drv2", ulLibNameBufLen);
            break;
        
        // h263 backend
        case 4:
            SafeStrCpy(pLibName, "d263", ulLibNameBufLen);
            break;
        default:
            return FALSE;
    }

#if defined(WIN32) || defined(_WIN32)
    if (2 == nIndex)
    {
        SafeStrCat(pLibName, "3260", ulLibNameBufLen);
    }

    SafeStrCat(pLibName, ".DLL", ulLibNameBufLen);

#elif defined (_MAC_UNIX)
    SafeStrCat(pLibName, ".bundle", ulLibNameBufLen);
#elif _MACINTOSH 
#if defined(_CARBON)
#ifdef _MAC_MACHO
    SafeStrCat(pLibName, ".bundle", ulLibNameBufLen);
#else	// _MAC_MACHO
    SafeStrCat(pLibName, ".shlb", ulLibNameBufLen);
#endif	// _MAC_MACHO
#else	// _CARBON
        SafeStrCat(pLibName, ".DLL", ulLibNameBufLen);
#endif	// _CARBON

#elif defined (_UNIX)
    //
    // codecs are named like this :XXXX.so" where XXXX is the
    // codec id string
    //
    SafeStrCat(pLibName, ".so", ulLibNameBufLen);
#endif

    return rtn;
}


HXBOOL CH263Decoder::LoadCodecFunctions()
{
    HXBOOL bReturn = m_pCodecAccess->isOpen();

    if (bReturn)
    {
        // we got a module so get the symbols we need
	if (!m_fpH263Init)
        {
            m_fpH263Init = (FPTRANSFORMINIT) m_pCodecAccess->getSymbol("RV20toYUV420Init");
        }
        if (!m_fpH263Init)
        {
            m_fpH263Init = (FPTRANSFORMINIT) m_pCodecAccess->getSymbol("HXVtoYUV420Init");
        }
	if (!m_fpH263Free)
        {
            m_fpH263Free = (FPTRANSFORMFREE) m_pCodecAccess->getSymbol("RV20toYUV420Free");
        }
        if (!m_fpH263Free)
        {
            m_fpH263Free = (FPTRANSFORMFREE) m_pCodecAccess->getSymbol("HXVtoYUV420Free");
        }
	if (!m_fpH263Transform)
        {
            m_fpH263Transform = (FPTRANSFORMHXV10TOYUV) m_pCodecAccess->getSymbol("RV20toYUV420Transform");
        }
        if (!m_fpH263Transform)
        {
            m_fpH263Transform = (FPTRANSFORMHXV10TOYUV) m_pCodecAccess->getSymbol("HXVtoYUV420Transform");
        }
	
        if (m_fpH263Init == NULL ||
            m_fpH263Free == NULL ||
            m_fpH263Transform == NULL)
        {
            bReturn = FALSE;
        }
    }

    return bReturn;
}
