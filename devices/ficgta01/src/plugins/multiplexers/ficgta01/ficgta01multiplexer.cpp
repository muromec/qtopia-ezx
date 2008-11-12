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

#include "ficgta01multiplexer.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <qgsm0710multiplexer.h>
#include <qserialport.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

QTOPIA_EXPORT_PLUGIN( Ficgta01MultiplexerPlugin )

// Define this to disable GSM 07.10 multiplexing, but still do wakeup handling.
#define FICGTA01_NO_MUX


// Size of GSM 07.10 frames to use with the multiplexer.
#ifndef FICGTA01_FRAME_SIZE
#define FICGTA01_FRAME_SIZE       64
#endif

#define N_TIHTC 17

Ficgta01MultiplexerPlugin::Ficgta01MultiplexerPlugin( QObject* parent )
    : QSerialIODeviceMultiplexerPlugin( parent )
{
}

Ficgta01MultiplexerPlugin::~Ficgta01MultiplexerPlugin()
{
}


bool Ficgta01MultiplexerPlugin::detect( QSerialIODevice *device )
{
// The FIC needs a special line discipline set on the device.
    QSerialPort *port = qobject_cast<QSerialPort *>( device );
    if (port) {
        int discipline = N_TIHTC;
        ::ioctl(port->fd(), TIOCSETD, &discipline);
    }
    device->discard();
    int rc;
    struct termios t;
    rc = tcgetattr(port->fd(), &t);
    t.c_cflag |= CRTSCTS;
    rc = tcsetattr(port->fd(), TCSANOW, &t);


    // Issue an innocuous command to wake up the device.
    // It will respond with either "OK" or "AT-Command Interpreter ready".

    // We will do this up to 10 times as the modem is losing the first at
    // commands (due waking up)
    int attempts = 10;
    while (--attempts >= 0 && !QSerialIODeviceMultiplexer::chat( device, "ATZ"));


    // Issue the AT+CMUX command to determine if this device
    // uses GSM 07.10-style multiplexing.
#ifndef FICGTA01_NO_MUX
    return QGsm0710Multiplexer::cmuxChat( device, FICGTA01_FRAME_SIZE, true );
#else
    return true;
#endif
}

QSerialIODeviceMultiplexer *Ficgta01MultiplexerPlugin::create
        ( QSerialIODevice *device )
{
#ifndef FICGTA01_NO_MUX
    return new QGsm0710Multiplexer( device, FICGTA01_FRAME_SIZE, true );
#else
    return new QNullSerialIODeviceMultiplexer( device );
#endif
}
