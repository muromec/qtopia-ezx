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

#include <qsettings.h>
#include <qdir.h>
#include <qurl.h>
#include <qstringlist.h>
#include <qprocess.h>
#include <qtemporaryfile.h>

#include <windows.h>
#include <shlobj.h>
#include <intshcut.h>

#ifndef QT_NO_DESKTOPSERVICES

//#undef UNICODE

static bool openDocument(const QUrl &file)
{
    if (!file.isValid())
        return false;

    QT_WA({
                ShellExecute(0, 0, (TCHAR *)file.toString().utf16(), 0, 0, SW_SHOWNORMAL);
            } , {
                ShellExecuteA(0, 0, file.toString().toLocal8Bit().constData(), 0, 0, SW_SHOWNORMAL);
            });

    return true;
}

static bool launchWebBrowser(const QUrl &url)
{
    if (url.scheme() == QLatin1String("mailto")) {
        //Retrieve the commandline for the default mail cleint
        //the key used below is the command line for the mailto: shell command
        long  bufferSize = 2*MAX_PATH;    
        long  returnValue =  -1;
        QString command;
        QT_WA ({
            wchar_t subKey[] = L"mailto\\shell\\open\\command";    
            wchar_t keyValue[2*MAX_PATH];    
            returnValue = RegQueryValue(HKEY_CLASSES_ROOT, subKey, keyValue, &bufferSize);
            if (!returnValue)
                command = QString::fromRawData((QChar*)keyValue, bufferSize);
        }, {
            char subKey[] = "mailto\\shell\\open\\command";    
            char keyValue[2*MAX_PATH];    
            returnValue = RegQueryValueA(HKEY_CLASSES_ROOT, subKey, keyValue, &bufferSize);
            if (!returnValue)
                command = QString::fromLocal8Bit(keyValue);
        });
        if(returnValue)
            return false;
        command = command.trimmed();
        //Make sure the path for the process is in quotes
        int index = -1 ;
        if (command[0]!= QLatin1Char('\"')) {
            index = command.indexOf(QLatin1String(".exe "), 0, Qt::CaseInsensitive);
            command.insert(index+4, QLatin1Char('\"'));
            command.insert(0, QLatin1Char('\"'));
        }
        //pass the url as the parameter
        index =  command.lastIndexOf(QLatin1String("%1"));
        if (index != -1){
            command.replace(index, 2, url.toString());
        }
        //start the process
        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));
        QT_WA ({
            STARTUPINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);

            returnValue = CreateProcess(NULL, (TCHAR*)command.utf16(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        }, {
            STARTUPINFOA si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);

            returnValue = CreateProcessA(NULL, command.toLocal8Bit().data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        });

        if (!returnValue)
            return false;

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    return openDocument(url);
}

/*
QString QDesktopServices::storageLocation(const Location type)
{
    QSettings settings(QSettings::UserScope, "Microsoft", "Windows");
    settings.beginGroup("CurrentVersion/Explorer/Shell Folders");
    switch (type) {
    case Desktop:
        return settings.value("Desktop").toString();
        break;

    case Documents:
        return settings.value("Personal").toString();
        break;

    case Fonts:
        return settings.value("Fonts").toString();
        break;

    case Applications:
        return settings.value("Programs").toString();
        break;

    case Music:
        return settings.value("My Music").toString();
        break;

    case Movies:
        return settings.value("My Video").toString();
        break;

    case Pictures:
        return settings.value("My Pictures").toString();
        break;

    case QDesktopServices::Home:
        return QDir::homePath(); break;

    case QDesktopServices::Temp:
        return QDir::tempPath(); break;

    default:
        break;
    }

    return QString();
}

QString QDesktopServices::displayName(const Location type)
{
    Q_UNUSED(type);
    return QString();
}
*/
#endif // QT_NO_DESKTOPSERVICES

