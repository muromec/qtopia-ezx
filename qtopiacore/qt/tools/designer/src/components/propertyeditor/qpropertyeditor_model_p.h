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

#ifndef QPROPERTYEDITOR_MODEL_P_H
#define QPROPERTYEDITOR_MODEL_P_H

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

#include "qpropertyeditor_items_p.h"
#include <qabstractitemmodel.h>

namespace qdesigner_internal {

class ResourceMimeData;

class QPropertyEditorModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    QPropertyEditorModel(QObject *parent = 0);
    ~QPropertyEditorModel();

    inline IProperty *initialInput() const
    { return m_initialInput; }

    inline QModelIndex indexOf(IProperty *property, int column = 0) const
    {
        const int row = rowOf(property);
        return row == -1 ?  QModelIndex() : createIndex(row, column, property);
    }

    inline IProperty *privateData(const QModelIndex &index) const
    { return static_cast<IProperty*>(index.internalPointer()); }

    Qt::ItemFlags flags(const QModelIndex &index) const;
    
    bool resourceImageDropped(const QModelIndex &index, const ResourceMimeData &m);

signals:
    void propertyChanged(IProperty *property);
    void resetProperty(const QString &name);

public slots:
    void setInitialInput(IProperty *initialInput);
    void refresh(IProperty *property);

public:
//
// QAbstractItemModel interface
//
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;

    virtual QModelIndex parent(const QModelIndex &index) const;

    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual bool hasChildren(const QModelIndex &parent) const
    { return rowCount(parent) > 0; }

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    virtual bool isEditable(const QModelIndex &index) const;
    virtual QModelIndex buddy(const QModelIndex &index) const;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

protected:
    QString columnText(int column) const;

private:
    inline IProperty *childAt(IProperty *parent, int pos) const
    {
        if (parent && parent->kind() == IProperty::Property_Group)
            return static_cast<IPropertyGroup*>(parent)->propertyAt(pos);

        return 0;
    }

    inline IProperty *parentOf(IProperty *property) const
    { return property ? property->parent() : 0; }

private:
    int rowOf(IProperty *property) const;
    
    IProperty *m_initialInput;
};

}  // namespace qdesigner_internal

#endif // QPROPERTYEDITOR_MODEL_P_H
