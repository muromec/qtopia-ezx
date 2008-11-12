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

#ifndef PIMIO_PRIVATE_H
#define PIMIO_PRIVATE_H

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

#include <QObject>
#include <QVariant>

#include <QUniqueId>
#include <QCategoryManager>
#include "qpimsource.h"

class QRecordIO : public QObject
{
    Q_OBJECT

public:
    explicit QRecordIO(QObject *parent) : QObject(parent) { }
    virtual int count() const = 0;

    virtual bool exists(const QUniqueId &) const = 0;
    virtual bool contains(const QUniqueId &) const = 0;

    // acts on visible sources.
    virtual bool startSyncTransaction(const QSet<QPimSource> &, const QDateTime &) = 0;
    virtual bool abortSyncTransaction() = 0;
    virtual bool commitSyncTransaction() = 0;

    // acts on visible sources.
    virtual QList<QUniqueId> removed(const QSet<QPimSource> &, const QDateTime &) const = 0;
    virtual QList<QUniqueId> added(const QSet<QPimSource> &, const QDateTime &) const = 0;
    virtual QList<QUniqueId> modified(const QSet<QPimSource> &, const QDateTime &) const = 0;

    virtual QUuid contextId() const = 0;

    // needs to be comparable?
    virtual QVariant key(int row) const = 0;
    virtual QUniqueId id(int row) const = 0;
    virtual int row(const QUniqueId &) const = 0;

    virtual int predictedRow(const QVariant &, const QUniqueId &) const;

    virtual void setCategoryFilter(const QCategoryFilter &) = 0;
    virtual QCategoryFilter categoryFilter() const = 0;

signals:
    void recordsUpdated();
    void filtersUpdated();
};

#endif // PIMIO_PRIVATE_H
