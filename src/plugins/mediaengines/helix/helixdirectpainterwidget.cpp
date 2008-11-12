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

#include <QtGui/qscreen_qws.h>
#include <QtGui/qwsevent_qws.h>
#include <QtGui/qwsdisplay_qws.h>
#include <QtGui/private/qwsdisplay_qws_p.h>
#include <QtGui/private/qwindowsurface_qws_p.h>

#include <custom.h>

#include "helixdirectpainterwidget.h"



DirectPainterVideoWidget::DirectPainterVideoWidget(GenericVideoSurface* surface, QWidget* parent):
    QDirectPainter(parent, QDirectPainter::NonReserved),
    m_surface(surface),
    m_isVisible(false),
    m_painting(false),
    m_black(0)
{
    m_frameBufferImage = QImage(frameBuffer(),
                                screenWidth(),
                                screenHeight(),
                                screenDepth() == 16 ? QImage::Format_RGB16 :
                                QImage::Format_RGB32);

    HX_ADDREF(m_surface);
    m_surface->setPaintObserver(this);
}

DirectPainterVideoWidget::~DirectPainterVideoWidget()
{
    m_surface->setPaintObserver(0);
    HX_RELEASE(m_surface);
}

void DirectPainterVideoWidget::setVideoSize(QSize const& size)
{
    m_videoSize = size;

    regionChanged(allocatedRegion());
}

void DirectPainterVideoWidget::paint(QImage const& frame)
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
        qt_screen->blit(buffer, m_destTopLeft, paintRegion);
#endif
        qt_screen->setDirty(paintRegion.boundingRect());

        endPainting();
    }

    m_painting = false;
}

int DirectPainterVideoWidget::isSupported()
{
    return screenDepth() == 16 || screenDepth() == 32;
}

// private
void DirectPainterVideoWidget::regionChanged(const QRegion &exposedRegion)
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
                if ((m_destRect.width() < fourfifths * bounds.width() &&
                     m_destRect.height() < fourfifths * bounds.height()) ||
                    (m_destRect.width() > m_geometry.width() ||
                     m_destRect.height() > m_geometry.height()))
                {
                    QSize scaled = m_destRect.size();
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
                m_destRect.moveCenter(bounds.center());
                m_destTopLeft = m_destRect.topLeft();
#endif
                m_blackRegion = requestedRegion() ^ QRegion(m_destRect);
            }
            else
                paint(m_surface->buffer());        // force repaint in case paused

            // redraw black region (int(2) = weight against the race)
            m_black = m_blackRegion.isEmpty() ? 0 : 2;

            m_isVisible = true;
        }
    }
}

