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

#ifndef __GSTREAMER_DIRECTPAINTERSINK_H
#define __GSTREAMER_DIRECTPAINTERSINK_H

#include <QDirectPainter>

#include "gstreamersinkwidget.h"


namespace gstreamer
{

class DirectPainterWidget :
    public QDirectPainter,
    public SinkWidget
{
    Q_OBJECT

public:
    DirectPainterWidget(QWidget* parent = 0);
    ~DirectPainterWidget();

    // Sink widget
    GstElement* element();

    int displayDepth() const;
    void setVideoSize(int width, int height);
    void paint(QImage const& frame);

private:
    void regionChanged(const QRegion &exposedRegion);

    bool        m_isVisible;
    bool        m_painting;
    int         m_black;
    QSize       m_videoSize;
    QRect       m_geometry;
    QRect       m_destRect;
    QRegion     m_blackRegion;
    GstElement* m_sink;
    QImage      m_savedFrame;
#ifndef QTOPIA_NO_MEDIAVIDEOSCALING
    QImage      m_frameBufferImage;
    QRect       m_windowRect;
    QRect       m_viewPort;
    QTransform  m_transform;
#else
    QPoint      m_destTopLeft;
#endif
};

}   // ns gstreamer


#endif  // __GSTREAMER_DIRECTPAINTERSINK_H
