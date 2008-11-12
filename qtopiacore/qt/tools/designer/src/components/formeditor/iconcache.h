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

#ifndef ICONCACHE_H
#define ICONCACHE_H

#include "formeditor_global.h"

#include <QtDesigner/QDesignerIconCacheInterface>

#include <resourcefile_p.h>

#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtCore/qdebug.h>

#include <QtGui/QPixmap>
#include <QtGui/QIcon>

namespace qdesigner_internal {

/*
    We need two caches - one for icons and one for pixmaps - which are in all
    other respects identical.
*/
template <typename Item>
class ResourceCache
{
public:
    Item keyToItem(const QString &path, const QString &resourcePath = QString());
    QString itemToFilePath(const Item &item) const;
    QString itemToQrcPath(const Item &item) const;

    QList<Item> itemList() const;

private:
    typedef QPair<QString, QString> Key;
    typedef QMap<Key, Item> KeyToItemMap;
    typedef QMap<int, Key> SerialToKeyMap;

    KeyToItemMap m_key_to_item;
    SerialToKeyMap m_serial_to_key;
};

template <typename Item>
Item ResourceCache<Item>::keyToItem(const QString &filePath, const QString &qrcPath)
{
    Key key = qMakePair(filePath, qrcPath);
    typename KeyToItemMap::const_iterator it = m_key_to_item.constFind(key);
    if (it != m_key_to_item.constEnd())
        return *it;

    QString real_path;
    if (!qrcPath.isEmpty()) {
        ResourceFile rf(qrcPath);
        if (rf.load()) {
            real_path = rf.resolvePath(filePath);
        } else {
            qWarning() <<  QObject::tr("The icon specified by %1 could not be opened: %2").arg(qrcPath).arg(rf.errorMessage());
        }
    } else {
       real_path = filePath;
    }

    if (real_path.isEmpty())
        return Item();

    Item item(real_path);
    if (item.isNull())
        return Item();
    m_key_to_item.insert(key, item);
    m_serial_to_key.insert(item.serialNumber(), key);

    return item;
}

template <typename Item>
QString ResourceCache<Item>::itemToFilePath(const Item &item) const
{
    typename SerialToKeyMap::const_iterator it = m_serial_to_key.find(item.serialNumber());
    if (it != m_serial_to_key.end())
        return (*it).first;
    return QString();
}

template <typename Item>
QString ResourceCache<Item>::itemToQrcPath(const Item &item) const
{
    typename SerialToKeyMap::const_iterator it = m_serial_to_key.find(item.serialNumber());
    if (it != m_serial_to_key.end())
        return (*it).second;
    return QString();
}

template <typename Item>
QList<Item> ResourceCache<Item>::itemList() const
{
    return m_key_to_item.values();
}

class QT_FORMEDITOR_EXPORT IconCache : public QDesignerIconCacheInterface
{
    Q_OBJECT
public:
    IconCache(QObject *parent);

    virtual QIcon nameToIcon(const QString &path, const QString &resourcePath = QString());
    virtual QString iconToFilePath(const QIcon &pm) const;
    virtual QString iconToQrcPath(const QIcon &pm) const;
    virtual QPixmap nameToPixmap(const QString &path, const QString &resourcePath = QString());
    virtual QString pixmapToFilePath(const QPixmap &pm) const;
    virtual QString pixmapToQrcPath(const QPixmap &pm) const;

    virtual QList<QPixmap> pixmapList() const;
    virtual QList<QIcon> iconList() const;

    virtual QString resolveQrcPath(const QString &filePath, const QString &qrcPath, const QString &workingDirectory = QString()) const;

private:
    ResourceCache<QIcon> m_icon_cache;
    ResourceCache<QPixmap> m_pixmap_cache;
};

}  // namespace qdesigner_internal

#endif // ICONCACHE_H
