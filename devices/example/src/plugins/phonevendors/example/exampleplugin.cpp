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
#include "exampleplugin.h"
#include "vendor_example_p.h"


QTOPIA_EXPORT_PLUGIN( ExamplePluginImpl )

ExamplePluginImpl::ExamplePluginImpl()
{
}


ExamplePluginImpl::~ExamplePluginImpl()
{
}


bool ExamplePluginImpl::supports( const QString& manufacturer )
{
    return manufacturer.contains( "Example" ) ||
           manufacturer.contains( "TROLLTECH" );
}

QModemService *ExamplePluginImpl::create
    ( const QString& service, QSerialIODeviceMultiplexer *mux, QObject *parent )
{
    return new ExampleModemService( service, mux, parent );
}
