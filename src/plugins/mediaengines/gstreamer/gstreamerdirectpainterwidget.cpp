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

#include <QSize>
#include <QRect>
#include <QPainter>
#include <QPaintEvent>
#include <QtGui/qscreen_qws.h>
#include <QDebug>

#include "gstreamerqtopiavideosink.h"

#include "gstreamerdirectpainterwidget.h"


namespace gstreamer
{


/*!
    \class gstreamer::DirectPainterWidget
    \internal
*/

DirectPainterWidget::DirectPainterWidget(QWidget* parent):
    QDirectPainter(parent, QDirectPainter::NonReserved),
    m_isVisible(false),
    m_painting(false),
    m_black(0)
{
    m_frameBufferImage = QImage(frameBuffer(),
                                screenWidth(),
                                screenHeight(),
                                screenDepth() == 16 ? QImage::Format_RGB16 :
                                QImage::Format_RGB32);
}

DirectPainterWidget::~DirectPainterWidget()
{
    // Don't unref the sink.
}

GstElement* DirectPainterWidget::element()
{
    m_sink = GST_ELEMENT(g_object_new(QtopiaVideoSinkClass::get_type(), NULL));

    if (m_sink != 0) {
        QtopiaVideoSink*  sink = reinterpret_cast<QtopiaVideoSink*>(m_sink);

        sink->widget = this;
    }

    return m_sink;
}

int DirectPainterWidget::displayDepth() const
{
    return qt_screen->depth();
}

void DirectPainterWidget::setVideoSize(int w, int h)
{
    m_videoSize = QSize(w, h);

    regionChanged(allocatedRegion());
}

void DirectPainterWidget::paint(QImage const& frame)
{
    if (!m_isVisible || m_painting)
        return;

    m_painting = true;

    startPainting(); {

        QRegion paintRegion = allocatedRegion();

        if (m_black != 0) {
            qt_screen->solidFill(Qt::black, paintRegion & m_blackRegion);
            --m_black;
        }

#ifndef QTOPIA_NO_MEDIAVIDEOSCALING
        QPainter    p(&m_frameBufferImage);
        p.setClipRegion(paintRegion);
        p.setWindow(m_windowRect);
        p.setViewport(m_viewPort);
        p.setWorldTransform(m_transform);
        p.drawImage(0, 0, frame);
#else
        qt_screen->blit(frame, m_destTopLeft, paintRegion);
#endif
        endPainting();

        qt_screen->setDirty(paintRegion.boundingRect());
    }

    m_savedFrame = frame;

    m_painting = false;
}

// private
void DirectPainterWidget::regionChanged(const QRegion &exposedRegion)
{
    if (exposedRegion.isEmpty())
        m_isVisible = false;
    else {
        if (m_videoSize.isValid()) {
            QRect bounds = geometry();

            if (m_geometry != bounds) {
                m_geometry = bounds;

                m_destRect = QRect(QPoint(0, 0), m_videoSize);

#ifndef QTOPIA_NO_MEDIAVIDEOSCALING
                const double fourfifths = double(4) / 5;
                if ((m_destRect.width() < fourfifths * m_geometry.width() &&
                            m_destRect.height() < fourfifths * m_geometry.height()) ||
                        (m_destRect.width() > m_geometry.width() ||
                         m_destRect.height() > m_geometry.height()))
                {
                    QSize scaled = m_videoSize;
                    scaled.scale(m_geometry.size(), Qt::KeepAspectRatio);
                    m_destRect.setSize(scaled);
                }

                m_destRect.moveCenter(m_geometry.center());

                if (qt_screen->isTransformed()) {
                    m_windowRect = QRect(QPoint(0, 0), qt_screen->mapToDevice(m_videoSize));
                    m_viewPort = qt_screen->mapToDevice(m_destRect, QSize(qt_screen->width(), qt_screen->height()));

                    m_transform.reset();
                    switch (qt_screen->transformOrientation()) {
                        case 1: m_transform.translate(0, m_windowRect.height()); break;
                        case 2: m_transform.translate(m_windowRect.width(), m_windowRect.height()); break;
                        case 3: m_transform.translate(m_windowRect.width(), 0); break;
                    }
                    m_transform.rotate(360 - qt_screen->transformOrientation() * 90);
                }
                else {
                    m_windowRect = QRect(QPoint(0, 0), m_videoSize);
                    m_viewPort = m_destRect;
                }
#else
                m_destRect.moveCenter(m_geometry.center());
                m_destTopLeft = m_destRect.topLeft();
#endif
                m_blackRegion = requestedRegion() ^ QRegion(m_destRect);
            }
            else
                paint(m_savedFrame);

            m_black = m_blackRegion.isEmpty() ? 0 : 2;

            m_isVisible = true;
        }
    }
}

}   // ns gstreamer
