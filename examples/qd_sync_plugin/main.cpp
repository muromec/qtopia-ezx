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

#include <qdplugin.h>

class MySyncPlugin : public QDServerSyncPlugin
{
    Q_OBJECT
    QD_CONSTRUCT_PLUGIN(MySyncPlugin,QDServerSyncPlugin)
public:
    QString id() { return "com.myorg.mysyncplugin"; }
    QString displayName() { return tr("MySyncPlugin"); }

    QString dataset() { return "mydataset"; }
    //QByteArray referenceSchema() { return QByteArray(); }
    void prepareForSync()
    {
        emit readyForSync( true );
    }

    void finishSync()
    {
        emit finishedSync();
    }

    void fetchChangesSince( const QDateTime & /*timestamp*/ )
    {
        // emit createServerRecord(QByteArray)
        // emit replaceServerRecord(QByteArray)
        // emit deleteServerRecord(QString)

        // emit serverChangesCompleted()
    }

    void beginTransaction( const QDateTime & /*timestamp*/ )
    {
        // set a point for roll back
    }

    void createClientRecord( const QByteArray & /*record*/ )
    {
        // store record
        // emit mappedId(localId,clientId)
    }

    void replaceClientRecord( const QByteArray & /*record*/ )
    {
        // store record
    }

    void removeClientRecord( const QString & /*localId*/ )
    {
        // remove record
    }

    void abortTransaction()
    {
        // revert any create/replace/delete events since beginTransaction()
    }

    void commitTransaction()
    {
        // commit any create/replace/delete events since beginTransaction()
    }
};

QD_REGISTER_PLUGIN(MySyncPlugin)
