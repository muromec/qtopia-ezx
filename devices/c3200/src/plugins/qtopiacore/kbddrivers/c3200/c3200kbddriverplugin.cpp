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

#include "c3200kbddriverplugin.h"
#include "c3200kbdhandler.h"

#include <qtopiaglobal.h>

C3200KbdDriverPlugin::C3200KbdDriverPlugin( QObject *parent )
    : QKbdDriverPlugin( parent )
{}

C3200KbdDriverPlugin::~C3200KbdDriverPlugin()
{}

QWSKeyboardHandler* C3200KbdDriverPlugin::create(const QString& driver, const QString&)
{
    qWarning("C3200KbdDriverPlugin:create()");
    return create( driver );
}

QWSKeyboardHandler* C3200KbdDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "c3200kbdhandler" ) {
        qWarning("Before call C3200KbdHandler()");
        return new C3200KbdHandler();
    }
    return 0;
}

QStringList C3200KbdDriverPlugin::keys() const
{
    return QStringList() << "c3200kbdhandler";
}

QTOPIA_EXPORT_QT_PLUGIN(C3200KbdDriverPlugin)
