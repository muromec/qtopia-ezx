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
#include "ficgta01plugin.h"
#include "vendor_ficgta01_p.h"

QTOPIA_EXPORT_PLUGIN( Ficgta01PluginImpl )

Ficgta01PluginImpl::Ficgta01PluginImpl()
{
}


Ficgta01PluginImpl::~Ficgta01PluginImpl()
{
}


bool Ficgta01PluginImpl::supports( const QString& manufacturer )
{
    return  manufacturer.contains( "Openmoko", Qt::CaseInsensitive );
    // "Neo1973 GTA01 Embedded GSM Modem"
    //
}

QModemService *Ficgta01PluginImpl::create
    ( const QString& service, QSerialIODeviceMultiplexer *mux, QObject *parent )
{
    return new Ficgta01ModemService( service, mux, parent );
}
