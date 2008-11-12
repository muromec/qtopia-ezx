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

#include "zylonitemousedriverplugin.h"
#include "zylonitemousehandler.h"

#include <qtopiaglobal.h>

ZyloniteMouseDriverPlugin::ZyloniteMouseDriverPlugin( QObject *parent )
    : QMouseDriverPlugin( parent )
{}

ZyloniteMouseDriverPlugin::~ZyloniteMouseDriverPlugin()
{}

QWSMouseHandler* ZyloniteMouseDriverPlugin::create(const QString& driver, const QString&)
{
    qWarning("ZyloniteMouseDriverPlugin:create()");
    return create( driver );
}

QWSMouseHandler* ZyloniteMouseDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "zylonitemousehandler" ) {
        qWarning("Before call ZyloniteMouseHandler()");
        return new ZyloniteMouseHandler();
    }
    return 0;
}

QStringList ZyloniteMouseDriverPlugin::keys() const
{
    return QStringList() << "zylonitemousehandler";
}

QTOPIA_EXPORT_QT_PLUGIN(ZyloniteMouseDriverPlugin)
