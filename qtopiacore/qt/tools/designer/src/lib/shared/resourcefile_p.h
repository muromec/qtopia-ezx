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

#ifndef RESOURCEFILE_H
#define RESOURCEFILE_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QAbstractItemModel>

#include "shared_global_p.h"

namespace qdesigner_internal {

class QDESIGNER_SHARED_EXPORT ResourceFile
{
public:
    ResourceFile(const QString &file_name = QString());

    void setFileName(const QString &file_name) { m_file_name = file_name; }
    QString fileName() const { return m_file_name; }
    bool load();
    bool save();
    QString errorMessage() const { return m_error_message; }

    QString resolvePath(const QString &path) const;

    QStringList prefixList() const;
    QStringList fileList(int pref_idx) const;

    int prefixCount() const;
    QString prefix(int idx) const;
    QString lang(int idx) const;
    
    int fileCount(int prefix_idx) const;
    QString file(int prefix_idx, int file_idx) const;
    QString alias(int prefix_idx, int file_idx) const;

    void addFile(int prefix_idx, const QString &file);
    void addPrefix(const QString &prefix);
    
    void removePrefix(int prefix_idx);
    void removeFile(int prefix_idx, int file_idx);
    
    void replacePrefix(int prefix_idx, const QString &prefix);
    void replaceLang(int prefix_idx, const QString &lang);
    void replaceAlias(int prefix_idx, int file_idx, const QString &alias);
    void replaceFile(int pref_idx, int file_idx, const QString &file);

    int indexOfPrefix(const QString &prefix) const;
    int indexOfFile(int pref_idx, const QString &file) const;
    
    bool contains(const QString &prefix, const QString &file = QString()) const;
    bool contains(int pref_idx, const QString &file) const;

    QString relativePath(const QString &abs_path) const;
    QString absolutePath(const QString &rel_path) const;

    static QString fixPrefix(const QString &prefix);
    bool split(const QString &path, QString *prefix, QString *file) const;

    bool isEmpty() const;

    struct File {
        File(const QString &_name = QString(), const QString &_alias = QString())
            : name(_name), alias(_alias) {}
        bool operator < (const File &other) const { return name < other.name; }
        bool operator == (const File &other) const { return name == other.name; }
        bool operator != (const File &other) const { return name != other.name; }
        QString name;
        QString alias;
    };
    typedef QList<File> FileList;
    struct Prefix {
        Prefix(const QString &_name = QString(), const QString &_lang = QString(), const FileList &_file_list = FileList())
            : name(_name), lang(_lang), file_list(_file_list) {}
        QString name;
        QString lang;
        FileList file_list;
    };
    typedef QList<Prefix> PrefixList;
private:
    PrefixList m_prefix_list;
    QString m_file_name;
    QString m_error_message;
};

class QDESIGNER_SHARED_EXPORT ResourceModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ResourceModel(const ResourceFile &resource_file, QObject *parent = 0);

    QModelIndex index(int row, int column,
                        const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QString fileName() const { return m_resource_file.fileName(); }
    void setFileName(const QString &file_name) { m_resource_file.setFileName(file_name); }
    void getItem(const QModelIndex &index, QString &prefix, QString &file) const;

    QString lang(const QModelIndex &index) const;
    QString alias(const QModelIndex &index) const;
    
    virtual QModelIndex addNewPrefix();
    virtual QModelIndex addFiles(const QModelIndex &idx, const QStringList &file_list);
    virtual void changePrefix(const QModelIndex &idx, const QString &prefix);
    virtual void changeLang(const QModelIndex &idx, const QString &lang);
    virtual void changeAlias(const QModelIndex &idx, const QString &alias);
    virtual QModelIndex deleteItem(const QModelIndex &idx);
    QModelIndex getIndex(const QString &prefix, const QString &file);
    QModelIndex getIndex(const QString &prefixed_file);
    QModelIndex prefixIndex(const QModelIndex &sel_idx) const;

    QString absolutePath(const QString &path) const
        { return m_resource_file.absolutePath(path); }
    QString relativePath(const QString &path) const
        { return m_resource_file.relativePath(path); }

    QString lastResourceOpenDirectory() const;

    virtual bool reload();
    virtual bool save();
    QString errorMessage() const { return m_resource_file.errorMessage(); }

    bool dirty() const { return m_dirty; }
    void setDirty(bool b);

    virtual QMimeData *mimeData (const QModelIndexList & indexes) const;

    static bool iconFileExtension(const QString &path);
    static QString resourcePath(const QString &prefix, const QString &file);

signals:
    void dirtyChanged(bool b);

private:
    ResourceFile m_resource_file;
    bool m_dirty;
    QString m_lastResourceDir;
};

} // namespace qdesigner_internal

#endif // RESOURCEFILE_H
