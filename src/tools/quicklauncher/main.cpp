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
#ifdef QUICKLAUNCHER_FORKED
#include "quicklaunchforked.h"
#else
#include "quicklaunch.h"
#endif

#include <qpainter.h>
#include <qtimer.h>
#include <qpointer.h>
#include <qtopiachannel.h>
#include <QIcon>
#include <qtimezone.h>
#include <qtopiaapplication.h>
#include <qpluginmanager.h>
#include <qapplicationplugin.h>
#include <QSocketNotifier>
#include <qtopialog.h>
#include <QImageReader>
#include <QtopiaSql>
#include <qtopia/qsoftmenubar.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef SINGLE_EXEC
#define MAIN_FUNC main_quicklaunch
#else
#define MAIN_FUNC main
#ifndef QT_NO_SXE
QSXE_APP_KEY
#endif
#endif

extern char **environ;

int MAIN_FUNC( int argc, char** argv )
{
#ifdef QTOPIA_SETPROC_ARGV0
    // Setup to change proc title.  Must be done before anything else
    // could possibly change argv or envp or argv_lth will be very wrong.
    int i;
    char **envp = environ;
    // Move the environment so we can reuse the memory.
    // (Code borrowed from sendmail.)
    for (i = 0; envp[i] != NULL; i++)
        continue;
    environ = (char **) malloc(sizeof(char *) * (i + 1));
    if (environ == NULL)
        return -1;
    for (i = 0; envp[i] != NULL; i++)
        if ((environ[i] = strdup(envp[i])) == NULL)
            return -1;
    environ[i] = NULL;

    QuickLauncher::argv0 = argv;
    if (i > 0)
        QuickLauncher::argv_lth = envp[i-1] + strlen(envp[i-1]) - QuickLauncher::argv0[0];
    else
        QuickLauncher::argv_lth = QuickLauncher::argv0[argc-1] + strlen(QuickLauncher::argv0[argc-1]) - QuickLauncher::argv0[0];
#endif

#ifndef QT_NO_SXE

    // This is just the key for the quicklauncher, launched processes get
    // their own key
    QTransportAuth::getInstance()->setProcessKey( _key, "quicklauncher" );
#endif

    QuickLauncher::app = new QtopiaApplication( argc, argv );
    QuickLauncher::app->registerRunningTask("QuickLaunch");

    ::unsetenv( "LD_BIND_NOW" );

#ifndef SINGLE_EXEC
    QuickLauncher::loader = new QPluginManager( "application" );
#endif

    QString arg0 = argv[0];
    int sep = arg0.lastIndexOf( '/' );
    if ( sep > 0 )
        arg0 = arg0.mid( sep+1 );
    if ( arg0 != "quicklauncher" ) {
        qLog(Quicklauncher) << "QuickLauncher invoked as:" << arg0.toLatin1();
        QuickLauncher::exec( argc, argv );
    } else {
        QuickLauncher::eventLoop = new QEventLoop();
#ifdef QUICKLAUNCHER_FORKED
        qLog(Quicklauncher) << "QuickLauncherForked running";
#else
        qLog(Quicklauncher) << "QuickLauncher running";
#endif

        // Pre-load default fonts
        QFontMetrics fm( QApplication::font() );
        fm.ascent(); // causes font load.
        QFont f( QApplication::font() );
        f.setWeight( QFont::Bold );
        QFontMetrics fmb( f );
        fmb.ascent(); // causes font load.

        // Each of the following force internal structures/internal
        // initialization to be performed.  This may mean allocating
        // memory that is not needed by all applications.
        QTimeZone::current().isValid(); // populate timezone cache
        QTimeString::currentAMPM(); // create internal structures
        QIcon(":icon/new"); // do internal init
        QImageReader::supportedImageFormats(); // Load image plugins
        QtopiaSql::instance()->openDatabase();
        QtopiaSql::instance()->systemDatabase();

        QSoftMenuBar::menuKey(); // read config.

        // Create a widget to force initialization of title bar images, etc.
        QObject::disconnect(QuickLauncher::app, SIGNAL(lastWindowClosed()), QuickLauncher::app, SLOT(hideOrQuit()));
#if QT_VERSION < 0x040400
        // Avoid warning about Qt::Tool and Qt::FramelessWindowHint in Qt 4.2
        Qt::WindowFlags wFlags = Qt::WindowContextHelpButtonHint | Qt::Tool | Qt::FramelessWindowHint;
#else
        // We do want this behaviour.  Hopefully Qt 4.3 will have it.
        Qt::WindowFlags wFlags = Qt::WindowContextHelpButtonHint|Qt::Tool;
#endif
        QWidget *w = new QWidget(0,wFlags);
        w->setWindowTitle("_ignore_");  // no tr
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setGeometry( -100, -100, 10, 10 );
        w->show();
        QTimer::singleShot( 0, w, SLOT(close()) );

#ifdef QUICKLAUNCHER_FORKED
        QuickLauncher* ql = new QuickLauncher();
#else
        (void)new QuickLauncher();
#endif
        while (!QuickLauncher::validExitLoop)
            if(-1 == QuickLauncher::eventLoop->exec())
                break;

#ifdef QUICKLAUNCHER_FORKED
        delete ql;
#endif
    }

    QPerformanceLog(QuickLauncher::app->applicationName()) << (QPerformanceLog::Begin|QPerformanceLog::EventLoop);
    int rv = -1;
    if(QuickLauncher::app->willKeepRunning())
        QuickLauncher::app->exec();
    QPerformanceLog(QuickLauncher::app->applicationName()) << (QPerformanceLog::End|QPerformanceLog::EventLoop);

    if ( QuickLauncher::mainWindow )
        delete (QWidget*)QuickLauncher::mainWindow;

/* Causes problems with undeleted TLWs
    if ( appIface )
        loader->releaseInterface( appIface );
    delete loader;
*/
    // For performance testing
    QString appName = QuickLauncher::app->applicationName();

    delete QuickLauncher::app;
    delete QuickLauncher::eventLoop;
#ifndef SINGLE_EXEC
    delete QuickLauncher::loader;
#endif

    QPerformanceLog(appName)  << QPerformanceLog::End << "quicklauncher main";

    return rv;
}

