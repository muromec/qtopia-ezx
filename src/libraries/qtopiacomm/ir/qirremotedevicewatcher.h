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

#ifndef __QIRREMOTEDEVICEWATCHER_H__
#define __QIRREMOTEDEVICEWATCHER_H__

#include <QObject>
#include <qirnamespace.h>
#include <qirglobal.h>

class QIrRemoteDeviceWatcher_Private;

class QIR_EXPORT QIrRemoteDeviceWatcher : public QObject
{
    Q_OBJECT
    friend class QIrRemoteDeviceWatcher_Private;

public:
    explicit QIrRemoteDeviceWatcher(QObject *parent = 0);
    ~QIrRemoteDeviceWatcher();

    bool watch(int ms, QIr::DeviceClasses = QIr::All);

signals:
    void deviceFound();

private:
    Q_DISABLE_COPY(QIrRemoteDeviceWatcher)
    QIrRemoteDeviceWatcher_Private *m_data;
};

#endif
