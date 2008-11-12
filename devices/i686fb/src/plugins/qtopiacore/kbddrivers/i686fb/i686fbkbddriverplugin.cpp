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

#include "i686fbkbddriverplugin.h"
#include "i686fbkbdhandler.h"

#include <qtopiaglobal.h>

I686fbKbdDriverPlugin::I686fbKbdDriverPlugin( QObject *parent )
    : QKbdDriverPlugin( parent )
{}

I686fbKbdDriverPlugin::~I686fbKbdDriverPlugin()
{}

QWSKeyboardHandler* I686fbKbdDriverPlugin::create(const QString& driver, const QString&)
{
    qWarning("I686fbKbdDriverPlugin:create()");
    return create( driver );
}

QWSKeyboardHandler* I686fbKbdDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "i686fbkbdhandler" ) {
        qWarning("Before call I686fbKbdHandler()");
        return new I686fbKbdHandler();
    }
    return 0;
}

QStringList I686fbKbdDriverPlugin::keys() const
{
    return QStringList() << "i686fbkbdhandler";
}

QTOPIA_EXPORT_QT_PLUGIN(I686fbKbdDriverPlugin)
