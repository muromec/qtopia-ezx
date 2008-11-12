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

#include "nokiakbddriverplugin.h"
#include "nokiakbdhandler.h"

#include <qtopiaglobal.h>

NokiaKbdDriverPlugin::NokiaKbdDriverPlugin( QObject *parent )
    : QKbdDriverPlugin( parent )
{
}

NokiaKbdDriverPlugin::~NokiaKbdDriverPlugin()
{
}

QWSKeyboardHandler* NokiaKbdDriverPlugin::create(const QString& driver, const QString&)
{
    qWarning("NokiaKbdDriverPlugin:create()");
    return create( driver );
}

QWSKeyboardHandler* NokiaKbdDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "nokiakbdhandler" ) {
        qWarning("Before call NokiaKbdHandler()");
        return new NokiaKbdHandler();
    }
    return 0;
}

QStringList NokiaKbdDriverPlugin::keys() const
{
    return QStringList() << "nokiakbdhandler";
}

QTOPIA_EXPORT_QT_PLUGIN(NokiaKbdDriverPlugin)
