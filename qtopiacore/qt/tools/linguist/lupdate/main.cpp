/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Linguist of the Qt Toolkit.
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

#include "metatranslator.h"
#include "proparser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QCoreApplication>

#include <errno.h>
#include <string.h>
#include <QtCore/QDebug>

// defined in fetchtr.cpp
extern void fetchtr_cpp( const char *fileName, MetaTranslator *tor,
                         const char *defaultContext, bool mustExist, const QByteArray &codecForSource );

extern void fetchtr_java( const char *fileName, MetaTranslator *tor,
			  const char *defaultContext, bool mustExist, const QByteArray &codecForSource );

extern void fetchtr_ui( const char *fileName, MetaTranslator *tor,
                        const char *defaultContext, bool mustExist );

// defined in merge.cpp
extern void merge( const MetaTranslator *tor, const MetaTranslator *virginTor, MetaTranslator *out,
                   bool verbose, bool noObsolete );

typedef QList<MetaTranslatorMessage> TML;

static const char *g_defaultExtensions = "ui,c,c++,cc,cpp,cxx,ch,h,h++,hh,hpp,hxx";

static void printUsage()
{
    fprintf( stderr, "Usage:\n"
             "    lupdate [options] [project-file]\n"
             "    lupdate [options] [source-file|path]... -ts ts-files\n"
             "Options:\n"
             "    -help  Display this information and exit.\n"
             "    -noobsolete\n"
             "           Drop all obsolete strings.\n"
             "    -extensions <ext>[,<ext>]...\n"
             "           Process files with the given extensions only.\n"
             "           The extension list must be separated with commas, not with whitespace.\n"
             "           Default: '%s'.\n"
             "    -silent\n"
             "           Don't explain what is being done.\n"
             "    -version\n"
             "           Display the version of lupdate and exit.\n", g_defaultExtensions);
}

static void updateTsFiles( const MetaTranslator& fetchedTor,
                           const QStringList& tsFileNames, const QString& codecForTr,
						   bool noObsolete, bool verbose )
{
    QStringList::ConstIterator t = tsFileNames.begin();
    QDir dir;
    while ( t != tsFileNames.end() ) {
        QString fn = dir.relativeFilePath(*t);
        MetaTranslator tor;
        MetaTranslator out;
        tor.load( *t );
        if ( !codecForTr.isEmpty() ) {
            out.setCodec( codecForTr.toLatin1() );
            tor.setCodec( codecForTr.toLatin1() );
        }
        if ( verbose )
            fprintf( stderr, "Updating '%s'...\n", fn.toLatin1().constData() );
 
        merge( &tor, &fetchedTor, &out, verbose, noObsolete );
        if ( noObsolete )
            out.stripObsoleteMessages();
        out.stripEmptyContexts();
        
	    if ( !out.save(*t) ) {
#if defined(_MSC_VER) && _MSC_VER >= 1400
	        char buf[100];
	        strerror_s(buf, sizeof(buf), errno);
	        fprintf( stderr, "lupdate error: Cannot save '%s': %s\n",
                     fn.toLatin1().constData(), buf );
#else
            fprintf( stderr, "lupdate error: Cannot save '%s': %s\n",
                     fn.toLatin1().constData(), strerror(errno) );
#endif
	    }
        ++t;
    }
}

void recursiveFileInfoList(const QDir &dir, const QStringList &nameFilters, QDir::Filters filter, bool recursive, QFileInfoList *fileinfolist)
{
    if (recursive) filter |= QDir::AllDirs;
    QFileInfoList entries = dir.entryInfoList(nameFilters, filter);

    QFileInfoList::iterator it;
    for (it = entries.begin(); it != entries.end(); ++it) {
        QString fname = it->fileName();
        if (fname != QLatin1String(".") && fname != QLatin1String("..")) {
            if (it->isDir()) {
                recursiveFileInfoList(QDir(it->absoluteFilePath()), nameFilters, filter, recursive, fileinfolist);
            }else {
                fileinfolist->append(*it);
            }
        }
    }
}

int main( int argc, char **argv )
{
    QCoreApplication app(argc, argv);   
    
    QString defaultContext = QLatin1String("@default");
    MetaTranslator fetchedTor;
    QByteArray codecForTr;
    QByteArray codecForSource;
    QStringList tsFileNames;
    QStringList proFiles;
    QStringList sourceFiles;

    bool verbose = true; // verbose is on by default starting with Qt 4.2
    bool noObsolete = false;
    int numFiles = 0;
    bool standardSyntax = true;
    bool metTsFlag = false;

    QString extensions = QLatin1String(g_defaultExtensions);
    QStringList extensionsNameFilters;
    int i;

    for ( i = 1; i < argc; i++ ) {
        if ( qstrcmp(argv[i], "-ts") == 0 )
            standardSyntax = false;
    }

    QString oldDir = QDir::currentPath();

    for ( i = 1; i < argc; i++ ) {
        if ( qstrcmp(argv[i], "-help") == 0 ) {
            printUsage();
            return 0;
        } else if ( qstrcmp(argv[i], "-noobsolete") == 0 ) {
            noObsolete = true;
            continue;
        } else if ( qstrcmp(argv[i], "-silent") == 0 ) {
            verbose = false;
            continue;
        } else if ( qstrcmp(argv[i], "-verbose") == 0 ) {
            verbose = true;
            continue;
        } else if ( qstrcmp(argv[i], "-version") == 0 ) {
            fprintf( stderr, "lupdate version %s\n", QT_VERSION_STR );
            return 0;
        } else if ( qstrcmp(argv[i], "-ts") == 0 ) {
            metTsFlag = true;
            continue;
        } else if ( qstrcmp(argv[i], "-extensions") == 0 ) {
            ++i;
            if (i == argc) {
                fprintf( stderr, "The -extensions option should be followed by an extension list.");
                return 1;
            }
            extensions = QString(QLatin1String(argv[i]));
            continue;
        }

        numFiles++;
        
        QString fullText;

        if ( standardSyntax && !metTsFlag ) {
            QFile f( QString::fromLatin1(argv[i]) );
            if ( !f.open(QIODevice::ReadOnly) ) {
#if defined(_MSC_VER) && _MSC_VER >= 1400
				char buf[100];
				strerror_s(buf, sizeof(buf), errno);
				fprintf( stderr, "lupdate error: Cannot open file '%s': %s\n",
                         argv[i], buf );
#else
                fprintf( stderr, "lupdate error: Cannot open file '%s': %s\n",
                         argv[i], strerror(errno) );
#endif
                return 1;
            }
            f.close();
        }

        codecForTr.clear();
		codecForSource.clear();

        if (metTsFlag) {
            if ( QString::fromLatin1(argv[i]).endsWith(QLatin1String(".ts"), Qt::CaseInsensitive) 
                || QString::fromLatin1(argv[i]).endsWith(QLatin1String(".xlf"), Qt::CaseInsensitive)) {
                QFileInfo fi( QString::fromLatin1(argv[i]) );
                if ( !fi.exists() || fi.isWritable() ) {
                    tsFileNames.append( QFileInfo(QString::fromLatin1(argv[i])).absoluteFilePath() );
                } else {
                    fprintf( stderr,
                             "lupdate warning: For some reason, I cannot"
                             " save '%s'\n",
                             argv[i] );
                }
            } else {
                fprintf( stderr,
                         "lupdate error: File '%s' lacks .ts or .xlf extension\n",
                         argv[i] );
            }
        } else if (QString::fromLatin1(argv[i]).endsWith(QLatin1String(".pro"), Qt::CaseInsensitive)) {
            proFiles << QLatin1String(argv[i]);
        } else {
            QFileInfo fi(QString::fromLatin1(argv[i]));
            if (fi.isDir()) {
                if ( verbose ) fprintf(stderr, "Scanning directory '%s'...\n", argv[i]);
                QDir dir = QDir(fi.filePath());
                if (extensionsNameFilters.isEmpty()) {
                    extensions = extensions.trimmed();
                    // Remove the potential dot in front of each extension
                    if (extensions.startsWith(QLatin1Char('.')))
                        extensions.remove(0,1);
                    extensions.replace(QLatin1String(",."), QLatin1String(","));

                    extensions.insert(0, QLatin1String("*."));
                    extensions.replace(QLatin1Char(','), QLatin1String(",*."));
                    extensionsNameFilters = extensions.split(QLatin1Char(','));
                }
                QDir::Filters filters = QDir::Files | QDir::NoSymLinks;
                QFileInfoList fileinfolist;
                recursiveFileInfoList(dir, extensionsNameFilters, filters, true, &fileinfolist);
                QFileInfoList::iterator ii;
                QString fn;
                QDir baseDir(oldDir);
                for (ii = fileinfolist.begin(); ii != fileinfolist.end(); ++ii) {
                    // Make sure the path separator is stored with '/' in the ts file
                    fn = ii->canonicalFilePath().replace(QLatin1Char('\\'),QLatin1Char('/'));
#ifdef LINGUIST_DEBUG
                    fprintf(stderr, "%s\n", fn.data());
#endif
                    sourceFiles << fn;
                }
            }else{
                sourceFiles << fi.canonicalFilePath().replace(QLatin1Char('\\'),QLatin1Char('/'));
            }            
        }
    }   //for

    
    if ( proFiles.count() > 0 ) {
        proFiles = getListOfProfiles(proFiles, verbose);
    }
    bool firstPass = true;
    for (int pi = 0; firstPass || pi < proFiles.count(); ++pi) {
        QStringList tsFiles = tsFileNames;
        if (proFiles.count() > 0) {
            QString pf = proFiles.at(pi);
            QDir::setCurrent( QFileInfo(pf).path() );
            QMap<QByteArray, QStringList> variables;

            if(!evaluateProFile(QFileInfo(pf).fileName(), verbose, &variables))
                return 2;

            sourceFiles = variables.value("SOURCES");

            QStringList tmp = variables.value("CODECFORTR");
            if (!tmp.isEmpty()) {
                codecForTr = tmp.first().toAscii();
                fetchedTor.setCodecForTr(codecForTr.constData());
            }
            tmp = variables.value("CODECFORSRC");
            if (!tmp.isEmpty()) {
                codecForSource = tmp.first().toAscii();
            }

            tsFiles += variables.value("TRANSLATIONS");
        }

        for (QStringList::iterator it = sourceFiles.begin(); it != sourceFiles.end(); ++it) {
#ifdef LINGUIST_DEBUG
            qDebug() << "  " << (*it);
#endif
	    if ( (*it).endsWith(QLatin1String(".java"), Qt::CaseInsensitive) ) {
	        fetchtr_java( (*it).toAscii(), &fetchedTor, defaultContext.toAscii(), true, codecForSource );
	    }
            else if ( (*it).endsWith(QLatin1String(".ui"), Qt::CaseInsensitive) ) {
#ifdef LINGUIST_DEBUG
                qDebug() << "  " << (*it) + ".h";
#endif
                fetchtr_ui( (*it).toAscii(), &fetchedTor, defaultContext.toAscii(), true );
                fetchtr_cpp( QString((*it) + QLatin1String(".h")).toAscii(), &fetchedTor,
                             defaultContext.toAscii(), false, codecForSource );
            }else{
                fetchtr_cpp( (*it).toAscii(), &fetchedTor, defaultContext.toAscii(), true, codecForSource );
            }
        }

        removeDuplicates(&tsFiles, false);
        
        QDir::setCurrent( oldDir );
        if ( tsFiles.count() > 0) {
            updateTsFiles( fetchedTor, tsFiles, QString::fromLatin1(codecForTr.constData()), noObsolete, verbose );
        }
        firstPass = false;
    }
    QDir::setCurrent( oldDir );

    if ( numFiles == 0 ) {
        printUsage();
        return 1;
    }
    return 0;
}
