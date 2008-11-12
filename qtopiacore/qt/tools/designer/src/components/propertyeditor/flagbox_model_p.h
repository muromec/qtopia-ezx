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

#ifndef FLAGBOX_MODEL_P_H
#define FLAGBOX_MODEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "propertyeditor_global.h"
#include <QtCore/QAbstractItemModel>
#include <QtCore/QList>

namespace qdesigner_internal {

class QT_PROPERTYEDITOR_EXPORT FlagBoxModelItem
{
public:
    FlagBoxModelItem(const QString &name, unsigned int value, bool checked = false)
        : m_name(name), m_value(value), m_checked(checked) {}

    inline unsigned int value() const { return m_value; }

    inline QString name() const { return m_name; }
    inline void setName(const QString &name) { m_name = name; }

    inline bool isChecked() const { return m_checked; }
    inline void setChecked(bool checked) { m_checked = checked; }

private:
    QString m_name;
    unsigned int m_value;
    bool m_checked;
};

class QT_PROPERTYEDITOR_EXPORT FlagBoxModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    FlagBoxModel(QObject *parent = 0);
    virtual ~FlagBoxModel();

    inline FlagBoxModelItem itemAt(int index) const
    { return m_items.at(index); }

    inline FlagBoxModelItem &item(int index)
    { return m_items[index]; }

    inline QList<FlagBoxModelItem> items() const { return m_items; }
    void setItems(const QList<FlagBoxModelItem> &items);

    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual bool hasChildren(const QModelIndex &parent) const
    { return rowCount(parent) > 0; }

    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    static int bitcount(int mask);

private:
    QList<FlagBoxModelItem> m_items;
};

}  // namespace qdesigner_internal

#endif // FLAGBOX_MODEL_P_H
