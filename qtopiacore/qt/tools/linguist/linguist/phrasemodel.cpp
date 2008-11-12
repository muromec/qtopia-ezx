/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Linguist of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "phrasemodel.h"
#include <QtAlgorithms>

static Qt::SortOrder sSortOrder = Qt::AscendingOrder;
static int sSortColumn = 1;

void PhraseModel::removePhrases()
{
    int r = plist.count();
    if (r > 0) {
        plist.clear();
        reset();
    }
}

Phrase PhraseModel::phrase(const QModelIndex &index) const
{
    return plist.at(index.row());
}

void PhraseModel::setPhrase(const QModelIndex &indx, Phrase ph)
{
    int r = indx.row();

    plist[r] = ph;

    // update item in view
    QModelIndex si = QAbstractTableModel::index(r, 0);
    QModelIndex ei = QAbstractTableModel::index(r, 2);
    emit dataChanged(si, ei);
}

QModelIndex PhraseModel::addPhrase(Phrase p)
{
    int r = plist.count();

    plist.append(p);

    // update phrases as we add them
    beginInsertRows(QModelIndex(), r, r);
    QModelIndex i = QAbstractTableModel::index(r, 0);
    endInsertRows();
    return i;
}

void PhraseModel::removePhrase(const QModelIndex &index)
{
    int r = index.row();
    beginRemoveRows(QModelIndex(), r, r);
    plist.removeAt(r);
    endRemoveRows();
}

bool PhraseModel::sortParameters(Qt::SortOrder &so, int &sc) const
{
    if (sortColumn == -1)
        return false;

    so = sortOrder;
    sc = sortColumn;

    return true;
}

void PhraseModel::resort()
{
    if (sortColumn == -1)
        return;

    sort(sortColumn, sortOrder);
}

QModelIndex PhraseModel::index(const Phrase phr) const
{
    int row;
    if ((row = plist.indexOf(phr)) == -1)
        return QModelIndex();

    return QAbstractTableModel::index(row,0);
}

int PhraseModel::rowCount(const QModelIndex &) const
{
    return plist.count();
}

int PhraseModel::columnCount(const QModelIndex &) const
{
    return 3;
}

QVariant PhraseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((role == Qt::DisplayRole) && (orientation == Qt::Horizontal)) {
        switch(section) {
        case 0:
            return tr("Source phrase");
        case 1:
            return tr("Translation");
        case 2:
            return tr("Definition");
        }
    }

    return QVariant();
}

QVariant PhraseModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int column = index.column();

    if (row >= plist.count() || !index.isValid())
        return QVariant();

    Phrase phrase = plist.at(row);

    if (role == Qt::DisplayRole) {
        switch(column) {
        case 0: // source phrase
            return phrase.source().simplified();
        case 1: // translation
            return phrase.target().simplified();
        case 2: // definition
            return phrase.definition();
        }
    }

    return QVariant();
}

void PhraseModel::sort(int column, Qt::SortOrder order)
{
    if (plist.count() <= 0)
        return;

    sortOrder = sSortOrder = order;
    sortColumn = sSortColumn = column;

    qSort(plist.begin(), plist.end(), PhraseModel::compare);
    emit dataChanged(QAbstractTableModel::index(0,0),
        QAbstractTableModel::index(plist.count()-1, 2));
}

bool PhraseModel::compare(const Phrase &left, const Phrase &right)
{
    int res;
    switch (sSortColumn) {
    case 0:
        res = QString::localeAwareCompare(left.source().remove(QLatin1Char('&')),
            right.source().remove(QLatin1Char('&')));
        if ((sSortOrder == Qt::AscendingOrder) ? (res < 0) : !(res < 0))
            return true;
        break;
    case 1:
        res = QString::localeAwareCompare(left.target().remove(QLatin1Char('&')),
            right.target().remove(QLatin1Char('&')));
        if ((sSortOrder == Qt::AscendingOrder) ? (res < 0) : !(res < 0))
            return true;
        break;
    case 2:
        // handle the shortcuts when sorting
        if ((left.shortcut() != -1) && (right.shortcut() == -1))
            return (sSortOrder == Qt::AscendingOrder);
        else if ((left.shortcut() == -1) && (right.shortcut() != -1))
            return (sSortOrder != Qt::AscendingOrder);
        res = QString::localeAwareCompare(left.definition(), right.definition());
        if ((sSortOrder == Qt::AscendingOrder) ? (res < 0) : !(res < 0))
            return true;
        break;
    }

    return false;
}
