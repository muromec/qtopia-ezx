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

#include "qrasterizer_p.h"

#include <QDebug>
#include <QLine>
#include <QPoint>
#include <QRect>
#include <QTransform>

#include <private/qmath_p.h>
#include <private/qdrawhelper_p.h>
#include <private/qpaintengine_raster_p.h>

typedef int Q16Dot16;
#define Q16Dot16ToFloat(i) ((i)/65536.)
#define FloatToQ16Dot16(i) (int)((i) * 65536.)
#define IntToQ16Dot16(i) ((i) << 16)
#define Q16Dot16ToInt(i) ((i) >> 16)
#define Q16Dot16Factor 65536

#define Q16Dot16Multiply(x, y) (int)((qlonglong(x) * qlonglong(y)) >> 16)
#define Q16Dot16FastMultiply(x, y) (((x) * (y)) >> 16)

#define SPAN_BUFFER_SIZE 256

class QSpanBuffer {
public:
    QSpanBuffer(QSpanData *data, QRasterBuffer *rb, const QRect &deviceRect)
        : m_spanCount(0)
        , m_data(data)
        , m_rb(rb)
        , m_blend(data->blend)
        , m_deviceRect(deviceRect)
    {
    }

    ~QSpanBuffer()
    {
        flushSpans();
    }

    void addSpan(int x, unsigned int len, int y, unsigned char coverage)
    {
        if (!coverage || !len)
            return;

        Q_ASSERT(y >= m_deviceRect.top());
        Q_ASSERT(y <= m_deviceRect.bottom());
        Q_ASSERT(x >= m_deviceRect.left());
        Q_ASSERT(x + int(len) - 1 <= m_deviceRect.right());

        m_spans[m_spanCount].x = x;
        m_spans[m_spanCount].len = len;
        m_spans[m_spanCount].y = y;
        m_spans[m_spanCount].coverage = coverage;

        if (++m_spanCount == SPAN_BUFFER_SIZE)
            flushSpans();
    }

    void setBoundingRect(int left, int top, int right, int bottom)
    {
        m_blend = isUnclipped(QRect(left, top, right - left + 1, bottom - top + 1))
                    ? m_data->unclipped_blend
                    : m_data->blend;
    }

private:
    bool isUnclipped(const QRect &r) {
        if (m_rb->clip)
            return false;

        if (m_rb->clipRect == m_deviceRect)
            return true;

        if (!m_rb->clipRect.isEmpty()) {
            const QRect &r1 = m_rb->clipRect;
            return (r.left() >= r1.left() && r.right() <= r1.right()
                    && r.top() >= r1.top() && r.bottom() <= r1.bottom());
        } else {
            return qt_region_strictContains(m_rb->clipRegion, r);
        }
    }

    void flushSpans()
    {
        m_blend(m_spanCount, m_spans, m_data);
        m_spanCount = 0;
    }

    QT_FT_Span m_spans[SPAN_BUFFER_SIZE];
    int m_spanCount;

    QSpanData *m_data;
    QRasterBuffer *m_rb;
    QRasterPaintEnginePrivate *m_pe;
    ProcessSpans m_blend;

    QRect m_deviceRect;
};

class QRasterizerPrivate
{
public:
    bool antialiased;
    QSpanData *spanData;
    QRasterBuffer *rasterBuffer;
    QRect deviceRect;
};

QRasterizer::QRasterizer()
    : d(new QRasterizerPrivate)
{
}

QRasterizer::~QRasterizer()
{
    delete d;
}

void QRasterizer::initialize(bool antialiased, QRasterBuffer *rb)
{
    d->antialiased = antialiased;
    d->rasterBuffer = rb;
}

void QRasterizer::setSpanData(QSpanData *data)
{
    d->spanData = data;
}

void QRasterizer::setDeviceRect(const QRect &deviceRect)
{
    d->deviceRect = deviceRect;
}

static Q16Dot16 intersectPixelFP(int x, Q16Dot16 top, Q16Dot16 bottom, Q16Dot16 leftIntersectX, Q16Dot16 rightIntersectX, Q16Dot16 slope, Q16Dot16 invSlope)
{
    Q16Dot16 leftX = IntToQ16Dot16(x);
    Q16Dot16 rightX = IntToQ16Dot16(x) + Q16Dot16Factor;

    Q16Dot16 leftIntersectY, rightIntersectY;
    if (slope > 0) {
        leftIntersectY = top + Q16Dot16Multiply(leftX - leftIntersectX, invSlope);
        rightIntersectY = leftIntersectY + invSlope;
    } else {
        leftIntersectY = top + Q16Dot16Multiply(leftX - rightIntersectX, invSlope);
        rightIntersectY = leftIntersectY + invSlope;
    }

    if (leftIntersectX >= leftX && rightIntersectX <= rightX) {
        return Q16Dot16Multiply(bottom - top, leftIntersectX - leftX + ((rightIntersectX - leftIntersectX) >> 1));
    } else if (leftIntersectX >= rightX) {
        return bottom - top;
    } else if (leftIntersectX >= leftX) {
        if (slope > 0) {
            return (bottom - top) - Q16Dot16FastMultiply((rightX - leftIntersectX) >> 1, rightIntersectY - top);
        } else {
            return (bottom - top) - Q16Dot16FastMultiply((rightX - leftIntersectX) >> 1, bottom - rightIntersectY);
        }
    } else if (rightIntersectX <= leftX) {
        return 0;
    } else if (rightIntersectX <= rightX) {
        if (slope > 0) {
            return Q16Dot16FastMultiply((rightIntersectX - leftX) >> 1, bottom - leftIntersectY);
        } else {
            return Q16Dot16FastMultiply((rightIntersectX - leftX) >> 1, leftIntersectY - top);
        }
    } else {
        if (slope > 0) {
            return (bottom - rightIntersectY) + ((rightIntersectY - leftIntersectY) >> 1);
        } else {
            return (rightIntersectY - top) + ((leftIntersectY - rightIntersectY) >> 1);
        }
    }
}

static inline bool q16Dot16Compare(qreal p1, qreal p2)
{
    return FloatToQ16Dot16(p2 - p1) == 0;
}

void QRasterizer::rasterizeLine(const QPointF &a, const QPointF &b, qreal width, bool squareCap)
{
    QPointF pa = a;
    QPointF pb = b;

    {
        const qreal gridResolution = 64;
        const qreal reciprocal = 1 / gridResolution;

        // snap to grid to prevent large slopes
        pa.rx() = int(pa.rx() * gridResolution + 0.5) * reciprocal;
        pa.ry() = int(pa.ry() * gridResolution + 0.5) * reciprocal;
        pb.rx() = int(pb.rx() * gridResolution + 0.5) * reciprocal;
        pb.ry() = int(pb.ry() * gridResolution + 0.5) * reciprocal;
    }

    QSpanBuffer buffer(d->spanData, d->rasterBuffer, d->deviceRect);

    if (q16Dot16Compare(pa.y(), pb.y())) {
        const qreal x = (a.x() + b.x()) * 0.5f;
        const qreal dx = qAbs(b.x() - a.x()) * 0.5f;

        const qreal dy = width * dx;

        pa = QPointF(x, a.y() - dy);
        pb = QPointF(x, a.y() + dy);

        if (squareCap)
            width = 1 / width + 1.0f;
        else
            width = 1 / width;

        squareCap = false;
    }

    if (q16Dot16Compare(pa.x(), pb.x())) {
        if (pa.y() > pb.y())
            qSwap(pa, pb);

        const qreal dy = pb.y() - pa.y();
        const qreal halfWidth = 0.5f * width * dy;

        if (squareCap) {
            pa.ry() -= halfWidth;
            pb.ry() += halfWidth;
        }

        qreal left = pa.x() - halfWidth;
        qreal right = pa.x() + halfWidth;

        left = qBound(qreal(d->deviceRect.left()), left, qreal(d->deviceRect.right() + 1));
        right = qBound(qreal(d->deviceRect.left()), right, qreal(d->deviceRect.right() + 1));

        pa.ry() = qBound(qreal(d->deviceRect.top()), pa.y(), qreal(d->deviceRect.bottom() + 1));
        pb.ry() = qBound(qreal(d->deviceRect.top()), pb.y(), qreal(d->deviceRect.bottom() + 1));

        if (q16Dot16Compare(left, right) || q16Dot16Compare(pa.y(), pb.y()))
            return;

        if (d->antialiased) {
            int iTop = int(pa.y()) ;
            int iBottom = int(pb.y());
            int iLeft = int(left);
            int iRight = int(right);
            buffer.setBoundingRect(iLeft, iTop, iRight, iBottom);

            Q16Dot16 leftWidth = FloatToQ16Dot16(iLeft + 1.0f - left);
            Q16Dot16 rightWidth = FloatToQ16Dot16(right - iRight);

            Q16Dot16 coverage[3];
            int x[3];
            int len[3];

            int n = 1;
            if (iLeft == iRight) {
                coverage[0] = leftWidth + rightWidth;
                x[0] = iLeft;
                len[0] = 1;
            } else {
                coverage[0] = leftWidth;
                x[0] = iLeft;
                len[0] = 1;
                if (leftWidth == Q16Dot16Factor) {
                    len[0] = iRight - iLeft;
                } else if (iRight - iLeft > 1) {
                    coverage[1] = Q16Dot16Factor;
                    x[1] = iLeft + 1;
                    len[1] = iRight - iLeft - 1;
                    ++n;
                }
                if (rightWidth) {
                    coverage[n] = rightWidth;
                    x[n] = iRight;
                    len[n] = 1;
                    ++n;
                }
            }

            for (int y = iTop; y <= iBottom; ++y) {
                Q16Dot16 rowHeight = FloatToQ16Dot16(qMin(qreal(y + 1), pb.y()) - qMax(qreal(y), pa.y()));
                for (int i = 0; i < n; ++i)
                    buffer.addSpan(x[i], len[i], y, Q16Dot16ToInt(255 * Q16Dot16Multiply(rowHeight, coverage[i])));
            }
        } else { // aliased
            int iTop = int(pa.y() + 0.5f);
            int iBottom = int(pb.y() - 0.5f);
            int iLeft = int(left + 0.5f);
            int iRight = int(right - 0.5f);
            buffer.setBoundingRect(iLeft, iTop, iRight, iBottom);

            int iWidth = iRight - iLeft + 1;
            for (int y = iTop; y <= iBottom; ++y)
                buffer.addSpan(iLeft, iWidth, y, 255);
        }
    } else {
        if (pa.y() > pb.y())
            qSwap(pa, pb);

        QPointF delta = pb - pa;
        delta *= 0.5f * width;
        QPointF perp(delta.y(), -delta.x());

        if (squareCap) {
            pa -= delta;
            pb += delta;
        }

        QPointF top;
        QPointF left;
        QPointF right;
        QPointF bottom;

        if (pa.x() < pb.x()) {
            top = pa + perp;
            left = pa - perp;
            right = pb + perp;
            bottom = pb - perp;
        } else {
            top = pa - perp;
            left = pb - perp;
            right = pa + perp;
            bottom = pb + perp;
        }

        qreal topBound = qBound(qreal(d->deviceRect.top()), top.y(), qreal(d->deviceRect.bottom()));
        qreal bottomBound = qBound(qreal(d->deviceRect.top()), bottom.y(), qreal(d->deviceRect.bottom()));

        if (q16Dot16Compare(topBound, bottomBound))
            return;

        qreal leftSlope = (left.x() - top.x()) / (left.y() - top.y());
        qreal rightSlope = -1.0f / leftSlope;

        Q16Dot16 leftSlopeFP = FloatToQ16Dot16(leftSlope);
        Q16Dot16 rightSlopeFP = FloatToQ16Dot16(rightSlope);

        if (d->antialiased) {
            int iTop = int(topBound);
            int iLeft = int(left.y());
            int iRight = int(right.y());
            int iBottom = int(bottomBound);
            buffer.setBoundingRect(int(left.x()), iTop, int(right.x()), iBottom);

            Q16Dot16 leftIntersectAf = FloatToQ16Dot16(top.x() + (iTop - top.y()) * leftSlope);
            Q16Dot16 rightIntersectAf = FloatToQ16Dot16(top.x() + (iTop - top.y()) * rightSlope);
            Q16Dot16 leftIntersectBf = 0;
            Q16Dot16 rightIntersectBf = 0;

            if (iLeft < iTop)
                leftIntersectBf = FloatToQ16Dot16(left.x() + (iTop - left.y()) * rightSlope);

            if (iRight < iTop)
                rightIntersectBf = FloatToQ16Dot16(right.x() + (iTop - right.y()) * leftSlope);

            Q16Dot16 rowTop, rowBottomLeft, rowBottomRight, rowTopLeft, rowTopRight, rowBottom;
            Q16Dot16 topLeftIntersectAf, topLeftIntersectBf, topRightIntersectAf, topRightIntersectBf;
            Q16Dot16 bottomLeftIntersectAf, bottomLeftIntersectBf, bottomRightIntersectAf, bottomRightIntersectBf;

            int leftMin, leftMax, rightMin, rightMax;

            for (int y = iTop; y <= iBottom; ++y) {
                rowTop = FloatToQ16Dot16(qMax(qreal(y), top.y()));
                rowBottomLeft = FloatToQ16Dot16(qMin(qreal(y + 1), left.y()));
                rowBottomRight = FloatToQ16Dot16(qMin(qreal(y + 1), right.y()));
                rowTopLeft = FloatToQ16Dot16(qMax(qreal(y), left.y()));
                rowTopRight = FloatToQ16Dot16(qMax(qreal(y), right.y()));
                rowBottom = FloatToQ16Dot16(qMin(qreal(y + 1), bottom.y()));

                Q16Dot16 yFP = IntToQ16Dot16(y);

                if (y == iTop) {
                    topLeftIntersectAf = leftIntersectAf + Q16Dot16Multiply(leftSlopeFP, rowTop - yFP);
                    topRightIntersectAf = rightIntersectAf + Q16Dot16Multiply(rightSlopeFP, rowTop - yFP);
                } else {
                    topLeftIntersectAf = leftIntersectAf;
                    topRightIntersectAf = rightIntersectAf;
                }

                if (y == iLeft) {
                    leftIntersectBf = FloatToQ16Dot16(left.x() + (y - left.y()) * rightSlope);
                    topLeftIntersectBf = leftIntersectBf + Q16Dot16Multiply(rightSlopeFP, rowTopLeft - yFP);
                    bottomLeftIntersectAf = leftIntersectAf + Q16Dot16Multiply(leftSlopeFP, rowBottomLeft - yFP);
                } else {
                    topLeftIntersectBf = leftIntersectBf;
                    bottomLeftIntersectAf = leftIntersectAf + leftSlopeFP;
                }

                if (y == iRight) {
                    rightIntersectBf = FloatToQ16Dot16(right.x() + (y - right.y()) * leftSlope);
                    topRightIntersectBf = rightIntersectBf + Q16Dot16Multiply(leftSlopeFP, rowTopRight - yFP);
                    bottomRightIntersectAf = rightIntersectAf + Q16Dot16Multiply(rightSlopeFP, rowBottomRight - yFP);
                } else {
                    topRightIntersectBf = rightIntersectBf;
                    bottomRightIntersectAf = rightIntersectAf + rightSlopeFP;
                }

                if (y == iBottom) {
                    bottomLeftIntersectBf = leftIntersectBf + Q16Dot16Multiply(rightSlopeFP, rowBottom - yFP);
                    bottomRightIntersectBf = rightIntersectBf + Q16Dot16Multiply(leftSlopeFP, rowBottom - yFP);
                } else {
                    bottomLeftIntersectBf = leftIntersectBf + rightSlopeFP;
                    bottomRightIntersectBf = rightIntersectBf + leftSlopeFP;
                }

                if (y < iLeft) {
                    leftMin = Q16Dot16ToInt(bottomLeftIntersectAf);
                    leftMax = Q16Dot16ToInt(topLeftIntersectAf);
                } else if (y == iLeft) {
                    leftMin = Q16Dot16ToInt(qMax(bottomLeftIntersectAf, topLeftIntersectBf));
                    leftMax = Q16Dot16ToInt(qMax(topLeftIntersectAf, bottomLeftIntersectBf));
                } else {
                    leftMin = Q16Dot16ToInt(topLeftIntersectBf);
                    leftMax = Q16Dot16ToInt(bottomLeftIntersectBf);
                }

                leftMin = qBound(d->deviceRect.left(), leftMin, d->deviceRect.right());
                leftMax = qBound(d->deviceRect.left(), leftMax, d->deviceRect.right());

                if (y < iRight) {
                    rightMin = Q16Dot16ToInt(topRightIntersectAf);
                    rightMax = Q16Dot16ToInt(bottomRightIntersectAf);
                } else if (y == iRight) {
                    rightMin = Q16Dot16ToInt(qMin(topRightIntersectAf, bottomRightIntersectBf));
                    rightMax = Q16Dot16ToInt(qMin(bottomRightIntersectAf, topRightIntersectBf));
                } else {
                    rightMin = Q16Dot16ToInt(bottomRightIntersectBf);
                    rightMax = Q16Dot16ToInt(topRightIntersectBf);
                }

                rightMin = qBound(d->deviceRect.left(), rightMin, d->deviceRect.right());
                rightMax = qBound(d->deviceRect.left(), rightMax, d->deviceRect.right());

                Q16Dot16 rowHeight = rowBottom - rowTop;

                int x = leftMin;
                while (x <= leftMax) {
                    Q16Dot16 excluded = 0;

                    if (y <= iLeft)
                        excluded += intersectPixelFP(x, rowTop, rowBottomLeft,
                                                     bottomLeftIntersectAf, topLeftIntersectAf,
                                                     leftSlopeFP, -rightSlopeFP);
                    if (y >= iLeft)
                        excluded += intersectPixelFP(x, rowTopLeft, rowBottom,
                                                     topLeftIntersectBf, bottomLeftIntersectBf,
                                                     rightSlopeFP, -leftSlopeFP);

                    if (x >= rightMin) {
                        if (y <= iRight)
                            excluded += (rowBottomRight - rowTop) - intersectPixelFP(x, rowTop, rowBottomRight,
                                                                                     topRightIntersectAf, bottomRightIntersectAf,
                                                                                     rightSlopeFP, -leftSlopeFP);
                        if (y >= iRight)
                            excluded += (rowBottom - rowTopRight) - intersectPixelFP(x, rowTopRight, rowBottom,
                                                                                     bottomRightIntersectBf, topRightIntersectBf,
                                                                                     leftSlopeFP, -rightSlopeFP);
                    }

                    Q16Dot16 coverage = rowHeight - excluded;
                    buffer.addSpan(x, 1, y, Q16Dot16ToInt(255 * coverage));
                    ++x;
                }
                if (x < rightMin) {
                    buffer.addSpan(x, rightMin - x, y, Q16Dot16ToInt(255 * rowHeight));
                    x = rightMin;
                }
                while (x <= rightMax) {
                    Q16Dot16 excluded = 0;
                    if (y <= iRight)
                        excluded += (rowBottomRight - rowTop) - intersectPixelFP(x, rowTop, rowBottomRight,
                                                                                 topRightIntersectAf, bottomRightIntersectAf,
                                                                                 rightSlopeFP, -leftSlopeFP);
                    if (y >= iRight)
                        excluded += (rowBottom - rowTopRight) - intersectPixelFP(x, rowTopRight, rowBottom,
                                                                                 bottomRightIntersectBf, topRightIntersectBf,
                                                                                 leftSlopeFP, -rightSlopeFP);

                    Q16Dot16 coverage = rowHeight - excluded;
                    buffer.addSpan(x, 1, y, Q16Dot16ToInt(255 * coverage));
                    ++x;
                }

                leftIntersectAf += leftSlopeFP;
                leftIntersectBf += rightSlopeFP;
                rightIntersectAf += rightSlopeFP;
                rightIntersectBf += leftSlopeFP;
            }
        } else { // aliased
            int iTop = int(top.y() + 0.5f);
            int iLeft = int(left.y() - 0.5f);
            int iRight = int(right.y() - 0.5f);
            int iBottom = int(bottom.y() - 0.5f);

            buffer.setBoundingRect((int)left.x(), iTop, (int)right.x(), iBottom);

            Q16Dot16 leftIntersectAf = FloatToQ16Dot16(top.x() + 0.5f + (iTop + 0.5f - top.y()) * leftSlope);
            Q16Dot16 leftIntersectBf = FloatToQ16Dot16(left.x() + 0.5f + (iLeft + 1.5f - left.y()) * rightSlope);
            Q16Dot16 rightIntersectAf = FloatToQ16Dot16(top.x() - 0.5f + (iTop + 0.5f - top.y()) * rightSlope);
            Q16Dot16 rightIntersectBf = FloatToQ16Dot16(right.x() - 0.5f + (iRight + 1.5f - right.y()) * leftSlope);

            Q16Dot16 iMiddle = qMin(iLeft, iRight);

            int y;
            for (y = iTop; y <= iMiddle; ++y) {
                if (y >= d->deviceRect.top() && y <= d->deviceRect.bottom()) {
                    int x1 = Q16Dot16ToInt(leftIntersectAf);
                    int x2 = Q16Dot16ToInt(rightIntersectAf);
                    if (x2 >= d->deviceRect.left() && x1 <= d->deviceRect.right()) {
                        x1 = qMax(x1, d->deviceRect.left());
                        x2 = qMin(x2, d->deviceRect.right());
                        buffer.addSpan(x1, x2 - x1 + 1, y, 255);
                    }
                }
                leftIntersectAf += leftSlopeFP;
                rightIntersectAf += rightSlopeFP;
            }
            for (; y <= iRight; ++y) {
                if (y >= d->deviceRect.top() && y <= d->deviceRect.bottom()) {
                    int x1 = Q16Dot16ToInt(leftIntersectBf);
                    int x2 = Q16Dot16ToInt(rightIntersectAf);
                    if (x2 >= d->deviceRect.left() && x1 <= d->deviceRect.right()) {
                        x1 = qMax(x1, d->deviceRect.left());
                        x2 = qMin(x2, d->deviceRect.right());
                        buffer.addSpan(x1, x2 - x1 + 1, y, 255);
                    }
                }
                leftIntersectBf += rightSlopeFP;
                rightIntersectAf += rightSlopeFP;
            }
            for (; y <= iLeft; ++y) {
                if (y >= d->deviceRect.top() && y <= d->deviceRect.bottom()) {
                    int x1 = Q16Dot16ToInt(leftIntersectAf);
                    int x2 = Q16Dot16ToInt(rightIntersectBf);
                    if (x2 >= d->deviceRect.left() && x1 <= d->deviceRect.right()) {
                        x1 = qMax(x1, d->deviceRect.left());
                        x2 = qMin(x2, d->deviceRect.right());
                        buffer.addSpan(x1, x2 - x1 + 1, y, 255);
                    }
                }
                leftIntersectAf += leftSlopeFP;
                rightIntersectBf += leftSlopeFP;
            }
            for (; y <= iBottom; ++y) {
                if (y >= d->deviceRect.top() && y <= d->deviceRect.bottom()) {
                    int x1 = Q16Dot16ToInt(leftIntersectBf);
                    int x2 = Q16Dot16ToInt(rightIntersectBf);
                    if (x2 >= d->deviceRect.left() && x1 <= d->deviceRect.right()) {
                        x1 = qMax(x1, d->deviceRect.left());
                        x2 = qMin(x2, d->deviceRect.right());
                        buffer.addSpan(x1, x2 - x1 + 1, y, 255);
                    }
                }
                leftIntersectBf += rightSlopeFP;
                rightIntersectBf += leftSlopeFP;
            }
        }
    }
}
