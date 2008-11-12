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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDESIGNER_UTILS_H
#define QDESIGNER_UTILS_H

#include "shared_global_p.h"

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtCore/QVariant>
#include <QtCore/QMap>
#include <QtGui/QMainWindow>

class QIcon;
class QPixmap;

namespace qdesigner_internal {
class ResourceMimeData;

QDESIGNER_SHARED_EXPORT void designerWarning(const QString &message);

class QDESIGNER_SHARED_EXPORT EnumType
{
public:
    typedef QMap<QString, QVariant> ItemMap;

    QString id() const;

    QVariant value;
    ItemMap items;
    QStringList names;
};


class QDESIGNER_SHARED_EXPORT FlagType
{
public:
    typedef QMap<QString, QVariant> ItemMap;

    QStringList flags() const;
    QString flagString() const;

    QVariant value;
    ItemMap items;
};

// Convenience to return a dropped icon, normalized to form directory
QDESIGNER_SHARED_EXPORT QIcon resourceMimeDataToIcon(const ResourceMimeData &rmd, QDesignerFormWindowInterface *fw);
// Convenience to return an dropped pixmap, normalized to form directory
QDESIGNER_SHARED_EXPORT QPixmap resourceMimeDataToPixmap(const ResourceMimeData &rmd, QDesignerFormWindowInterface *fw);

// Convenience to run UIC
enum UIC_Mode { UIC_GenerateCode, UIC_ConvertV3 };
QDESIGNER_SHARED_EXPORT bool runUIC(const QString &fileName, UIC_Mode mode, QByteArray& ba, QString &errorMessage);
} // namespace qdesigner_internal

Q_DECLARE_METATYPE(qdesigner_internal::EnumType)
Q_DECLARE_METATYPE(qdesigner_internal::FlagType)

namespace qdesigner_internal { namespace Utils {

inline int valueOf(const QVariant &value, bool *ok = 0)
{
    if (qVariantCanConvert<EnumType>(value))
        return qVariantValue<EnumType>(value).value.toInt(ok);
    else if (qVariantCanConvert<FlagType>(value))
        return qVariantValue<FlagType>(value).value.toInt(ok);

    return value.toInt(ok);
}

inline bool isObjectAncestorOf(QObject *ancestor, QObject *child)
{
    QObject *obj = child;
    while (obj != 0) {
        if (obj == ancestor)
            return true;
        obj = obj->parent();
    }
    return false;
}

inline bool isCentralWidget(QDesignerFormWindowInterface *fw, QWidget *widget)
{
    if (! fw || ! widget)
        return false;

    if (widget == fw->mainContainer())
        return true;

    // ### generalize for other containers
    if (QMainWindow *mw = qobject_cast<QMainWindow*>(fw->mainContainer())) {
        return mw->centralWidget() == widget;
    }

    return false;
}

} // namespace Utils

} // namespace qdesigner_internal

#endif // QDESIGNER_UTILS_H
