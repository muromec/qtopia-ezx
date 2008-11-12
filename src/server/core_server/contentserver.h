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

#ifndef CONTENTSERVER_H
#define CONTENTSERVER_H

#include <QThread>
#include <QStringList>
#include <QSet>
#include <QTimer>

#include <qcontent.h>
#include <qcontentset.h>
#include <qtopiaipcadaptor.h>
#include "qtopiaserverapplication.h"
#include <qtopia/private/contentserverinterface_p.h>
#include <QtopiaDocumentServer>

class ServerInterface;
class AppLoaderPrivate;
class QValueSpaceObject;

class ContentServer : public ContentServerInterface
{
    Q_OBJECT
public:
    static ContentServer *getInstance();

    ContentServer( QObject *parent = 0 );
    ~ContentServer();
    virtual void run();

    void removeContent(QContentIdList cids);
    void addContent(const QFileInfo &fi);

private slots:
    void initDocScan();
    void processContent();

private:
    QContentIdList removeList;
    QList<QFileInfo> addList;
    QTimer contentTimer;
    QValueSpaceObject *scannerVSObject;
    QMutex guardMutex;
    friend class ContentProcessor;
    friend class DirectoryScannerManager;
};

/*!
  \internal
  \class RequestQueue
  Models all registered QContentSet objects and on receipt of requests to
  update the database from new path information, forwards updates to those
  objects which are affected.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
*/
class RequestQueue : public QtopiaIpcAdaptor
{
    Q_OBJECT
public:
    RequestQueue();
    ~RequestQueue();
public slots:
    //void newRequest( const QString &msg, const QByteArray &data );
    void receiveUpdate();
    void reflectMirror( QContentId cid, QContent::ChangeType ctype );
    void scanPath(const QString &newPath, int priority);
    void registerCLS(const QString& CLSid, const QContentFilter& filters, int index);
    void unregisterCLS(const QString& CLSid, int index);
private:
    friend class ContentServer;
    QContentSet *cls;
    QHash<QString, QContentSet *> mirrors;
    QHash<QContentSet *, QString> mirrorsByPtr;
};

// declare ContentServerTask
class ContentServerTask : public SystemShutdownHandler
{
Q_OBJECT
public:
    ContentServerTask();

    virtual bool systemRestart();
    virtual bool systemShutdown();

private slots:
    void finished();

private:
    void doShutdown();

    bool m_finished;
};

class DocumentServerTask : public SystemShutdownHandler
{
    Q_OBJECT
public:
    DocumentServerTask();

    virtual bool systemRestart();
    virtual bool systemShutdown();

private:
    QtopiaDocumentServer m_server;
};

#endif
