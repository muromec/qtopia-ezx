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

#include <QTimer>
#include "qrecordiomerge_p.h"

#include <QDateTime>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QDebug>

#include <qtopiasql.h>

static const int cacheincrement = 1000;

bool lessThan(const QVariant &left, const QVariant &right) {
    if (left.type() != right.type()) {
        if (left.canConvert(QVariant::String) && right.canConvert(QVariant::String))
            return left.toString() < right.toString();
        return QtopiaSql::stringCompare(left.toString(), right.toString()) < 0;
    }

    switch(left.type()) {
        default:
            break;
        case QVariant::Time:
            return left.toTime() < right.toTime();
        case QVariant::Date:
            return left.toDate() < right.toDate();
        case QVariant::DateTime:
            return left.toDateTime() < right.toDateTime();
        case QVariant::UInt:
            return left.toUInt() < right.toUInt();
        case QVariant::Int:
            return left.toInt() < right.toInt();
        case QVariant::Double:
            return left.toDouble() < right.toDouble();
        case QVariant::ULongLong:
            return left.toULongLong() < right.toULongLong();
        case QVariant::LongLong:
            return left.toLongLong() < right.toLongLong();
        case QVariant::ByteArray:
            return left.toByteArray() < right.toByteArray();
        case QVariant::Char:
            return left.toChar() < right.toChar();
        case QVariant::String:
            return QtopiaSql::stringCompare(left.toString(), right.toString()) < 0;
        case QVariant::StringList:
            {
                QStringList leftl = left.toStringList();
                QStringList rightl = right.toStringList();
                QStringListIterator lefti(leftl);
                QStringListIterator righti(rightl);
                while(lefti.hasNext() && righti.hasNext()) {
                    QString lefts = lefti.next();
                    QString rights = righti.next();
                    int r = QtopiaSql::stringCompare(lefts, rights);
                    if (r < 0)
                        return true;
                    if (r > 0)
                        return false;
                }
                // at least one list emptied out. compare based of which.
                return righti.hasNext();
            }
    }
    return false;
}

//////////////////////////
// QBiasedRecordIOMerge
/////////////////////////

/*!
  \class QBiasedRecordIOMerge
  \mainclass
  \module qpepim
  \ingroup pim
  \brief The QBiasedRecordIOMerge class provides a way of merging records from multiple
  subclasses of QRecordIO.

  The QBiasedRecordIOMerge class provides a way of merging records from multiple
  subclasses of QRecordIO.  It is suitable for cases where the main list
  arbitrarily large and the other lists are small.  100000 items only takes
  25% longer to merge than if the longest list was 10000 items.
  Caches the shorter lists in respect to the longer lists.

  Performance should be sufficient up until on of the shorter list is about
  5000 items.

  \internal
*/

/*!
  \internal
  Construct a QBiasedRecordIOMerge object with parent \a parent.
*/
QBiasedRecordIOMerge::QBiasedRecordIOMerge(QObject *parent)
    : QObject(parent), mPrimary(0), lastIndex(-1), lastRow(-1), lastModel(0),
    modelChanged(false)
{
    rebuildTimer = new QTimer(this);
    rebuildTimer->setSingleShot(true);
    rebuildTimer->setInterval(0);

    connect(rebuildTimer, SIGNAL(timeout()),
            this, SLOT(rebuildCache()));
}

/*!
  \internal
  Destroys the QBiasedRecordIOMerge object.
*/
QBiasedRecordIOMerge::~QBiasedRecordIOMerge()
{
}

/*!
  \internal
  Sets the primary model to \a model.
*/
void QBiasedRecordIOMerge::setPrimaryModel(QRecordIO *model)
{
    if (mPrimary)
        removePrimaryModel();

    mPrimary = model;

    connect(mPrimary, SIGNAL(recordsUpdated()),
            this, SLOT(updateCacheNow()));
    connect(mPrimary, SIGNAL(filtersUpdated()),
            this, SLOT(updateCache()));
    updateCache();
}

/*!
  \internal
  Removes the primary model from the list.  In this case all shorter lists
  are cached against the empty list.
*/
void QBiasedRecordIOMerge::removePrimaryModel()
{
    if (mPrimary) {
        disconnect(mPrimary, SIGNAL(recordsUpdated()),
                this, SLOT(updateCacheNow()));
        disconnect(mPrimary, SIGNAL(filtersUpdated()),
                this, SLOT(updateCache()));
        mPrimary = 0;
        updateCache();
    }
}

/*!
  \internal
  Adds the secondary model \a model.  Count for the model should be
  under 1000 items.
*/
void QBiasedRecordIOMerge::addSecondaryModel(QRecordIO *model)
{
    QVector<int> lookup(model->count());
    mSecondaries.insert(model, lookup);
    connect(model, SIGNAL(filtersUpdated()),
            this, SLOT(updateCache()));
    connect(model, SIGNAL(recordsUpdated()),
            this, SLOT(updateCacheNow()));
    updateCache();
}

/*!
  \internal
  Removes the secondary model \a model.
*/
void QBiasedRecordIOMerge::removeSecondaryModel(QRecordIO *model)
{
    disconnect(model, SIGNAL(filtersUpdated()),
            this, SLOT(updateCache()));
    disconnect(model, SIGNAL(recordsUpdated()),
            this, SLOT(updateCacheNow()));
    mSecondaries.remove(model);
    updateCache();
}


/*!
  Returns the list of models stored used for merging.
*/
QList<QRecordIO*> QBiasedRecordIOMerge::models() const
{
    QList<QRecordIO *> result;
    if (mPrimary)
        result.append(mPrimary);

    result += mSecondaries.keys();
    return result;
}

/*!
  Returns the count for the merged records.
*/
int QBiasedRecordIOMerge::count() const
{
    int count;
    if (mPrimary)
        count = mPrimary->count();
    else
        count = 0;
    QList<QRecordIO *> others = mSecondaries.keys();
    foreach(QRecordIO *model, others) {
        count += model->count();
    }
    return count;
}

/*!
  \internal
  Returns the model for which the record at \a index is stored.
*/
QRecordIO *QBiasedRecordIOMerge::model(int index) const
{
    if (lastIndex == index)
        return lastModel;
    lookupRowModel(index);
    return lastModel;
}

/*!
  \internal
  Caches information for record and model at index \a index.
*/
void QBiasedRecordIOMerge::lookupRowModel(int index) const
{
    lastIndex = index;

    /* for each model, work out how many are at or before index
       That gives offset into primary for last compare */
    QMapIterator<QRecordIO *, QVector<int> > it(mSecondaries);
    QMap<QRecordIO *, int> eqivs;
    int earlier = 0;
    while(it.hasNext()) {
        it.next();
        // could bsearch then shift.  but is expected to be pretty small.
        QVector<int> lookup = it.value();
        int i;
        for(i = 0; i < lookup.size(); ++i) {

            if (lookup[i] == index) {
                lastModel = it.key();
                lastRow = i;
                return;
            } else if (lookup[i] > index) {
                break;
            }
        }
        earlier += i;
    }
    lastModel = mPrimary;
    if (lastModel)
        lastRow = index-earlier;
    else
        lastRow = -1;
}

/*!
  \internal
  Returns the row in the model for which the record at \a index is stored.
*/
int QBiasedRecordIOMerge::row(int index) const
{
    if (lastIndex == index)
        return lastRow;
    lookupRowModel(index);
    return lastRow;
}

/*!
  \internal
  Returns the index that the record at \a row in model \a m is shown at
  when merged.
*/
int QBiasedRecordIOMerge::index(QRecordIO *model, int r) const
{
    if (mSecondaries.contains(model))
        return mSecondaries[model][r];

    if (lastModel == model && lastRow == r)
        return lastIndex;

    // assert model == primary
    // could bsearch model() 0..r;
    // or could have one additional map.
    int earlier = 0;
    // again, could bsearch, but expected to be pretty small.
    foreach(int skip, mSkips) {
        if (skip <= r) {
            earlier++;
        } else {
            break;
        }
    }
    lastRow = r;
    lastModel = model;
    lastIndex = r+earlier;
    return lastIndex;
}

/*!
  \internal
  Returns the index that the record with identity \a id is shown at.
*/
int QBiasedRecordIOMerge::index(const QUniqueId &id) const
{
    if (mPrimary && mPrimary->contains(id))
        return index(mPrimary, mPrimary->row(id));

    QMapIterator<QRecordIO*, QVector<int> > it(mSecondaries);
    while(it.hasNext()) {
        it.next();
        QRecordIO *m = it.key();
        if (m->contains(id))
            return index(m, m->row(id));
    }
    return -1;

}

/*!
  \internal
  Schedules the index merge cache to be rebuilt.  Will be done
  in the next event loop.

  This is because rebuilds can be expensive and if all models
  change will result in unnecessary rebuilds
 */
void QBiasedRecordIOMerge::updateCache()
{
    modelChanged = true;
    if (!rebuildTimer->isActive())
        rebuildTimer->start();
}

/*!
  \internal
  Forces the index merge cache to be rebuilt immediately.

  This can be expensive.
 */
void QBiasedRecordIOMerge::updateCacheNow()
{
    modelChanged = true;
    rebuildCache();
}

/*!
  \internal
  Rebuilds cache of secondary lists merge information.
  performance should be
  (sum of counts of secondary lists)*log(count of primary list)
*/
void QBiasedRecordIOMerge::rebuildCache()
{
    if (!modelChanged)
        return;
    modelChanged = false;
    rebuildTimer->stop();

    lastIndex = -1;
    lastRow = -1;
    lastModel = 0;
    mSkips.clear();

    QMutableMapIterator<QRecordIO *, QVector<int> > it(mSecondaries);
    QMap<QRecordIO *,int> orderedRows;
    while(it.hasNext()) {
        it.next();
        QVector<int> v = it.value();
        v.resize(it.key()->count());
        it.setValue(v);
        orderedRows.insert(it.key(), 0);
    }

    QList<QRecordIO *> orderedModels;

    it.toFront();
    while(it.hasNext()) {
        it.next();
        QRecordIO *m = it.key();
        int r = orderedRows[m];
        if (r >= mSecondaries[m].count()) {
            continue;
        }

        QMutableListIterator<QRecordIO *> lit(orderedModels);
        QVariant key = m->key(r);
        while(lit.hasNext()) {
            QRecordIO *compare = lit.next();
            int cr = orderedRows[compare];
            QVariant ckey = compare->key(cr);
            if (lessThan(key, ckey) || (ckey == key && m->id(r) < compare->id(cr))) {
                lit.previous();
                break;
            }
        }
        lit.insert(m);
    }

    // pull front.
    // find index into main.  (int keeps track of added)
    // repeat.

    int skipped = 0;
    while (orderedModels.count()) {
        // pull front
        QRecordIO *m = orderedModels.takeFirst();
        int r = orderedRows[m];

        QVariant key = m->key(r);
        QUniqueId id = m->id(r);

        /* TODO find index */
        /* this should be bsearch or whatever is most appropriate */
        int index = findPrimaryIndex(key, id);

        mSecondaries[m][r] = index + skipped;
        mSkips.append(index);
        skipped++;

        r++;
        orderedRows[m] = r;

        if (r >= mSecondaries[m].size()) {
            continue;
        }

        key = m->key(r);

        QMutableListIterator<QRecordIO *> lit(orderedModels);
        while(lit.hasNext()) {
            QRecordIO *compare = lit.next();
            int cr = orderedRows[compare];
            QVariant ckey = compare->key(cr);
            if (lessThan(key, ckey) || (ckey == key && m->id(r) < compare->id(cr))) {
                lit.previous();
                break;
            }
        }
        lit.insert(m);
    }
    emit reset();
}

/*!
  \internal
  bsearch algorithm
*/
int QBiasedRecordIOMerge::findPrimaryIndex(const QVariant &key, const QUniqueId &id)
{
    // bsearch.
    if (!mPrimary)
        return 0;

    return mPrimary->predictedRow(key, id);
}

int QRecordIO::predictedRow(const QVariant &k, const QUniqueId &i) const
{
    // simple bsearch by default
    int from = 0;
    int to = count();
    int offset = (to-from)>>1;
    int r = from + offset;
    while(to != from) {
        QVariant pk = key(r);
        QUniqueId pi = id(r);

        if (lessThan(k, pk) || (pk == k && i < pi)) {
            if (from == r)
                return r;
            to = r;
        } else {
            if (from == r)
                return r+1;
            from = r;
        }
        offset = (to-from)>>1;
        r = from + offset;
    }
    return from;
}

bool QBiasedRecordIOMerge::compare(const QVariant &left, const QVariant &right)
{
    return lessThan(left, right);
}

/*!
  \class QRecordIO
  \mainclass
  \module qpepim
  \ingroup pim
  \brief The QRecordIO class provides an interface that will allow merging
  of records into a single list.

  \internal

  \sa QBiasedRecordIOMerge
*/

/*!
  \fn QRecordIO::QRecordIO(QObject *parent)
  \internal

  Construct a QRecordIO object with parent \a parent.
*/

/*!
  \fn virtual int QRecordIO::count() const
  \internal

  Return the number of records provided by the QRecordIO.
*/

/*!
  \fn virtual bool QRecordIO::exists(const QUniqueId &id) const
  \internal

  Returns true if a record with identifier \a id exists in the
  QRecordIO.  Otherwise returns false.

  Does not necessarily mean that the record is contained in the
  filtered and shown list, only that it can be updated or removed
  by the model and may be made to be shown with the correct filter
  settings.

  \sa contains()
*/

/*!
  \fn virtual bool QRecordIO::contains(const QUniqueId &id) const

  Returns true if a record with identifier \a id is included in the
  current filter settings for the QRecordIO.  Otherwise returns false.

  \sa exists()
*/

/*!
  \fn virtual QVariant QRecordIO::key(int row) const
  \internal

  Returns a key value for sorting of records for the record at \a row.
  key is expected to be of the type time, date, datetime, number,
  or string.
*/

/*!
  \fn virtual QUniqueId QRecordIO::id(int row) const
  \internal

  Returns the identifier for the record at \a row.
*/

/*!
  \fn virtual int QRecordIO::row(const QUniqueId &id) const
  \internal

  Returns the row of the record with identifier \a id.  If does
  not contain a record with identifier \a id will return -1.

  \sa contains(), exists()
*/

/*!
  \fn void QRecordIO::recordsUpdated()
  \internal

  This signal should be emitted whenever an actual record is
  updated (via add, remove, update etc).

  \sa filtersUpdated()
 */

/*!
  \fn void QRecordIO::filtersUpdated()
  \internal

  This signal should be emitted whenever the set of filtered
  records of the QRecordIO are changed (via changing filters,
  sources or ranges etc).

  \sa recordsUpdated()
*/
