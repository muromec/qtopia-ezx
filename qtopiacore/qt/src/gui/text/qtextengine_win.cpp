/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QT_NO_THREAD
#  include "private/qmutexpool_p.h"
#endif
#include "qlibrary.h"

// these defines are from usp10.h
typedef void *SCRIPT_CACHE;
typedef struct tag_SCRIPT_CONTROL {
    DWORD   uDefaultLanguage    :16;
    DWORD   fContextDigits      :1;
    DWORD   fInvertPreBoundDir  :1;
    DWORD   fInvertPostBoundDir :1;
    DWORD   fLinkStringBefore   :1;
    DWORD   fLinkStringAfter    :1;
    DWORD   fNeutralOverride    :1;
    DWORD   fNumericOverride    :1;
    DWORD   fLegacyBidiClass    :1;
    DWORD   fReserved           :8;
} SCRIPT_CONTROL;

typedef struct tag_SCRIPT_STATE {
    WORD    uBidiLevel         :5;
    WORD    fOverrideDirection :1;
    WORD    fInhibitSymSwap    :1;
    WORD    fCharShape         :1;
    WORD    fDigitSubstitute   :1;
    WORD    fInhibitLigate     :1;
    WORD    fDisplayZWG        :1;
    WORD    fArabicNumContext  :1;
    WORD    fGcpClusters       :1;
    WORD    fReserved          :1;
    WORD    fEngineReserved    :2;
} SCRIPT_STATE;

typedef struct tag_SCRIPT_ITEM {
    int              iCharPos;
    QScriptAnalysis  a;
} SCRIPT_ITEM;

typedef QGlyphLayout::Attributes SCRIPT_VISATTR;

typedef struct tagGOFFSET {
  LONG  du;
  LONG  dv;
} GOFFSET;

#define USP_E_SCRIPT_NOT_IN_FONT   \
        MAKE_HRESULT(SEVERITY_ERROR,FACILITY_ITF,0x200)    // Script doesn't exist in font

typedef struct {
  DWORD   langid              :16;
  DWORD   fNumeric            :1;
  DWORD   fComplex            :1;
  DWORD   fNeedsWordBreaking  :1;
  DWORD   fNeedsCaretInfo     :1;
  DWORD   bCharSet            :8;
  DWORD   fControl            :1;
  DWORD   fPrivateUseArea     :1;
  DWORD   fNeedsCharacterJustify :1;
  DWORD   fInvalidGlyph       :1;
  DWORD   fInvalidLogAttr     :1;
  DWORD   fCDM                :1;
  DWORD   fAmbiguousCharSet   :1;
  DWORD   fClusterSizeVaries  :1;
  DWORD   fRejectInvalid      :1;
} SCRIPT_PROPERTIES;

#if defined(Q_OS_TEMP) && UNDER_CE < 400
typedef struct _ABC {
  int     abcA;
  UINT    abcB;
  int     abcC;
} ABC;
#endif

typedef HRESULT (WINAPI *fScriptFreeCache)(SCRIPT_CACHE *);
typedef HRESULT (WINAPI *fScriptItemize)(const WCHAR *, int, int, const SCRIPT_CONTROL *,
                                          const SCRIPT_STATE *, SCRIPT_ITEM *, int *);
typedef HRESULT (WINAPI *fScriptShape)(HDC hdc, SCRIPT_CACHE *, const WCHAR *, int, int,
                                        QScriptAnalysis *, WORD *, WORD *, SCRIPT_VISATTR *, int *);
typedef HRESULT (WINAPI *fScriptPlace)(HDC, SCRIPT_CACHE *, const WORD *, int, const SCRIPT_VISATTR *, QScriptAnalysis *, int *,
                                        GOFFSET *, ABC *);
typedef HRESULT (WINAPI *fScriptTextOut)(const HDC, SCRIPT_CACHE *, int, int, UINT, const RECT *, const QScriptAnalysis *,
                                         const WCHAR *, int, const WORD *, int, const int *, const int *, const GOFFSET *);
typedef HRESULT (WINAPI *fScriptBreak)(const WCHAR *, int, const QScriptAnalysis *, QCharAttributes *);
//typedef HRESULT (WINAPI *fScriptGetFontProperties)(HDC, SCRIPT_CACHE *, SCRIPT_FONTPROPERTIES *);
typedef HRESULT (WINAPI *fScriptGetProperties)(const SCRIPT_PROPERTIES ***, int *);

fScriptFreeCache ScriptFreeCache = 0;
static fScriptItemize ScriptItemize = 0;
static fScriptShape ScriptShape = 0;
static fScriptPlace ScriptPlace = 0;
fScriptTextOut ScriptTextOut = 0;
static fScriptBreak ScriptBreak = 0;
//static fScriptGetFontProperties ScriptGetFontProperties = 0;
static fScriptGetProperties ScriptGetProperties = 0;

static bool resolvedUsp10 = false;
bool hasUsp10 = false;

const SCRIPT_PROPERTIES **script_properties = 0;
int num_scripts = 0;
int usp_latin_script = 0;

static void uspAppendItems(QTextEngine *engine, int &start, int &stop, QBidiControl &control, QChar::Direction dir);

static void resolveUsp10()
{
#ifndef QT_NO_LIBRARY
    if (!resolvedUsp10) {
        // need to resolve the security info functions

#ifndef QT_NO_THREAD
        // protect initialization
        QMutexLocker locker(QMutexPool::globalInstanceGet((void*)&resolveUsp10));
        // check triedResolve again, since another thread may have already
        // done the initialization
        if (resolvedUsp10) {
            // another thread did initialize the security function pointers,
            // so we shouldn't do it again.
            return;
        }
#endif

        resolvedUsp10 = true;
        QLibrary lib(QLatin1String("usp10"));

	hasUsp10 = false;

        ScriptFreeCache = (fScriptFreeCache) lib.resolve("ScriptFreeCache");
        ScriptItemize = (fScriptItemize) lib.resolve("ScriptItemize");
        ScriptShape = (fScriptShape) lib.resolve("ScriptShape");
        ScriptPlace = (fScriptPlace) lib.resolve("ScriptPlace");
        ScriptTextOut = (fScriptTextOut) lib.resolve("ScriptTextOut");
        ScriptBreak = (fScriptBreak) lib.resolve("ScriptBreak");
        ScriptGetProperties = (fScriptGetProperties) lib.resolve("ScriptGetProperties");

        if (!ScriptFreeCache)
            return;

        // ### Disable uniscript for windows 9x for now
        if (QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)
            return;


        hasUsp10 = true;
        ScriptGetProperties(&script_properties, &num_scripts);

         // get the usp script for western
         for(int i = 0; i < num_scripts; i++) {
             if (script_properties[i]->langid == LANG_ENGLISH &&
                 !script_properties[i]->fAmbiguousCharSet) {
                 usp_latin_script = i;
                 break;
             }
         }

        appendItems = uspAppendItems;
    }
#endif
}

static unsigned char script_for_win_language[0x80] = {
    //0x00 LANG_NEUTRAL Neutral
    QUnicodeTables::Latin,
    //0x01 LANG_ARABIC Arabic
    QUnicodeTables::Arabic,
    //0x02 LANG_BULGARIAN Bulgarian
    QUnicodeTables::Common,
    //0x03 LANG_CATALAN Catalan
    QUnicodeTables::Common,
    //0x04 LANG_CHINESE Chinese
    QUnicodeTables::Han,
    //0x05 LANG_CZECH Czech
    QUnicodeTables::Common,
    //0x06 LANG_DANISH Danish
    QUnicodeTables::Common,
    //0x07 LANG_GERMAN German
    QUnicodeTables::Common,
    //0x08 LANG_GREEK Greek
    QUnicodeTables::Greek,
    //0x09 LANG_ENGLISH English
    QUnicodeTables::Latin,
    //0x0a LANG_SPANISH Spanish
    QUnicodeTables::Common,
    //0x0b LANG_FINNISH Finnish
    QUnicodeTables::Common,
    //0x0c LANG_FRENCH French
    QUnicodeTables::Common,
    //0x0d LANG_HEBREW Hebrew
    QUnicodeTables::Hebrew,
    //0x0e LANG_HUNGARIAN Hungarian
    QUnicodeTables::Common,
    //0x0f LANG_ICELANDIC Icelandic
    QUnicodeTables::Common,

    //0x10 LANG_ITALIAN Italian
    QUnicodeTables::Common,
    //0x11 LANG_JAPANESE Japanese
    QUnicodeTables::Hiragana,
    //0x12 LANG_KOREAN Korean
    QUnicodeTables::Hangul,
    //0x13 LANG_DUTCH Dutch
    QUnicodeTables::Common,
    //0x14 LANG_NORWEGIAN Norwegian
    QUnicodeTables::Common,
    //0x15 LANG_POLISH Polish
    QUnicodeTables::Common,
    //0x16 LANG_PORTUGUESE Portuguese
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x18 LANG_ROMANIAN Romanian
    QUnicodeTables::Common,
    //0x19 LANG_RUSSIAN Russian
    QUnicodeTables::Cyrillic,
    //0x1a LANG_CROATIAN Croatian
    //0x1a LANG_SERBIAN Serbian
    QUnicodeTables::Common,
    //0x1b LANG_SLOVAK Slovak
    QUnicodeTables::Common,
    //0x1c LANG_ALBANIAN Albanian
    QUnicodeTables::Common,
    //0x1d LANG_SWEDISH Swedish
    QUnicodeTables::Common,
    //0x1e LANG_THAI Thai
    QUnicodeTables::Thai,
    //0x1f LANG_TURKISH Turkish
    QUnicodeTables::Common,

    //0x20 LANG_URDU Urdu
    QUnicodeTables::Arabic,
    //0x21 LANG_INDONESIAN Indonesian
    QUnicodeTables::Common,
    //0x22 LANG_UKRAINIAN Ukrainian
    QUnicodeTables::Common,
    //0x23 LANG_BELARUSIAN Belarusian
    QUnicodeTables::Cyrillic,
    //0x24 LANG_SLOVENIAN Slovenian
    QUnicodeTables::Common,
    //0x25 LANG_ESTONIAN Estonian
    QUnicodeTables::Common,
    //0x26 LANG_LATVIAN Latvian
    QUnicodeTables::Common,
    //0x27 LANG_LITHUANIAN Lithuanian
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x29 LANG_FARSI Farsi
    QUnicodeTables::Arabic,
    //0x2a LANG_VIETNAMESE Vietnamese
    QUnicodeTables::Latin, // ##### maybe use QUnicodeTables::CombiningMarks instead?
    //0x2b LANG_ARMENIAN Armenian
    QUnicodeTables::Armenian,
    //0x2c LANG_AZERI Azeri
    QUnicodeTables::Common,
    //0x2d LANG_BASQUE Basque
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x2f LANG_MACEDONIAN FYRO Macedonian
    QUnicodeTables::Common,

    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x36 LANG_AFRIKAANS Afrikaans
    QUnicodeTables::Common,
    //0x37 LANG_GEORGIAN Georgian
    QUnicodeTables::Common,
    //0x38 LANG_FAEROESE Faeroese
    QUnicodeTables::Common,
    //0x39 LANG_HINDI Hindi
    QUnicodeTables::Devanagari,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x3e LANG_MALAY Malay
    QUnicodeTables::Common,
    //0x3f LANG_KAZAK Kazak
    QUnicodeTables::Common,

    //0x40 LANG_KYRGYZ Kyrgyz
    QUnicodeTables::Common,
    //0x41 LANG_SWAHILI Swahili
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x43 LANG_UZBEK Uzbek
    QUnicodeTables::Common,
    //0x44 LANG_TATAR Tatar
    QUnicodeTables::Common,
    //0x45 LANG_BENGALI Not supported.
    QUnicodeTables::Bengali,
    //0x46 LANG_PUNJABI Punjabi
    QUnicodeTables::Gurmukhi,
    //0x47 LANG_GUJARATI Gujarati
    QUnicodeTables::Gujarati,
    //0x48 LANG_ORIYA Not supported.
    QUnicodeTables::Oriya,
    //0x49 LANG_TAMIL Tamil
    QUnicodeTables::Tamil,
    //0x4a LANG_TELUGU Telugu
    QUnicodeTables::Telugu,
    //0x4b LANG_KANNADA Kannada
    QUnicodeTables::Kannada,
    //0x4c LANG_MALAYALAM Not supported.
    QUnicodeTables::Malayalam,
    //0x4d LANG_ASSAMESE Not supported.
    QUnicodeTables::Common,
    //0x4e LANG_MARATHI Marathi
    QUnicodeTables::Common,
    //0x4f LANG_SANSKRIT Sanskrit
    QUnicodeTables::Devanagari,

    //0x50 LANG_MONGOLIAN Mongolian
    QUnicodeTables::Mongolian,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x53 LANG_KHMER Khmer
    QUnicodeTables::Khmer,
    //0x54 LANG_LAO Lao
    QUnicodeTables::Lao,
    QUnicodeTables::Common,
    //0x56 LANG_GALICIAN Galician
    QUnicodeTables::Common,
    //0x57 LANG_KONKANI Konkani
    QUnicodeTables::Common,
    //0x58 LANG_MANIPURI Not supported.
    QUnicodeTables::Common,
    //0x59 LANG_SINDHI Not supported.
    QUnicodeTables::Common,
    //0x5a LANG_SYRIAC Syriac
    QUnicodeTables::Syriac,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,

    //0x60 LANG_KASHMIRI Not supported.
    QUnicodeTables::Common,
    //0x61 LANG_NEPALI Not supported.
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x65 LANG_DIVEHI Divehi
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,

    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    QUnicodeTables::Common,
    //0x7f LANG_INVARIANT
    QUnicodeTables::Common,
};

static inline QUnicodeTables::Script scriptForWinLanguage(DWORD langid)
{
    QUnicodeTables::Script script = langid < 0x80 ? (QUnicodeTables::Script)script_for_win_language[langid] : QUnicodeTables::Common;
    // if (script == QUnicodeTables::Common)
    //     qWarning("QTextEngine: Uniscribe support internal error: Encountered unhandled language %x", (unsigned int)langid);
    return script;
}

// we're not using Uniscribe's BiDi algorithm, since it is (a) not 100% Unicode compliant and
// (b) seems to work wrongly when trying to use it with a base level != 0.
//
// This function does uses Uniscribe to do the script analysis and creates items from this.
static void uspAppendItems(QTextEngine *engine, int &start, int &stop, QBidiControl &control, QChar::Direction dir)
{
    QScriptItemArray &items = engine->layoutData->items;
    const QChar *text = engine->layoutData->string.unicode();

    if (start > stop) {
        // #### the algorithm is currently not really safe against this. Still needs fixing.
//         qWarning("QTextEngine: BiDi internal error in uspAppendItems()");
        return;
    }

    int level = control.level;

    if(dir != QChar::DirON) {
        // add level of run (cases I1 & I2)
        if(level % 2) {
            if(dir == QChar::DirL || dir == QChar::DirAN || dir == QChar::DirEN)
                level++;
        } else {
            if(dir == QChar::DirR)
                level++;
            else if(dir == QChar::DirAN || dir == QChar::DirEN)
                level += 2;
        }
    }

    SCRIPT_ITEM s_items[256];
    SCRIPT_ITEM *usp_items = s_items;

    int numItems;
    HRESULT res = ScriptItemize((WCHAR *)(text+start), stop-start+1, 255, 0, 0, usp_items, &numItems);

    if (res == E_OUTOFMEMORY) {
        int alloc = 256;
        usp_items = 0;
        while(res == E_OUTOFMEMORY) {
            alloc *= 2;
            usp_items = (SCRIPT_ITEM *)realloc(usp_items, alloc * sizeof(SCRIPT_ITEM));
            res = ScriptItemize((WCHAR *)(text+start), stop-start+1, alloc-1, 0, 0, usp_items, &numItems);
        }
    }
    int i;
    for(i = 0; i < numItems; i++) {
        QScriptItem item;
        item.analysis = usp_items[i].a;
        item.position = usp_items[i].iCharPos+start;
        item.analysis.bidiLevel = level;
        item.analysis.override = control.override;

        int rstart = usp_items[i].iCharPos;
        int rstop = usp_items[i+1].iCharPos-1;
        bool b = true;
        for (int j = rstart; j <= rstop; j++) {

            unsigned short uc = text[j+start].unicode();
            if (uc == QChar::ObjectReplacementCharacter || uc == QChar::LineSeparator) {
                item.analysis.script = usp_latin_script;
                item.isObject = true;
                b = true;
            } else if (uc == 9) {
                item.analysis.script = usp_latin_script;
                item.isSpace = true;
                item.isTab = true;
                item.analysis.bidiLevel = control.baseLevel();
                b = true;
            } else if (b) {
                b = false;
            } else {
                if (j - rstart < 32000)
                    continue;
                rstart = j;
            }

            item.position = j+start;
            items.append(item);
            item.analysis = usp_items[i].a;
            item.analysis.bidiLevel = level;
            item.analysis.override = control.override;
            item.isSpace = item.isTab = item.isObject = false;
        }
    }

    if (usp_items != s_items)
        free(usp_items);

    ++stop;
    start = stop;
}

// -----------------------------------------------------------------------------------------------------
//
// Text engine classes
//
// -----------------------------------------------------------------------------------------------------


void QTextEngine::shapeText(int item) const
{
    QScriptItem &si = layoutData->items[item];

    if (si.num_glyphs)
        return;

    int script = si.analysis.script;
    if (hasUsp10) {
        const SCRIPT_PROPERTIES *script_prop = script_properties[si.analysis.script];
        script = scriptForWinLanguage(script_prop->langid);
    }

    // Just to get the warning away
    int from = si.position;
    const int len = length(item);
    Q_ASSERT(len > 0);
    Q_UNUSED(len); // --release warning

    si.glyph_data_offset = layoutData->used;

    QFontEngine *fontEngine = this->fontEngine(si, &si.ascent, &si.descent);

    if (hasUsp10 && fontEngine->ttf) {
        int l = len;
        si.analysis.logicalOrder = true;
        HRESULT res = E_OUTOFMEMORY;
        HDC hdc = 0;

        QVarLengthArray<WORD> glyphs(l);
        QVarLengthArray<WORD> logClusters(l);
        QVarLengthArray<SCRIPT_VISATTR> glyphAttributes(l);

        do {
            res = ScriptShape(hdc, &fontEngine->script_cache, (WCHAR *)layoutData->string.unicode() + from, len,
                               l, &si.analysis, glyphs.data(), logClusters.data(), glyphAttributes.data(),
                               &si.num_glyphs);
            if (res == E_PENDING) {
                hdc = GetDC(0);
                SelectObject(hdc, fontEngine->hfont);
            } else if (res == USP_E_SCRIPT_NOT_IN_FONT) {
                si.analysis.script = 0;
            } else if (res == E_OUTOFMEMORY) {
                l += 32;
                glyphs.resize(l);
                logClusters.resize(l);
                glyphAttributes.resize(l);
            } else if (res != S_OK) {
                goto fail;
            }
        } while(res != S_OK);

        for(int i = 0; i < len; ++i) {
            if(glyphs[logClusters[i]] == 0) {
                glyphAttributes[logClusters[i]].clusterStart = true;
                glyphAttributes[logClusters[i]].zeroWidth = false;
            }
        }
        {
            ABC abc;
            QVarLengthArray<int> advances(si.num_glyphs);
            QVarLengthArray<GOFFSET> offsets(si.num_glyphs);
            res = ScriptPlace(hdc, &fontEngine->script_cache, glyphs.data(), si.num_glyphs,
                               glyphAttributes.data(), &si.analysis, advances.data(), offsets.data(), &abc);
            if (res == E_PENDING) {
                Q_ASSERT(hdc == 0);
                hdc = GetDC(0);
                SelectObject(hdc, fontEngine->hfont);
                ScriptPlace(hdc, &fontEngine->script_cache, glyphs.data(), si.num_glyphs,
                             glyphAttributes.data(), &si.analysis, advances.data(), offsets.data(), &abc);
            }
            if (res != S_OK)
                goto fail;

            ensureSpace(si.num_glyphs);
            si.glyph_data_offset = layoutData->used;
            QGlyphLayout *g = this->glyphs(&si);
            const int direction = (si.analysis.bidiLevel % 2) ? -1 : 1;
            for(int i = 0; i < si.num_glyphs; ++i) {
                g[i].glyph = glyphs[i];
                g[i].advance.x = advances[i];
                g[i].advance.y = 0;
                g[i].offset.x = offsets[i].du * direction;
                g[i].offset.y = offsets[i].dv;
                g[i].attributes = glyphAttributes[i];
            }
            unsigned short *lc = this->logClusters(&si);
            for(int i = 0; i < len; ++i)
                lc[i] = logClusters[i];
        }
fail:
        if (hdc)
            ReleaseDC(0, hdc);
        if(res == S_OK) {
            QGlyphLayout *g = this->glyphs(&si);
            unsigned short *lc = this->logClusters(&si);
            int pos = 0;
            while (pos < len) {
                const ushort uc = layoutData->string.at(si.position + pos).unicode();
                const bool dontPrint = ((uc == 0x00ad && !fontEngine->symbol) || qIsControlChar(uc));
                const int gp = lc[pos];
                g[gp].attributes.dontPrint = dontPrint;
                ++pos;
                while (pos < len && gp == lc[pos]) {
                    g[gp].attributes.dontPrint = dontPrint;
                    ++pos;
                }
            }
            goto end;
        }
    }

    {
        // non uniscribe code path, also used if uniscribe fails for some reason
        Q_ASSERT(script < QUnicodeTables::ScriptCount);

        QShaperItem shaper_item;
        shaper_item.script = script;
        shaper_item.string = &layoutData->string;
        shaper_item.from = si.position;
        shaper_item.length = length(item);
        shaper_item.font = fontEngine;
        shaper_item.num_glyphs = qMax(layoutData->num_glyphs - layoutData->used, shaper_item.length);
        shaper_item.flags = si.analysis.bidiLevel % 2 ? RightToLeft : 0;
        if (option.useDesignMetrics())
            shaper_item.flags |= DesignMetrics;

        while (1) {
            ensureSpace(shaper_item.num_glyphs);
            shaper_item.num_glyphs = layoutData->num_glyphs - layoutData->used;
            shaper_item.glyphs = glyphs(&si);
            shaper_item.log_clusters = logClusters(&si);
            if (qt_scriptEngines[shaper_item.script].shape(&shaper_item))
                break;
        }
        si.num_glyphs = shaper_item.num_glyphs;
    }
end:
    si.analysis.script = script;

    QGlyphLayout *g = glyphs(&si);
    if (this->font(si).d->kerning)
        fontEngine->doKerning(si.num_glyphs, g, QFlag(option.useDesignMetrics() ? DesignMetrics : 0));

    si.width = 0;
    QGlyphLayout *end = g + si.num_glyphs;
    while (g < end)
        si.width += (g++)->advance.x;


    layoutData->used += si.num_glyphs;
}
