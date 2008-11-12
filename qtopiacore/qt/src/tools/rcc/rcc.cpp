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

#include "rcc.h"
#include <qdebug.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qbytearray.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qstack.h>
#include <qdom.h>

static bool qt_rcc_write_number(FILE *out, quint32 number, int width, RCCResourceLibrary::Format format)
{
    int dividend = 1;
    switch (width) {
    case 2:
        dividend = 256;
        break;
    case 3:
        dividend = 65536;
        break;
    case 4:
        dividend = 16777216;
        break;
    default:
        break;
    }

    // Write <width> bytes
    while (dividend >= 1) {
        const quint8 tmp = number / dividend;
        if(format == RCCResourceLibrary::Binary) {
            fwrite(&tmp, sizeof(tmp), 1, out);
        } else if(format == RCCResourceLibrary::C_Code) {
            fprintf(out, "0x%x", tmp);
            if (dividend > 1)
                fprintf(out, ",");
        }
        number -= tmp * dividend;
        dividend /= 256;
    }
    return true;
}

bool RCCFileInfo::writeDataInfo(FILE *out, RCCResourceLibrary::Format format)
{
    //some info
    if(format == RCCResourceLibrary::C_Code) {
        if(language != QLocale::C)
            fprintf(out, "  // %s [%d::%d]\n  ", resourceName().toLocal8Bit().constData(),
                    country, language);
        else
            fprintf(out, "  // %s\n  ", resourceName().toLocal8Bit().constData());
    }

    //pointer data
    if(flags & RCCFileInfo::Directory) {
        //name offset
        qt_rcc_write_number(out, nameOffset, 4, format);
        if(format == RCCResourceLibrary::C_Code)
            fprintf(out, ",");

        //flags
        qt_rcc_write_number(out, flags, 2, format);
        if(format == RCCResourceLibrary::C_Code)
            fprintf(out, ",");

        //child count
        qt_rcc_write_number(out, children.size(), 4, format);
        if(format == RCCResourceLibrary::C_Code)
            fprintf(out, ",");

        //first child offset
        qt_rcc_write_number(out, childOffset, 4, format);
    } else {
        //name offset
        qt_rcc_write_number(out, nameOffset, 4, format);
        if(format == RCCResourceLibrary::C_Code)
            fprintf(out, ",");

        //flags
        qt_rcc_write_number(out, flags, 2, format);
        if(format == RCCResourceLibrary::C_Code)
            fprintf(out, ",");

        //locale
        qt_rcc_write_number(out, country, 2, format);
        if(format == RCCResourceLibrary::C_Code)
            fprintf(out, ",");
        qt_rcc_write_number(out, language, 2, format);
        if(format == RCCResourceLibrary::C_Code)
            fprintf(out, ",");

        //data offset
        qt_rcc_write_number(out, dataOffset, 4, format);
    }
    if(format == RCCResourceLibrary::C_Code)
        fprintf(out, ",\n");
    return true;
}

qint64 RCCFileInfo::writeDataBlob(FILE *out, qint64 offset, RCCResourceLibrary::Format format)
{
    //capture the offset
    dataOffset = offset;

    //find the data to be written
    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QFile::ReadOnly)) {
        fprintf(stderr, "Couldn't open %s\n", fileInfo.absoluteFilePath().toLocal8Bit().constData());
        return false;
    }
    QByteArray data = file.readAll();

#ifndef QT_NO_COMPRESS
    // Check if compression is useful for this file
    if (mCompressLevel != 0 && data.size() != 0) {
        QByteArray compressed = qCompress(reinterpret_cast<uchar *>(data.data()), data.size(), mCompressLevel);

        int compressRatio = int(100.0f * (float(data.size() - compressed.size()) / float(data.size())));
        if (compressRatio >= mCompressThreshold) {
            data = compressed;
            flags |= Compressed;
        }
    }
#endif // QT_NO_COMPRESS

    //some info
    if(format == RCCResourceLibrary::C_Code)
        fprintf(out, "  // %s\n  ", fileInfo.absoluteFilePath().toLocal8Bit().constData());

    //write the length
    qt_rcc_write_number(out, data.size(), 4, format);
    if(format == RCCResourceLibrary::C_Code)
        fprintf(out, ",\n  ");
    offset += 4;

    //write the payload
    for (int i=0; i<data.size(); i++) {
        qt_rcc_write_number(out, data.at(i), 1, format);
        if(format == RCCResourceLibrary::C_Code) {
            fprintf(out, ",");
            if(!(i % 16))
                fprintf(out, "\n  ");
        }
    }
    offset += data.size();

    //done
    if(format == RCCResourceLibrary::C_Code)
        fprintf(out, "\n  ");
    return offset;
}

qint64 RCCFileInfo::writeDataName(FILE *out, qint64 offset, RCCResourceLibrary::Format format)
{
    //capture the offset
    nameOffset = offset;

    //some info
    if(format == RCCResourceLibrary::C_Code)
        fprintf(out, "  // %s\n  ", name.toLocal8Bit().constData());

    //write the length
    qt_rcc_write_number(out, name.length(), 2, format);
    if(format == RCCResourceLibrary::C_Code)
        fprintf(out, ",\n  ");
    offset += 2;

    //write the hash
    qt_rcc_write_number(out, qHash(name), 4, format);
    if(format == RCCResourceLibrary::C_Code)
        fprintf(out, ",\n  ");
    offset += 4;

    //write the name
    const QChar *unicode = name.unicode();
    for (int i=0; i<name.length(); i++) {
        qt_rcc_write_number(out, unicode[i].unicode(), 2, format);
        if(format == RCCResourceLibrary::C_Code) {
            fprintf(out, ",");
            if(!(i % 16))
                fprintf(out, "\n  ");
        }
    }
    offset += name.length()*2;

    //done
    if(format == RCCResourceLibrary::C_Code)
        fprintf(out, "\n  ");
    return offset;
}

RCCResourceLibrary::~RCCResourceLibrary()
{
    delete root;
}

bool RCCResourceLibrary::interpretResourceFile(QIODevice *inputDevice, QString fname, QString currentPath, bool ignoreErrors)
{
    if (!currentPath.isEmpty() && !currentPath.endsWith(QLatin1String("/")))
        currentPath += QLatin1Char('/');

    QDomDocument document;
    {
        QString errorMsg;
        int errorLine = 0, errorColumn = 0;
        if(!document.setContent(inputDevice, &errorMsg, &errorLine, &errorColumn)) {
            if(ignoreErrors)
                return true;
            fprintf(stderr, "RCC Parse Error: '%s' Line:%d Column:%d [%s]\n",
                    qPrintable(fname), errorLine, errorColumn,
                    qPrintable(errorMsg));
            return false;
        }
    }

    QDomElement root = document.firstChildElement(TAG_RCC).toElement();
    if (!root.isNull() && root.tagName() == QLatin1String(TAG_RCC)) {
        for (QDomNode node = root.firstChild(); !node.isNull(); node = node.nextSibling()) {
            if (!node.isElement())
                continue;

            QDomElement child = node.toElement();
            if (!child.isNull() && child.tagName() == QLatin1String(TAG_RESOURCE)) {
                QLocale::Language language = QLocale::c().language();
                QLocale::Country country = QLocale::c().country();

                if (child.hasAttribute(ATTRIBUTE_LANG)) {
                    QString attribute = child.attribute(ATTRIBUTE_LANG);
                    QLocale lang = QLocale(attribute);
                    language = lang.language();
                    if(2 == attribute.length()) {
                        // Language only
                        country = QLocale::AnyCountry;
                    } else {
                        country = lang.country();
                    }
                }

                QString prefix;
                if (child.hasAttribute(ATTRIBUTE_PREFIX))
                    prefix = child.attribute(ATTRIBUTE_PREFIX);
                if (!prefix.startsWith(QLatin1String("/")))
                    prefix.prepend(QLatin1Char('/'));
                if (!prefix.endsWith(QLatin1Char('/')))
                    prefix += QLatin1Char('/');

                for (QDomNode res = child.firstChild(); !res.isNull(); res = res.nextSibling()) {
                    if (res.isElement() && res.toElement().tagName() == QLatin1String(TAG_FILE)) {

                        QString fileName(res.firstChild().toText().data());
                        if (ignoreErrors && fileName.isEmpty())
                            fprintf(stderr, "RCC: Warning: Null node in XML\n");

                        QString alias;
                        if (res.toElement().hasAttribute(ATTRIBUTE_ALIAS))
                            alias = res.toElement().attribute(ATTRIBUTE_ALIAS);
                        else
                            alias = fileName;

                        int compressLevel = mCompressLevel;
                        if (res.toElement().hasAttribute(ATTRIBUTE_COMPRESS))
                            compressLevel = res.toElement().attribute(ATTRIBUTE_COMPRESS).toInt();
                        int compressThreshold = mCompressThreshold;
                        if (res.toElement().hasAttribute(ATTRIBUTE_THRESHOLD))
                            compressThreshold = res.toElement().attribute(ATTRIBUTE_THRESHOLD).toInt();

                        // Special case for -no-compress. Overrides all other settings.
                        if (mCompressLevel == -2)
                            compressLevel = 0;

                        alias = QDir::cleanPath(alias);
                        while (alias.startsWith(QLatin1String("../")))
                            alias.remove(0, 3);
                        alias = QDir::cleanPath(mResourceRoot) + prefix + alias;

                        QString absFileName = fileName;
                        if(QDir::isRelativePath(absFileName))
                            absFileName.prepend(currentPath);
                        QFileInfo file(absFileName);
                        if (!file.exists()) {
                            if(ignoreErrors)
                                continue;
                            fprintf(stderr, "RCC: Error: Cannot find file '%s'\n", qPrintable(fileName));
                            return false;
                        } else if (file.isFile()) {
                            addFile(alias, RCCFileInfo(alias.section(QLatin1Char('/'), -1), file, language, country,
                                                       RCCFileInfo::NoFlags, compressLevel, compressThreshold));
                        } else {
                            QDir dir;
                            if(file.isDir()) {
                                dir.setPath(file.filePath());
                            } else {
                                dir.setPath(file.path());
                                dir.setNameFilters(QStringList(file.fileName()));
                                if(alias.endsWith(file.fileName()))
                                    alias = alias.left(alias.length()-file.fileName().length());
                            }
                            if (!alias.endsWith(QLatin1String("/")))
                                alias += QLatin1Char('/');
                            QDirIterator it(dir, QDirIterator::FollowSymlinks|QDirIterator::Subdirectories);
                            while(it.hasNext()) {
                                it.next();
                                QFileInfo child(it.fileInfo());
                                if(child.fileName() != QLatin1String(".") && child.fileName() != QLatin1String(".."))
                                    addFile(alias + child.fileName(),
                                            RCCFileInfo(child.fileName(), child, language, country,
                                                        RCCFileInfo::NoFlags, compressLevel, compressThreshold));
                            }
                        }
                    }
                }
            }
        }
    }
    if(!ignoreErrors && this->root == 0)
        fprintf(stderr, "RCC: Warning: No resources in resource description.\n");
    return true;
}

bool RCCResourceLibrary::addFile(const QString &alias, const RCCFileInfo &file)
{
    if (file.fileInfo.size() > 0xffffffff) {
        fprintf(stderr, "File too big: %s",
                qPrintable(file.fileInfo.absoluteFilePath()));
        return false;
    }
    if(!root)
        root = new RCCFileInfo(QLatin1String(""), QFileInfo(), QLocale::C, QLocale::AnyCountry, RCCFileInfo::Directory);

    RCCFileInfo *parent = root;
    const QStringList nodes = alias.split(QLatin1Char('/'));
    for(int i = 1; i < nodes.size()-1; ++i) {
        const QString node = nodes.at(i);
        if(node.isEmpty())
            continue;
        if(!parent->children.contains(node)) {
            RCCFileInfo *s = new RCCFileInfo(node, QFileInfo(), QLocale::C, QLocale::AnyCountry, RCCFileInfo::Directory);
            s->parent = parent;
            parent->children.insert(node, s);
            parent = s;
        } else {
            parent = parent->children[node];
        }
    }

    const QString filename = nodes.at(nodes.size()-1);
    RCCFileInfo *s = new RCCFileInfo(file);
    s->parent = parent;
    parent->children.insertMulti(filename, s);
    return true;
}

bool RCCResourceLibrary::readFiles(bool ignoreErrors)
{
    //read in data
    if (mVerbose)
        fprintf(stderr, "Processing %d files [%d]\n", mFileNames.size(), ignoreErrors);
    for (int i=0; i<mFileNames.size(); ++i) {
        QFile fileIn;
        QString fname = mFileNames.at(i), pwd;
        if(fname == QLatin1String("-")) {
            fname = QLatin1String("(stdin)");
            pwd = QDir::currentPath();
            fileIn.setFileName(fname);
            if (!fileIn.open(stdin, QIODevice::ReadOnly)) {
                fprintf(stderr, "Unable to open file: %s\n", qPrintable(fname));
                return false;
            }
        } else {
            pwd = QFileInfo(fname).path();
            fileIn.setFileName(fname);
            if (!fileIn.open(QIODevice::ReadOnly)) {
                fprintf(stderr, "Unable to open file: %s\n", qPrintable(fname));
                return false;
            }
        }
        if (mVerbose)
            fprintf(stderr, "Interpreting %s\n", qPrintable(fname));

        if (!interpretResourceFile(&fileIn, fname, pwd, ignoreErrors))
            return false;
    }
    return true;
}

QStringList
RCCResourceLibrary::dataFiles() const
{
    QStringList ret;
    QStack<RCCFileInfo*> pending;

    if (!root)
        return ret;

    pending.push(root);
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for(QHash<QString, RCCFileInfo*>::iterator it = file->children.begin();
            it != file->children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
            ret.append(child->fileInfo.filePath());
        }
    }
    return ret;
}

bool RCCResourceLibrary::output(FILE *out)
{
    //write out
    if (mVerbose)
        fprintf(stderr, "Outputting code\n");
    if (!writeHeader(out)) {
        fprintf(stderr, "Couldn't write header\n");
        return false;
    }
    if(this->root) {
        if (!writeDataBlobs(out)) {
            fprintf(stderr, "Couldn't write data blob\n");
            return false;
        }
        if (!writeDataNames(out)) {
            fprintf(stderr, "Couldn't write file names\n");
            return false;
        }
        if (!writeDataStructure(out)) {
            fprintf(stderr, "Couldn't write data tree\n");
            return false;
        }
    }
    if (!writeInitializer(out)) {
        fprintf(stderr, "Couldn't write footer\n");
        return false;
    }
    return true;
}

bool
RCCResourceLibrary::writeHeader(FILE *out)
{
    if(mFormat == C_Code) {
        fprintf(out, "/****************************************************************************\n");
        fprintf(out, "** Resource object code\n");
        fprintf(out, "**\n");
        fprintf(out, "** Created: %s\n", qPrintable(QDateTime::currentDateTime().toString()));
        fprintf(out, "**      by: The Resource Compiler for Qt version %s\n", QT_VERSION_STR);
        fprintf(out, "**\n");
        fprintf(out, "** WARNING! All changes made in this file will be lost!\n");
        fprintf(out,  "*****************************************************************************/\n\n");
        fprintf(out, "#include <QtCore/qglobal.h>\n\n");
    } else if(mFormat == Binary) {
        fprintf(out,"qres");
        qt_rcc_write_number(out, 0, 4, mFormat);
        qt_rcc_write_number(out, 0, 4, mFormat);
        qt_rcc_write_number(out, 0, 4, mFormat);
        qt_rcc_write_number(out, 0, 4, mFormat);
    }
    return true;
}

bool
RCCResourceLibrary::writeDataBlobs(FILE *out)
{
    if(mFormat == C_Code)
        fprintf(out, "static const unsigned char qt_resource_data[] = {\n");
    else if(mFormat == Binary)
        mDataOffset = ftell(out);
    QStack<RCCFileInfo*> pending;

    if (!root)
        return false;

    pending.push(root);
    qint64 offset = 0;
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for(QHash<QString, RCCFileInfo*>::iterator it = file->children.begin();
            it != file->children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
            else
                offset = child->writeDataBlob(out, offset, mFormat);
        }
    }
    if(mFormat == C_Code)
        fprintf(out, "\n};\n\n");
    return true;
}

bool
RCCResourceLibrary::writeDataNames(FILE *out)
{
    if(mFormat == C_Code)
        fprintf(out, "static const unsigned char qt_resource_name[] = {\n");
    else if(mFormat == Binary)
        mNamesOffset = ftell(out);

    QHash<QString, int> names;
    QStack<RCCFileInfo*> pending;

    if (!root)
        return false;

    pending.push(root);
    qint64 offset = 0;
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for(QHash<QString, RCCFileInfo*>::iterator it = file->children.begin();
            it != file->children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
            if(names.contains(child->name)) {
                child->nameOffset = names.value(child->name);
            } else {
                names.insert(child->name, offset);
                offset = child->writeDataName(out, offset, mFormat);
            }
        }
    }
    if(mFormat == C_Code)
        fprintf(out, "\n};\n\n");
    return true;
}

static bool qt_rcc_compare_hash(const RCCFileInfo *left, const RCCFileInfo *right)
{
    return qHash(left->name) < qHash(right->name);
}

bool
RCCResourceLibrary::writeDataStructure(FILE *out)
{
    if(mFormat == C_Code)
        fprintf(out, "static const unsigned char qt_resource_struct[] = {\n");
    else if(mFormat == Binary)
        mTreeOffset = ftell(out);
    QStack<RCCFileInfo*> pending;

    if (!root)
        return false;

    //calculate the child offsets (flat)
    pending.push(root);
    int offset = 1;
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        file->childOffset = offset;

        //sort by hash value for binary lookup
        QList<RCCFileInfo*> children = file->children.values();
        qSort(children.begin(), children.end(), qt_rcc_compare_hash);

        //write out the actual data now
        for(int i = 0; i < children.size(); ++i) {
            RCCFileInfo *child = children.at(i);
            ++offset;
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }

    //write out the structure (ie iterate again!)
    pending.push(root);
    root->writeDataInfo(out, mFormat);
    while(!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();

        //sort by hash value for binary lookup
        QList<RCCFileInfo*> children = file->children.values();
        qSort(children.begin(), children.end(), qt_rcc_compare_hash);

        //write out the actual data now
        for(int i = 0; i < children.size(); ++i) {
            RCCFileInfo *child = children.at(i);
            child->writeDataInfo(out, mFormat);
            if(child->flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }
    if(mFormat == C_Code)
        fprintf(out, "\n};\n\n");

    return true;
}

bool
RCCResourceLibrary::writeInitializer(FILE *out)
{
    if(mFormat == C_Code) {
        QString initName = mInitName;
        if(!initName.isEmpty()) {
            initName.prepend(QLatin1Char('_'));
            initName.replace(QRegExp(QLatin1String("[^a-zA-Z0-9_]")), QLatin1String("_"));
        }

        //init
        fprintf(out, "int qInitResources%s()\n{\n", initName.toLatin1().constData());
        if(this->root) {
            fprintf(out, "    extern bool qRegisterResourceData(int, const unsigned char *, "
                    "const unsigned char *, const unsigned char *);\n");
            fprintf(out, "    qRegisterResourceData(0x01, qt_resource_struct, "
                    "qt_resource_name, qt_resource_data);\n");
        }
        fprintf(out, "    return 1;\n");
        fprintf(out, "}\n");
        fprintf(out, "Q_CONSTRUCTOR_FUNCTION(qInitResources%s)\n",
                initName.toLatin1().constData());

        //cleanup
        fprintf(out, "int qCleanupResources%s()\n{\n", initName.toLatin1().constData());
        if(this->root) {
            fprintf(out, "    extern bool qUnregisterResourceData(int, const unsigned char *, "
                    "const unsigned char *, const unsigned char *);\n");
            fprintf(out, "    qUnregisterResourceData(0x01, qt_resource_struct, "
                    "qt_resource_name, qt_resource_data);\n");
        }
        fprintf(out, "    return 1;\n");
        fprintf(out, "}\n");
        fprintf(out, "Q_DESTRUCTOR_FUNCTION(qCleanupResources%s)\n",
                initName.toLatin1().constData());
    } else if(mFormat == Binary) {
        const long old_pos = ftell(out);
        fseek(out, 4, SEEK_SET);
        qt_rcc_write_number(out, 0x01, 4, mFormat);
        qt_rcc_write_number(out, mTreeOffset, 4, mFormat);
        qt_rcc_write_number(out, mDataOffset, 4, mFormat);
        qt_rcc_write_number(out, mNamesOffset, 4, mFormat);
        fseek(out, old_pos, SEEK_SET);
    }
    return true;
}
