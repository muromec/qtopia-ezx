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

#ifndef MERGEIO_PRIVATE_H
#define MERGEIO_PRIVATE_H

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
#include <QString>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <QList>

#include <quniqueid.h>

#include "qrecordio_p.h"

class QTimer;

// different from above in that it assumes
// main list is v.large (10000) and other lists
// v.small (<200).  Since the only current concrete use case
// is merging with sim card which is < 200, this is appropriate.
// will have to change when add new models, but thats what abstraction is for.
class QBiasedRecordIOMerge : public QObject
{
    Q_OBJECT
public:
    explicit QBiasedRecordIOMerge(QObject *parent);
    ~QBiasedRecordIOMerge();

    void setPrimaryModel(QRecordIO *);
    void removePrimaryModel();

    void addSecondaryModel(QRecordIO *);
    void removeSecondaryModel(QRecordIO *);
    // models expected to filter themselves.

    QList<QRecordIO*> models() const;

    int count() const;

    QRecordIO *model(int index) const;
    int row(int index) const;

    int index(QRecordIO *, int row) const;
    int index(const QUniqueId &) const;

    static bool compare(const QVariant &, const QVariant &);

signals:
    void reset();

public slots:
    void rebuildCache();
    void updateCache();
    void updateCacheNow();

private:
    void lookupRowModel(int index) const;
    int findPrimaryIndex(const QVariant &key, const QUniqueId &id);

    QRecordIO *mPrimary;
    QMap<QRecordIO*, QVector<int> > mSecondaries;
    QList<int> mSkips;
    mutable int lastIndex;
    mutable int lastRow;
    mutable QRecordIO *lastModel;
    QTimer *rebuildTimer;
    bool modelChanged;
};

#endif //MERGEIO_PRIVATE_H
