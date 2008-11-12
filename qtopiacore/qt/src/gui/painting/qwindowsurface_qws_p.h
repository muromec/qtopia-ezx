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

#ifndef QWINDOWSURFACE_QWS_P_H
#define QWINDOWSURFACE_QWS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwindowsurface_p.h"
#include <qregion.h>
#include <qimage.h>
#include <qdirectpainter_qws.h>
#include <private/qsharedmemory_p.h>

class QScreen;
class QWSWindowSurfacePrivate;

class Q_GUI_EXPORT QWSWindowSurface : public QWindowSurface
{
public:
    QWSWindowSurface();
    QWSWindowSurface(QWidget *widget);
    ~QWSWindowSurface();

    virtual bool isValid() const = 0;

    virtual void setGeometry(const QRect &rect);
    virtual void setGeometry(const QRect &rect, const QRegion &mask);
    virtual void flush(QWidget *widget, const QRegion &region,
                       const QPoint &offset);

    virtual bool move(const QPoint &offset);
    virtual QRegion move(const QPoint &offset, const QRegion &newClip);

    virtual QPoint painterOffset() const; // remove!!!

    virtual void beginPaint(const QRegion &);
    virtual void endPaint(const QRegion &);

    virtual bool lock(int timeout = -1);
    virtual void unlock();

    virtual QString key() const = 0;

    // XXX: not good enough
    virtual QByteArray transientState() const;
    virtual QByteArray permanentState() const;
    virtual void setTransientState(const QByteArray &state);
    virtual void setPermanentState(const QByteArray &state);

    virtual QImage image() const = 0;
    virtual QPaintDevice *paintDevice() = 0;


    const QRegion dirtyRegion() const;
    void setDirty(const QRegion &) const;

    const QRegion clipRegion() const;
    void setClipRegion(const QRegion &);

    enum SurfaceFlag {
        RegionReserved = 0x1,
        Buffered = 0x2,
        Opaque = 0x4
    };
    Q_DECLARE_FLAGS(SurfaceFlags, SurfaceFlag)

    SurfaceFlags surfaceFlags() const;

    inline bool isRegionReserved() const {
        return surfaceFlags() & RegionReserved;
    }
    inline bool isBuffered() const { return surfaceFlags() & Buffered; }
    inline bool isOpaque() const { return surfaceFlags() & Opaque; }

    int winId() const;

protected:
    void setSurfaceFlags(SurfaceFlags type);
    void setWinId(int id);

private:
    friend class QWidgetPrivate;

    void invalidateBuffer();

    QWSWindowSurfacePrivate *d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWSWindowSurface::SurfaceFlags)

class QWSLock;

class Q_GUI_EXPORT QWSMemorySurface : public QWSWindowSurface
{
public:
    QWSMemorySurface();
    QWSMemorySurface(QWidget *widget);
    ~QWSMemorySurface();

    bool isValid() const;

    QPaintDevice *paintDevice() { return &img; }
    bool scroll(const QRegion &area, int dx, int dy);

    QPixmap grabWidget(const QWidget *widget, const QRect &rectangle) const;
    QImage image() const { return img; };
    QPoint painterOffset() const;

    bool lock(int timeout = -1);
    void unlock();

protected:
    QImage::Format preferredImageFormat(const QWidget *widget) const;

#ifndef QT_NO_QWS_MULTIPROCESS
    void setLock(int lockId);
    QWSLock *memlock;
#endif

    QImage img;
};

class Q_GUI_EXPORT QWSLocalMemSurface : public QWSMemorySurface
{
public:
    QWSLocalMemSurface();
    QWSLocalMemSurface(QWidget *widget);
    ~QWSLocalMemSurface();

    void setGeometry(const QRect &rect);

    QString key() const { return QLatin1String("mem"); }
    QByteArray permanentState() const;

    void setPermanentState(const QByteArray &data);

protected:
    uchar *mem;
    int memsize;
};

#ifndef QT_NO_QWS_MULTIPROCESS
class Q_GUI_EXPORT QWSSharedMemSurface : public QWSMemorySurface
{
public:
    QWSSharedMemSurface();
    QWSSharedMemSurface(QWidget *widget);
    ~QWSSharedMemSurface();

    void setGeometry(const QRect &rect);

    QString key() const { return QLatin1String("shm"); }
    QByteArray permanentState() const;

    void setPermanentState(const QByteArray &data);

private:
    bool setMemory(int memId);

    QSharedMemory mem;
};
#endif // QT_NO_QWS_MULTIPROCESS

#ifndef QT_NO_PAINTONSCREEN
class Q_GUI_EXPORT QWSOnScreenSurface : public QWSMemorySurface
{
public:
    QWSOnScreenSurface();
    QWSOnScreenSurface(QWidget *widget);
    ~QWSOnScreenSurface();

    bool isValid() const;
    QPoint painterOffset() const;

    QString key() const { return QLatin1String("OnScreen"); }
    QByteArray permanentState() const;

    void setPermanentState(const QByteArray &data);

private:
    void attachToScreen(const QScreen *screen);

    const QScreen *screen;
};
#endif // QT_NO_PAINTONSCREEN

#ifndef QT_NO_PAINT_DEBUG
class Q_GUI_EXPORT QWSYellowSurface : public QWSWindowSurface
{
public:
    QWSYellowSurface(bool isClient = false);
    ~QWSYellowSurface();

    void setDelay(int msec) { delay = msec; }

    bool isValid() const { return true; }

    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);

    QString key() const { return QLatin1String("Yellow"); }
    QByteArray permanentState() const;

    void setPermanentState(const QByteArray &data);

    QPaintDevice *paintDevice() { return &img; }
    QImage image() const { return img; }

private:
    int delay;
    QSize surfaceSize; // client side
    QImage img; // server side
};
#endif // QT_NO_PAINT_DEBUG

#ifndef QT_NO_DIRECTPAINTER

class QScreen;

class Q_GUI_EXPORT QWSDirectPainterSurface : public QWSWindowSurface
{
public:
    QWSDirectPainterSurface(bool isClient = false,
                            QDirectPainter::SurfaceFlag flags = QDirectPainter::NonReserved);
    ~QWSDirectPainterSurface();

    void setReserved() { setSurfaceFlags(RegionReserved); }

    void setGeometry(const QRect &rect) { setRegion(rect); }

    void setRegion(const QRegion &region);
    QRegion region() const { return clipRegion(); }

    void flush(QWidget*, const QRegion &, const QPoint &);

    bool isValid() const { return false; }

    QString key() const { return QLatin1String("DirectPainter"); }
    QByteArray permanentState() const;

    void setPermanentState(const QByteArray &);

    QImage image() const { return QImage(); }
    QPaintDevice *paintDevice() { return 0; }

    // hw: get rid of this
    WId windowId() const { return static_cast<WId>(winId()); }

    QScreen *screen() const { return _screen; }

    void beginPaint(const QRegion &);
    bool lock(int timeout = -1);
    void unlock();

private:
    QScreen *_screen;

    friend void qt_directpainter_region(QDirectPainter*, const QRegion&, int);
    bool flushingRegionEvents;
    bool synchronous;
};

#endif // QT_NO_DIRECTPAINTER

#endif // QWINDOWSURFACE_QWS_P_H
