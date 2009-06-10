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

/****************************************************************************
 *  Includes
 */
#include "qtatoms.h"

#ifndef QTCONFIG_SPEED_OVER_SIZE
#include "qtatoms_inline.h"
#endif	// QTCONFIG_SPEED_OVER_SIZE


/****************************************************************************
 *  The atom factory
 */
CQTAtom* CreateQTAtom(QTAtomType AtomType,
		      ULONG32 ulOffset,
		      ULONG32 ulSize, 
		      CQTAtom *pParent,
		      ULONG32 ulAtomID,
		      UINT16 uChildCount)
{
    switch (AtomType)
    {
    case QT_trak:
	return new CQT_trak_Atom(ulOffset, ulSize, pParent);
    case QT_tkhd:
	return new CQT_tkhd_Atom(ulOffset, ulSize, pParent);
    case QT_mdia:
	return new CQT_mdia_Atom(ulOffset, ulSize, pParent);
    case QT_mdhd:
	return new CQT_mdhd_Atom(ulOffset, ulSize, pParent);
    case QT_hdlr:
	return new CQT_hdlr_Atom(ulOffset, ulSize, pParent);
    case QT_edts:
	return new CQT_edts_Atom(ulOffset, ulSize, pParent);
    case QT_elst:
	return new CQT_elst_Atom(ulOffset, ulSize, pParent);
    case QT_udta:
	return new CQT_udta_Atom(ulOffset, ulSize, pParent);
    case QT_name:
        return new CQT_name_Atom(ulOffset, ulSize, pParent);

#if defined(HELIX_FEATURE_3GPP_METAINFO) || defined(HELIX_FEATURE_SERVER)
    // 3GPP Asset Info atom types
    case QT_titl:
        return new CQT_titl_Atom(ulOffset, ulSize, pParent);
    case QT_auth:
        return new CQT_auth_Atom(ulOffset, ulSize, pParent);
    case QT_cprt:
        return new CQT_cprt_Atom(ulOffset, ulSize, pParent);
#ifdef HELIX_FEATURE_3GPP_METAINFO
    case QT_dscp:
        return new CQT_dscp_Atom(ulOffset, ulSize, pParent);
    case QT_perf:
        return new CQT_perf_Atom(ulOffset, ulSize, pParent);
    case QT_gnre:
        return new CQT_gnre_Atom(ulOffset, ulSize, pParent);
    case QT_rtng:
        return new CQT_rtng_Atom(ulOffset, ulSize, pParent);
    case QT_clsf:
        return new CQT_clsf_Atom(ulOffset, ulSize, pParent);
    case QT_kywd:
        return new CQT_kywd_Atom(ulOffset, ulSize, pParent);
    case QT_loci:
        return new CQT_loci_Atom(ulOffset, ulSize, pParent);
    case QT_albm:
        return new CQT_albm_Atom(ulOffset, ulSize, pParent);
    case QT_yrrc:
        return new CQT_yrrc_Atom(ulOffset, ulSize, pParent);
#endif // HELIX_FEATURE_3GPP_METAINFO
#endif // HELIX_FEATURE_3GPP_METAINFO || HELIX_FEATURE_SERVER
	
    case QT_minf:
	return new CQT_minf_Atom(ulOffset, ulSize, pParent);
    case QT_dinf:
	return new CQT_dinf_Atom(ulOffset, ulSize, pParent);
    case QT_dref:
	return new CQT_dref_Atom(ulOffset, ulSize, pParent);
    case QT_stbl:
	return new CQT_stbl_Atom(ulOffset, ulSize, pParent);
    case QT_stts:
	return new CQT_stts_Atom(ulOffset, ulSize, pParent);
    case QT_ctts:
	return new CQT_ctts_Atom(ulOffset, ulSize, pParent);
    case QT_stss:
	return new CQT_stss_Atom(ulOffset, ulSize, pParent);
    case QT_stsd:
	return new CQT_stsd_Atom(ulOffset, ulSize, pParent);
    case QT_stsz:
	return new CQT_stsz_Atom(ulOffset, ulSize, pParent);
    case QT_stz2:
	return new CQT_stz2_Atom(ulOffset, ulSize, pParent);
    case QT_stsc:
	return new CQT_stsc_Atom(ulOffset, ulSize, pParent);
    case QT_stco:
	return new CQT_stco_Atom(ulOffset, ulSize, pParent);
    case QT_co64:
	return new CQT_co64_Atom(ulOffset, ulSize, pParent);
    case QT_tsel:
	return new CQT_tsel_Atom(ulOffset, ulSize, pParent);
    case QT_moov:
	return new CQT_moov_Atom(ulOffset, ulSize, pParent);
    case QT_mvhd:
	return new CQT_mvhd_Atom(ulOffset, ulSize, pParent);
    case QT_ftyp:
	return new CQT_ftyp_Atom(ulOffset, ulSize, pParent);
#if !defined(QTCONFIG_3GPPCLIENT_ATOMSET_ONLY) && !defined(QTCONFIG_M4A_ATOMSET_ONLY)
    case QT_tref:
	return new CQT_tref_Atom(ulOffset, ulSize, pParent);
    case QT_hint:
	return new CQT_hint_Atom(ulOffset, ulSize, pParent);
    case QT_hinf:
	return new CQT_hinf_Atom(ulOffset, ulSize, pParent);
    case QT_hnti:
	return new CQT_hnti_Atom(ulOffset, ulSize, pParent);
    case QT_sdp:
	return new CQT_sdp_Atom(ulOffset, ulSize, pParent);
    case QT_trpy:
	return new CQT_trpy_Atom(ulOffset, ulSize, pParent);
    case QT_nump:
	return new CQT_nump_Atom(ulOffset, ulSize, pParent);
    case QT_tpyl:
	return new CQT_tpyl_Atom(ulOffset, ulSize, pParent);
    case QT_payt:
	return new CQT_payt_Atom(ulOffset, ulSize, pParent);
    case QT_rtp:
	return new CQT_rtp_Atom(ulOffset, ulSize, pParent);
    case QT_iods:
	return new CQT_iods_Atom(ulOffset, ulSize, pParent);
    case QT_rmra:
	return new CQT_rmra_Atom(ulOffset, ulSize, pParent);
    case QT_rmda:
	return new CQT_rmda_Atom(ulOffset, ulSize, pParent);
    case QT_rdrf:
	return new CQT_rdrf_Atom(ulOffset, ulSize, pParent);
    case QT_rmdr:
	return new CQT_rmdr_Atom(ulOffset, ulSize, pParent);
#endif	// QTCONFIG_3GPPCLIENT_ATOMSET_ONLY

    /***
// /-->> XXXEH- UNFINISHED code; much of this won't need to be handled here:
    case QT_tx3g:  // / TextSampleEntry
//	return new CQT_tx3g_Atom(ulOffset, ulSize, pParent);
    case QT_ftab:  // / FontTableBox
//	return new CQT_ftab_Atom(ulOffset, ulSize, pParent);
    case QT_styl:  // / TextStyleBox
//	return new CQT_styl_Atom(ulOffset, ulSize, pParent);
    case QT_hlit:  // / TextHighlightBox
//	return new CQT_hlit_Atom(ulOffset, ulSize, pParent);
    case QT_hclr:  // / TextHighlightColorBox
//	return new CQT_hclr_Atom(ulOffset, ulSize, pParent);
    case QT_krok:  // / TextKaraokeBox
//	return new CQT_krok_Atom(ulOffset, ulSize, pParent);
    case QT_href:  // / TextHyperTextBox
//	return new CQT_href_Atom(ulOffset, ulSize, pParent);
    case QT_tbox:  // / TextboxBox
//	return new CQT_tbox_Atom(ulOffset, ulSize, pParent);
    case QT_blnk:  // / BlinkBox
//	return new CQT_blnk_Atom(ulOffset, ulSize, pParent);
	HX_ASSERT(0);
	break;
// / <<--  end UNFINISHED code.
    ***/

    default:
	/* nothing to do */
	break;
    }

    return NULL;
}



CQT_stsd_Atom::TextSampleEntry::TextSampleEntry(ArrayEntry* pPackedEntry,
               UINT32 ulSzOfPackedEntry)
{
#if defined(NEED_DATA_IN_TEXTSAMPLEENTRY_FOR_MP4FF)
    UINT8* pUnpkdEntryData = (UINT8*)pPackedEntry;
    UINT32 ulByteCount = 0;
    UINT16 ui = 0;
    memcpy(this, pUnpkdEntryData, sizeof(ArrayEntry));
    if (ulSzOfPackedEntry < sizeof(TextSampleEntry))
    {
	goto done;
    }
    ulByteCount += sizeof(ArrayEntry);
    ulDisplayFlags = GetUL32(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 4;
    horizontalJustification = pUnpkdEntryData[ulByteCount];
    ulByteCount += 1;
    verticalJustification = pUnpkdEntryData[ulByteCount];
    ulByteCount += 1;
    memcpy(&uBackgroundColorRGBA, &pUnpkdEntryData[ulByteCount], 4); /* Flawfinder: ignore */
    ulByteCount += 4;
    defaultTextBox.top = (INT16)GetUI16(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 2;
    defaultTextBox.left = (INT16)GetUI16(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 2;
    defaultTextBox.bottom = (INT16)GetUI16(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 2;
    defaultTextBox.right = (INT16)GetUI16(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 2;
    defaultStyle.startChar =  (INT16)GetUI16(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 2;
    defaultStyle.endChar =  (INT16)GetUI16(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 2;
    defaultStyle.fontID =  (INT16)GetUI16(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 2;
    defaultStyle.faceStyleFlags = pUnpkdEntryData[ulByteCount];
    ulByteCount += 1;
    defaultStyle.fontSize = pUnpkdEntryData[ulByteCount];
    ulByteCount += 1;
    memcpy(&defaultStyle.textColorRGBA, &pUnpkdEntryData[ulByteCount], 4); /* Flawfinder: ignore */
    ulByteCount += 4;
    fontTable.ulFontTableSizeInBytes = GetUL32(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 4;
    memcpy(&fontTable.FTAB, &pUnpkdEntryData[ulByteCount], 4);
    HX_ASSERT(!strncmp((const char*)fontTable.FTAB, (const char*)"ftab", 4)); /* Flawfinder: ignore */
    ulByteCount += 4;
    fontTable.entryCount = GetUI16(&pUnpkdEntryData[ulByteCount]);
    ulByteCount += 2;
    fontTable.pFontEntries = new FontRecord[fontTable.entryCount];
    for (ui = 0; ui < fontTable.entryCount  &&
	    ulByteCount<=ulSzOfPackedEntry; ui++)
    {
	fontTable.pFontEntries[ui].fontID = GetUI16(&pUnpkdEntryData[ulByteCount]);
	ulByteCount += 2;
	fontTable.pFontEntries[ui].fontNameLength = pUnpkdEntryData[ulByteCount];
	ulByteCount += 1;
	fontTable.pFontEntries[ui].pFont =
		new UINT8[fontTable.pFontEntries[ui].fontNameLength+1];
	memcpy(fontTable.pFontEntries[ui].pFont, &pUnpkdEntryData[ulByteCount], /* Flawfinder: ignore */
		fontTable.pFontEntries[ui].fontNameLength);
	(fontTable.pFontEntries[ui].pFont)[fontTable.pFontEntries[ui].fontNameLength] = '\0';
	ulByteCount += fontTable.pFontEntries[ui].fontNameLength;
    }
    HX_ASSERT(ulSzOfPackedEntry == ulByteCount);

done:
    return;

#endif // /defined(NEED_DATA_IN_TEXTSAMPLEENTRY_FOR_MP4FF)
}


CQT_stsd_Atom::FontTableBox::~FontTableBox()
{
#if defined(NEED_DATA_IN_TEXTSAMPLEENTRY_FOR_MP4FF)
    if (pFontEntries  &&  entryCount)
    {
	for (UINT16 ui = entryCount-1; ui; ui--)
	{
	    if (pFontEntries[ui].pFont)
	    {
		delete [] (pFontEntries[ui].pFont);
	    }
	}
	delete pFontEntries;
	pFontEntries = NULL;
    }
#endif // /defined(NEED_DATA_IN_TEXTSAMPLEENTRY_FOR_MP4FF)
}


void ExtractLanguageEncoding(UINT8* pPadAndLang, char out[3])
{
    HX_ASSERT(pPadAndLang);
    if(pPadAndLang)
    {
        UINT16 uLang = CQTAtom::GetUI16(pPadAndLang);
        out[0] = char(UINT8((uLang >> 10) & 0x001F) + 0x60);
        out[1] = char(UINT8((uLang >> 5) & 0x001F) + 0x60);
        out[2] = char(UINT8((uLang >> 0) & 0x001F) + 0x60);
    }
}
