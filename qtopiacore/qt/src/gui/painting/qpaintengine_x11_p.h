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

#ifndef QPAINTENGINE_X11_P_H
#define QPAINTENGINE_X11_P_H

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

#include "QtGui/qpaintengine.h"
#include "QtGui/qregion.h"
#include "QtGui/qpen.h"
#include "QtCore/qpoint.h"
#include "private/qpaintengine_p.h"
#include "private/qpainter_p.h"
#include "private/qpolygonclipper_p.h"

class QX11PaintEnginePrivate;
class QFontEngineFT;
class QXRenderTessellator;

typedef unsigned long Picture;

struct qt_float_point
{
    qreal x, y;
};

class QX11PaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QX11PaintEngine)
public:
    QX11PaintEngine();
    ~QX11PaintEngine();

    bool begin(QPaintDevice *pdev);
    bool end();

    void updateState(const QPaintEngineState &state);

    void updatePen(const QPen &pen);
    void updateBrush(const QBrush &brush, const QPointF &pt);
    void updateRenderHints(QPainter::RenderHints hints);
    void updateFont(const QFont &font);
    void updateMatrix(const QTransform &matrix);
    void updateClipRegion_dev(const QRegion &region, Qt::ClipOperation op);

    void drawLines(const QLine *lines, int lineCount);
    inline void drawLines(const QLineF *lines, int lineCount) { QPaintEngine::drawLines(lines, lineCount); }

    void drawRects(const QRect *rects, int rectCount);
    inline void drawRects(const QRectF *rects, int rectCount) { QPaintEngine::drawRects(rects, rectCount); }

    void drawPoints(const QPoint *points, int pointCount);
    void drawPoints(const QPointF *points, int pointCount);

    void drawEllipse(const QRect &r);
    inline void drawEllipse(const QRectF &r) { QPaintEngine::drawEllipse(r); }

    virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    inline void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode)
        { QPaintEngine::drawPolygon(points, pointCount, mode); }

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s);
    void drawPath(const QPainterPath &path);
    void drawTextItem(const QPointF &p, const QTextItem &textItem);
    void drawImage(const QRectF &r, const QImage &img, const QRectF &sr,
                   Qt::ImageConversionFlags flags = Qt::AutoColor);

    virtual Qt::HANDLE handle() const;
    inline Type type() const { return QPaintEngine::X11; }

    QPainter::RenderHints supportedRenderHints() const;

protected:
    QX11PaintEngine(QX11PaintEnginePrivate &dptr);

    void drawBox(const QPointF &p, const QTextItemInt &si);
    void drawXLFD(const QPointF &p, const QTextItemInt &si);
#ifndef QT_NO_FONTCONFIG
    void drawFreetype(const QPointF &p, const QTextItemInt &si);
#endif

    friend class QPixmap;
    friend class QFontEngineBox;
    friend Q_GUI_EXPORT GC qt_x11_get_pen_gc(QPainter *);
    friend Q_GUI_EXPORT GC qt_x11_get_brush_gc(QPainter *);

private:
    Q_DISABLE_COPY(QX11PaintEngine)
};

class QX11PaintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QX11PaintEngine)
public:
    QX11PaintEnginePrivate()
    {
        scrn = -1;
        hd = 0;
        picture = 0;
        gc = gc_brush = 0;
        dpy  = 0;
        xinfo = 0;
        txop = QTransform::TxNone;
        has_clipping = false;
        render_hints = 0;
#ifndef QT_NO_XRENDER
        tessellator = 0;
#endif
    }
    enum GCMode {
        PenGC,
        BrushGC
    };

    void init();
    void fillPolygon_translated(const QPointF *points, int pointCount, GCMode gcMode,
                                QPaintEngine::PolygonDrawMode mode);
    void fillPolygon_dev(const QPointF *points, int pointCount, GCMode gcMode,
                         QPaintEngine::PolygonDrawMode mode);
    void fillPath(const QPainterPath &path, GCMode gcmode, bool transform);
    void strokePolygon_dev(const QPointF *points, int pointCount, bool close);
    void strokePolygon_translated(const QPointF *points, int pointCount, bool close);
    void setupAdaptedOrigin(const QPoint &p);
    void resetAdaptedOrigin();
    void decidePathFallback() {
        use_path_fallback = has_alpha_brush
                            || has_alpha_pen
                            || has_custom_pen
                            || has_complex_xform
                            || (cpen.widthF() > 0 && has_complex_xform)
                            || (render_hints & QPainter::Antialiasing);
    }
    void clipPolygon_dev(const QPolygonF &poly, QPolygonF *clipped_poly);

    Display *dpy;
    int scrn;
    int pdev_depth;
    Qt::HANDLE hd;
    QPixmap brush_pm;
#if !defined (QT_NO_XRENDER)
    Qt::HANDLE picture;
    Qt::HANDLE current_brush;
    QPixmap bitmap_texture;
    int composition_mode;
#else
    Qt::HANDLE picture;
#endif
    GC gc;
    GC gc_brush;

    QPen cpen;
    QBrush cbrush;
    QRegion crgn;
    QTransform matrix;
    qreal opacity;

    uint has_complex_xform : 1;
    uint has_custom_pen : 1;
    uint use_path_fallback : 1;
    uint has_clipping : 1;
    uint adapted_brush_origin : 1;
    uint adapted_pen_origin : 1;
    uint has_pen : 1;
    uint has_brush : 1;
    uint has_texture : 1;
    uint has_pattern : 1;
    uint has_alpha_pen : 1;
    uint has_alpha_brush : 1;
    uint render_hints;

    const QX11Info *xinfo;
    QPointF bg_origin;
    QTransform::TransformationType txop;
    QPolygonClipper<qt_float_point, qt_float_point, float> polygonClipper;

    int xlibMaxLinePoints;
#ifndef QT_NO_XRENDER
    QXRenderTessellator *tessellator;
#endif
};

#endif // QPAINTENGINE_X11_P_H
