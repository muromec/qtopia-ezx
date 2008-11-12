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

#include "ficgta01kbddriverplugin.h"
#include "ficgta01kbdhandler.h"

#include <qtopiaglobal.h>

Ficgta01KbdDriverPlugin::Ficgta01KbdDriverPlugin( QObject *parent )
    : QKbdDriverPlugin( parent )
{}

Ficgta01KbdDriverPlugin::~Ficgta01KbdDriverPlugin()
{}

QWSKeyboardHandler* Ficgta01KbdDriverPlugin::create(const QString& driver, const QString&)
{
    qWarning("Ficgta01KbdDriverPlugin:create()");
    return create( driver );
}

QWSKeyboardHandler* Ficgta01KbdDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "ficgta01kbdhandler" ) {
        qWarning("Before call Ficgta01KbdHandler()");
        return new Ficgta01KbdHandler();
    }
    return 0;
}

QStringList Ficgta01KbdDriverPlugin::keys() const
{
    return QStringList() << "ficgta01kbdhandler";
}

QTOPIA_EXPORT_QT_PLUGIN(Ficgta01KbdDriverPlugin)
