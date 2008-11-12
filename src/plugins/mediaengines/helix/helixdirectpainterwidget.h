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

#ifndef __QTOPIA_HELIXDIRECTPAINTERWIDGET_H
#define __QTOPIA_HELIXDIRECTPAINTERWIDGET_H


#include <QDirectPainter>

#include "helixvideosurface.h"



class DirectPainterVideoWidget :
    public QDirectPainter,
    public PaintObserver
{
public:
    DirectPainterVideoWidget(GenericVideoSurface* surface,
                             QWidget* parent = 0);
    ~DirectPainterVideoWidget();

    // Observer
    void setVideoSize(QSize const& size);
    void paint(QImage const& frame);

    static int isSupported();

private:
    void regionChanged(const QRegion &exposedRegion);

    GenericVideoSurface *m_surface;

    bool        m_isVisible;
    bool        m_painting;
    int         m_black;
    QSize       m_videoSize;
    QRect       m_geometry;
    QRect       m_destRect;
    QRegion     m_blackRegion;
#ifndef QTOPIA_NO_MEDIAVIDEOSCALING
    QImage      m_frameBufferImage;
    QRect       m_windowRect;
    QRect       m_viewPort;
    QTransform  m_transform;
#else
    QPoint      m_destTopLeft;
#endif
};

#endif  // __QTOPIA_HELIXDIRECTPAINTERWIDGET_H

