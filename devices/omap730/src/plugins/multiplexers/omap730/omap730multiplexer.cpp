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

#include "omap730multiplexer.h"
#include <qmultiportmultiplexer.h>
#include <qserialport.h>
#include <qtopiaglobal.h>

Omap730MultiplexerPlugin::Omap730MultiplexerPlugin( QObject* parent )
    : QSerialIODeviceMultiplexerPlugin( parent )
{
}

Omap730MultiplexerPlugin::~Omap730MultiplexerPlugin()
{
}

bool Omap730MultiplexerPlugin::detect( QSerialIODevice * )
{
    // If this plugin is called, then it is the one we want.
    return true;
}

QSerialIODeviceMultiplexer *Omap730MultiplexerPlugin::create
	( QSerialIODevice *device )
{
    // The primary AT command device, /dev/csmi/5, is configured
    // in the custom.h file as QTOPIA_PHONE_DEVICE and then passed
    // down to us in the "device" parameter.
    QMultiPortMultiplexer *mux = new QMultiPortMultiplexer( device );

    // The secondary channel is the same as the primary channel.
    mux->addChannel( "secondary", device );

    // Add the data channel.
    QSerialPort *data = QSerialPort::create( "/dev/csmi/6" );
    mux->addChannel( "data", data );

    // Add the data setup channel.  Pass "device" as the second argument
    // to use the primary AT command device for data setup.
    mux->addChannel( "datasetup", data );
    return mux;
}

QTOPIA_EXPORT_PLUGIN( Omap730MultiplexerPlugin )
