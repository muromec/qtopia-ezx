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

#ifndef QTEXTENGINE_P_H
#define QTEXTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qglobal.h"
#include "QtCore/qstring.h"
#include "QtCore/qnamespace.h"
#include "QtGui/qtextlayout.h"
#include "private/qtextformat_p.h"
#include "private/qfont_p.h"
#include "QtCore/qvector.h"
#include "QtGui/qpaintengine.h"
#include "QtGui/qtextobject.h"
#include "QtGui/qtextoption.h"
#include "QtCore/qset.h"
#include "QtCore/qdebug.h"
#ifndef QT_BUILD_COMPAT_LIB
#include "private/qtextdocument_p.h"
#endif

#include <stdlib.h>

class QFontPrivate;
class QFontEngine;

class QString;
class QOpenType;
class QPainter;

class QAbstractTextDocumentLayout;

struct QFixed {
public:
    QFixed() : val(0) {}
    QFixed(int i) : val(i<<6) {}
    QFixed(long i) : val(i<<6) {}
    QFixed &operator=(int i) { val = (i<<6); return *this; }
    QFixed &operator=(long i) { val = (i<<6); return *this; }

    static QFixed fromReal(qreal r) { QFixed f; f.val = (int)(r*qreal(64)); return f; }
    static QFixed fromFixed(int fixed) { QFixed f; f.val = fixed; return f; }

    inline int value() const { return val; }
    inline void setValue(int value) { val = value; }

    inline int toInt() const { return (((val)+32) & -64)>>6; }
    inline qreal toReal() const { return ((qreal)val)/(qreal)64; }

    inline int truncate() const { return val>>6; }
    inline QFixed round() const { QFixed f; f.val = ((val)+32) & -64; return f; }
    inline QFixed floor() const { QFixed f; f.val = (val) & -64; return f; }
    inline QFixed ceil() const { QFixed f; f.val = (val+63) & -64; return f; }

    inline QFixed operator+(int i) const { QFixed f; f.val = (val + (i<<6)); return f; }
    inline QFixed operator+(uint i) const { QFixed f; f.val = (val + (i<<6)); return f; }
    inline QFixed operator+(const QFixed &other) const { QFixed f; f.val = (val + other.val); return f; }
    inline QFixed &operator+=(int i) { val += (i<<6); return *this; }
    inline QFixed &operator+=(uint i) { val += (i<<6); return *this; }
    inline QFixed &operator+=(const QFixed &other) { val += other.val; return *this; }
    inline QFixed operator-(int i) const { QFixed f; f.val = (val - (i<<6)); return f; }
    inline QFixed operator-(uint i) const { QFixed f; f.val = (val - (i<<6)); return f; }
    inline QFixed operator-(const QFixed &other) const { QFixed f; f.val = (val - other.val); return f; }
    inline QFixed &operator-=(int i) { val -= (i<<6); return *this; }
    inline QFixed &operator-=(uint i) { val -= (i<<6); return *this; }
    inline QFixed &operator-=(const QFixed &other) { val -= other.val; return *this; }
    inline QFixed operator-() const { QFixed f; f.val = -val; return f; }

    inline bool operator==(const QFixed &other) const { return val == other.val; }
    inline bool operator!=(const QFixed &other) const { return val != other.val; }
    inline bool operator<(const QFixed &other) const { return val < other.val; }
    inline bool operator>(const QFixed &other) const { return val > other.val; }
    inline bool operator<=(const QFixed &other) const { return val <= other.val; }
    inline bool operator>=(const QFixed &other) const { return val >= other.val; }
    inline bool operator!() const { return !val; }

    inline QFixed &operator/=(int x) { val /= x; return *this; }
    inline QFixed &operator/=(const QFixed &o) {
        if (o.val == 0) {
            val = 0x7FFFFFFFL;
        } else {
            bool neg = false;
            qint64 a = val;
            qint64 b = o.val;
            if (a < 0) { a = -a; neg = true; }
            if (b < 0) { b = -b; neg = !neg; }

            int res = (int)(((a << 6) + (b >> 1)) / b);

            val = (neg ? -res : res);
        }
        return *this;
    }
    inline QFixed operator/(int d) const { QFixed f; f.val = val/d; return f; }
    inline QFixed operator/(QFixed b) const { QFixed f = *this; return (f /= b); }
    inline QFixed operator>>(int d) const { QFixed f = *this; f.val >>= d; return f; }
    inline QFixed &operator*=(int i) { val *= i; return *this; }
    inline QFixed &operator*=(uint i) { val *= i; return *this; }
    inline QFixed &operator*=(const QFixed &o) {
        bool neg = false;
        qint64 a = val;
        qint64 b = o.val;
        if (a < 0) { a = -a; neg = true; }
        if (b < 0) { b = -b; neg = !neg; }

        int res = (int)((a * b + 0x20L) >> 6);
        val = neg ? -res : res;
        return *this;
    }
    inline QFixed operator*(int i) const { QFixed f = *this; return (f *= i); }
    inline QFixed operator*(uint i) const { QFixed f = *this; return (f *= i); }
    inline QFixed operator*(const QFixed &o) const { QFixed f = *this; return (f *= o); }

private:
    QFixed(qreal i) : val((int)(i*qreal(64))) {}
    QFixed &operator=(qreal i) { val = (int)(i*qreal(64)); return *this; }
    inline QFixed operator+(qreal i) const { QFixed f; f.val = (val + (int)(i*qreal(64))); return f; }
    inline QFixed &operator+=(qreal i) { val += (int)(i*64); return *this; }
    inline QFixed operator-(qreal i) const { QFixed f; f.val = (val - (int)(i*qreal(64))); return f; }
    inline QFixed &operator-=(qreal i) { val -= (int)(i*64); return *this; }
    inline QFixed &operator/=(qreal r) { val = (int)(val/r); return *this; }
    inline QFixed operator/(qreal d) const { QFixed f; f.val = (int)(val/d); return f; }
    inline QFixed &operator*=(qreal d) { val = (int) (val*d); return *this; }
    inline QFixed operator*(qreal d) const { QFixed f = *this; return (f *= d); }
    int val;
};
Q_DECLARE_TYPEINFO(QFixed, Q_PRIMITIVE_TYPE);

#define QFIXED_MAX (INT_MAX/256)

inline int qRound(const QFixed &f) { return f.toInt(); }

inline QFixed operator*(int i, const QFixed &d) { return d*i; }
inline QFixed operator+(int i, const QFixed &d) { return d+i; }
inline QFixed operator-(int i, const QFixed &d) { return -(d-i); }
inline QFixed operator*(uint i, const QFixed &d) { return d*i; }
inline QFixed operator+(uint i, const QFixed &d) { return d+i; }
inline QFixed operator-(uint i, const QFixed &d) { return -(d-i); }
// inline QFixed operator*(qreal d, const QFixed &d2) { return d2*d; }

inline bool operator==(const QFixed &f, int i) { return f.value() == (i<<6); }
inline bool operator==(int i, const QFixed &f) { return f.value() == (i<<6); }
inline bool operator!=(const QFixed &f, int i) { return f.value() != (i<<6); }
inline bool operator!=(int i, const QFixed &f) { return f.value() != (i<<6); }
inline bool operator<=(const QFixed &f, int i) { return f.value() <= (i<<6); }
inline bool operator<=(int i, const QFixed &f) { return (i<<6) <= f.value(); }
inline bool operator>=(const QFixed &f, int i) { return f.value() >= (i<<6); }
inline bool operator>=(int i, const QFixed &f) { return (i<<6) >= f.value(); }
inline bool operator<(const QFixed &f, int i) { return f.value() < (i<<6); }
inline bool operator<(int i, const QFixed &f) { return (i<<6) < f.value(); }
inline bool operator>(const QFixed &f, int i) { return f.value() > (i<<6); }
inline bool operator>(int i, const QFixed &f) { return (i<<6) > f.value(); }

inline QDebug &operator<<(QDebug &dbg, const QFixed &f)
{ return dbg << f.toReal(); }

struct QFixedPoint {
    QFixed x;
    QFixed y;
    inline QFixedPoint() {}
    inline QFixedPoint(const QFixed &_x, const QFixed &_y) : x(_x), y(_y) {}
    QPointF toPointF() const { return QPointF(x.toReal(), y.toReal()); }
    static QFixedPoint fromPointF(const QPointF &p) {
        return QFixedPoint(QFixed::fromReal(p.x()), QFixed::fromReal(p.y()));
    }
};
Q_DECLARE_TYPEINFO(QFixedPoint, Q_PRIMITIVE_TYPE);

inline QFixedPoint operator-(const QFixedPoint &p1, const QFixedPoint &p2)
{ return QFixedPoint(p1.x - p2.x, p1.y - p2.y); }
inline QFixedPoint operator+(const QFixedPoint &p1, const QFixedPoint &p2)
{ return QFixedPoint(p1.x + p2.x, p1.y + p2.y); }

struct QFixedSize {
    QFixed width;
    QFixed height;
    QSizeF toSizeF() const { return QSizeF(width.toReal(), height.toReal()); }
    static QFixedSize fromSizeF(const QSizeF &s) {
        QFixedSize size;
        size.width = QFixed::fromReal(s.width());
        size.height = QFixed::fromReal(s.height());
        return size;
    }
};
Q_DECLARE_TYPEINFO(QFixedSize, Q_PRIMITIVE_TYPE);

struct QScriptItem;
class QTextItemInt : public QTextItem
{
public:
    inline QTextItemInt()
        : justified(false), underlineStyle(QTextCharFormat::NoUnderline), num_chars(0), chars(0),
          logClusters(0), f(0), glyphs(0), num_glyphs(0), fontEngine(0)
    {}
    QTextItemInt(const QScriptItem &si, QFont *font, const QTextCharFormat &format = QTextCharFormat());

    QFixed descent;
    QFixed ascent;
    QFixed width;

    RenderFlags flags;
    bool justified;
    QTextCharFormat::UnderlineStyle underlineStyle;
    int num_chars;
    const QChar *chars;
    const unsigned short *logClusters;
    const QFont *f;
    QColor underlineColor;

    QGlyphLayout *glyphs;
    int num_glyphs;
    QFontEngine *fontEngine;
};


// this uses the same coordinate system as Qt, but a different one to freetype.
// * y is usually negative, and is equal to the ascent.
// * negative yoff means the following stuff is drawn higher up.
// the characters bounding rect is given by QRect(x,y,width,height), it's advance by
// xoo and yoff
struct glyph_metrics_t
{
    inline glyph_metrics_t()
        : x(100000),  y(100000) {}
    inline glyph_metrics_t(QFixed _x, QFixed _y, QFixed _width, QFixed _height, QFixed _xoff, QFixed _yoff)
        : x(_x),
          y(_y),
          width(_width),
          height(_height),
          xoff(_xoff),
          yoff(_yoff)
        {}
    QFixed x;
    QFixed y;
    QFixed width;
    QFixed height;
    QFixed xoff;
    QFixed yoff;
};
Q_DECLARE_TYPEINFO(glyph_metrics_t, Q_PRIMITIVE_TYPE);

typedef unsigned int glyph_t;

#if defined(Q_WS_X11) || defined (Q_WS_QWS) || defined (Q_WS_MAC)


struct QScriptAnalysis
{
    unsigned short script    : 7;
    unsigned short override  : 1;  // Set when in LRO/RLO embedding
    unsigned short bidiLevel : 6;  // Unicode Bidi algorithm embedding level (0-61)
    unsigned short reserved  : 2;
    bool operator == (const QScriptAnalysis &other) {
        return
            script == other.script &&
            bidiLevel == other.bidiLevel;
        // ###
//             && override == other.override;
    }

};
Q_DECLARE_TYPEINFO(QScriptAnalysis, Q_PRIMITIVE_TYPE);

#elif defined(Q_WS_WIN)

struct QScriptAnalysis {
    unsigned short script         :10;
    unsigned short rtl            :1;
    unsigned short layoutRTL      :1;
    unsigned short linkBefore     :1;
    unsigned short linkAfter      :1;
    unsigned short logicalOrder   :1;
    unsigned short noGlyphIndex   :1;
    unsigned short bidiLevel         :5;
    unsigned short override          :1;
    unsigned short inhibitSymSwap    :1;
    unsigned short charShape         :1;
    unsigned short digitSubstitute   :1;
    unsigned short inhibitLigate     :1;
    unsigned short fDisplayZWG        :1;
    unsigned short arabicNumContext  :1;
    unsigned short gcpClusters       :1;
    unsigned short reserved          :1;
    unsigned short engineReserved    :2;
};
Q_DECLARE_TYPEINFO(QScriptAnalysis, Q_PRIMITIVE_TYPE);

inline bool operator == (const QScriptAnalysis &sa1, const QScriptAnalysis &sa2)
{
    return
        sa1.script == sa2.script &&
        sa1.bidiLevel == sa2.bidiLevel;
        // ###
//             && override == other.override;
}

#endif

struct QGlyphLayout
{
    inline QGlyphLayout()
        : glyph(0), justificationType(0), nKashidas(0), space_18d6(0)
        {}

    // highest value means highest priority for justification. Justification is done by first inserting kashidas
    // starting with the highest priority positions, then stretching spaces, afterwards extending inter char
    // spacing, and last spacing between arabic words.
    // NoJustification is for example set for arabic where no Kashida can be inserted or for diacritics.
    enum Justification {
        NoJustification= 0,   // Justification can't be applied after this glyph
        Arabic_Space   = 1,   // This glyph represents a space inside arabic text
        Character      = 2,   // Inter-character justification point follows this glyph
        Space          = 4,   // This glyph represents a blank outside an Arabic run
        Arabic_Normal  = 7,   // Normal Middle-Of-Word glyph that connects to the right (begin)
        Arabic_Waw     = 8,    // Next character is final form of Waw/Ain/Qaf/Fa
        Arabic_BaRa    = 9,   // Next two chars are Ba + Ra/Ya/AlefMaksura
        Arabic_Alef    = 10,  // Next character is final form of Alef/Tah/Lam/Kaf/Gaf
        Arabic_HaaDal  = 11,  // Next character is final form of Haa/Dal/Taa Marbutah
        Arabic_Seen    = 12,  // Initial or Medial form Of Seen/Sad
        Arabic_Kashida = 13   // Kashida(U+640) in middle of word
    };

    glyph_t glyph;
    struct Attributes {
        unsigned short justification   :4;  // Justification class
        unsigned short clusterStart    :1;  // First glyph of representation of cluster
        unsigned short mark            :1;  // needs to be positioned around base char
        unsigned short zeroWidth       :1;  // ZWJ, ZWNJ etc, with no width
        unsigned short dontPrint       :1;
        unsigned short combiningClass  :8;
    };
    Attributes attributes;
    QFixedPoint advance;
    QFixedPoint offset;

    enum JustificationType {
        JustifyNone,
        JustifySpace,
        JustifyKashida
    };
    uint justificationType :2;
    uint nKashidas : 6; // more do not make sense...
    uint space_18d6 : 24;
};
Q_DECLARE_TYPEINFO(QGlyphLayout, Q_PRIMITIVE_TYPE);

inline bool qIsControlChar(ushort uc)
{
    return uc >= 0x200b && uc <= 0x206f
        && (uc <= 0x200f /* ZW Space, ZWNJ, ZWJ, LRM and RLM */
            || (uc >= 0x2028 && uc <= 0x202f /* LS, PS, LRE, RLE, PDF, LRO, RLO, NNBSP */)
            || uc >= 0x206a /* ISS, ASS, IAFS, AFS, NADS, NODS */);
}


struct QCharAttributes {
    enum LineBreakType {
        NoBreak,
        SoftHyphen,
        Break,
        ForcedBreak
    };
    uchar lineBreakType  :2;
    uchar whiteSpace     :1;     // A unicode whitespace character, except NBSP, ZWNBSP
    uchar charStop       :1;     // Valid cursor position (for left/right arrow)
};
Q_DECLARE_TYPEINFO(QCharAttributes, Q_PRIMITIVE_TYPE);

struct QScriptItem
{
    inline QScriptItem() : position(0), isSpace(false), isTab(false),
                           isObject(false),
                           num_glyphs(0), descent(-1), ascent(-1), width(-1),
                           glyph_data_offset(0) {}

    int position;
    QScriptAnalysis analysis;
    unsigned short isSpace  : 1;
    unsigned short isTab    : 1;
    unsigned short isObject : 1;
    int num_glyphs;
    QFixed descent;
    QFixed ascent;
    QFixed width;
    int glyph_data_offset;
    QFixed height() const { return ascent + descent; }
};


Q_DECLARE_TYPEINFO(QScriptItem, Q_MOVABLE_TYPE);

typedef QVector<QScriptItem> QScriptItemArray;

struct QScriptLine
{
    QScriptLine()
        : from(0), length(0),
        justified(0), gridfitted(0),
        hasTrailingSpaces(0) {}
    QFixed descent;
    QFixed ascent;
    QFixed x;
    QFixed y;
    QFixed width;
    QFixed textWidth;
    int from;
    signed int length : 29;
    mutable uint justified : 1;
    mutable uint gridfitted : 1;
    uint hasTrailingSpaces : 1;
    QFixed height() const { return ascent + descent + 1; }
    void setDefaultHeight(QTextEngine *eng);
    void operator+=(const QScriptLine &other);
};
Q_DECLARE_TYPEINFO(QScriptLine, Q_PRIMITIVE_TYPE);


inline void QScriptLine::operator+=(const QScriptLine &other)
{
    descent = qMax(descent, other.descent);
    ascent = qMax(ascent, other.ascent);
    textWidth += other.textWidth;
    length += other.length;
}

typedef QVector<QScriptLine> QScriptLineArray;

class QFontPrivate;
class QTextFormatCollection;

class Q_GUI_EXPORT QTextEngine {
public:
    struct LayoutData {
        LayoutData(const QString &str, void **stack_memory, int mem_size);
        LayoutData();
        ~LayoutData();
        mutable QScriptItemArray items;
        int allocated;
        int available_glyphs;
        void **memory;
        unsigned short *logClustersPtr;
        QGlyphLayout *glyphPtr;
        int num_glyphs;
        mutable int used;
        uint hasBidi : 1;
        uint inLayout : 1;
        uint memory_on_stack : 1;
        bool haveCharAttributes;
        QString string;
        void reallocate(int totalGlyphs);
    };

    QTextEngine(LayoutData *data);
    QTextEngine();
    QTextEngine(const QString &str, const QFont &f);
    ~QTextEngine();

    enum Mode {
        WidthOnly = 0x07
    };

    // keep in sync with QAbstractFontEngine::TextShapingFlag!!
    enum ShaperFlag {
        RightToLeft = 0x0001,
        DesignMetrics = 0x0002,
        GlyphIndicesOnly = 0x0004
    };
    Q_DECLARE_FLAGS(ShaperFlags, ShaperFlag)

    void invalidate();

    void validate() const;
    void itemize() const;

    static void bidiReorder(int numRuns, const quint8 *levels, int *visualOrder);

    const QCharAttributes *attributes() const;

    void shape(int item) const;

    void justify(const QScriptLine &si);

    QFixed width(int charFrom, int numChars) const;
    glyph_metrics_t boundingBox(int from,  int len) const;
    glyph_metrics_t tightBoundingBox(int from,  int len) const;

    int length(int item) const {
        const QScriptItem &si = layoutData->items[item];
        int from = si.position;
        item++;
        return (item < layoutData->items.size() ? layoutData->items[item].position : layoutData->string.length()) - from;
    }
    int length(const QScriptItem *si) const {
        int end;
        if (si + 1 < layoutData->items.constData()+ layoutData->items.size())
            end = (si+1)->position;
        else
            end = layoutData->string.length();
        return end - si->position;
    }

    QFontEngine *fontEngine(const QScriptItem &si, QFixed *ascent = 0, QFixed *descent = 0) const;
    QFont font(const QScriptItem &si) const;
    inline QFont font() const { return fnt; }

    inline unsigned short *logClusters(const QScriptItem *si) const
        { return layoutData->logClustersPtr+si->position; }
    inline QGlyphLayout *glyphs(const QScriptItem *si) const
        { return layoutData->glyphPtr + si->glyph_data_offset; }

    inline void ensureSpace(int nGlyphs) const {
        if (layoutData->num_glyphs - layoutData->used < nGlyphs)
            layoutData->reallocate((((layoutData->used + nGlyphs)*3/2 + 15) >> 4) << 4);
    }

    void freeMemory();

    int findItem(int strPos) const;
    inline QTextFormatCollection *formats() const {
#ifdef QT_BUILD_COMPAT_LIB
        return 0; // Compat should never reference this symbol
#else
        return block.docHandle()->formatCollection();
#endif
    }
    QTextCharFormat format(const QScriptItem *si) const;
    inline QAbstractTextDocumentLayout *docLayout() const {
#ifdef QT_BUILD_COMPAT_LIB
        return 0; // Compat should never reference this symbol
#else
        return block.docHandle()->document()->documentLayout();
#endif
    }
    int formatIndex(const QScriptItem *si) const;

    QFixed nextTab(const QScriptItem *si, QFixed x) const;

    mutable QScriptLineArray lines;

    QString text;
    QFont fnt;
    QTextBlock block;

    QTextOption option;

    QFixed minWidth;
    QFixed maxWidth;
    QPointF position;
    uint ignoreBidi : 1;
    uint cacheGlyphs : 1;
    uint stackEngine : 1;
    uint forceJustification : 1;

    int *underlinePositions;

    mutable LayoutData *layoutData;

    inline bool hasFormats() const { return (block.docHandle() || specialData); }

    struct SpecialData {
        int preeditPosition;
        QString preeditText;
        QList<QTextLayout::FormatRange> addFormats;
        QVector<int> addFormatIndices;
        QVector<int> resolvedFormatIndices;
    };
    SpecialData *specialData;

    bool atWordSeparator(int position) const;
    void indexAdditionalFormats();

    QString elidedText(Qt::TextElideMode mode, const QFixed &width, int flags = 0) const;

private:
    void setBoundary(int strPos) const;
    void addRequiredBoundaries() const;
    void shapeText(int item) const;
    void splitItem(int item, int pos) const;

    void resolveAdditionalFormats() const;
};

class QStackTextEngine : public QTextEngine {
public:
    enum { MemSize = 256*40/sizeof(void *) };
    QStackTextEngine(const QString &string, const QFont &f);
    LayoutData _layoutData;
    void *_memory[MemSize];
};


Q_DECLARE_OPERATORS_FOR_FLAGS(QTextEngine::ShaperFlags)


#endif // QTEXTENGINE_P_H
