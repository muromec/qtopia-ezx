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

#include "c3200mousedriverplugin.h"
#include "c3200mousehandler.h"

#include <qtopiaglobal.h>

C3200MouseDriverPlugin::C3200MouseDriverPlugin( QObject *parent )
    : QMouseDriverPlugin( parent )
{}

C3200MouseDriverPlugin::~C3200MouseDriverPlugin()
{}

QWSMouseHandler* C3200MouseDriverPlugin::create(const QString& driver, const QString&)
{
    qWarning("C3200MouseDriverPlugin:create()");
    return create( driver );
}

QWSMouseHandler* C3200MouseDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "c3200mousehandler" ) {
        qWarning("Before call C3200MouseHandler()");
        return new C3200MouseHandler();
    }
    return 0;
}

QStringList C3200MouseDriverPlugin::keys() const
{
    return QStringList() << "c3200mousehandler";
}

QTOPIA_EXPORT_QT_PLUGIN(C3200MouseDriverPlugin)
