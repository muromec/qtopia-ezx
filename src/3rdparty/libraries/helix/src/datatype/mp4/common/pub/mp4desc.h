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

#ifndef _MP4DESC_H
#define _MP4DESC_H

/****************************************************************************
 *  Defines
 */
// MPEG4 Object Type Indication Values
#define MP4OBJ_FORBIDDEN			0x00
#define MP4OBJ_SYSTEMS_ISO_IEC_14496_1_A	0x01
#define MP4OBJ_SYSTEMS_ISO_IEC_14496_1_B	0x02
#define MP4OBJ_VISUAL_ISO_IEC_14496_2_C		0x20
#define MP4OBJ_AUDIO_ISO_IEC_14496_3_D		0x40
#define MP4OBJ_VISUAL_ISO_IEC_13818_2_Simple	0x60
#define MP4OBJ_VISUAL_ISO_IEC_13818_2_Main	0x61
#define MP4OBJ_VISUAL_ISO_IEC_13818_2_SNR	0x62
#define MP4OBJ_VISUAL_ISO_IEC_13818_2_Spatial	0x63
#define MP4OBJ_VISUAL_ISO_IEC_13818_2_High	0x64
#define MP4OBJ_VISUAL_ISO_IEC_13818_2_422	0x65
#define MP4OBJ_AUDIO_ISO_IEC_13818_7_Main	0x66
#define MP4OBJ_AUDIO_ISO_IEC_13818_7_LC		0x67
#define MP4OBJ_AUDIO_ISO_IEC_13818_7_SSR	0x68
#define MP4OBJ_AUDIO_ISO_IEC_13818_3		0x69
#define MP4OBJ_VISUAL_ISO_IEC_11172_2		0x6A
#define MP4OBJ_VISUAL_ISO_IEC_11172_3		0x6B
#define MP4OBJ_VISUAL_ISO_IEC_10918_1		0x6C

// MPEG4 Stream Type Values
#define MP4STRM_FORBIDDEN			0x00
#define MP4STRM_OBJECT_DESCRIPTOR		0x01
#define MP4STRM_CLOCK_REFERENCE			0x02
#define MP4STRM_SCENE_DESCRIPTION		0x03
#define MP4STRM_VISUAL				0x04
#define MP4STRM_AUDIO				0x05
#define MP4STRM_MPEG7				0x06
#define MP4STRM_IPMP				0x07
#define MP4STRM_OBJECT_CONTENT_INFO		0x08
#define MP4STRM_MPEGJ				0x09


/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxtypes.h"
#include "hxslist.h"
#include "ihxpckts.h"
#include "bitstuff.h"


/****************************************************************************
 *  Globals
 */


/****************************************************************************
 *  Descriptor Class Definitions
 */
/****************************************************************************
 *  Base Descriptor
 */
class MP4BaseDescriptor
{
public:
    typedef enum
    {
	DTYP_UNKNOWN,
	DTYP_ESTREAM,
	DTYP_EXTENSION,
	DTYP_LANGUAGE,
	DTYP_DECODER_CONFIG,
	DTYP_DECODER_INFO_SHORT,
	DTYP_SL_CONFIG,
	DTYP_IPI_PTR,
	DTYP_ID_DATASET,
	DTYP_QOS
    } DescType;

    typedef enum
    {
	SIZE_HEADER_INCLUSIVE	    = 0x01
    } Flag;
    
    virtual HX_RESULT Unpack(UINT8* &pData, 
			     ULONG32 &ulSize, 
			     ULONG32 ulFlags = 0) = 0;

    static DescType GetDescType(UINT8 uTag);
    
    UINT8 m_uTag;

    ULONG32 m_ulFlags;
    
protected:
    MP4BaseDescriptor(UINT8 uTag = 0) 
	: m_uTag(uTag)
	, m_ulFlags(0)
    {
	;
    }
    
    ~MP4BaseDescriptor()
    {
	;
    }

    static UINT8 GetTag(UINT8* &pData, 
			ULONG32 &ulDataSize, 
			HXBOOL bAdvance = FALSE);

    static ULONG32 GetSize(UINT8* &pData, 
			   ULONG32 &ulSize, 
			   HXBOOL bAdvance = FALSE);
};


/****************************************************************************
 *  ExtensionDescriptorArray
 */
class ExtensionDescriptorArray : public MP4BaseDescriptor
{
public:
    UINT8	uEntryCount;
    ULONG32	ulLength;
    UINT8*	pData;
};


/****************************************************************************
 *  LanguageDescriptor
 */
class LanguageDescriptor : public MP4BaseDescriptor
{
public:
    UINT8	uLength;
    ULONG32	ulCode;
};


class IPI_DescPointer
{
public:
    UINT8	uLength;
    UINT16	uIPI_ES_ID;
};


class IP_IdentificationDataSet
{
public:
    UINT8	uLength;
    UINT8*	pData;
};


class QoS_Descriptor
{
public:
    UINT8	uLength;
    UINT8*	pData;
};

/****************************************************************************
 *  DecoderSpecifcInfo
 */
class DecoderSpecifcInfo : public MP4BaseDescriptor
{
public:
    DecoderSpecifcInfo(void);
    
    ~DecoderSpecifcInfo();
    
    HX_RESULT Unpack(UINT8* &pData, 
		     ULONG32 &ulSize, 
		     ULONG32 ulFlags = 0);
    
    ULONG32	m_ulLength;
    UINT8*	m_pData;
};

class DecoderConfigDescriptor : public MP4BaseDescriptor
{
public:
    DecoderConfigDescriptor(void);
    
    ~DecoderConfigDescriptor();
    
    HX_RESULT Unpack(UINT8* &pData, 
		     ULONG32 &ulSize,
		     ULONG32 ulFlags = 0);
    
    UINT8	m_uLength;
    UINT8	m_uObjectProfileIndication;
    UINT8	m_uStreamType;
    HXBOOL	m_bUpStream;
    HXBOOL	m_bReservedBit;
    ULONG32	m_ulBufferSizeDB;
    ULONG32	m_ulMaxBitrate;
    ULONG32	m_ulAvgBitrate;
    DecoderSpecifcInfo* m_pDecSpecificInfo;	// MBO: Should support array later
};

/****************************************************************************
 *  SLConfigPredefined
 */
class SLConfigPredefined
{
public:
    SLConfigPredefined(void)
    {
	;
    }
    
    ~SLConfigPredefined()
    {
	;
    }
    
    HX_RESULT Unpack(UINT8* &pData, 
		     ULONG32 &ulSize,
		     ULONG32 ulFlags = 0);
    
    HXBOOL	m_bUseAccessUnitStartFlag;
    HXBOOL	m_bUseAccessUnitEndFlag;
    HXBOOL	m_bUseRandomAccessPointFlag;
    HXBOOL	m_bUsePaddingFlag;
    HXBOOL	m_bUseTimeStampsFlag;
    HXBOOL	m_bUseWallClockTimeStampFlag;
    HXBOOL	m_bUseIdleFlag;
    HXBOOL	m_bDurationFlag;
    ULONG32	m_ulTimeStampResolution;
    ULONG32	m_ulOCRResolution;
    UINT8	m_uTimeStampLength;	// must be less than 64
    UINT8	m_uOCRLength;		// must be less than 64
    UINT8	m_AU_Length;		// must be less than 32
    UINT8	m_uInstantBitrateLength;
    UINT8	m_uDegradationPriorityLength;
    UINT8	m_uSeqNumLength;
    ULONG32	m_ulTimeScale;
    UINT16	m_uAccessUnitDuration;
    UINT16	m_uCompositionUnitDuration;
    double	m_dWallClockTimeStamp;
    UINT64	m_ullStartDecodingTimeStamp;
    UINT64	m_ullStartCompositionTimeStamp;
};

class SLConfigDescriptor : public MP4BaseDescriptor
{
public:
    SLConfigDescriptor(void);
    
    ~SLConfigDescriptor();
    
    HX_RESULT Unpack(UINT8* &pData, 
		     ULONG32 &ulSize,
		     ULONG32 ulFlags = 0);
    
    UINT8   m_uLength;
    UINT8   m_uPredefined;
    SLConfigPredefined*  m_pPredefined;
    HXBOOL    m_bOCRstreamFlag;
    UINT8   m_uReserved;
    UINT16  m_OCR_ES_Id;
};

/****************************************************************************
 *  ES_Descriptor
 */
class ES_Descriptor : public MP4BaseDescriptor
{
public:
    ES_Descriptor(void);
    
    ~ES_Descriptor();
    
    HX_RESULT Unpack(UINT8* &pData, 
		     ULONG32 &ulSize,
		     ULONG32 ulFlags = 0);
    
    UINT16	m_uLength;
    UINT16	m_uESid;
    HXBOOL	m_bStreamDependenceFlag;
    HXBOOL	m_bURLflag;
    HXBOOL	m_bOCRStreamFlag;
    UINT8	m_uStreamPriority;
    UINT16	m_uDependsOn_ES_ID;
    UINT8*	m_pURLString;
    UINT16	m_uOCR_ES_ID;
    ExtensionDescriptorArray*	m_pExtDescrArray;
    LanguageDescriptor*		m_pLangDescr;
    DecoderConfigDescriptor*	m_pDecConfigDescr;
    SLConfigDescriptor*		m_pSLConfigDescr;
    IPI_DescPointer*		m_pIPIPtr;
    IP_IdentificationDataSet*	m_pIPIDS;
    QoS_Descriptor*		m_pQOSDescr;
};

#endif	// _MP4DESC_H
