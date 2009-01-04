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

#ifndef VIDEOVIEW_H
#define VIDEOVIEW_H

#include <qwidget.h>
#include <qimage.h>
#include <qlist.h>

#include <QSocketNotifier>


namespace camera
{
class VideoCaptureDevice;
}

class QSlider;
class VideoCaptureView : public QWidget
{
    Q_OBJECT

public:
    VideoCaptureView(QWidget *parent = 0, Qt::WFlags fl = 0);
    ~VideoCaptureView();

    bool available() const;

    QImage image() ;
    void setLive(int period=0);
    void setStill(const QImage&);

    QList<QSize> photoSizes() const;
    QList<QSize> videoSizes() const;

    QSize recommendedPhotoSize() const;
    QSize recommendedVideoSize() const;
    QSize recommendedPreviewSize() const;

    QSize captureSize() const;
    void setCaptureSize( QSize size );

    uint refocusDelay() const;

    void zoomIn();
    void zoomOut();
    void doZoom();

    int maxZoom() const;
    int minZoom() const;

protected:
    void moveEvent(QMoveEvent* moveEvent);
    void resizeEvent(QResizeEvent* resizeEvent);
    void paintEvent(QPaintEvent* paintEvent);
    void timerEvent(QTimerEvent* timerEvent);
private:
    bool                m_cleared;
    int                 m_tidUpdate;
    QImage              m_image;
    camera::VideoCaptureDevice  *m_capture;

    // Zoom 
    bool m_doZoom;
    int m_maxZoom;
    int m_minZoom;
    int m_zoomlevel;
    float m_zoomfactor;

    bool m_force;

    QSlider *m_zoomWidget;
    void showZoom();
    void hideZoom();
};

#endif

