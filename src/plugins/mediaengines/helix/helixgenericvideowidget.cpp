/****************************************************************************
**
** This file is part of the Qtopia Opensource Edition Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** versions 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#include "helixgenericvideowidget.h"


GenericVideoWidget::GenericVideoWidget(GenericVideoSurface* surface, QWidget* parent):
    QWidget(parent),
    m_surface(surface)
{
    HX_ADDREF( m_surface );

    m_surface->setPaintObserver(this);

    // Optimize paint event
    setAttribute(Qt::WA_NoSystemBackground);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);

    QPalette pal(palette());
    pal.setBrush(QPalette::Window, Qt::black);

    setPalette(pal);
}

GenericVideoWidget::~GenericVideoWidget()
{
    m_surface->setPaintObserver(0);
    HX_RELEASE(m_surface);
}

void GenericVideoWidget::setVideoSize(QSize const& size)
{
    Q_UNUSED(size);
}

void GenericVideoWidget::paint(QImage const& frame)
{
    QWidget::update();
}

void GenericVideoWidget::paintEvent( QPaintEvent* )
{
    QPainter        painter(this);

    painter.setCompositionMode(QPainter::CompositionMode_Source);

    QImage const& buffer = m_surface->buffer();

    if (!buffer.isNull())
    {
        QSize scaled = buffer.size();

        scaled.scale(width(), height(), Qt::KeepAspectRatio);

        painter.drawImage(QRect(QPoint((width() - scaled.width()) / 2, (height() - scaled.height()) / 2 ), scaled),
                          buffer,
                          buffer.rect());
    }
}


