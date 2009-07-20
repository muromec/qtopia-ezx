/* ***** BEGIN LICENSE BLOCK ***** 
 * Source last modified: $Id:$ 
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

//#ifndef USE_SIMPLE_SEGMENT_PAYLOAD
//#define	USE_SIMPLE_SEGMENT_PAYLOAD  // SimpleSegmentPayload is used in case it
//#endif                              // is required that dwSuggestedBufferSize
                                    // for each of the streams is used.

#include "aviffpln.h"
#include "aviindx.h"
#include "avistrm.h"

#include "hxcom.h"
#include "ihxpckts.h"
#include "hxresult.h"
#include "netbyte.h"
#include "pckunpck.h"

// payload handlers:
#include "sspayld.h"

#include "rtptypes.h"

#include "hxheap.h"

#include "hxtlogutil.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

// Operational defines:
#define DEFAULT_NON_LOCAL_PACKET_SIZE 1500

#define MIN_VIDEO_PREROLL             250
#define MAX_VIDEO_PREROLL             30000
#define MIN_AUDIO_PREROLL             750
#define MAX_AUDIO_PREROLL             30000

#ifdef NET_ENDIAN
#define LE32_TO_HOST(x)  ((x << 24)              | \
                          (x << 8  & 0x00FF0000) | \
                          (x >> 8  & 0x0000FF00) | \
                          (x >> 24 & 0xFF))
#define LE16_TO_HOST(x)  ((x << 8) |               \
                          (x & 0xFF))
#define HOST_TO_LE16(x) (LE16_TO_HOST(x))
#define HOST_TO_LE32(x) (LE32_TO_HOST(x))
#else
#define LE32_TO_HOST(x) (x)
#define LE16_TO_HOST(x) (x)
#define HOST_TO_LE16(x) (x)
#define HOST_TO_LE32(x) (x)
#endif // NET_ENDIAN

#define ENDIAN_SWAP_32(x) ((x << 24)             | \
                          (x << 8  & 0x00FF0000) | \
                          (x >> 8  & 0x0000FF00) | \
                          (x >> 24 & 0xFF))

#define AVI_VIDS_TYPE       HX_MAKE4CC('v', 'i', 'd', 's') /* 'vids' */
#define AVI_AUDS_TYPE       HX_MAKE4CC('a', 'u', 'd', 's') /* 'auds' */

// FourCCs - TODO: make macros?
#define AVI_MJPG_VIDEO      HX_MAKE4CC('G', 'P', 'J', 'M') /* 'GPJM' */

/* WAVE formats */
#define  WAVE_FORMAT_UNKNOWN                    0x0000 /* Microsoft Corporation */

#ifndef  WAVE_FORMAT_PCM
#define  WAVE_FORMAT_PCM                        0x0001
#endif // WAVE_FORMAT_PCM

#define  WAVE_FORMAT_ADPCM                      0x0002 /* Microsoft Corporation */
#define  WAVE_FORMAT_IEEE_FLOAT                 0x0003 /* Microsoft Corporation */
#define  WAVE_FORMAT_VSELP                      0x0004 /* Compaq Computer Corp. */
#define  WAVE_FORMAT_IBM_CVSD                   0x0005 /* IBM Corporation */
#define  WAVE_FORMAT_ALAW                       0x0006 /* Microsoft Corporation */
#define  WAVE_FORMAT_MULAW                      0x0007 /* Microsoft Corporation */
#define  WAVE_FORMAT_DTS                        0x0008 /* Microsoft Corporation */
#define  WAVE_FORMAT_OKI_ADPCM                  0x0010 /* OKI */
#define  WAVE_FORMAT_DVI_ADPCM                  0x0011 /* Intel Corporation */
#define  WAVE_FORMAT_IMA_ADPCM                  (WAVE_FORMAT_DVI_ADPCM) /*  Intel Corporation */
#define  WAVE_FORMAT_MEDIASPACE_ADPCM           0x0012 /* Videologic */
#define  WAVE_FORMAT_SIERRA_ADPCM               0x0013 /* Sierra Semiconductor Corp */
#define  WAVE_FORMAT_G723_ADPCM                 0x0014 /* Antex Electronics Corporation */
#define  WAVE_FORMAT_DIGISTD                    0x0015 /* DSP Solutions, Inc. */
#define  WAVE_FORMAT_DIGIFIX                    0x0016 /* DSP Solutions, Inc. */
#define  WAVE_FORMAT_DIALOGIC_OKI_ADPCM         0x0017 /* Dialogic Corporation */
#define  WAVE_FORMAT_MEDIAVISION_ADPCM          0x0018 /* Media Vision, Inc. */
#define  WAVE_FORMAT_CU_CODEC                   0x0019 /* Hewlett-Packard Company */
#define  WAVE_FORMAT_YAMAHA_ADPCM               0x0020 /* Yamaha Corporation of America */
#define  WAVE_FORMAT_SONARC                     0x0021 /* Speech Compression */
#define  WAVE_FORMAT_DSPGROUP_TRUESPEECH        0x0022 /* DSP Group, Inc */
#define  WAVE_FORMAT_ECHOSC1                    0x0023 /* Echo Speech Corporation */
#define  WAVE_FORMAT_AUDIOFILE_AF36             0x0024 /* Virtual Music, Inc. */
#define  WAVE_FORMAT_APTX                       0x0025 /* Audio Processing Technology */
#define  WAVE_FORMAT_AUDIOFILE_AF10             0x0026 /* Virtual Music, Inc. */
#define  WAVE_FORMAT_PROSODY_1612               0x0027 /* Aculab plc */
#define  WAVE_FORMAT_LRC                        0x0028 /* Merging Technologies S.A. */
#define  WAVE_FORMAT_DOLBY_AC2                  0x0030 /* Dolby Laboratories */
#define  WAVE_FORMAT_GSM610                     0x0031 /* Microsoft Corporation */
#define  WAVE_FORMAT_MSNAUDIO                   0x0032 /* Microsoft Corporation */
#define  WAVE_FORMAT_ANTEX_ADPCME               0x0033 /* Antex Electronics Corporation */
#define  WAVE_FORMAT_CONTROL_RES_VQLPC          0x0034 /* Control Resources Limited */
#define  WAVE_FORMAT_DIGIREAL                   0x0035 /* DSP Solutions, Inc. */
#define  WAVE_FORMAT_DIGIADPCM                  0x0036 /* DSP Solutions, Inc. */
#define  WAVE_FORMAT_CONTROL_RES_CR10           0x0037 /* Control Resources Limited */
#define  WAVE_FORMAT_NMS_VBXADPCM               0x0038 /* Natural MicroSystems */
#define  WAVE_FORMAT_CS_IMAADPCM                0x0039 /* Crystal Semiconductor IMA ADPCM */
#define  WAVE_FORMAT_ECHOSC3                    0x003A /* Echo Speech Corporation */
#define  WAVE_FORMAT_ROCKWELL_ADPCM             0x003B /* Rockwell International */
#define  WAVE_FORMAT_ROCKWELL_DIGITALK          0x003C /* Rockwell International */
#define  WAVE_FORMAT_XEBEC                      0x003D /* Xebec Multimedia Solutions Limited */
#define  WAVE_FORMAT_G721_ADPCM                 0x0040 /* Antex Electronics Corporation */
#define  WAVE_FORMAT_G728_CELP                  0x0041 /* Antex Electronics Corporation */
#define  WAVE_FORMAT_MSG723                     0x0042 /* Microsoft Corporation */
#define  WAVE_FORMAT_MPEG                       0x0050 /* Microsoft Corporation */
#define  WAVE_FORMAT_RT24                       0x0052 /* InSoft, Inc. */
#define  WAVE_FORMAT_PAC                        0x0053 /* InSoft, Inc. */
#define  WAVE_FORMAT_MPEGLAYER3                 0x0055 /* ISO/MPEG Layer3 Format Tag */
#define  WAVE_FORMAT_LUCENT_G723                0x0059 /* Lucent Technologies */
#define  WAVE_FORMAT_CIRRUS                     0x0060 /* Cirrus Logic */
#define  WAVE_FORMAT_ESPCM                      0x0061 /* ESS Technology */
#define  WAVE_FORMAT_VOXWARE                    0x0062 /* Voxware Inc */
#define  WAVE_FORMAT_CANOPUS_ATRAC              0x0063 /* Canopus, co., Ltd. */
#define  WAVE_FORMAT_G726_ADPCM                 0x0064 /* APICOM */
#define  WAVE_FORMAT_G722_ADPCM                 0x0065 /* APICOM */
#define  WAVE_FORMAT_DSAT_DISPLAY               0x0067 /* Microsoft Corporation */
#define  WAVE_FORMAT_VOXWARE_BYTE_ALIGNED       0x0069 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_AC8                0x0070 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_AC10               0x0071 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_AC16               0x0072 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_AC20               0x0073 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_RT24               0x0074 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_RT29               0x0075 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_RT29HW             0x0076 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_VR12               0x0077 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_VR18               0x0078 /* Voxware Inc */
#define  WAVE_FORMAT_VOXWARE_TQ40               0x0079 /* Voxware Inc */
#define  WAVE_FORMAT_SOFTSOUND                  0x0080 /* Softsound, Ltd. */
#define  WAVE_FORMAT_VOXWARE_TQ60               0x0081 /* Voxware Inc */
#define  WAVE_FORMAT_MSRT24                     0x0082 /* Microsoft Corporation */
#define  WAVE_FORMAT_G729A                      0x0083 /* AT&T Labs, Inc. */
#define  WAVE_FORMAT_MVI_MVI2                   0x0084 /* Motion Pixels */
#define  WAVE_FORMAT_DF_G726                    0x0085 /* DataFusion Systems (Pty) (Ltd) */
#define  WAVE_FORMAT_DF_GSM610                  0x0086 /* DataFusion Systems (Pty) (Ltd) */
#define  WAVE_FORMAT_ISIAUDIO                   0x0088 /* Iterated Systems, Inc. */
#define  WAVE_FORMAT_ONLIVE                     0x0089 /* OnLive! Technologies, Inc. */
#define  WAVE_FORMAT_SBC24                      0x0091 /* Siemens Business Communications Sys */
#define  WAVE_FORMAT_DOLBY_AC3_SPDIF            0x0092 /* Sonic Foundry */
#define  WAVE_FORMAT_MEDIASONIC_G723            0x0093 /* MediaSonic */
#define  WAVE_FORMAT_PROSODY_8KBPS              0x0094 /* Aculab plc */
#define  WAVE_FORMAT_ZYXEL_ADPCM                0x0097 /* ZyXEL Communications, Inc. */
#define  WAVE_FORMAT_PHILIPS_LPCBB              0x0098 /* Philips Speech Processing */
#define  WAVE_FORMAT_PACKED                     0x0099 /* Studer Professional Audio AG */
#define  WAVE_FORMAT_MALDEN_PHONYTALK           0x00A0 /* Malden Electronics Ltd. */
#define  WAVE_FORMAT_RHETOREX_ADPCM             0x0100 /* Rhetorex Inc. */
#define  WAVE_FORMAT_IRAT                       0x0101 /* BeCubed Software Inc. */
#define  WAVE_FORMAT_VIVO_G723                  0x0111 /* Vivo Software */
#define  WAVE_FORMAT_VIVO_SIREN                 0x0112 /* Vivo Software */
#define  WAVE_FORMAT_DIGITAL_G723               0x0123 /* Digital Equipment Corporation */
#define  WAVE_FORMAT_SANYO_LD_ADPCM             0x0125 /* Sanyo Electric Co., Ltd. */
#define  WAVE_FORMAT_SIPROLAB_ACEPLNET          0x0130 /* Sipro Lab Telecom Inc. */
#define  WAVE_FORMAT_SIPROLAB_ACELP4800         0x0131 /* Sipro Lab Telecom Inc. */
#define  WAVE_FORMAT_SIPROLAB_ACELP8V3          0x0132 /* Sipro Lab Telecom Inc. */
#define  WAVE_FORMAT_SIPROLAB_G729              0x0133 /* Sipro Lab Telecom Inc. */
#define  WAVE_FORMAT_SIPROLAB_G729A             0x0134 /* Sipro Lab Telecom Inc. */
#define  WAVE_FORMAT_SIPROLAB_KELVIN            0x0135 /* Sipro Lab Telecom Inc. */
#define  WAVE_FORMAT_G726ADPCM                  0x0140 /* Dictaphone Corporation */
#define  WAVE_FORMAT_QUALCOMM_PUREVOICE         0x0150 /* Qualcomm, Inc. */
#define  WAVE_FORMAT_QUALCOMM_HALFRATE          0x0151 /* Qualcomm, Inc. */
#define  WAVE_FORMAT_TUBGSM                     0x0155 /* Ring Zero Systems, Inc. */
#define  WAVE_FORMAT_MSAUDIO1                   0x0160 /* Microsoft Corporation */
#define  WAVE_FORMAT_CREATIVE_ADPCM             0x0200 /* Creative Labs, Inc */
#define  WAVE_FORMAT_CREATIVE_FASTSPEECH8       0x0202 /* Creative Labs, Inc */
#define  WAVE_FORMAT_CREATIVE_FASTSPEECH10      0x0203 /* Creative Labs, Inc */
#define  WAVE_FORMAT_UHER_ADPCM                 0x0210 /* UHER informatic GmbH */
#define  WAVE_FORMAT_QUARTERDECK                0x0220 /* Quarterdeck Corporation */
#define  WAVE_FORMAT_ILINK_VC                   0x0230 /* I-link Worldwide */
#define  WAVE_FORMAT_RAW_SPORT                  0x0240 /* Aureal Semiconductor */
#define  WAVE_FORMAT_IPI_HSX                    0x0250 /* Interactive Products, Inc. */
#define  WAVE_FORMAT_IPI_RPELP                  0x0251 /* Interactive Products, Inc. */
#define  WAVE_FORMAT_CS2                        0x0260 /* Consistent Software */
#define  WAVE_FORMAT_SONY_SCX                   0x0270 /* Sony Corp. */
#define  WAVE_FORMAT_FM_TOWNS_SND               0x0300 /* Fujitsu Corp. */
#define  WAVE_FORMAT_BTV_DIGITAL                0x0400 /* Brooktree Corporation */
#define  WAVE_FORMAT_QDESIGN_MUSIC              0x0450 /* QDesign Corporation */
#define  WAVE_FORMAT_VME_VMPCM                  0x0680 /* AT&T Labs, Inc. */
#define  WAVE_FORMAT_TPC                        0x0681 /* AT&T Labs, Inc. */
#define  WAVE_FORMAT_OLIGSM                     0x1000 /* Ing C. Olivetti & C., S.p.A. */
#define  WAVE_FORMAT_OLIADPCM                   0x1001 /* Ing C. Olivetti & C., S.p.A. */
#define  WAVE_FORMAT_OLICELP                    0x1002 /* Ing C. Olivetti & C., S.p.A. */
#define  WAVE_FORMAT_OLISBC                     0x1003 /* Ing C. Olivetti & C., S.p.A. */
#define  WAVE_FORMAT_OLIOPR                     0x1004 /* Ing C. Olivetti & C., S.p.A. */
#define  WAVE_FORMAT_LH_CODEC                   0x1100 /* Lernout & Hauspie */
#define  WAVE_FORMAT_NORRIS                     0x1400 /* Norris Communications, Inc. */
#define  WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS    0x1500 /* AT&T Labs, Inc. */
#define  WAVE_FORMAT_DVM                        0x2000 /* FAST Multimedia AG */
#define  WAVE_FORMAT_EXTENSIBLE                 0xFFFE /* Microsoft */
#define  WAVE_FORMAT_DEVELOPMENT                0xFFFF /* new wave formats in development */

#define  VIDEO_FORMAT_H263 HX_MAKE4CC('H', '2', '6', '3')
#define  VIDEO_FORMAT_DIVX HX_MAKE4CC('d', 'i', 'v', 'x')

#define AVI_LIST_OBJECT     0x4c495354 /* 'LIST' */
#define AVI_RECORD_TYPE     0x72656320 /* 'rec ' */

typedef struct _videohdr
{
    char   format [4];  // 'raw1' means uncompressed AVI rgb
    UINT16 width;
    UINT16 height;
    UINT16 depth;       // bits per pixel
} QT_VIDEO_HEADER;
#define SIZE_OF_QT_VIDEO_HEADER 10

#ifndef BI_RGB 
// This is defined by a windows header, so define it for non-windows platforms 
#define BI_RGB 0 
#endif 

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::CAVIStream
//
CAVIStream::CAVIStream(CAVIFileFormat* pOuter, UINT16 usStream,
                       CAVIFileFormat::MainHeader& MainHeader,
                       BOOL bLocalPlayback, IUnknown* pContext)
    : m_fChunksPerSecond(0)
    , m_fSamplesPerSecond(0)
    , m_pFormat(NULL)
    , m_bEndianSwap16(FALSE)
    , m_state(ePreReader)
    , m_bSeeking(FALSE)
    , m_pOuter(pOuter)
    , m_pIndex(NULL)
    , m_mainHeader(MainHeader)
    , m_bPendingHeaderRequest(FALSE)
    , m_usStream(usStream)
    , m_ulPendingPacketRequests(0)
    , m_pPayloadFormatter(NULL)
    , m_bLocalPlayback(bLocalPlayback)
    , m_pNextPacket(NULL)
    , m_ePacketFormat(PFMT_RDT)
    , m_pReader(NULL)
    , m_ulMinReadChunk(0)
    , m_ulMaxReadChunk(0)
    , m_bRead(FALSE)
    , m_bDiscardPendingIO(FALSE)
    , m_ulChunkReadTarget(NULL)
    , m_pContext(pContext)
    , m_pCommonClassFactory(NULL)
    , m_lRefCount(0)
    , m_ulTotalBytesRead(0)
    , m_ulTotalBytesPacketised(0)
    , m_ulSeekTime (0)
    , m_pFile (NULL)
{
    HX_ASSERT_VALID_PTR(m_pOuter);
    HX_ASSERT_VALID_PTR(m_pContext);
    HX_ADDREF(m_pContext);
    HX_ADDREF(m_pOuter);

    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                               (void**) &m_pCommonClassFactory);
    HX_ASSERT_VALID_PTR(m_pCommonClassFactory);

#ifdef _DEBUG
    for (int i = 0; i < MAX_CHUNK_PREFETCH; i++)
    {
        m_chunkPrefetchArray[i].pBuffer = NULL;
    }
#endif
}


/////////////////////////////////////////////////////////////////////////
//  CAVIStream::~CAVIStream
//
CAVIStream::~CAVIStream()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::~CAVIStream()\n");
/*    HX_RELEASE(m_pReader);
    HX_DELETE(m_pFormat);
    HX_RELEASE(m_pPayloadFormatter);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pContext);
	HX_RELEASE(m_pOuter);	*/
    // Don't recount m_pIndex; it aggregates m_pOuter as well
}


// Header methods:

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::SetHeader
//
HX_RESULT CAVIStream::SetHeader(UINT16 usStream, IHXBuffer* pHeader)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::SetHeader()\n");
    HX_RESULT result = HXR_FAIL;

    UCHAR* buf;
    ULONG32 len;

    if (pHeader && SUCCEEDED(result = pHeader->Get(buf, len)))
    {
        HX_ASSERT(len >= sizeof(m_header));
        if (len < sizeof(m_header))
        {
            return HXR_FAIL;
        }

        m_header.ulType                 = LE32_TO_HOST(*(UINT32*) &buf[0]);
        m_header.ulHandler              = LE32_TO_HOST(*(UINT32*) &buf[4]);
        m_header.ulFlags                = LE32_TO_HOST(*(UINT32*) &buf[8]);
        m_header.sPriority              = (INT16) LE16_TO_HOST(*(UINT32*) &buf[12]);
        m_header.sLanguage              = (INT16) LE16_TO_HOST(*(UINT32*) &buf[14]);
        m_header.ulInitialFrames        = LE32_TO_HOST(*(UINT32*) &buf[16]);
        m_header.ulScale                = LE32_TO_HOST(*(UINT32*) &buf[20]);
        m_header.ulRate                 = LE32_TO_HOST(*(UINT32*) &buf[24]);
        m_header.ulStart                = LE32_TO_HOST(*(UINT32*) &buf[28]);
        m_header.ulLength               = LE32_TO_HOST(*(UINT32*) &buf[32]);
        m_header.ulSuggestedBufferSize  = LE32_TO_HOST(*(UINT32*) &buf[36]);
        m_header.ulQuality              = LE32_TO_HOST(*(UINT32*) &buf[40]);
        m_header.ulSampleSize           = LE32_TO_HOST(*(UINT32*) &buf[44]);
        m_header.sTop                   = (INT16) LE16_TO_HOST(*(UINT32*) &buf[48]);
        m_header.sLeft                  = (INT16) LE16_TO_HOST(*(UINT32*) &buf[50]);
        m_header.sBottom                = (INT16) LE16_TO_HOST(*(UINT32*) &buf[52]);
        m_header.sRight                 = (INT16) LE16_TO_HOST(*(UINT32*) &buf[54]);
    }

    return result;
}


/////////////////////////////////////////////////////////////////////////
//  CAVIStream::SetFormat
//
HX_RESULT CAVIStream::SetFormat(IHXBuffer* pFormat)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::SetFormat()\n");
    HX_ASSERT(pFormat);
    HX_RESULT result = HXR_FAIL;

    UCHAR* buf;
    ULONG32 len;

    if (pFormat && SUCCEEDED(result = pFormat->Get(buf, len)))
    {
        switch ( m_header.ulType )
        {
            case AVI_VIDS_TYPE:
            {
                if (len < sizeof(BitmapInfo) - sizeof(RGBQuad[MAX_COLOR_TABLE_SIZE]))
                {
                    return HXR_FAIL;
                }

                m_pFormat = new UCHAR[len];
                memcpy(m_pFormat, buf, len);

                BitmapInfo* bi = (BitmapInfo*) m_pFormat;
                bi->ulSize          = LE32_TO_HOST(bi->ulSize);
                bi->ulWidth         = LE32_TO_HOST(bi->ulWidth);
                bi->ulHeight        = LE32_TO_HOST(bi->ulHeight);
                bi->usPlanes        = LE16_TO_HOST(bi->usPlanes);
                bi->usBitCount      = LE16_TO_HOST(bi->usBitCount);
                bi->ulCompression   = LE32_TO_HOST(bi->ulCompression);
                bi->ulSizeImage     = LE32_TO_HOST(bi->ulSizeImage);
                bi->ulXPelsPerMeter = LE32_TO_HOST(bi->ulXPelsPerMeter);
                bi->ulYPelsPerMeter = LE32_TO_HOST(bi->ulYPelsPerMeter);
                bi->ulClrUsed       = LE32_TO_HOST(bi->ulClrUsed);
                bi->ulClrImportant  = LE32_TO_HOST(bi->ulClrImportant);

                //XXXEH- this is a hack to get around bad bi->m_sizeImage values
                // in MS-RLE avi files that use only 16 colors of the 8-bit-wide
                // palette (256 colors).  The fix that works (until I get MS to
                // tell me what's really going on) is to change the m_sizeImage
                // to be exactly the width times the height:
                if ( 16L == bi->ulClrUsed  &&  8L == bi->usBitCount )
                {
                    bi->ulSizeImage = bi->ulWidth * bi->ulHeight;
                }

                // XXXJR Need to Make sure we actually read enough for this
                if ( bi->usBitCount <= 8 )
                {
                    int i, bpos;
                    for ( i = 0, bpos = 40; i < (int) bi->ulClrUsed; i++)
                    {
                        bi->rgbQuad[i].ubRed   = buf[bpos++];
                        bi->rgbQuad[i].ubGreen = buf[bpos++];
                        bi->rgbQuad[i].ubBlue  = buf[bpos++];
                        bi->rgbQuad[i].ubReserved = 0; bpos++;
                    }
                }

                break;
            }
            case AVI_AUDS_TYPE:
            {
                // Some AVIs don't include a usSize
                if (len < sizeof(WaveInfo) - 2)
                {
                    return HXR_FAIL;
                }

                int nPadding = sizeof(WaveInfo) - len;

                if(nPadding < 0)
                {
                    nPadding = 0;
                }

                m_pFormat = new UCHAR[len + nPadding];
                memcpy(m_pFormat, buf, len);

                HXLOGL3(HXLOG_AVIX, "CAVIStream::SetFormat with wave m_pFormat=%p\n", m_pFormat);

                if(nPadding > 0)
                {
                    memset(m_pFormat + len, 0, nPadding);
                }

                WaveInfo* bi = (WaveInfo*) m_pFormat;

                bi->usFormatTag         = LE16_TO_HOST(bi->usFormatTag);
                bi->usChannels          = LE16_TO_HOST(bi->usChannels);
                bi->ulSamplesPerSec     = LE32_TO_HOST(bi->ulSamplesPerSec);
                bi->ulAvgBytesPerSec    = LE32_TO_HOST(bi->ulAvgBytesPerSec);
                bi->usBlockAlign        = LE16_TO_HOST(bi->usBlockAlign);
                bi->usBitsPerSample     = LE16_TO_HOST(bi->usBitsPerSample);

                if (len < sizeof(WaveInfo))
                {
                    bi->usSize = 0;
                }
                bi->usSize              = LE16_TO_HOST(bi->usSize);

                break;
            }
        }
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::SetPacketFormat
//
HX_RESULT CAVIStream::SetPacketFormat(UINT32 ePacketFormat)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::SetPacketFormat()\n");
    HX_ASSERT(m_state <= ePreHeader);

    m_ePacketFormat = (PacketFormat) ePacketFormat;
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::SetOpaque
//
HX_RESULT CAVIStream::SetOpaque(IHXBuffer* pOpaque)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::SetOpaque()\n");
    return HXR_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::SetIndex
//
HX_RESULT CAVIStream::SetIndex(CAVIIndex* pIndex)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::SetIndex()\n");
    HX_ASSERT(!m_pIndex);
    HX_ASSERT_VALID_PTR(pIndex);
    m_pIndex = pIndex;

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::SetPendingHeaderRequest
//
void CAVIStream::SetPendingHeaderRequest()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::SetPendingHeaderRequest()\n");
    m_bPendingHeaderRequest = TRUE;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::PendingHeaderRequest
//
BOOL CAVIStream::PendingHeaderRequest()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::PendingHeaderRequest()\n");
    return m_bPendingHeaderRequest;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::GetHeader
//
HX_RESULT CAVIStream::GetHeader(IHXValues* pHeader)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::GetHeader()\n");
    HX_ASSERT(pHeader);

    HX_ASSERT(m_bPendingHeaderRequest);
    m_bPendingHeaderRequest = FALSE;
    HX_RESULT result = HXR_OK;

    // Opaque data, MIME type, payload type, packet format:
    IHXBuffer* pMTBuf = NULL;
    IHXBuffer* pSNBuf = NULL;

    IHXBuffer* pOpaqueData = NULL;
    if ( HXR_OK != m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                         (void**)&pOpaqueData) )
    {
        pOpaqueData = NULL;
        return HXR_OUTOFMEMORY;
    }

    if ( (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                          (void**)&pMTBuf)) &&
         (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                          (void**)&pSNBuf)))
    {
        char    szMimeType[256] = {0};
        char    szStreamName[256] = {0};
        INT32   nRTPPayloadType = -1;

        UINT32  ulMaxPacketSize = m_header.ulSuggestedBufferSize;

        if (!ulMaxPacketSize)
        {
            ulMaxPacketSize = DEFAULT_NON_LOCAL_PACKET_SIZE;
        }

        HX_RESULT result = HXR_OK;

        switch (m_header.ulType)
        {
            case AVI_VIDS_TYPE:
            {
                BitmapInfo* bi = (BitmapInfo*) m_pFormat;

                pHeader->SetPropertyULONG32("Width", bi->ulWidth);
                pHeader->SetPropertyULONG32("Height", bi->ulHeight);

                if (bi->ulCompression == BI_RGB)
                {
                    QT_VIDEO_HEADER vhead;
                    strncpy(vhead.format, "raw1", 4);
                    vhead.width = WToNet((UINT16) bi->ulWidth);
                    vhead.height = WToNet((UINT16) bi->ulHeight);
                    vhead.depth = WToNet(bi->usBitCount);

                    // Copy palette information:
                    if (bi->usBitCount <= 8)
                    {
                        // Here we have a problem - old RGB renderers can't handle
                        // palettes.  We could send this to the ICM renderer, but
                        // that wouldn't be cross platform.  Instead we increment
                        // the stream version and push for an AU on the renderer end:
                        // TODO: Increment stream version
                        //memcpy(hi.bitmapInfo.m_rgbQuad, bi->m_rgbQuad, bi->ulClrUsed);
                    }

                    pOpaqueData->Set((UCHAR*) &vhead,
                                     (ULONG32) SIZE_OF_QT_VIDEO_HEADER);
                    strcpy(szMimeType, "video/x-pn-qtvideo-stream");

                    pHeader->SetPropertyULONG32("MicrosecondsPerFrame",
                               (ULONG32) (1e6 * ((double) m_header.ulScale / m_header.ulRate)));
                    //HX_TRACE("\t\tMicrosecondsPerFrame:\t%lu\n", (ULONG32) 1e6 * ((double) m_header.ulScale / m_header.ulRate));
                }
                else
                {
#if 0 // JPEGPayloadFormat not supported (yet)
                    if ( bi->ulCompression == AVI_MJPG_VIDEO )
                    {
                        if (!m_bLocalPlayback)
                        {
                            m_pPayloadFormatter = new JPEGPayloadFormat();
                            if (!m_pPayloadFormatter)
                            {
                                HX_RELEASE(pOpaqueData);
                                HX_RELEASE(pSNBuf);
                                HX_RELEASE(pMTBuf);
                                return HXR_OUTOFMEMORY;
                            }
                            HX_ADDREF(m_pPayloadFormatter);
                            m_pPayloadFormatter->Init(m_pContext, TRUE);
                            // We'll set the headers below.
                        }
                        strcpy(szMimeType, "video/x-pn-jpeg-plugin");
                        nRTPPayloadType = RTP_PAYLOAD_JPEG;
                    }
                    else
#endif // 0
                    {
                        if (m_header.ulHandler == VIDEO_FORMAT_H263)
                        {
                            SetCStringPropertyCCF(pHeader, "ASMRuleBook", "Marker=0;Marker=1;",
                                m_pContext);

                            strcpy(szStreamName, "Video Track");
                            strcpy (szMimeType, "video/X-RN-3GPP-H263");
                        }
                        else if (m_header.ulHandler == VIDEO_FORMAT_DIVX)
                        {
                            strcpy(szStreamName, "Video Track");
                            strcpy (szMimeType, "video/X-HX-DIVX");
                        }
                        else
                        {
                            strcpy(szStreamName, "An AVI stream");
                            strcpy(szMimeType, "video/x-pn-icm-plugin");
                        }
                    }

                    HeaderInfo hi;
                    hi.frameRect.left = 0;
                    hi.frameRect.top = 0;
                    hi.frameRect.right  = DwToNet(bi->ulWidth);
                    hi.frameRect.bottom = DwToNet(bi->ulHeight);
                    hi.ulSampleRate     = DwToNet(m_header.ulRate);
                    hi.ulSampleScale    = DwToNet(m_header.ulScale);

                    hi.bitmapInfo = *bi;

                    // Copy palette information:
                    if (bi->usBitCount <= 8)
                    {
                        memcpy(hi.bitmapInfo.rgbQuad, bi->rgbQuad, (bi->ulClrUsed)*sizeof(RGBQuad));
                    }

                    // Convert to net endian:
                    hi.bitmapInfo.ulSize            = DwToNet(hi.bitmapInfo.ulSize);
                    hi.bitmapInfo.ulWidth           = DwToNet(hi.bitmapInfo.ulWidth);
                    hi.bitmapInfo.ulHeight          = DwToNet(hi.bitmapInfo.ulHeight);
                    hi.bitmapInfo.usPlanes          =  WToNet(hi.bitmapInfo.usPlanes);
                    hi.bitmapInfo.usBitCount        =  WToNet(hi.bitmapInfo.usBitCount);
                    hi.bitmapInfo.ulCompression     = DwToNet(hi.bitmapInfo.ulCompression);
                    hi.bitmapInfo.ulSizeImage       = DwToNet(hi.bitmapInfo.ulSizeImage);
                    hi.bitmapInfo.ulXPelsPerMeter   = DwToNet(hi.bitmapInfo.ulXPelsPerMeter);
                    hi.bitmapInfo.ulYPelsPerMeter   = DwToNet(hi.bitmapInfo.ulYPelsPerMeter);
                    hi.bitmapInfo.ulClrUsed         = DwToNet(hi.bitmapInfo.ulClrUsed);
                    hi.bitmapInfo.ulClrImportant    = DwToNet(hi.bitmapInfo.ulClrImportant);

                    pOpaqueData->Set((UCHAR*)&hi, sizeof(hi));
                }

                m_fChunksPerSecond = (double) m_header.ulRate / m_header.ulScale;
                m_fSamplesPerSecond = m_fChunksPerSecond;

                INT32 ulMaxChunkSize = m_pIndex->GetMaxChunkSize(m_usStream);

                if (m_bLocalPlayback)
                {
                    if (ulMaxChunkSize)
                    {
                        ulMaxPacketSize = ulMaxChunkSize;
                    }

                    pHeader->SetPropertyULONG32("PacketsSegmented", 0);
                }
                else
                {
                    pHeader->SetPropertyULONG32("PacketsSegmented", 1);
                }

                pHeader->SetPropertyULONG32("SamplesPerSecond", (UINT32) m_fSamplesPerSecond);

                break;
            }
            case AVI_AUDS_TYPE:
            {
                // To do: set min packet size as a function of the block
                // alignment; support in packetizer

                INT32 ulMaxPacketSize = m_pIndex->GetMaxChunkSize(m_usStream);

                strcpy(szStreamName, "An audio stream");

                // Audio stream, do it just like the wvffplin.
                WaveInfo* pWaveInfo = (WaveInfo*) m_pFormat;

                const char* pMimeType = NULL;
                char    szMimeTypePCM[] = "audio/x-pn-wav";
                char    szMimeTypeACM[] = "audio/x-pn-windows-acm";

                if ( pWaveInfo->usFormatTag != WAVE_FORMAT_PCM )
                {
                    /////////////////////////////////////////////////////
                    // This wave file is not PCM and must be converted //
                    // by the ACM renderer                             //
                    /////////////////////////////////////////////////////
                    switch ( pWaveInfo->usFormatTag )
                    {
                        case WAVE_FORMAT_ALAW:
                            {
                                pMimeType = "audio/x-pn-alaw";
                                if ( pWaveInfo->usBitsPerSample == 8 &&
                                     pWaveInfo->ulSamplesPerSec == 8000 &&
                                     m_ePacketFormat == PFMT_RTP )
                                {
                                    nRTPPayloadType = RTP_PAYLOAD_PCMA;
                                    ulMaxPacketSize = 320;
                                }
                            }
                            break;

                        case WAVE_FORMAT_MULAW:
                            {
                                pMimeType = "audio/x-pn-mulaw";
                                if ( pWaveInfo->usBitsPerSample == 8 &&
                                     pWaveInfo->ulSamplesPerSec == 8000 &&
                                     m_ePacketFormat == PFMT_RTP )
                                {
                                    nRTPPayloadType = RTP_PAYLOAD_PCMU;
                                    ulMaxPacketSize = 320;
                                }
                            }
                            break;

                        case WAVE_FORMAT_G723_ADPCM:
                            {
                                pMimeType = "audio/x-pn-g723";
                                if ( m_ePacketFormat == PFMT_RTP )
                                {
                                    nRTPPayloadType = RTP_PAYLOAD_G723;
                                }
                            }
                            break;

                        case WAVE_FORMAT_G721_ADPCM:
                            {
                                pMimeType = "audio/x-pn-g721";
                                if ( m_ePacketFormat == PFMT_RTP )
                                {
                                    nRTPPayloadType = RTP_PAYLOAD_G721;
                                }
                            }
                            break;

                        case WAVE_FORMAT_GSM610:
                            {
                                pMimeType = "audio/x-pn-gsm610";
                                if ( m_ePacketFormat == PFMT_RTP )
                                {
                                    nRTPPayloadType = RTP_PAYLOAD_GSM;
                                }
                            }
                            break;
                        case WAVE_FORMAT_MPEGLAYER3:
                            {
                                pMimeType = "audio/rn-mpeg";
                                if ( m_ePacketFormat == PFMT_RTP )
                                {
                                    nRTPPayloadType = RTP_PAYLOAD_MPA;
                                }                                
                            }
                            break;
                        default:
                            {
                                pMimeType = szMimeTypeACM;
                            }
                            break;
                    }

                    strcpy(szMimeType, pMimeType);

                    AudioWAVEHEADER waveHeader;

                    waveHeader.usVersion        = 0;
                    waveHeader.usMagicNumberTag =  HOST_TO_LE16(AUDIO_WAVHEADER_MAGIC_NUMBER);
                    waveHeader.usFormatTag      =  HOST_TO_LE16(pWaveInfo->usFormatTag);
                    waveHeader.usChannels       =  HOST_TO_LE16(pWaveInfo->usChannels);
                    waveHeader.ulSamplesPerSec  =  HOST_TO_LE32(pWaveInfo->ulSamplesPerSec);
                    waveHeader.ulAvgBytesPerSec =  HOST_TO_LE32(pWaveInfo->ulAvgBytesPerSec);
                    waveHeader.usBlockAlign     =  HOST_TO_LE16(pWaveInfo->usBlockAlign);
                    waveHeader.usBitsPerSample  =  HOST_TO_LE16(pWaveInfo->usBitsPerSample);
                    waveHeader.usCbSize         =  HOST_TO_LE16(pWaveInfo->usSize);

                    pOpaqueData->SetSize(waveHeader.static_size() + pWaveInfo->usSize);

                    UINT32 len;
                    waveHeader.pack(pOpaqueData->GetBuffer(), len);

                    memcpy(pOpaqueData->GetBuffer() + waveHeader.static_size(),
                           m_pFormat + sizeof(WaveInfo), pWaveInfo->usSize);
                }
                else
                {
                    // Make sure we don't add opaque data
                    HX_RELEASE(pOpaqueData);

                    ///////////////////////////////
                    // This file is raw PCM //
                    ///////////////////////////////

                    // Set appropriate mime type
                    if ( pWaveInfo->usBitsPerSample == 8 )
                    {
                        strcpy(szMimeType, "audio/L8");
                    }
                    else if ( pWaveInfo->usBitsPerSample == 16 )
                    {
                        strcpy(szMimeType, "audio/L16");
                        m_bEndianSwap16= TRUE;
                    }
                    else
                    {
                        // Default mime type
                        pMimeType = szMimeTypePCM;
                    }

                    // offer an RTP payload type if compatible
                    if ( pWaveInfo->usBitsPerSample == 16 &&
                         pWaveInfo->ulSamplesPerSec == 44100 &&
                         m_ePacketFormat == PFMT_RTP )
                    {
                        if ( pWaveInfo->usChannels == 1 )
                        {
                            nRTPPayloadType = RTP_PAYLOAD_L16_1CH;
                        }
                        else if ( pWaveInfo->usChannels == 2 )
                        {
                            nRTPPayloadType = RTP_PAYLOAD_L16_2CH;
                        }
                    }
                }

                if ( m_ePacketFormat == PFMT_RTP )
                {
                    pHeader->SetPropertyULONG32("RTPPayloadType",
                                                (UINT32)nRTPPayloadType);
                }

                pHeader->SetPropertyULONG32("BitsPerSample", pWaveInfo->usBitsPerSample);
                //HX_TRACE("\t\tBitsPerSample:\t%lu\n", pWaveInfo->usBitsPerSample);

                pHeader->SetPropertyULONG32("SamplesPerSecond", pWaveInfo->ulSamplesPerSec);
                //HX_TRACE("\t\tSamplesPerSecond:\t%lu\n", pWaveInfo->ulSamplesPerSec);

                pHeader->SetPropertyULONG32("Channels", pWaveInfo->usChannels);
                //HX_TRACE("\t\tChannels:\t%lu\n", pWaveInfo->usChannels);

                m_fChunksPerSecond = m_pIndex->GetChunkTotal(m_usStream) / ((double) m_pIndex->GetByteTotal(m_usStream) / pWaveInfo->ulAvgBytesPerSec);
                m_fSamplesPerSecond = (double) m_header.ulRate / m_header.ulScale;
                break;
            }
            default:
                HX_RELEASE(pOpaqueData);
                HX_RELEASE(pSNBuf);
                HX_RELEASE(pMTBuf);
                return HXR_OUTOFMEMORY;
        }

        // Set opaque data:
        if ( pOpaqueData )
        {
            pHeader->SetPropertyBuffer ("OpaqueData", pOpaqueData);
        }

        // Stream name and MIME type:
        pSNBuf->Set((const BYTE*)szStreamName, strlen(szStreamName)+1);
        pHeader->SetPropertyCString("StreamName", pSNBuf);
        //HX_TRACE("\t\tStreamName:\t%s\n", szStreamName);

        pMTBuf->Set((const BYTE*)szMimeType, strlen(szMimeType)+1);
        pHeader->SetPropertyCString("MimeType", pMTBuf);
        //HX_TRACE("\t\tMimeType:\t%s\n", szMimeType);

        // Packet format:
        if ( m_ePacketFormat == PFMT_RTP )
        {
            pHeader->SetPropertyULONG32("RTPPayloadType", nRTPPayloadType);
            //HX_TRACE("\t\tRTPPayloadType:\t%lu\n", nRTPPayloadType);
        }

        // Max packet size:
        pHeader->SetPropertyULONG32("MaxPacketSize", ulMaxPacketSize);
        //HX_TRACE("\t\tMaxPacketSize:\t%lu\n", ulMaxPacketSize);

        // Average packet size:
        pHeader->SetPropertyULONG32("AvgPacketSize", ulMaxPacketSize);
        //HX_TRACE("\t\tAvgPacketSize:\t%lu\n", ulMaxPacketSize);
    }

    // Format specific data (for future use):
    // XXXKB: don't send it until we have a use for it

    // Stream number:
    pHeader->SetPropertyULONG32("StreamNumber", m_usStream);
    //HX_TRACE("\t\tStreamNumber:\t%lu\n", m_usStream);

//    HX_ASSERT(m_fChunksPerSecond && m_fSamplesPerSecond);

    // Start time:
    pHeader->SetPropertyULONG32("StartTime", (ULONG32) (m_header.ulStart * m_fChunksPerSecond * 1000));
    //HX_TRACE("\t\tStartTime:\t%lu\n", (ULONG32) (m_header.ulStart * m_fSamplesPerSecond * 1000));

    // Duration:
    pHeader->SetPropertyULONG32("Duration", (ULONG32) (((double) m_header.ulLength / m_fSamplesPerSecond) * 1000));
    //HX_TRACE("\t\tDuration:\t%lu\n", (ULONG32) (((double) m_header.ulLength / m_fSamplesPerSecond) * 1000));

    // Max bit rate:
    pHeader->SetPropertyULONG32("MaxBitRate", (ULONG32) (8 * m_pIndex->GetMaxChunkSize(m_usStream) * m_fChunksPerSecond));
    //HX_TRACE("\t\tMaxBitRate:\t%lu\n", (ULONG32) (8 * m_pIndex->GetMaxChunkSize(m_usStream) * m_fChunksPerSecond));

    // Averate bit rate:
    ULONG32 ulAverageBitrate = (ULONG32) (8 * m_pIndex->GetAverageChunkSize(m_usStream) * m_fChunksPerSecond);
    pHeader->SetPropertyULONG32("AvgBitRate", ulAverageBitrate);
    //HX_TRACE("\t\tAvgBitRate:\t%lu\n", ulAverageBitrate);

    // Preroll:
    UINT32 ulPreroll = (UINT32) (((double) m_pIndex->GetMaxByteDeflict(m_usStream) / ulAverageBitrate) * 1000);

    if (IsAudio())
    {
        if (m_bLocalPlayback)
        {
            ulPreroll = 1000;
        }
        else if (ulPreroll < MIN_AUDIO_PREROLL)
        {
            ulPreroll = MIN_AUDIO_PREROLL;
        }
        else if (ulPreroll > MAX_AUDIO_PREROLL)
        {
            ulPreroll = MAX_AUDIO_PREROLL;
        }
    }
    else
    {
        if (m_bLocalPlayback)
        {
            ulPreroll = 500;
        }
        else if (ulPreroll < MIN_VIDEO_PREROLL)
        {
            ulPreroll = MIN_VIDEO_PREROLL;
        }
        else if (ulPreroll > MAX_VIDEO_PREROLL)
        {
            ulPreroll = MAX_VIDEO_PREROLL;
        }
    }

    pHeader->SetPropertyULONG32("Preroll", ulPreroll);
    //HX_TRACE("\t\tPreroll:\t%lu\n", ulPreroll);

    // Set rule book:
    #if 0
    BOOL bGotRuleBook = SetRuleBook(m_ePacketFormat,
                                    pHeader, m_pCommonClassFactory,
                                    ulAvgBitRate, nRTPPayloadType);
    #endif 0

#ifdef USE_SIMPLE_SEGMENT_PAYLOAD

    // Load packetizer:
    m_pPayloadFormatter = new SimpleSegmentPayloadFormat();
    if (!m_pPayloadFormatter)
    {
        HX_RELEASE(pOpaqueData);
        HX_RELEASE(pSNBuf);
        HX_RELEASE(pMTBuf);
        return HXR_OUTOFMEMORY;
    }
    HX_ADDREF(m_pPayloadFormatter);
    m_pPayloadFormatter->Init(m_pContext, TRUE);
    m_pPayloadFormatter->SetStreamHeader(pHeader);

#endif

    switch ( m_header.ulType )
    {
        case AVI_VIDS_TYPE:
        case AVI_AUDS_TYPE:
            // All AVI audio and video types are supported
            break;

        default:
            // Unsupported file type
            result = HXR_DEC_INIT_FAILED;
            break;
    }

    HX_RELEASE(pOpaqueData);
    HX_RELEASE(pSNBuf);
    HX_RELEASE(pMTBuf);

    return result;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::IsAudio
//
BOOL CAVIStream::IsAudio()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::IsAudio()\n");
    return m_header.ulType == AVI_AUDS_TYPE;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::GetDuration
//
double CAVIStream::GetDuration()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::GetDuration()\n");
    HX_ASSERT(m_header.ulScale);
    return  m_header.ulLength / (m_header.ulRate / (double) m_header.ulScale);
}

// Packet handling methods:

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::InitForReading
//


HX_RESULT CAVIStream::InitForReading(IUnknown* pContext, IHXFileObject* pFile)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::InitForReading()\n");

    HX_ASSERT(m_state == ePreReader);

    m_pFile = pFile;
    HX_ASSERT_VALID_PTR(m_pFile);
    HX_ADDREF(m_pFile);

    HX_ASSERT_VALID_PTR(pContext);

    if (m_pFile && pContext)
    {
        m_pReader = new CRIFFReader(pContext, this, m_pFile);
        HX_ADDREF(m_pReader);
        m_pReader->Open("");
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::ReadInitialized
//
BOOL CAVIStream::ReadInitialized()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::ReadInitialized()\n");
    HX_ASSERT(m_pReader ? m_state > ePreReader : TRUE);
    return m_pReader != NULL;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::HasPackets
//
BOOL CAVIStream::HasPackets()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::HasPackets()\n");
    return (BOOL) m_pNextPacket;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::GetPendingPacketCount
//
UINT32 CAVIStream::GetPendingPacketCount()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::GetPendingPacketCount()\n");
    return m_ulPendingPacketRequests;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::IncrementPendingPacketCount
//
UINT32 CAVIStream::IncrementPendingPacketCount()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::IncrementPendingPacketCount()\n");
    m_ulPendingPacketRequests++;
    return m_ulPendingPacketRequests;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::ClearPendingPacketCount
//
void CAVIStream::ClearPendingPacketCount()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::ClearPendingPacketCount()\n");
    m_ulPendingPacketRequests = 0;
}

// Chunk handling methods:
/////////////////////////////////////////////////////////////////////////
//  CAVIStream::Seek
//
void CAVIStream::Seek(UINT32 ulTime)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::Seek()\ttime: %lu\n", ulTime);

    m_ulPendingPacketRequests = 0;
    m_bRead = FALSE;

    // Flush the packetizer; always resend the entire chunk even if the
    // seek chunk is current chunk and no packets have been sent
    HX_RELEASE(m_pNextPacket);

#ifdef USE_SIMPLE_SEGMENT_PAYLOAD
    m_pPayloadFormatter->Flush();
#endif

    UINT32 ulTargetChunk;
    UINT32 ulTargetKeyChunk;
    UINT32 ulTargetKeyChunkOffset;
    BOOL bTargetKeyChunkFound = TRUE;

    if (IsAudio ())
    {
        INT32 lLength;
        UINT32 ulTotalLength=0;
        UINT32 ulTotalTime=0;

        ulTargetKeyChunk = 0;

        WaveInfo* pWaveInfo = (WaveInfo*) m_pFormat;

        while (ulTotalTime < ulTime)
        {
            lLength = m_pIndex->GetChunkLength (m_usStream, ulTargetKeyChunk);

            if (lLength == -1)
            {
                bTargetKeyChunkFound = FALSE;
                break;
            }

            ulTotalLength += lLength;
            ulTotalTime = (UINT32) (((double)ulTotalLength / pWaveInfo->ulAvgBytesPerSec) * 1000);

            if (ulTotalTime < ulTime)
                ulTargetKeyChunk++;
        }
    }
    else
    {
        ulTargetChunk = (UINT32) (ulTime / 1000.0 * m_fChunksPerSecond);

        if (FAILED(m_pIndex->FindClosestKeyChunk(m_usStream, ulTargetChunk, ulTargetKeyChunkOffset, ulTargetKeyChunk)))
        {
            bTargetKeyChunkFound = FALSE;
        }
    }

    // Do we have the target keychunk   already?
    // TODO: Optimize for the case where we have a partial set of
    // chunks between the keychunk and the target chunk.
    for (; m_ulMinReadChunk < m_ulMaxReadChunk && (!bTargetKeyChunkFound || 
            (bTargetKeyChunkFound && m_ulMinReadChunk != ulTargetKeyChunk)); ++m_ulMinReadChunk)
    {
        HX_RELEASE(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer);
    }

    // If we have a correct video chunk in memory (a short seek, for example)
    // add it to the packetizer:
    if (m_ulMinReadChunk < m_ulMaxReadChunk)
    {
        HX_ASSERT_VALID_PTR(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer);
        HX_ASSERT(ulTargetKeyChunk == m_ulMinReadChunk);

        IHXPacket* pNewPacket;
        if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(IID_IHXPacket,
                                                            (void**) &pNewPacket)))
        {
            // HX_TRACE("CAVIFileFormat::CAVIStream::Seek()\tstream %d\tpacket time: %lu\t minreadchunk: %lu\n", m_usStream, ulTime, m_ulMinReadChunk);
            if (IsAudio ())
            {
                WaveInfo* pWaveInfo = (WaveInfo*) m_pFormat;
                ulTime = (UINT32) (((double)m_ulTotalBytesPacketised / pWaveInfo->ulAvgBytesPerSec) * 1000);
            }
            else
            {
                ulTime = (UINT32) ((m_ulMinReadChunk / m_fChunksPerSecond) * 1000);
            }

            // TODO: Set correct ASM rules
            if (m_bEndianSwap16)
            {
                // How inefficient:
                IHXBuffer* pSwappedBuffer;
                if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                                                    (void**) &pSwappedBuffer)))
                {
                    IHXBuffer* pUnswappedBuffer = m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer;
                    pSwappedBuffer->Set(pUnswappedBuffer->GetBuffer(), pUnswappedBuffer->GetSize());
                    SwapWordBytes((unsigned short*) pSwappedBuffer->GetBuffer(), pUnswappedBuffer->GetSize() / 2);
                    pNewPacket->Set(pSwappedBuffer, ulTime, m_usStream,
                                    HX_ASM_SWITCH_OFF, 0 && !m_pIndex->IsKeyChunk(m_usStream, m_ulMinReadChunk));
                    HX_RELEASE(pSwappedBuffer);
                }
            }
            else
            {
                pNewPacket->Set(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer, ulTime, m_usStream,
                                HX_ASM_SWITCH_OFF, 0 && !m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].bKeyChunk);
            }

#ifdef USE_SIMPLE_SEGMENT_PAYLOAD
            m_pPayloadFormatter->SetPacket(pNewPacket);
            HX_RELEASE(pNewPacket);
            HX_RELEASE(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer);
            ++m_ulMinReadChunk;

            if (FAILED(m_pPayloadFormatter->GetPacket(m_pNextPacket)))
            {
                m_pNextPacket = NULL;
            }
#else
            HX_RELEASE(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer);
            ++m_ulMinReadChunk;
            m_pNextPacket = pNewPacket;
#endif
        }
    }
    else
    {
        m_ulMinReadChunk = m_ulMaxReadChunk = m_ulChunkReadTarget = ulTargetKeyChunk;

        if (!bTargetKeyChunkFound)
        {
            m_bSeeking = TRUE;

            if (ulTime >= PRIOR_SEEK_TIME)
            {
                m_ulSeekTime = ulTime - PRIOR_SEEK_TIME;
            }
            else
            {
                m_ulSeekTime = 0;
            }
        }

        if (m_state != eReady)
        {
            m_bDiscardPendingIO = TRUE;
        }
        else
        {
            HX_ASSERT(!m_bDiscardPendingIO);
        }
    }

    m_ulTotalBytesPacketised = 0;

    for (UINT32 i=0; i<m_ulMinReadChunk; i++)
    {
        m_ulTotalBytesPacketised += m_pIndex->GetChunkLength (m_usStream, i);
    }

    m_ulTotalBytesRead = m_ulTotalBytesPacketised;

    return;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::GetNextPacket
//
//  On failure, returns NULL and increments the pending packet count;
//  on success, returns an AddRef'd packet
IHXPacket* CAVIStream::GetNextPacket()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::GetNextPacket()\n");
    HX_ASSERT(m_state > ePreReader);
    HX_ASSERT(m_ulPendingPacketRequests > 0);
    HX_ASSERT(m_pNextPacket);

    IHXPacket* pPacket = m_pNextPacket;
    m_pNextPacket = NULL;

    while (!m_pNextPacket && m_ulMinReadChunk < m_ulMaxReadChunk)
    {
        HX_ASSERT_VALID_PTR(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer);

        IHXPacket* pNewPacket;
        if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(IID_IHXPacket,
                                                            (void**) &pNewPacket)))
        {
            IHXBuffer* pBuffer = m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer;
            UINT32 bufferSize = pBuffer->GetSize();

            //HX_TRACE("CAVIFileFormat::CAVIStream::GetNextPacket()\tstream %d\tpacket time: %lu\t minreadchunk: %lu\n", m_usStream, ulTime, m_ulMinReadChunk);

            UINT32 ulTime;
            if (IsAudio ())
            {
                WaveInfo* pWaveInfo = (WaveInfo*) m_pFormat;
                ulTime = (UINT32) (((double)m_ulTotalBytesPacketised / pWaveInfo->ulAvgBytesPerSec) * 1000);
            }
            else
            {
                ulTime = (UINT32) ((m_ulMinReadChunk / m_fChunksPerSecond) * 1000);
            }

            // TODO: Set correct ASM rules
            if (m_bEndianSwap16)
            {
                // How inefficient:
                IHXBuffer* pSwappedBuffer;
                if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                                                    (void**) &pSwappedBuffer)))
                {
                    IHXBuffer* pUnswappedBuffer = m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer;
                    pSwappedBuffer->Set(pUnswappedBuffer->GetBuffer(), pUnswappedBuffer->GetSize());
                    SwapWordBytes((unsigned short*) pSwappedBuffer->GetBuffer(), pUnswappedBuffer->GetSize() / 2);
                    pNewPacket->Set(pSwappedBuffer, ulTime, m_usStream,
                                    HX_ASM_SWITCH_OFF, 0 && !m_pIndex->IsKeyChunk(m_usStream, m_ulMinReadChunk));
                    HX_RELEASE(pSwappedBuffer);
                }
            }
            else
            {
                pNewPacket->Set(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer, ulTime, m_usStream,
                                HX_ASM_SWITCH_OFF, 0 && !m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].bKeyChunk);
            }


#ifdef USE_SIMPLE_SEGMENT_PAYLOAD
            m_pPayloadFormatter->SetPacket(pNewPacket);
            HX_RELEASE(pNewPacket);
            HX_RELEASE(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer);
            ++m_ulMinReadChunk;
            m_pPayloadFormatter->GetPacket(m_pNextPacket);
#else
            HX_RELEASE(m_chunkPrefetchArray[m_ulMinReadChunk % MAX_CHUNK_PREFETCH].pBuffer);
            ++m_ulMinReadChunk;
            m_pNextPacket = pNewPacket;
#endif

            m_ulTotalBytesPacketised += bufferSize;

            if (!pPacket && m_pNextPacket)
            {
                IHXPacket* pPacket = m_pNextPacket;
                m_pNextPacket = NULL;
            }
        }
    }

    HX_ASSERT(pPacket);
    if (pPacket)
    {
        --m_ulPendingPacketRequests;
    }

    return pPacket;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::GetNextPacketTime
//
UINT32 CAVIStream::PeekPacketTime()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::GetNextPacketTime()\n");
    if (m_pNextPacket)
    {
        return m_pNextPacket->GetTime();
    }

    UINT32 ulTime;

    if (IsAudio ())
    {
        WaveInfo* pWaveInfo = (WaveInfo*) m_pFormat;
        ulTime = (UINT32) (((double)m_ulTotalBytesPacketised / pWaveInfo->ulAvgBytesPerSec) * 1000);
    }
    else
    {
        ulTime = (UINT32) ((m_ulMinReadChunk / m_fChunksPerSecond) * 1000);
    }

    return ulTime;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::GetNextSlice
//
void CAVIStream::GetNextSlice()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::GetNextSlice()\n");

    UINT32 ulNextChunkOffset;
    HX_RESULT indexResult;

    indexResult = m_pIndex->FindClosestChunk(m_usStream, m_ulMaxReadChunk,
                                                       ulNextChunkOffset,
                                                       m_ulChunkReadTarget);
//  HX_ASSERT(m_ulMaxReadChunk == m_ulChunkReadTarget); // for indexed clips only

    if(indexResult == HXR_OK || indexResult == HXR_NOT_INDEXABLE)
    {
        // Try to get chunk even if this is not indexed.
        if (ulNextChunkOffset != m_pReader->GetOffset())
        {
            m_state = eChunkSeek;
            m_pReader->FileSeek(ulNextChunkOffset);
        }
        else
        {
            m_state = eChunkFind;
            m_pReader->FindNextChunk();
        }
    }
    else
    {
        if (indexResult == HXR_CHUNK_MISSING)
        {
            // we've reached EOF
            m_bRead = TRUE;
            m_pOuter->IOEvent();
        }
    }
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::CanPrefetchSlice
//
BOOL CAVIStream::CanPrefetchSlice()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::CanPrefetchSlice()\n");
    return (m_ulMaxReadChunk - m_ulMinReadChunk < MAX_CHUNK_PREFETCH) && !m_bRead;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::PeekPrefetchSliceTime
//
UINT32 CAVIStream::PeekPrefetchTime()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::PeekPrefetchSliceTime()\n");
    UINT32 ulTime;

    if (IsAudio ())
    {
        WaveInfo* pWaveInfo = (WaveInfo*) m_pFormat;
        ulTime = (UINT32) (((double)m_ulTotalBytesRead / pWaveInfo->ulAvgBytesPerSec) * 1000);
    }
    else
    {
        ulTime = (UINT32) ((m_ulMaxReadChunk / m_fChunksPerSecond) * 1000);
    }

    return ulTime;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIStream::AtEndOfStream
//
BOOL CAVIStream::AtEndOfStream()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::AtEndOfStream()\n");
    //HX_ASSERT( (m_state != eReady) || (m_pNextPacket && m_bRead)); // Currently we don't prime on init
    return m_bRead;
}

void CAVIStream::DiscardPendingIO()
{
    m_bDiscardPendingIO = TRUE;
}

STDMETHODIMP CAVIStream::QueryInterface(REFIID riid, void** ppvObj)
{
//    return m_pOuter->QueryInterface(riid, ppvObj);

	HX_RESULT retVal = HXR_NOINTERFACE;
	*ppvObj = NULL;

	if ( IsEqualIID(riid, IID_IUnknown) )
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
	else if(m_pOuter)
	{
		retVal = m_pOuter->QueryInterface(riid, ppvObj);
	}

	return retVal;
}



/////////////////////////////////////////////////////////////////////////
//  CAVIStream::AddRef
//
STDMETHODIMP_(ULONG32) CAVIStream::AddRef()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::AddRef()\n");
    // return m_pOuter->AddRef();

	return InterlockedIncrement(&m_lRefCount);
}


/////////////////////////////////////////////////////////////////////////
//  CAVIStream::Release
//
STDMETHODIMP_(ULONG32) CAVIStream::Release()
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::Release()\n");
    // return m_pOuter->Release();

    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;

}

// CRiffResponse
STDMETHODIMP CAVIStream::RIFFOpenDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::RIFFOpenDone()\n");
    HX_ASSERT(m_state == ePreReader);
    m_state = eReady;

    m_pOuter->IOEvent();

    return HXR_OK;
}

STDMETHODIMP CAVIStream::RIFFCloseDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::RIFFCloseDone()\n");
    return HXR_OK;
}

/* RIFFFindChunkDone is called when a FindChunk completes.
 * len is the length of the data in the chunk (i.e. Read(len)
 * would read the whole chunk)
 */

STDMETHODIMP CAVIStream::RIFFFindChunkDone(HX_RESULT status, UINT32 len)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::RIFFFindChunkDone()\n");

    HX_ASSERT(m_state == eChunkFind);
    HX_ASSERT(SUCCEEDED(status));

    if (m_bDiscardPendingIO)
    {
        // We've seeked or the stream has ended since the I/O request;
        // let the the outer object reschedule disk reads.
        m_bDiscardPendingIO = FALSE;
        m_state = eReady;
        m_pOuter->IOEvent();
        return HXR_OK;
    }

    if (FAILED(status))
    {
        m_bRead = TRUE;
        m_state = eReady;
        m_pIndex->FileRead(TRUE);
        m_pOuter->IOEvent();
        return HXR_OK;
    }

    UINT32 ulChunkType = m_pReader->GetChunkType();

    // If this is not an audio/video chunk for this stream then
    // Seek to the end of this chunk and then search if next chunk is for this stream
    // Read if a chunk for this stream is found, otherwise continue till the end of the
    // file. In case there is no chunk for this stream till the end then close this stream
    // There is no point in worrying abt adding chunk for the other streams to the Index table
    // As each stream has its own reader and pointer to IndexTable, Adding corresponding chunks
    // is responsibility of the individual streams.

    BOOL chunkTypeAV = CAVIFileFormat::IsAVChunk(ENDIAN_SWAP_32(ulChunkType));
    UINT16 usStream = CAVIFileFormat::GetStream(ENDIAN_SWAP_32(ulChunkType));

    if (chunkTypeAV && (usStream == m_usStream))
    {
        m_pIndex->AddToIndex(usStream, m_ulChunkReadTarget, m_pReader->GetOffset() - 8, len);

        if (m_ulChunkReadTarget == m_ulMaxReadChunk)
        {
            //HX_TRACE("\tstream: %d\toffset: %lu\n", m_usStream, m_pReader->GetOffset());
            if (m_bSeeking)
            {
                // Find the time of the next packet.
                UINT32 ulNextPacketTime;
                UINT32 ulChunkSize = m_pReader->GetChunkRawSize ();

                if (IsAudio ())
                {
                    WaveInfo* pWaveInfo = (WaveInfo*) m_pFormat;
                    ulNextPacketTime = (UINT32) (((double)(m_ulTotalBytesRead + ulChunkSize) / pWaveInfo->ulAvgBytesPerSec) * 1000);
                }
                else
                {
                    ulNextPacketTime = (UINT32) (((m_ulMaxReadChunk + 1)/ m_fChunksPerSecond) * 1000);
                }

                if (ulNextPacketTime < m_ulSeekTime)
                {
                    // Discard the packet.
                    m_ulChunkReadTarget++;
                    m_ulMaxReadChunk = m_ulMinReadChunk = m_ulChunkReadTarget;
                    m_ulTotalBytesPacketised += ulChunkSize;
                    m_ulTotalBytesRead += ulChunkSize;
                    m_state = eChunkSeek;
                    m_pReader->FileSeek(m_pReader->GetOffset () + len);
                    return HXR_OK;
                }
                else
                {
                    // Do not discard the packet. This is the first packet to be passed.
                    m_bSeeking = FALSE;
                }
            }

            if (len > 0)
            {
                m_state = eChunkRead;
                m_pReader->Read(len);
            }
            else
            {
                m_state = eChunkFind;
                m_pReader->FindNextChunk();
            }

            return HXR_OK;
        }
        else
        {
            HX_ASSERT(FALSE);
            ++m_ulChunkReadTarget;
            return HXR_OK;
        }
    }
    else
    {
        m_state = eChunkSeek;
        m_pReader->FileSeek(m_pReader->GetOffset() + len);
        return HXR_OK;
    }
}

/* Called after a Descend completes */
STDMETHODIMP CAVIStream::RIFFDescendDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::RIFFDescendDone()\n");
    HX_ASSERT(FALSE);   // we don't support multiple MOVI chunks yet
    return HXR_NOTIMPL;
}

/* Called after an Ascend completes */
STDMETHODIMP CAVIStream::RIFFAscendDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::RIFFAscendDone()\n");
    HX_ASSERT(FALSE);   // we don't support multiple MOVI chunks yet
    return HXR_NOTIMPL;
}

/* Called when a read completes.  IHXBuffer contains the data read
 */
STDMETHODIMP CAVIStream::RIFFReadDone(HX_RESULT status, IHXBuffer *pBuffer)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::RIFFReadDone()\n");
    HX_ASSERT(m_state == eChunkRead);
    HX_ASSERT(CAVIFileFormat::GetStream(ENDIAN_SWAP_32(m_pReader->GetChunkType())) == m_usStream);
    HX_ASSERT(m_ulChunkReadTarget == m_ulMaxReadChunk);
    HX_ASSERT(SUCCEEDED(status));

    if (m_bDiscardPendingIO)
    {
        // We've seeked or the stream has ended since the I/O request;
        // let the the outer object reschedule disk reads.
        m_bDiscardPendingIO = FALSE;
        m_state = eReady;
        m_pOuter->IOEvent();
        return HXR_OK;
    }

    pBuffer->SetSize (m_pReader->GetChunkRawSize ());
    UINT32 bufferSize = pBuffer->GetSize();

    if (!m_pNextPacket)
    {
        IHXPacket* pNewPacket;
        if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(IID_IHXPacket,
                                                            (void**) &pNewPacket)))
        {
            // Find the time of the current packet.
            UINT32 ulTime;
            if (IsAudio ())
            {
                WaveInfo* pWaveInfo = (WaveInfo*) m_pFormat;
                ulTime = (UINT32) (((double)m_ulTotalBytesRead / pWaveInfo->ulAvgBytesPerSec) * 1000);
            }
            else
            {
                ulTime = (UINT32) ((m_ulMaxReadChunk / m_fChunksPerSecond) * 1000);
            }

            // TODO: Set correct ASM rules
            if (m_bEndianSwap16)
            {
                // How inefficient:
                IHXBuffer* pSwappedBuffer;
                if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(IID_IHXBuffer,
                                                                    (void**) &pSwappedBuffer)))
                {
                    IHXBuffer* pUnswappedBuffer = pBuffer;
                    pSwappedBuffer->Set(pUnswappedBuffer->GetBuffer(), pUnswappedBuffer->GetSize());
                    SwapWordBytes((unsigned short*) pSwappedBuffer->GetBuffer(), pUnswappedBuffer->GetSize() / 2);
                    pNewPacket->Set(pSwappedBuffer, ulTime, m_usStream,
                                    HX_ASM_SWITCH_OFF, 0 && !m_pIndex->IsKeyChunk(m_usStream, m_ulMinReadChunk));
                    HX_RELEASE(pSwappedBuffer);
                }
            }
            else
            {
                pNewPacket->Set(pBuffer, ulTime, m_usStream,
                                HX_ASM_SWITCH_OFF, 0 && !m_pIndex->IsKeyChunk(m_usStream, m_ulMinReadChunk));
            }

#ifdef USE_SIMPLE_SEGMENT_PAYLOAD

            m_pPayloadFormatter->SetPacket(pNewPacket);
            HX_RELEASE(pNewPacket);

            if (FAILED(m_pPayloadFormatter->GetPacket(m_pNextPacket)))
            {
                m_pNextPacket = NULL;
            }

#else
            m_pNextPacket = pNewPacket;

#endif
            ++m_ulMinReadChunk;

            m_ulTotalBytesPacketised += bufferSize;
        }
    }
    else
    {
        HX_ASSERT(!m_chunkPrefetchArray[m_ulMaxReadChunk % MAX_CHUNK_PREFETCH].pBuffer);
        HX_ADDREF(pBuffer);
        m_chunkPrefetchArray[m_ulMaxReadChunk % MAX_CHUNK_PREFETCH].pBuffer = pBuffer;
        m_chunkPrefetchArray[m_ulMaxReadChunk % MAX_CHUNK_PREFETCH].bKeyChunk = m_pIndex->IsKeyChunk(m_usStream, m_ulMinReadChunk);
    }

    ++m_ulMaxReadChunk;

    m_ulTotalBytesRead += bufferSize;

    m_pOuter->IOEvent();

    return HXR_OK;
}

/* Called when a seek completes */
STDMETHODIMP CAVIStream::RIFFSeekDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::RIFFSeekDone()\n");
    HX_ASSERT(m_state == eChunkSeek);
    HX_ASSERT(SUCCEEDED(status));

    if (m_bDiscardPendingIO)
    {
        // We've seeked since the I/O request; let the the outer object
        // reschedule disk reads.
        m_bDiscardPendingIO = FALSE;
        m_state = eReady;
        m_pOuter->IOEvent();
        return HXR_OK;
    }

    // Call a chunk find to force the reader to scan the current chunk:
    m_state = eChunkFind;
    m_pReader->FindNextChunk();

    return HXR_OK;
}

/* Called with the data from a GetChunk request */
STDMETHODIMP CAVIStream::RIFFGetChunkDone(HX_RESULT status, UINT32 chunkType,
                                         IHXBuffer* pBuffer)
{
//HX_TRACE("CAVIFileFormat::CAVIStream::RIFFGetChunkDone()\n");
//    HX_ASSERT(!"RIFFGetChunkDone is not used");
//    return HXR_NOTIMPL;

// Control will reach here only if there is an error in reading the chunk
// No further read is possible for this stream

//    if(status == HXR_FAIL && chunkType == 0 && pBuffer == NULL)
    {
        m_bRead = TRUE;
    }
    m_pOuter->IOEvent();
    return HXR_OK;
}

HXBOOL
CAVIStream::SetRuleBook(PacketFormat packetFormat, IHXValues* pHeader,
                            IHXCommonClassFactory* pCommonClassFactory,
                            ULONG32 ulAvgBitRate, INT32 lRTPPayloadType)
{
    //HX_TRACE("CAVIFileFormat::CAVIStream::SetRuleBook()\n");
    if ( !pHeader || !pCommonClassFactory )
    {
        return FALSE;
    }
    ///XXXEH- Do we really only want to do this for RTP?  Find out who wrote
    // the original "if(packetFormat == PFMT_RTP)" code (in any version of
    // this file prior to 4/22/1998) and ask them why...
    if ( packetFormat == PFMT_RTP )
    {
        // set ASM rules for marker bits
        IHXBuffer* pASMRuleBook = NULL;
        pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                            (void**)&pASMRuleBook);
        char pRuleBook[256];
        sprintf(pRuleBook, "marker=0,averagebandwidth=%d; "
                "marker=1,averagebandwidth=0;",
                ulAvgBitRate);
        pASMRuleBook->Set((BYTE*)pRuleBook, strlen(pRuleBook)+1);
        pHeader->SetPropertyCString("ASMRuleBook", pASMRuleBook);
        pASMRuleBook->Release();

        pHeader->SetPropertyULONG32("RTPPayloadType",
                                    (UINT32)lRTPPayloadType);

        return TRUE;
    }
    return FALSE;
}

UINT32 CAVIStream::GetMaxChunkRead()
{
    return m_ulMaxReadChunk;
}

void CAVIStream::CloseStream()
{
    m_bRead = TRUE;
}

void CAVIStream::Close()
{
    if(m_pFile)
    {
        m_pFile->Close();
        HX_RELEASE(m_pFile);
    }

    //HX_TRACE("CAVIFileFormat::CAVIStream::~CAVIStream()\n");
    HX_RELEASE(m_pReader);
    HX_VECTOR_DELETE(m_pFormat);
    HX_RELEASE(m_pPayloadFormatter);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pOuter);
}
