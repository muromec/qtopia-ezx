/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Designer of the Qt Toolkit.
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

#include "flagbox_model_p.h"
#include <QtCore/qdebug.h>

using namespace qdesigner_internal;

FlagBoxModel::FlagBoxModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

FlagBoxModel::~FlagBoxModel()
{
}

void FlagBoxModel::setItems(const QList<FlagBoxModelItem> &items)
{
    m_items = items;
    emit reset();
}

int FlagBoxModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_items.count() : 0;
}

int FlagBoxModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex FlagBoxModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

QModelIndex FlagBoxModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column, 0);
}

QVariant FlagBoxModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.row() != -1);

    const FlagBoxModelItem &item = m_items.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return item.name();

    case Qt::CheckStateRole:
        return item.isChecked() ? Qt::Checked : Qt::Unchecked;

    default:
        return QVariant();
    } // end switch
}

bool FlagBoxModel::setData(const QModelIndex &modelIndex, const QVariant &value, int role)
{
    Q_ASSERT(modelIndex.row() != -1);

    FlagBoxModelItem &item = m_items[modelIndex.row()];

    switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole: {
        item.setName(value.toString());
    } return true;

    case Qt::CheckStateRole: {
        const Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
        item.setChecked(state == Qt::Unchecked ? false : true);
        // There are special flags like QDialogButtonBox::NoButton[AtAll]
        // that affect others. Invalidate the whole model.
        emit dataChanged(index(0, 0, QModelIndex()), index(m_items.size() - 1, 0, QModelIndex()));
    } return true;

    default: break;
    } // end switch

    return false;
}

Qt::ItemFlags FlagBoxModel::flags(const QModelIndex &index) const
{
    Q_ASSERT(index.row() != -1);

    const FlagBoxModelItem &thisItem = m_items[index.row()];
    if (thisItem.value() == 0) {
        // Disabled if checked
        if (thisItem.isChecked())
            return 0;
    } else if (bitcount(thisItem.value()) > 1) {
        // Disabled if all flags contained in the mask are checked
        unsigned int currentMask = 0;
        for (int i = 0; i < m_items.size(); ++i) {
            const FlagBoxModelItem &item = m_items[i];
            if (bitcount(item.value()) == 1)
                currentMask |= item.isChecked() ? item.value() : 0;
        }
        if ((currentMask & thisItem.value()) == thisItem.value())
            return 0;
    }
    return QAbstractItemModel::flags(index);
}

// Helper function that counts the number of 1 bits in argument
int FlagBoxModel::bitcount(int mask)
{
    int count = 0;
    for (int i = 31; i >= 0; --i)
        count += ((mask >> i) & 1) ? 1 : 0;
    return count;
}
