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

#include "videocaptureview.h"
#include "videocapturedevice.h"
#include "videocapturedevicefactory.h"
#include <math.h>

#include <qimage.h>
#include <qpainter.h>
#include <qevent.h>

#include <QDebug>

VideoCaptureView::VideoCaptureView(QWidget *parent, Qt::WFlags fl):
    QWidget(parent, fl),
    m_cleared(false),
    m_tidUpdate(0)
{
    m_capture = camera::VideoCaptureDeviceFactory::createVideoCaptureDevice();

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Optimize paint event
    setAttribute(Qt::WA_NoSystemBackground);

    QPalette    pal(palette());
    pal.setBrush(QPalette::Window, Qt::black);
    setPalette(pal);

    setLive();


    m_doZoom = false;
    m_zoomlevel = 0;
    m_zoomfactor = 1.0;
    m_minZoom = 0;
    m_maxZoom = 2;

    m_force = false;

}

VideoCaptureView::~VideoCaptureView()
{
    delete m_capture;
}

int VideoCaptureView::minZoom() const
{
    return m_minZoom;
}
int VideoCaptureView::maxZoom() const
{
    return m_maxZoom;
}

void VideoCaptureView::setLive(int period)
{
    if (m_tidUpdate)
        killTimer(m_tidUpdate);
    if (period == 0) {
        m_tidUpdate = startTimer(m_capture->minimumFramePeriod());
    }
    else if ( period > 0 ) {
        m_tidUpdate = startTimer(period);
    }
    else {
        m_tidUpdate = 0;
     }
}

void VideoCaptureView::setStill(const QImage& i)
{
    setLive(-1);
    m_image = i;
    repaint();
}

QList<QSize> VideoCaptureView::photoSizes() const
{
    return m_capture->photoSizes();
}

QList<QSize> VideoCaptureView::videoSizes() const
{
    return m_capture->videoSizes();
}

QSize VideoCaptureView::recommendedPhotoSize() const
{
    return m_capture->recommendedPhotoSize();
}

QSize VideoCaptureView::recommendedVideoSize() const
{
    return m_capture->recommendedVideoSize();
}

QSize VideoCaptureView::recommendedPreviewSize() const
{
    return m_capture->recommendedPreviewSize();
}

QSize VideoCaptureView::captureSize() const
{
    return m_capture->captureSize();
}

void VideoCaptureView::setCaptureSize( QSize size )
{
    m_capture->setCaptureSize(size);
}

uint VideoCaptureView::refocusDelay() const
{
    return m_capture->refocusDelay();
}

bool VideoCaptureView::available() const
{
    return m_capture->hasCamera();
}

void VideoCaptureView::moveEvent(QMoveEvent*)
{
    m_cleared = false;
}

void VideoCaptureView::resizeEvent(QResizeEvent*)
{
    m_cleared = false;
}

void VideoCaptureView::paintEvent(QPaintEvent* paintEvent)
{
    QPainter    painter(this);

    if (!m_cleared)
    {
        QPoint      brushOrigin = painter.brushOrigin();

        // Paint window background
        painter.setBrushOrigin(-mapToGlobal(QPoint(0, 0)));
        painter.fillRect(paintEvent->rect(), window()->palette().brush(QPalette::Window));

        // Reset origin
        painter.setBrushOrigin(brushOrigin);

        m_cleared = true;
    }

    if (m_tidUpdate && !available()) {

        painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, tr("No Camera"));

        killTimer(m_tidUpdate);
    }
    else
    {

        if (m_tidUpdate > 0 || m_force)
        {
            m_capture->getCameraImage(m_image);
            doZoom();
        }
        int w = m_image.width();
        int h = m_image.height();

        if (!(w == 0 || h == 0))
        {
            if (width() * w > height() * h) {
                w = w * height() / h;
                h = height();
            }
            else {
                h = h * width() / w;
                w = width();
            }

            painter.drawImage(QRect((width() - w) / 2, (height() - h) / 2, w, h),
                                     m_image,
                                     QRect(0, 0, m_image.width(), m_image.height()));
        }

     }
}

void VideoCaptureView::timerEvent(QTimerEvent*)
{
    m_cleared = true;
    repaint();
    m_cleared = false;
}


void VideoCaptureView::zoomIn()
{
    m_zoomlevel = (m_zoomlevel+1<=m_maxZoom)?++m_zoomlevel:m_zoomlevel;
    if(m_zoomlevel != 0)
        m_doZoom = true;
    else
        m_doZoom = false;
    m_zoomfactor = 1.0/pow(2,m_zoomlevel);
}

void VideoCaptureView::zoomOut()
{
    m_zoomlevel = (m_zoomlevel-1>=m_minZoom)?--m_zoomlevel:m_zoomlevel;
    if(m_zoomlevel != 0)
        m_doZoom = true;
    else
        m_doZoom = false;
    m_zoomfactor = 1.0 / pow(2,m_zoomlevel);
}

void VideoCaptureView::doZoom()
{
    if(!m_doZoom)
        return;
    float dw = 0.0,dh = 0.0;
    QRect r = m_image.rect();
    int w = (int)(r.width() * m_zoomfactor);
    int h = (int)(r.height() * m_zoomfactor);
    for(int i = m_zoomlevel; i > 0; i--) {
        dw += w/2*i;
        dh += h/2*i;
    }
    QRect d ((int)dw,(int)dh , w, h);
    QImage img2 = m_image.copy(d);
    m_image = img2;
}



