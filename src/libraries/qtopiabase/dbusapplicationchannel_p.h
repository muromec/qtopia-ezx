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
#ifndef __DBUSAPPLICATIONCHANNEL_H__
#define __DBUSAPPLICATIONCHANNEL_H__

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

#if defined(QTOPIA_DBUS_IPC)

#include <QString>
#include <qtopiaglobal.h>

class QDBusMessage;

class QTOPIABASE_EXPORT DBUSQtopiaApplicationChannel : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.qtopia")

public:
    DBUSQtopiaApplicationChannel(const QString &appName, QObject *parent = 0);
    ~DBUSQtopiaApplicationChannel();

    const QString &appName() const { return m_appName; }

public slots:
    Q_SCRIPTABLE void appMessage(const QString &msg, const QByteArray &data);

signals:
    void received(const QString &msg, const QByteArray &data);

private:
    QString m_serviceName;
    QString m_appName;
};

#endif

#endif // __DBUSAPPLICATIONCHANNEL_H__
