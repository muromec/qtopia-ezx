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

#ifndef _QTATOMS_H_
#define _QTATOMS_H_

/****************************************************************************
 *  Defines
 */
#ifdef QTCONFIG_SPEED_OVER_SIZE
#define QTATOMS_INLINE inline
#else	// QTCONFIG_SPEED_OVER_SIZE
#define QTATOMS_INLINE /**/
#endif	// QTCONFIG_SPEED_OVER_SIZE

/****************************************************************************
 *  Includes
 */
#include "qtbatom.h"
#include "mempager.h"
#include "proptools.h"

#define QT_BAD_SAMPLE_SIZE	0xFFFFFFFF

// /26245-013R6.doc says the unity matrix in the track header is:
// { 0x00010000,0,0,0,0x00010000,0,tx,ty,0x40000000 }, so (tx,ty) is here:
#define TKHD_MATRIX_TX_LOCATION (sizeof(UINT32) * 6)
#define TKHD_MATRIX_TY_LOCATION (sizeof(UINT32) * 7)

#if defined(_SYMBIAN) && !defined(__WINS__)
#  define PACKING __attribute__ ((packed))
#else
#  define PACKING
#endif
/****************************************************************************
 *  Atom Type definitions
 */
typedef enum
{
    QT_HXROOT = 0,
    QT_mdat = QT_ENCODE_TYPE('m', 'd', 'a', 't'),
    QT_moov = QT_ENCODE_TYPE('m', 'o', 'o', 'v'),
    QT_mvhd = QT_ENCODE_TYPE('m', 'v', 'h', 'd'),
    QT_trak = QT_ENCODE_TYPE('t', 'r', 'a', 'k'),
    QT_tkhd = QT_ENCODE_TYPE('t', 'k', 'h', 'd'),
    QT_mdia = QT_ENCODE_TYPE('m', 'd', 'i', 'a'),
    QT_mdhd = QT_ENCODE_TYPE('m', 'd', 'h', 'd'),
    QT_hdlr = QT_ENCODE_TYPE('h', 'd', 'l', 'r'),
    QT_minf = QT_ENCODE_TYPE('m', 'i', 'n', 'f'),
    QT_vmhd = QT_ENCODE_TYPE('v', 'm', 'h', 'd'),
    QT_dinf = QT_ENCODE_TYPE('d', 'i', 'n', 'f'),
    QT_dref = QT_ENCODE_TYPE('d', 'r', 'e', 'f'),
    QT_stbl = QT_ENCODE_TYPE('s', 't', 'b', 'l'),
    QT_stts = QT_ENCODE_TYPE('s', 't', 't', 's'),
    QT_ctts = QT_ENCODE_TYPE('c', 't', 't', 's'),
    QT_stss = QT_ENCODE_TYPE('s', 't', 's', 's'),
    QT_stsd = QT_ENCODE_TYPE('s', 't', 's', 'd'),
    QT_stsz = QT_ENCODE_TYPE('s', 't', 's', 'z'),
    QT_stz2 = QT_ENCODE_TYPE('s', 't', 'z', '2'),
    QT_stsc = QT_ENCODE_TYPE('s', 't', 's', 'c'),
    QT_stco = QT_ENCODE_TYPE('s', 't', 'c', 'o'),
    QT_co64 = QT_ENCODE_TYPE('c', 'o', '6', '4'),
    QT_smhd = QT_ENCODE_TYPE('s', 'm', 'h', 'd'),
    QT_hint = QT_ENCODE_TYPE('h', 'i', 'n', 't'),
    QT_mhlr = QT_ENCODE_TYPE('m', 'h', 'l', 'r'),
    QT_alis = QT_ENCODE_TYPE('a', 'l', 'i', 's'),
    QT_rsrc = QT_ENCODE_TYPE('r', 's', 'r', 'c'),
    QT_tsel = QT_ENCODE_TYPE('t', 's', 'e', 'l'),
    QT_hinf = QT_ENCODE_TYPE('h', 'i', 'n', 'f'),
    QT_hnti = QT_ENCODE_TYPE('h', 'n', 't', 'i'),
    QT_sdp  = QT_ENCODE_TYPE('s', 'd', 'p', ' '),
    QT_trpy = QT_ENCODE_TYPE('t', 'r', 'p', 'y'),
    QT_nump = QT_ENCODE_TYPE('n', 'u', 'm', 'p'),
    QT_tpyl = QT_ENCODE_TYPE('t', 'p', 'y', 'l'),
    QT_payt = QT_ENCODE_TYPE('p', 'a', 'y', 't'),
    QT_udta = QT_ENCODE_TYPE('u', 'd', 't', 'a'),
    QT_name = QT_ENCODE_TYPE('n', 'a', 'm', 'e'),
#if defined(HELIX_FEATURE_3GPP_METAINFO) || defined(HELIX_FEATURE_SERVER)
    // /-->> 3GPP Asset Info Data box types:
    QT_titl = QT_ENCODE_TYPE('t', 'i', 't', 'l'),
    QT_auth = QT_ENCODE_TYPE('a', 'u', 't', 'h'),
    QT_cprt = QT_ENCODE_TYPE('c', 'p', 'r', 't'),
#ifdef HELIX_FEATURE_3GPP_METAINFO
    QT_dscp = QT_ENCODE_TYPE('d', 's', 'c', 'p'),
    QT_perf = QT_ENCODE_TYPE('p', 'e', 'r', 'f'),
    QT_gnre = QT_ENCODE_TYPE('g', 'n', 'r', 'e'),
    QT_rtng = QT_ENCODE_TYPE('r', 't', 'n', 'g'),
    QT_clsf = QT_ENCODE_TYPE('c', 'l', 's', 'f'),
    QT_kywd = QT_ENCODE_TYPE('k', 'y', 'w', 'd'),
    QT_loci = QT_ENCODE_TYPE('l', 'o', 'c', 'i'),
    QT_albm = QT_ENCODE_TYPE('a', 'l', 'b', 'm'),
    QT_yrrc = QT_ENCODE_TYPE('y', 'r', 'r', 'c'),
    // /<<-- end Asset Info Data box types
#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER
    QT_edts = QT_ENCODE_TYPE('e', 'd', 't', 's'),
    QT_elst = QT_ENCODE_TYPE('e', 'l', 's', 't'),
    QT_tref = QT_ENCODE_TYPE('t', 'r', 'e', 'f'),
    QT_tims = QT_ENCODE_TYPE('t', 'i', 'm', 's'),
    QT_tsro = QT_ENCODE_TYPE('t', 's', 'r', 'o'),
    QT_twos = QT_ENCODE_TYPE('t', 'w', 'o', 's'),    
    QT_sowt = QT_ENCODE_TYPE('s', 'o', 'w', 't'),    
    QT_snro = QT_ENCODE_TYPE('s', 'n', 'r', 'o'),
    QT_rtp  = QT_ENCODE_TYPE('r', 't', 'p', ' '),
    QT_rtpo = QT_ENCODE_TYPE('r', 't', 'p', 'o'),
    QT_iods = QT_ENCODE_TYPE('i', 'o', 'd', 's'),
    QT_vide = QT_ENCODE_TYPE('v', 'i', 'd', 'e'),
    QT_soun = QT_ENCODE_TYPE('s', 'o', 'u', 'n'),
    QT_mp4v = QT_ENCODE_TYPE('m', 'p', '4', 'v'),
    QT_mp4a = QT_ENCODE_TYPE('m', 'p', '4', 'a'),
    QT__mp3 = QT_ENCODE_TYPE('.', 'm', 'p', '3'),
    QT_ms_U = QT_ENCODE_TYPE('m', 's', 0, 'U'),
    QT_SVQ1 = QT_ENCODE_TYPE('S', 'V', 'Q', '1'),
    QT_SVQ3 = QT_ENCODE_TYPE('S', 'V', 'Q', '3'), 
    QT_dvcp = QT_ENCODE_TYPE('d', 'v', 'c', 'p'),    
    QT_ftyp = QT_ENCODE_TYPE('f', 't', 'y', 'p'),
    QT_samr = QT_ENCODE_TYPE('s', 'a', 'm', 'r'),
    QT_sqcp = QT_ENCODE_TYPE('s', 'q', 'c', 'p'),
    QT_s263 = QT_ENCODE_TYPE('s', '2', '6', '3'),
    QT_h263 = QT_ENCODE_TYPE('h', '2', '6', '3'),
    QT_jpeg = QT_ENCODE_TYPE('j', 'p', 'e', 'g'),
    QT_MJPG = QT_ENCODE_TYPE('M', 'J', 'P', 'G'),    
    QT_mpg1 = QT_ENCODE_TYPE('m', 'p', 'g', '1'),
    QT_avc1 = QT_ENCODE_TYPE('a', 'v', 'c', '1'),
    QT_sawb = QT_ENCODE_TYPE('s', 'a', 'w', 'b'),
    QT_alac = QT_ENCODE_TYPE('a', 'l', 'a', 'c'),
    QT_alaw = QT_ENCODE_TYPE('a', 'l', 'a', 'w'),    
    QT_ulaw = QT_ENCODE_TYPE('u','l','a','w'),
    // /-->> 3GPP Timed Text box types:
    QT_text = QT_ENCODE_TYPE('t', 'e', 'x', 't'),
    QT_tx3g = QT_ENCODE_TYPE('t', 'x', '3', 'g'),
    QT_ftab = QT_ENCODE_TYPE('f', 't', 'a', 'b'),
    QT_styl = QT_ENCODE_TYPE('s', 't', 'y', 'l'),
    QT_hlit = QT_ENCODE_TYPE('h', 'l', 'i', 't'),
    QT_hclr = QT_ENCODE_TYPE('h', 'c', 'l', 'r'),
    QT_krok = QT_ENCODE_TYPE('k', 'r', 'o', 'k'),
    QT_href = QT_ENCODE_TYPE('h', 'r', 'e', 'f'),
    QT_tbox = QT_ENCODE_TYPE('t', 'b', 'o', 'x'),
    QT_blnk = QT_ENCODE_TYPE('b', 'l', 'n', 'k'),
    QT_rmra = QT_ENCODE_TYPE('r', 'm', 'r', 'a'),
    QT_rmda = QT_ENCODE_TYPE('r', 'm', 'd', 'a'),
    QT_rdrf = QT_ENCODE_TYPE('r', 'd', 'r', 'f'),
    QT_rmdr = QT_ENCODE_TYPE('r', 'm', 'd', 'r'),
    QT_url = QT_ENCODE_TYPE('u', 'r', 'l', ' ')
    // /<<-- end 3GPP Timed Text box types.
} QTKnownAtomType;

/****************************************************************************
 *  Brand Type definitions
 */
typedef enum
{
    QT_isom = QT_ENCODE_TYPE('i', 's', 'o', 'm'),
    QT_3gp4 = QT_ENCODE_TYPE('3', 'g', 'p', '4'),
    QT_3gp5 = QT_ENCODE_TYPE('3', 'g', 'p', '5'),
    QT_mp41 = QT_ENCODE_TYPE('m', 'p', '4', '1'),
    QT_mp42 = QT_ENCODE_TYPE('m', 'p', '4', '2'),
    QT_mmp4 = QT_ENCODE_TYPE('m', 'm', 'p', '4'),
    QT_m4a  = QT_ENCODE_TYPE('M', '4', 'A', ' '),
    QT_qcelp= QT_ENCODE_TYPE('3', 'g', '2', 'a'),
    QT_3gg6 = QT_ENCODE_TYPE('3', 'g', 'g', '6'),
    QT_3gp6 = QT_ENCODE_TYPE('3', 'g', 'p', '6'),
    QT_3gr6 = QT_ENCODE_TYPE('3', 'g', 'r', '6'),
    QT_3gs6 = QT_ENCODE_TYPE('3', 'g', 's', '6'),
    QT_MSNV = QT_ENCODE_TYPE('M', 'S', 'N', 'V')
} QTKnownBrandType;

/****************************************************************************
 *  Track Selection Attribute Definitions
 */
typedef enum
{
    QT_lang = QT_ENCODE_TYPE('l', 'a', 'n', 'g'),
    QT_bwas = QT_ENCODE_TYPE('b', 'w', 'a', 's'),
    QT_cdec = QT_ENCODE_TYPE('c', 'd', 'e', 'c'),
    QT_scsz = QT_ENCODE_TYPE('s', 'c', 's', 'z'),
    QT_mpsz = QT_ENCODE_TYPE('m', 'p', 's', 'z'),
    QT_mtyp = QT_ENCODE_TYPE('m', 't', 'y', 'p')
} QTKnownTrackSelectionType;


/****************************************************************************
 *  Atom factory
 */
extern CQTAtom* CreateQTAtom	(QTAtomType AtomType,
				 ULONG32 ulOffset,
				 ULONG32 ulSize,
				 CQTAtom *pParent,
				 ULONG32 ulAtomID    = 0,
				 UINT16 uChildCount  = 0);

/****************************************************************************
 *  Atom Classes
 */
/****************************************************************************
 *  Root Atom Class - RN Data Type
 */
class CQTRootAtom : public CQTAtom
{
public:
    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_HXROOT; }
};

/****************************************************************************
 *  moov Atom Class
 */
class CQT_moov_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_moov_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_moov; }
};

/****************************************************************************
 *  mvhd Atom Class
 */
class CQT_mvhd_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3];
	UINT8 pCreatTime[4]; 
	UINT8 pModifTime[4]; 
	UINT8 pTimeScale[4];
	UINT8 pDuration[4];
	UINT8 pPrefRate[4];
	UINT8 pPrefVolume[2];
	UINT8 pReserved[10];
	UINT8 pMatrix[36];
	UINT8 pPrevTime[4];
	UINT8 pPrevDuration[4];
	UINT8 pPosterTime[4];
	UINT8 pSelectionTime[4];
	UINT8 pSelectionDuration[4];
	UINT8 pCurrentTime[4];
	UINT8 pNextTrackID[4];
    } PACKING;

    struct Data64
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3];
	UINT8 pCreatTime[8];  
	UINT8 pModifTime[8]; 
	UINT8 pTimeScale[4];
	UINT8 pDuration[8];
	UINT8 pPrefRate[4];
	UINT8 pPrefVolume[2];
	UINT8 pReserved[10];
	UINT8 pMatrix[36];
	UINT8 pPrevTime[4];
	UINT8 pPrevDuration[4];
	UINT8 pPosterTime[4];
	UINT8 pSelectionTime[4];
	UINT8 pSelectionDuration[4];
	UINT8 pCurrentTime[4];
	UINT8 pNextTrackID[4];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_mvhd_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_mvhd; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_TimeScale(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUL32(((Data*) m_pData)->pTimeScale);
	}

	return GetUL32(((Data64*) m_pData)->pTimeScale);
    }

    ULONG32 Get_Duration(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUL32(((Data*) m_pData)->pDuration);
	}

	return INT64_TO_UINT32(GetUL64(((Data64*) m_pData)->pDuration));
    }
};

/****************************************************************************
 *  udta Atom Class
 */
#define QT_UDTA_TERMINATOR_LENGTH   4

class CQT_udta_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_udta_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_udta; }
};

#if defined(HELIX_FEATURE_3GPP_METAINFO) || defined(HELIX_FEATURE_SERVER)


/****************************************************************************
 *  Language encoding field is common to most meta atoms.
 */
inline void InitLanguageEncoding(char out[3])
{
    out[0] = out[1] = out[2] = ' ';
}
void ExtractLanguageEncoding(UINT8* pPadAndLang, char out[3]);


/****************************************************************************
 *  cprt Atom Class
 */
class CQT_cprt_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pCopyright[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_cprt_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_cprt; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    UINT8* GetCopyright(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pCopyright : NULL;
    }

    ULONG32 GetCopyrightLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetCopyright(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
};

/****************************************************************************
 *  auth Atom Class
 */
class CQT_auth_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pAuthor[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_auth_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_auth; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    UINT8* GetAuthor(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pAuthor : NULL;
    }

    ULONG32 GetAuthorLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetAuthor(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
};

/****************************************************************************
 *  titl Atom Class
 */
class CQT_titl_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pTitle[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_titl_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_titl; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    UINT8* GetTitle(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pTitle : NULL;
    }

    ULONG32 GetTitleLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetTitle(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
};

#ifdef HELIX_FEATURE_3GPP_METAINFO

/****************************************************************************
*  dscp Atom Class
 */
class CQT_dscp_Atom : public CQTAtom
{
public:
   /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pDescription[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_dscp_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_dscp; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    UINT8* GetDescription(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pDescription : NULL;
    }

    ULONG32 GetDescriptionLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetDescription(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
};

/****************************************************************************
 *  perf Atom Class -- 3GPP Asset Info: Performer
 */
class CQT_perf_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pPerformer[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_perf_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_perf; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    UINT8* GetPerformer(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pPerformer : NULL;
    }

    ULONG32 GetPerformerLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetPerformer(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
};

/****************************************************************************
 *  gnre Atom Class -- 3GPP Asset Info: Genre
 */
class CQT_gnre_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pGenre[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_gnre_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_gnre; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    UINT8* GetGenre(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pGenre : NULL;
    }

    ULONG32 GetGenreLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetGenre(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
};

/****************************************************************************
 *  rtng Atom Class -- 3GPP Asset Info: Rating
 */
class CQT_rtng_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pRatingEntity[4];
        UINT8 pRatingCriteria[4];
        UINT8 pPadAndLang[2];
        UINT8 pRatingInfo[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_rtng_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_rtng; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    ULONG32 GetRatingEntityLength(void)
    {
        // rating entity is 4 bytes
        return 4;
    }
    
    UINT8* GetRatingEntity(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pRatingEntity : NULL;
    }

    ULONG32 GetRatingCriteriaLength(void)
    {
        // rating criteria is 4 bytes
        return 4;
    }
    
    UINT8* GetRatingCriteria(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pRatingCriteria : NULL; 
    }

    ULONG32 GetRatingInfoLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetRatingInfo(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
    
    UINT8* GetRatingInfo(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pRatingInfo : NULL;
    }
};

/****************************************************************************
 *  clsf Atom Class -- 3GPP Asset Info: Classification
 */
class CQT_clsf_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pClassEntity[4];
        UINT8 pClassTable[2];
        UINT8 pPadAndLang[2];
        UINT8 pClassInfo[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_clsf_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_clsf; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    ULONG32 GetClassEntityLength(void)
    {
        // class entity is 4 bytes
        return 4;
    }
    
    UINT8* GetClassEntity(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pClassEntity : NULL;
    }

    UINT16 GetClassTable(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? GetUI16(((Data*) m_pData)->pClassTable) : 0;
    }

    ULONG32 GetClassInfoLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetClassInfo(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
    
    UINT8* GetClassInfo(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pClassInfo : NULL;
    }
};

/****************************************************************************
 *  kywd Atom Class -- 3GPP Asset Info: Keywords
 */
class CQT_kywd_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pKeywordCnt[1];
        UINT8 pKeywords[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_kywd_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_kywd; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    UINT8 Get_KeywordCnt(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? *(((Data*) m_pData)->pKeywordCnt) : 0;
    }

    UINT8 Get_KeywordSize(ULONG32 ulKeywordIdx)
    {
        HX_ASSERT(ulKeywordIdx < (ULONG32)Get_KeywordCnt());
        if(ulKeywordIdx >= (ULONG32)Get_KeywordCnt())
            return 0;

        ULONG32 ulOffset = Find_Keyword(ulKeywordIdx);

        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*)m_pData)->pKeywords[ulOffset] : 0;
    }

    UINT8* Get_KeywordEntry(ULONG32 ulKeywordIdx)
    {
        HX_ASSERT(ulKeywordIdx < (ULONG32)Get_KeywordCnt());
        if(ulKeywordIdx >= (ULONG32)Get_KeywordCnt())
            return NULL;

        ULONG32 ulOffset = Find_Keyword(ulKeywordIdx);

        HX_ASSERT(m_pData);
        return (m_pData) ? &( ((Data*)m_pData)->pKeywords[ulOffset + 1] ) : NULL;
    }

private:

    ULONG32 Find_Keyword(ULONG32 ulKeywordIdx)
    {
        HX_ASSERT(ulKeywordIdx < (ULONG32)Get_KeywordCnt());
        if(ulKeywordIdx >= (ULONG32)Get_KeywordCnt())
            return 0L;

        HX_ASSERT(m_pData);
        if(!m_pData)
            return 0L;

        ULONG32 ulOffset = 0;
        UINT8* pSize = ((Data*)m_pData)->pKeywords; // size of 1st keyword

        for(ULONG32 ulIndex = 0; ulIndex < ulKeywordIdx; ulIndex++)
        {
            ulOffset += *pSize + 1;
            pSize = ((Data*)m_pData)->pKeywords + ulOffset; // size of next kywd
        }

        return ulOffset;
    }
};

/****************************************************************************
 *  loci Atom Class -- 3GPP Asset Info: Location Info
 */
class CQT_loci_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pName[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_loci_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_loci; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    ULONG32 GetNameLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetName(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }

    UINT8* GetName(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pName : NULL;
    }

    UINT8 GetRole(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*)m_pData)->pName[ulOffset] : 0;
    }

    INT16 GetLongitude_WholePart(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // return 2 bytes at position of longitude whole part
        HX_ASSERT(m_pData);
        return (m_pData) ? GetI16(((Data*)m_pData)->pName + ulOffset + 1) : 0;
    }

    // fractional part is assumbed to be a positive number
    UINT16 GetLongitude_FractionalPart(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // return 2 bytes at position of longitude fractional part
        HX_ASSERT(m_pData);
        return (m_pData) ? GetUI16(((Data*)m_pData)->pName + ulOffset + 3) : 0;
    }

    INT16 GetLatitude_WholePart(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // return 2 bytes at position of latitude whole part
        HX_ASSERT(m_pData);
        return (m_pData) ? GetI16(((Data*)m_pData)->pName + ulOffset + 5) : 0;
    }

    // fractional part is assumbed to be a positive number
    UINT16 GetLatitude_FractionalPart(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // return 2 bytes at position of latitude fractional part
        HX_ASSERT(m_pData);
        return (m_pData) ? GetUI16(((Data*)m_pData)->pName + ulOffset + 7) : 0;
    }

    INT16 GetAltitude_WholePart(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // return 2 bytes at position of altitude whole part
        HX_ASSERT(m_pData);
        return (m_pData) ? GetI16(((Data*)m_pData)->pName + ulOffset + 9) : 0;
    }

    // fractional part is assumbed to be a positive number
    UINT16 GetAltitude_FractionalPart(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // return 2 bytes at position of altitude fractional part
        HX_ASSERT(m_pData);
        return (m_pData) ? GetUI16(((Data*)m_pData)->pName + ulOffset + 11) : 0;
    }

    ULONG32 GetAstronomicalBodyLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetAstronomicalBody(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }

    UINT8* GetAstronomicalBody(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // return '\0' terminated string at position of astronomical body
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*)m_pData)->pName + ulOffset + 13 : NULL;
    }

    ULONG32 GetAdditionalNotesLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetAdditionalNotes(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }

    UINT8* GetAdditionalNotes(void)
    {
        // scan past name
        ULONG32 ulOffset = ScanPastString(0); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // scan past astronomical body
        ulOffset = ScanPastString(ulOffset + 13); 

        // still in bounds of atom?
        HX_ASSERT(ulOffset < m_ulSize); 
        if(ulOffset >= m_ulSize)
            return 0;

        // return '\0' terminated string at end of astronomical body string
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*)m_pData)->pName + ulOffset : NULL;
    }

private:

    ULONG32 ScanPastString(ULONG32 ulInitialOffset)
    {
        HX_ASSERT(m_pData);
        if(!m_pData)
            return 0L;

        return (ulInitialOffset + EncStrUtils::ByteLengthScan((const char*)(((Data*)m_pData)->pName + ulInitialOffset),
                                            TRUE, EncStrUtils::SCAN_FLAG_UTF16BE));
    }
};

/****************************************************************************
 *  clsf Atom Class -- 3GPP Asset Info: Album
 */
class CQT_albm_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pPadAndLang[2];
        UINT8 pAlbumTitle[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_albm_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_albm; }

    /*
     *	Data Access Methods
     */
    void GetLanguageEncoding(char out[3])
    {
        InitLanguageEncoding(out);
        HX_ASSERT(m_pData);
        if(m_pData)
        {
            ExtractLanguageEncoding(((Data*)m_pData)->pPadAndLang, out);
        }
    }

    ULONG32 GetAlbumTitleLength(void)
    {
        return EncStrUtils::ByteLengthScan((const char*)GetAlbumTitle(), FALSE, EncStrUtils::SCAN_FLAG_UTF16BE);
    }
    
    UINT8* GetAlbumTitle(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? ((Data*) m_pData)->pAlbumTitle : NULL;
    }

    HXBOOL HasTrackNumber(void)
    {
        ULONG32 ulTrackNumberOffset = 0;
        return GetTrackNumberOffset(ulTrackNumberOffset);
    }

    UINT8 GetTrackNumber(void)
    {
        ULONG32 ulTrackNumberOffset = 0;
        if(GetTrackNumberOffset(ulTrackNumberOffset))
        {
            // return 1 byte at the position of track number
            HX_ASSERT(m_pData);
            return (m_pData) ? *(((Data*)m_pData)->pAlbumTitle + ulTrackNumberOffset) : 0;
        }
        return 0;
    }
    
private:

    HXBOOL GetTrackNumberOffset(ULONG32& ulTrackNumberOffset)
    {
        HX_ASSERT(m_pData);
        if(!m_pData) return FALSE;

        UINT32 len = EncStrUtils::ByteLengthScan((const char*)(((Data*)m_pData)->pAlbumTitle),
                                    TRUE, EncStrUtils::SCAN_FLAG_UTF16BE);

        const ULONG32 ulTitleOffset = 6;
        if((ulTitleOffset + len) < m_ulSize)
        {
            // track number found
            ulTrackNumberOffset = len;
            return TRUE;
        }
        // track number missing (it's an optional field)
        return FALSE;
    }
};

/****************************************************************************
 *  yrrc Atom Class -- 3GPP Asset Info: Classification
 */
class CQT_yrrc_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pRecordingYear[2];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_yrrc_Atom(ULONG32 ulOffset,
                  ULONG32 ulSize,
                  CQTAtom *pParent) : CQTAtom(ulOffset,
                                              ulSize,
                                              pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)  { return TRUE; }
    virtual QTAtomType	GetType(void)	{ return QT_yrrc; }

    /*
     *	Data Access Methods
     */
    UINT16 GetRecordingYear(void)
    {
        HX_ASSERT(m_pData);
        return (m_pData) ? GetUI16(((Data*) m_pData)->pRecordingYear) : 0;
    }
};

#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER

/****************************************************************************
 *  name Atom Class
 */
class CQT_name_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pName[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_name_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_name; }
};

/****************************************************************************
 *  tsel Atom Class
 */
class CQT_tsel_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct AttributeEntry
    {
	UINT8 pAttribute[4];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3];
	UINT8 pSwitchGroup[4]; 
	AttributeEntry pAttributeList[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_tsel_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_tsel; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_SwitchGroup(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pSwitchGroup);
    }

    ULONG32 Get_NumEntries(void)
    {
	HX_ASSERT(m_pData && m_pBuffer);
	return ((m_ulSize - 
		 QT_HEADER_SIZE - 
		 ((((Data*) m_pData)->pAttributeList[0].pAttribute) - m_pData)) / 
		sizeof(AttributeEntry));
    }

    ULONG32 Get_Attribute(ULONG32 ulEntryIdx)
    {
	HX_ASSERT(m_pData);

	return GetUL32(((Data*) m_pData)->pAttributeList[ulEntryIdx].pAttribute);
    }
};

/****************************************************************************
 *  hinf Atom Class
 */
class CQT_hinf_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_hinf_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_hinf; }
};

/****************************************************************************
 *  trpy Atom Class
 */
class CQT_trpy_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pBytesToSendHi[4];    // not including Net. headers
	UINT8 pBytesToSendLo[4];    // not including Net. headers
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_trpy_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_trpy; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_BytesToSend(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pBytesToSendLo);
    }
};

/****************************************************************************
 *  nump Atom Class
 */
class CQT_nump_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pPacketsToSendHi[4];
	UINT8 pPacketsToSendLo[4];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_nump_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_nump; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_PacketsToSend(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pPacketsToSendLo);
    }
};

/****************************************************************************
 *  tpyl Atom Class
 */
class CQT_tpyl_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pBytesToSendHi[4];    // not including RTP or Net. headers
	UINT8 pBytesToSendLo[4];    // not including RTP or Net. headers
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_tpyl_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_tpyl; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_BytesToSend(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pBytesToSendLo);
    }
};

/****************************************************************************
 *  payt Atom Class
 */
class CQT_payt_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pPayloadType[4];    
	UINT8 pPayloadString[1];    // Pascal String
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_payt_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_payt; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_PayloadType(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pPayloadType);
    }

    UINT8* Get_PayloadString(void)
    {
	HX_ASSERT(m_pData);
	return (((Data*) m_pData)->pPayloadString);
    }
};

/****************************************************************************
 *  hnti Atom Class
 */
class CQT_hnti_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_hnti_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_hnti; }
};

/****************************************************************************
 *  sdp Atom Class
 */
class CQT_sdp_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pDesc[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_sdp_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_sdp; }
};

/****************************************************************************
 *  rtp Atom Class
 */
class CQT_rtp_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pSubType[4];
	UINT8 pData[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_rtp_Atom(ULONG32 ulOffset,
		 ULONG32 ulSize, 
		 CQTAtom *pParent) : CQTAtom(ulOffset,
					     ulSize,
					     pParent) {;}
    /*
     *	Data Access Methods
     */
    ULONG32 Get_SubType(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pSubType);
    }

    UINT8* Get_Data(void)
    {
	HX_ASSERT(m_pData);
	return ((Data*) m_pData)->pData;
    }

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_rtp; }
};

/****************************************************************************
 *  trak Atom Class
 */
class CQT_trak_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_trak_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_trak; }
};

/****************************************************************************
 *  tkhd Atom Class
 */
class CQT_tkhd_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3];
	UINT8 pCreatTime[4]; 
	UINT8 pModifTime[4]; 
	UINT8 pTrackID[4];
	UINT8 pReserved1[4];
	UINT8 pDuration[4];
	UINT8 pReserved2[8];
	UINT8 pLayer[2];
	UINT8 pAltGroup[2];
	UINT8 pVolume[2];
	UINT8 pReserved3[2];
	UINT8 Matrix[36];
	UINT8 pTrackWidth[4];
	UINT8 pTrackHeight[4];
    } PACKING;

    struct Data64
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3];
	UINT8 pCreatTime[8]; 
	UINT8 pModifTime[8]; 
	UINT8 pTrackID[4];
	UINT8 pReserved1[4];
	UINT8 pDuration[8];
	UINT8 pReserved2[8];
	UINT8 pLayer[2];
	UINT8 pAltGroup[2];
	UINT8 pVolume[2];
	UINT8 pReserved3[2];
	UINT8 Matrix[36];
	UINT8 pTrackWidth[4];
	UINT8 pTrackHeight[4];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_tkhd_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_tkhd; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_TrackID(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUL32(((Data*) m_pData)->pTrackID);
	}

	return GetUL32(((Data64*) m_pData)->pTrackID);
    }

    ULONG32 Get_Duration(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUL32(((Data*) m_pData)->pDuration);
	}

	return INT64_TO_UINT32(GetUL64(((Data64*) m_pData)->pDuration));
    }

    UINT16 Get_AltGroup(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUI16(((Data*) m_pData)->pAltGroup);
	}

	return GetUI16(((Data64*) m_pData)->pAltGroup);
    }

    double Get_TrackWidth(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetFixed32(((Data*) m_pData)->pTrackWidth);
	}

	return GetFixed32(((Data64*) m_pData)->pTrackWidth);
    }

    double Get_TrackHeight(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetFixed32(((Data*) m_pData)->pTrackHeight);
	}

	return GetFixed32(((Data64*) m_pData)->pTrackHeight);
    }

    double Get_TrackMatrixTx(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetFixed32((UINT8*)(((Data*) m_pData)->Matrix) +
				       TKHD_MATRIX_TX_LOCATION);
	}

	return GetFixed32((UINT8*)(((Data64*) m_pData)->Matrix) +
				       TKHD_MATRIX_TX_LOCATION);
    }

    double Get_TrackMatrixTy(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetFixed32((UINT8*)(((Data*) m_pData)->Matrix) +
			      TKHD_MATRIX_TY_LOCATION);
	}

	return GetFixed32((UINT8*)(((Data64*) m_pData)->Matrix) +
			  TKHD_MATRIX_TY_LOCATION);
    }
};

/****************************************************************************
 *  mdia Atom Class
 */
class CQT_mdia_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_mdia_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_mdia; }
};

/****************************************************************************
 *  mdhd Atom Class
 */
class CQT_mdhd_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3];
	UINT8 pCreatTime[4]; 
	UINT8 pModifTime[4]; 
	UINT8 pTimeScale[4];
	UINT8 pDuration[4];
	UINT8 pLangCode[2];
	UINT8 pQuality[2];
    } PACKING;

    struct Data64
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3];
	UINT8 pCreatTime[8]; 
	UINT8 pModifTime[8]; 
	UINT8 pTimeScale[4];
	UINT8 pDuration[8];
	UINT8 pLangCode[2];
	UINT8 pQuality[2];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_mdhd_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_mdhd; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_TimeScale(void)
    {
	HX_ASSERT(m_pData);
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUL32(((Data*) m_pData)->pTimeScale);
	}

	return GetUL32(((Data64*) m_pData)->pTimeScale);
    }
};

/****************************************************************************
 *  hdlr Atom Class
 */
class CQT_hdlr_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3];
	UINT8 pCompType[4]; 
	UINT8 pCompSubtype[4]; 
	UINT8 pCompManufacturer[4];
	UINT8 pCompFlags[4];
	UINT8 pCompFlagsMask[4];
	UINT8 pName[1];	    // Pascal String
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_hdlr_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_hdlr; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_CompType(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pCompType);
    }

    ULONG32 Get_CompSubtype(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pCompSubtype);
    }
};

/****************************************************************************
 *  minf Atom Class
 */
class CQT_minf_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_minf_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_minf; }

};

/****************************************************************************
 *  dinf Atom Class
 */
class CQT_dinf_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_dinf_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_dinf; }
};

/****************************************************************************
 *  dref Atom Class
 */
#define QT_SELFREF_FLAG 0x00000001
#define QT_ALIS_END	    -1	/* Last Marker */
#define QT_ALIS_ABSPATH	    2	/* Absolute path name marker */
#define QT_ALIS_MAXCOUNT    10	/* Maximum number of Markers */

class CQT_dref_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct ItemEntry
    {
	UINT8 pType[2];   // type of information
	UINT8 pSize[2];   // size of variable data
	UINT8 pData[1];   // actual data
    } varInfo;
    
    struct DataRef
    {
	UINT8 pReserved1[130];
	UINT8 pNLvlFrom[2];	// # of levels from fromFile/toFile until
	UINT8 pNLvlTo[2];	// a common ancestor directory is found
	UINT8 pReserved2[16];
	UINT8 pArray[1];	// variable length info
    } PACKING;

    struct ArrayEntry
    {
	UINT8 pSize[4];
	UINT8 pType[4];
	UINT8 pVersion[1];
	UINT8 pFlags[3];
	UINT8 pDataRef[1];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	UINT8 pArray[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_dref_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_dref; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pNumEntries);
    }

    CQT_dref_Atom::ArrayEntry* GetRefEntry(ULONG32 ulArrayIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulArrayIdx < Get_NumEntries());
	return (ArrayEntry*) FindArrayEntry(((Data*) m_pData)->pArray,
					    ulArrayIdx);
    }

    ULONG32 Get_RefType(ULONG32 ulArrayIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulArrayIdx < Get_NumEntries());
	return GetUL32(((ArrayEntry*) FindArrayEntry(
					((Data*) m_pData)->pArray,
					ulArrayIdx))->pType);
    }

    ULONG32 Get_RefFlags(ULONG32 ulArrayIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulArrayIdx < Get_NumEntries());
	return GetFlags(((ArrayEntry*) FindArrayEntry(
					((Data*) m_pData)->pArray,
					ulArrayIdx))->pFlags);
    }

    ULONG32 Get_RefType(CQT_dref_Atom::ArrayEntry* pRefEntry)
    {
	HX_ASSERT(pRefEntry);
	return GetUL32(pRefEntry->pType);
    }

    ULONG32 Get_RefFlags(CQT_dref_Atom::ArrayEntry* pRefEntry)
    {
	HX_ASSERT(pRefEntry);
	return GetFlags(pRefEntry->pFlags);
    }

    CQT_dref_Atom::DataRef* Get_RefData
    (
	CQT_dref_Atom::ArrayEntry* pRefEntry
    )
    {
	HX_ASSERT(pRefEntry);
	return (CQT_dref_Atom::DataRef*) pRefEntry->pDataRef;
    }

    ULONG32 Get_RefDataLen(CQT_dref_Atom::ArrayEntry* pRefEntry)
    {
	HX_ASSERT(pRefEntry);
	return (GetUL32(pRefEntry->pSize) - sizeof(ArrayEntry) + 1);
    }
};

/****************************************************************************
 *  stbl Atom Class
 */
class CQT_stbl_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_stbl_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_stbl; }
};

/****************************************************************************
 *  stsd Atom Class
 */
class CQT_stsd_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TaggedEntry
    {
	UINT8 pSize[4];
	UINT8 pTag[4];
	UINT8 pData[1];
    } PACKING;

    class ArrayEntry
    {
    public:
	UINT8 pSize[4];
	UINT8 pDataFormat[4];
	UINT8 pReserved[6];
	UINT8 pDataRefIdx[2];
    } PACKING;

    class HintArrayEntry : public ArrayEntry
    {
    public:
	UINT8 pHintTrakVersion[2];
	UINT8 pLastCompHintTrakVersion[2];
	UINT8 pMaxPacketSize[4];
	UINT8 pTaggedArray[1];
    } PACKING;

    class AudioArrayEntry : public ArrayEntry	// just a space holder in MP4
    {
    public:
	UINT8 pVersion[2];
	UINT8 pRevLevel[2];
	UINT8 pVendor[4];
	UINT8 pNumChannels[2];
	UINT8 pSampleSize[2];
	UINT8 pCompressionID[2];
	UINT8 pPacketSize[2];
	UINT8 pSampleRate[4];
    } PACKING;

    class VideoArrayEntry : public ArrayEntry
    {
    public:
	UINT8 pVersion[2];
	UINT8 pRevision[2];
	UINT8 pVendor[4];
	UINT8 pTemporalQuality[4];
	UINT8 pSpatialQuality[4];
	UINT8 pWidth[2];
	UINT8 pHeight[2];
	UINT8 pHRes[4];
	UINT8 pVRes[4];
	UINT8 pDataSize[4];
	UINT8 pFrameCount[2];
	UINT8 pName[32];
	UINT8 pDepth[2];
	UINT8 pClutID[2];
    } PACKING;

    class AudioMP4ArrayEntry : public AudioArrayEntry
    {
    public:
	UINT8 pSize[4];
	UINT8 pType[4];
	UINT8 pVersion[1];
	UINT8 pFlags[3];
	UINT8 pESDescriptor[1];
    } PACKING;

    class QTSoundCompressionInfo: public AudioArrayEntry
    {
    public:
        UINT8 pSamplesPerPacket[4];//Quick Time 3 Specific 
        UINT8 pBytesPerPacket[4];
        UINT8 pBytesPerFrame[4];
        UINT8 pBytesPerSample[4];
    } PACKING;


    class SounDescSIDecomParam: public QTSoundCompressionInfo 
    {
    public:
        UINT8 pSize[4];
        UINT8 pType[4];//wave
        UINT8 pSizeFrma[4];
        UINT8 pTypeFrma[4];//generally
        UINT8 pDataFormatMp4a[4];	
    }PACKING;

    class AudioQTMP4ArrayEntry : public SounDescSIDecomParam
    {
    public:
        UINT8 pSize[4];
        UINT8 pType[4];
        UINT8 pSkip[4];
        UINT8 pSizeEsds[4];	
        UINT8 pEsdS[4];	
        UINT8 pVersion[1];
        UINT8 pFlags[3];
        UINT8 pESDescriptor[1];
    } PACKING;

    class VideoMP4ArrayEntry : public VideoArrayEntry
    {
    public:
	UINT8 pSize[4];
	UINT8 pType[4];
	UINT8 pVersion[1];
	UINT8 pFlags[3];
	UINT8 pESDescriptor[1];
    } PACKING;

    class AudioSAMRArrayEntry : public AudioArrayEntry
    {
    public:
	UINT8 pDecoderSpecificInfo[1];
    } PACKING;

    class AudioQCELPArrayEntry : public AudioArrayEntry
    {
    public:
	UINT8 pDecoderSpecificInfo[1];
    } PACKING;

    class AudioSAWBArrayEntry : public AudioArrayEntry
    {
    public:
	UINT8 pDecoderSpecificInfo[1];
    } PACKING;

    class VideoS263ArrayEntry : public VideoArrayEntry
    {
    public:
	UINT8 pDecoderSpecificInfo[1];
    } PACKING;

    class VideoAVCArrayEntry : public VideoArrayEntry
    {
    public:
	UINT8 pDecoderSpecificInfo[1];
    } PACKING;

    class AudioALACArrayEntry : public AudioArrayEntry
    {
    public:
	UINT8 pDecoderSpecificInfo[1];
    } PACKING;

    ////////////////////////////////
    // /  3GPP Timed Text structs: 
    ///
    class BoxRecord
    {
    public:
	INT16 top;
	INT16 left;
	INT16 bottom;
	INT16 right;
    } PACKING;

    class StyleRecord
    {
    public:
	UINT16 startChar;
	UINT16 endChar;
	UINT16 fontID;
	UINT8  faceStyleFlags;
	UINT8  fontSize;
	UINT8  textColorRGBA[4];
    } PACKING;

    class FontRecord
    {
    public:
	UINT16 fontID;
	UINT8  fontNameLength;
	UINT8* pFont; // /There are [fontNameLength] bytes in the string;
    } PACKING;

    class FontTableBox // / ‘ftab’, extends Box
    {
    public:
	FontTableBox() { pFontEntries = NULL; }
	~FontTableBox();

	UINT32      ulFontTableSizeInBytes;
	UINT8       FTAB[4]; // / Holds string 'ftab'.  It must be this string.
	UINT16      entryCount;
	FontRecord* pFontEntries; // /= array of [entryCount] FontRecords 
    };

    class TextSampleEntry : public ArrayEntry	// / 'tx3g', extends SampleEntry
    {
    public:
	TextSampleEntry(ArrayEntry* pUnpackedEntry,
                       UINT32 ulSzOfUnpackedEntry);

	UINT32       ulDisplayFlags;
	INT8         horizontalJustification;
	INT8         verticalJustification;
	UINT8        uBackgroundColorRGBA[4];
	BoxRecord    defaultTextBox;
	StyleRecord  defaultStyle;
	FontTableBox fontTable;
    };
    //
    // / END 3GPP TT structs.
    ////////////////////////////////

    
    
    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	UINT8 pArray[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_stsd_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_stsd; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pNumEntries);
    }

    CQT_stsd_Atom::ArrayEntry* Get_SampleDesc(ULONG32 ulArrayIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulArrayIdx < Get_NumEntries());
	return (ArrayEntry*) FindArrayEntry(((Data*) m_pData)->pArray,
					    ulArrayIdx);
    }

    ULONG32 Get_DataRefIdx(ULONG32 ulArrayIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulArrayIdx < Get_NumEntries());
	return GetUI16(((ArrayEntry*) FindArrayEntry(
					((Data*) m_pData)->pArray,
					ulArrayIdx))->pDataRefIdx);
    }

    ULONG32 Get_DataRefIdx(CQT_stsd_Atom::ArrayEntry* pSampleDesc)
    {
	HX_ASSERT(pSampleDesc);
	return GetUI16(pSampleDesc->pDataRefIdx);
    }

    ULONG32 Get_DataFormat(CQT_stsd_Atom::ArrayEntry* pSampleDesc)
    {
	HX_ASSERT(pSampleDesc);
	return GetUL32(pSampleDesc->pDataFormat);
    }

    TaggedEntry* Get_TaggedEntry(CQT_stsd_Atom::HintArrayEntry* pSampleDesc,
				 ULONG32 ulTaggedEntryIdx)
    {
	HX_ASSERT(pSampleDesc);

	UINT8 *pTaggedEntry = pSampleDesc->pTaggedArray;
	UINT8 *pTaggedTableEnd = ((UINT8*) pSampleDesc) + 
				 GetUL32(pSampleDesc->pSize);

	while (pTaggedEntry < pTaggedTableEnd)
	{
	    if (ulTaggedEntryIdx == 0)
	    {
		return (TaggedEntry*) pTaggedEntry;
	    }

	    ulTaggedEntryIdx--;
	    pTaggedEntry += GetUL32(((TaggedEntry*) pTaggedEntry)->pSize);
	};

	return NULL;
    }
};

/****************************************************************************
 *  stts Atom Class
 */
class CQT_stts_Atom : public CQTPagingAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pSampleCount[4];
	UINT8 pSampleDuration[4];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	TableEntry pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_stts_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) 
    : CQTPagingAtom(ulOffset, ulSize, pParent)
    , m_ulNumEntries(0) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_stts; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	UINT8* pData;
	
	if (m_ulNumEntries)
	{
	    return m_ulNumEntries;
	}

	pData = m_pData;
	HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pNumEntries, 
		sizeof(((Data*) pData)->pNumEntries),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    m_ulNumEntries = GetUL32(((Data*) pData)->pNumEntries);
	    return m_ulNumEntries;
	}

	return 0;
    }

    ULONG32 Get_SampleCount(ULONG32 ulEntryIdx)
    {
	UINT8* pData = m_pData;
	HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pTable[ulEntryIdx].pSampleCount, 
		sizeof(((Data*) pData)->pTable[ulEntryIdx].pSampleCount),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pSampleCount);
	}

	return 0;
    }

    QTATOMS_INLINE ULONG32 Get_SampleDuration(ULONG32 ulEntryIdx);

    virtual void SetBuffer(IHXBuffer *pBuffer)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetBuffer(pBuffer);
    }

    virtual void SetMemPager(CMemPager* pMemPager)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetMemPager(pMemPager);
    }

private:
    ULONG32 m_ulNumEntries;
};

/****************************************************************************
 *  ctts Atom Class
 */
class CQT_ctts_Atom : public CQTPagingAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pSampleCount[4];
	UINT8 pSampleOffset[4];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	TableEntry pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_ctts_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) 
    : CQTPagingAtom(ulOffset, ulSize, pParent)
    , m_ulNumEntries(0) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_ctts; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	UINT8* pData;

	if (m_ulNumEntries != 0)
	{
	    return m_ulNumEntries;
	}

	pData = m_pData;
	HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pNumEntries, 
		sizeof(((Data*) pData)->pNumEntries),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    m_ulNumEntries = GetUL32(((Data*) pData)->pNumEntries);
	    return m_ulNumEntries;
	}

	return 0;
    }

    ULONG32 Get_SampleCount(ULONG32 ulEntryIdx)
    {
	UINT8* pData = m_pData;
	HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pTable[ulEntryIdx].pSampleCount, 
		sizeof(((Data*) pData)->pTable[ulEntryIdx].pSampleCount),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pSampleCount);
	}

	return 0;
    }

    ULONG32 Get_SampleOffset(ULONG32 ulEntryIdx)
    {
	UINT8* pData = m_pData;
	HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pTable[ulEntryIdx].pSampleOffset, 
		sizeof(((Data*) pData)->pTable[ulEntryIdx].pSampleOffset),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pSampleOffset);
	}

	return 0;
    }

    virtual void SetBuffer(IHXBuffer *pBuffer)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetBuffer(pBuffer);
    }

    virtual void SetMemPager(CMemPager* pMemPager)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetMemPager(pMemPager);
    }

private:
    ULONG32 m_ulNumEntries;
};

/****************************************************************************
 *  stss Atom Class
 */
class CQT_stss_Atom : public CQTPagingAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pSampleNum[4];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	TableEntry pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_stss_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent)
    : CQTPagingAtom(ulOffset, ulSize, pParent)
    , m_ulNumEntries(0) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_stss; }

    /*
     *	Data Access Methods
     */
    QTATOMS_INLINE ULONG32 Get_NumEntries(void);

    QTATOMS_INLINE ULONG32 Get_SampleNum(ULONG32 ulEntryIdx);

    virtual void SetBuffer(IHXBuffer *pBuffer)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetBuffer(pBuffer);
    }

    virtual void SetMemPager(CMemPager* pMemPager)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetMemPager(pMemPager);
    }

private:
    ULONG32 m_ulNumEntries;
};

/****************************************************************************
 *  stsc Atom Class
 */
class CQT_stsc_Atom : public CQTPagingAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pFirstChunk[4];
	UINT8 pSamplesPerChunk[4];
	UINT8 pSampleDescID[4];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	TableEntry pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_stsc_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) 
    : CQTPagingAtom(ulOffset, ulSize, pParent)
    , m_ulNumEntries(0) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_stsc; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	UINT8* pData;

	if (m_ulNumEntries != 0)
	{
	    return m_ulNumEntries;
	}

	pData = m_pData;
	HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pNumEntries, 
		sizeof(((Data*) pData)->pNumEntries),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    m_ulNumEntries = GetUL32(((Data*) pData)->pNumEntries);
	    return m_ulNumEntries;
	}

	return 0;
    }

    QTATOMS_INLINE ULONG32 Get_FirstChunk(ULONG32 ulEntryIdx);

    QTATOMS_INLINE ULONG32 Get_SamplesPerChunk(ULONG32 ulEntryIdx);

    QTATOMS_INLINE ULONG32 Get_SampleDescID(ULONG32 ulEntryIdx);

    virtual void SetBuffer(IHXBuffer *pBuffer)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetBuffer(pBuffer);
    }

    virtual void SetMemPager(CMemPager* pMemPager)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetMemPager(pMemPager);
    }

private:
    ULONG32 m_ulNumEntries;
};

/****************************************************************************
 *  stsz Atom Class
 */
class CQT_stsz_Atom : public CQTPagingAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pSampleSize[4];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pSampleSize[4];
	UINT8 pNumEntries[4]; 
	TableEntry pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */  
    CQT_stsz_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) 
    : CQTPagingAtom(ulOffset, ulSize, pParent)
    , m_ulNumEntries(0)
    , m_ulSampleSize(QT_BAD_SAMPLE_SIZE)
    , m_uFieldSizeBits(32) {;}   
	
    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_stsz; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	UINT8* pData;

	if (m_ulNumEntries != 0)
	{
	    return m_ulNumEntries;
	}

	pData = m_pData;
	HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pNumEntries, 
		sizeof(((Data*) pData)->pNumEntries),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    m_ulNumEntries = GetUL32(((Data*) pData)->pNumEntries);
	    return m_ulNumEntries;
	}

	return 0;
    }

    QTATOMS_INLINE ULONG32 Get_SampleSize(void);

    // This method is called frequently and needs to be inlined for performance.  
    // Do not make virtual.
    QTATOMS_INLINE ULONG32 Get_SampleSize(ULONG32 ulFieldIdx);

    virtual void SetBuffer(IHXBuffer *pBuffer)
    {
	m_ulNumEntries = 0;
	m_ulSampleSize = QT_BAD_SAMPLE_SIZE;
	CQTPagingAtom::SetBuffer(pBuffer);
    }

    virtual void SetMemPager(CMemPager* pMemPager)
    {
	m_ulNumEntries = 0;
	m_ulSampleSize = QT_BAD_SAMPLE_SIZE;
	CQTPagingAtom::SetMemPager(pMemPager);
    }

private:
    ULONG32 m_ulNumEntries;

protected:
    ULONG32 m_ulSampleSize;
    // Below member variable is for stz2 handling.
    // It is included here (base class of stz2) to avoid making
    // key methods virtual which would prohibit inlining and thus
    // have performance consequences.
    UINT8 m_uFieldSizeBits;
};

/****************************************************************************
 *  stsz Atom Class
 */
class CQT_stz2_Atom : public CQT_stsz_Atom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry16
    {
	UINT8 pSampleSize[2];
    } PACKING;

    struct TableEntry8
    {
	UINT8 pSampleSize[1];
    } PACKING;

    struct Data16
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pReserved[3];
	UINT8 pFieldSize[1];
	UINT8 pNumEntries[4]; 
	TableEntry16 pTable[1];
    } PACKING;

    struct Data8
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pReserved[3];
	UINT8 pFieldSize[1];
	UINT8 pNumEntries[4]; 
	TableEntry8 pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_stz2_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) 
    : CQT_stsz_Atom(ulOffset, ulSize, pParent)
    {
	m_uFieldSizeBits = 0;
	m_ulSampleSize = 0;	// No generic size in stz2 atom
    }	

    /*
     *	Required Virtual Methods
     */
    virtual QTAtomType	GetType(void)	    { return QT_stz2; }

    
    UINT8 GetFieldBits(void)
    { 
	UINT8* pData;

	if (m_uFieldSizeBits == 0)
	{
	    return m_uFieldSizeBits;
	}

	pData = m_pData;
	HX_ASSERT(m_pData);

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data8*) pData)->pFieldSize, 
		sizeof(((Data8*) pData)->pFieldSize),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    m_uFieldSizeBits = *(((Data8*) pData)->pFieldSize);

	    HX_ASSERT((m_uFieldSizeBits == 4) ||
		      (m_uFieldSizeBits == 8) ||
		      (m_uFieldSizeBits == 16));

	    return m_uFieldSizeBits;
	}

	return 0;
    }

    ULONG32 GetCompactSampleSize(ULONG32 ulFieldIdx)
    {
	UINT8* pData;

	if (m_uFieldSizeBits == 0)
	{
	    GetFieldBits();

	    if (m_lastStatus != HXR_OK)
	    {
		return 0;
	    }
	}

	pData = m_pData;
	HX_ASSERT(m_pData);

	if (m_uFieldSizeBits == 16)
	{
#if !defined(QTCONFIG_NO_PAGING)
	    if (m_pMemPager)
	    { 
		m_lastStatus = m_pMemPager->PageIn(
		    ((Data16*) pData)->pTable[ulFieldIdx].pSampleSize, 
		    sizeof(((Data16*) pData)->pTable[ulFieldIdx].pSampleSize),
		    pData);
	    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
	    
	    if (m_lastStatus == HXR_OK)
	    {
		return GetUI16(((Data16*) pData)->pTable[ulFieldIdx].pSampleSize);
	    }
	}
	else 
	{
	    ULONG32 ulSubFieldIdx = 0;

	    if (m_uFieldSizeBits == 4)
	    {
		ulSubFieldIdx = (ulFieldIdx & 0x00000001);
		ulFieldIdx = (ulFieldIdx >> 1);
	    }

#if !defined(QTCONFIG_NO_PAGING)
	    if (m_pMemPager)
	    { 
		m_lastStatus = m_pMemPager->PageIn(
		    ((Data8*) pData)->pTable[ulFieldIdx].pSampleSize, 
		    sizeof(((Data8*) pData)->pTable[ulFieldIdx].pSampleSize),
		    pData);
	    }
#endif /* #if !defined(QTCONFIG_NO_PAGING) */
	    
	    if (m_lastStatus == HXR_OK)
	    {
		if (m_uFieldSizeBits == 8)
		{
		    return *(((Data8*) pData)->pTable[ulFieldIdx].pSampleSize);
		}
		else
		{
		    HX_ASSERT(m_uFieldSizeBits == 4);

		    if (ulSubFieldIdx == 0)
		    {
			// Get high 4 bits (assumes MSB occurs first in bit vector)
			return (((*(((Data8*) pData)->pTable[ulFieldIdx].pSampleSize)) >> 4) & 0x0F);
		    }
		    else
		    {
			// Get low 4 bits (assumes MSB occurs first in bit vector)
			return ((*(((Data8*) pData)->pTable[ulFieldIdx].pSampleSize)) & 0x0F);
		    }
		}
	    }
	}

	return 0;
    }

    virtual void SetBuffer(IHXBuffer *pBuffer)
    {
	CQT_stsz_Atom::SetBuffer(pBuffer);
        m_uFieldSizeBits = 0;
	m_ulSampleSize = 0;
    }

    virtual void SetMemPager(CMemPager* pMemPager)
    {
	CQT_stsz_Atom::SetMemPager(pMemPager);
        m_uFieldSizeBits = 0;
	m_ulSampleSize = 0;
    }    
};

/****************************************************************************
 *  stco Atom Class
 */
class CQT_stco_Atom : public CQTPagingAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pChunkOffset[4];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	TableEntry pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_stco_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) 
    : CQTPagingAtom(ulOffset, ulSize, pParent)
    , m_ulNumEntries(0)
    , m_ulEntrySizeMultiplier(1)    // used to avoid use of virtual method for co64 atom
    , m_ulEntrySizeOffset(0)	    // used to avoid use of virtual method for co64 atom
    {
	;
    }

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_stco; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	UINT8* pData = m_pData;
	HX_ASSERT(m_pData);

	if (m_ulNumEntries)
	{
	    return m_ulNumEntries;
	}

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pNumEntries, 
		sizeof(((Data*) pData)->pNumEntries),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    m_ulNumEntries = GetUL32(((Data*) pData)->pNumEntries);
	    return m_ulNumEntries;
	}

	return 0;
    }

    // This frequently called method we need to be able to inline for 
    // performance.  Do not make virtual.
    ULONG32 Get_ChunkOffset(ULONG32 ulEntryIdx)
    {
	UINT8* pData = m_pData;
	HX_ASSERT(m_pData);

	ulEntryIdx = ulEntryIdx * m_ulEntrySizeMultiplier + m_ulEntrySizeOffset;

#if !defined(QTCONFIG_NO_PAGING)
	if (m_pMemPager)
	{ 
	    m_lastStatus = m_pMemPager->PageIn(
		((Data*) pData)->pTable[ulEntryIdx].pChunkOffset, 
		sizeof(((Data*) pData)->pTable[ulEntryIdx].pChunkOffset),
		pData);
	}
#endif /* #if !defined(QTCONFIG_NO_PAGING) */

	if (m_lastStatus == HXR_OK)
	{
	    return GetUL32(((Data*) pData)->pTable[ulEntryIdx].pChunkOffset);
	}

	return 0;
    }

    virtual void SetBuffer(IHXBuffer *pBuffer)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetBuffer(pBuffer);
    }

    virtual void SetMemPager(CMemPager* pMemPager)
    {
	m_ulNumEntries = 0;
	CQTPagingAtom::SetMemPager(pMemPager);
    }

private:
    ULONG32 m_ulNumEntries;

protected:
    ULONG32 m_ulEntrySizeMultiplier;
    ULONG32 m_ulEntrySizeOffset;
};

/****************************************************************************
 *  stco Atom Class
 */
class CQT_co64_Atom : public CQT_stco_Atom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_co64_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) 
    : CQT_stco_Atom(ulOffset, ulSize, pParent) 
    {
	m_ulEntrySizeMultiplier = 2;
	m_ulEntrySizeOffset = 1;
    }

    /*
     *	Required Virtual Methods
     */
    virtual QTAtomType	GetType(void)	    { return QT_co64; }
};

/****************************************************************************
 *  edts Atom Class
 */
class CQT_edts_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_edts_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_edts; }
};

/****************************************************************************
 *  elst Atom Class
 */
#define QT_EMPTY_EDIT	0xFFFFFFFF

class CQT_elst_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pTrackDuration[4];
	UINT8 pMediaTime[4];
	UINT8 pMediaRate[4];
    } PACKING;

    struct Data
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	TableEntry pTable[1];
    } PACKING;

    struct TableEntry64
    {
	UINT8 pTrackDuration[8];
	UINT8 pMediaTime[8];
	UINT8 pMediaRate[4];
    } PACKING;

    struct Data64
    {
	UINT8 pVersion[1]; 
	UINT8 pFlags[3]; 
	UINT8 pNumEntries[4]; 
	TableEntry64 pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_elst_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_elst; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	HX_ASSERT(m_pData);
	return GetUL32(((Data*) m_pData)->pNumEntries);
    }

    ULONG32 Get_TrackDuration(ULONG32 ulEntryIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulEntryIdx < Get_NumEntries());
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUL32(((Data*) m_pData)->pTable[ulEntryIdx].pTrackDuration);
	}

	return INT64_TO_UINT32(GetUL64(((Data64*) m_pData)->pTable[ulEntryIdx].pTrackDuration));
    }

    ULONG32 Get_MediaTime(ULONG32 ulEntryIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulEntryIdx < Get_NumEntries());
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUL32(((Data*) m_pData)->pTable[ulEntryIdx].pMediaTime);
	}

	return INT64_TO_UINT32(GetUL64(((Data64*) m_pData)->pTable[ulEntryIdx].pTrackDuration));
    }

    ULONG32 Get_MediaRate(ULONG32 ulEntryIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulEntryIdx < Get_NumEntries());
	if ((*(((Data*) m_pData)->pVersion)) == 0)
	{
	    return GetUL32(((Data*) m_pData)->pTable[ulEntryIdx].pMediaRate);
	}

	return GetUL32(((Data64*) m_pData)->pTable[ulEntryIdx].pMediaRate);
    }
};

/****************************************************************************
 *  tref Atom Class
 */
class CQT_tref_Atom : public CQTAtom
{
public:
    /*
     *	Constructor/Destructor
     */
    CQT_tref_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_tref; }
};

/****************************************************************************
 *  hint Atom Class
 */
class CQT_hint_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pTrackID[4];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_hint_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_hint; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumEntries(void)
    {
	HX_ASSERT(m_pBuffer);
	return m_pBuffer->GetSize() / sizeof(TableEntry);
    }

    ULONG32 Get_TrackID(ULONG32 ulTableIdx)
    {
	HX_ASSERT(ulTableIdx < Get_NumEntries());
	return GetUL32(((TableEntry*) m_pData)[ulTableIdx].pTrackID);
    }
};

/****************************************************************************
 *  iods Atom Class
 */
class CQT_iods_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct Data
    {
	UINT8 pIODS[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_iods_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_iods; }
};

/****************************************************************************
 *  ftyp Atom Class
 */
class CQT_ftyp_Atom : public CQTAtom
{
public:
    /*
     *	Leaf Data Format
     */
    struct TableEntry
    {
	UINT8 pCompatibleBrand[4];
    } PACKING;

    struct Data
    {
	UINT8 pMajorBrand[4]; 
	UINT8 pMinorVersion[4]; 
	TableEntry pTable[1];
    } PACKING;

    /*
     *	Constructor/Destructor
     */
    CQT_ftyp_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_ftyp; }

    /*
     *	Data Access Methods
     */
    ULONG32 Get_NumCompatibleBrands(void)
    {
	HX_ASSERT(m_pData);

 	ULONG32 dataSize = GetDataSize() + sizeof(TableEntry);
	return (dataSize < sizeof(Data)) ? 0 :
                                 ((dataSize - sizeof(Data))/sizeof(TableEntry));
    }

    ULONG32 Get_MajorBrand(void)
    {
	HX_ASSERT(m_pData);
	return (m_pData) ? GetUL32(((Data*) m_pData)->pMajorBrand) : 0;
    }

    ULONG32 Get_MinorVersion(void)
    {
	HX_ASSERT(m_pData);
	return (m_pData) ? GetUL32(((Data*) m_pData)->pMinorVersion): 0;
    }

    ULONG32 Get_CompatibleBrand(ULONG32 ulEntryIdx)
    {
	HX_ASSERT(m_pData);
	HX_ASSERT(ulEntryIdx < Get_NumCompatibleBrands());
	return (m_pData) ? GetUL32(((Data*) m_pData)->pTable[ulEntryIdx].pCompatibleBrand) : 0;
    }
};

class CQT_rmra_Atom : public CQTAtom
{
public:
     /*	Constructor/Destructor
    */
    CQT_rmra_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_rmra; }
};

class CQT_rmda_Atom : public CQTAtom
{
public:
    /*	Constructor/Destructor
    */
    CQT_rmda_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return FALSE; }
    virtual QTAtomType	GetType(void)	    { return QT_rmda; }
};

class CQT_rdrf_Atom : public CQTAtom
{
public:
    /*	Constructor/Destructor
    */
    CQT_rdrf_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_rdrf; }
};

class CQT_rmdr_Atom : public CQTAtom
{
public:
	     /*	Constructor/Destructor
     */
    CQT_rmdr_Atom(ULONG32 ulOffset,
		  ULONG32 ulSize, 
		  CQTAtom *pParent) : CQTAtom(ulOffset,
					      ulSize,
					      pParent) {;}

    /*
     *	Required Virtual Methods
     */
    virtual HXBOOL	IsLeafType(void)    { return TRUE; }
    virtual QTAtomType	GetType(void)	    { return QT_rmdr; }
};
#ifdef QTCONFIG_SPEED_OVER_SIZE
#include "qtatoms_inline.h"
#endif	// QTCONFIG_SPEED_OVER_SIZE

#endif  // _QTATOMS_H_

