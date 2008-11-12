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

#ifndef DEVICE_UPDATER_LOCALSOCKETLISTENER_H
#define DEVICE_UPDATER_LOCALSOCKETLISTENER_H

#define MAX_CONNECTIONS 5

#include "localsocket.h"

#include <QObject>

class QSocketNotifier;

class LocalSocketListener : public LocalSocket
{
    Q_OBJECT
public:
    LocalSocketListener();
    virtual ~LocalSocketListener();
    bool listen();
    void setupNotifier();
signals:
    void commandReceived(const QString &);
private slots:
    void receiveMessage();
private:
    QSocketNotifier *mNotifier;
};

#endif
