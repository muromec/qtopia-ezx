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
TRANSLATOR qdesigner_internal::StyledButton
*/

#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>
#include <QtGui/QColorDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QImageReader>

#include "styledbutton.h"

using namespace qdesigner_internal;

StyledButton::StyledButton (QWidget *parent, ButtonType type)
    : QPushButton(parent), btype(type)
{
    connect(this, SIGNAL(clicked()), this, SLOT(onEditor()));
    mBrush = QBrush(Qt::darkGray);
}

const QBrush &StyledButton::brush()
{
    return mBrush;
}

void StyledButton::setBrush(const QBrush &b)
{
    mBrush = b;

    if (btype == PixmapButton)
        mBrush.setColor(Qt::darkGray);

    update();
}

void StyledButton::setButtonType(ButtonType type)
{
    btype = type;
    update();
}

void StyledButton::paintEvent (QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    QStyleOptionButton opt;
    opt.init(this);
    QRect contentRect = style()->subElementRect(QStyle::SE_PushButtonContents, &opt, this);

    QPainter paint(this);

    if (btype == ColorButton)
        paint.setBrush(QBrush(mBrush.color()));
    else
        paint.setBrush(mBrush);

    paint.drawRect(contentRect.left()+2, contentRect.top()+2, contentRect.width()-5, contentRect.height()-5);
}

QString StyledButton::buildImageFormatList() const
{
    QString filter;

#if 0 // ### port me
    QString all = tr("All Pixmaps (");
    const QList<QByteArray> supportedImageFormats = QImageReader::supportedImageFormats();
    const QString jpeg = QLatin1String("JPEG");
    for (int i=0; i< supportedImageFormats.count(); ++i) {
        const QString outputFormat = QString::fromUtf8(supportedImageFormats.at(i));
        QString outputExtension = QLatin1String("*.");
        if (outputFormat != jpeg)
            outputExtension = outputFormat.toLower();
        else
            outputExtension = QLatin1String("jpg;*.jpeg");

        filter += tr("%1-Pixmaps (%2)\n").arg(outputFormat).arg(outputExtension);
        all += QLatin1String("*.");
        all += outputExtension;
        all += QLatin1Char(';');
    }
    all += QLatin1String(")\n");
    filter.prepend(all);
#endif

    filter += tr("All Files (*.*)");

    return filter;
}

bool StyledButton::openPixmap()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), QString(), buildImageFormatList());

    if (!fileName.isEmpty()) {
        pixFile = fileName;
        return true;
    }

    return false;
}

QString StyledButton::pixmapFileName() const
{
    return pixFile;
}

void StyledButton::onEditor()
{
    if (btype == ColorButton) {
        QColor c = QColorDialog::getColor(mBrush.color(), this);
        if (c.isValid()) {
            mBrush.setColor(c);
            emit changed();
        }
    }
    else if(openPixmap()) {
        emit changed();
    }
}
