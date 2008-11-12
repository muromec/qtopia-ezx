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

#ifndef PROPERTYEDITOR_H
#define PROPERTYEDITOR_H

#include "propertyeditor_global.h"
#include "qpropertyeditor.h"
#include <qdesigner_propertyeditor_p.h>

#include <QtCore/QPointer>

class DomProperty;
class QDesignerMetaDataBaseItemInterface;
class QDesignerPropertySheetExtension;

namespace qdesigner_internal {
class StringProperty;

class QT_PROPERTYEDITOR_EXPORT PropertyEditor: public QDesignerPropertyEditor
{
    Q_OBJECT
public:
    PropertyEditor(QDesignerFormEditorInterface *core, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    virtual ~PropertyEditor();

    virtual QDesignerFormEditorInterface *core() const;

    virtual bool isReadOnly() const;
    virtual void setReadOnly(bool readOnly);
    virtual void setPropertyValue(const QString &name, const QVariant &value, bool changed = true);
    virtual void setPropertyComment(const QString &name, const QString &value);
    virtual void updatePropertySheet();

    virtual void setObject(QObject *object);

    virtual QObject *object() const
    { return m_object; }

    virtual QString currentPropertyName() const;
    
private slots:
    void slotFirePropertyChanged(IProperty *property);
    void slotResetProperty(const QString &prop_name);
    void slotCustomContextMenuRequested(const QPoint &pos);

private:
    IProperty *propertyByName(IProperty *p, const QString &name);
    void clearDirty(IProperty *p);
    void createPropertySheet(PropertyCollection *root, QObject *object);
    static IProperty *createSpecialProperty(const QVariant &value, const QString &name);

private:
    QDesignerMetaDataBaseItemInterface *metaDataBaseItem() const;
    StringProperty* createStringProperty(QObject *object, const QString &pname, const QVariant &value, bool isMainContainer) const;
  
    QDesignerFormEditorInterface *m_core;
    QPropertyEditor *m_editor;
    IPropertyGroup *m_properties;
    QDesignerPropertySheetExtension *m_prop_sheet;
    QPointer<QObject> m_object;
    typedef QMap<int, IProperty *> IndexToPropertyMap;
    IndexToPropertyMap m_indexToProperty;
};

}  // namespace qdesigner_internal

#endif // PROPERTYEDITOR_H
