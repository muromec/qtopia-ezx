/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxlang.cpp,v 1.5 2004/07/09 18:43:11 hubbe Exp $
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
 
#include "hxtypes.h"
#include "hxlang.h"
#include "hlxclib/string.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


const HXLangMapping CHXLang::Languages[] =
	{
#ifdef _WINDOWS        
		{"af",MAKELANGID(LANG_AFRIKAANS,SUBLANG_NEUTRAL)},
		{"sq",MAKELANGID(LANG_ALBANIAN,SUBLANG_NEUTRAL)},
		{"ar",MAKELANGID(LANG_ARABIC,SUBLANG_NEUTRAL)},
		{"ar-DZ",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_ALGERIA)},
		{"ar-BH",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_BAHRAIN)},
		{"ar-EG",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_EGYPT)},
		{"ar-JO",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_JORDAN)},
		{"ar-KW",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_KUWAIT)},
		{"ar-LB",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_LEBANON)},
		{"ar-MA",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_MOROCCO)},
		{"ar-OM",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_OMAN)},
		{"ar-QA",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_QATAR)},
		{"ar-SA",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_SAUDI_ARABIA)},
		{"ar-TN",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_TUNISIA)},
		{"ar-AE",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_UAE)},
		{"ar-YE",MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_YEMEN)},
		{"hy",MAKELANGID(LANG_ARMENIAN,SUBLANG_NEUTRAL)},
		{"as",MAKELANGID(LANG_ASSAMESE,SUBLANG_NEUTRAL)},
		{"az",MAKELANGID(LANG_AZERI,SUBLANG_AZERI_LATIN)},
		{"az",MAKELANGID(LANG_AZERI,SUBLANG_AZERI_CYRILLIC)},
		{"eu",MAKELANGID(LANG_BASQUE,SUBLANG_NEUTRAL)},
		{"be",MAKELANGID(LANG_BELARUSIAN,SUBLANG_NEUTRAL)},
		{"bn",MAKELANGID(LANG_BENGALI,SUBLANG_NEUTRAL)},
		{"bg",MAKELANGID(LANG_BULGARIAN,SUBLANG_NEUTRAL)},
		{"ca",MAKELANGID(LANG_CATALAN,SUBLANG_NEUTRAL)},
		{"cs",MAKELANGID(LANG_CZECH,SUBLANG_NEUTRAL)},
		{"da",MAKELANGID(LANG_DANISH,SUBLANG_NEUTRAL)},
		{"de-AT",MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_AUSTRIAN)},
		{"de-CH",MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_SWISS)},
		{"de-LI",MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_LIECHTENSTEIN)},
		{"de-LU",MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_LUXEMBOURG)},
		{"de",MAKELANGID(LANG_GERMAN,SUBLANG_NEUTRAL)},
		{"el",MAKELANGID(LANG_GREEK,SUBLANG_NEUTRAL)},
		{"en-AU",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_AUS)},
		{"en-BZ",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_BELIZE)},
		{"en-CA",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_CAN)},
		{"en",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_CARIBBEAN)},
		{"en-GB",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_UK)},
		{"en-IE",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_EIRE)},
		{"en-JM",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_JAMAICA)},
		{"en-NZ",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_NZ)},
		{"en-TT",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_TRINIDAD)},
#endif /* _WINDOWS */
		{"en-US",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US)},
#ifdef _WINDOWS        
		{"en-ZA",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_SOUTH_AFRICA)},
		{"en-PH",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_PHILIPPINES)},
		{"en-ZW",MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_ZIMBABWE)},
		{"en",MAKELANGID(LANG_ENGLISH,SUBLANG_NEUTRAL)},
		{"fa",MAKELANGID(LANG_FARSI,SUBLANG_NEUTRAL)},
		{"es-AR",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_ARGENTINA)},
		{"es-BO",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_BOLIVIA  )},
		{"es-CL",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_CHILE    )},
		{"es-CO",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_COLOMBIA )},
		{"es-CR",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_COSTA_RICA)},
		{"es-DO",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_DOMINICAN_REPUBLIC)},
		{"es-EC",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_ECUADOR  )},
		{"es-GT",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_GUATEMALA  )},
		{"es-HN",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_HONDURAS )},
		{"es-MX",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_MEXICAN    )},
		{"es-NI",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_NICARAGUA)},
		{"es-PA",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_PANAMA     )},
		{"es-PE",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_PERU     )},
		{"es-PR",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_PUERTO_RICO)},
		{"es-PY",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_PARAGUAY )},
		{"es-SV",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_EL_SALVADOR)},
		{"es-UY",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_URUGUAY  )},
		{"es-VE",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_VENEZUELA)},
		{"es",MAKELANGID(LANG_SPANISH,SUBLANG_NEUTRAL)},
		{"et",MAKELANGID(LANG_ESTONIAN,SUBLANG_NEUTRAL)},
		{"fi",MAKELANGID(LANG_FINNISH,SUBLANG_NEUTRAL)},
		{"fo",MAKELANGID(LANG_FAEROESE,SUBLANG_NEUTRAL)},
		{"fr-BE",MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_BELGIAN)},
		{"fr-CA",MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_CANADIAN)},
		{"fr-CH",MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_SWISS)},
		{"fr-LU",MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_LUXEMBOURG)},
		{"fr-MC",MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_MONACO)},
		{"fr",MAKELANGID(LANG_FRENCH,SUBLANG_NEUTRAL)},
		{"mk",MAKELANGID(LANG_MACEDONIAN,SUBLANG_NEUTRAL)},
		{"ka",MAKELANGID(LANG_GEORGIAN,SUBLANG_NEUTRAL)},
		{"gu",MAKELANGID(LANG_GUJARATI,SUBLANG_NEUTRAL)},
		{"he",MAKELANGID(LANG_HEBREW,SUBLANG_NEUTRAL)},
		{"hi",MAKELANGID(LANG_HINDI,SUBLANG_NEUTRAL)},
		{"hr",MAKELANGID(LANG_CROATIAN,SUBLANG_NEUTRAL)},
		{"hu",MAKELANGID(LANG_HUNGARIAN,SUBLANG_NEUTRAL)},
		{"id",MAKELANGID(LANG_INDONESIAN,SUBLANG_NEUTRAL)},
		{"is",MAKELANGID(LANG_ICELANDIC,SUBLANG_NEUTRAL)},
		{"it-CH",MAKELANGID(LANG_ITALIAN,SUBLANG_ITALIAN_SWISS)},
		{"it",MAKELANGID(LANG_ITALIAN,SUBLANG_NEUTRAL)},
		{"ja",MAKELANGID(LANG_JAPANESE,SUBLANG_NEUTRAL)},
		{"ko",MAKELANGID(LANG_KOREAN,SUBLANG_NEUTRAL)},
		{"kn",MAKELANGID(LANG_KANNADA,SUBLANG_NEUTRAL)},
		{"kk",MAKELANGID(LANG_KAZAK,SUBLANG_NEUTRAL)},
		{"lt",MAKELANGID(LANG_LITHUANIAN,SUBLANG_NEUTRAL)},
		{"lv",MAKELANGID(LANG_LATVIAN,SUBLANG_NEUTRAL)},
		{"ms",MAKELANGID(LANG_MALAY,SUBLANG_MALAY_BRUNEI_DARUSSALAM)},
		{"ms",MAKELANGID(LANG_MALAY,SUBLANG_MALAY_MALAYSIA)},
		{"ml",MAKELANGID(LANG_MALAYALAM,SUBLANG_NEUTRAL)},
		{"mr",MAKELANGID(LANG_MARATHI,SUBLANG_NEUTRAL)},
		{"ne",MAKELANGID(LANG_NEPALI,SUBLANG_NEUTRAL)},
		{"nl-BE",MAKELANGID(LANG_DUTCH,SUBLANG_DUTCH_BELGIAN)},
		{"nl",MAKELANGID(LANG_DUTCH,SUBLANG_NEUTRAL)},
		{"no",MAKELANGID(LANG_NORWEGIAN,SUBLANG_NEUTRAL)},
		{"nb-no",MAKELANGID(LANG_NORWEGIAN,SUBLANG_NORWEGIAN_BOKMAL)},
		{"nn-no",MAKELANGID(LANG_NORWEGIAN,SUBLANG_NORWEGIAN_NYNORSK)},
		{"or",MAKELANGID(LANG_ORIYA,SUBLANG_NEUTRAL)},
		{"pa",MAKELANGID(LANG_PUNJABI,SUBLANG_NEUTRAL)},
		{"sa",MAKELANGID(LANG_SANSKRIT,SUBLANG_NEUTRAL)},
		{"pl",MAKELANGID(LANG_POLISH,SUBLANG_NEUTRAL)},
		{"pt-BR",MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE_BRAZILIAN)},
		{"pt",MAKELANGID(LANG_PORTUGUESE,SUBLANG_NEUTRAL)},
		{"ro",MAKELANGID(LANG_ROMANIAN,SUBLANG_NEUTRAL)},
		{"ru",MAKELANGID(LANG_RUSSIAN,SUBLANG_NEUTRAL)},
		{"sk",MAKELANGID(LANG_SLOVAK,SUBLANG_NEUTRAL)},
		{"sl",MAKELANGID(LANG_SLOVENIAN,SUBLANG_NEUTRAL)},
		{"sr",MAKELANGID(LANG_SERBIAN,SUBLANG_SERBIAN_CYRILLIC)},
		{"sr",MAKELANGID(LANG_SERBIAN,SUBLANG_SERBO_CROATIAN_LATIN)},
		{"sw",MAKELANGID(LANG_SWAHILI,SUBLANG_NEUTRAL)},
		{"sv-FI",MAKELANGID(LANG_SWEDISH,SUBLANG_SWEDISH_FINLAND)},
		{"sv",MAKELANGID(LANG_SWEDISH,SUBLANG_NEUTRAL)},
		{"ta",MAKELANGID(LANG_TAMIL,SUBLANG_NEUTRAL)},
		{"tt",MAKELANGID(LANG_TATAR,SUBLANG_NEUTRAL)},
		{"te",MAKELANGID(LANG_TELUGU,SUBLANG_NEUTRAL)},
		{"th",MAKELANGID(LANG_THAI,SUBLANG_NEUTRAL)},
		{"tr",MAKELANGID(LANG_TURKISH,SUBLANG_NEUTRAL)},
		{"uk",MAKELANGID(LANG_UKRAINIAN,SUBLANG_NEUTRAL)},
		{"ur",MAKELANGID(LANG_URDU,SUBLANG_NEUTRAL)},
		{"uz",MAKELANGID(LANG_UZBEK,SUBLANG_UZBEK_CYRILLIC)},
		{"uz",MAKELANGID(LANG_UZBEK,SUBLANG_UZBEK_LATIN)},
		{"vi",MAKELANGID(LANG_VIETNAMESE,SUBLANG_NEUTRAL)},
		{"zh-CN",MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED)},
		{"zh-HK",MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_HONGKONG)},
		{"zh-SG",MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SINGAPORE)},
		{"zh-MO",MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_MACAU)},
		{"zh-TW",MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL)},
		{"zh",MAKELANGID(LANG_CHINESE,SUBLANG_NEUTRAL)}
#endif /* _WINDOWS */        
	};


/////////////////////////////////////////////////////////////////
// Given a languageID (defined in olenls.h), this returns the
// index of the exact language, or -1 if not found.
INT16 CHXLang::FindExact(UINT16 nLangID)
{
	INT16 nIdx=0;
	for (nIdx=0; nIdx<GetCount(); nIdx++ )
	{
		if (Languages[nIdx].nLangID == nLangID)
			break;
	}

	if (nIdx==GetCount())
		nIdx=-1;
		
	return(nIdx);
}


/////////////////////////////////////////////////////////////////
// Given a languageID (defined in olenls.h), this returns the
// index of the exact language, or -1 if not found.
INT16 CHXLang::FindExact(const char* szISO639)
{
	if (szISO639==NULL) return(-1);
	
	INT16 nIdx=0;
	for (nIdx=0; nIdx<GetCount(); nIdx++ )
	{
		if (!stricmp(Languages[nIdx].szISO639,szISO639))
			break;
	}

	if (nIdx==GetCount())
		nIdx=-1;
		
	return(nIdx);
}


/////////////////////////////////////////////////////////////////
// Given a languageID (defined in olenls.h), this returns the
// index of the closest matching language, or -1
INT16 CHXLang::FindClosest(UINT16 nLangID)
{
	INT16 nIdx = FindExact(nLangID);

	// didn't find exact match?
	if (nIdx==-1)
	{
// On non windows system we always just return english for now. 
#ifdef _WINDOWS        
		UINT16 nNeutral = MAKELANGID(PRIMARYLANGID(nLangID), SUBLANG_NEUTRAL);
#else
		UINT16 nNeutral = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
#endif        
		
		if (nNeutral != nLangID)
		{
			nIdx = FindExact(nLangID);
		}
	}
	
	return(nIdx);
}


/////////////////////////////////////////////////////////////////
// Given an index from 0 to GetCount()-1 ...
const char*	CHXLang::GetISO639(INT16 nIndex)
{
	if (IsValidIndex(nIndex))
		return(Languages[nIndex].szISO639);
	
	return(NULL);
}


/////////////////////////////////////////////////////////////////
// Given an index from 0 to GetCount()-1 ...
UINT16 CHXLang::GetLangID(INT16 nIndex)
{
	if (IsValidIndex(nIndex))
		return(Languages[nIndex].nLangID);
	
	return(0);
}


/////////////////////////////////////////////////////////////////
// Returns the # of langauges in the static database
INT16 CHXLang::GetCount()
{
	return(sizeof(Languages)/sizeof(HXLangMapping));
	
}
	
