/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qplatformdefs.h"

#include <qfile.h>
#include "qlibrary_p.h"
#include <qfileinfo.h>
#include <qcoreapplication.h>

#ifndef QT_NO_LIBRARY

#ifdef Q_OS_MAC
#  include <private/qcore_mac_p.h>
#endif

#if defined(QT_AOUT_UNDERSCORE)
#include <string.h>
#endif

#if defined(QT_HPUX_LD) // for HP-UX < 11.x and 32 bit

bool QLibraryPrivate::load_sys()
{
    pHnd = (void*)shl_load(QFile::encodeName(fileName), BIND_DEFERRED | BIND_NONFATAL | DYNAMIC_PATH, 0);
    if (pluginState != IsAPlugin) {
        if (!pHnd)
            pHnd = (void*)shl_load(QFile::encodeName(fileName + ".sl"), BIND_DEFERRED | BIND_NONFATAL | DYNAMIC_PATH, 0);
        if (!pHnd) {
            QFileInfo fi(fileName);
            int dlFlags = DYNAMIC_PATH | BIND_NONFATAL;
            if (loadHints & QLibrary::ResolveAllSymbolsHint) {
                dlFlags |= BIND_IMMEDIATE;
            } else {
                dlFlags |= BIND_DEFERRED;
            }
            pHnd = (void*)shl_load(QFile::encodeName(fi.path() + "/lib" + fi.fileName() + ".sl"),
                                   dlFlags, 0);
        }
    }
    if (!pHnd) {
        errorString = QLibrary::tr("QLibrary::load_sys: Cannot load %1 (%2)").arg(fileName).arg(QString());
    } else {
        errorString.clear();
    }
    return pHnd != 0;
}

bool QLibraryPrivate::unload_sys()
{
    if (shl_unload((shl_t)pHnd)) {
        errorString = QLibrary::tr("QLibrary::unload_sys: Cannot unload %1 (%2)").arg(fileName).arg(QString());
        return false;
    }
    errorString.clear();
    return true;
}

void* QLibraryPrivate::resolve_sys(const char* symbol)
{
    void* address = 0;
    if (shl_findsym((shl_t*)&pHnd, symbol, TYPE_UNDEFINED, &address) < 0) {
        errorString = QLibrary::tr("QLibrary::resolve_sys: Symbol \"%1\" undefined in %2 (%3)").arg(
            QString::fromAscii(symbol)).arg(fileName).arg(QString());
        address = 0;
    } else {
        errorString.clear();
    }
    return address;
}

#else // POSIX
#include <dlfcn.h>

static QString qdlerror()
{
    const char *err = dlerror();
    return err ? QString::fromLocal8Bit(err) : QString();
}

bool QLibraryPrivate::load_sys()
{
    QFileInfo fi(fileName);
    QString path = fi.path();
    QString name = fi.fileName();
    if (path == QLatin1String(".") && !fileName.startsWith(path))
        path.clear();
    else
        path += QLatin1Char('/');

    // The first filename we want to attempt to load is the filename as the callee specified.
    // Thus, the first attempt we do must be with an empty prefix and empty suffix.
    QStringList suffixes(QLatin1String("")), prefixes(QLatin1String(""));
    if (pluginState != IsAPlugin) {
        prefixes << QLatin1String("lib");
#if defined(Q_OS_HPUX)
        if (majorVerNum > -1) {
            suffixes << QString::fromLatin1(".sl.%1").arg(majorVerNum);
        } else {
            suffixes << QLatin1String(".sl");
        }
# if defined(__ia64)
        if (majorVerNum > -1) {
            suffixes << QString::fromLatin1(".so.%1").arg(majorVerNum);
        } else {
            suffixes << QLatin1String(".so");
        }
# endif
#elif defined(Q_OS_AIX)
        suffixes << ".a";
#else
        if (majorVerNum > -1) {
            suffixes << QString::fromLatin1(".so.%1").arg(majorVerNum);
        } else {
            suffixes << QLatin1String(".so");
        }
#endif
# ifdef Q_OS_MAC
        if (majorVerNum > -1) {
            suffixes << QString::fromLatin1(".%1.bundle").arg(majorVerNum);
            suffixes << QString::fromLatin1(".%1.dylib").arg(majorVerNum);
        } else {
            suffixes << QLatin1String(".bundle") << QLatin1String(".dylib");
        }
#endif
    }
    int dlFlags = 0;
    if (loadHints & QLibrary::ResolveAllSymbolsHint) {
        dlFlags |= RTLD_NOW;
    } else {
        dlFlags |= RTLD_LAZY;
    }
    if (loadHints & QLibrary::ExportExternalSymbolsHint) {
        dlFlags |= RTLD_GLOBAL;
    }
#if defined(Q_OS_AIX)	// Not sure if any other platform actually support this thing.
    if (loadHints & QLibrary::LoadArchiveMemberHint) {
        dlFlags |= RTLD_MEMBER;
    }
#endif
    QString attempt;
    for(int prefix = 0; !pHnd && prefix < prefixes.size(); prefix++) {
        for(int suffix = 0; !pHnd && suffix < suffixes.size(); suffix++) {
            if (!prefixes.at(prefix).isEmpty() && name.startsWith(prefixes.at(prefix)))
                continue;
            if (!suffixes.at(suffix).isEmpty() && name.endsWith(suffixes.at(suffix)))
                continue;
            if (loadHints & QLibrary::LoadArchiveMemberHint) {
                attempt = name;
                int lparen = attempt.indexOf(QLatin1Char('('));
                if (lparen == -1)
                    lparen = attempt.count();
                attempt = path + prefixes.at(prefix) + attempt.insert(lparen, suffixes.at(suffix));
            } else {
                attempt = path + prefixes.at(prefix) + name + suffixes.at(suffix);
            }
            pHnd = dlopen(QFile::encodeName(attempt), dlFlags);
        }
    }
#ifdef Q_OS_MAC
    if (!pHnd) {
        if (CFBundleRef bundle = CFBundleGetBundleWithIdentifier(QCFString(fileName))) {
            QCFType<CFURLRef> url = CFBundleCopyExecutableURL(bundle);
            QCFString str = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
            pHnd = dlopen(QFile::encodeName(str), dlFlags);
            attempt = str;
        }
    }
# endif
    if (!pHnd) {
        errorString = QLibrary::tr("QLibrary::load_sys: Cannot load %1 (%2)").arg(fileName).arg(qdlerror());
    }
    if (pHnd) {
        qualifiedFileName = attempt;
        errorString.clear();
    }
    return (pHnd != 0);
}

bool QLibraryPrivate::unload_sys()
{
    if (dlclose(pHnd)) {
        errorString = QLibrary::tr("QLibrary::unload_sys: Cannot unload %1 (%2)").arg(fileName).arg(qdlerror());
        return false;
    }
    errorString.clear();
    return true;
}

#ifdef Q_OS_MAC
Q_CORE_EXPORT void *qt_mac_resolve_sys(void *handle, const char *symbol)
{
    return dlsym(handle, symbol);
}
#endif

void* QLibraryPrivate::resolve_sys(const char* symbol)
{
#if defined(QT_AOUT_UNDERSCORE)
    // older a.out systems add an underscore in front of symbols
    char* undrscr_symbol = new char[strlen(symbol)+2];
    undrscr_symbol[0] = '_';
    strcpy(undrscr_symbol+1, symbol);
    void* address = dlsym(pHnd, undrscr_symbol);
    delete [] undrscr_symbol;
#else
    void* address = dlsym(pHnd, symbol);
#endif
    if (!address) {
        errorString = QLibrary::tr("QLibrary::resolve_sys: Symbol \"%1\" undefined in %2 (%3)").arg(
            QString::fromAscii(symbol)).arg(fileName).arg(qdlerror());
    } else {
        errorString.clear();
    }
    return address;
}

#endif // POSIX

#endif // QT_NO_LIBRARY
