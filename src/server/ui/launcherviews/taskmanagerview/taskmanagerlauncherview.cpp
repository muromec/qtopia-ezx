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

#include "taskmanagerlauncherview.h"
#include "taskmanagerentry.h"

#include <QSoftMenuBar>
#include <QtopiaChannel>
#include <QtopiaIpcEnvelope>


/*
    The TaskManagerLauncherView talks over IPC to the
    RunningApplicationsViewItem class in src/server/phone/runningapplicationsviewitem.h.
*/

const QString TaskManagerLauncherView::LAUNCH_MSG_PREFIX = "launch_";

TaskManagerLauncherView::TaskManagerLauncherView(QWidget *parent)
: LauncherView(parent)
{
    setObjectName(QLatin1String("taskmanager"));
    (void)QSoftMenuBar::menuFor(this); // ensure help

    contentSet->setSortOrder(QStringList());

    m_channel = new QtopiaChannel("QPE/RunningAppsLauncherViewService");
    connect(m_channel, SIGNAL(received(QString,QByteArray)),
            SLOT(receivedLauncherServiceMessage(QString,QByteArray)));

    TaskManagerEntry *homeItem =
            new TaskManagerEntry(tr("Home"), "home", this);
    connect(homeItem, SIGNAL(activated()), SLOT(activatedHomeItem()));
    homeItem->show();

    applicationStateChanged();
    QObject::connect(&monitor, SIGNAL(applicationStateChanged(QString,UIApplicationMonitor::ApplicationState)), this, SLOT(applicationStateChanged()));

    QtopiaChannel::send(m_channel->channel(), "runningApplicationsViewLoaded()");
}

TaskManagerLauncherView::~TaskManagerLauncherView()
{
    QList<QString> keys = m_dynamicallyAddedItems.keys();
    for (int i=0; i<keys.size(); i++)
        delete m_dynamicallyAddedItems[keys[i]];
}

void TaskManagerLauncherView::activatedHomeItem()
{
    QtopiaIpcEnvelope env("QPE/System", "showHomeScreen()");
}

// XXX still need to remove Running apps from display when they all exit

void TaskManagerLauncherView::applicationStateChanged()
{
    // TODO: now that the model is smart, redo this so that it doesn't recreate it all from scratch.
    QStringList apps = monitor.runningApplications();

    QContentSetModel csModel(contentSet);
    for (int i=0; i< csModel.rowCount(); i++) {
        QContent c = csModel.content(i);
        if (!m_dynamicallyAddedItems.contains(c.type()) &&
             !apps.contains(c.executableName())) {
            removeItem(c);
        }
    }

    QContentFilter filter(QContent::Application);
    QContentSet set(filter);
    for (int ii=0; ii < apps.count(); ii++) {
        QContent app = set.findExecutable(apps.at(ii));
        if(app.isValid() && !contentSet->contains(app)) {
            addItem(new QContent(app), false);
        }
    }
    //resetSelection();
}

void TaskManagerLauncherView::receivedLauncherServiceMessage(const QString &msg, const QByteArray &args)
{
    if (msg == QLatin1String("addDynamicLauncherItem(int,QString,QString)")) {
        QDataStream ds(args);
        int id;
        QString name;
        QString iconPath;
        ds >> id >> name >> iconPath;
        addDynamicLauncherItem(id, name, iconPath);
    } else if (msg == QLatin1String("removeDynamicLauncherItem(int)")) {
        QDataStream ds(args);
        int id;
        ds >> id;
        removeDynamicLauncherItem(id);
    } else if (msg.startsWith(LAUNCH_MSG_PREFIX)) {
        // expect msg to look like "launch_X()" where X is the item's integer ID
        int launchPrefixLength = LAUNCH_MSG_PREFIX.length();
        QByteArray bytes;
        QDataStream ds(&bytes, QIODevice::WriteOnly);
        int id = msg.mid(launchPrefixLength, msg.length() - launchPrefixLength - 2).toInt();
        ds << id;
        QtopiaChannel::send(m_channel->channel(), "activatedLaunchItem(int)", bytes);
    }
}

/*
    Returns the channel/message string that will be used to send an IPC
    message in order to signal that the item with the given \a itemId should
    now be launched.

    This string should be set as the "type" for the QContent that corresponds
    to the given \a itemId.
*/
QString TaskManagerLauncherView::itemActivationIpcMessage(int itemId)
{
    // make a string like "Ipc/QPE/RunningAppsLauncherViewService/launch_X"
    // where X is the item's integer ID
    return QString("Ipc/%1::%2%3()")
            .arg(m_channel->channel())
            .arg(LAUNCH_MSG_PREFIX)
            .arg(itemId);
}

void TaskManagerLauncherView::addDynamicLauncherItem(int id, const QString &name, const QString &iconPath)
{
    QString ipcMsgString = itemActivationIpcMessage(id);

    // remove item if already present
    if (m_dynamicallyAddedItems.contains(ipcMsgString))
        removeDynamicLauncherItem(id);

    QContent *c = new QContent;
    c->setName(name);
    c->setType(ipcMsgString);
    c->setIcon(iconPath);

    // Use the type string (i.e. the IPC message) as the key instead of the ID
    // so the applicationStateChanged() method in this class can easily tell
    // whether an item a dynamically added launch item, by looking at the
    // content type().
    // (The ID is unique, so the IPC message should also be unique among items.)
    m_dynamicallyAddedItems.insert(ipcMsgString, c);

    addItem(c);
}

void TaskManagerLauncherView::removeDynamicLauncherItem(int id)
{
    QString ipcMsgString = itemActivationIpcMessage(id);
    if (m_dynamicallyAddedItems.contains(ipcMsgString)) {
        QContent *c = m_dynamicallyAddedItems.take(ipcMsgString);
        removeItem(*c);
        delete c;
    }
}


