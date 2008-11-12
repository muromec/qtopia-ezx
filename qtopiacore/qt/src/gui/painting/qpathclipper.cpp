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

#include "qpathclipper_p.h"

#include <private/qbezier_p.h>
#include <private/qdatabuffer_p.h>
#include <private/qmath_p.h>

#include <math.h>

#include <QImage>
#include <QPainter>

/**
  The algorithm is as follows:

  1. Find all intersections between the two paths (including self-intersections),
     and build a winged edge structure of non-intersecting parts.
  2. While there are more unhandled edges:
    3. Pick a y-coordinate from an unhandled edge.
    4. Intersect the horizontal line at y-coordinate with all edges.
    5. Traverse intersections left to right deciding whether each subpath should be added or not.
    6. If the subpath should be added, traverse the winged-edge structure and add the edges to
       a separate winged edge structure.
    7. Mark all edges in subpaths crossing the horizontal line as handled.
 8. (Optional) Simplify the resulting winged edge structure by merging shared edges.
 9. Convert the resulting winged edge structure to a painter path.
 */

#include <qdebug.h>

//#define QDEBUG_CLIPPER
static qreal dist(qreal x1, qreal y1, qreal x2, qreal y2)
{
    return qSqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

static qreal dot(const QPointF &a, const QPointF &b)
{
    return a.x() * b.x() + a.y() * b.y();
}

static QPointF normalize(const QPointF &p)
{
    return p / sqrt(p.x() * p.x() + p.y() * p.y());
}

struct QIntersection
{
    qreal alpha;
    QPointF pos;

    bool operator<(const QIntersection &isect) const
    {
        return alpha < isect.alpha;
    }
};

class QIntersectionFinder
{
public:
    QIntersectionFinder(QDataBuffer<QIntersection> *intersections = 0);

    bool hasIntersections() const;
    void addIntersections(const QPathSegments *a, const QPathSegments *b, int i, bool ignoreAutoClosing = false);

private:
    void intersectBeziers(const QBezier &one, const QBezier &two, bool swap);
    void intersectLines(const QLineF &a, const QLineF &b, bool swap);

    QDataBuffer<QIntersection> *m_intersections;

    QVector<qreal> m_t0;
    QVector<qreal> m_t1;

    bool m_hasIntersections;
};

QIntersectionFinder::QIntersectionFinder(QDataBuffer<QIntersection> *intersections)
    : m_intersections(intersections)
    , m_hasIntersections(false)
{
}

inline bool QIntersectionFinder::hasIntersections() const
{
    return m_hasIntersections;
}

void QIntersectionFinder::intersectBeziers(const QBezier &one, const QBezier &two, bool swap)
{
    if (one.pt1() == two.pt1() && one.pt2() == two.pt2() && one.pt3() == two.pt3() && one.pt4() == two.pt4() ||
        one.pt1() == two.pt4() && one.pt2() == two.pt3() && one.pt3() == two.pt2() && one.pt4() == two.pt1()) {

        m_hasIntersections = true;
        return;
    }

    int a = 0;
    int b = 1;

    if (swap)
        qSwap(a, b);

    const QBezier *bez[2] = { &one, &two };

    m_t0.clear();
    m_t1.clear();

    if (!QBezier::findIntersections(*bez[a], *bez[b], m_t0, m_t1))
        return;

    m_hasIntersections = true;

    if (!m_intersections)
        return;

    const QVector<qreal> &alpha_ps = m_t0;
    const QVector<qreal> &alpha_qs = m_t1;

    int count = alpha_ps.size();

    for (int i = 0; i < count; ++i) {
        qreal alpha_p = alpha_ps[i];
        qreal alpha_q = alpha_qs[i];

        QPointF pt;
        if (alpha_p == 0) {
            pt = bez[a]->pt1();
        } else if (alpha_p == 1) {
            pt = bez[a]->pt4();
        } else if (alpha_q == 0) {
            pt = bez[b]->pt1();
        } else if (alpha_q == 1) {
            pt = bez[b]->pt4();
        } else {
            pt = bez[a]->pointAt(alpha_p);
        }

        QIntersection intersection;
        intersection.alpha = (a == 0 ? alpha_p : alpha_q);
        intersection.pos = pt;
        m_intersections->add(intersection);
    }
}

void QIntersectionFinder::intersectLines(const QLineF &a, const QLineF &b, bool swap)
{
    const QPointF p1 = a.p1();
    const QPointF p2 = a.p2();

    const QPointF q1 = b.p1();
    const QPointF q2 = b.p2();

    const QPointF pDelta = p2 - p1;
    const QPointF qDelta = q2 - q1;

    const qreal par = pDelta.x() * qDelta.y() - pDelta.y() * qDelta.x();

    if (qFuzzyCompare(par, qreal(0.0))) {
        const QPointF normal(-pDelta.y(), pDelta.x());

        // coinciding?
        if (qFuzzyCompare(dot(normal, q1 - p1), qreal(0.0))) {
            const qreal invDp = 1 / dot(pDelta, pDelta);

            const qreal tq1 = dot(pDelta, q1 - p1) * invDp;
            const qreal tq2 = dot(pDelta, q2 - p1) * invDp;

            if (tq1 > 0 && tq1 < 1) {
                m_hasIntersections = true;

                if (m_intersections) {
                    QIntersection intersection;
                    intersection.alpha = tq1;
                    intersection.pos = q1;
                    m_intersections->add(intersection);
                }
            }

            if (tq2 > 0 && tq2 < 1) {
                m_hasIntersections = true;

                if (m_intersections) {
                    QIntersection intersection;
                    intersection.alpha = tq2;
                    intersection.pos = q2;
                    m_intersections->add(intersection);
                }
            }

            if (!m_hasIntersections) {
                if (tq1 == 0 || tq1 == 1)
                    m_hasIntersections = true;
                else if (tq2 == 0 || tq2 == 1)
                    m_hasIntersections = true;
                else if (tq1 <= 0 && tq2 >= 1 || tq2 <= 0 && tq1 >= 1)
                    m_hasIntersections = true;
            }
        }

        return;
    }

    const qreal tp = (qDelta.y() * (q1.x() - p1.x()) -
                      qDelta.x() * (q1.y() - p1.y())) / par;
    const qreal tq = (pDelta.y() * (q1.x() - p1.x()) -
                      pDelta.x() * (q1.y() - p1.y())) / par;

    if (tp<0 || tp>1 || tq<0 || tq>1)
        return;

    const qreal x = p1.x() + tp*(p2.x() - p1.x());
    const qreal y = p1.y() + tp*(p2.y() - p1.y());

    const qreal nalpha_p = dist(p1.x(), p1.y(), x, y) /
                           dist(p1.x(), p1.y(), p2.x(), p2.y());
    const qreal nalpha_q = dist(q1.x(), q1.y(), x, y) /
                           dist(q1.x(), q1.y(), q2.x(), q2.y());

    QPointF pt;
    if (qFuzzyCompare(nalpha_p, qreal(0)) || qFuzzyCompare(nalpha_p, qreal(1)))
        return;

    m_hasIntersections = true;

    if (!m_intersections)
        return;

    if (qFuzzyCompare(nalpha_q, qreal(0))) {
        pt = q1;
    } else if (qFuzzyCompare(nalpha_q, qreal(1))) {
        pt = q2;
    } else if (swap) {
        pt = p1 + (p2 - p1) * nalpha_p;
    } else {
        pt = q1 + (q2 - q1) * nalpha_q;
    }

    QIntersection intersection;
    intersection.alpha = nalpha_p;
    intersection.pos = pt;
    m_intersections->add(intersection);
}

void QIntersectionFinder::addIntersections(const QPathSegments *a, const QPathSegments *b, int i, bool ignoreAutoClosing)
{
    const QBezier *bezierA = a->bezierAt(i);
    bool isBezierA = bezierA != 0;

    QBezier tempA;
    QBezier tempB;

    if (!isBezierA && ignoreAutoClosing && a->isAutoClosingLine(i))
        return;

    for (int j = 0; j < b->segments(); ++j) {
        if (a == b && i == j)
            continue;

        bool swap = a < b || (a == b) && (i < j);

        bool isBezierB = b->bezierAt(j) != 0;

        if (!isBezierB && ignoreAutoClosing && b->isAutoClosingLine(j))
            continue;

        if (isBezierA || isBezierB) {
            if (!isBezierA)
                swap = false;
            else if (!isBezierB)
                swap = true;

            const QBezier *bezierB;
            if (isBezierB) {
                bezierB = b->bezierAt(j);
            } else {
                const QLineF *line = b->lineAt(j);
                QPointF p1 = line->p1();
                QPointF delta = (line->p2() - p1) / 3;
                tempB = QBezier::fromPoints(p1, p1 + delta, p1 + 2 * delta, p1 + 3 * delta);
                bezierB = &tempB;
            }

            if (!bezierA) {
                const QLineF *line = a->lineAt(i);
                QPointF p1 = line->p1();
                QPointF delta = (line->p2() - p1) / 3;
                tempA = QBezier::fromPoints(p1, p1 + delta, p1 + 2 * delta, p1 + 3 * delta);
                bezierA = &tempA;
            }

            intersectBeziers(*bezierA, *bezierB, swap);
        } else {
            const QLineF &lineA = *a->lineAt(i);
            const QLineF &lineB = *b->lineAt(j);

            intersectLines(lineA, lineB, swap);
        }
    }
}

void QWingedEdge::intersectAndAdd(bool mightIntersect)
{
    const QPathSegments *segments[2] = { &m_subjectSegments, &m_clipSegments };

    QDataBuffer<QIntersection> intersections;
    QIntersectionFinder finder(&intersections);
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < segments[i]->segments(); ++j) {
            intersections.reset();

            finder.addIntersections(segments[i], segments[i], j);

            if (mightIntersect)
                finder.addIntersections(segments[i], segments[(i+1)%2], j);

            qSort(intersections.data(), intersections.data() + intersections.size());

            const QBezier *bezier = segments[i]->bezierAt(j);
            if (bezier) {
                QPointF first = bezier->pt1();
                QPointF second = bezier->pt4();

                qreal alpha = 0.0;
                QPointF last = first;
                for (int j = 0; j < intersections.size(); ++j) {
                    const QIntersection &isect = intersections.at(j);

                    addBezierEdge(bezier, last, isect.pos, alpha, isect.alpha, i);

                    alpha = isect.alpha;
                    last = isect.pos;
                }

                addBezierEdge(bezier, last, second, alpha, 1.0, i);
            } else {
                QPointF first = segments[i]->lineAt(j)->p1();
                QPointF second = segments[i]->lineAt(j)->p2();

                QPointF last = first;
                for (int k = 0; k < intersections.size(); ++k) {
                    const QIntersection &isect = intersections.at(k);

                    QPathEdge *ep = edge(addEdge(last, isect.pos));

                    if (ep) {
                        const int dir = last.y() < isect.pos.y() ? 1 : -1;
                        if (i == 0)
                            ep->windingA += dir;
                        else
                            ep->windingB += dir;
                    }

                    last = isect.pos;
                }

                QPathEdge *ep = edge(addEdge(last, second));

                if (ep) {
                    const int dir = last.y() < second.y() ? 1 : -1;
                    if (i == 0)
                        ep->windingA += dir;
                    else
                        ep->windingB += dir;
                }
            }
        }
    }
}

QWingedEdge::QWingedEdge()
{
}

QWingedEdge::QWingedEdge(const QPainterPath &subject, const QPainterPath &clip)
{
    m_subjectSegments.setPath(subject);
    m_clipSegments.setPath(clip);

    const QRectF r1 = subject.controlPointRect();
    const QRectF r2 = clip.controlPointRect();

    // same as QRectF::intersets() except using <= instead of <
    const bool intersect = qMax(r1.x(), r2.x()) <= qMin(r1.x() + r1.width(), r2.x() + r2.width())
                           && qMax(r1.y(), r2.y()) <= qMin(r1.y() + r1.height(), r2.y() + r2.height());

    intersectAndAdd(intersect);
}

QWingedEdge::TraversalStatus QWingedEdge::next(const QWingedEdge::TraversalStatus &status) const
{
    const QPathEdge *sp = edge(status.edge);
    Q_ASSERT(sp);

    TraversalStatus result;
    result.edge = sp->next(status.traversal, status.direction);
    result.traversal = status.traversal;
    result.direction = status.direction;

    const QPathEdge *rp = edge(result.edge);
    Q_ASSERT(rp);

    if (sp->vertex(status.direction) == rp->vertex(status.direction))
        result.flip();

    return result;
}

static bool isLine(const QBezier &bezier)
{
    const bool equal_1_2 = bezier.pt1() == bezier.pt2();
    const bool equal_2_3 = bezier.pt2() == bezier.pt3();
    const bool equal_3_4 = bezier.pt3() == bezier.pt4();

    // point?
    if (equal_1_2 && equal_2_3 && equal_3_4)
        return true;

    if (bezier.pt1() == bezier.pt4())
        return equal_1_2 || equal_3_4;

    return equal_1_2 && equal_3_4 || equal_1_2 && equal_2_3 || equal_2_3 && equal_3_4;
}

void QPathSegments::setPath(const QPainterPath &path)
{
    m_lines.reset();
    m_beziers.reset();

    bool hasMoveTo = false;
    QPointF lastMoveTo;
    QPointF last;
    for (int i = 0; i < path.elementCount(); ++i) {
        QPointF current = path.elementAt(i);
        switch (path.elementAt(i).type) {
        case QPainterPath::MoveToElement:
            if (hasMoveTo && last != lastMoveTo) {
                Line line(QLineF(last, lastMoveTo), true);
                m_lines << line;
            }
            hasMoveTo = true;
            last = lastMoveTo = current;
            break;
        case QPainterPath::LineToElement:
            {
                Line line(QLineF(last, current));
                m_lines << line;
            }
            last = current;
            break;
        case QPainterPath::CurveToElement:
            {
                QBezier bezier = QBezier::fromPoints(last, path.elementAt(i), path.elementAt(i+1), path.elementAt(i+2));
                if (isLine(bezier)) {
                    Line line(QLineF(last, path.elementAt(i+2)));
                    m_lines << line;
                } else {
                    m_beziers << bezier;
                }
            }
            last = path.elementAt(i + 2);
            i += 2;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
    }

    if (hasMoveTo && last != lastMoveTo) {
        Line line(QLineF(last, lastMoveTo), true);
        m_lines << line;
    }
}

qreal QWingedEdge::delta(int vertex, int a, int b) const
{
    const QPathEdge *ap = edge(a);
    const QPathEdge *bp = edge(b);

    qreal a_angle = ap->angle;
    qreal b_angle = bp->angle;

    if (vertex == ap->second)
        a_angle = ap->invAngle;

    if (vertex == bp->second)
        b_angle = bp->invAngle;

    qreal result = b_angle - a_angle;

    if (qFuzzyCompare(result, qreal(0)) || qFuzzyCompare(result, qreal(128)))
        return 0;

    if (result < 0)
        return result + 128.;
    else
        return result;
}

static inline QPointF tangentAt(const QWingedEdge &list, int vi, int ei)
{
    const QPathEdge *ep = list.edge(ei);
    Q_ASSERT(ep);

    qreal t;
    qreal sign;

    if (ep->first == vi) {
        t = ep->t0;
        sign = 1;
    } else {
        t = ep->t1;
        sign = -1;
    }

    QPointF normal;
    if (ep->bezier) {
        normal = ep->bezier->derivedAt(t);

        if (normal == QPointF())
            normal = ep->bezier->secondDerivedAt(t);
    } else {
        const QPointF a = *list.vertex(ep->first);
        const QPointF b = *list.vertex(ep->second);
        normal = b - a;
    }

    return normalize(sign * normal);
}

static inline QPointF midPoint(const QWingedEdge &list, int ei)
{
    const QPathEdge *ep = list.edge(ei);
    Q_ASSERT(ep);

    if (ep->bezier) {
        return ep->bezier->pointAt(0.5 * (ep->t0 + ep->t1));
    } else {
        const QPointF a = *list.vertex(ep->first);
        const QPointF b = *list.vertex(ep->second);
        return a + 0.5 * (b - a);
    }
}

static QBezier transform(const QBezier &bezier, const QPointF &xAxis, const QPointF &yAxis, const QPointF &origin)
{
    QPointF points[4] = {
        bezier.pt1(),
        bezier.pt2(),
        bezier.pt3(),
        bezier.pt4()
    };

    for (int i = 0; i < 4; ++i) {
        const QPointF p = points[i] - origin;

        points[i].rx() = dot(xAxis, p);
        points[i].ry() = dot(yAxis, p);
    }

    return QBezier::fromPoints(points[0], points[1], points[2], points[3]);
}

static bool isLeftOf(const QWingedEdge &list, int vi, int ai, int bi)
{
    const QPathEdge *ap = list.edge(ai);
    const QPathEdge *bp = list.edge(bi);

    Q_ASSERT(ap);
    Q_ASSERT(bp);

    // shouldn't have two line segments with same normal
    // (they should have been merged during winged edge construction)
    Q_ASSERT(ap->bezier || bp->bezier);

    const QPointF tangent = tangentAt(list, vi, ai);
    const QPointF normal(tangent.y(), -tangent.x());

    const QPointF origin = *list.vertex(vi);

    const QPointF dpA = midPoint(list, ai) - origin;
    const QPointF dpB = midPoint(list, bi) - origin;

    qreal xA = dot(normal, dpA);
    qreal xB = dot(normal, dpB);

    if (xA <= 0 && xB >= 0)
        return true;

    if (xA >= 0 && xB <= 0)
        return false;

    if (!ap->bezier)
        return xB > 0;

    if (!bp->bezier)
        return xA < 0;

    // both are beziers on the same side of the tangent

    // transform the beziers into the local coordinate system
    // such that positive y is along the tangent, and positive x is along the normal

    QBezier bezierA = transform(*ap->bezier, normal, tangent, origin);
    QBezier bezierB = transform(*bp->bezier, normal, tangent, origin);

    qreal y = qMin(bezierA.pointAt(0.5 * (ap->t0 + ap->t1)).y(),
                   bezierB.pointAt(0.5 * (bp->t0 + bp->t1)).y());

    xA = bezierA.pointAt(bezierA.tForY(ap->t0, ap->t1, y)).x();
    xB = bezierB.pointAt(bezierB.tForY(bp->t0, bp->t1, y)).x();

    return xA < xB;
}

QWingedEdge::TraversalStatus QWingedEdge::findInsertStatus(int vi, int ei) const
{
    const QPathVertex *vp = vertex(vi);

    Q_ASSERT(vp);
    Q_ASSERT(ei >= 0);
    Q_ASSERT(vp->edge >= 0);

    int position = vp->edge;
    qreal d = 128.;

    TraversalStatus status;
    status.direction = edge(vp->edge)->directionTo(vi);
    status.traversal = QPathEdge::RightTraversal;
    status.edge = vp->edge;

#ifdef QDEBUG_CLIPPER
    const QPathEdge *ep = edge(ei);
    qDebug() << "Finding insert status for edge" << ei << "at vertex" << QPointF(*vp) << ", angles: " << ep->angle << ep->invAngle;
#endif

    do {
        status = next(status);
        status.flip();

        Q_ASSERT(edge(status.edge)->vertex(status.direction) == vi);

        qreal d2 = delta(vi, ei, status.edge);

#ifdef QDEBUG_CLIPPER
        const QPathEdge *op = edge(status.edge);
        qDebug() << "Delta to edge" << status.edge << d2 << ", angles: " << op->angle << op->invAngle;
#endif

        if (!(qFuzzyCompare(d2, 0) && isLeftOf(*this, vi, status.edge, ei))
            && (d2 < d || qFuzzyCompare(d2, d) && isLeftOf(*this, vi, status.edge, position))) {
            position = status.edge;
            d = d2;
        }
    } while (status.edge != vp->edge);

    status.traversal = QPathEdge::LeftTraversal;
    status.direction = QPathEdge::Forward;
    status.edge = position;

    if (edge(status.edge)->vertex(status.direction) != vi)
        status.flip();

#ifdef QDEBUG_CLIPPER
    qDebug() << "Inserting edge" << ei << "to" << (status.traversal == QPathEdge::LeftTraversal ? "left" : "right") << "of edge" << status.edge;
#endif

    Q_ASSERT(edge(status.edge)->vertex(status.direction) == vi);

    return status;
}

void QWingedEdge::removeEdge(int ei)
{
    QPathEdge *ep = edge(ei);

    TraversalStatus status;
    status.direction = QPathEdge::Forward;
    status.traversal = QPathEdge::RightTraversal;
    status.edge = ei;

    TraversalStatus forwardRight = next(status);
    forwardRight.flipDirection();

    status.traversal = QPathEdge::LeftTraversal;
    TraversalStatus forwardLeft = next(status);
    forwardLeft.flipDirection();

    status.direction = QPathEdge::Backward;
    TraversalStatus backwardLeft = next(status);
    backwardLeft.flipDirection();

    status.traversal = QPathEdge::RightTraversal;
    TraversalStatus backwardRight = next(status);
    backwardRight.flipDirection();

    edge(forwardRight.edge)->setNext(forwardRight.traversal, forwardRight.direction, forwardLeft.edge);
    edge(forwardLeft.edge)->setNext(forwardLeft.traversal, forwardLeft.direction, forwardRight.edge);

    edge(backwardRight.edge)->setNext(backwardRight.traversal, backwardRight.direction, backwardLeft.edge);
    edge(backwardLeft.edge)->setNext(backwardLeft.traversal, backwardLeft.direction, backwardRight.edge);

    ep->setNext(QPathEdge::Forward, ei);
    ep->setNext(QPathEdge::Backward, ei);

    QPathVertex *a = vertex(ep->first);
    QPathVertex *b = vertex(ep->second);

    a->edge = backwardRight.edge;
    b->edge = forwardRight.edge;
}

static int commonEdge(const QWingedEdge &list, int a, int b)
{
    const QPathVertex *ap = list.vertex(a);
    Q_ASSERT(ap);

    const QPathVertex *bp = list.vertex(b);
    Q_ASSERT(bp);

    if (ap->edge < 0 || bp->edge < 0)
        return -1;

    QWingedEdge::TraversalStatus status;
    status.edge = ap->edge;
    status.direction = list.edge(status.edge)->directionTo(a);
    status.traversal = QPathEdge::RightTraversal;

    do {
        const QPathEdge *ep = list.edge(status.edge);

        if (ep->first == a && ep->second == b ||
            ep->first == b && ep->second == a)
            return status.edge;

        status = list.next(status);
        status.flip();
    } while (status.edge != ap->edge);

    return -1;
}

static qreal computeAngle(const QPointF &v)
{
#if 1
    if (v.x() == 0) {
        return v.y() <= 0 ? 0 : 64.;
    } else if (v.y() == 0) {
        return v.x() <= 0 ? 32. : 96.;
    }

    QPointF nv = normalize(v);
    if (nv.y() < 0) {
        if (nv.x() < 0) { // 0 - 32
            return -32. * nv.x();
        } else { // 96 - 128
            return 128. - 32. * nv.x();
        }
    } else { // 32 - 96
        return 64. + 32 * nv.x();
    }
#else
    // doesn't seem to be robust enough
    return atan2(v.x(), v.y()) + Q_PI;
#endif
}

int QWingedEdge::addEdge(const QPointF &a, const QPointF &b, const QBezier *bezier, qreal t0, qreal t1)
{
    int fi = insert(a);
    int si = insert(b);

    if (fi == si)
        return -1;

    int common = commonEdge(*this, fi, si);
    if (common >= 0)
        return common;

    m_edges << QPathEdge(fi, si);

    int ei = m_edges.size() - 1;

    QPathVertex *fp = vertex(fi);
    QPathVertex *sp = vertex(si);

    QPathEdge *ep = edge(ei);

    ep->bezier = bezier;
    ep->t0 = t0;
    ep->t1 = t1;

    if (bezier) {
        QPointF aTangent = bezier->derivedAt(t0);
        QPointF bTangent = bezier->derivedAt(t1);

        if (aTangent == QPointF())
            aTangent = bezier->secondDerivedAt(t0);

        if (bTangent == QPointF())
            bTangent = bezier->secondDerivedAt(t1);

        ep->angle = computeAngle(aTangent);
        ep->invAngle = computeAngle(-bTangent);
    } else {
        const QPointF tangent = b - a;
        ep->angle = computeAngle(tangent);
        ep->invAngle = computeAngle(-tangent);
    }

    QPathVertex *vertices[2] = { fp, sp };
    QPathEdge::Direction dirs[2] = { QPathEdge::Backward, QPathEdge::Forward };

#ifdef QDEBUG_CLIPPER
    qDebug() << "** Adding edge" << ei << "/ vertices:" << a << b;
#endif

    for (int i = 0; i < 2; ++i) {
        QPathVertex *vp = vertices[i];
        if (vp->edge < 0) {
            vp->edge = ei;
            ep->setNext(dirs[i], ei);
        } else {
            int vi = ep->vertex(dirs[i]);
            Q_ASSERT(vertex(vi) == vertices[i]);

            TraversalStatus os = findInsertStatus(vi, ei);
            QPathEdge *op = edge(os.edge);

            Q_ASSERT(vertex(op->vertex(os.direction)) == vertices[i]);

            TraversalStatus ns = next(os);
            ns.flipDirection();
            QPathEdge *np = edge(ns.edge);

            op->setNext(os.traversal, os.direction, ei);
            np->setNext(ns.traversal, ns.direction, ei);

            int oe = os.edge;
            int ne = ns.edge;

            os = next(os);
            ns = next(ns);

            os.flipDirection();
            ns.flipDirection();

            Q_ASSERT(os.edge == ei);
            Q_ASSERT(ns.edge == ei);

            ep->setNext(os.traversal, os.direction, oe);
            ep->setNext(ns.traversal, ns.direction, ne);
        }
    }

    Q_ASSERT(ep->next(QPathEdge::RightTraversal, QPathEdge::Forward) >= 0);
    Q_ASSERT(ep->next(QPathEdge::RightTraversal, QPathEdge::Backward) >= 0);
    Q_ASSERT(ep->next(QPathEdge::LeftTraversal, QPathEdge::Forward) >= 0);
    Q_ASSERT(ep->next(QPathEdge::LeftTraversal, QPathEdge::Backward) >= 0);

    return ei;
}

void QWingedEdge::addBezierEdge(const QBezier *bezier, const QPointF &a, const QPointF &b, qreal alphaA, qreal alphaB, int path)
{
    if (qFuzzyCompare(alphaA, alphaB))
        return;

    qreal alphaMid = (alphaA + alphaB) * 0.5;

    qreal s0 = 0;
    qreal s1 = 1;
    int count = bezier->stationaryYPoints(s0, s1);

    m_splitPoints.clear();
    m_splitPoints << alphaA;
    m_splitPoints << alphaMid;
    m_splitPoints << alphaB;

    if (count > 0 && !qFuzzyCompare(s0, alphaA) && !qFuzzyCompare(s0, alphaMid) && !qFuzzyCompare(s0, alphaB) && s0 > alphaA && s0 < alphaB)
        m_splitPoints << s0;

    if (count > 1 && !qFuzzyCompare(s1, alphaA) && !qFuzzyCompare(s1, alphaMid) && !qFuzzyCompare(s1, alphaB) && s1 > alphaA && s1 < alphaB)
        m_splitPoints << s1;

    if (count > 0)
        qSort(m_splitPoints.begin(), m_splitPoints.end());

    QPointF pa = a;
    for (int i = 0; i < m_splitPoints.size() - 1; ++i) {
        const qreal t0 = m_splitPoints[i];
        const qreal t1 = m_splitPoints[i+1];

        const QPointF pb = (i + 1) == (m_splitPoints.size() - 1) ? b : bezier->pointAt(t1);

        QPathEdge *ep = edge(addEdge(pa, pb, bezier, t0, t1));

        if (ep) {
            const int dir = pa.y() < pb.y() ? 1 : -1;
            if (path == 0)
                ep->windingA += dir;
            else
                ep->windingB += dir;
        }

        pa = pb;
    }
}

int QWingedEdge::insert(const QPathVertex &vertex)
{
    if (!m_vertices.isEmpty()) {
        const QPathVertex &last = m_vertices.last();
        if (vertex.x == last.x && vertex.y == last.y)
            return m_vertices.size() - 1;

        for (int i = 0; i < m_vertices.size(); ++i) {
            const QPathVertex &v = m_vertices.at(i);
            if (qFuzzyCompare(v.x, vertex.x) && qFuzzyCompare(v.y, vertex.y)) {
                return i;
            }
        }
    }

    m_vertices << vertex;
    return m_vertices.size() - 1;
}

static void add(QPainterPath &path, const QWingedEdge &list, int edge, QPathEdge::Traversal traversal)
{
    QWingedEdge::TraversalStatus status;
    status.edge = edge;
    status.traversal = traversal;
    status.direction = QPathEdge::Forward;

    const QBezier *bezier = 0;
    qreal t0 = 1;
    qreal t1 = 0;
    bool forward = true;

    path.moveTo(*list.vertex(list.edge(edge)->first));

    do {
        const QPathEdge *ep = list.edge(status.edge);

        if (ep->bezier != bezier || bezier && t0 != ep->t1 && t1 != ep->t0) {
            if (bezier) {
                QBezier sub = bezier->bezierOnInterval(t0, t1);

                if (forward)
                    path.cubicTo(sub.pt2(), sub.pt3(), sub.pt4());
                else
                    path.cubicTo(sub.pt3(), sub.pt2(), sub.pt1());
            }

            bezier = ep->bezier;
            t0 = 1;
            t1 = 0;
            forward = status.direction == QPathEdge::Forward;
        }

        if (ep->bezier) {
            t0 = qMin(t0, ep->t0);
            t1 = qMax(t1, ep->t1);
        } else
            path.lineTo(*list.vertex(ep->vertex(status.direction)));

        if (status.traversal == QPathEdge::LeftTraversal)
            ep->flag &= ~16;
        else
            ep->flag &= ~32;

        status = list.next(status);
    } while (status.edge != edge);

    if (bezier) {
        QBezier sub = bezier->bezierOnInterval(t0, t1);
        if (forward)
            path.cubicTo(sub.pt2(), sub.pt3(), sub.pt4());
        else
            path.cubicTo(sub.pt3(), sub.pt2(), sub.pt1());
    }
}

void QWingedEdge::simplify()
{
    for (int i = 0; i < edgeCount(); ++i) {
        const QPathEdge *ep = edge(i);

        // if both sides are part of the inside then we can collapse the edge
        int flag = 0x3 << 4;
        if ((ep->flag & flag) == flag) {
            removeEdge(i);

            ep->flag &= ~flag;
        }
    }
}

QPainterPath QWingedEdge::toPath() const
{
    QPainterPath path;

    for (int i = 0; i < edgeCount(); ++i) {
        const QPathEdge *ep = edge(i);

        if (ep->flag & 16) {
            add(path, *this, i, QPathEdge::LeftTraversal);
        }

        if (ep->flag & 32)
            add(path, *this, i, QPathEdge::RightTraversal);
    }

    return path;
}

class QPathClipper::Private
{
public:
    Private()
    {
    }

    Private(const QPainterPath &s,
            const QPainterPath &c)
        : subjectPath(s)
        , clipPath(c)
    {
    }

    ~Private()
    {
    }

    void handleCrossingEdges(QWingedEdge &list, qreal y);

    bool areIntersecting()
    {
        QRectF subjControl = subjectPath.controlPointRect();
        QRectF clipControl = clipPath.controlPointRect();

        QRectF r1 = subjControl.normalized();
        QRectF r2 = clipControl.normalized();
        if (qMax(r1.x(), r2.x()) > qMin(r1.x() + r1.width(), r2.x() + r2.width()) ||
            qMax(r1.y(), r2.y()) > qMin(r1.y() + r1.height(), r2.y() + r2.height())) {
            // no way we could intersect
            return false;
        }

        QPathSegments subjectSegments;
        subjectSegments.setPath(subjectPath);
        QPathSegments clipSegments;
        clipSegments.setPath(clipPath);

        QDataBuffer<QIntersection> intersections;
        QIntersectionFinder finder(&intersections);

        for (int i = 0; i < subjectSegments.segments(); ++i) {
            finder.addIntersections(&subjectSegments, &clipSegments, i, true);

            if (finder.hasIntersections())
                return true;
        }

        return false;
    }

    QPainterPath subjectPath;
    QPainterPath clipPath;
    Operation    op;
};

QPathClipper::QPathClipper(const QPainterPath &subject,
                           const QPainterPath &clip)
    : d(new Private)
{
    d->subjectPath = subject;
    d->clipPath = clip;
}

QPathClipper::~QPathClipper()
{
    delete d;
}

template <typename Iterator, typename Equality>
Iterator qRemoveDuplicates(Iterator begin, Iterator end, Equality eq)
{
    if (begin == end)
        return end;

    Iterator last = begin;
    ++begin;
    Iterator insert = begin;
    for (Iterator it = begin; it != end; ++it) {
        if (!eq(*it, *last))
            *insert++ = *it;

        last = it;
    }

    return insert;
}

static void clear(QWingedEdge& list, int edge, QPathEdge::Traversal traversal)
{
    QWingedEdge::TraversalStatus status;
    status.edge = edge;
    status.traversal = traversal;
    status.direction = QPathEdge::Forward;

    do {
        if (status.traversal == QPathEdge::LeftTraversal)
            list.edge(status.edge)->flag |= 1;
        else
            list.edge(status.edge)->flag |= 2;

        status = list.next(status);
    } while (status.edge != edge);
}

template <typename InputIterator>
InputIterator qFuzzyFind(InputIterator first, InputIterator last, qreal val)
{
    while (first != last && !qFuzzyCompare(qreal(*first), qreal(val)))
        ++first;
    return first;
}

static bool fuzzyCompare(qreal a, qreal b)
{
    return qFuzzyCompare(a, b);
}

QPainterPath QPathClipper::clip(Operation op)
{
    d->op = op;

    if (d->subjectPath == d->clipPath)
        return op == BoolSub ? QPainterPath() : d->subjectPath;

    QWingedEdge list(d->subjectPath, d->clipPath);

    QVector<qreal> y_coords;
    y_coords.reserve(list.vertexCount());
    for (int i = 0; i < list.vertexCount(); ++i)
        y_coords << list.vertex(i)->y;

    qSort(y_coords.begin(), y_coords.end());
    y_coords.resize(qRemoveDuplicates(y_coords.begin(), y_coords.end(), fuzzyCompare) - y_coords.begin());

#ifdef QDEBUG_CLIPPER
    printf("sorted y coords:\n");
    for (int i = 0; i < y_coords.size(); ++i) {
        printf("%.9f\n", y_coords[i]);
    }
#endif

    bool found;
    do {
        found = false;
        int index = 0;
        qreal maxHeight = 0;
        for (int i = 0; i < list.edgeCount(); ++i) {
            QPathEdge *edge = list.edge(i);

            // have both sides of this edge already been handled?
            if ((edge->flag & 0x3) == 0x3)
                continue;

            QPathVertex *a = list.vertex(edge->first);
            QPathVertex *b = list.vertex(edge->second);

            if (qFuzzyCompare(a->y, b->y))
                continue;

            found = true;

            qreal height = qAbs(a->y - b->y);
            if (height > maxHeight) {
                index = i;
                maxHeight = height;
            }
        }

        if (found) {
            QPathEdge *edge = list.edge(index);

            QPathVertex *a = list.vertex(edge->first);
            QPathVertex *b = list.vertex(edge->second);

            // FIXME: this can be optimized by using binary search
            const int first = qFuzzyFind(y_coords.begin(), y_coords.end(), qMin(a->y, b->y)) - y_coords.begin();
            const int last = qFuzzyFind(y_coords.begin() + first, y_coords.end(), qMax(a->y, b->y)) - y_coords.begin();

            Q_ASSERT(first < y_coords.size() - 1);
            Q_ASSERT(last < y_coords.size());

            qreal bestY = 0.5 * (y_coords[first] + y_coords[first+1]);
            qreal biggestGap = y_coords[first+1] - y_coords[first];

            for (int i = first + 1; i < last - 1; ++i) {
                qreal gap = y_coords[i+1] - y_coords[i];

                if (gap > biggestGap) {
                    bestY = 0.5 * (y_coords[i] + y_coords[i+1]);
                    biggestGap = gap;
                }
            }

#ifdef QDEBUG_CLIPPER
            printf("y: %.9f, gap: %.9f\n", bestY, biggestGap);
#endif

            d->handleCrossingEdges(list, bestY);
            edge->flag |= 0x3;
        }
    } while (found);

    list.simplify();

    QPainterPath path = list.toPath();
    return path;
}

static void traverse(QWingedEdge &list, int edge, QPathEdge::Traversal traversal)
{
    QWingedEdge::TraversalStatus status;
    status.edge = edge;
    status.traversal = traversal;
    status.direction = QPathEdge::Forward;

    do {
        int flag = status.traversal == QPathEdge::LeftTraversal ? 1 : 2;

        QPathEdge *ep = list.edge(status.edge);

        ep->flag |= (flag | (flag << 4));

#ifdef QDEBUG_CLIPPER
        qDebug() << "traverse: adding edge " << status.edge << ", mask:" << (flag << 4) <<ep->flag;
#endif

        status = list.next(status);
    } while (status.edge != edge);
}

struct QCrossingEdge
{
    int edge;
    qreal x;

    bool operator<(const QCrossingEdge &edge) const
    {
        return x < edge.x;
    }
};

static bool bool_op(bool a, bool b, QPathClipper::Operation op)
{
    switch (op) {
    case QPathClipper::BoolAnd:
        return a && b;
    case QPathClipper::BoolOr:
        return a || b;
    case QPathClipper::BoolSub:
        return a && !b;
    default:
        Q_ASSERT(false);
        return false;
    }
}

bool QWingedEdge::isInside(qreal x, qreal y) const
{
    int winding = 0;
    for (int i = 0; i < edgeCount(); ++i) {
        const QPathEdge *ep = edge(i);

        // left xor right
        int w = ((ep->flag >> 4) ^ (ep->flag >> 5)) & 1;

        if (!w)
            continue;

        QPointF a = *vertex(ep->first);
        QPointF b = *vertex(ep->second);

        if (a.y() < y && b.y() > y || a.y() > y && b.y() < y) {
            if (ep->bezier) {
                qreal maxX = qMax(a.x(), qMax(b.x(), qMax(ep->bezier->x2, ep->bezier->x3)));
                qreal minX = qMin(a.x(), qMin(b.x(), qMin(ep->bezier->x2, ep->bezier->x3)));

                if (minX > x) {
                    winding += w;
                } else if (maxX > x) {
                    const qreal t = ep->bezier->tForY(ep->t0, ep->t1, y);
                    const qreal intersection = ep->bezier->pointAt(t).x();

                    if (intersection > x)
                        winding += w;
                }
            } else {
                qreal intersectionX = a.x() + (b.x() - a.x()) * (y - a.y()) / (b.y() - a.y());

                if (intersectionX > x)
                    winding += w;
            }
        }
    }

    return winding & 1;
}

static QVector<QCrossingEdge> findCrossings(const QWingedEdge &list, qreal y)
{
    QVector<QCrossingEdge> crossings;
    for (int i = 0; i < list.edgeCount(); ++i) {
        const QPathEdge *edge = list.edge(i);
        QPointF a = *list.vertex(edge->first);
        QPointF b = *list.vertex(edge->second);

        if (a.y() < y && b.y() > y || a.y() > y && b.y() < y) {
            if (edge->bezier) {
                const qreal t = edge->bezier->tForY(edge->t0, edge->t1, y);
                const qreal intersection = edge->bezier->pointAt(t).x();

                const QCrossingEdge edge = { i, intersection };
                crossings << edge;
            } else {
                const qreal intersection = a.x() + (b.x() - a.x()) * (y - a.y()) / (b.y() - a.y());
                const QCrossingEdge edge = { i, intersection };
                crossings << edge;
            }
        }
    }
    return crossings;
}

void QPathClipper::Private::handleCrossingEdges(QWingedEdge &list, qreal y)
{
    QVector<QCrossingEdge> crossings = findCrossings(list, y);

    Q_ASSERT(!crossings.isEmpty());
    qSort(crossings.begin(), crossings.end());

    int windingA = 0;
    int windingB = 0;

    int windingD = 0;

    const int aMask = subjectPath.fillRule() == Qt::WindingFill ? ~0x0 : 0x1;
    const int bMask = clipPath.fillRule() == Qt::WindingFill ? ~0x0 : 0x1;

#ifdef QDEBUG_CLIPPER
    qDebug() << "crossings:" << crossings.size();
#endif
    for (int i = 0; i < crossings.size() - 1; ++i) {
        int ei = crossings.at(i).edge;
        const QPathEdge *edge = list.edge(ei);

        windingA += edge->windingA;
        windingB += edge->windingB;

        const bool hasLeft = (edge->flag >> 4) & 1;
        const bool hasRight = (edge->flag >> 4) & 2;

        windingD += hasLeft ^ hasRight;

        const bool inA = (windingA & aMask) != 0;
        const bool inB = (windingB & bMask) != 0;
        const bool inD = (windingD & 0x1) != 0;

        const bool inside = bool_op(inA, inB, op);
        const bool add = inD ^ inside;

#ifdef QDEBUG_CLIPPER
        printf("y %f, x %f, inA: %d, inB: %d, inD: %d, inside: %d, flag: %x, bezier: %p, edge: %d\n", y, crossings.at(i).x, inA, inB, inD, inside, edge->flag, edge->bezier, ei);
#endif

        if (add) {
            qreal y0 = list.vertex(edge->first)->y;
            qreal y1 = list.vertex(edge->second)->y;

            if (y0 < y1) {
                if (!(edge->flag & 1))
                    traverse(list, ei, QPathEdge::LeftTraversal);

                if (!(edge->flag & 2))
                    clear(list, ei, QPathEdge::RightTraversal);
            } else {
                if (!(edge->flag & 1))
                    clear(list, ei, QPathEdge::LeftTraversal);

                if (!(edge->flag & 2))
                    traverse(list, ei, QPathEdge::RightTraversal);
            }

            ++windingD;
        } else {
            if (!(edge->flag & 1))
                clear(list, ei, QPathEdge::LeftTraversal);

            if (!(edge->flag & 2))
                clear(list, ei, QPathEdge::RightTraversal);
        }
    }
}

bool QPathClipper::intersect()
{
    return d->areIntersecting();
}

bool QPathClipper::contains()
{
    bool intersect = d->areIntersecting();

    //we have an intersection clearly we can't be fully contained
    if (intersect)
        return false;

    //if there's no intersections the path is already completely outside
    //or fully inside. if the first element of the clip is inside then
    //due to no intersections, the rest will be inside as well...
    return d->subjectPath.contains(d->clipPath.elementAt(0));
}
