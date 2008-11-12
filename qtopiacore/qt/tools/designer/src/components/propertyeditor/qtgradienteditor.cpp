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
TRANSLATOR qdesigner_internal::QtGradientEditor
*/

#include "qtgradienteditor.h"
#include "ui_qtgradienteditor.h"

#include "qdebug.h"

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtGradientEditorPrivate
{
    QtGradientEditor *q_ptr;
    Q_DECLARE_PUBLIC(QtGradientEditor)
public:

    void slotGradientStopsChanged(const QGradientStops &stops);
    void slotTypeChanged(int type);
    void slotSpreadChanged(int spread);
    void slotStartLinearXChanged();
    void slotStartLinearYChanged();
    void slotEndLinearXChanged();
    void slotEndLinearYChanged();
    void slotCentralRadialXChanged();
    void slotCentralRadialYChanged();
    void slotFocalRadialXChanged();
    void slotFocalRadialYChanged();
    void slotRadiusRadialChanged();
    void slotCentralConicalXChanged();
    void slotCentralConicalYChanged();
    void slotAngleConicalChanged();
    void startLinearChanged(const QPointF &point);
    void endLinearChanged(const QPointF &point);
    void centralRadialChanged(const QPointF &point);
    void focalRadialChanged(const QPointF &point);
    void radiusRadialChanged(qreal radius);
    void centralConicalChanged(const QPointF &point);
    void angleConicalChanged(qreal angle);

    Ui::QtGradientEditor m_ui;
};

}

void QtGradientEditorPrivate::slotGradientStopsChanged(const QGradientStops &stops)
{
    m_ui.gradientWidget->setGradientStops(stops);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotTypeChanged(int type)
{
    if (type == 0) {
        m_ui.stackedWidget->setCurrentWidget(m_ui.linearPage);
        m_ui.gradientWidget->setGradientType(QGradient::LinearGradient);
    } else if (type == 1) {
        m_ui.stackedWidget->setCurrentWidget(m_ui.radialPage);
        m_ui.gradientWidget->setGradientType(QGradient::RadialGradient);
    } else if (type == 2) {
        m_ui.stackedWidget->setCurrentWidget(m_ui.conicalPage);
        m_ui.gradientWidget->setGradientType(QGradient::ConicalGradient);
    }
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotSpreadChanged(int spread)
{
    if (spread == 0) {
        m_ui.gradientWidget->setGradientSpread(QGradient::PadSpread);
    } else if (spread == 1) {
        m_ui.gradientWidget->setGradientSpread(QGradient::RepeatSpread);
    } else if (spread == 2) {
        m_ui.gradientWidget->setGradientSpread(QGradient::ReflectSpread);
    }
    m_ui.spreadLinearComboBox->setCurrentIndex(spread);
    m_ui.spreadRadialComboBox->setCurrentIndex(spread);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotStartLinearXChanged()
{
    QPointF point = m_ui.gradientWidget->startLinear();
    point.setX(m_ui.startLinearXSpinBox->value());
    m_ui.gradientWidget->setStartLinear(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotStartLinearYChanged()
{
    QPointF point = m_ui.gradientWidget->startLinear();
    point.setY(m_ui.startLinearYSpinBox->value());
    m_ui.gradientWidget->setStartLinear(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotEndLinearXChanged()
{
    QPointF point = m_ui.gradientWidget->endLinear();
    point.setX(m_ui.endLinearXSpinBox->value());
    m_ui.gradientWidget->setEndLinear(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotEndLinearYChanged()
{
    QPointF point = m_ui.gradientWidget->endLinear();
    point.setY(m_ui.endLinearYSpinBox->value());
    m_ui.gradientWidget->setEndLinear(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotCentralRadialXChanged()
{
    QPointF point = m_ui.gradientWidget->centralRadial();
    point.setX(m_ui.centralRadialXSpinBox->value());
    m_ui.gradientWidget->setCentralRadial(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotCentralRadialYChanged()
{
    QPointF point = m_ui.gradientWidget->centralRadial();
    point.setY(m_ui.centralRadialYSpinBox->value());
    m_ui.gradientWidget->setCentralRadial(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotFocalRadialXChanged()
{
    QPointF point = m_ui.gradientWidget->focalRadial();
    point.setX(m_ui.focalRadialXSpinBox->value());
    m_ui.gradientWidget->setFocalRadial(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotFocalRadialYChanged()
{
    QPointF point = m_ui.gradientWidget->focalRadial();
    point.setY(m_ui.focalRadialYSpinBox->value());
    m_ui.gradientWidget->setFocalRadial(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotRadiusRadialChanged()
{
    m_ui.gradientWidget->setRadiusRadial(m_ui.radiusRadialSpinBox->value());
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotCentralConicalXChanged()
{
    QPointF point = m_ui.gradientWidget->centralConical();
    point.setX(m_ui.centralConicalXSpinBox->value());
    m_ui.gradientWidget->setCentralConical(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotCentralConicalYChanged()
{
    QPointF point = m_ui.gradientWidget->centralConical();
    point.setY(m_ui.centralConicalYSpinBox->value());
    m_ui.gradientWidget->setCentralConical(point);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::slotAngleConicalChanged()
{
    m_ui.gradientWidget->setAngleConical(m_ui.angleConicalSpinBox->value());
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::startLinearChanged(const QPointF &point)
{
    m_ui.startLinearXSpinBox->setValue(point.x());
    m_ui.startLinearYSpinBox->setValue(point.y());
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::endLinearChanged(const QPointF &point)
{
    m_ui.endLinearXSpinBox->setValue(point.x());
    m_ui.endLinearYSpinBox->setValue(point.y());
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::centralRadialChanged(const QPointF &point)
{
    m_ui.centralRadialXSpinBox->setValue(point.x());
    m_ui.centralRadialYSpinBox->setValue(point.y());
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::focalRadialChanged(const QPointF &point)
{
    m_ui.focalRadialXSpinBox->setValue(point.x());
    m_ui.focalRadialYSpinBox->setValue(point.y());
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::radiusRadialChanged(qreal radius)
{
    m_ui.radiusRadialSpinBox->setValue(radius);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::centralConicalChanged(const QPointF &point)
{
    m_ui.centralConicalXSpinBox->setValue(point.x());
    m_ui.centralConicalYSpinBox->setValue(point.y());
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

void QtGradientEditorPrivate::angleConicalChanged(qreal angle)
{
    m_ui.angleConicalSpinBox->setValue(angle);
    emit q_ptr->gradientChanged(q_ptr->gradient());
}

QtGradientEditor::QtGradientEditor(QWidget *parent)
    : QWidget(parent)
{
    d_ptr = new QtGradientEditorPrivate();
    d_ptr->q_ptr = this;
    d_ptr->m_ui.setupUi(this);

    d_ptr->m_ui.startLinearXSpinBox->setValue(0);
    d_ptr->m_ui.startLinearYSpinBox->setValue(0);
    d_ptr->m_ui.endLinearXSpinBox->setValue(1);
    d_ptr->m_ui.endLinearYSpinBox->setValue(1);
    d_ptr->m_ui.centralRadialXSpinBox->setValue(0.5);
    d_ptr->m_ui.centralRadialYSpinBox->setValue(0.5);
    d_ptr->m_ui.focalRadialXSpinBox->setValue(0.5);
    d_ptr->m_ui.focalRadialYSpinBox->setValue(0.5);
    d_ptr->m_ui.radiusRadialSpinBox->setValue(0.5);
    d_ptr->m_ui.centralConicalXSpinBox->setValue(0.5);
    d_ptr->m_ui.centralConicalYSpinBox->setValue(0.5);
    d_ptr->m_ui.angleConicalSpinBox->setValue(0);

    connect(d_ptr->m_ui.gradientStopsEditor, SIGNAL(gradientStopsChanged(const QGradientStops &)),
                this, SLOT(slotGradientStopsChanged(const QGradientStops &)));

    QStringList types;
    types << tr("Linear") << tr("Radial") << tr("Conical");
    d_ptr->m_ui.typeComboBox->addItems(types);

    connect(d_ptr->m_ui.typeComboBox, SIGNAL(activated(int)),
                this, SLOT(slotTypeChanged(int)));

    QStringList spreads;
    spreads << tr("Pad") << tr("Repeat") << tr("Reflect");
    d_ptr->m_ui.spreadLinearComboBox->addItems(spreads);
    d_ptr->m_ui.spreadRadialComboBox->addItems(spreads);

    connect(d_ptr->m_ui.spreadLinearComboBox, SIGNAL(activated(int)),
                this, SLOT(slotSpreadChanged(int)));
    connect(d_ptr->m_ui.spreadRadialComboBox, SIGNAL(activated(int)),
                this, SLOT(slotSpreadChanged(int)));

    connect(d_ptr->m_ui.startLinearXSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotStartLinearXChanged()));
    connect(d_ptr->m_ui.startLinearYSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotStartLinearYChanged()));
    connect(d_ptr->m_ui.endLinearXSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotEndLinearXChanged()));
    connect(d_ptr->m_ui.endLinearYSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotEndLinearYChanged()));
    connect(d_ptr->m_ui.centralRadialXSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotCentralRadialXChanged()));
    connect(d_ptr->m_ui.centralRadialYSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotCentralRadialYChanged()));
    connect(d_ptr->m_ui.focalRadialXSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotFocalRadialXChanged()));
    connect(d_ptr->m_ui.focalRadialYSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotFocalRadialYChanged()));
    connect(d_ptr->m_ui.radiusRadialSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotRadiusRadialChanged()));
    connect(d_ptr->m_ui.centralConicalXSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotCentralConicalXChanged()));
    connect(d_ptr->m_ui.centralConicalYSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotCentralConicalYChanged()));
    connect(d_ptr->m_ui.angleConicalSpinBox, SIGNAL(editingFinished()),
                this, SLOT(slotAngleConicalChanged()));

    connect(d_ptr->m_ui.gradientWidget, SIGNAL(startLinearChanged(const QPointF &)),
                this, SLOT(startLinearChanged(const QPointF &)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(endLinearChanged(const QPointF &)),
                this, SLOT(endLinearChanged(const QPointF &)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(centralRadialChanged(const QPointF &)),
                this, SLOT(centralRadialChanged(const QPointF &)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(focalRadialChanged(const QPointF &)),
                this, SLOT(focalRadialChanged(const QPointF &)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(radiusRadialChanged(qreal)),
                this, SLOT(radiusRadialChanged(qreal)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(centralConicalChanged(const QPointF &)),
                this, SLOT(centralConicalChanged(const QPointF &)));
    connect(d_ptr->m_ui.gradientWidget, SIGNAL(angleConicalChanged(qreal)),
                this, SLOT(angleConicalChanged(qreal)));
}

QtGradientEditor::~QtGradientEditor()
{
    delete d_ptr;
}

void QtGradientEditor::setGradient(const QGradient &grad)
{
    if (grad == gradient())
        return;

    QGradient::Type type = grad.type();
    QWidget *page = 0;
    int idx = 0;
    switch (type) {
        case QGradient::LinearGradient:
            page = d_ptr->m_ui.linearPage;
            idx = 0;
            break;
        case QGradient::RadialGradient:
            page = d_ptr->m_ui.radialPage;
            idx = 1;
            break;
        case QGradient::ConicalGradient:
            page = d_ptr->m_ui.conicalPage;
            idx = 2;
            break;
        default:
            return;
    }
    d_ptr->m_ui.stackedWidget->setCurrentWidget(page);
    d_ptr->m_ui.typeComboBox->setCurrentIndex(idx);
    d_ptr->m_ui.gradientWidget->setGradientType(type);

    QGradient::Spread spread = grad.spread();
    switch (spread) {
        case QGradient::PadSpread: idx = 0; break;
        case QGradient::RepeatSpread: idx = 1; break;
        case QGradient::ReflectSpread: idx = 2; break;
    }
    d_ptr->m_ui.spreadLinearComboBox->setCurrentIndex(idx);
    d_ptr->m_ui.spreadRadialComboBox->setCurrentIndex(idx);
    d_ptr->m_ui.gradientWidget->setGradientSpread(spread);

    if (type == QGradient::LinearGradient) {
        QLinearGradient *gr = (QLinearGradient *)(&grad);
        d_ptr->m_ui.startLinearXSpinBox->setValue(gr->start().x());
        d_ptr->m_ui.startLinearYSpinBox->setValue(gr->start().y());
        d_ptr->m_ui.endLinearXSpinBox->setValue(gr->finalStop().x());
        d_ptr->m_ui.endLinearYSpinBox->setValue(gr->finalStop().y());
        d_ptr->m_ui.gradientWidget->setStartLinear(gr->start());
        d_ptr->m_ui.gradientWidget->setEndLinear(gr->finalStop());
    } else if (type == QGradient::RadialGradient) {
        QRadialGradient *gr = (QRadialGradient *)(&grad);
        d_ptr->m_ui.centralRadialXSpinBox->setValue(gr->center().x());
        d_ptr->m_ui.centralRadialYSpinBox->setValue(gr->center().y());
        d_ptr->m_ui.focalRadialXSpinBox->setValue(gr->focalPoint().x());
        d_ptr->m_ui.focalRadialYSpinBox->setValue(gr->focalPoint().y());
        d_ptr->m_ui.radiusRadialSpinBox->setValue(gr->radius());
        d_ptr->m_ui.gradientWidget->setCentralRadial(gr->center());
        d_ptr->m_ui.gradientWidget->setFocalRadial(gr->focalPoint());
        d_ptr->m_ui.gradientWidget->setRadiusRadial(gr->radius());
    } else if (type == QGradient::ConicalGradient) {
        QConicalGradient *gr = (QConicalGradient *)(&grad);
        d_ptr->m_ui.centralConicalXSpinBox->setValue(gr->center().x());
        d_ptr->m_ui.centralConicalYSpinBox->setValue(gr->center().y());
        d_ptr->m_ui.angleConicalSpinBox->setValue(gr->angle());
        d_ptr->m_ui.gradientWidget->setCentralConical(gr->center());
        d_ptr->m_ui.gradientWidget->setAngleConical(gr->angle());
    }

    d_ptr->m_ui.gradientStopsEditor->setGradientStops(grad.stops());
    d_ptr->m_ui.gradientWidget->setGradientStops(grad.stops());
}

QGradient QtGradientEditor::gradient() const
{
    QGradient *gradient = 0;
    switch (d_ptr->m_ui.gradientWidget->gradientType()) {
        case QGradient::LinearGradient:
            gradient = new QLinearGradient(d_ptr->m_ui.gradientWidget->startLinear(),
                        d_ptr->m_ui.gradientWidget->endLinear());
            break;
        case QGradient::RadialGradient:
            gradient = new QRadialGradient(d_ptr->m_ui.gradientWidget->centralRadial(),
                        d_ptr->m_ui.gradientWidget->radiusRadial(),
                        d_ptr->m_ui.gradientWidget->focalRadial());
            break;
        case QGradient::ConicalGradient:
            gradient = new QConicalGradient(d_ptr->m_ui.gradientWidget->centralConical(),
                        d_ptr->m_ui.gradientWidget->angleConical());
            break;
        default:
            break;
    }
    if (!gradient)
        return QGradient();
    gradient->setStops(d_ptr->m_ui.gradientWidget->gradientStops());
    gradient->setSpread(d_ptr->m_ui.gradientWidget->gradientSpread());
    gradient->setCoordinateMode(QGradient::StretchToDeviceMode);
    QGradient gr = *gradient;
    delete gradient;
    return gr;
}

#include "moc_qtgradienteditor.cpp"
