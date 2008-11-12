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
TRANSLATOR qdesigner_internal::QtBrushPatternEditor
*/

#include "qtbrushpatterneditor.h"
#include "ui_qtbrushpatterneditor.h"

#include "qdebug.h"

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtBrushPatternEditorPrivate
{
    QtBrushPatternEditor *q_ptr;
    Q_DECLARE_PUBLIC(QtBrushPatternEditor)
public:
    void slotHsvClicked();
    void slotRgbClicked();
    void slotPatternChanged(int pattern);
    void slotChangeColor(const QColor &color);
    void slotChangeHue(const QColor &color);
    void slotChangeSaturation(const QColor &color);
    void slotChangeValue(const QColor &color);
    void slotChangeAlpha(const QColor &color);
    void slotChangeHue(int color);
    void slotChangeSaturation(int color);
    void slotChangeValue(int color);
    void slotChangeAlpha(int color);

    void setColorSpinBoxes(const QColor &color);

    QBrush m_brush;

    Ui::QtBrushPatternEditor m_ui;
};

}

void QtBrushPatternEditorPrivate::slotHsvClicked()
{
    m_ui.hueLabel->setText(QApplication::translate("qdesigner_internal::QtBrushPatternEditor", "Hue", 0, QApplication::UnicodeUTF8));
    m_ui.saturationLabel->setText(QApplication::translate("qdesigner_internal::QtBrushPatternEditor", "Saturation", 0, QApplication::UnicodeUTF8));
    m_ui.valueLabel->setText(QApplication::translate("qdesigner_internal::QtBrushPatternEditor", "Value", 0, QApplication::UnicodeUTF8));
    m_ui.hueColorLine->setColorComponent(QtColorLine::Hue);
    m_ui.saturationColorLine->setColorComponent(QtColorLine::Saturation);
    m_ui.valueColorLine->setColorComponent(QtColorLine::Value);
    setColorSpinBoxes(m_ui.colorButton->color());
}

void QtBrushPatternEditorPrivate::slotRgbClicked()
{
    m_ui.hueLabel->setText(QApplication::translate("qdesigner_internal::QtBrushPatternEditor", "Red", 0, QApplication::UnicodeUTF8));
    m_ui.saturationLabel->setText(QApplication::translate("qdesigner_internal::QtBrushPatternEditor", "Green", 0, QApplication::UnicodeUTF8));
    m_ui.valueLabel->setText(QApplication::translate("qdesigner_internal::QtBrushPatternEditor", "Blue", 0, QApplication::UnicodeUTF8));
    m_ui.hueColorLine->setColorComponent(QtColorLine::Red);
    m_ui.saturationColorLine->setColorComponent(QtColorLine::Green);
    m_ui.valueColorLine->setColorComponent(QtColorLine::Blue);
    setColorSpinBoxes(m_ui.colorButton->color());
}

void QtBrushPatternEditorPrivate::slotPatternChanged(int pattern)
{
    QBrush brush = m_brush;
    brush.setStyle((Qt::BrushStyle)pattern);
    q_ptr->setBrush(brush);
}

void QtBrushPatternEditorPrivate::slotChangeColor(const QColor &color)
{
    QBrush brush = m_brush;
    brush.setColor(color);
    q_ptr->setBrush(brush);
}

void QtBrushPatternEditorPrivate::slotChangeHue(const QColor &color)
{
    QBrush brush = m_brush;
    brush.setColor(color);
    q_ptr->setBrush(brush);
}

void QtBrushPatternEditorPrivate::slotChangeHue(int color)
{
    QColor c = m_ui.hueColorLine->color();
    if (m_ui.hsvRadioButton->isChecked())
        c.setHsvF((qreal)color / 360.0, c.saturationF(), c.valueF(), c.alphaF());
    else
        c.setRed(color);
    slotChangeHue(c);
}

void QtBrushPatternEditorPrivate::slotChangeSaturation(const QColor &color)
{
    QBrush brush = m_brush;
    brush.setColor(color);
    q_ptr->setBrush(brush);
}

void QtBrushPatternEditorPrivate::slotChangeSaturation(int color)
{
    QColor c = m_ui.saturationColorLine->color();
    if (m_ui.hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), (qreal)color / 255, c.valueF(), c.alphaF());
    else
        c.setGreen(color);
    slotChangeSaturation(c);
}

void QtBrushPatternEditorPrivate::slotChangeValue(const QColor &color)
{
    QBrush brush = m_brush;
    brush.setColor(color);
    q_ptr->setBrush(brush);
}

void QtBrushPatternEditorPrivate::slotChangeValue(int color)
{
    QColor c = m_ui.valueColorLine->color();
    if (m_ui.hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), c.saturationF(), (qreal)color / 255, c.alphaF());
    else
        c.setBlue(color);
    slotChangeValue(c);
}

void QtBrushPatternEditorPrivate::slotChangeAlpha(const QColor &color)
{
    QBrush brush = m_brush;
    brush.setColor(color);
    q_ptr->setBrush(brush);
}

void QtBrushPatternEditorPrivate::slotChangeAlpha(int color)
{
    QColor c = m_ui.alphaColorLine->color();
    if (m_ui.hsvRadioButton->isChecked())
        c.setHsvF(c.hueF(), c.saturationF(), c.valueF(), (qreal)color / 255);
    else
        c.setAlpha(color);
    slotChangeAlpha(c);
}

void QtBrushPatternEditorPrivate::setColorSpinBoxes(const QColor &color)
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
QtBrushPatternEditor::QtBrushPatternEditor(QWidget *parent)
    : QWidget(parent)
{
    d_ptr = new QtBrushPatternEditorPrivate();
    d_ptr->q_ptr = this;
    d_ptr->m_ui.setupUi(this);

    d_ptr->m_ui.hueColorLine->setColorComponent(QtColorLine::Hue);
    d_ptr->m_ui.saturationColorLine->setColorComponent(QtColorLine::Saturation);
    d_ptr->m_ui.valueColorLine->setColorComponent(QtColorLine::Value);
    d_ptr->m_ui.alphaColorLine->setColorComponent(QtColorLine::Alpha);

    QStringList patterns;
    patterns << tr("No Brush") << tr("Solid") << tr("Dense 1") << tr("Dense 2") << tr("Dense 3") << tr("Dense 4")
            << tr("Dense 5") << tr("Dense 6") << tr("Dense 7") << tr("Horizontal") << tr("Vertical")
            << tr("Cross") << tr("Backward Diagonal") << tr("Forward Diagonal") << tr("Crossing Diagonal");
    d_ptr->m_ui.patternComboBox->addItems(patterns);
    d_ptr->m_ui.patternComboBox->setCurrentIndex(1);

    connect(d_ptr->m_ui.patternComboBox, SIGNAL(activated(int)),
                this, SLOT(slotPatternChanged(int)));

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

    connect(d_ptr->m_ui.hsvRadioButton, SIGNAL(clicked()),
                this, SLOT(slotHsvClicked()));
    connect(d_ptr->m_ui.rgbRadioButton, SIGNAL(clicked()),
                this, SLOT(slotRgbClicked()));

    QBrush brush(Qt::white);
    setBrush(brush);
}

QtBrushPatternEditor::~QtBrushPatternEditor()
{
    delete d_ptr;
}

void QtBrushPatternEditor::setBrush(const QBrush &brush)
{
    if (d_ptr->m_brush == brush)
        return;

    if (brush.style() == Qt::LinearGradientPattern ||
            brush.style() == Qt::RadialGradientPattern ||
            brush.style() == Qt::ConicalGradientPattern ||
            brush.style() == Qt::TexturePattern)
        return;

    d_ptr->m_brush = brush;
    d_ptr->m_ui.brushWidget->setBrush(brush);

    d_ptr->m_ui.patternComboBox->setCurrentIndex((int)d_ptr->m_brush.style());
    d_ptr->m_ui.colorButton->setColor(d_ptr->m_brush.color());
    d_ptr->m_ui.hueColorLine->setColor(d_ptr->m_brush.color());
    d_ptr->m_ui.saturationColorLine->setColor(d_ptr->m_brush.color());
    d_ptr->m_ui.valueColorLine->setColor(d_ptr->m_brush.color());
    d_ptr->m_ui.alphaColorLine->setColor(d_ptr->m_brush.color());
    d_ptr->setColorSpinBoxes(d_ptr->m_brush.color());
}

QBrush QtBrushPatternEditor::brush() const
{
    return d_ptr->m_brush;
}

#include "moc_qtbrushpatterneditor.cpp"
