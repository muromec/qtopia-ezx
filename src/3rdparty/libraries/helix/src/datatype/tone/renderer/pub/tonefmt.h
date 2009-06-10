/* ***** BEGIN LICENSE BLOCK *****
 *
 * Source last modified: $Id:
 *
 * Copyright Notices:
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
 *
 * Patent Notices: This file may contain technology protected by one or
 * more of the patents listed at www.helixcommunity.org
 *
 * 1.   The contents of this file, and the files included with this file,
 * are protected by copyright controlled by RealNetworks and its
 * licensors, and made available by RealNetworks subject to the current
 * version of the RealNetworks Public Source License (the "RPSL")
 * available at  * http://www.helixcommunity.org/content/rpsl unless
 * you have licensed the file under the current version of the
 * RealNetworks Community Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply.  You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * 2.  Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above.  Please note that RealNetworks and its
 * licensors disclaim any implied patent license under the GPL.
 * If you wish to allow use of your version of this file only under
 * the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting Paragraph 1 above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete Paragraph 1 above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 *
 * This file is part of the Helix DNA Technology.  RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.   Copying, including reproducing, storing,
 * adapting or translating, any or all of this material other than
 * pursuant to the license terms referred to above requires the prior
 * written consent of RealNetworks and its licensors
 *
 * This file, and the files included with this file, is distributed
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TONEFMT_H
#define TONEFMT_H
#include "tonerend.h"
#include "tonegen.h"
#include "hxtonetype.h"

/*#ifndef TONE_OUTPUT_LOG
#define TONE_OUTPUT_LOG
#endif*/

// Forward declarations
typedef _INTERFACE IHXCommonClassFactory IHXCommonClassFactory;
typedef _INTERFACE IHXValues             IHXValues;
typedef _INTERFACE IHXPacket             IHXPacket;
class CTONEAudioRenderer;
class CMediaPacket;
class CTONEGen;

// Legacy flags for audio/x-pn-wav
#define PN_TONE_TWOS_COMPLEMENT 1
#define PN_TONE_ZERO_OFFSET     2

typedef struct _TONE
{
    UINT32 m_uldur;
    BYTE m_uctone;
    BYTE m_ucVol;
}TONE;

class CTONEAudioFormat : public CAudioFormat
{

public:
	CTONEGen	    *m_pToneGen; // refers to Tone generator implemented class
    ts_t            m_tState;    //used to preserve tone state
#ifdef TONE_OUTPUT_LOG
    FILE            *fp;         //Used to write the output sequence to file.
#endif

	CTONEAudioFormat(IHXCommonClassFactory* pCommonClassFactory,
		    CTONEAudioRenderer*     pTONEAudioRenderer);
    virtual ~CTONEAudioFormat();
    virtual HX_RESULT Init(IHXValues* pHeader);
	HX_RESULT SetSeekPktDur(UINT32 timeAfterSeek);

protected:
    enum
    {
        kMimeTypeUnknown,
        kMimeTypeAudioL8,
        kMimeTypeAudioL16,
    };

	CMediaPacket*   m_pMediapkt;
    TONE            m_usToneSeq;
	IUnknown*			m_pContext;
	CTONEAudioRenderer *m_pTONEAudioRenderer;

    UINT32  m_lTimeOffset;
    UINT32  m_ulOffset;//bufferoffset
    UINT32  m_ulBlkOffset[256];
    UINT32  m_ulNoteDur;
	UINT32  m_ulBlkStk[2048];
	INT16   m_usStkTop;
	UINT32	m_ulSeekOffset;
	UINT32  m_ulTtlDur;

    BOOL    m_bFirstCreateAssembledPacket;
    BOOL    m_bSwapSampleBytes;
    BOOL    m_bBlkPasre;
    BOOL    m_bFirstnote;
    BOOL    m_bPCMBufRdy;
    BOOL    m_ubNewToneVol;
	BOOL	m_blclSeekMode;

    BYTE    m_ucRptCount;
	BYTE    m_ucBlkNum;
    BYTE    m_ucMimeType;
    BYTE    m_ucTempo;
    BYTE    m_ucResolution;
    BYTE    *m_ucBufEnd;

    virtual CMediaPacket*   CreateAssembledPacket(IHXPacket* pCodecData);
    virtual HX_RESULT	    DecodeAudioData(HXAudioData& audioData, BOOL bFlushCodec);
    HX_RESULT		        DecodeAudioData(HXAudioData& audioData, HXBOOL bFlushCodec, CMediaPacket *pPacket);
    HX_RESULT       MakeRawPCMData(TONE *toneSeq,BYTE* pPCMBuffer,IHXBuffer* pOutBuffer,UINT16 *pNoteOffset);
    HX_RESULT       RptToneEvnt(TONE *m_usToneSeq,IHXBuffer* pOutBuffer,UINT16 *pNoteOffset,BYTE* pPCMBuffer);
    HX_RESULT	    ParseAudioData(CMediaPacket *pPacket,IHXBuffer *pOutBuffer);
	HX_RESULT	    ParseAudioBlockData(CMediaPacket *pPacket);	
	HX_RESULT		SeekPktData(CMediaPacket *pPacket,UINT32 timeAfterSeek,TONE *toneSeq);
};

#endif /* #ifndef TONEFMT_H */
