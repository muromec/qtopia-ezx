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

#ifndef QT_NO_DESKTOPSERVICES

#include <qprocess.h>
#include <qstringlist.h>
#include <qdir.h>
#include <qurl.h>
#include <qstringlist.h>
#include <private/qcore_mac_p.h>

/*
    Translates a QDesktopServices::Location into the mac equivalent.
*/
/*
OSType translateLocation(Location type)
{
    switch (type) {
    case QDesktopServices::Desktop:
        return kDesktopFolderType; break;

    case QDesktopServices::Documents:
        return kDocumentsFolderType; break;

    case QDesktopServices::Fonts:
        // There are at least two different font directories on the mac: /Library/Fonts and ~/Library/Fonts.
        // To select a specific one we have to specify a different first parameter when calling FSFindFolder.
        return kFontsFolderType; break;

    case QDesktopServices::Applications:
        return kApplicationsFolderType; break;

    case QDesktopServices::Music:
        return kMusicDocumentsFolderType; break;

    case QDesktopServices::Movies:
        return kMovieDocumentsFolderType; break;

    case QDesktopServices::Pictures:
        return kPictureDocumentsFolderType; break;


    case QDesktopServices::Temp:
        return kTemporaryFolderType; break;

    default:
        return kDesktopFolderType; break;
    }
}
*/
static bool lsOpen(const QUrl &url)
{
    if (!url.isValid())
        return false;

    QCFType<CFURLRef> cfUrl = CFURLCreateWithString(0, QCFString(QString::fromLatin1(url.toEncoded())), 0);
    if (cfUrl == 0)
        return false;

    const OSStatus err = LSOpenCFURLRef(cfUrl, 0);
    return (err == noErr);
}

static bool launchWebBrowser(const QUrl &url)
{
    return lsOpen(url);
}

static bool openDocument(const QUrl &file)
{
    if (!file.isValid())
        return false;

   // LSOpen does not work in this case, use QProcess open instead.
   return QProcess::startDetached(QLatin1String("open"), QStringList() << file.toLocalFile());
}

#if 0
/*
    Returns a QString given an HFSUniStr255.
*/
static QString qt_mac_hfsunistr_to_qstring(const HFSUniStr255 *hfs)
{
    const QChar *charPointer =  reinterpret_cast<const QChar*>(hfs->unicode);
    return QString(charPointer, hfs->length);
}

/*
    Returns wether the given fsRef is something valid.
*/
static Boolean FSRefIsValid(const FSRef &fsRef)
{
  return (FSGetCatalogInfo(&fsRef, kFSCatInfoNone, NULL, NULL, NULL, NULL) == noErr);
}

/*
    Constructs a full unicode path from a FSRef (I can't find a system function
    that does this.. )
*/
static QString getFullPath(FSRef ref)
{
    QString path;
    FSRef parent;
    do {
        HFSUniStr255 name;
        OSErr err = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, &name, NULL, &parent);
        if (err)
            return QString();

        if (FSRefIsValid(parent))
            path.prepend(qt_mac_hfsunistr_to_qstring(name) + "/");
        ref = parent;
    } while (FSRefIsValid(ref));

    path.prepend("/");
    return path;
}

QString QDesktopServices::storageLocation(const Location type)
{
     if (QDesktopServices::Home == type)
        return QDir::homePath();

     // http://developer.apple.com/documentation/Carbon/Reference/Folder_Manager/folder_manager_ref/chapter_1.4_section_7.htm
     FSRef ref;
     OSErr err = FSFindFolder(kOnAppropriateDisk, translateLocation(type), false, &ref);
     if (err)
        return QString();

     return getFullPath(ref);
}

QString QDesktopServices::displayName(const Location type)
{
    if (QDesktopServices::Home == type)
        return QObject::tr("Home");

    FSRef ref;
    OSErr err = FSFindFolder(kOnAppropriateDisk, translateLocation(type), false, &ref);
    if (err)
        return QString();

    QCFString displayName;
    err = LSCopyDisplayNameForRef(&ref, &displayName);
    if (err)
        return QString();

    return static_cast<QString>(displayName);
}
#endif


#endif // QT_NO_DESKTOPSERVICES

