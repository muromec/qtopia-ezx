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

#ifndef ADDRESSBOOK_SQLIO_PRIVATE_H
#define ADDRESSBOOK_SQLIO_PRIVATE_H

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

#include <qtopiasql.h>
#include <qsqlquery.h>
#include <qvariant.h>
#include <qmap.h>
#include <qhash.h>
#include <qlist.h>
#include <qcache.h>

#include <qtopia/pim/qcontact.h>

#include "qcontactio_p.h"
#include "qpimsqlio_p.h"
#include "qpimsource.h"

class QContactDefaultContextData;
class QTOPIAPIM_EXPORT QContactDefaultContext : public QContactContext
{
    Q_OBJECT
public:
    // could have constructor protected/private with friends class.
    QContactDefaultContext(QObject *parent, QObject *access);

    QIcon icon() const; // default empty
    QString description() const;

    using QContactContext::title;
    QString title() const;

    // better to be flags ?
    using QContactContext::editable;
    bool editable() const; // default true

    QPimSource defaultSource() const;
    void setVisibleSources(const QSet<QPimSource> &);
    QSet<QPimSource> visibleSources() const;
    QSet<QPimSource> sources() const;
    QUuid id() const;

    using QContactContext::exists;
    bool exists(const QUniqueId &) const;
    QPimSource source(const QUniqueId &) const;

    bool updateContact(const QContact &);
    bool removeContact(const QUniqueId &);
    QUniqueId addContact(const QContact &, const QPimSource &);

    bool importContacts(const QPimSource &, const QList<QContact> &);

    QList<QContact> exportContacts(const QPimSource &, bool &) const;
    QContact exportContact(const QUniqueId &id, bool &) const;


    void startSync();
    bool syncing() const;
    void syncProgress(int &amount, int &total) const;

    enum Status {
        NotStarted,
        InProgress,
        Completed,
        BadAuthentication,
        ServiceUnavailable,
        ParseError,
        DataAccessError,
        UnknownError
    };

    Status status() const;
    static QString statusMessage(Status status);

signals:
    void syncProgressChanged(int, int);
    void syncStatusChanged(const QString &account, Status);
    void finishedSyncing();

private:
    QContactDefaultContextData *d;
};

class QSqlPimTableModel;
class ContactSimpleQueryCache;
class ContactSqlIO : public QContactIO, public QPimSqlIO {

    Q_OBJECT

public:
    explicit ContactSqlIO( QObject *parent = 0, const QString &name = QString());

    ~ContactSqlIO();

    QUuid contextId() const;

    int count() const { return QPimSqlIO::count(); }

    bool editableByRow() const { return true; }
    bool editableByField() const { return true; }

    QVariant key(const QUniqueId &) const;

    void setSortKey(QContactModel::Field k);
    QContactModel::Field sortKey() const;

    void setCategoryFilter(const QCategoryFilter &f);
    QCategoryFilter categoryFilter() const { return QPimSqlIO::categoryFilter(); }

    void setContextFilter(const QSet<int> &, ContextFilterType = ExcludeContexts );

    bool startSyncTransaction(const QSet<QPimSource> &sources, const QDateTime &syncTime) { return QPimSqlIO::startSync(sources, syncTime); }
    bool abortSyncTransaction() { return QPimSqlIO::abortSync(); }
    bool commitSyncTransaction() { return QPimSqlIO::commitSync(); }

    QList<QUniqueId> removed(const QSet<QPimSource> &sources, const QDateTime &timestamp) const
    { return QPimSqlIO::removed(sources, timestamp); }

    QList<QUniqueId> added(const QSet<QPimSource> &sources, const QDateTime &timestamp) const
    { return QPimSqlIO::added(sources, timestamp); }

    QList<QUniqueId> modified(const QSet<QPimSource> &sources, const QDateTime &timestamp) const
    { return QPimSqlIO::modified(sources, timestamp); }

    QVariant contactField(int row, QContactModel::Field k) const;
    QContact simpleContact(int row) const;

    QContact contact(const QUniqueId &) const;
    QContact contact(int row) const;
    QUniqueId id(int row) const { return QPimSqlIO::recordId(row); }
    QVariant key(int row) const;
    int row(const QUniqueId & id) const { return QPimSqlIO::row(id); }
    int predictedRow(const QVariant &k, const QUniqueId &i) const;

    bool removeContact(int row);
    bool removeContact(const QUniqueId & id);
    bool removeContact(const QContact &t);
    bool removeContacts(const QList<int> &rows);
    bool removeContacts(const QList<QUniqueId> &ids);

    bool updateContact(const QContact &t);
    QUniqueId addContact(const QContact &contact, const QPimSource &s)
    { return addContact(contact, s, true); }
    QUniqueId addContact(const QContact &contact, const QPimSource &, bool);

    bool exists(const QUniqueId & id) const { return !contact(id).uid().isNull(); }
    bool contains(const QUniqueId & id) const { return QPimSqlIO::contains(id); }

    void setFilter(const QString &, int);
    void clearFilter();

#ifdef QTOPIA_PHONE
    QUniqueId matchPhoneNumber(const QString &, int &) const;
#endif
    void invalidateCache();

    void checkAdded(const QUniqueId &) { invalidateCache(); }
    void checkRemoved(const QUniqueId &) { invalidateCache(); }
    void checkRemoved(const QList<QUniqueId> &) { invalidateCache(); }
    void checkUpdated(const QUniqueId &) { invalidateCache(); }
protected:
    void bindFields(const QPimRecord &, QPreparedSqlQuery &) const;
    QStringList sortColumns() const;
    QString sqlColumn(QContactModel::Field k) const;

    bool updateExtraTables(uint, const QPimRecord &);
    bool insertExtraTables(uint, const QPimRecord &);
    bool removeExtraTables(uint);

private slots:
    void updateSqlLabel();

private:
    QContactModel::Field orderKey;

    QString sqlLabel() const;
    QString sqlField(QContactModel::Field) const;
    bool canUpdate(QContactModel::Field) const;
    QContact::PhoneType fieldToPhoneType(QContactModel::Field) const;
    void initMaps();

    mutable bool contactByRowValid;
    mutable QContact lastContact;
    QString sqlLabelCache;

    QString mSearchText;
    int mSearchIndex;
    QSqlPimTableModel *searchTable;

    bool tmptable;

    typedef QMultiHash<QString, QPair<QUniqueId, QString> > LocalNumberCache;
    mutable LocalNumberCache mLocalNumberCache;

    static QMap<QContactModel::Field, QString> mFields;
    static QMap<QContactModel::Field, bool> mUpdateable;

    // Saved queries
    mutable QPreparedSqlQuery contactQuery;
    mutable QPreparedSqlQuery emailsQuery;
    mutable QPreparedSqlQuery addressesQuery;
    mutable QPreparedSqlQuery phoneQuery;
    mutable QPreparedSqlQuery insertEmailsQuery;
    mutable QPreparedSqlQuery insertAddressesQuery;
    mutable QPreparedSqlQuery insertPhoneQuery;
    mutable QPreparedSqlQuery removeEmailsQuery;
    mutable QPreparedSqlQuery removeAddressesQuery;
    mutable QPreparedSqlQuery removePhoneQuery;
    mutable ContactSimpleQueryCache *simpleCache;
};

#endif
