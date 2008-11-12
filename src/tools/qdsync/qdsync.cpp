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
#include <trace.h>
QD_LOG_OPTION(QDSync)

#include "qdsync.h"
#include "qcopbridge.h"
#include "log.h"
#include "qcopenvelope_qd.h"
#include "qcopchannel_qd.h"
#include "qtopia4sync.h"

#include <QtopiaApplication>
#include <QCloseEvent>
#include <version.h>
#include <QSoftMenuBar>
#include <QMenu>
#include <QProcess>
#include <custom.h>
#include <QPluginManager>
#include <private/contextkeymanager_p.h>

// See documentation for this macro in doc/src/syscust/custom.qdoc
#ifndef QDSYNC_DEFAULT_TCP_PORT
#define QDSYNC_DEFAULT_TCP_PORT 4245
#endif

// See documentation for this macro in doc/src/syscust/custom.qdoc
#ifndef QDSYNC_DEFAULT_SERIAL_PORT
#define QDSYNC_DEFAULT_SERIAL_PORT "/dev/ttyS0"
#endif

// See documentation for this macro in doc/src/syscust/custom.qdoc
#ifndef QDSYNC_DEFAULT_PORTS
#define QDSYNC_DEFAULT_PORTS QStringList() << "tcp"
#endif

QTextBrowser *qdsync_tb;
QTOPIA_EXPORT_PLUGIN(QDSync)

QDSync::QDSync( QWidget *parent, Qt::WFlags /*f*/ )
    : QTextBrowser( parent ),
    bridge( 0 ), serialAction( 0 ), ethernetAction( 0 )
    , selectDown( false ), connected( false ), syncing( false )
    , selectLabelState( Blank )
{
    TRACE(QDSync) << "QDSync::QDSync";
    setHtml("Synchronization communicates with Qtopia Sync Agent."
            "<hr>");
    qdsync_tb = this;

    connect( qApp, SIGNAL(appMessage(QString,QByteArray)), this, SLOT(appMessage(QString,QByteArray)) );

    QMenu *menu = new QMenu( this );
    // This is setup for the Greenphone which writes the current gadget to /etc/gadget
    QFile f( "/etc/gadget" );
    if ( f.exists() && f.open(QIODevice::ReadOnly) ) {
        serialAction = menu->addAction( "Serial Gadget", this, SLOT(serialGadget()) );
        ethernetAction = menu->addAction( "Ethernet Gadget", this, SLOT(ethernetGadget()) );
        QByteArray gadget = f.readAll();
        if ( gadget.trimmed() == "ether" )
            ethernetAction->setVisible( false );
        else
            serialAction->setVisible( false );
        f.close();
    }

    menu->addAction( "Restart Synchronization", qApp, SLOT(quit()) );
    QSoftMenuBar::addMenuTo( this, menu );

    // Initialize tasks
    Qtopia4Sync *sync = Qtopia4Sync::instance();
    QPluginManager *manager = new QPluginManager( "qdsync", this );
    foreach ( const QString &name, manager->list() ) {
        if ( name == "base" ) continue;
        QObject *object = manager->instance( name );
        if ( Qtopia4SyncPlugin *plugin = qobject_cast<Qtopia4SyncPlugin*>(object) ) {
            sync->registerPlugin( plugin );
        } else if ( Qtopia4SyncPluginFactory *pf = qobject_cast<Qtopia4SyncPluginFactory*>(object) ) {
            foreach ( const QString &name, pf->keys() ) {
                Qtopia4SyncPlugin *plugin = pf->plugin( name );
                sync->registerPlugin( plugin );
            }
        }
    }

    QCopChannel *chan = new QCopChannel( "QPE/QDSync", this );
    connect( chan, SIGNAL(received(QString,QByteArray)), this, SLOT(qdMessage(QString,QByteArray)) );
}

QDSync::~QDSync()
{
    TRACE(QDSync) << "QDSync::~QDSync";
}

void QDSync::appMessage( const QString &message, const QByteArray & /*data*/ )
{
    TRACE(QDSync) << "QDSync::appMessage" << message;
    if ( message == "startDaemons()" ) {
        startDaemons();
    } else if ( message == "stopDaemons()" ) {
        stopDaemons();
    } else if ( message == "shutdown()" ) {
        stopDaemons();
        QtopiaApplication::instance()->unregisterRunningTask("qdsync");
    }
}

void QDSync::qdMessage( const QString &message, const QByteArray &data )
{
    TRACE(QDSync) << "QDSync::qdMessage" << message;
    static int syncSteps = 0;
    QDataStream stream( data );
    if ( message == "syncStart()" ) {
        USERLOG("Sync started");
        // Just in case this message is processed after a disconnect event,
        // only change the label if it's currently saying "Sync"
        if ( selectLabelState == Sync ) {
            selectLabelState = Cancel;
            // This doesn't work due to a behavior bug in ContextKeyManager
            //QSoftMenuBar::setLabel( this, Qt::Key_Select, QSoftMenuBar::Cancel );
            // FIXME This work around should be removed once the code above works
            QSoftMenuBar::setLabel( this, Qt::Key_Select,
                    ContextKeyManager::standardPixmap(QSoftMenuBar::Cancel),
                    ContextKeyManager::standardText(QSoftMenuBar::Cancel) );
        }
        syncSteps = 0;
        syncing = true;
    } else if ( message == "syncEnd()" ) {
        USERLOG("Sync finished");
        // Just in case this message is processed after a disconnect event,
        // only change the label if it's currently saying "Cancel"
        if ( selectLabelState == Cancel ) {
            selectLabelState = Sync;
            QSoftMenuBar::setLabel( this, Qt::Key_Select, ":icon/qdsync/syncall", tr("Sync") );
        }
        syncSteps = 0;
        syncing = false;
    } else if ( message == "syncSteps(int)" ) {
        stream >> syncSteps;
        LOG() << "Sync steps" << syncSteps;
    } else if ( message == "syncProgress(int)" ) {
        int prog;
        stream >> prog;
        LOG() << "Sync progress" << prog;
        USERLOG(QString("Sync in progress (%1 of %2)").arg(prog).arg(syncSteps));
    } else if ( message == "displayText(QString)" ) {
        QString text;
        stream >> text;
        USERLOG(text);
    }
}

void QDSync::closeEvent( QCloseEvent * /*e*/ )
{
    TRACE(QDSync) << "QDSync::closeEvent";
    // The registerRunningTask() call prevents us from dying
}

void QDSync::showEvent( QShowEvent * /*e*/ )
{
    TRACE(QDSync) << "QDSync::showEvent";
    static bool created = false;
    if ( !created ) {
        created = true;
        setWindowTitle("Synchronization");
        QTextCursor cursor = textCursor();
        cursor.movePosition( QTextCursor::End, QTextCursor::MoveAnchor );
        setTextCursor(cursor);
        ensureCursorVisible();
    }
}

void QDSync::startDaemons()
{
    TRACE(QDSync) << "QDSync::startDaemons" << (bridge?"RUNNING":"NOT RUNNING");
    if ( !bridge ) {
        QtopiaApplication::instance()->registerRunningTask("qdsync", this);

        QSettings settings("Trolltech", "qdsync");

        int version = settings.value("/version", 0).toInt();
        if ( version == 0 ) {
            // version 0: pre-beta
            // remove everything so the new defaults can take effect
            version++;
            settings.remove("/port/tcp");
            settings.remove("/port/serial");
            settings.remove("/port/greenphone_serial");
            settings.remove("/ports");
        }
        settings.setValue("/version", version);

        int port = settings.value("/port/tcp", QDSYNC_DEFAULT_TCP_PORT).toInt();
        QRegExp disp("-(\\d+)\\/$");
        if ( disp.indexIn(Qtopia::tempDir()) )
            port += QVariant(disp.cap(1)).toInt();
        QString serialPort = settings.value("/port/serial", QDSYNC_DEFAULT_SERIAL_PORT).toString();
        QStringList ports = settings.value("/ports", QDSYNC_DEFAULT_PORTS ).toStringList();

        bridge = new QCopBridge( this );
        connect( bridge, SIGNAL(gotConnection()), this, SLOT(gotConnection()) );
        connect( bridge, SIGNAL(lostConnection()), this, SLOT(lostConnection()) );
        int interfaces = 0;

        // Don't start the TCP server if the ethernet gadget is not loaded
        if ( (!ethernetAction || !ethernetAction->isVisible()) && ports.contains("tcp") ) {
            LOG() << "Starting QCopBridge on TCP port" << port;
            if ( bridge->startTcp( port ) ) {
                interfaces++;
                USERLOG(QString("QCopBridge started on TCP port %1").arg(port));
            } else {
                WARNING() << "Synchronization cannot start QCopBridge on port" << port;
            }
        }

        // Don't start the serial server if the serial gadget is not loaded
        if ( (!serialAction || !serialAction->isVisible()) && ports.contains("serial") ) {
            LOG() << "Starting QCopBridge on serial port" << serialPort;
            if ( bridge->startSerial( serialPort ) ) {
                interfaces++;
            } else {
                WARNING() << "Synchronization cannot start QCopBridge on" << serialPort;
            }
        }
    }
}

void QDSync::stopDaemons()
{
    TRACE(QDSync) << "QDSync::stopDaemons" << (bridge?"RUNNING":"NOT RUNNING");
    if ( bridge ) {
        USERLOG("Stopping QCopBridge");
        if ( bridge ) { delete bridge; bridge = 0; }
    }
}

void QDSync::ethernetGadget()
{
    TRACE(QDSync) << "QDSync::ethernetGadget";
    ethernetAction->setVisible( false );
    serialAction->setVisible( true );
    bool running = ( bridge );
    if ( running )
        stopDaemons();
    // This is setup for the Greenphone which uses this script to switch gadgets
    QProcess::execute("/opt/Qtopia/bin/usb-gadget.sh", QStringList() << "unload");
    QProcess::execute("/opt/Qtopia/bin/usb-gadget.sh", QStringList() << "ether");
    if ( running )
        startDaemons();
}

void QDSync::serialGadget()
{
    TRACE(QDSync) << "QDSync::serialGadget";
    serialAction->setVisible( false );
    ethernetAction->setVisible( true );
    bool running = ( bridge );
    if ( running )
        stopDaemons();
    // This is setup for the Greenphone which uses this script to switch gadgets
    QProcess::execute("/opt/Qtopia/bin/usb-gadget.sh", QStringList() << "unload");
    QProcess::execute("/opt/Qtopia/bin/usb-gadget.sh", QStringList() << "winserial");
    if ( running )
        startDaemons();
}

void QDSync::gotConnection()
{
    TRACE(QDSync) << "QDSync::gotConnection";
    // We just got a connection, force the label to "Sync"
    selectLabelState = Sync;
    QSoftMenuBar::setLabel( this, Qt::Key_Select, ":icon/qdsync/syncall", tr("Sync") );
    connected = true;
    selectDown = false;
}

void QDSync::lostConnection()
{
    TRACE(QDSync) << "QDSync::lostConnection";
    // We just got lost the connection, force the label to "Blank"
    selectLabelState = Blank;
    QSoftMenuBar::setLabel( this, Qt::Key_Select, QSoftMenuBar::NoLabel );
    connected = false;
    selectDown = false;
    syncing = false;
}

void QDSync::keyPressEvent( QKeyEvent *e )
{
    if ( e->key() == Qt::Key_Select ) {
        TRACE(QDSync) << "QDSync::keyPressEvent" << "e->key()" << "Qt::Key_Select";
        if ( connected )
            selectDown = (!e->isAutoRepeat());
        else
            selectDown = false;
        LOG() << "selectDown" << selectDown;
    } else {
        QTextBrowser::keyPressEvent( e );
    }
}

void QDSync::keyReleaseEvent( QKeyEvent *e )
{
    if ( e->key() == Qt::Key_Select ) {
        TRACE(QDSync) << "QDSync::keyReleaseEvent" << "e->key()" << "Qt::Key_Select";
        if ( connected && selectDown && !e->isAutoRepeat() ) {
            if ( syncing ) {
                USERLOG("Aborting Sync");
                QCopEnvelope e("QD/Server", "cancelSync()");
            } else {
                USERLOG("Requestng Sync");
                QCopEnvelope e("QD/Server", "startSync()");
            }
        }
        selectDown = false;
        LOG() << "selectDown" << selectDown;
    } else {
        QTextBrowser::keyReleaseEvent( e );
    }
}

