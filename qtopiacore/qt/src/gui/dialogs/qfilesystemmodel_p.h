/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QFILESYSTEMMODEL_H
#define QFILESYSTEMMODEL_H

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

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qpair.h>
#include <QtCore/qdir.h>
#include <QtGui/qicon.h>

class ExtendedInformation;
class QFileSystemModelPrivate;
class QFileIconProvider;

#ifndef QT_NO_FILESYSTEMWATCHER

class Q_AUTOTEST_EXPORT QFileSystemModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool resolveSymlinks READ resolveSymlinks WRITE setResolveSymlinks)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool nameFilterDisables READ nameFilterDisables WRITE setNameFilterDisables)

Q_SIGNALS:
    void rootPathChanged(const QString &newPath);

public:
    enum Roles {
        FileIconRole = Qt::DecorationRole,
        FilePathRole = Qt::UserRole + 1,
        FileNameRole
    };

    explicit QFileSystemModel(QObject *parent = 0);
    ~QFileSystemModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(const QString &path, int column = 0) const;
    QModelIndex parent(const QModelIndex &child) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant myComputer(int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;

    // QFileSystemModel specific API
    QModelIndex setRootPath(const QString &path);
    QString rootPath() const;
    QDir rootDirectory() const;

    void setIconProvider(QFileIconProvider *provider);
    QFileIconProvider *iconProvider() const;

    void setFilter(QDir::Filters filters);
    QDir::Filters filter() const;

    void setResolveSymlinks(bool enable);
    bool resolveSymlinks() const;

    void setReadOnly(bool enable);
    bool isReadOnly() const;

    void setNameFilterDisables(bool enable);
    bool nameFilterDisables() const;

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    QString filePath(const QModelIndex &index) const;
    bool isDir(const QModelIndex &index) const;
    qint64 size(const QModelIndex &index) const;
    QString type(const QModelIndex &index) const;
    QDateTime lastModified(const QModelIndex &index) const;

    QModelIndex mkdir(const QModelIndex &parent, const QString &name);
    inline bool rmdir(const QModelIndex &index) { QDir dir; return dir.rmdir(filePath(index)); }
    inline QString fileName(const QModelIndex &index) const { return index.data(Qt::DisplayRole).toString(); }
    inline QIcon fileIcon(const QModelIndex &index) const { return qvariant_cast<QIcon>(index.data(Qt::DecorationRole)); }
    QFile::Permissions permissions(const QModelIndex &index) const;
    inline QFileInfo fileInfo(const QModelIndex &index) const { return QFileInfo(filePath(index)); }
    inline bool remove(const QModelIndex &index) { if (isDir(index)) return rmdir(index); else return QFile::remove(filePath(index)); }

protected:
    QFileSystemModel(QFileSystemModelPrivate &, QObject *parent = 0);
    void timerEvent(QTimerEvent *event);

private:
    Q_DECLARE_PRIVATE(QFileSystemModel)
    Q_DISABLE_COPY(QFileSystemModel)

    Q_PRIVATE_SLOT(d_func(), void _q_directoryChanged(const QString &directory, const QStringList &list))
    Q_PRIVATE_SLOT(d_func(), void _q_performDelayedSort())
    Q_PRIVATE_SLOT(d_func(), void _q_fileSystemChanged(const QString &path, const QList<QPair<QString, QFileInfo> > &))
    Q_PRIVATE_SLOT(d_func(), void _q_resolvedName(const QString &fileName, const QString &resolvedName))
};

#include <qabstractitemmodel.h>
#include <private/qabstractitemmodel_p.h>
#include "qfileinfogatherer_p.h"
#include <qdir.h>
#include <qicon.h>
#include <qfileinfo.h>
#include <qtimer.h>
#include <qhash.h>

class Q_AUTOTEST_EXPORT QFileSystemModelPrivate : public QAbstractItemModelPrivate
{
    Q_DECLARE_PUBLIC(QFileSystemModel)

public:
    class QFileSystemNode
    {
    public:
        QFileSystemNode(const QString &filename = QString(), QFileSystemNode *p=0)
            : fileName(filename), populatedChildren(false), parent(p), info(0) {}
        ~QFileSystemNode() { delete info; info = 0; }

        QString fileName;

        inline qint64 size() const { if (info && !info->isDir()) return info->size; return 0; }
        inline QString type() const { if (info) return info->displayType; return QLatin1String(""); }
        inline QDateTime lastModified() const { if (info) return info->lastModified; return QDateTime(); }
        inline QFile::Permissions permissions() const { if (info) return info->permissions; return 0; }
        inline bool isReadable() const { return ((permissions() & QFile::ReadUser) != 0); }
        inline bool isWritable() const { return ((permissions() & QFile::WriteUser) != 0); }
        inline bool isExecutable() const { return ((permissions() & QFile::ExeUser) != 0); }
        inline bool isDir() const {
            if (info)
                return info->isDir();
            if (children.count() > 0)
                return true;
            return false;
        }
        inline bool isFile() const { if (info) return info->isFile(); return true; }
        inline bool isSystem() const { if (info) return info->isSystem(); return true; }
        inline bool isHidden() const { if (info) return info->isHidden; return false; }
        inline bool isSymLink() const { if (info) return info->isSymLink; return false; }
        inline bool caseSensitive() const { if (info) return info->caseSensitive; return false; }
        inline QIcon icon() const { if (info) return info->icon; return QIcon(); }

        inline bool operator <(const QFileSystemNode &node) const {
            if (caseSensitive() || node.caseSensitive())
                return fileName < node.fileName;
            return fileName.toLower() < node.fileName.toLower();
        }
        inline bool operator >(const QString &name) const {
            if (caseSensitive())
                return fileName > name;
            return fileName.toLower() > name.toLower();
        }
        inline bool operator <(const QString &name) const {
            if (caseSensitive())
                return fileName < name;
            return fileName.toLower() < name.toLower();
        }
        inline bool operator !=(const QExtendedInformation &fileInfo) const {
            return !operator==(fileInfo);
        }
        bool operator ==(const QString &name) const {
            if (caseSensitive())
                return fileName == name;
            return fileName.toLower() == name.toLower();
        }
        bool operator ==(const QExtendedInformation &fileInfo) const {
            return info && (*info == fileInfo);
        }

        inline bool hasInformation() const { return info != 0; }

        void populate(const QExtendedInformation &fileInfo) {
            if (!info)
                info = new QExtendedInformation();
            (*info) = fileInfo;
        }

        // children shouldn't normally be accessed directly, use node()
        inline int visibleLocation(int childRow) {
            return visibleChildren.indexOf(childRow);
        }
        bool populatedChildren;
        QList<QFileSystemNode> children;
        QList<int> visibleChildren;
        QFileSystemNode *parent;

    private:
        QExtendedInformation *info;

    };

    QFileSystemModelPrivate() :
            forceSort(true),
            sortColumn(0),
            sortOrder(Qt::AscendingOrder),
            readOnly(true),
            setRootPath(false),
            filters(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs),
            nameFilterDisables(true) // false on windows, true on mac and unix
    {
        delayedSortTimer.setSingleShot(true);
    }

    void init();
    /*
      \internal

      Return true if index which is owned by node is hidden by the filter.
    */
    inline bool isHiddenByFilter(QFileSystemNode *indexNode, const QModelIndex &index) const
    {
       return (indexNode != &root && !index.isValid());
    }
    QFileSystemNode *node(const QModelIndex &index) const;
    QFileSystemNode *node(const QString &path, bool fetch = true) const;
    inline QModelIndex index(const QString &path) { return index(node(path)); }
    QModelIndex index(const QFileSystemNode *node) const;
    bool filtersAcceptsNode(const QFileSystemNode *node) const;
    bool passNameFilters(const QFileSystemNode *node) const;
    void removeNode(QFileSystemNode *parentNode, int itemLocation);
    int addNode(QFileSystemNode *parentNode, const QString &fileName);
    void addVisibleFiles(QFileSystemNode *parentNode, const QStringList &newFiles);
    void removeVisibleFile(QFileSystemNode *parentNode, int visibleLocation);
    void sortChildren(int column, Qt::SortOrder order, const QModelIndex &parent);

    inline int translateVisibleLocation(QFileSystemNode *parent, int row) const {
        return (sortOrder == Qt::AscendingOrder) ? row : parent->visibleChildren.count() - row - 1;
    }

    inline static QString myComputer() {
        // ### TODO We should query the system to find out what the string should be
        // XP == "My Computer",
        // Vista == "Computer",
        // OS X == "Computer" (sometime user generated) "Benjamin's PowerBook G4"
#ifdef Q_OS_WIN
        return QFileSystemModel::tr("My Computer");
#else
        return QFileSystemModel::tr("Computer");
#endif
    }

    inline void delayedSort() {
        if (!delayedSortTimer.isActive())
            delayedSortTimer.start(0);
    }

    static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
    {
       return s1.toLower() < s2.toLower();
    }

    static bool nodeCaseInsensitiveLessThan(const QFileSystemModelPrivate::QFileSystemNode &s1, const QFileSystemModelPrivate::QFileSystemNode &s2)
    {
       return s1.fileName.toLower() < s2.fileName.toLower();
    }

    inline int findChild(const QFileSystemNode *parent, const QFileSystemNode &node) const {
        QList<QFileSystemNode>::const_iterator iterator;
        if (parent->caseSensitive())
            iterator = (qBinaryFind(parent->children.begin(), parent->children.end(), node));
        else
            iterator = (qBinaryFind(parent->children.begin(), parent->children.end(), node, nodeCaseInsensitiveLessThan));
        if (iterator == parent->children.end())
            return -1;
        int location = (iterator - parent->children.begin());
        Q_ASSERT(location < parent->children.count());
        return location;
    }

    inline int findWhereToInsertChild(const QFileSystemNode *parent, const QFileSystemNode *node) const {
        QList<QFileSystemNode>::const_iterator iterator;
        if (parent->caseSensitive()) {
            iterator = (qUpperBound(parent->children.begin(), parent->children.end(), *node));
        } else
            iterator = (qUpperBound(parent->children.begin(), parent->children.end(), *node, QFileSystemModelPrivate::nodeCaseInsensitiveLessThan));
        return (iterator - parent->children.begin());
    }

    QIcon icon(const QModelIndex &index) const;
    QString name(const QModelIndex &index) const;
    QString filePath(const QModelIndex &index) const;
    QString size(const QModelIndex &index) const;
    static QString size(qint64 bytes);
    QString type(const QModelIndex &index) const;
    QString time(const QModelIndex &index) const;

    void _q_directoryChanged(const QString &directory, const QStringList &list);
    void _q_performDelayedSort();
    void _q_fileSystemChanged(const QString &path, const QList<QPair<QString, QFileInfo> > &);
    void _q_resolvedName(const QString &fileName, const QString &resolvedName);

    static int naturalCompare(const QString &s1, const QString &s2, Qt::CaseSensitivity cs);

    QDir rootDir;
#ifndef QT_NO_FILESYSTEMWATCHER
    QFileInfoGatherer fileInfoGatherer;
#endif
    QTimer delayedSortTimer;
    bool forceSort;
    int sortColumn;
    Qt::SortOrder sortOrder;
    bool readOnly;
    bool setRootPath;
    QDir::Filters filters;
    QList<const QFileSystemNode*> bypassFilters;
    bool nameFilterDisables;
#ifndef QT_NO_REGEXP
    QList<QRegExp> nameFilters;
#endif
    // ### Qt 5: resolvedSymLinks goes away
    QHash<QString, QString> resolvedSymLinks;

    QFileSystemNode root;

    QBasicTimer fetchingTimer;
    struct Fetching {
        QString dir;
        QString file;
        const QFileSystemNode *node;
    };
    QList<Fetching> toFetch;

};
#endif // QT_NO_FILESYSTEMWATCHER

#endif

