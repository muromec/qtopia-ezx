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
#ifndef QUICKLAUNCH_H
#define QUICKLAUNCH_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QPointer>
#include <QWidget>
#include <QMap>
#include <QList>

class QtopiaChannel;
#ifndef SINGLE_EXEC
class QPluginManager;
class QApplicationFactoryInterface;
#endif
class QtopiaApplication;
class QEventLoop;
class QSocketNotifier;
class QtopiaIpcAdaptor;

// ============================================================================
//
//  QuickLauncher
//
// ============================================================================

class QuickLauncher : public QObject
{
    Q_OBJECT
public:
    QuickLauncher();
    ~QuickLauncher();

    static void exec( int argc, char **argv );

public slots:
    void childDied();
    void fillPool();
    void listenToChannel();
    void disconnectChannel();

    void execute( const QStringList& arguments );
    void execute( const QString& argument );
    void run( const QStringList& arguments );
    void quit();

private:
    void doQuickLaunch( const QStringList& argList );
    void runApplication( const QStringList& argList );
    void reinit();

private:
    QtopiaIpcAdaptor* channel;
    QMap<int, QString> apps;
    QList<int> pool;

public:
#ifndef SINGLE_EXEC
    static QPluginManager *loader;
    static QObject *appInstance;
    static QApplicationFactoryInterface *appIface;
#endif

    static QtopiaApplication *app;
    static QPointer<QWidget> mainWindow;
    static bool validExitLoop;
    static QEventLoop *eventLoop;
    static QSocketNotifier* notifier;
    static int ql_deadChild_pipe[2];

    static char **argv0;
    static int argv_lth;
};

#endif
