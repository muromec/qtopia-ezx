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

#include <qtopiaserverapplication.h>
#include <qtopiaipcenvelope.h>
#include <QTimer>

#include "dummyvolumeservice.h"


DummyVolumeService::DummyVolumeService():
    QtopiaIpcAdaptor("QPE/AudioVolumeManager/DummyVolumeService")
{
    QTimer::singleShot(0, this, SLOT(registerService()));
}

DummyVolumeService::~DummyVolumeService()
{
}

void DummyVolumeService::registerService()
{
    QtopiaIpcEnvelope   e("QPE/AudioVolumeManager", "registerHandler(QString,QString)");

    e << QString("Headset") << QString("QPE/AudioVolumeManager/DummyVolumeService");

    QTimer::singleShot(0, this, SLOT(setCallDomain()));
}

void DummyVolumeService::setCallDomain()
{
    QtopiaIpcEnvelope   e("QPE/AudioVolumeManager", "setActiveDomain(QString)");

    e << QString("Headset");
}


QTOPIA_TASK(DummyVolumeService, DummyVolumeService);

