/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h263vidfmt.h,v 1.7 2005/11/02 15:34:54 ehyche Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef __H263VIDFMT_H__
#define __H263VIDFMT_H__

/****************************************************************************
 *  Defines
 */
#define H263_PIXEL_SIZE		12
#define H263_PIXEL_FORMAT	HX_I420

/****************************************************************************
 *  Includes
 */
#include "vidrendf.h"
#include "hxformt.h"
// #include "rncolor.h"
#ifdef _WIN32
// #include "ddraw.h"
// #include "colormap.h"
// #include "coloracc.h"
#endif  /* _WIN32 */
#include "h263vdec.h"
#include "h263pyld.h"
#include "h263dec.h"

/****************************************************************************
 *  Globals
 */
class CH263VideoRenderer;

/****************************************************************************
 *  CH263VideoFormat
 */
class CH263VideoFormat : public CVideoFormat
{
public:
    /*
     *	Constructor/Destructor
     */
    CH263VideoFormat(IHXCommonClassFactory* pCommonClassFactory,
		     CH263VideoRenderer* pH263VideoRenderer);
    virtual ~CH263VideoFormat();
    
    /*
     *	Public and Customizable functionality - derived
     */
    virtual HX_RESULT Init(IHXValues* pHeader);
    
    virtual void Reset(void);

    virtual HX_RESULT InitBitmapInfoHeader(HXBitmapInfoHeader &bitmapInfoHeader,
					   CMediaPacket* pVideoPacket);

    virtual HXBOOL IsBitmapFormatChanged(HXBitmapInfoHeader &BitmapInfoHeader,
				       CMediaPacket* pVideoPacket);
    class H263DecSpecStruct
    {
    public:
	UINT8 pVendor[4];
	UINT8 pDecoderVersion[1];
	UINT8 pH263Level[1];
	UINT8 pH263Profile[1];
    };
protected:
    /*
     *	Protected but Customizable functionality - derived
     */
    virtual CMediaPacket* CreateAssembledPacket(IHXPacket* pCodecData);
    virtual CMediaPacket* CreateDecodedPacket(CMediaPacket* pFrameToDecode);
    virtual CH263Decoder* CreateDecoder();

    CH263Decoder* m_pDecoder;
    CH263VideoRenderer* m_pH263VideoRenderer;
 
private:

    class DecoderSpecificInfoV10
    {
    public:
	UINT8 pTag[1];
	UINT8 pSize[4];
	H263DecSpecStruct sData;
	UINT8 pMaxWidth[2];
	UINT8 pMaxHeight[2];
    };

    class DecoderSpecificInfoV20
    {
    public:
	UINT8 pSize[4];
	UINT8 pType[4];
	H263DecSpecStruct sData;
    };

    static void KillH263ampleDesc(void* pSampleDesc, void* pUserData);

    void _Reset(void);

    HX_RESULT ConfigFrom3GPPHeader(IHXBuffer* pConfigData);

    IHXPayloadFormatObject* m_pRssm;

    HXxSize* m_pMaxDims;
    HXxSize* m_pAssmDims;
    HXxSize  m_DecoderDims;
    HXxSize  m_LastFrameDims;
    HXxSize  m_DesiredFrameSize;
    ULONG32  m_ulDecoderBufSize;	
};

#endif	// __H263VIDFMT_H__
