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
#include "omap730plugin.h"
#include "vendor_omap730_p.h"

QTOPIA_EXPORT_PLUGIN( Omap730PluginImpl )

Omap730PluginImpl::Omap730PluginImpl()
{
}


Omap730PluginImpl::~Omap730PluginImpl()
{
}


bool Omap730PluginImpl::supports( const QString& manufacturer )
{
    return manufacturer.contains( "Texas Instruments" );
}

QModemService *Omap730PluginImpl::create
    ( const QString& service, QSerialIODeviceMultiplexer *mux, QObject *parent )
{
    return new Omap730ModemService( service, mux, parent );
}
