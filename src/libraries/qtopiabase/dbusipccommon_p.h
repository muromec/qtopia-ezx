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

#ifndef __DBUSIPCCOMMON_P_H__
#define __DBUSIPCCOMMON_P_H__

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QString>

static const QString dbusPathBase("/com/trolltech/qtopia/");
static const QString dbusInterface("com.trolltech.qtopia");

void convert_dbus_path_to_qcop_channel(const QString &path,
                                       QString &channel);

void convert_qcop_channel_to_dbus_path(const QString &channel,
                                       QString &dbusPath);

void convert_dbus_to_qcop_message_name(const QString &dbusMsg,
                                       QString &msg);

void convert_qcop_message_name_to_dbus(const QString &msg,
                                       QString &dbusMsg);

#endif
