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

#include "kernelkeymapkbddriverplugin.h"
#include "kernelkeymapkbdhandler.h"

#include <qtopiaglobal.h>
#include <qtopialog.h>

KernelkeymapKbdDriverPlugin::KernelkeymapKbdDriverPlugin( QObject *parent )
    : QKbdDriverPlugin( parent )
{
}

KernelkeymapKbdDriverPlugin::~KernelkeymapKbdDriverPlugin()
{
}

QWSKeyboardHandler* KernelkeymapKbdDriverPlugin::create(const QString& driver, const QString&)
{
    qLog(Input) << "KernelkeymapKbdDriverPlugin:create()";
    return create( driver );
}

QWSKeyboardHandler* KernelkeymapKbdDriverPlugin::create( const QString& driver)
{
    if( driver.toLower() == "kernelkeymapkbdhandler" ) {

        return new KernelkeymapKbdHandler();
    }
    return 0;
}

QStringList KernelkeymapKbdDriverPlugin::keys() const
{
    return QStringList() << "kernelkeymapkbdhandler";
}

QTOPIA_EXPORT_QT_PLUGIN(KernelkeymapKbdDriverPlugin)
