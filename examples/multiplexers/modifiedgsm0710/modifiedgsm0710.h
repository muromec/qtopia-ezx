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

#ifndef MODIFEDGSM0710_H
#define MODIFEDGSM0710_H

#include <qserialiodevicemultiplexerplugin.h>

class ModifiedGsm0710MultiplexerPlugin : public QSerialIODeviceMultiplexerPlugin
{
    Q_OBJECT
public:
    ModifiedGsm0710MultiplexerPlugin( QObject *parent = 0 )
	: QSerialIODeviceMultiplexerPlugin( parent ) {}

    bool detect( QSerialIODevice *device );
    QSerialIODeviceMultiplexer *create( QSerialIODevice *device );
};

#endif // MODIFIEDGSM0710_H
