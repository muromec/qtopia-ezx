/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef RCC_H
#define RCC_H

#include <QStringList>
#include <QFileInfo>
#include <QLocale>
#include <QHash>
#include <QString>

#define TAG_RCC "RCC"
#define TAG_RESOURCE "qresource"
#define TAG_FILE "file"

#define ATTRIBUTE_LANG       QLatin1String("lang")
#define ATTRIBUTE_PREFIX     QLatin1String("prefix")
#define ATTRIBUTE_ALIAS      QLatin1String("alias")
#define ATTRIBUTE_THRESHOLD  QLatin1String("threshold")
#define ATTRIBUTE_COMPRESS   QLatin1String("compress")

#define CONSTANT_HEADER_SIZE 8
#define CONSTANT_COMPRESSLEVEL_DEFAULT -1
#define CONSTANT_COMPRESSTHRESHOLD_DEFAULT 70

struct RCCFileInfo;

class RCCResourceLibrary
{
public:
    inline RCCResourceLibrary();
    ~RCCResourceLibrary();

    bool output(FILE *out);

    bool readFiles(bool ignoreErrors=false);

    enum Format { Binary, C_Code };
    inline void setFormat(Format f) { mFormat = f; }
    inline Format format() const { return mFormat; }

    inline void setInputFiles(QStringList files) { mFileNames = files; }
    inline QStringList inputFiles() const { return mFileNames; }

    QStringList dataFiles() const;

    inline void setVerbose(bool b) { mVerbose = b; }
    inline bool verbose() const { return mVerbose; }

    inline void setInitName(const QString &n) { mInitName = n; }
    inline QString initName() const { return mInitName; }

    inline void setCompressLevel(int c) { mCompressLevel = c; }
    inline int compressLevel() const { return mCompressLevel; }

    inline void setCompressThreshold(int t) { mCompressThreshold = t; }
    inline int compressThreshold() const { return mCompressThreshold; }

    inline void setResourceRoot(QString str) { mResourceRoot = str; }
    inline QString resourceRoot() const { return mResourceRoot; }

private:
    RCCFileInfo *root;
    bool addFile(const QString &alias, const RCCFileInfo &file);
    bool interpretResourceFile(QIODevice *inputDevice, QString file, QString currentPath = QString(), bool ignoreErrors = false);

    bool writeHeader(FILE *out);
    bool writeDataBlobs(FILE *out);
    bool writeDataNames(FILE *out);
    bool writeDataStructure(FILE *out);
    bool writeInitializer(FILE *out);

    QStringList mFileNames;
    QString mResourceRoot, mInitName;
    Format mFormat;
    bool mVerbose;
    int mCompressLevel;
    int mCompressThreshold;
    int mTreeOffset, mNamesOffset, mDataOffset;
};

inline RCCResourceLibrary::RCCResourceLibrary()
{
    root = 0;
    mVerbose = false;
    mFormat = C_Code;
    mCompressLevel = -1;
    mCompressThreshold = 70;
    mTreeOffset = mNamesOffset = mDataOffset = 0;
}

struct RCCFileInfo
{
    enum Flags
    {
        NoFlags = 0x00,
        Compressed = 0x01,
        Directory = 0x02
    };

    inline RCCFileInfo(QString name = QString(), QFileInfo fileInfo = QFileInfo(),
                       QLocale::Language language = QLocale::C, 
                       QLocale::Country country = QLocale::AnyCountry,
                       uint flags = NoFlags,
                       int compressLevel = CONSTANT_COMPRESSLEVEL_DEFAULT, int compressThreshold = CONSTANT_COMPRESSTHRESHOLD_DEFAULT);
    ~RCCFileInfo() { qDeleteAll(children); }
    inline QString resourceName() {
        QString resource = name;
        for(RCCFileInfo *p = parent; p; p = p->parent)
            resource = resource.prepend(p->name + QLatin1Char('/'));
        return QLatin1Char(':') + resource;
    }

    int flags;
    QString name;
    QLocale::Language language;
    QLocale::Country country;
    QFileInfo fileInfo;
    RCCFileInfo *parent;
    QHash<QString, RCCFileInfo*> children;
    int mCompressLevel;
    int mCompressThreshold;

    qint64 nameOffset, dataOffset, childOffset;
    qint64 writeDataBlob(FILE *out, qint64 offset, RCCResourceLibrary::Format format);
    qint64 writeDataName(FILE *out, qint64 offset, RCCResourceLibrary::Format format);
    bool   writeDataInfo(FILE *out, RCCResourceLibrary::Format format);
};

inline RCCFileInfo::RCCFileInfo(QString name, QFileInfo fileInfo, QLocale::Language language, QLocale::Country country, uint flags,
                                int compressLevel, int compressThreshold)
{
    this->name = name;
    this->fileInfo = fileInfo;
    this->language = language;
    this->country = country;
    this->flags = flags;
    this->parent = 0;
    this->nameOffset = this->dataOffset = this->childOffset = 0;
    this->mCompressLevel = compressLevel;
    this->mCompressThreshold = compressThreshold;
}

#endif // RCC_H
