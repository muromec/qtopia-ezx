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
#ifndef __storage_h__
#define __storage_h__

#include <qtopiaglobal.h>
#include <qobject.h>
#include <qlist.h>
#include <QSharedDataPointer>

class QFileSystem;
class QFileSystemFilter;
class QtopiaChannel;

#if defined (QTOPIA_TEMPLATEDLL)
//MOC_SKIP_BEGIN
template class QTOPIABASE_EXPORT QList<QFileSystem*>;
//MOC_SKIP_END
#endif

class QStorageMetaInfoPrivate;

class QTOPIABASE_EXPORT QStorageMetaInfo : public QObject
{
    Q_OBJECT
public:
    explicit QStorageMetaInfo( QObject *parent=0 );
    ~QStorageMetaInfo();

    QList<QFileSystem*> fileSystems( QFileSystemFilter *filter, bool connectedOnly=true );
    const QFileSystem *fileSystemOf( const QString &filename, bool connectedOnly=true );
    const QFileSystem *applicationsFileSystem();
    const QFileSystem *documentsFileSystem();
    QStringList fileSystemNames( QFileSystemFilter *filter, bool connectedOnly=true ); // libqtopia
    QString cardInfoString();
    QString installLocationsString();

    static QStorageMetaInfo *instance();

signals:
    void disksChanged();

public slots:
    void update();
    void update(QString& mountpoint, bool connected);

private slots:
    void cardMessage( const QString& msg, const QByteArray& data );
private:
    QStorageMetaInfoPrivate *d;

    QString infoString( QList<QFileSystem*> filesystems, const QString &extension );
};

class QFileSystemPrivate;

class QTOPIABASE_EXPORT QFileSystem
{
public:

    QFileSystem();
    QFileSystem( const QFileSystem &other );
    ~QFileSystem();

    QFileSystem &operator =( const QFileSystem &other );

    bool isNull() const;

    const QString &disk() const;
    const QString &path() const;
    const QString &prevPath() const;
    const QString &options() const;
    const QString &name() const;
    const QString &documentsPath() const;
    const QString &applicationsPath() const;
    bool isRemovable() const;
    bool applications() const;
    bool documents() const;
    bool contentDatabase() const;
    bool isConnected() const;

    long blockSize() const;
    long totalBlocks() const;
    long availBlocks() const;

    bool isWritable() const;

    static QFileSystem fromFileName( const QString &fileName, bool connectedOnly = true );
    static QFileSystem documentsFileSystem();
    static QFileSystem applicationsFileSystem();

private:
    QSharedDataPointer < QFileSystemPrivate > d;

    QFileSystem( const QString &disk, const QString &path, const QString &prevPath, const QString &options,
                 const QString &name, const QString &documentsPath, const QString &applicationsPath, bool removable,
                 bool applications, bool documents, bool contentDatabase, bool connected);

    friend class QStorageMetaInfo;

    void update();
    void update( bool connected, const QString &path );
};

class QTOPIABASE_EXPORT QFileSystemFilter
{
public:
    enum FilterOption { Either, Set, NotSet };

    QFileSystemFilter();
    virtual ~QFileSystemFilter();

    virtual bool filter( QFileSystem *fs );

    FilterOption documents;
    FilterOption applications;
    FilterOption removable;
    FilterOption content;
};


#endif
