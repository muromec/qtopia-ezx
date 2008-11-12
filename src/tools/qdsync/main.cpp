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
#include <QtopiaApplication>
#include <QWidget>
#include <QPluginManager>

#ifdef SINGLE_EXEC
QTOPIA_ADD_APPLICATION(QTOPIA_TARGET,qdsync)
#define MAIN_FUNC main_qdsync
#else
#define MAIN_FUNC main
#endif

QSXE_APP_KEY
int MAIN_FUNC( int argc, char **argv )
{
    QSXE_SET_APP_KEY(argv[0]);
    QtopiaApplication app( argc, argv );
    QPluginManager manager( "qdsync" );
    QObject *base = manager.instance("base");
    if ( !base ) {
        qWarning() << "Could not load plugin 'base'";
        return -1;
    }
    QWidget *window = qobject_cast<QWidget*>(base);
    if ( !window ) {
        qWarning() << "Plugin 'base' is not a QWidget";
        return -1;
    }
    app.setMainWidget( window );
    return app.exec();
}

