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

#ifndef _VIDEORINGTONE_H_
#define _VIDEORINGTONE_H_

#include <QObject>


class QWidget;

class VideoRingtonePrivate;

class VideoRingtone : public QObject
{
    Q_OBJECT

public:
    ~VideoRingtone();

    void playVideo(const QString& fileName);
    void stopVideo();
    QWidget* videoWidget();

    static VideoRingtone* instance();

signals:
    void videoRingtoneFailed();
    void videoWidgetReady();
    void videoRingtoneStopped();

private:
    VideoRingtone();

    VideoRingtonePrivate *d;
};

#endif // _VIDEORINGTONE_H_
