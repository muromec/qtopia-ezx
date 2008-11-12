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

#ifndef PHONESIMSERVER_H
#define PHONESIMSERVER_H

#include <qtcpserver.h>
#include <qtcpsocket.h>
#include <qpointer.h>

#include "phonesim.h"

class PhoneTestServer;
class HardwareManipulatorFactory;

class PhoneSimServer : public QTcpServer
{
public:
    PhoneSimServer(const QString &, quint16 port, QObject *parent = 0);
    ~PhoneSimServer();

    void setHardwareManipulator(HardwareManipulatorFactory *f);

    SimRules *rules() const { return currentRules; }

protected:
    void incomingConnection(int s);

private:
    QString filename;

    HardwareManipulatorFactory *fact;
    QPointer<SimRules> currentRules;
};

#endif

