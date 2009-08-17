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

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxrendr.h"
#include "baseobj.h"
#include "netbyte.h"
#include "audhead.h"
#include "audrendf.h"
#include "tonefmt.h"
#include "audrend.h"
#include "tonerend.h"
#include <stdio.h>
#include "hxtlogutil.h"


#define BUF_SIZE 442

CTONEAudioFormat::CTONEAudioFormat(IHXCommonClassFactory* pCommonClassFactory,
				 CTONEAudioRenderer*     pTONEAudioRenderer)
    : CAudioFormat(pCommonClassFactory, pTONEAudioRenderer),
	m_pTONEAudioRenderer(pTONEAudioRenderer)
{
	HX_ASSERT(m_pCommonClassFactory);
    HX_ASSERT(m_pTONEAudioRenderer);
    m_ucMimeType       = kMimeTypeUnknown;
    m_bSwapSampleBytes = FALSE;
    m_bFirstCreateAssembledPacket = TRUE;
    m_usToneSeq.m_ucVol = 80;
    m_pMediapkt     = NULL;
    m_lTimeOffset   = 0;
    m_tState.x      = 0 ;
    m_tState.y      = 0 ;
    m_tState.k      = 0 ;
    m_ulOffset      = 0;
    m_bBlkPasre     = TRUE;
    m_ucTempo       = 30 ;
    m_ucResolution  = 64 ;
    m_ubNewToneVol  = FALSE;
    m_bPCMBufRdy    = FALSE;
    m_ulNoteDur     = 0;
    m_ucRptCount    = 0;
    m_ucBlkNum      = (UINT8)-1;
    m_usStkTop      = -1;
    m_ucBufEnd      = NULL;
    m_bFirstnote    = TRUE;
	m_blclSeekMode  = FALSE;
	m_ulSeekOffset  = 0;
	m_ulTtlDur = 0;
	m_pContext = NULL;
	m_pTONEAudioRenderer->AddRef();	
}

CTONEAudioFormat::~CTONEAudioFormat()
{
	HX_RELEASE(m_pTONEAudioRenderer);
	HX_RELEASE(m_pContext);
}

HX_RESULT CTONEAudioFormat::Init(IHXValues* pHeader)
{
    m_pToneGen = new CTONEGen;
    HX_RESULT retVal = CAudioFormat::Init(pHeader);
    if (SUCCEEDED(retVal))
    {
        // Get the mime type
        IHXBuffer* pMimeTypeStr = NULL;
        retVal = pHeader->GetPropertyCString("MimeType", pMimeTypeStr);
        if (SUCCEEDED(retVal))
        {
            pHeader->GetPropertyULONG32("duration", m_ulTtlDur);
			// Get the mime type string
            const char* pszMimeType = (const char*) pMimeTypeStr->GetBuffer();
            // Reset the mime type member variable
            m_ucMimeType = kMimeTypeUnknown;
            // Determine the mime type
            if (!strcmp(pszMimeType, "audio/x-hx-tonesequence"))
            {
                m_ucMimeType = kMimeTypeAudioL16;
		        m_pAudioFmt->uBitsPerSample = 16;
            }
            // Get the endianness
            BOOL bBigEndian = TestBigEndian();
            m_pAudioFmt->uChannels       = 1;
	        m_pAudioFmt->uBitsPerSample  = 16;
	        m_pAudioFmt->ulSamplesPerSec = 44100;
	        //ToneSamplesPerSecond" preference in range of 8 - 44.1KHz, default is 44.1KHz
#define MAX_BLOCK_DUR 100
#ifdef MAX_BLOCK_DUR
			m_pAudioFmt->uMaxBlockSize	 = (UINT32)(44100 * MAX_BLOCK_DUR/1000)*(m_pAudioFmt->uBitsPerSample/8);// MAX_BLOCK_DUR is in miilliseconds
#else
			m_pAudioFmt->uMaxBlockSize	 = (UINT32)(44100*100/1000)*(m_pAudioFmt->uBitsPerSample/8);//samplingrate * duration in milliseconds/1000
#endif
            if(m_ucMimeType == kMimeTypeAudioL16)
            {
                // audio/L16. Since these
                // formats are ALWAYS net-endian (big-endian), then
                // we only need to swap if we are NOT running on
                // a big-endian processor
                if (m_pAudioFmt->uBitsPerSample == 16 && !bBigEndian)
                {
                    m_bSwapSampleBytes = TRUE;
                }
                else
                {
                    m_bSwapSampleBytes = FALSE;
                }
                // We don't need to override the audio stream format
                // parameters for audio/L8 or audio/L16 since they
                // are covered correctly by CAudioFormat::Init().
            }
            else
            {
                retVal = HXR_FAIL;
            }
        }
	HX_RELEASE(pMimeTypeStr);
	m_pContext = m_pTONEAudioRenderer->GetContext();
    m_pContext->AddRef();
    HX_ENABLE_LOGGING(m_pContext);
    }
    return retVal;
}

//No need to enqueue Assembled packets
//Tone audio renderer should ignore any subsequent call to CreateAssembledPacket().
CMediaPacket* CTONEAudioFormat::CreateAssembledPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_FAIL;
	if(m_blclSeekMode)
		m_bFirstCreateAssembledPacket = TRUE;
    if (m_bFirstCreateAssembledPacket)
    {
	    m_bFirstCreateAssembledPacket = FALSE;
		m_bBlkPasre = TRUE;
	
	    if (pPacket)
	    {
	        IHXBuffer* pBuffer = pPacket->GetBuffer();
			
	        if (pBuffer)
	        {
		        UINT32 ulFlags = MDPCKT_USES_IHXBUFFER_FLAG;
		        m_pMediapkt = new CMediaPacket(pBuffer,
					        (UINT8*) pBuffer->GetBuffer(),
					        pBuffer->GetSize(),
					        pBuffer->GetSize(),
					        pPacket->GetTime(),
					        ulFlags,
					        NULL);
                pBuffer->Release();
	        }
        }
    }
    return m_pMediapkt;
}

HX_RESULT CTONEAudioFormat::DecodeAudioData(HXAudioData& audioData,
					   HXBOOL bFlushCodec)
{
    
	HX_RESULT retVal = HXR_FAIL;
	retVal = DecodeAudioData(audioData,
                           bFlushCodec,
                           m_pMediapkt);
	HXLOGL3(HXLOG_TONE,"DecodeAudioData()");
	return retVal;
}

HX_RESULT CTONEAudioFormat::DecodeAudioData(HXAudioData& audioData,
					   BOOL bFlushCodec,
					   CMediaPacket *pPacket)
{
    HX_RESULT retVal = HXR_FAIL;
    m_bPCMBufRdy  = FALSE;
    if (pPacket->m_pData && m_pCommonClassFactory && m_lTimeOffset <= m_ulTtlDur )
    {
        IHXBuffer *pPCMBuffer = NULL;
        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**) &pPCMBuffer);
		
		if (pPacket->m_pData && pPCMBuffer)
        {
            pPCMBuffer->SetSize(m_pAudioFmt->uMaxBlockSize);
			if(m_bBlkPasre)
				retVal = ParseAudioBlockData(pPacket);
            //Seeking the buffer for the provided duration
            if(m_blclSeekMode == TRUE && pPacket->m_pData )
            {
				//Reset the startup paramters before seeking
				m_ulOffset = 0;
				pPacket->m_ulTime = m_lTimeOffset;
				m_ucRptCount = 0;
				pPacket->m_ulTime = 0;
				m_ulNoteDur = 0;
				m_usStkTop = -1;
				//seek the fileoffset to the provided time offset
                SeekPktData(pPacket,m_lTimeOffset,&m_usToneSeq);
            }
            //input : Tone Seq. Packet,output : PCM buffer
			retVal  = ParseAudioData(pPacket,pPCMBuffer); 
			//Disable the Seek Flag
			if(m_blclSeekMode)
			{
				m_blclSeekMode =  FALSE;
			}
        }
        if (SUCCEEDED(retVal))
        {
#ifdef TONE_OUTPUT_LOG
            fp = fopen("test1.pcm","ab+");
#endif
            UINT32 ulRemTimeoffset= m_lTimeOffset - pPacket->m_ulTime;
            IHXBuffer *pPCMtmpBuffer = NULL;
			
            if(ulRemTimeoffset < 100 && ulRemTimeoffset !=0)
            {
                m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**) &pPCMtmpBuffer);
				
                if(pPCMtmpBuffer)
                {
                    UINT32 nRemDurSamples =(UINT32)(44100*ulRemTimeoffset/1000);
                    retVal = pPCMtmpBuffer->Set((UINT8*)pPCMBuffer->GetBuffer(),nRemDurSamples*2);
                    if (SUCCEEDED(retVal))
                    {
                        audioData.pData = pPCMtmpBuffer;
#ifdef TONE_OUTPUT_LOG
                        fwrite(pPCMtmpBuffer->GetBuffer(),pPCMtmpBuffer->GetSize(),1,fp);
#endif
                    }
                }
            }
            else
            {
                audioData.pData            = pPCMBuffer;
#ifdef TONE_OUTPUT_LOG
                fwrite(pPCMBuffer->GetBuffer(),pPCMBuffer->GetSize(),1,fp);
#endif
            }
            audioData.ulAudioTime      = pPacket->m_ulTime;
            pPacket->m_ulTime          = m_lTimeOffset;
            audioData.uAudioStreamType = STREAMING_AUDIO;
            audioData.pData->AddRef();
#ifdef TONE_OUTPUT_LOG
            fclose(fp);
#endif
            if(pPCMtmpBuffer)
                HX_RELEASE(pPCMtmpBuffer);
        }
		if(pPCMBuffer)
			HX_RELEASE(pPCMBuffer);
    }

	return retVal;
}

//check for input and output parameters to be passed for parse function.
HX_RESULT CTONEAudioFormat::ParseAudioData(CMediaPacket *pInPacket,IHXBuffer *pOutBuffer)
{
    HX_RESULT retVal = HXR_FAIL;
    UINT16 noteoffset = 0;
	HXLOGL3(HXLOG_TONE,"ParseAudioData()");

	if (pInPacket->m_pData)
    {
        BYTE*   pBuf      = (BYTE*) pInPacket->m_pData;
        BYTE*   pBufLimit = pBuf + pInPacket->m_ulDataSize;
		pBuf += m_ulSeekOffset + m_ulOffset;
        m_ucBufEnd = pBufLimit;
        if(m_ucRptCount == 0)
        {
           BYTE*   pBufIndx  = (BYTE*) pInPacket->m_pData ;
            while(pBuf < pBufLimit)
	        {
		        switch(*pBuf)
		        {
                    case HXTONE_VERSION:
		            {
			            pBuf += 2;
			            break;
		            }
		            case HXTONE_TEMPO:
		            {
			            m_ucTempo = (UINT8)*(++pBuf);
                        pBuf += 1;
			            break;
		            }
		            case HXTONE_RESOLUTION:
		            {
			            m_ucResolution = (UINT8)*(++pBuf);
                        pBuf += 1;
			            break;
		            }
		            case HXTONE_BLOCK_START:
		            {
                        m_ucBlkNum = (UINT8)*(pBuf+1);
                        pBuf += 2;
                        while(*pBuf != HXTONE_BLOCK_END)
                        {
                            if(*pBuf == HXTONE_PLAY_BLOCK)
                                break;
                            pBuf++;
                        }
			            break;
		            }
		            case HXTONE_BLOCK_END:
		            {
			            m_ucBlkNum = (UINT8)*(++pBuf);
                        if(m_usStkTop>=0)
                        {   
							//remove the m_ulOffset from stack...
                            pBuf = (BYTE *) m_ulBlkStk[m_usStkTop];
                            m_usStkTop--;
                            pBufIndx = (BYTE*) pInPacket->m_pData;
                        }
                        pBuf += 1;
			            break;
		            }
		            case HXTONE_SET_VOLUME:
		            {
			            BYTE m_ucToneVol=(UINT8)*(++pBuf);
                        if(m_usToneSeq.m_ucVol != m_ucToneVol)
                        {
                            m_ubNewToneVol = TRUE;
			                m_usToneSeq.m_ucVol = m_ucToneVol;
                        }
                        pBuf += 1;
			            break;
		            }
		            case HXTONE_REPEAT:
		            {
			            m_ucRptCount = (UINT8)*(pBuf+1);
                        retVal = RptToneEvnt(&m_usToneSeq,pOutBuffer,&noteoffset,pBuf);
                        m_bPCMBufRdy = TRUE;
			            break;
		            }
		            case HXTONE_PLAY_BLOCK:
		            {
		                pBuf += 1;
		                m_ucBlkNum = (UINT8)*pBuf;
                        //store current pbuf address
                        m_usStkTop += 1 ;
                        m_ulBlkStk[m_usStkTop] = (UINT32)pBuf;
                        pBufIndx = (BYTE*) pInPacket->m_pData ;
                        pBuf = (UINT8 *)m_ulBlkOffset[m_ucBlkNum];
                        pBuf += 2;
                        break;
		            }
		            default:
		            {
                        noteoffset = 0;
			            retVal = MakeRawPCMData(&m_usToneSeq,pBuf,pOutBuffer,&noteoffset);
                        m_bPCMBufRdy = TRUE;
			            break;
		            }
                }
		        m_ulOffset = pBuf - pBufIndx + noteoffset;
				m_ulSeekOffset = 0;
                if(m_bPCMBufRdy == TRUE)
		        {
			         break;
                }
            }
        }
        else
        {
            retVal = RptToneEvnt(&m_usToneSeq,pOutBuffer,&noteoffset,pBuf);
            m_ulOffset += noteoffset;
			m_ulOffset += m_ulSeekOffset;
			m_ulSeekOffset = 0;
            m_bPCMBufRdy = TRUE;
        }
        noteoffset = 0;
        if(m_bPCMBufRdy == TRUE)
        {
            if(m_ulNoteDur >= 100)
            {
                m_ulNoteDur -= 100;
                m_lTimeOffset += 100;//100 millisec
            }
            else
            {
                m_lTimeOffset += m_ulNoteDur;
                m_ulNoteDur -= m_ulNoteDur;
            }
        }
    }
	return retVal;	
}

//Generates PCM packets with the provided info.
HX_RESULT CTONEAudioFormat::MakeRawPCMData(TONE *toneSeq, BYTE* pPCMBuffer,IHXBuffer *pOutBuf, UINT16 *pNoteOffset)
{
    HX_RESULT retval = HXR_OK;
    BYTE*   pBuf = pPCMBuffer;
    BYTE*	pOutBuffer = NULL;
    UINT32  nsamples = m_pAudioFmt->uMaxBlockSize/2;
    pOutBuffer = (BYTE *)pOutBuf->GetBuffer();
	HXLOGL3(HXLOG_TONE,"MakeRawPCMData()");

    if(m_ulNoteDur <= m_ulTtlDur && m_ulNoteDur >= 0)
    {
        BYTE  m_ucNewTone = *pBuf;
        if(m_bFirstnote == TRUE)
        {
            toneSeq->m_uctone  = m_ucNewTone;
            m_pToneGen->ToneInit(&m_tState, toneSeq->m_ucVol, m_ucNewTone, 44100);
            m_ubNewToneVol = FALSE;
			m_bFirstnote = FALSE;
        }
        //call tone init,tongen,fade-in and fade-out
        if(toneSeq->m_uctone != m_ucNewTone || m_ubNewToneVol || m_blclSeekMode)
        {
            m_pToneGen->ToneInit(&m_tState, toneSeq->m_ucVol, m_ucNewTone, 44100);
        }
		if( m_ulNoteDur == 0)
		{
			toneSeq->m_uldur = *(pBuf+1) * 60000 /(m_ucTempo * m_ucResolution);
			m_ulNoteDur = toneSeq->m_uldur;
		}
		UINT32 nRemDurSamples = 0;
		if(m_ulNoteDur < 100)
		{			
			//samplingrate * duration in milliseconds/1000;
			nRemDurSamples =(UINT32)(44100*m_ulNoteDur/1000);
			m_pToneGen->ToneGen(&m_tState, (short*)pOutBuffer, nRemDurSamples);
		}
		else
		{
			m_pToneGen->ToneGen(&m_tState, (short*)pOutBuffer, nsamples);
		}
        if(toneSeq->m_uctone != m_ucNewTone || m_ubNewToneVol || m_blclSeekMode)
        {
			if(m_ulNoteDur > 5)
			{
				m_pToneGen->fadeout_loop((short *)pOutBuffer);
			}
            m_ubNewToneVol = FALSE;
			toneSeq->m_uctone  = m_ucNewTone;
        }
		if(m_ulNoteDur<=100)
		{			
			*pNoteOffset = 2;
			m_ucNewTone =*(pBuf+2);
            if((m_ucNewTone != HXTONE_BLOCK_START) &&  (m_ucNewTone != HXTONE_BLOCK_END)
            && (m_ucNewTone != HXTONE_REPEAT) && (m_ucNewTone !=  HXTONE_PLAY_BLOCK) && (pBuf < m_ucBufEnd))
			{
				if(m_ucNewTone == HXTONE_SET_VOLUME)
				{
					BYTE m_ucTone=(UINT8)*(pBuf+3);
					if(toneSeq->m_ucVol != m_ucTone)
					{
						m_ubNewToneVol = TRUE;
					}
				}
				if(m_ubNewToneVol || m_ucNewTone != toneSeq->m_uctone || m_blclSeekMode)
				{
					BYTE *pOutBufoffset;
					if(m_ulNoteDur!=100)
						pOutBufoffset=pOutBuffer+(nRemDurSamples<<1);
					else
						pOutBufoffset=pOutBuffer+(nsamples<<1);
					if(m_ulNoteDur > 5)
					{
						m_pToneGen->fadein_loop((short *)pOutBufoffset - (BUF_SIZE/2));
					}
				}
			}
		}
	}
    return retval;
}

HX_RESULT CTONEAudioFormat::RptToneEvnt(TONE *toneSeq,IHXBuffer* pOutBuf,UINT16 *pNoteOffset,BYTE* pPCMBuffer)
{
    HX_RESULT retval = HXR_OK;
    BYTE    *pOutBuffer = NULL;
    BYTE*   pBuf = pPCMBuffer;
	UINT32 nRemDurSamples = 0;
    UINT32 nsamples = m_pAudioFmt->uMaxBlockSize/2;
    pOutBuffer = (BYTE *)pOutBuf->GetBuffer();
	HXLOGL3(HXLOG_TONE,"RptToneEvnt()");

    if(m_ulNoteDur >= 0 && m_ulNoteDur <= m_ulTtlDur)
    {
		if(m_ubNewToneVol || m_blclSeekMode)
        {
            m_pToneGen->ToneInit(&m_tState, toneSeq->m_ucVol, toneSeq->m_uctone, 44100);
        }
		if( m_ulNoteDur == 0)
		{
			m_ulNoteDur = toneSeq->m_uldur;
		}
		if(m_ulNoteDur <= 100)
		{
			m_ucRptCount--;
			if(m_ucRptCount)
			{
				m_ulNoteDur += toneSeq->m_uldur;
				m_pToneGen->ToneGen(&m_tState,(short *)pOutBuffer, nsamples);
			}
			else
			{
				//samplingrate * duration in milliseconds/1000;
				nRemDurSamples =(UINT32)(44100*m_ulNoteDur/1000);
				m_pToneGen->ToneGen(&m_tState,(short *)pOutBuffer, nRemDurSamples);
			}
		}
		else
		{
			m_pToneGen->ToneGen(&m_tState,(short *)pOutBuffer, nsamples);
		}
		if(m_ubNewToneVol || m_blclSeekMode)
        {
			if(m_ulNoteDur > 5)
			{
				m_pToneGen->fadeout_loop((short *)pOutBuffer);
			}
            m_ubNewToneVol = FALSE;
        }
		if(m_ulNoteDur<=100)
		{			
			*pNoteOffset = 2;
			BYTE m_ucNewTone =*(pBuf+2);
            if((m_ucNewTone != HXTONE_BLOCK_START) &&  (m_ucNewTone != HXTONE_BLOCK_END)
            && (m_ucNewTone != HXTONE_REPEAT) && (m_ucNewTone !=  HXTONE_PLAY_BLOCK) && (pBuf < m_ucBufEnd))
			{
				if(m_ucNewTone == HXTONE_SET_VOLUME)
				{
					BYTE m_ucTone=(UINT8)*(pBuf+3);
					if(toneSeq->m_ucVol != m_ucTone)
					{
						m_ubNewToneVol = TRUE;
					}
				}
				if(m_ubNewToneVol || m_ucNewTone != toneSeq->m_uctone)
				{
					BYTE *pOutBufoffset;
					if(m_ulNoteDur!=100)
						pOutBufoffset=pOutBuffer+(nRemDurSamples<<1);
					else
						pOutBufoffset=pOutBuffer+(nsamples<<1);
					if(m_ulNoteDur > 5)
					{
						m_pToneGen->fadein_loop((short *)pOutBufoffset - (BUF_SIZE/2));
					}
				}
			}
		}
    }
    
	return retval;
}

HX_RESULT CTONEAudioFormat::SeekPktData(CMediaPacket *pInPacket,UINT32 timeAfterSeek,TONE *toneSeq)
{
    
    HX_RESULT retVal = HXR_OK;
    IHXValues* pHdr  = NULL;
    UINT32 ulBlkNum  = -1;
	HXLOGL3(HXLOG_TONE,"seekPktData()");

    if (pInPacket->m_pData)
    {
        BYTE*     pBuf      = pInPacket->m_pData ;
        BYTE*     pBufLimit = pInPacket->m_pData + pInPacket->m_ulDataSize;
        UINT32    ulMSSum   = 0;
        UINT32	  ulEffDur  = 0;
        UINT32    ulResolution = 64;
        UINT32    ulTempo = 0;
        UINT32    ulPrvToneDur = 0;
        while ((pBuf < pBufLimit) && (ulEffDur < timeAfterSeek))
        {            
            switch(*pBuf)
		    {
                case HXTONE_VERSION:
		        {
			        pBuf += 2;
			        break;
		        }
		        case HXTONE_TEMPO:
		        {
			        ulTempo = (UINT8)*(pBuf+1);
                    pBuf += 2;
			        break;
		        }
		        case HXTONE_RESOLUTION:
		        {
			        ulResolution = (UINT8)*(pBuf+1);
                    pBuf += 2;
			        break;
		        }
		        case HXTONE_BLOCK_START:
		        {
			        ulBlkNum = (UINT8)*(pBuf+1);
                    m_ulBlkOffset[ulBlkNum] = (UINT32)(pBuf);
                    pBuf += 2;
                    while(*pBuf != HXTONE_BLOCK_END)
                    {
                        if(*pBuf == HXTONE_PLAY_BLOCK)
                            break;
                        pBuf++;
                    }
                    break;
		        }
		        case HXTONE_BLOCK_END:
		        {
			        ulBlkNum = (UINT8)*(++pBuf);
                    if(m_usStkTop>=0)
                    {   
						//remove the m_ulOffset from stack...
                        pBuf = (BYTE *) m_ulBlkStk[m_usStkTop];
                        m_usStkTop--;
                    }		
                    pBuf += 1;
			        break;
		        }
		        case HXTONE_SET_VOLUME:
		        {
					BYTE m_ucToneVol= *(pBuf + 1);;
					if(toneSeq->m_ucVol != m_ucToneVol)
                    {
						m_ubNewToneVol = TRUE;
			            toneSeq->m_ucVol =  m_ucToneVol;
                    }                    
			        pBuf += 2;
			        break;
		        }
		        case HXTONE_REPEAT:
		        {
                    BYTE ulRptcount =(*(pBuf+1));
			        ulMSSum = (UINT32)ulRptcount * ulPrvToneDur;
                    if((ulMSSum + ulEffDur) > timeAfterSeek)
                    {
                        BYTE ulTmpCount = 1;
                        UINT32 ulMSSum1;
                        ulMSSum1 = ulEffDur;
                        while(ulMSSum1  <= timeAfterSeek)
                        {
                            ulMSSum1 += ulPrvToneDur;
							if(ulMSSum1 >= timeAfterSeek)
								break;
							else
								ulTmpCount++;
                        }
                        m_ucRptCount = ulRptcount - (ulTmpCount);
                        m_ulNoteDur = ulMSSum1 - timeAfterSeek;
                        ulMSSum = timeAfterSeek - ulEffDur;
                    }   
                    else
                    {
                        pBuf += 2;
                    }
                    break;
		        }
		        case HXTONE_PLAY_BLOCK:
		        {      
		            pBuf += 1;
		            ulBlkNum = (UINT8)*pBuf;
                    //store current pbuf address
                    m_usStkTop += 1 ;
                    m_ulBlkStk[m_usStkTop] = (UINT32)pBuf; 
					//moving new buf address to pBuf.
                    pBuf = (UINT8 *)m_ulBlkOffset[ulBlkNum];
                    pBuf += 2;
                    break;
		        }
		        default:
		        {
                    toneSeq->m_uctone = *pBuf;
                    ulMSSum = *(pBuf+1);
                    ulPrvToneDur  = (UINT32) floor((float)((ulMSSum * 60 * 1000) /(ulResolution * ulTempo)));
                    toneSeq->m_uldur = ulPrvToneDur;
                    ulMSSum = ulPrvToneDur;
                    if(ulEffDur+ulPrvToneDur > timeAfterSeek)
                    {
                        UINT32 ulRemDur;
                        ulRemDur = timeAfterSeek - ulEffDur;
                        m_ulNoteDur = ulMSSum - ulRemDur;
                        ulMSSum = ulRemDur;
                    }
                    else
                    {
                        pBuf+=2;
                    }
			        break;
		        }
            }
            ulEffDur += ulMSSum;
            ulMSSum = 0;
        }            
		m_ulSeekOffset = pBuf - pInPacket->m_pData;
		m_ulOffset = 0;
		pInPacket->m_ulTime = m_lTimeOffset;
    }
	
    return HXR_OK;
}

HX_RESULT CTONEAudioFormat::SetSeekPktDur(UINT32 timeAfterSeek)
{
    m_lTimeOffset = timeAfterSeek;
	m_blclSeekMode = TRUE;
    return HXR_OK;
}


HX_RESULT CTONEAudioFormat::ParseAudioBlockData(CMediaPacket *pInPacket)
{
	HX_RESULT retVal = HXR_FAIL;
    //parsing buffer and setting the block offset to m_ulBlkOffset array.
    //Maximum number of blocks should be 255 in a single tone packet.
    if(pInPacket->m_pData && m_bBlkPasre)
    {
        BYTE*   pBuf      = (BYTE*)pInPacket->m_pData;
        BYTE*   pBufLimit = (BYTE*)pBuf + pInPacket->m_ulDataSize;
        unsigned short index = 0;
        while(pBuf <= pBufLimit)
        {
            while(*pBuf != (UINT8)-5)
            {
                pBuf++;
                if(pBuf >= pBufLimit)
                    break;
            }
            if(pBuf < pBufLimit)
            {
                index = *(pBuf+1);
                m_ulBlkOffset[index] = (UINT32)pBuf;
            }
            pBuf++;
        }
        m_bBlkPasre = FALSE;
    }
	return HXR_OK;
}

