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

#ifndef TODO_SQLIO_PRIVATE_H
#define TODO_SQLIO_PRIVATE_H

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

#include <qtopia/pim/qtask.h>
#include <qtopiasql.h>
#include <qsqlquery.h>
#include <QHash>
#include "qpimsource.h"
#include "qtaskio_p.h"
#include "qpimsqlio_p.h"

class QTaskSqlIO;
class QTaskDefaultContextData;
class QTaskDefaultContext : public QTaskContext
{
    Q_OBJECT
public:
    // could have constructor protected/private with friends class.
    QTaskDefaultContext(QObject *parent, QObject *access);

    QIcon icon() const; // default empty
    QString description() const;

    using QTaskContext::title;
    QString title() const;

    // better to be flags ?
    using QTaskContext::editable;
    bool editable() const; // default true

    QPimSource defaultSource() const;
    void setVisibleSources(const QSet<QPimSource> &);
    QSet<QPimSource> visibleSources() const;
    QSet<QPimSource> sources() const;
    QUuid id() const;

    using QTaskContext::exists;
    bool exists(const QUniqueId &) const;
    QPimSource source(const QUniqueId &) const;

    bool updateTask(const QTask &);
    bool removeTask(const QUniqueId &);
    QUniqueId addTask(const QTask &, const QPimSource &);

    void processRecurringTasks();

private:
    QTaskDefaultContextData *d;
};

class QTaskSqlIO : public QTaskIO, public QPimSqlIO {

    Q_OBJECT

 public:
     explicit QTaskSqlIO(QObject *parent = 0, const QString &name = QString());
  ~QTaskSqlIO();

  QUuid contextId() const;
  int count() const { return QPimSqlIO::count(); }

  bool editableByRow() const { return true; }
  bool editableByField() const { return true; }

  void setSortKey(QTaskModel::Field k);
  QTaskModel::Field sortKey() const;

  void setCategoryFilter(const QCategoryFilter &f);
  QCategoryFilter categoryFilter() const { return QPimSqlIO::categoryFilter(); }

  void setContextFilter(const QSet<int> &);
  QSet<int> contextFilter() const;

  bool startSyncTransaction(const QSet<QPimSource> &sources, const QDateTime &syncTime) { return QPimSqlIO::startSync(sources, syncTime); }
  bool abortSyncTransaction() { return QPimSqlIO::abortSync(); }
  bool commitSyncTransaction() { return QPimSqlIO::commitSync(); }

  QList<QUniqueId> removed(const QSet<QPimSource> &sources, const QDateTime &timestamp) const
  { return QPimSqlIO::removed(sources, timestamp); }

  QList<QUniqueId> added(const QSet<QPimSource> &sources, const QDateTime &timestamp) const
  { return QPimSqlIO::added(sources, timestamp); }

  QList<QUniqueId> modified(const QSet<QPimSource> &sources, const QDateTime &timestamp) const
  { return QPimSqlIO::modified(sources, timestamp); }

  bool completedFilter() const;
  void setCompletedFilter(bool);

  QTask task(const QUniqueId &) const;
  QTask task(int row) const;
  QUniqueId taskId(int row) const { return QPimSqlIO::recordId(row); }
  int row(const QUniqueId & id) const { return QPimSqlIO::row(id); }
  QUniqueId id(int row) const { return QPimSqlIO::recordId(row); }
  QVariant key(int row) const;

  QVariant taskField(int row, QTaskModel::Field k) const;
  bool setTaskField(int row, QTaskModel::Field k,  const QVariant &);

  bool removeTask(int row);
  bool removeTask(const QUniqueId & id);
  bool removeTask(const QTask &);
  bool removeTasks(const QList<int> &rows);
  bool removeTasks(const QList<QUniqueId> &ids);

  bool updateTask(const QTask &t);
  QUniqueId addTask(const QTask &t, const QPimSource &s)
  { return addTask(t, s, true); }
  QUniqueId addTask(const QTask &, const QPimSource &, bool);

  bool exists(const QUniqueId & id) const { return !task(id).uid().isNull(); }
  bool contains(const QUniqueId & id) const { return row(id) != -1; }

  // searching.  row based.
  // pda based
  int startSearch(const QString &) { return -1; }
  int nextSearchItem() { return -1; }
  void clearSearch() { }

  void checkAdded(const QUniqueId &) { invalidateCache(); }
  void checkRemoved(const QUniqueId &) { invalidateCache(); }
  void checkRemoved(const QList<QUniqueId> &) { invalidateCache(); }
  void checkUpdated(const QUniqueId &) { invalidateCache(); }
 protected:
  void bindFields(const QPimRecord &, QPreparedSqlQuery &) const;
  void invalidateCache();
  QStringList sortColumns() const;
  QStringList otherFilters() const;

 private:
  bool cCompFilter;
  QTaskModel::Field cSort;

  mutable bool taskByRowValid;
  mutable QTask lastTask;
  mutable QPreparedSqlQuery repeatFieldQuery;
};

#endif
