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

#include <private/qapplication_p.h>
#include <private/qfontengine_p.h>
#include <private/qpainter_p.h>
#include <private/qtextengine_p.h>
#include <qbitmap.h>
#include <private/qpaintengine_mac_p.h>
#include <private/qprintengine_mac_p.h>
#include <private/qpdf_p.h>
#include <qglobal.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qvarlengtharray.h>
#include <qdebug.h>
#include <qendian.h>

#include <ApplicationServices/ApplicationServices.h>

/*****************************************************************************
  QFontEngine debug facilities
 *****************************************************************************/
//#define DEBUG_ADVANCES

extern int qt_antialiasing_threshold; // QApplication.cpp

#ifndef FixedToQFixed
#define FixedToQFixed(a) QFixed::fromFixed((a) >> 10)
#define QFixedToFixed(x) ((x).value() << 10)
#endif

class QMacFontPath
{
    float x, y;
    QPainterPath *path;
public:
    inline QMacFontPath(float _x, float _y, QPainterPath *_path) : x(_x), y(_y), path(_path) { }
    inline void setPosition(float _x, float _y) { x = _x; y = _y; }
    inline void advance(float _x) { x += _x; }
    static OSStatus lineTo(const Float32Point *, void *);
    static OSStatus cubicTo(const Float32Point *, const Float32Point *,
                            const Float32Point *, void *);
    static OSStatus moveTo(const Float32Point *, void *);
    static OSStatus closePath(void *);
};

OSStatus QMacFontPath::lineTo(const Float32Point *pt, void *data)

{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->lineTo(p->x + pt->x, p->y + pt->y);
    return noErr;
}

OSStatus QMacFontPath::cubicTo(const Float32Point *cp1, const Float32Point *cp2,
                               const Float32Point *ep, void *data)

{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->cubicTo(p->x + cp1->x, p->y + cp1->y,
                     p->x + cp2->x, p->y + cp2->y,
                     p->x + ep->x, p->y + ep->y);
    return noErr;
}

OSStatus QMacFontPath::moveTo(const Float32Point *pt, void *data)
{
    QMacFontPath *p = static_cast<QMacFontPath*>(data);
    p->path->moveTo(p->x + pt->x, p->y + pt->y);
    return noErr;
}

OSStatus QMacFontPath::closePath(void *data)
{
    static_cast<QMacFontPath*>(data)->path->closeSubpath();
    return noErr;
}

#include "qscriptengine_p.h"

QFontEngineMacMulti::QFontEngineMacMulti(const ATSFontFamilyRef &atsFamily, const ATSFontRef &atsFontRef, const QFontDef &fontDef, bool kerning)
    : QFontEngineMulti(0)
{
    this->fontDef = fontDef;
    this->kerning = kerning;

    FMFontFamily fmFamily;
    FMFontStyle fntStyle = 0;
    fmFamily = FMGetFontFamilyFromATSFontFamilyRef(atsFamily);
    if (fmFamily == kInvalidFontFamily) {
        // Use the ATSFont then...
        fontID = FMGetFontFromATSFontRef(atsFontRef);
    } else {
        if (fontDef.weight >= QFont::Bold)
            fntStyle |= ::bold;
        if (fontDef.style != QFont::StyleNormal)
            fntStyle |= ::italic;

        FMFontStyle intrinsicStyle;
        FMFont fnt = 0;
        if (FMGetFontFromFontFamilyInstance(fmFamily, fntStyle, &fnt, &intrinsicStyle) == noErr)
           fontID = FMGetATSFontRefFromFont(fnt);
    }

    OSStatus status;

    status = ATSUCreateTextLayout(&textLayout);
    Q_ASSERT(status == noErr);

    const int maxAttributeCount = 5;
    ATSUAttributeTag tags[maxAttributeCount + 1];
    ByteCount sizes[maxAttributeCount + 1];
    ATSUAttributeValuePtr values[maxAttributeCount + 1];
    int attributeCount = 0;

    Fixed size = FixRatio(fontDef.pixelSize, 1);
    tags[attributeCount] = kATSUSizeTag;
    sizes[attributeCount] = sizeof(size);
    values[attributeCount] = &size;
    ++attributeCount;

    tags[attributeCount] = kATSUFontTag;
    sizes[attributeCount] = sizeof(fontID);
    values[attributeCount] = &this->fontID;
    ++attributeCount;

    transform = CGAffineTransformIdentity;
    if (fontDef.stretch != 100) {
        transform = CGAffineTransformMakeScale(float(fontDef.stretch) / float(100), 1);
        tags[attributeCount] = kATSUFontMatrixTag;
        sizes[attributeCount] = sizeof(transform);
        values[attributeCount] = &transform;
        ++attributeCount;
    }

    status = ATSUCreateStyle(&style);
    Q_ASSERT(status == noErr);

    Q_ASSERT(attributeCount < maxAttributeCount + 1);
    status = ATSUSetAttributes(style, attributeCount, tags, sizes, values);
    Q_ASSERT(status == noErr);

    QFontEngineMac *fe = new QFontEngineMac(style, fontID, fontDef, this);
    fe->ref.ref();
    engines.append(fe);
}

QFontEngineMacMulti::~QFontEngineMacMulti()
{
    ATSUDisposeTextLayout(textLayout);
    ATSUDisposeStyle(style);

    for (int i = 0; i < engines.count(); ++i) {
        QFontEngineMac *fe = const_cast<QFontEngineMac *>(static_cast<const QFontEngineMac *>(engines.at(i)));
        fe->multiEngine = 0;
        if (!fe->ref.deref())
            delete fe;
    }
    engines.clear();
}

struct QGlyphLayoutInfo
{
    QGlyphLayout *glyphs;
    int *numGlyphs;
    bool callbackCalled;
    int *mappedFonts;
    QTextEngine::ShaperFlags flags;
    QShaperItem *shaperItem;
};

static OSStatus atsuPostLayoutCallback(ATSULayoutOperationSelector selector, ATSULineRef lineRef, URefCon refCon,
                                 void *operationExtraParameter, ATSULayoutOperationCallbackStatus *callbackStatus)
{
    Q_UNUSED(selector);
    Q_UNUSED(operationExtraParameter);

    QGlyphLayoutInfo *nfo = reinterpret_cast<QGlyphLayoutInfo *>(refCon);
    nfo->callbackCalled = true;

    ATSLayoutRecord *layoutData = 0;
    ItemCount itemCount = 0;

    OSStatus e = noErr;
    e = ATSUDirectGetLayoutDataArrayPtrFromLineRef(lineRef, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
                                                   /*iCreate =*/ false,
                                                   (void **) &layoutData,
                                                   &itemCount);
    if (e != noErr)
        return e;

    *nfo->numGlyphs = itemCount - 1;

    Fixed *baselineDeltas = 0;

    e = ATSUDirectGetLayoutDataArrayPtrFromLineRef(lineRef, kATSUDirectDataBaselineDeltaFixedArray,
                                                   /*iCreate =*/ true,
                                                   (void **) &baselineDeltas,
                                                   &itemCount);
    if (e != noErr)
        return e;

    int nextCharStop = -1;
    int currentClusterGlyph = -1; // first glyph in log cluster
    QShaperItem *item = 0;
    if (nfo->shaperItem) {
        item = nfo->shaperItem;
#if !defined(QT_NO_DEBUG)
        int surrogates = 0;
        const QString &str = *item->string;
        for (int i = item->from; i < item->from + item->length - 1; ++i) {
            surrogates += (str[i].unicode() >= 0xd800 && str[i].unicode() < 0xdc00
                           && str[i+1].unicode() >= 0xdc00 && str[i+1].unicode() < 0xe000);
        }
        Q_ASSERT(*nfo->numGlyphs == item->length - surrogates);
#endif
        for (nextCharStop = item->from; nextCharStop < item->from + item->length; ++nextCharStop)
            if (item->charAttributes[nextCharStop].charStop)
                break;
        nextCharStop -= item->from;
    }

    nfo->glyphs[0].attributes.clusterStart = true;
    int glyphIdx = 0;
    int glyphIncrement = 1;
    if (nfo->flags & QTextEngine::RightToLeft) {
        glyphIdx  = itemCount - 2;
        glyphIncrement = -1;
    }
    for (int i = 0; i < *nfo->numGlyphs; ++i, glyphIdx += glyphIncrement) {

        int charOffset = layoutData[glyphIdx].originalOffset / sizeof(UniChar);
        const int fontIdx = nfo->mappedFonts[charOffset];

        ATSGlyphRef glyphId = layoutData[glyphIdx].glyphID;

        QFixed yAdvance = FixedToQFixed(baselineDeltas[glyphIdx]);
        QFixed xAdvance = FixedToQFixed(layoutData[glyphIdx + 1].realPos - layoutData[glyphIdx].realPos);

        if (glyphId != 0xffff || i == 0) {
            nfo->glyphs[i].glyph = (glyphId & 0x00ffffff) | (fontIdx << 24);

            nfo->glyphs[i].advance.y = yAdvance;
            nfo->glyphs[i].advance.x = xAdvance;
        } else {
            // ATSUI gives us 0xffff as glyph id at the index in the glyph array for
            // a character position that maps to a ligtature. Such a glyph id does not
            // result in any visual glyph, but it may have an advance, which is why we
            // sum up the glyph advances.
            --i;
            nfo->glyphs[i].advance.y += yAdvance;
            nfo->glyphs[i].advance.x += xAdvance;
            *nfo->numGlyphs -= 1;
        }

        if (item) {
            if (charOffset >= nextCharStop) {
                nfo->glyphs[i].attributes.clusterStart = true;
                currentClusterGlyph = i;

                ++nextCharStop;
                for (; nextCharStop < item->length; ++nextCharStop)
                    if (item->charAttributes[item->from + nextCharStop].charStop)
                        break;
            } else {
                if (currentClusterGlyph == -1)
                    currentClusterGlyph = i;
            }
            item->log_clusters[charOffset] = currentClusterGlyph;

            // surrogate handling
            if (charOffset < item->length - 1) {
                QChar current = item->string->at(item->from + charOffset);
                QChar next = item->string->at(item->from + charOffset + 1);
                if (current.unicode() >= 0xd800 && current.unicode() < 0xdc00
                    && next.unicode() >= 0xdc00 && next.unicode() < 0xe000) {
                    item->log_clusters[charOffset + 1] = currentClusterGlyph;
                }
            }
        }
    }

    /*
    if (item) {
        qDebug() << "resulting logclusters:";
        for (int i = 0; i < item->length; ++i)
            qDebug() << "logClusters[" << i << "] =" << item->log_clusters[i];
        qDebug() << "clusterstarts:";
        for (int i = 0; i < *nfo->numGlyphs; ++i)
            qDebug() << "clusterStart[" << i << "] =" << nfo->glyphs[i].attributes.clusterStart;
    }
    */

    ATSUDirectReleaseLayoutDataArrayPtr(lineRef, kATSUDirectDataBaselineDeltaFixedArray,
                                        (void **) &baselineDeltas);

    ATSUDirectReleaseLayoutDataArrayPtr(lineRef, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
                                        (void **) &layoutData);

    *callbackStatus = kATSULayoutOperationCallbackStatusHandled;
    return noErr;
}

int QFontEngineMacMulti::fontIndexForFontID(ATSUFontID id) const
{
    for (int i = 0; i < engines.count(); ++i) {
        if (engineAt(i)->fontID == id)
            return i;
    }

    QFontEngineMacMulti *that = const_cast<QFontEngineMacMulti *>(this);
    QFontEngineMac *fe = new QFontEngineMac(style, id, fontDef, that);
    fe->ref.ref();
    that->engines.append(fe);
    return engines.count() - 1;
}

bool QFontEngineMacMulti::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    return stringToCMap(str, len, glyphs, nglyphs, flags, /*shaperItem=*/0);
}

bool QFontEngineMacMulti::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags,
                                  QShaperItem *shaperItem) const
{
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    const int maxChars = qMax(1,
                              int(SHRT_MAX / maxCharWidth())
                              - 10 // subtract a few to be on the safe side
                             );
    if (len < maxChars)
        return stringToCMapInternal(str, len, glyphs, nglyphs, flags, shaperItem);

    int charIdx = 0;
    int glyphIdx = 0;
    QShaperItem tmpItem = *shaperItem;

    do {
        tmpItem.from = shaperItem->from + charIdx;

        int charCount = qMin(maxChars, len - charIdx);

        int lastWhitespace = tmpItem.from + charCount - 1;
        int lastSoftBreak = lastWhitespace;
        int lastCharStop = lastSoftBreak;
        for (int i = lastCharStop; i >= tmpItem.from; --i) {
            if (tmpItem.charAttributes[i].whiteSpace) {
                lastWhitespace = i;
                break;
            } if (tmpItem.charAttributes[i].lineBreakType != QCharAttributes::NoBreak) {
                lastSoftBreak = i;
            } if (tmpItem.charAttributes[i].charStop) {
                lastCharStop = i;
            }
        }
        charCount = qMin(lastWhitespace, qMin(lastSoftBreak, lastCharStop)) - tmpItem.from + 1;

        int glyphCount = shaperItem->num_glyphs - glyphIdx;
        if (glyphCount <= 0)
            return false;
        tmpItem.length = charCount;
        tmpItem.num_glyphs = glyphCount;
        tmpItem.glyphs = shaperItem->glyphs + glyphIdx;
        tmpItem.log_clusters = shaperItem->log_clusters + charIdx;
        if (!stringToCMapInternal(tmpItem.string->constData() + tmpItem.from,
                                  tmpItem.length,
                                  tmpItem.glyphs,
                                  &glyphCount,
                                  flags, &tmpItem)) {
            return false;
        }
        for (int i = 0; i < charCount; ++i)
            tmpItem.log_clusters[i] += glyphIdx;
        glyphIdx += glyphCount;
        charIdx += charCount;
    } while (charIdx < len);
    shaperItem->num_glyphs = *nglyphs = glyphIdx;

    return true;
}

bool QFontEngineMacMulti::stringToCMapInternal(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
                                               QTextEngine::ShaperFlags flags, QShaperItem *shaperItem) const
{
    //qDebug() << "stringToCMap" << QString(str, len);

    OSStatus e = noErr;

    e = ATSUSetTextPointerLocation(textLayout, (UniChar *)(str), 0, len, len);
    if (e != noErr) {
        qWarning("Qt: internal: %ld: Error ATSUSetTextPointerLocation %s: %d", long(e), __FILE__, __LINE__);
        return false;
    }

    QGlyphLayoutInfo nfo;
    nfo.glyphs = glyphs;
    nfo.numGlyphs = nglyphs;
    nfo.callbackCalled = false;
    nfo.flags = flags;
    nfo.shaperItem = shaperItem;

    QVarLengthArray<int> mappedFonts(len);
    for (int i = 0; i < len; ++i)
        mappedFonts[i] = 0;
    nfo.mappedFonts = mappedFonts.data();

    Q_ASSERT(sizeof(void *) <= sizeof(URefCon));
    e = ATSUSetTextLayoutRefCon(textLayout, (URefCon)&nfo);
    if (e != noErr) {
        qWarning("Qt: internal: %ld: Error ATSUSetTextLayoutRefCon %s: %d", long(e), __FILE__, __LINE__);
        return false;
    }

    {
        const int maxAttributeCount = 3;
        ATSUAttributeTag tags[maxAttributeCount + 1];
        ByteCount sizes[maxAttributeCount + 1];
        ATSUAttributeValuePtr values[maxAttributeCount + 1];
        int attributeCount = 0;

        tags[attributeCount] = kATSULineLayoutOptionsTag;
        ATSLineLayoutOptions layopts = kATSLineHasNoOpticalAlignment
                                       | kATSLineIgnoreFontLeading
                                       | kATSLineNoSpecialJustification // we do kashidas ourselves
                                       | kATSLineDisableAllJustification
                                       ;

        if (!(flags & QTextEngine::DesignMetrics)) {
            layopts |= kATSLineFractDisable | kATSLineUseDeviceMetrics
                       | kATSLineDisableAutoAdjustDisplayPos;
        }

        if (fontDef.styleStrategy & QFont::NoAntialias)
            layopts |= kATSLineNoAntiAliasing;

        if (!kerning)
            layopts |= kATSLineDisableAllKerningAdjustments;

        values[attributeCount] = &layopts;
        sizes[attributeCount] = sizeof(layopts);
        ++attributeCount;

        tags[attributeCount] = kATSULayoutOperationOverrideTag;
        ATSULayoutOperationOverrideSpecifier spec;
        spec.operationSelector = kATSULayoutOperationPostLayoutAdjustment;
        spec.overrideUPP = atsuPostLayoutCallback;
        values[attributeCount] = &spec;
        sizes[attributeCount] = sizeof(spec);
        ++attributeCount;

        Boolean direction;
        if (flags & QTextEngine::RightToLeft)
            direction = kATSURightToLeftBaseDirection;
        else
            direction = kATSULeftToRightBaseDirection;
        tags[attributeCount] = kATSULineDirectionTag;
        values[attributeCount] = &direction;
        sizes[attributeCount] = sizeof(direction);
        ++attributeCount;

        Q_ASSERT(attributeCount < maxAttributeCount + 1);
        e = ATSUSetLayoutControls(textLayout, attributeCount, tags, sizes, values);
        if (e != noErr) {
            qWarning("Qt: internal: %ld: Error ATSUSetLayoutControls %s: %d", long(e), __FILE__, __LINE__);
            return false;
        }

    }

    e = ATSUSetRunStyle(textLayout, style, 0, len);
    if (e != noErr) {
        qWarning("Qt: internal: %ld: Error ATSUSetRunStyle %s: %d", long(e), __FILE__, __LINE__);
        return false;
    }

    if (!(fontDef.styleStrategy & QFont::NoFontMerging)) {
        int pos = 0;
        do {
            ATSUFontID substFont = 0;
            UniCharArrayOffset changedOffset = 0;
            UniCharCount changeCount = 0;

            e = ATSUMatchFontsToText(textLayout, pos, len - pos,
                                     &substFont, &changedOffset,
                                     &changeCount);
            if (e == kATSUFontsMatched) {
                int fontIdx = fontIndexForFontID(substFont);
                for (uint i = 0; i < changeCount; ++i)
                    mappedFonts[changedOffset + i] = fontIdx;
                pos = changedOffset + changeCount;
                ATSUSetRunStyle(textLayout, engineAt(fontIdx)->style, changedOffset, changeCount);
            } else if (e == kATSUFontsNotMatched) {
                pos = changedOffset + changeCount;
            }
        } while (pos < len && e != noErr);
    }
    {    // trigger the a layout
        Rect rect;
        e = ATSUMeasureTextImage(textLayout, kATSUFromTextBeginning, kATSUToTextEnd,
                                 /*iLocationX =*/ 0, /*iLocationY =*/ 0,
                                 &rect);
        if (e != noErr) {
            qWarning("Qt: internal: %ld: Error ATSUMeasureTextImage %s: %d", long(e), __FILE__, __LINE__);
            return false;
        }
    }

    if (!nfo.callbackCalled) {
            qWarning("Qt: internal: %ld: Error ATSUMeasureTextImage did not trigger callback %s: %d", long(e), __FILE__, __LINE__);
            return false;
    }

    ATSUClearLayoutCache(textLayout, kATSUFromTextBeginning);
    return true;
}

void QFontEngineMacMulti::recalcAdvances(int numGlyphs, QGlyphLayout *glyphs, QTextEngine::ShaperFlags flags) const
{
    Q_ASSERT(false);
    Q_UNUSED(numGlyphs);
    Q_UNUSED(glyphs);
    Q_UNUSED(flags);
}

void QFontEngineMacMulti::doKerning(int , QGlyphLayout *, QTextEngine::ShaperFlags) const
{
    //Q_ASSERT(false);
}

void QFontEngineMacMulti::loadEngine(int /*at*/)
{
    // should never be called!
    Q_ASSERT(false);
}

bool QFontEngineMacMulti::canRender(const QChar *string, int len)
{
    ATSUSetTextPointerLocation(textLayout, reinterpret_cast<const UniChar *>(string), 0, len, len);
    ATSUSetRunStyle(textLayout, style, 0, len);

    OSStatus e = noErr;
    int pos = 0;
    do {
        FMFont substFont = 0;
        UniCharArrayOffset changedOffset = 0;
        UniCharCount changeCount = 0;

        e = ATSUMatchFontsToText(textLayout, pos, len - pos,
                                 &substFont, &changedOffset,
                                 &changeCount);
        if (e == kATSUFontsMatched) {
            pos = changedOffset + changeCount;
        } else if (e == kATSUFontsNotMatched) {
            break;
        }
    } while (pos < len && e != noErr);

    return e == noErr || e == kATSUFontsMatched;
}

QFontEngineMac::QFontEngineMac(ATSUStyle baseStyle, ATSUFontID fontID, const QFontDef &def, QFontEngineMacMulti *multiEngine)
    : fontID(fontID), multiEngine(multiEngine), cmap(0), symbolCMap(false)
{
    fontDef = def;
    ATSUCreateAndCopyStyle(baseStyle, &style);
    ATSFontRef atsFont = FMGetATSFontRefFromFont(fontID);
    cgFont = CGFontCreateWithPlatformFont(&atsFont);

    const int maxAttributeCount = 4;
    ATSUAttributeTag tags[maxAttributeCount + 1];
    ByteCount sizes[maxAttributeCount + 1];
    ATSUAttributeValuePtr values[maxAttributeCount + 1];
    int attributeCount = 0;

    synthesisFlags = 0;

    quint16 macStyle = 0;
    {
        uchar data[4];
        ByteCount len = 4;
        if (ATSFontGetTable(atsFont, MAKE_TAG('h', 'e', 'a', 'd'), 44, 4, &data, &len) == noErr)
            macStyle = qFromBigEndian<quint16>(data);
    }

    Boolean atsuBold = false;
    Boolean atsuItalic = false;
    if (fontDef.weight >= QFont::Bold) {
        if (!(macStyle & 1)) {
            synthesisFlags |= SynthesizedBold;
            atsuBold = true;
            tags[attributeCount] = kATSUQDBoldfaceTag;
            sizes[attributeCount] = sizeof(atsuBold);
            values[attributeCount] = &atsuBold;
            ++attributeCount;
        }
    }
    if (fontDef.style != QFont::StyleNormal) {
        if (!(macStyle & 2)) {
            synthesisFlags |= SynthesizedItalic;
            atsuItalic = true;
            tags[attributeCount] = kATSUQDItalicTag;
            sizes[attributeCount] = sizeof(atsuItalic);
            values[attributeCount] = &atsuItalic;
            ++attributeCount;
        }
    }

    tags[attributeCount] = kATSUFontTag;
    values[attributeCount] = &fontID;
    sizes[attributeCount] = sizeof(fontID);
    ++attributeCount;

    Q_ASSERT(attributeCount < maxAttributeCount + 1);
    OSStatus err = ATSUSetAttributes(style, attributeCount, tags, sizes, values);
    Q_ASSERT(err == noErr);
    Q_UNUSED(err);

    quint16 tmpFsType;
    if (ATSFontGetTable(atsFont, MAKE_TAG('O', 'S', '/', '2'), 8, 2, &tmpFsType, 0) == noErr)
       fsType = qFromBigEndian<quint16>(tmpFsType);
    else
        fsType = 0;
}

QFontEngineMac::~QFontEngineMac()
{
    ATSUDisposeStyle(style);
}

static inline unsigned int getChar(const QChar *str, int &i, const int len)
{
    unsigned int uc = str[i].unicode();
    if (uc >= 0xd800 && uc < 0xdc00 && i < len-1) {
        uint low = str[i+1].unicode();
       if (low >= 0xdc00 && low < 0xe000) {
            uc = (uc - 0xd800)*0x400 + (low - 0xdc00) + 0x10000;
            ++i;
        }
    }
    return uc;
}

bool QFontEngineMac::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if (flags & QTextEngine::GlyphIndicesOnly) {
        if (!cmap) {
            cmapTable = getSfntTable(MAKE_TAG('c', 'm', 'a', 'p'));
            int size = 0;
            cmap = getCMap(reinterpret_cast<const uchar *>(cmapTable.constData()), cmapTable.size(), &symbolCMap, &size);
            if (!cmap)
                return false;
        }
        if (symbolCMap) {
            for (int i = 0; i < len; ++i) {
                unsigned int uc = getChar(str, i, len);
                glyphs->glyph = getTrueTypeGlyphIndex(cmap, uc);
                if(!glyphs->glyph && uc < 0x100)
                    glyphs->glyph = getTrueTypeGlyphIndex(cmap, uc + 0xf000);
                glyphs++;
            }
        } else {
            for (int i = 0; i < len; ++i) {
                unsigned int uc = getChar(str, i, len);
                glyphs->glyph = getTrueTypeGlyphIndex(cmap, uc);
                glyphs++;
            }
        }

        *nglyphs = len;
        return true;
    }
    if (!multiEngine)
        return false;

    const bool kashidaRequest = (len == 1 && *str == QChar(0x640));
    if (kashidaRequest && kashidaGlyph.glyph != 0) {
        *glyphs = kashidaGlyph;
        *nglyphs = 1;
        return true;
    }

    bool result = multiEngine->stringToCMap(str, len, glyphs, nglyphs, flags);

    if (result && kashidaRequest) {
        kashidaGlyph = *glyphs;
    }

    return result;
}

glyph_metrics_t QFontEngineMac::boundingBox(const QGlyphLayout *glyphs, int numGlyphs)
{
    QFixed w;
    const QGlyphLayout *end = glyphs + numGlyphs;
    while(end > glyphs)
        w += (--end)->advance.x;
    return glyph_metrics_t(0, -(ascent()), w, ascent()+descent(), w, 0);
}

glyph_metrics_t QFontEngineMac::boundingBox(glyph_t glyph)
{
    GlyphID atsuGlyph = glyph;

    ATSGlyphScreenMetrics metrics;

    ATSUGlyphGetScreenMetrics(style, 1, &atsuGlyph, 0,
                              /* iForcingAntiAlias =*/ false,
                              /* iAntiAliasSwitch =*/true,
                              &metrics);

    // ### check again

    glyph_metrics_t gm;
    gm.width = int(metrics.width);
    gm.height = int(metrics.height);
    gm.x = QFixed::fromReal(metrics.topLeft.x);
    gm.y = -QFixed::fromReal(metrics.topLeft.y);
    gm.xoff = QFixed::fromReal(metrics.deviceAdvance.x);
    gm.yoff = QFixed::fromReal(metrics.deviceAdvance.y);

    return gm;
}

QFixed QFontEngineMac::ascent() const
{
    ATSUTextMeasurement metric;
    ATSUGetAttribute(style, kATSUAscentTag, sizeof(metric), &metric, 0);
    return FixRound(metric);
}

QFixed QFontEngineMac::descent() const
{
    ATSUTextMeasurement metric;
    ATSUGetAttribute(style, kATSUDescentTag, sizeof(metric), &metric, 0);
    return FixRound(metric);
}

QFixed QFontEngineMac::leading() const
{
    ATSUTextMeasurement metric;
    ATSUGetAttribute(style, kATSULeadingTag, sizeof(metric), &metric, 0);
    return FixRound(metric);
}

qreal QFontEngineMac::maxCharWidth() const
{
    ATSFontMetrics metrics;
    ATSFontGetHorizontalMetrics(FMGetATSFontRefFromFont(fontID), kATSOptionFlagsDefault, &metrics);
    return metrics.maxAdvanceWidth * fontDef.pointSize;
}

QFixed QFontEngineMac::xHeight() const
{
    ATSFontMetrics metrics;
    ATSFontGetHorizontalMetrics(FMGetATSFontRefFromFont(fontID), kATSOptionFlagsDefault, &metrics);
    return QFixed::fromReal(metrics.xHeight * fontDef.pointSize);
}

QFixed QFontEngineMac::averageCharWidth() const
{
    ATSFontMetrics metrics;
    ATSFontGetHorizontalMetrics(FMGetATSFontRefFromFont(fontID), kATSOptionFlagsDefault, &metrics);
    return QFixed::fromReal(metrics.avgAdvanceWidth * fontDef.pointSize);
}

static void addGlyphsToPath(ATSUStyle style, glyph_t *glyphs, QFixedPoint *positions, int numGlyphs, QPainterPath *path)
{
    if (!numGlyphs)
        return;

    OSStatus e;

    QMacFontPath fontpath(0, 0, path);
    ATSCubicMoveToUPP moveTo = NewATSCubicMoveToUPP(QMacFontPath::moveTo);
    ATSCubicLineToUPP lineTo = NewATSCubicLineToUPP(QMacFontPath::lineTo);
    ATSCubicCurveToUPP cubicTo = NewATSCubicCurveToUPP(QMacFontPath::cubicTo);
    ATSCubicClosePathUPP closePath = NewATSCubicClosePathUPP(QMacFontPath::closePath);

    for (int i = 0; i < numGlyphs; ++i) {
        GlyphID glyph = glyphs[i];

        fontpath.setPosition(positions[i].x.toReal(), positions[i].y.toReal());
        ATSUGlyphGetCubicPaths(style, glyph, moveTo, lineTo,
                               cubicTo, closePath, &fontpath, &e);
    }

    DisposeATSCubicMoveToUPP(moveTo);
    DisposeATSCubicLineToUPP(lineTo);
    DisposeATSCubicCurveToUPP(cubicTo);
    DisposeATSCubicClosePathUPP(closePath);
}

void QFontEngineMac::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int numGlyphs, QPainterPath *path,
                                           QTextItem::RenderFlags)
{
    ::addGlyphsToPath(style, glyphs, positions, numGlyphs, path);
}

QImage QFontEngineMac::alphaMapForGlyph(glyph_t glyph)
{
    const glyph_metrics_t br = boundingBox(glyph);
    QImage im(qRound(br.width)+2, qRound(br.height)+2, QImage::Format_RGB32);
    im.fill(0);

    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
    uint cgflags = kCGImageAlphaNoneSkipFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
    if(QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4)
        cgflags |= kCGBitmapByteOrder32Host;
#endif
#else
    CGImageAlphaInfo cgflags = kCGImageAlphaNoneSkipFirst;
#endif
    CGContextRef ctx = CGBitmapContextCreate(im.bits(), im.width(), im.height(),
                                             8, im.bytesPerLine(), colorspace,
                                             cgflags);
    CGColorSpaceRelease(colorspace);
    CGContextSetFontSize(ctx, fontDef.pixelSize);
    CGContextSetShouldAntialias(ctx, fontDef.pointSize > qt_antialiasing_threshold && !(fontDef.styleStrategy & QFont::NoAntialias));
    CGAffineTransform oldTextMatrix = CGContextGetTextMatrix(ctx);
    CGAffineTransform cgMatrix = CGAffineTransformMake(1, 0, 0, 1, 0, 0);
    CGAffineTransformConcat(cgMatrix, oldTextMatrix);

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, tanf(14 * acosf(0) / 90), 1, 0, 0));

    cgMatrix = CGAffineTransformConcat(cgMatrix, multiEngine->transform);

    CGContextSetTextMatrix(ctx, cgMatrix);
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextSetTextDrawingMode(ctx, kCGTextFill);
    CGContextSetFont(ctx, cgFont);

    qreal pos_x = -br.x.toReal()+1, pos_y = im.height()+br.y.toReal();
    CGContextSetTextPosition(ctx, pos_x, pos_y);

    CGSize advance;
    advance.width = 0;
    advance.height = 0;
    CGGlyph cgGlyph = glyph;
    CGContextShowGlyphsWithAdvances(ctx, &cgGlyph, &advance, 1);

    if (synthesisFlags & QFontEngine::SynthesizedBold) {
        CGContextSetTextPosition(ctx, pos_x + 0.5 * lineThickness().toReal(), pos_y);
        CGContextShowGlyphsWithAdvances(ctx, &cgGlyph, &advance, 1);
    }

    CGContextRelease(ctx);

    QImage indexed(im.width(), im.height(), QImage::Format_Indexed8);
    QVector<QRgb> colors(256);
    for (int i=0; i<256; ++i)
        colors[i] = qRgba(0, 0, 0, i);
    indexed.setColorTable(colors);

    for (int y=0; y<im.height(); ++y) {
        uint *src = (uint*) im.scanLine(y);
        uchar *dst = indexed.scanLine(y);
        for (int x=0; x<im.width(); ++x) {
            *dst = qGray(*src);
            ++dst;
            ++src;
        }
    }

    return indexed;
}

bool QFontEngineMac::canRender(const QChar *string, int len)
{
    Q_ASSERT(false);
    Q_UNUSED(string);
    Q_UNUSED(len);
    return false;
}

void QFontEngineMac::draw(CGContextRef ctx, qreal x, qreal y, const QTextItemInt &ti, int paintDeviceHeight)
{
    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix;
    matrix.translate(x, y);
    getGlyphPositions(ti.glyphs, ti.num_glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;

    CGContextSetFontSize(ctx, fontDef.pixelSize);

    CGAffineTransform oldTextMatrix = CGContextGetTextMatrix(ctx);

    CGAffineTransform cgMatrix = CGAffineTransformMake(1, 0, 0, -1, 0, -paintDeviceHeight);

    CGAffineTransformConcat(cgMatrix, oldTextMatrix);

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, -tanf(14 * acosf(0) / 90), 1, 0, 0));

    cgMatrix = CGAffineTransformConcat(cgMatrix, multiEngine->transform);

    CGContextSetTextMatrix(ctx, cgMatrix);

    CGContextSetTextDrawingMode(ctx, kCGTextFill);


    QVarLengthArray<CGSize> advances(glyphs.size());
    QVarLengthArray<CGGlyph> cgGlyphs(glyphs.size());

    for (int i = 0; i < glyphs.size() - 1; ++i) {
        advances[i].width = (positions[i + 1].x - positions[i].x).toReal();
        advances[i].height = (positions[i + 1].y - positions[i].y).toReal();
        cgGlyphs[i] = glyphs[i];
    }
    advances[glyphs.size() - 1].width = 0;
    advances[glyphs.size() - 1].height = 0;
    cgGlyphs[glyphs.size() - 1] = glyphs[glyphs.size() - 1];

    CGContextSetFont(ctx, cgFont);

    CGContextSetTextPosition(ctx, positions[0].x.toReal(), positions[0].y.toReal());

    CGContextShowGlyphsWithAdvances(ctx, cgGlyphs.data(), advances.data(), glyphs.size());

    if (synthesisFlags & QFontEngine::SynthesizedBold) {
        CGContextSetTextPosition(ctx, positions[0].x.toReal() + 0.5 * lineThickness().toReal(),
                                      positions[0].y.toReal());

        CGContextShowGlyphsWithAdvances(ctx, cgGlyphs.data(), advances.data(), glyphs.size());
    }

    CGContextSetTextMatrix(ctx, oldTextMatrix);
}

QFontEngine::FaceId QFontEngineMac::faceId() const
{
    FaceId ret;
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    FSRef ref;
    if (ATSFontGetFileReference(FMGetATSFontRefFromFont(fontID), &ref) != noErr)
        return ret;
    ret.filename = QByteArray(128, 0);
    ret.index = fontID;
    FSRefMakePath(&ref, (UInt8 *)ret.filename.data(), ret.filename.size());
#else
    FSSpec spec;
    if (ATSFontGetFileSpecification(FMGetATSFontRefFromFont(fontID), &spec) != noErr)
        return ret;

    FSRef ref;
    FSpMakeFSRef(&spec, &ref);
    ret.filename = QByteArray(128, 0);
    ret.index = fontID;
    FSRefMakePath(&ref, (UInt8 *)ret.filename.data(), ret.filename.size());
#endif
    return ret;
}

QByteArray QFontEngineMac::getSfntTable(uint tag) const
{
    ATSFontRef atsFont = FMGetATSFontRefFromFont(fontID);

    ByteCount length;
    OSStatus status = ATSFontGetTable(atsFont, tag, 0, 0, 0, &length);
    if (status != noErr)
        return QByteArray();
    QByteArray table(length, 0);
    status = ATSFontGetTable(atsFont, tag, 0, table.length(), table.data(), &length);
    if (status != noErr)
        return QByteArray();
    return table;
}

QFontEngine::Properties QFontEngineMac::properties() const
{
    QFontEngine::Properties props;
    
    ATSFontRef atsFont = FMGetATSFontRefFromFont(fontID);
    quint16 tmp;
    if (ATSFontGetTable(atsFont, MAKE_TAG('h', 'e', 'a', 'd'), 18, 2, &tmp, 0) == noErr)
       props.emSquare = qFromBigEndian<quint16>(tmp);
    struct {
        qint16 xMin;
        qint16 yMin;
        qint16 xMax;
        qint16 yMax;
    } bbox;
    bbox.xMin = bbox.xMax = bbox.yMin = bbox.yMax = 0;
    if (ATSFontGetTable(atsFont, MAKE_TAG('h', 'e', 'a', 'd'), 36, 8, &bbox, 0) == noErr) {
        bbox.xMin = qFromBigEndian<quint16>(bbox.xMin);
        bbox.yMin = qFromBigEndian<quint16>(bbox.yMin);
        bbox.xMax = qFromBigEndian<quint16>(bbox.xMax);
        bbox.yMax = qFromBigEndian<quint16>(bbox.yMax);
    }
    struct {
        qint16 ascender;
        qint16 descender;
        qint16 linegap;
    } metrics;
    metrics.ascender = metrics.descender = metrics.linegap = 0;
    if (ATSFontGetTable(atsFont, MAKE_TAG('h', 'h', 'e', 'a'), 4, 6, &metrics, 0) == noErr) {
        metrics.ascender = qFromBigEndian<quint16>(metrics.ascender);
        metrics.descender = qFromBigEndian<quint16>(metrics.descender);
        metrics.linegap = qFromBigEndian<quint16>(metrics.linegap);
    }
    props.ascent = metrics.ascender;
    props.descent = -metrics.descender;
    props.leading = metrics.linegap;
    props.boundingBox = QRectF(bbox.xMin, -bbox.yMax,
                           bbox.xMax - bbox.xMin,
                           bbox.yMax - bbox.yMin);
    props.italicAngle = 0;
    props.capHeight = props.ascent;

    qint16 lw = 0;
    if (ATSFontGetTable(atsFont, MAKE_TAG('p', 'o', 's', 't'), 10, 2, &lw, 0) == noErr)
       lw = qFromBigEndian<quint16>(lw);
    props.lineWidth = lw;
    
    QCFString psName;
    if (ATSFontGetPostScriptName(FMGetATSFontRefFromFont(fontID), kATSOptionFlagsDefault, &psName) == noErr)
        props.postscriptName = QString(psName).toUtf8();
    props.postscriptName = QPdf::stripSpecialCharacters(props.postscriptName);
    return props;
}

void QFontEngineMac::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metrics)
{
    ATSUStyle unscaledStyle;
    ATSUCreateAndCopyStyle(style, &unscaledStyle);

    int emSquare = properties().emSquare.toInt();
    
    const int maxAttributeCount = 4;
    ATSUAttributeTag tags[maxAttributeCount + 1];
    ByteCount sizes[maxAttributeCount + 1];
    ATSUAttributeValuePtr values[maxAttributeCount + 1];
    int attributeCount = 0;

    Fixed size = FixRatio(emSquare, 1);
    tags[attributeCount] = kATSUSizeTag;
    sizes[attributeCount] = sizeof(size);
    values[attributeCount] = &size;
    ++attributeCount;
    
    Q_ASSERT(attributeCount < maxAttributeCount + 1);
    OSStatus err = ATSUSetAttributes(unscaledStyle, attributeCount, tags, sizes, values);
    Q_ASSERT(err == noErr);
    Q_UNUSED(err);

    GlyphID atsuGlyph = glyph;
    ATSGlyphScreenMetrics atsuMetrics;
    ATSUGlyphGetScreenMetrics(unscaledStyle, 1, &atsuGlyph, 0,
                              /* iForcingAntiAlias =*/ false,
                              /* iAntiAliasSwitch =*/true,
                              &atsuMetrics);

    metrics->width = int(atsuMetrics.width);
    metrics->height = int(atsuMetrics.height);
    metrics->x = QFixed::fromReal(atsuMetrics.topLeft.x);
    metrics->y = -QFixed::fromReal(atsuMetrics.topLeft.y);
    metrics->xoff = QFixed::fromReal(atsuMetrics.deviceAdvance.x);
    metrics->yoff = QFixed::fromReal(atsuMetrics.deviceAdvance.y);

    QFixedPoint p;
    ::addGlyphsToPath(unscaledStyle, &glyph, &p, 1, path);

    ATSUDisposeStyle(unscaledStyle);
}

