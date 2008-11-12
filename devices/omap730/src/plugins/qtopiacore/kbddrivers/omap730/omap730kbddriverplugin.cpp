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

#include "omap730kbddriverplugin.h"
#include "omap730kbdhandler.h"

#include <qtopiaglobal.h>

Omap730KbdDriverPlugin::Omap730KbdDriverPlugin( QObject *parent )
    : QKbdDriverPlugin( parent )
{}

Omap730KbdDriverPlugin::~Omap730KbdDriverPlugin()
{}

QWSKeyboardHandler* Omap730KbdDriverPlugin::create(const QString& driver, const QString&)
{
    qWarning("Omap730KbdDriverPlugin:create()");
    return create( driver );
}

QWSKeyboardHandler* Omap730KbdDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "omap730kbdhandler" ) {
        qWarning("Before call Omap730KbdHandler()");
        return new Omap730KbdHandler();
    }
    return 0;
}

QStringList Omap730KbdDriverPlugin::keys() const
{
    return QStringList() << "omap730kbdhandler";
}

QTOPIA_EXPORT_QT_PLUGIN(Omap730KbdDriverPlugin)
