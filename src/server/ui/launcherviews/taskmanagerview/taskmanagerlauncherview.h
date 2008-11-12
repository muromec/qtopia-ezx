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

#ifndef _TASKMANAGER_LAUNCHERVIEW_H_
#define _TASKMANAGER_LAUNCHERVIEW_H_


#include "launcherview.h"
#include "applicationmonitor.h"

#include <QString>
#include <QByteArray>
#include <QHash>


class QtopiaChannel;
class TaskManagerLauncherView : public LauncherView
{
Q_OBJECT
public:
    TaskManagerLauncherView(QWidget * = 0);
    ~TaskManagerLauncherView();

private slots:
    void applicationStateChanged();
    void receivedLauncherServiceMessage(const QString &msg, const QByteArray &args);
    void activatedHomeItem();

private:
    QString itemActivationIpcMessage(int itemId);
    void addDynamicLauncherItem(int id, const QString &name, const QString &iconPath);
    void removeDynamicLauncherItem(int id);

    static const QString LAUNCH_MSG_PREFIX;
    UIApplicationMonitor monitor;
    QtopiaChannel *m_channel;
    QHash<QString, QContent *> m_dynamicallyAddedItems;
};

#endif // _TASKMANAGER_LAUNCHERVIEW_H_
