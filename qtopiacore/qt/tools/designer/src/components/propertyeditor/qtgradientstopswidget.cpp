/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Designer of the Qt Toolkit.
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

/*
TRANSLATOR qdesigner_internal::QtGradientStopsWidget
*/

#include "qtgradientstopswidget.h"
#include "qtgradientstopsmodel.h"
#include <QMap>
#include <QImage>
#include <QPainter>
#include <QScrollBar>
#include <QMouseEvent>
#include <QRubberBand>
#include <QMenu>

#include "qdebug.h"

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtGradientStopsWidgetPrivate
{
    QtGradientStopsWidget *q_ptr;
    Q_DECLARE_PUBLIC(QtGradientStopsWidget)
public:
    void slotStopAdded(QtGradientStop *stop);
    void slotStopRemoved(QtGradientStop *stop);
    void slotStopMoved(QtGradientStop *stop, qreal newPos);
    void slotStopChanged(QtGradientStop *stop, const QColor &newColor);
    void slotStopSelected(QtGradientStop *stop, bool selected);
    void slotCurrentStopChanged(QtGradientStop *stop);
    void slotNewStop();
    void slotDelete();
    void slotSelectAll();

    double fromViewport(int x) const;
    double toViewport(double x) const;
    QtGradientStop *stopAt(const QPoint &viewportPos) const;
    QList<QtGradientStop *> stopsAt(const QPoint &viewportPos) const;
    void setupDrag(QtGradientStop *stop, int x);
    void ensureVisible(double x); // x = stop position
    void ensureVisible(QtGradientStop *stop);
    QtGradientStop *newStop(const QPoint &viewportPos);

    bool m_backgroundTransparent;
    QtGradientStopsModel *m_model;
    double m_handleSize;
    int m_scaleFactor;
    double m_zoom;

    QRubberBand *m_rubber;
    QPoint m_clickPos;

    QList<QtGradientStop *> m_stops;

    bool m_dragging;
    int m_dragOffset;
    QMap<QtGradientStop *, qreal> m_dragStops;
    QMap<qreal, QColor> m_dragOriginal;
};

}

double QtGradientStopsWidgetPrivate::fromViewport(int x) const
{
    QSize size = q_ptr->viewport()->size();
    int w = size.width();
    int max = q_ptr->horizontalScrollBar()->maximum();
    int val = q_ptr->horizontalScrollBar()->value();
    return ((double)x * m_scaleFactor + w * val) / (w * (m_scaleFactor + max));
}

double QtGradientStopsWidgetPrivate::toViewport(double x) const
{
    QSize size = q_ptr->viewport()->size();
    int w = size.width();
    int max = q_ptr->horizontalScrollBar()->maximum();
    int val = q_ptr->horizontalScrollBar()->value();
    return w * (x * (m_scaleFactor + max) - val) / m_scaleFactor;
}

QtGradientStop *QtGradientStopsWidgetPrivate::stopAt(const QPoint &viewportPos) const
{
    double posY = m_handleSize / 2;
    QListIterator<QtGradientStop *> itStop(m_stops);
    while (itStop.hasNext()) {
        QtGradientStop *stop = itStop.next();

        double posX = toViewport(stop->position());

        double x = viewportPos.x() - posX;
        double y = viewportPos.y() - posY;

        if ((m_handleSize * m_handleSize / 4) > (x * x + y * y))
            return stop;
    }
    return 0;
}

QList<QtGradientStop *> QtGradientStopsWidgetPrivate::stopsAt(const QPoint &viewportPos) const
{
    QList<QtGradientStop *> stops;
    double posY = m_handleSize / 2;
    QListIterator<QtGradientStop *> itStop(m_stops);
    while (itStop.hasNext()) {
        QtGradientStop *stop = itStop.next();

        double posX = toViewport(stop->position());

        double x = viewportPos.x() - posX;
        double y = viewportPos.y() - posY;

        if ((m_handleSize * m_handleSize / 4) > (x * x + y * y))
            stops.append(stop);
    }
    return stops;
}

void QtGradientStopsWidgetPrivate::setupDrag(QtGradientStop *stop, int x)
{
    m_model->setCurrentStop(stop);

    int viewportX = qRound(toViewport(stop->position()));
    m_dragOffset = x - viewportX;

    QList<QtGradientStop *> stops = m_stops;
    m_stops.clear();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (m_model->isSelected(s) || s == stop) {
            m_dragStops[s] = s->position() - stop->position();
            m_stops.append(s);
        } else {
            m_dragOriginal[s->position()] = s->color();
        }
    }
    itStop.toFront();
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (!m_model->isSelected(s))
            m_stops.append(s);
    }
    m_stops.removeAll(stop);
    m_stops.prepend(stop);
}

void QtGradientStopsWidgetPrivate::ensureVisible(double x)
{
    double viewX = toViewport(x);
    if (viewX < 0 || viewX > q_ptr->viewport()->size().width()) {
        int max = q_ptr->horizontalScrollBar()->maximum();
        int newVal = qRound(x * (max + m_scaleFactor) - m_scaleFactor / 2);
        q_ptr->horizontalScrollBar()->setValue(newVal);
    }
}

void QtGradientStopsWidgetPrivate::ensureVisible(QtGradientStop *stop)
{
    if (!stop)
        return;
    ensureVisible(stop->position());
}

QtGradientStop *QtGradientStopsWidgetPrivate::newStop(const QPoint &viewportPos)
{
    QtGradientStop *copyStop = stopAt(viewportPos);
    double posX = fromViewport(viewportPos.x());
    QtGradientStop *stop = m_model->at(posX);
    if (!stop) {
        QColor newColor;
        if (copyStop)
            newColor = copyStop->color();
        else
            newColor = m_model->color(posX);
        if (!newColor.isValid())
            newColor = Qt::white;
        stop = m_model->addStop(posX, newColor);
    }
    return stop;
}

void QtGradientStopsWidgetPrivate::slotStopAdded(QtGradientStop *stop)
{
    m_stops.append(stop);
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopRemoved(QtGradientStop *stop)
{
    m_stops.removeAll(stop);
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopMoved(QtGradientStop *stop, qreal newPos)
{
    Q_UNUSED(stop)
    Q_UNUSED(newPos)
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopChanged(QtGradientStop *stop, const QColor &newColor)
{
    Q_UNUSED(stop)
    Q_UNUSED(newColor)
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotStopSelected(QtGradientStop *stop, bool selected)
{
    Q_UNUSED(stop)
    Q_UNUSED(selected)
    q_ptr->viewport()->update();
}

void QtGradientStopsWidgetPrivate::slotCurrentStopChanged(QtGradientStop *stop)
{
    Q_UNUSED(stop)

    if (!m_model)
        return;
    q_ptr->viewport()->update();
    if (stop) {
        m_stops.removeAll(stop);
        m_stops.prepend(stop);
    }
}

void QtGradientStopsWidgetPrivate::slotNewStop()
{
    if (!m_model)
        return;

    QtGradientStop *stop = newStop(m_clickPos);

    if (!stop)
        return;

    m_model->clearSelection();
    m_model->selectStop(stop, true);
    m_model->setCurrentStop(stop);
}

void QtGradientStopsWidgetPrivate::slotDelete()
{
    if (!m_model)
        return;

    m_model->deleteStops();
}

void QtGradientStopsWidgetPrivate::slotSelectAll()
{
    if (!m_model)
        return;

    m_model->selectAll();
}

QtGradientStopsWidget::QtGradientStopsWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    d_ptr = new QtGradientStopsWidgetPrivate;
    d_ptr->q_ptr = this;
    d_ptr->m_backgroundTransparent = true;
    d_ptr->m_model = 0;
    d_ptr->m_handleSize = 25.0;
    d_ptr->m_scaleFactor = 1000;
    d_ptr->m_dragging = false;
    d_ptr->m_zoom = 1;
    d_ptr->m_rubber = new QRubberBand(QRubberBand::Rectangle, this);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    horizontalScrollBar()->setRange(0, (int)(d_ptr->m_scaleFactor * (d_ptr->m_zoom - 1) + 0.5));
    horizontalScrollBar()->setPageStep(d_ptr->m_scaleFactor);
    horizontalScrollBar()->setSingleStep(4);
    viewport()->setAutoFillBackground(false);

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
}

QtGradientStopsWidget::~QtGradientStopsWidget()
{
    delete d_ptr;
}

QSize QtGradientStopsWidget::sizeHint() const
{
    return QSize(qRound(2 * d_ptr->m_handleSize), qRound(2 * d_ptr->m_handleSize) + horizontalScrollBar()->sizeHint().height());
}

QSize QtGradientStopsWidget::minimumSizeHint() const
{
    return QSize(qRound(2 * d_ptr->m_handleSize), qRound(2 * d_ptr->m_handleSize) + horizontalScrollBar()->minimumSizeHint().height());
}

void QtGradientStopsWidget::setBackgroundTransparent(bool transparent)
{
    if (d_ptr->m_backgroundTransparent == transparent)
        return;
    d_ptr->m_backgroundTransparent = transparent;
    update();
}

bool QtGradientStopsWidget::backgroundTransparent() const
{
    return d_ptr->m_backgroundTransparent;
}

void QtGradientStopsWidget::setGradientStopsModel(QtGradientStopsModel *model)
{
    if (d_ptr->m_model == model)
        return;

    if (d_ptr->m_model) {
        disconnect(d_ptr->m_model, SIGNAL(stopAdded(QtGradientStop *)),
                    this, SLOT(slotStopAdded(QtGradientStop *)));
        disconnect(d_ptr->m_model, SIGNAL(stopRemoved(QtGradientStop *)),
                    this, SLOT(slotStopRemoved(QtGradientStop *)));
        disconnect(d_ptr->m_model, SIGNAL(stopMoved(QtGradientStop *, qreal)),
                    this, SLOT(slotStopMoved(QtGradientStop *, qreal)));
        disconnect(d_ptr->m_model, SIGNAL(stopChanged(QtGradientStop *, const QColor &)),
                    this, SLOT(slotStopChanged(QtGradientStop *, const QColor &)));
        disconnect(d_ptr->m_model, SIGNAL(stopSelected(QtGradientStop *, bool)),
                    this, SLOT(slotStopSelected(QtGradientStop *, bool)));
        disconnect(d_ptr->m_model, SIGNAL(currentStopChanged(QtGradientStop *)),
                    this, SLOT(slotCurrentStopChanged(QtGradientStop *)));

        d_ptr->m_stops.clear();
    }

    d_ptr->m_model = model;

    if (d_ptr->m_model) {
        connect(d_ptr->m_model, SIGNAL(stopAdded(QtGradientStop *)),
                    this, SLOT(slotStopAdded(QtGradientStop *)));
        connect(d_ptr->m_model, SIGNAL(stopRemoved(QtGradientStop *)),
                    this, SLOT(slotStopRemoved(QtGradientStop *)));
        connect(d_ptr->m_model, SIGNAL(stopMoved(QtGradientStop *, qreal)),
                    this, SLOT(slotStopMoved(QtGradientStop *, qreal)));
        connect(d_ptr->m_model, SIGNAL(stopChanged(QtGradientStop *, const QColor &)),
                    this, SLOT(slotStopChanged(QtGradientStop *, const QColor &)));
        connect(d_ptr->m_model, SIGNAL(stopSelected(QtGradientStop *, bool)),
                    this, SLOT(slotStopSelected(QtGradientStop *, bool)));
        connect(d_ptr->m_model, SIGNAL(currentStopChanged(QtGradientStop *)),
                    this, SLOT(slotCurrentStopChanged(QtGradientStop *)));

        QList<QtGradientStop *> stops = d_ptr->m_model->stops().values();
        QListIterator<QtGradientStop *> itStop(stops);
        while (itStop.hasNext())
            d_ptr->slotStopAdded(itStop.next());

        QList<QtGradientStop *> selected = d_ptr->m_model->selectedStops();
        QListIterator<QtGradientStop *> itSelect(selected);
        while (itSelect.hasNext())
            d_ptr->slotStopSelected(itSelect.next(), true);

        d_ptr->slotCurrentStopChanged(d_ptr->m_model->currentStop());
    }
}

void QtGradientStopsWidget::mousePressEvent(QMouseEvent *e)
{
    if (!d_ptr->m_model)
        return;

    if (e->button() != Qt::LeftButton)
        return;

    d_ptr->m_dragging = true;

    d_ptr->m_dragStops.clear();
    d_ptr->m_dragOriginal.clear();
    d_ptr->m_clickPos = e->pos();
    QtGradientStop *stop = d_ptr->stopAt(e->pos());
    if (stop) {
        if (e->modifiers() & Qt::ControlModifier) {
            d_ptr->m_model->selectStop(stop, !d_ptr->m_model->isSelected(stop));
        } else if (e->modifiers() & Qt::ShiftModifier) {
            QtGradientStop *oldCurrent = d_ptr->m_model->currentStop();
            if (oldCurrent) {
                QMap<qreal, QtGradientStop *> stops = d_ptr->m_model->stops();
                QMap<qreal, QtGradientStop *>::ConstIterator itSt = stops.constFind(oldCurrent->position());
                if (itSt != stops.constEnd()) {
                    while (itSt != stops.constFind(stop->position())) {
                        d_ptr->m_model->selectStop(itSt.value(), true);
                        if (oldCurrent->position() < stop->position())
                            itSt++;
                        else
                            itSt--;
                    }
                }
            }
            d_ptr->m_model->selectStop(stop, true);
        } else {
            if (!d_ptr->m_model->isSelected(stop)) {
                d_ptr->m_model->clearSelection();
                d_ptr->m_model->selectStop(stop, true);
            }
        }
        d_ptr->setupDrag(stop, e->pos().x());
    } else {
        d_ptr->m_model->clearSelection();
        d_ptr->m_rubber->setGeometry(QRect(d_ptr->m_clickPos, QSize()));
        d_ptr->m_rubber->show();
    }
    viewport()->update();
}

void QtGradientStopsWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (!d_ptr->m_model)
        return;

    if (e->button() != Qt::LeftButton)
        return;

    d_ptr->m_dragging = false;
    d_ptr->m_rubber->hide();
    d_ptr->m_dragStops.clear();
    d_ptr->m_dragOriginal.clear();
}

void QtGradientStopsWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (!d_ptr->m_model)
        return;

    if (!(e->buttons() & Qt::LeftButton))
        return;

    if (!d_ptr->m_dragging)
        return;

    if (!d_ptr->m_dragStops.isEmpty()) {
        double maxOffset = 0.0;
        double minOffset = 0.0;
        bool first = true;
        QMap<QtGradientStop *, qreal>::ConstIterator itStop = d_ptr->m_dragStops.constBegin();
        while (itStop != d_ptr->m_dragStops.constEnd()) {
            double offset = itStop.value();

            if (first) {
                maxOffset = offset;
                minOffset = offset;
                first = false;
            } else {
                if (maxOffset < offset)
                    maxOffset = offset;
                else if (minOffset > offset)
                    minOffset = offset;
            }
            itStop++;
        }

        double viewportMin = d_ptr->toViewport(-minOffset);
        double viewportMax = d_ptr->toViewport(1.0 - maxOffset);

        QMap<qreal, QtGradientStop *> newPositions;

        int viewportX = e->pos().x() - d_ptr->m_dragOffset;

        if (viewportX > viewport()->size().width())
            viewportX = viewport()->size().width();
        else if (viewportX < 0)
            viewportX = 0;

        double posX = d_ptr->fromViewport(viewportX);

        if (viewportX > viewportMax)
            posX = 1.0 - maxOffset;
        else if (viewportX < viewportMin)
            posX = -minOffset;

        itStop = d_ptr->m_dragStops.constBegin();
        while (itStop != d_ptr->m_dragStops.constEnd()) {
            QtGradientStop *stop = itStop.key();

            newPositions[posX + itStop.value()] = stop;

            itStop++;
        }

        bool forward = true;
        QMap<qreal, QtGradientStop *>::ConstIterator itNewPos = newPositions.constBegin();
        if (itNewPos.value()->position() < itNewPos.key())
            forward = false;

        itNewPos = forward ? newPositions.constBegin() : newPositions.constEnd();
        while (itNewPos != (forward ? newPositions.constEnd() : newPositions.constBegin())) {
            if (!forward)
                itNewPos--;
            QtGradientStop *stop = itNewPos.value();
            double newPos = itNewPos.key();
            if (newPos > 1)
                newPos = 1;
            else if (newPos < 0)
                newPos = 0;

            QtGradientStop *existingStop = d_ptr->m_model->at(newPos);
            if (existingStop && !d_ptr->m_dragStops.contains(existingStop))
                    d_ptr->m_model->removeStop(existingStop);
            d_ptr->m_model->moveStop(stop, newPos);

            if (forward)
                itNewPos++;
        }

        QMap<qreal, QColor>::ConstIterator itOld = d_ptr->m_dragOriginal.constBegin();
        while (itOld != d_ptr->m_dragOriginal.constEnd()) {
            double position = itOld.key();
            if (!d_ptr->m_model->at(position))
                d_ptr->m_model->addStop(position, itOld.value());

            itOld++;
        }

    } else {
        QRect r(QRect(d_ptr->m_clickPos, e->pos()).normalized());
        r.translate(1, 0);
        d_ptr->m_rubber->setGeometry(r);
        //d_ptr->m_model->clearSelection();

        int xv1 = d_ptr->m_clickPos.x();
        int xv2 = e->pos().x();
        if (xv1 > xv2) {
            int temp = xv1;
            xv1 = xv2;
            xv2 = temp;
        }
        int yv1 = d_ptr->m_clickPos.y();
        int yv2 = e->pos().y();
        if (yv1 > yv2) {
            int temp = yv1;
            yv1 = yv2;
            yv2 = temp;
        }

        QPoint p1, p2;

        if (yv2 < d_ptr->m_handleSize / 2) {
            p1 = QPoint(xv1, yv2);
            p2 = QPoint(xv2, yv2);
        } else if (yv1 > d_ptr->m_handleSize / 2) {
            p1 = QPoint(xv1, yv1);
            p2 = QPoint(xv2, yv1);
        } else {
            p1 = QPoint(xv1, qRound(d_ptr->m_handleSize / 2));
            p2 = QPoint(xv2, qRound(d_ptr->m_handleSize / 2));
        }

        QList<QtGradientStop *> beginList = d_ptr->stopsAt(p1);
        QList<QtGradientStop *> endList = d_ptr->stopsAt(p2);

        double x1 = d_ptr->fromViewport(xv1);
        double x2 = d_ptr->fromViewport(xv2);

        QListIterator<QtGradientStop *> itStop(d_ptr->m_stops);
        while (itStop.hasNext()) {
            QtGradientStop *stop = itStop.next();
            if ((stop->position() >= x1 && stop->position() <= x2) ||
                        beginList.contains(stop) || endList.contains(stop))
                d_ptr->m_model->selectStop(stop, true);
            else
                d_ptr->m_model->selectStop(stop, false);
        }
    }
}

void QtGradientStopsWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (!d_ptr->m_model)
        return;

    if (e->button() != Qt::LeftButton)
        return;

    if (d_ptr->m_clickPos != e->pos()) {
        mousePressEvent(e);
        return;
    }
    d_ptr->m_dragging = true;
    d_ptr->m_dragStops.clear();
    d_ptr->m_dragOriginal.clear();

    QtGradientStop *stop = d_ptr->newStop(e->pos());

    if (!stop)
        return;

    d_ptr->m_model->clearSelection();
    d_ptr->m_model->selectStop(stop, true);

    d_ptr->setupDrag(stop, e->pos().x());

    viewport()->update();
}

void QtGradientStopsWidget::keyPressEvent(QKeyEvent *e)
{
    if (!d_ptr->m_model)
        return;

    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
        d_ptr->m_model->deleteStops();
    } else if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right ||
                e->key() == Qt::Key_Home || e->key() == Qt::Key_End) {
        QMap<qreal, QtGradientStop *> stops = d_ptr->m_model->stops();
        if (stops.isEmpty())
            return;
        QtGradientStop *newCurrent = 0;
        QtGradientStop *current = d_ptr->m_model->currentStop();
        if (!current || e->key() == Qt::Key_Home || e->key() == Qt::Key_End) {
            if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Home)
                newCurrent = stops.constBegin().value();
            else if (e->key() == Qt::Key_Right || e->key() == Qt::Key_End)
                newCurrent = (--stops.constEnd()).value();
        } else {
            QMap<qreal, QtGradientStop *>::ConstIterator itStop = stops.constBegin();
            while (itStop.value() != current)
                itStop++;
            if (e->key() == Qt::Key_Left && itStop != stops.constBegin())
                itStop--;
            else if (e->key() == Qt::Key_Right && itStop != --stops.constEnd())
                itStop++;
            newCurrent = itStop.value();
        }
        d_ptr->m_model->clearSelection();
        d_ptr->m_model->selectStop(newCurrent, true);
        d_ptr->m_model->setCurrentStop(newCurrent);
        d_ptr->ensureVisible(newCurrent);
    } else if (e->key() == Qt::Key_A) {
        if (e->modifiers() & Qt::ControlModifier)
            d_ptr->m_model->selectAll();
    }
}

void QtGradientStopsWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    if (!d_ptr->m_model)
        return;

    QSize size = viewport()->size();
    int w = size.width();
    double h = size.height() - d_ptr->m_handleSize;
    if (w <= 0)
        return;

    QPixmap pix(size);
    QPainter p(viewport());

    if (!d_ptr->m_backgroundTransparent) {
        int pixSize = 20;
        QPixmap pm(2 * pixSize, 2 * pixSize);
        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);

        p.end();
        p.begin(&pix);
        p.setBrushOrigin((size.width() % pixSize + pixSize) / 2, (size.height() % pixSize + pixSize) / 2);
        p.fillRect(viewport()->rect(), pm);
        p.setBrushOrigin(0, 0);
    }

    double viewBegin = (double)w * horizontalScrollBar()->value() / d_ptr->m_scaleFactor;
    QListIterator<QtGradientStop *> itStop(d_ptr->m_stops);
    if (h > 0) {
        QLinearGradient lg(0, 0, w, 0);
        while (itStop.hasNext()) {
            QtGradientStop *stop = itStop.next();
            lg.setColorAt(stop->position(), stop->color());
        }
        QImage img(w, 1, QImage::Format_ARGB32_Premultiplied);
        QPainter p1(&img);
        p1.setCompositionMode(QPainter::CompositionMode_Source);

        if (viewBegin != 0)
            p1.translate(-viewBegin, 0);
        if (d_ptr->m_zoom != 1)
            p1.scale(d_ptr->m_zoom, 1);
        p1.fillRect(0, 0, w, 1, lg);

        p.fillRect(QRectF(0, d_ptr->m_handleSize, w, h), QPixmap::fromImage(img));
    }
    //p2.fillRect(QRectF(0, 0, w, size.height()), QPixmap::fromImage(img));
    // handles

    int val = horizontalScrollBar()->value();
    int max = horizontalScrollBar()->maximum();

    double begin = (double)val / (d_ptr->m_scaleFactor + max);
    double end = (double)(val + d_ptr->m_scaleFactor) / (d_ptr->m_scaleFactor + max);

    double handleWidth = d_ptr->m_handleSize * d_ptr->m_scaleFactor / (w * (d_ptr->m_scaleFactor + max));

    QColor insideColor = QColor::fromRgb(0x20, 0x20, 0x20, 0xFF);
    QColor borderColor = QColor(Qt::white);
    QColor drawColor;
    QColor back1 = QColor(Qt::lightGray);
    QColor back2 = QColor(Qt::darkGray);
    QColor back = QColor::fromRgb((back1.red() + back2.red()) / 2,
            (back1.green() + back2.green()) / 2,
            (back1.blue() + back2.blue()) / 2);

    QPen pen;
    p.setRenderHint(QPainter::Antialiasing);
    itStop.toBack();
    while (itStop.hasPrevious()) {
        QtGradientStop *stop = itStop.previous();
        double x = stop->position();
        if (x >= begin - handleWidth / 2 && x <= end + handleWidth / 2) {
            double viewX = x * w * (d_ptr->m_scaleFactor + max) / d_ptr->m_scaleFactor - viewBegin;
            p.save();
            QColor c = stop->color();
            if ((0.3 * c.redF() + 0.59 * c.greenF() + 0.11 * c.blueF()) * c.alphaF() +
                (0.3 * back.redF() + 0.59 * back.greenF() + 0.11 * back.blueF()) * (1.0 - c.alphaF()) < 0.5) {
                drawColor = QColor::fromRgb(0xC0, 0xC0, 0xC0, 0xB0);
            } else {
                drawColor = QColor::fromRgb(0x40, 0x40, 0x40, 0x80);
            }
            QRectF rect(viewX - d_ptr->m_handleSize / 2, 0, d_ptr->m_handleSize, d_ptr->m_handleSize);
            rect.adjust(0.5, 0.5, -0.5, -0.5);
            if (h > 0) {
                pen.setWidthF(1);
                QLinearGradient lg(0, d_ptr->m_handleSize, 0, d_ptr->m_handleSize + h / 2);
                lg.setColorAt(0, drawColor);
                QColor alphaZero = drawColor;
                alphaZero.setAlpha(0);
                lg.setColorAt(1, alphaZero);
                pen.setBrush(lg);
                p.setPen(pen);
                p.drawLine(QPointF(viewX, d_ptr->m_handleSize), QPointF(viewX, d_ptr->m_handleSize + h / 2));

                pen.setWidthF(1);
                pen.setBrush(drawColor);
                p.setPen(pen);
                QRectF r1 = rect.adjusted(0.5, 0.5, -0.5, -0.5);
                QRectF r2 = rect.adjusted(1.5, 1.5, -1.5, -1.5);
                QColor inColor = QColor::fromRgb(0x80, 0x80, 0x80, 0x80);
                if (!d_ptr->m_model->isSelected(stop)) {
                    p.setBrush(stop->color());
                    p.drawEllipse(rect);
                } else {
                    pen.setBrush(insideColor);
                    pen.setWidthF(2);
                    p.setPen(pen);
                    p.setBrush(Qt::NoBrush);
                    p.drawEllipse(r1);

                    pen.setBrush(inColor);
                    pen.setWidthF(1);
                    p.setPen(pen);
                    p.setBrush(stop->color());
                    p.drawEllipse(r2);
                }

                if (d_ptr->m_model->currentStop() == stop) {
                    p.setBrush(Qt::NoBrush);
                    pen.setWidthF(5);
                    pen.setBrush(drawColor);
                    int corr = 4;
                    if (!d_ptr->m_model->isSelected(stop)) {
                        corr = 3;
                        pen.setWidthF(7);
                    }
                    p.setPen(pen);
                    p.drawEllipse(rect.adjusted(corr, corr, -corr, -corr));
                }

            }
            p.restore();
        }
    }
    if (!d_ptr->m_backgroundTransparent) {
        p.end();
        p.begin(viewport());
        p.drawPixmap(0, 0, pix);
    }
}

void QtGradientStopsWidget::focusInEvent(QFocusEvent *e)
{
    Q_UNUSED(e)
    viewport()->update();
}

void QtGradientStopsWidget::focusOutEvent(QFocusEvent *e)
{
    Q_UNUSED(e)
    viewport()->update();
}

void QtGradientStopsWidget::contextMenuEvent(QContextMenuEvent *e)
{
    if (!d_ptr->m_model)
        return;

    d_ptr->m_clickPos = e->pos();

    QMenu menu(this);
    QAction *newStopAction = new QAction(tr("New Stop"), &menu);
    QAction *deleteAction = new QAction(tr("Delete"), &menu);
    QAction *selectAllAction = new QAction(tr("Select All"), &menu);
    if (d_ptr->m_model->selectedStops().isEmpty() && !d_ptr->m_model->currentStop())
        deleteAction->setEnabled(false);
    connect(newStopAction, SIGNAL(triggered()), this, SLOT(slotNewStop()));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(slotDelete()));
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(slotSelectAll()));
    menu.addAction(newStopAction);
    menu.addAction(deleteAction);
    menu.addAction(selectAllAction);
    menu.exec(e->globalPos());
}

void QtGradientStopsWidget::setZoom(double zoom)
{
    double z = zoom;
    if (z < 1)
        z = 1;
    else if (z > 100)
        z = 100;

    if (d_ptr->m_zoom == z)
        return;

    d_ptr->m_zoom = z;
    int oldMax = horizontalScrollBar()->maximum();
    int oldVal = horizontalScrollBar()->value();
    horizontalScrollBar()->setRange(0, qRound(d_ptr->m_scaleFactor * (d_ptr->m_zoom - 1)));
    int newMax = horizontalScrollBar()->maximum();
    double newVal = (oldVal + (double)d_ptr->m_scaleFactor / 2) * (newMax + d_ptr->m_scaleFactor)
                / (oldMax + d_ptr->m_scaleFactor) - (double)d_ptr->m_scaleFactor / 2;
    horizontalScrollBar()->setValue(qRound(newVal));
    viewport()->update();
}

double QtGradientStopsWidget::zoom() const
{
    return d_ptr->m_zoom;
}


#include "moc_qtgradientstopswidget.cpp"
