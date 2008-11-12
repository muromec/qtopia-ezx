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
#ifndef QDSYNC_H
#define QDSYNC_H

#include <QTextBrowser>

class QCopBridge;
class TransferServer;

class QDSync : public QTextBrowser
{
    Q_OBJECT
public:
    QDSync( QWidget *parent = 0, Qt::WFlags f = 0 );
    ~QDSync();

private slots:
    void appMessage( const QString &message, const QByteArray &data );
    void qdMessage( const QString &message, const QByteArray &data );
    void startDaemons();
    void stopDaemons();
    void ethernetGadget();
    void serialGadget();
    void gotConnection();
    void lostConnection();

private:
    void showEvent( QShowEvent *e );
    void closeEvent( QCloseEvent *e );
    void keyPressEvent( QKeyEvent *e );
    void keyReleaseEvent( QKeyEvent *e );

    QCopBridge *bridge;
    TransferServer *tserver;
    QAction *serialAction;
    QAction *ethernetAction;
    bool selectDown;
    bool connected;
    bool syncing;

    enum SelectLabelState {
        Blank,
        Sync,
        Cancel,
    } selectLabelState;
};

#endif
