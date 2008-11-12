/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the plugins of the Qt Toolkit.
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
#include "qsvgiconengine.h"

#include "qpainter.h"
#include "qpixmap.h"
#include "qsvgrenderer.h"
#include "qpixmapcache.h"
#include "qstyle.h"
#include "qapplication.h"
#include "qstyleoption.h"
#include "qfileinfo.h"
#include "qdebug.h"


struct QSvgCacheEntry
{
    QSvgCacheEntry()
        : mode(QIcon::Normal), state(QIcon::Off){}
    QSvgCacheEntry(const QPixmap &pm, QIcon::Mode m = QIcon::Normal, QIcon::State s = QIcon::Off)
        : pixmap(pm), mode(m), state(s){}
    QPixmap pixmap;
    QIcon::Mode mode;
    QIcon::State state;
};

class QSvgIconEnginePrivate : public QSharedData
{
public:
    explicit QSvgIconEnginePrivate()
    {
        render = new QSvgRenderer;
    }
    ~QSvgIconEnginePrivate()
    {
        delete render;
        render = 0;
    }

    QSvgRenderer *render;
    QHash<int, QSvgCacheEntry> svgCache;
    QString svgFile;
};
static inline int area(const QSize &s) { return s.width() * s.height(); }

static inline int createKey(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return ((((((size.width()<<11)|size.height())<<11)|mode)<<4)|state);
}

QSvgIconEngine::QSvgIconEngine()
    : d(new QSvgIconEnginePrivate)
{

}

QSvgIconEngine::QSvgIconEngine(const QSvgIconEngine &other)
    : QIconEngineV2(other), d(new QSvgIconEnginePrivate)
{
    d->render->load(other.d->svgFile);
    d->svgCache = other.d->svgCache;
}


QSvgIconEngine::~QSvgIconEngine()
{
}


QSize QSvgIconEngine::actualSize(const QSize &size, QIcon::Mode,
                                 QIcon::State )
{
    return size;
}


QPixmap QSvgIconEngine::pixmap(const QSize &size, QIcon::Mode mode,
                               QIcon::State state)
{
    int index = createKey(size, mode, state);
    if (d->svgCache.contains(index))
        return d->svgCache.value(index).pixmap;
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    QPainter p(&img);
    d->render->render(&p);
    p.end();
    QPixmap pm = QPixmap::fromImage(img);
    QStyleOption opt(0);
    opt.palette = QApplication::palette();
    QPixmap generated = QApplication::style()->generatedIconPixmap(mode, pm, &opt);
    if (!generated.isNull())
        pm = generated;

    d->svgCache.insert(index, QSvgCacheEntry(pm, mode, state));

    return pm;
}


void QSvgIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                               QIcon::State state)
{
    int index = createKey(pixmap.size(), mode, state);
    d->svgCache.insert(index, pixmap);
}


void QSvgIconEngine::addFile(const QString &fileName, const QSize &,
                             QIcon::Mode, QIcon::State)
{
    if (!fileName.isEmpty()) {
        QString abs = fileName;
        if (fileName.at(0) != QLatin1Char(':'))
            abs = QFileInfo(fileName).absoluteFilePath();
        d->svgFile = abs;
        d->render->load(abs);
        //qDebug()<<"loaded "<<abs<<", isOK = "<<d->render->isValid();
    }
}

void QSvgIconEngine::paint(QPainter *painter, const QRect &rect,
                           QIcon::Mode mode, QIcon::State state)
{
    painter->drawPixmap(rect, pixmap(rect.size(), mode, state));
}

QString QSvgIconEngine::key() const
{
    return QLatin1String("svg");
}

QIconEngineV2 *QSvgIconEngine::clone() const
{
    return new QSvgIconEngine(*this);
}

bool QSvgIconEngine::read(QDataStream &in)
{
    QPixmap pixmap;
    QByteArray data;
    uint mode;
    uint state;
    int num_entries;

    in >> data;
    if (!data.isEmpty()) {
#ifndef QT_NO_COMPRESS
        data = qUncompress(data);
#endif
        if (!data.isEmpty())
            d->render->load(data);
    }
    in >> num_entries;
    for (int i=0; i<num_entries; ++i) {
        if (in.atEnd()) {
            d->svgCache.clear();
            return false;
        }
        in >> pixmap;
        in >> mode;
        in >> state;
        addPixmap(pixmap, QIcon::Mode(mode), QIcon::State(state));
    }
    return true;
}

bool QSvgIconEngine::write(QDataStream &out) const
{
    if (!d->svgFile.isEmpty()) {
        QFile file(d->svgFile);
        if (file.open(QIODevice::ReadOnly))
#ifndef QT_NO_COMPRESS
            out << qCompress(file.readAll());
#else
            out << file.readAll();
#endif
        else
            out << QByteArray();
    } else {
        out << QByteArray();
    }
    QList<int> keys = d->svgCache.keys();
    out << keys.size();
    for (int i=0; i<keys.size(); ++i) {
        out << d->svgCache.value(keys.at(i)).pixmap;
        out << (uint) d->svgCache.value(keys.at(i)).mode;
        out << (uint) d->svgCache.value(keys.at(i)).state;
    }
    return true;
}
