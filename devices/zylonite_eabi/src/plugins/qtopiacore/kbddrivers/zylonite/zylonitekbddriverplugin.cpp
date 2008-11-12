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

#include "zylonitekbddriverplugin.h"
#include "zylonitekbdhandler.h"

#include <qtopiaglobal.h>

ZyloniteKbdDriverPlugin::ZyloniteKbdDriverPlugin( QObject *parent )
    : QKbdDriverPlugin( parent )
{}

ZyloniteKbdDriverPlugin::~ZyloniteKbdDriverPlugin()
{}

QWSKeyboardHandler* ZyloniteKbdDriverPlugin::create(const QString& driver, const QString&)
{
    qWarning("ZyloniteKbdDriverPlugin:create()");
    return create( driver );
}

QWSKeyboardHandler* ZyloniteKbdDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "zylonitekbdhandler" ) {
        qWarning("Before call ZyloniteKbdHandler()");
        return new ZyloniteKbdHandler();
    }
    return 0;
}

QStringList ZyloniteKbdDriverPlugin::keys() const
{
    return QStringList() << "zylonitekbdhandler";
}

QTOPIA_EXPORT_QT_PLUGIN(ZyloniteKbdDriverPlugin)
