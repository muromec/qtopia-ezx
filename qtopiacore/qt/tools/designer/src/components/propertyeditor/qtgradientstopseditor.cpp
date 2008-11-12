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
TRANSLATOR qdesigner_internal::QtGradientStopsEditor
*/

#include "qtgradientstopseditor.h"
#include "ui_qtgradientstopseditor.h"
#include "qtgradientstopsmodel.h"
#include <QTimer>

#include "qdebug.h"

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtGradientStopsEditorPrivate
{
    QtGradientStopsEditor *q_ptr;
    Q_DECLARE_PUBLIC(QtGradientStopsEditor)
public:
    void slotHsvClicked();
    void slotRgbClicked();

    void slotCurrentStopChanged(QtGradientStop *stop);
    void slotStopMoved(QtGradientStop *stop, qreal newPos);
    void slotStopChanged(QtGradientStop *stop, const QColor &newColor);
    void slotStopSelected(QtGradientStop *stop, bool selected);
    void slotStopAdded(QtGradientStop *stop);
    void slotStopRemoved(QtGradientStop *stop);
    void slotUpdatePositionSpinBox();

    void slotChangeColor(const QColor &color);
    void slotChangeHue(const QColor &color);
    void slotChangeSaturation(const QColor &color);
    void slotChangeValue(const QColor &color);
    void slotChangeAlpha(const QColor &color);
    void slotChangeHue(int color);
    void slotChangeSaturation(int color);
    void slotChangeValue(int color);
    void slotChangeAlpha(int color);
    void slotChangePosition();

    void slotChangeZoom();
    void slotZoomIn();
    void slotZoomOut();
    void slotZoomAll();

    void enableCurrent(bool enable);
    void setColorSpinBoxes(const QColor &color);
    QMap<qreal, QColor> stopsData(const QMap<qreal, QtGradientStop *> &stops) const;
    QGradientStops makeGradientStops(const QMap<qreal, QColor> &data) const;
    void updateZoom();

    QtGradientStopsModel *m_model;

    Ui::QtGradientStopsEditor m_ui;
};

}

void QtGradientStopsEditorPrivate::enableCurrent(bool enable)
{
    m_ui.positionLabel->setEnabled(enable);
    m_ui.colorLabel->setEnabled(enable);
    m_ui.hueLabel->setEnabled(enable);
    m_ui.saturationLabel->setEnabled(enable);
    m_ui.valueLabel->setEnabled(enable);
    m_ui.alphaLabel->setEnabled(enable);

    m_ui.positionSpinBox->setEnabled(enable);
    m_ui.colorButton->setEnabled(enable);

    m_ui.hueColorLine->setEnabled(enable);
    m_ui.saturationColorLine->setEnabled(enable);
    m_ui.valueColorLine->setEnabled(enable);
    m_ui.alphaColorLine->setEnabled(enable);

    m_ui.hueSpinBox->setEnabled(enable);
    m_ui.saturationSpinBox->setEnabled(enable);
    m_ui.valueSpinBox->setEnabled(enable);
    m_ui.alphaSpinBox->setEnabled(enable);
}

QMap<qreal, QColor> QtGradientStopsEditorPrivate::stopsData(const QMap<qreal, QtGradientStop *> &stops) const
{
    QMap<qreal, QColor> data;
    QMap<qreal, QtGradientStop *>::ConstIterator itStop = stops.constBegin();
    while (itStop != stops.constEnd()) {
        QtGradientStop *stop = itStop.value();
        data[stop->position()] = stop->color();

        itStop++;
    }
    return data;
}

QGradientStops QtGradientStopsEditorPrivate::makeGradientStops(const QMap<qreal, QColor> &data) const
{
    QGradientStops stops;
    QMap<qreal, QColor>::ConstIterator itData = data.constBegin();
    while (itData != data.constEnd()) {
        stops << QPair<qreal, QColor>(itData.key(), itData.value());

        itData++;
    }
    return stops;
}

void QtGradientStopsEditorPrivate::updateZoom()
{
    int zoom = qRound(m_ui.gradientWidget->zoom() * 100);
    bool zoomInEnabled = true;
    bool zoomOutEnabled = true;
    bool zoomAllEnabled = true;
    if (zoom <= 100) {
        zoomAllEnabled = false;
        zoomOutEnabled = false;
    } else if (zoom >= 10000) {
        zoomInEnabled = false;
    }
    m_ui.zoomInButton->setEnabled(zoomInEnabled);
    m_ui.zoomOutButton->setEnabled(zoomOutEnabled);
    m_ui.zoomAllButton->setEnabled(zoomAllEnabled);
}

void QtGradientStopsEditorPrivate::slotHsvClicked()
{
    m_ui.hueLabel->setText(QApplication::translate("qdesigner_internal::QtGradientStopsEditor", "Hue", 0, QApplication::UnicodeUTF8));
    m_ui.saturationLabel->setText(QApplication::translate("qdesigner_internal::QtGradientStopsEditor", "Saturation", 0, QApplication::UnicodeUTF8));
    m_ui.valueLabel->setText(QApplication::translate("qdesigner_internal::QtGradientStopsEditor", "Value", 0, QApplication::UnicodeUTF8));
    m_ui.hueColorLine->setColorComponent(QtColorLine::Hue);
    m_ui.saturationColorLine->setColorComponent(QtColorLine::Saturation);
    m_ui.valueColorLine->setColorComponent(QtColorLine::Value);
    setColorSpinBoxes(m_ui.colorButton->color());
}

void QtGradientStopsEditorPrivate::slotRgbClicked()
{
    m_ui.hueLabel->setText(QApplication::translate("qdesigner_internal::QtGradientStopsEditor", "Red", 0, QApplication::UnicodeUTF8));
    m_ui.saturationLabel->setText(QApplication::translate("qdesigner_internal::QtGradientStopsEditor", "Green", 0, QApplication::UnicodeUTF8));
    m_ui.valueLabel->setText(QApplication::translate("qdesigner_internal::QtGradientStopsEditor", "Blue", 0, QApplication::UnicodeUTF8));
    m_ui.hueColorLine->setColorComponent(QtColorLine::Red);
    m_ui.saturationColorLine->setColorComponent(QtColorLine::Green);
    m_ui.valueColorLine->setColorComponent(QtColorLine::Blue);
    setColorSpinBoxes(m_ui.colorButton->color());
}

void QtGradientStopsEditorPrivate::setColorSpinBoxes(const QColor &color)
{
    m_ui.hueSpinBox->blockSignals(true);
    m_ui.saturationSpinBox->blockSignals(true);
    m_ui.valueSpinBox->blockSignals(true);
    m_ui.alphaSpinBox->blockSignals(true);
    if (m_ui.hsvRadioButton->isChecked()) {
        if (m_ui.hueSpinBox->maximum() != 359)
            m_ui.hueSpinBox->setMaximum(359);
        if (m_ui.hueSpinBox->value() != color.hue())
            m_ui.hueSpinBox->setValue(color.hue());
        if (m_ui.saturationSpinBox->value() != color.saturation())
            m_ui.saturationSpinBox->setValue(color.saturation());
        if (m_ui.valueSpinBox->value() != color.value())
            m_ui.valueSpinBox->setValue(color.value());
    } else {
        if (m_ui.hueSpinBox->maximum() != 255)
            m_ui.hueSpinBox->setMaximum(255);
        if (m_ui.hueSpinBox->value() != color.red())
            m_ui.hueSpinBox->setValue(color.red());
        if (m_ui.saturationSpinBox->value() != color.green())
            m_ui.saturationSpinBox->setValue(color.green());
        if (m_ui.valueSpinBox->value() != color.blue())
            m_ui.valueSpinBox->setValue(color.blue());
    }
    m_ui.alphaSpinBox->setValue(color.alpha());
    m_ui.hueSpinBox->blockSignals(false);
    m_ui.saturationSpinBox->blockSignals(false);
    m_ui.valueSpinBox->blockSignals(false);
    m_ui.alphaSpinBox->blockSignals(false);
}

void QtGradientStopsEditorPrivate::slotCurrentStopChanged(QtGradientStop *stop)
{
    if (!stop) {
        enableCurrent(false);
        return;
    }
    enableCurrent(true);

    QTimer::singleShot(0, q_ptr, SLOT(slotUpdatePositionSpinBox()));

    m_ui.colorButton->setColor(stop->color());
    m_ui.hueColorLine->setColor(stop->color());
    m_ui.saturationColorLine->setColor(stop->color());
    m_ui.valueColorLine->setColor(stop->color());
    m_ui.alphaColorLine->setColor(stop->color());
    setColorSpinBoxes(stop->color());
}

void QtGradientStopsEditorPrivate::slotStopMoved(QtGradientStop *stop, qreal newPos)
{
    QTimer::singleShot(0, q_ptr, SLOT(slotUpdatePositionSpinBox()));

    QMap<qreal, QColor> stops = stopsData(m_model->stops());
    stops.remove(stop->position());
    stops[newPos] = stop->color();

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsEditorPrivate::slotStopAdded(QtGradientStop *stop)
{
    QMap<qreal, QColor> stops = stopsData(m_model->stops());
    stops[stop->position()] = stop->color();

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsEditorPrivate::slotStopRemoved(QtGradientStop *stop)
{
    QMap<qreal, QColor> stops = stopsData(m_model->stops());
    stops.remove(stop->position());

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsEditorPrivate::slotStopChanged(QtGradientStop *stop, const QColor &newColor)
{
    if (m_model->currentStop() == stop) {
        m_ui.colorButton->setColor(newColor);
        m_ui.hueColorLine->setColor(newColor);
        m_ui.saturationColorLine->setColor(newColor);
        m_ui.valueColorLine->setColor(newColor);
        m_ui.alphaColorLine->setColor(newColor);
        setColorSpinBoxes(newColor);
    }

    QMap<qreal, QColor> stops = stopsData(m_model->stops());
    stops[stop->position()] = newColor;

    QGradientStops gradStops = makeGradientStops(stops);
    emit q_ptr->gradientStopsChanged(gradStops);
}

void QtGradientStopsEditorPrivate::slotStopSelected(QtGradientStop *stop, bool selected)
{
    Q_UNUSED(stop)
    Q_UNUSED(selected)
    QTimer::singleShot(0, q_ptr, SLOT(slotUpdatePositionSpinBox()));
}

void QtGradientStopsEditorPrivate::slotUpdatePositionSpinBox()
{
    QtGradientStop *current = m_model->currentStop();
    if (!current)
        return;

    double min = 0.0;
    double max = 1.0;
    double pos = current->position();

    QtGradientStop *first = m_model->firstSelected();
    QtGradientStop *last = m_model->lastSelected();

    if (first && last) {
        double minPos = pos - first->position() - 0.0004999;
        double maxPos = pos + 1.0 - last->position() + 0.0004999;

        if (max > maxPos)
            max = maxPos;
        if (min < minPos)
            min = minPos;

        if (first->position() == 0.0)
            min = pos;
        if (last->position() == 1.0)
            max = pos;
    }

    int spinMin = qRound(m_ui.positionSpinBox->minimum() * 1000);
    int spinMax = qRound(m_ui.positionSpinBox->maximum() * 1000);

    int newMin = qRound(min * 1000);
    int newMax = qRound(max * 1000);

    m_ui.positionSpinBox->blockSignals(true);
    if (spinMin != newMin || spinMax != newMax) {
        m_ui.positionSpinBox->setRange((double)newMin / 1000, (double)newMax / 1000);
    }
    if (m_ui.positionSpinBox->value() != pos)
        m_ui.positionSpinBox->setValue(pos);
    m_ui.positionSpinBox->blockSignals(false);
}

void QtGradientStopsEditorPrivate::slotChangeColor(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop)
            m_model->changeStop(s, color);
    }
}

void QtGradientStopsEditorPrivate::slotChangeHue(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop) {
            QColor c = s->color();
            if (m_ui.hsvRadioButton->isChecked())
                c.setHsvF(color.hueF(), c.saturationF(), c.valueF(), c.alphaF());
            else
                c.setRgbF(color.redF(), c.greenF(), c.blueF(), c.alphaF());
            m_model->changeStop(s, c);
        }
    }
}

void QtGradientStopsEditorPrivate::slotChangeHue(int color)
{
    QColor c = m_ui.hueColorLine->color();
    if (m_ui.hsvRadioButton->isChecked())
        c.setHsvF((qreal)color / 360.0, c.saturationF(), c.valueF(), c.alphaF());
    else
        c.setRed(color);
    slotChangeHue(c);
}

void QtGradientStopsEditorPrivate::slotChangeSaturation(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop) {
            QColor c = s->color();
            if (m_ui.hsvRadioButton->isChecked()) {
                c.setHsvF(c.hueF(), color.saturationF(), c.valueF(), c.alphaF());
                int hue = c.hue();
                if (hue == 360 || hue == -1)
                    c.setHsvF(0.0, c.saturationF(), c.valueF(), c.alphaF());
            } else {
                c.setRgbF(c.redF(), color.greenF(), c.blueF(), c.alphaF());
            }
            m_model->changeStop(s, c);
        }
    }
}

void QtGradientStopsEditorPrivate::slotChangeSaturation(int color)
{
    QColor c = m_ui.saturationColorLine->color();
    if (m_ui.hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), (qreal)color / 255, c.valueF(), c.alphaF());
    else
        c.setGreen(color);
    slotChangeSaturation(c);
}

void QtGradientStopsEditorPrivate::slotChangeValue(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop) {
            QColor c = s->color();
            if (m_ui.hsvRadioButton->isChecked()) {
                c.setHsvF(c.hueF(), c.saturationF(), color.valueF(), c.alphaF());
                int hue = c.hue();
                if (hue == 360 || hue == -1)
                    c.setHsvF(0.0, c.saturationF(), c.valueF(), c.alphaF());
            } else {
                c.setRgbF(c.redF(), c.greenF(), color.blueF(), c.alphaF());
            }
            m_model->changeStop(s, c);
        }
    }
}

void QtGradientStopsEditorPrivate::slotChangeValue(int color)
{
    QColor c = m_ui.valueColorLine->color();
    if (m_ui.hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), c.saturationF(), (qreal)color / 255, c.alphaF());
    else
        c.setBlue(color);
    slotChangeValue(c);
}

void QtGradientStopsEditorPrivate::slotChangeAlpha(const QColor &color)
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;
    m_model->changeStop(stop, color);
    QList<QtGradientStop *> stops = m_model->selectedStops();
    QListIterator<QtGradientStop *> itStop(stops);
    while (itStop.hasNext()) {
        QtGradientStop *s = itStop.next();
        if (s != stop) {
            QColor c = s->color();
            if (m_ui.hsvRadioButton->isChecked()) {
                c.setHsvF(c.hueF(), c.saturationF(), c.valueF(), color.alphaF());
                int hue = c.hue();
                if (hue == 360 || hue == -1)
                    c.setHsvF(0.0, c.saturationF(), c.valueF(), c.alphaF());
            } else {
                c.setRgbF(c.redF(), c.greenF(), c.blueF(), color.alphaF());
            }
            m_model->changeStop(s, c);
        }
    }
}

void QtGradientStopsEditorPrivate::slotChangeAlpha(int color)
{
    QColor c = m_ui.alphaColorLine->color();
    if (m_ui.hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), c.saturationF(), c.valueF(), (qreal)color / 255);
    else
        c.setAlpha(color);
    slotChangeAlpha(c);
}

void QtGradientStopsEditorPrivate::slotChangePosition()
{
    QtGradientStop *stop = m_model->currentStop();
    if (!stop)
        return;

    m_model->moveStops(m_ui.positionSpinBox->value());
}

void QtGradientStopsEditorPrivate::slotChangeZoom()
{
    int zoom = m_ui.zoomSpinBox->value();
    m_ui.gradientWidget->setZoom((double)zoom / 100);
    updateZoom();
}

void QtGradientStopsEditorPrivate::slotZoomIn()
{
    int zoom = m_ui.zoomSpinBox->value();
    int newZoom = zoom * 2;
    if (newZoom > 10000)
        newZoom = 10000;
    m_ui.gradientWidget->setZoom((double)newZoom / 100);
    m_ui.zoomSpinBox->blockSignals(true);
    m_ui.zoomSpinBox->setValue(newZoom);
    m_ui.zoomSpinBox->blockSignals(false);
    updateZoom();
}

void QtGradientStopsEditorPrivate::slotZoomOut()
{
    int zoom = m_ui.zoomSpinBox->value();
    int newZoom = zoom / 2;
    if (newZoom < 100)
        newZoom = 100;
    m_ui.gradientWidget->setZoom((double)newZoom / 100);
    m_ui.zoomSpinBox->blockSignals(true);
    m_ui.zoomSpinBox->setValue(newZoom);
    m_ui.zoomSpinBox->blockSignals(false);
    updateZoom();
}

void QtGradientStopsEditorPrivate::slotZoomAll()
{
    m_ui.gradientWidget->setZoom(1);
    m_ui.zoomSpinBox->blockSignals(true);
    m_ui.zoomSpinBox->setValue(100);
    m_ui.zoomSpinBox->blockSignals(false);
    updateZoom();
}

QtGradientStopsEditor::QtGradientStopsEditor(QWidget *parent)
    : QWidget(parent)
{
    d_ptr = new QtGradientStopsEditorPrivate();
    d_ptr->q_ptr = this;
    d_ptr->m_ui.setupUi(this);

    d_ptr->m_ui.hueColorLine->setColorComponent(QtColorLine::Hue);
    d_ptr->m_ui.saturationColorLine->setColorComponent(QtColorLine::Saturation);
    d_ptr->m_ui.valueColorLine->setColorComponent(QtColorLine::Value);
    d_ptr->m_ui.alphaColorLine->setColorComponent(QtColorLine::Alpha);

    d_ptr->m_model = new QtGradientStopsModel(this);
    d_ptr->m_ui.gradientWidget->setGradientStopsModel(d_ptr->m_model);
    connect(d_ptr->m_model, SIGNAL(currentStopChanged(QtGradientStop *)),
                this, SLOT(slotCurrentStopChanged(QtGradientStop *)));
    connect(d_ptr->m_model, SIGNAL(stopMoved(QtGradientStop *, qreal)),
                this, SLOT(slotStopMoved(QtGradientStop *, qreal)));
    connect(d_ptr->m_model, SIGNAL(stopChanged(QtGradientStop *, const QColor &)),
                this, SLOT(slotStopChanged(QtGradientStop *, const QColor &)));
    connect(d_ptr->m_model, SIGNAL(stopSelected(QtGradientStop *, bool)),
                this, SLOT(slotStopSelected(QtGradientStop *, bool)));
    connect(d_ptr->m_model, SIGNAL(stopAdded(QtGradientStop *)),
                this, SLOT(slotStopAdded(QtGradientStop *)));
    connect(d_ptr->m_model, SIGNAL(stopRemoved(QtGradientStop *)),
                this, SLOT(slotStopRemoved(QtGradientStop *)));

    connect(d_ptr->m_ui.hueColorLine, SIGNAL(colorChanged(const QColor &)),
                this, SLOT(slotChangeHue(const QColor &)));
    connect(d_ptr->m_ui.saturationColorLine, SIGNAL(colorChanged(const QColor &)),
                this, SLOT(slotChangeSaturation(const QColor &)));
    connect(d_ptr->m_ui.valueColorLine, SIGNAL(colorChanged(const QColor &)),
                this, SLOT(slotChangeValue(const QColor &)));
    connect(d_ptr->m_ui.alphaColorLine, SIGNAL(colorChanged(const QColor &)),
                this, SLOT(slotChangeAlpha(const QColor &)));
    connect(d_ptr->m_ui.colorButton, SIGNAL(colorChanged(const QColor &)),
                this, SLOT(slotChangeColor(const QColor &)));

    connect(d_ptr->m_ui.hueSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeHue(int)));
    connect(d_ptr->m_ui.saturationSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeSaturation(int)));
    connect(d_ptr->m_ui.valueSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeValue(int)));
    connect(d_ptr->m_ui.alphaSpinBox, SIGNAL(valueChanged(int)),
                this, SLOT(slotChangeAlpha(int)));

    connect(d_ptr->m_ui.positionSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotChangePosition()));

    connect(d_ptr->m_ui.zoomSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotChangeZoom()));
    connect(d_ptr->m_ui.zoomInButton, SIGNAL(clicked()),
                this, SLOT(slotZoomIn()));
    connect(d_ptr->m_ui.zoomOutButton, SIGNAL(clicked()),
                this, SLOT(slotZoomOut()));
    connect(d_ptr->m_ui.zoomAllButton, SIGNAL(clicked()),
                this, SLOT(slotZoomAll()));

    connect(d_ptr->m_ui.hsvRadioButton, SIGNAL(clicked()),
                this, SLOT(slotHsvClicked()));
    connect(d_ptr->m_ui.rgbRadioButton, SIGNAL(clicked()),
                this, SLOT(slotRgbClicked()));

    d_ptr->enableCurrent(false);
    d_ptr->m_ui.zoomInButton->setIcon(QIcon(QLatin1String(":/qtgradienteditor/images/zoomin.png")));
    d_ptr->m_ui.zoomOutButton->setIcon(QIcon(QLatin1String(":/qtgradienteditor/images/zoomout.png")));
    d_ptr->updateZoom();
}

QtGradientStopsEditor::~QtGradientStopsEditor()
{
    delete d_ptr;
}

void QtGradientStopsEditor::setGradientStops(const QGradientStops &stops)
{
    d_ptr->m_model->clear();
    QVectorIterator<QPair<qreal, QColor> > it(stops);
    QtGradientStop *first = 0;
    while (it.hasNext()) {
        QPair<qreal, QColor> pair = it.next();
        QtGradientStop *stop = d_ptr->m_model->addStop(pair.first, pair.second);
        if (!first)
            first = stop;
    }
    if (first)
        d_ptr->m_model->setCurrentStop(first);
}

QGradientStops QtGradientStopsEditor::gradientStops() const
{
    QGradientStops stops;
    QList<QtGradientStop *> stopsList = d_ptr->m_model->stops().values();
    QListIterator<QtGradientStop *> itStop(stopsList);
    while (itStop.hasNext()) {
        QtGradientStop *stop = itStop.next();
        stops << QPair<qreal, QColor>(stop->position(), stop->color());
    }
    return stops;
}

#include "moc_qtgradientstopseditor.cpp"
