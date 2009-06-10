
/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxlang.h,v 1.7 2007/07/06 21:58:04 jfinnecy Exp $
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

 /***************************************************************************
 
 Abstraction:
 Contains a listing of ISO-639 compliant langauge tags and their
 equivalent LanguageID. Language & sub-language contants are defined
 in "olenls.h", and this file maps langIDs to ISO-639 compliant tags.

 THIS IS A STATIC LIBRARY, no need to instantiate objects from this
 class.

 This class defines a static array mapping ISO-639 language tags
 language IDs. LangIDs are a 16bit number composing of a primary
 langauge and a sub-language. For instance, the lang French-Canadian
 would have a primary language of "French" and a sub-lang of
 "Canadian". See "olenls.h" for more details on langIDs.

 **************************************************************************
 
 To use this class, if you have a languageID and want to get it's
 ISO-639 tag:

     LANGID SomeLangID = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH_GIBBERISH)
     UINT16 nIndex = CHXLang::FindClosest( SomeLangID )

 Now, since "LANG_FRENCH/SUBLANG_FRENCH_GIBBERISH" doesn't exist,
 the FindClosest() function will return the closest matching language, 
 mainly, French (LANG_FRENCH, SUBLANG_NEUTRAL).

 If French didn't exist, then the function returns -1 as the index. If
 we used "FindExact()" in the above example, we'd get -1 returned
 since "LANG_FRENCH/SUBLANG_FRENCH_GIBBERISH" doesn't exist.

 GetCount() returns the # of languages in the database.
 A langauge index goes from 0 to GetCount()-1.

 Once we have the index, we can get the ISO-639 tag using:

     const char* szISOtag = CHXLang::GetISO639( nIndex );
 
 NULL is returned if the index is out-of-range. You can iterate all
 the langauges in the database with:

     for(UINT16 nIdx=0; nIdx < CHXLang::GetCount(); nIdx++)
     {
         printf("%d = %s \n", nIdx, CHXLang::GetISO639(nIdx);
     }

 */

#ifndef _RPLANG_H_
#define _RPLANG_H_


#ifdef _WINDOWS
#   include "hlxclib/windows.h"
#endif

#include "hxtypes.h"

#ifndef LANG_ENGLISH
#define LANG_ENGLISH                     0x09
#endif

#ifndef SUBLANG_ENGLISH_US
#define SUBLANG_ENGLISH_US               0x01
#endif

//We need to define a MAKELCID and MAKELANGID for non windows systems.
#ifndef MAKELCID
#define MAKELCID(a) ((UINT32)(((UINT8)(a)) | (((UINT32)((UINT8)(0))) << 16)))
#endif
#ifndef MAKELANGID
#define MAKELANGID(a, b) ((((UINT8)(b))<<10) | (UINT8)(a))

#endif


#ifndef LANG_ARMENIAN
#define LANG_ARMENIAN                    0x2b
#endif

#ifndef LANG_ASSAMESE
#define LANG_ASSAMESE                    0x4d
#endif

#ifndef LANG_AZERI
#define LANG_AZERI                       0x2c
#endif

#ifndef SUBLANG_AZERI_LATIN
#define SUBLANG_AZERI_LATIN              0x01    // Azeri (Latin)
#endif

#ifndef SUBLANG_AZERI_CYRILLIC
#define SUBLANG_AZERI_CYRILLIC           0x02    // Azeri (Cyrillic)
#endif

#ifndef LANG_BASQUE
#define LANG_BASQUE                      0x2d
#endif

#ifndef LANG_BELARUSIAN
#define LANG_BELARUSIAN                  0x23
#endif

#ifndef LANG_BENGALI
#define LANG_BENGALI                     0x45
#endif

#ifndef SUBLANG_CHINESE_MACAU
#define SUBLANG_CHINESE_MACAU            0x05    // Chinese (Macau)
#endif

#ifndef SUBLANG_ENGLISH_PHILIPPINES
#define SUBLANG_ENGLISH_PHILIPPINES      0x0d    // English (Philippines)
#endif

#ifndef SUBLANG_ENGLISH_ZIMBABWE
#define SUBLANG_ENGLISH_ZIMBABWE         0x0c    // English (Zimbabwe)
#endif

#ifndef LANG_FARSI
#define LANG_FARSI                       0x29
#endif

#ifndef SUBLANG_FRENCH_MONACO
#define SUBLANG_FRENCH_MONACO            0x06    // French (Monaco)
#endif

#ifndef LANG_MACEDONIAN
#define LANG_MACEDONIAN                  0x2f
#endif

#ifndef LANG_GEORGIAN
#define LANG_GEORGIAN                    0x37
#endif

#ifndef LANG_GUJARATI
#define LANG_GUJARATI                    0x47
#endif

#ifndef LANG_HINDI
#define LANG_HINDI                       0x39
#endif

#ifndef LANG_KANNADA
#define LANG_KANNADA                     0x4b
#endif

#ifndef LANG_KAZAK
#define LANG_KAZAK                       0x3f
#endif

#ifndef LANG_MALAY
#define LANG_MALAY                       0x3e
#endif

#ifndef LANG_MALAYALAM
#define LANG_MALAYALAM                   0x4c
#endif

#ifndef SUBLANG_MALAY_MALAYSIA
#define SUBLANG_MALAY_MALAYSIA           0x01    // Malay (Malaysia)
#endif

#ifndef SUBLANG_MALAY_BRUNEI_DARUSSALAM
#define SUBLANG_MALAY_BRUNEI_DARUSSALAM  0x02    // Malay (Brunei Darussalam)
#endif

#ifndef LANG_MARATHI
#define LANG_MARATHI                     0x4e
#endif

#ifndef LANG_NEPALI
#define LANG_NEPALI                      0x61
#endif

#ifndef LANG_ORIYA
#define LANG_ORIYA                       0x48
#endif

#ifndef LANG_PUNJABI
#define LANG_PUNJABI                     0x46
#endif

#ifndef LANG_SANSKRIT
#define LANG_SANSKRIT                    0x4f
#endif

#ifndef LANG_SWAHILI
#define LANG_SWAHILI                     0x41
#endif

#ifndef LANG_TAMIL
#define LANG_TAMIL                       0x49
#endif

#ifndef LANG_TATAR
#define LANG_TATAR                       0x44
#endif

#ifndef LANG_TELUGU
#define LANG_TELUGU                      0x4a
#endif

#ifndef LANG_UZBEK
#define LANG_UZBEK                       0x43
#endif

#ifndef SUBLANG_UZBEK_LATIN
#define SUBLANG_UZBEK_LATIN              0x01    // Uzbek (Latin)
#endif

#ifndef SUBLANG_UZBEK_CYRILLIC
#define SUBLANG_UZBEK_CYRILLIC           0x02    // Uzbek (Cyrillic)
#endif

#ifndef SUBLANG_SERBO_CROATIAN_LATIN
#define SUBLANG_SERBO_CROATIAN_LATIN     0x01    /* Croato-Serbian (Latin) */
#endif

#ifndef LANG_URDU
#define LANG_URDU                        0x20
#endif

typedef struct _HXLangMapping
            {
                const char* szISO639;
                UINT16      nLangID;
            } HXLangMapping;

        
class CHXLang
{
public:
    // Given a languageID (defined in olenls.h), this returns the
    // index of the exact language, or -1 if not found.
    static INT16    FindExact(UINT16 nLangID);
    static INT16    FindExact(const char* szISO639);

    // Given a languageID (defined in olenls.h), this returns the
    // index of the closest matching language, or -1
    static INT16    FindClosest(UINT16 nLangID);

    // Given an index from 0 to GetCount()-1 ...
    static const char*  GetISO639(INT16 nIndex);
    static UINT16   GetLangID(INT16 nIndex);

    // Returns the # of langauges in the static database
    static INT16    GetCount();

    static HXBOOL IsValidIndex(INT16 nIdx) { return(nIdx>=0 && nIdx<GetCount()); }
    
private:

    static const HXLangMapping Languages[];

};

#endif


/*
 
"Afrikaans",                MAKELANGID(LANG_AFRIKAANS,SUBLANG_NEUTRAL)
"Albanian",                 MAKELANGID(LANG_ALBANIAN,SUBLANG_NEUTRAL)
"Arabic",                   MAKELANGID(LANG_ARABIC,SUBLANG_NEUTRAL)
"Arabic-Algeria",           MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_ALGERIA)
"Arabic-Bahrain",           MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_BAHRAIN)
"Arabic-Egypt",             MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_EGYPT)
"Arabic-Iraq",              MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_IRAQ)
"Arabic-Jordan",            MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_JORDAN)                
"Arabic-Kuwait",            MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_KUWAIT)                
"Arabic-Lebanon",           MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_LEBANON)               
"Arabic-Libya",             MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_LIBYA)                 
"Arabic-Morocco",           MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_MOROCCO)               
"Arabic-Oman",              MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_OMAN)                  
"Arabic-Qatar",             MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_QATAR)                 
"Arabic-Saudi Arabia",      MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_SAUDI_ARABIA)
"Arabic-Syria",             MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_SYRIA)                 
"Arabic-Tunisia",           MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_TUNISIA)               
"Arabic-UAE",               MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_UAE)                   
"Arabic-Yemen",             MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_YEMEN)                 
"Basque",                   MAKELANGID(LANG_BASQUE,SUBLANG_NEUTRAL)                      
"Bulgarian",                MAKELANGID(LANG_BULGARIAN,SUBLANG_NEUTRAL)                   
"Catalan",                  MAKELANGID(LANG_CATALAN,SUBLANG_NEUTRAL)
"Chinese",                     MAKELANGID(LANG_CHINESE,SUBLANG_NEUTRAL)
"Chinese-China",            MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED)          
"Chinese-Hong Kong",        MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_HONGKONG)  
"Chinese-Singapore",        MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SINGAPORE)           
"Chinese-Taiwan",           MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL)         
"Croatian",                    MAKELANGID(LANG_CROATIAN,SUBLANG_NEUTRAL)
"Czech",                       MAKELANGID(LANG_CZECH,SUBLANG_NEUTRAL)
"Danish",                      MAKELANGID(LANG_DANISH,SUBLANG_NEUTRAL)                      
"Dutch",                       MAKELANGID(LANG_DUTCH,SUBLANG_NEUTRAL)                       
"Dutch-Belgian",            MAKELANGID(LANG_DUTCH,SUBLANG_DUTCH_BELGIAN)                 
"English",                     MAKELANGID(LANG_ENGLISH,SUBLANG_NEUTRAL)                     
"English-Australia",        MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_AUS)                 
"English-Belize",           MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_BELIZE)              
"English-Canada",           MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_CAN)                 
"English-Caribbean",        MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_CARIBBEAN)           
"English-Irish",            MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_EIRE)                
"English-Jamaica",          MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_JAMAICA)             
"English-New Zealand",      MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_NZ)        
"English-South Africa",     MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_SOUTH_AFRICA)
"English-Trinidad",         MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_TRINIDAD)            
"English-UK",               MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_UK)                  
"English-US",               MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US)                  
"Estonian",                    MAKELANGID(LANG_ESTONIAN,SUBLANG_NEUTRAL)                    
"Faroese",                     MAKELANGID(LANG_FAEROESE,SUBLANG_NEUTRAL)                    
"Finnish",                     MAKELANGID(LANG_FINNISH,SUBLANG_NEUTRAL)                     
"French",                  MAKELANGID(LANG_FRENCH,SUBLANG_NEUTRAL)                      
"French-Belgian",           MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_BELGIAN)               
"French-Canadian",          MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_CANADIAN)              
"French-Luxembourg",    MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_LUXEMBOURG)            
"French-Swiss",         MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_SWISS)                 
"German",                  MAKELANGID(LANG_GERMAN,SUBLANG_NEUTRAL)                      
"German-Austrian",      MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_AUSTRIAN)              
"German-Liechtenstein", MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_LIECHTENSTEIN)         
"German-Luxembourg",    MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_LUXEMBOURG)            
"German-Swiss",         MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN_SWISS)                 
"Greek",                   MAKELANGID(LANG_GREEK,SUBLANG_NEUTRAL)                       
"Hebrew",                  MAKELANGID(LANG_HEBREW,SUBLANG_NEUTRAL)                      
"Hungarian",               MAKELANGID(LANG_HUNGARIAN,SUBLANG_NEUTRAL)                   
"Icelandic",               MAKELANGID(LANG_ICELANDIC,SUBLANG_NEUTRAL)                   
"Indonesian",              MAKELANGID(LANG_INDONESIAN,SUBLANG_NEUTRAL)                  
"Italian",                 MAKELANGID(LANG_ITALIAN,SUBLANG_NEUTRAL)                     
"Italian-Swiss",        MAKELANGID(LANG_ITALIAN,SUBLANG_ITALIAN_SWISS)               
"Japanese",                MAKELANGID(LANG_JAPANESE,SUBLANG_NEUTRAL)                    
"Korean",                  MAKELANGID(LANG_KOREAN,SUBLANG_NEUTRAL)                      
"Korean-Johab",         MAKELANGID(LANG_KOREAN,SUBLANG_KOREAN_JOHAB)                 
"Latvian",                 MAKELANGID(LANG_LATVIAN,SUBLANG_NEUTRAL)                     
"Lithuanian",              MAKELANGID(LANG_LITHUANIAN,SUBLANG_NEUTRAL)                  
"Norwegian",               MAKELANGID(LANG_NORWEGIAN,SUBLANG_NEUTRAL)                   
"Polish",                  MAKELANGID(LANG_POLISH,SUBLANG_NEUTRAL)                      
"Portuguese",              MAKELANGID(LANG_PORTUGUESE,SUBLANG_NEUTRAL)                  
"Portuguese-Brazilian", MAKELANGID(LANG_PORTUGUESE,SUBLANG_PORTUGUESE_BRAZILIAN)     
"Romanian",                MAKELANGID(LANG_ROMANIAN,SUBLANG_NEUTRAL)                    
"Russian",                 MAKELANGID(LANG_RUSSIAN,SUBLANG_NEUTRAL)                     
"Serbian",                 MAKELANGID(LANG_SERBIAN,SUBLANG_SERBIAN_CYRILLIC)                     
"Slovak",                  MAKELANGID(LANG_SLOVAK,SUBLANG_NEUTRAL)                      
"Slovenian",               MAKELANGID(LANG_SLOVENIAN,SUBLANG_NEUTRAL)                   
"Spanish",                 MAKELANGID(LANG_SPANISH,SUBLANG_NEUTRAL)                     
"Spanish-Argentina",    MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_ARGENTINA)                                                  
"Spanish-Bolivia",      MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_BOLIVIA  )                                                  
"Spanish-Chile",        MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_CHILE    )                                                  
"Spanish-Colombia",     MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_COLOMBIA )                                                  
"Spanish-Costa Rica",   MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_COSTA_RICA)
"Spanish-Dominican Republic",MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_DOMINICAN_R)
"Spanish-Ecuador",      MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_ECUADOR  )                                                  
"Spanish-El Salvador",  MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_EL_SALVADOR)       
"Spanish-Guatemala",    MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_GUATEMALA  )                                                  
"Spanish-Honduras",     MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_HONDURAS )                                                  
"Spanish-Mexican",      MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_MEXICAN    )                                                  
"Spanish-Modern",       MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_MODERN     )                                                  
"Spanish-Nicaragua",    MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_NICARAGUA)                                                  
"Spanish-Panama",       MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_PANAMA     )                                                  
"Spanish-Paraguay",     MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_PARAGUAY )                                                  
"Spanish-Peru",         MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_PERU     )                                                  
"Spanish-Puerto Rico",  MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_PUERTO_RICO)       
"Spanish-Uruguay",      MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_URUGUAY  )                                                  
"Spanish-Venezvela",    MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH_VENEZUELA)                                                  
"Swedish",                 MAKELANGID(LANG_SWEDISH,SUBLANG_NEUTRAL)                     
"Swedish-Finland",      MAKELANGID(LANG_SWEDISH,SUBLANG_SWEDISH_FINLAND)
"Thai",                    MAKELANGID(LANG_THAI,SUBLANG_NEUTRAL)                        
"Turkish",                 MAKELANGID(LANG_TURKISH,SUBLANG_NEUTRAL)                     
"Ukrainian",               MAKELANGID(LANG_UKRAINIAN,SUBLANG_NEUTRAL)                   
"Vietnamese",              MAKELANGID(LANG_VIETNAMESE,SUBLANG_NEUTRAL)                  



*/



// These are known languages that we don't have lang ID's for, so for now
// they aren't included in our master list.
// "mk","Macedonian",                                                        
// "mg","Malagasy",                                                          
// "ms","Malay",                                                             
// "ml","Maltese",                                                           
// "mi","Maori",                                                             
// "mr","Marathi",                                                           
// "mo","Moldavian",                                                         
// "mn","Mongolian",                                                         
// "na","Nauru",                                                             
// "ne","Nepali",                                                            
// "or","Oriya",                                                             
// "om","Oromo",                                                             
// "pa","Panjabi",                                                           
// "fa","Persian",                                                           
// "ps","Pushto",                                                            
// "qu","Quechua",                                                           
// "rn","Rundi",                                                             
// "sm","Samoan",                                                            
// "sg","Sango",                                                             
// "sa","Sanskrit",                                                          
// "sn","Shona",                                                             
// "sd","Sindhi",                                                            
// "si","Singhalese",                                                        
// "ss","Siswant",                                                           
// "so","Somali",                                                            
// "st","Sotho, Southern",                                          
// 





 




