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

#ifndef QABSTRACTFONTENGINE_QWS_H
#define QABSTRACTFONTENGINE_QWS_H

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>
#include <QtCore/qfactoryinterface.h>
#include <QtGui/qpaintengine.h>
#include <QtGui/qfontdatabase.h>

QT_BEGIN_HEADER

QT_MODULE(Gui)

class QFontEngineInfoPrivate;

class Q_GUI_EXPORT QFontEngineInfo
{
public:
    QDOC_PROPERTY(QString family READ family WRITE setFamily)
    QDOC_PROPERTY(qreal pixelSize READ pixelSize WRITE setPixelSize)
    QDOC_PROPERTY(int weight READ weight WRITE setWeight)
    QDOC_PROPERTY(QFont::Style style READ style WRITE setStyle)
    QDOC_PROPERTY(QList<QFontDatabase::WritingSystem> writingSystems READ writingSystems WRITE setWritingSystems)

    QFontEngineInfo();
    explicit QFontEngineInfo(const QString &family);
    QFontEngineInfo(const QFontEngineInfo &other);
    QFontEngineInfo &operator=(const QFontEngineInfo &other);
    ~QFontEngineInfo();

    void setFamily(const QString &name);
    QString family() const;

    void setPixelSize(qreal size);
    qreal pixelSize() const;

    void setWeight(int weight);
    int weight() const;

    void setStyle(QFont::Style style);
    QFont::Style style() const;

    QList<QFontDatabase::WritingSystem> writingSystems() const;
    void setWritingSystems(const QList<QFontDatabase::WritingSystem> &writingSystems);

private:
    QFontEngineInfoPrivate *d;
};

class QAbstractFontEngine;

struct Q_GUI_EXPORT QFontEngineFactoryInterface : public QFactoryInterface
{
     virtual QAbstractFontEngine *create(const QFontEngineInfo &info) = 0;
     virtual QList<QFontEngineInfo> availableFontEngines() const = 0;
};

#define QFontEngineFactoryInterface_iid "com.trolltech.Qt.QFontEngineFactoryInterface"
Q_DECLARE_INTERFACE(QFontEngineFactoryInterface, QFontEngineFactoryInterface_iid)

class QFontEnginePluginPrivate;

class Q_GUI_EXPORT QFontEnginePlugin : public QObject, public QFontEngineFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QFontEngineFactoryInterface:QFactoryInterface)
public:
    QFontEnginePlugin(const QString &foundry, QObject *parent = 0);
    ~QFontEnginePlugin();

    virtual QStringList keys() const;

    virtual QAbstractFontEngine *create(const QFontEngineInfo &info) = 0;
    virtual QList<QFontEngineInfo> availableFontEngines() const = 0;

private:
    Q_DECLARE_PRIVATE(QFontEnginePlugin)
    Q_DISABLE_COPY(QFontEnginePlugin)
};

class QAbstractFontEnginePrivate;

class Q_GUI_EXPORT QAbstractFontEngine : public QObject
{
    Q_OBJECT
public:
    enum Capability {
        CanOutlineGlyphs = 1,
        CanRenderGlyphs_Mono = 2,
        CanRenderGlyphs_Gray = 4,
        CanRenderGlyphs = CanRenderGlyphs_Mono | CanRenderGlyphs_Gray
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    explicit QAbstractFontEngine(QObject *parent = 0);
    ~QAbstractFontEngine();

    typedef int Fixed; // 26.6

    struct FixedPoint
    {
        Fixed x;
        Fixed y;
    };

    struct GlyphMetrics
    {
        inline GlyphMetrics()
            : x(0), y(0), width(0), height(0),
              advance(0) {}
        Fixed x;
        Fixed y;
        Fixed width;
        Fixed height;
        Fixed advance;
    };

    enum FontProperty {
        Ascent,
        Descent,
        Leading,
        XHeight,
        AverageCharWidth,
        LineThickness,
        UnderlinePosition,
        MaxCharWidth,
        MinLeftBearing,
        MinRightBearing,
        GlyphCount,

        // hints
        CacheGlyphsHint,
        OutlineGlyphsHint
    };

    // keep in sync with QTextEngine::ShaperFlag!!
    enum TextShapingFlag {
        RightToLeft         = 0x0001,
        ReturnDesignMetrics = 0x0002
    };
    Q_DECLARE_FLAGS(TextShapingFlags, TextShapingFlag)

    virtual Capabilities capabilities() const = 0;
    virtual QVariant fontProperty(FontProperty property) const = 0;

    virtual bool convertStringToGlyphIndices(const QChar *string, int length, uint *glyphs, int *numGlyphs, TextShapingFlags flags) const = 0;

    virtual void getGlyphAdvances(const uint *glyphs, int numGlyphs, Fixed *advances, TextShapingFlags flags) const = 0;

    virtual GlyphMetrics glyphMetrics(uint glyph) const = 0;

    virtual bool renderGlyph(uint glyph, int depth, int bytesPerLine, int height, uchar *buffer);

    virtual void addGlyphOutlinesToPath(uint *glyphs, int numGlyphs, FixedPoint *positions, QPainterPath *path);

    /*
    enum Extension {
        GetTrueTypeTable
    };

    virtual bool supportsExtension(Extension extension) const;
    virtual QVariant extension(Extension extension, const QVariant &argument = QVariant());
    */

private:
    Q_DECLARE_PRIVATE(QAbstractFontEngine)
    Q_DISABLE_COPY(QAbstractFontEngine)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractFontEngine::Capabilities)
Q_DECLARE_OPERATORS_FOR_FLAGS(QAbstractFontEngine::TextShapingFlags)

QT_END_HEADER

#endif
