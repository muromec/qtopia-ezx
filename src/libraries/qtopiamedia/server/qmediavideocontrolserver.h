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

#ifndef __QTOPIA_MEDIA_VIDEOCONTROLSERVER_H
#define __QTOPIA_MEDIA_VIDEOCONTROLSERVER_H

#include <QWSEmbedWidget>

#include "qmediahandle_p.h"
#include "qmediaabstractcontrolserver.h"

#include <qtopiaglobal.h>


class QTOPIAMEDIA_EXPORT QMediaVideoControlServer :
    public QMediaAbstractControlServer
{
    Q_OBJECT

public:
    QMediaVideoControlServer(QMediaHandle const& handle,
                             QWidget* target = 0,
                             QObject* parent = 0);
    ~QMediaVideoControlServer();

    void setRenderTarget(QWidget* target);
    void setRenderTarget(int wid);

    void unsetRenderTarget();

signals:
    void videoTargetAvailable();
    void videoTargetRemoved();
};


#endif  // __QTOPIA_MEDIA_VIDEOCONTROLSERVER_H
