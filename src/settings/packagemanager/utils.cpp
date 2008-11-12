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

#include "utils.h"

#include <QFileSystem>
#include <Qtopia>
#include <QRegExp>
#include <math.h>
#include <QProcess>
#include <QDebug>

bool SizeUtils::isSufficientSpace(qlonglong size, QString &neededSpace)
{
    QFileSystem packagesFs = QFileSystem::fromFileName( Qtopia::packagePath() );
    qlonglong avail = (qlonglong)packagesFs.blockSize() * packagesFs.availBlocks();
    
    if ( avail < size )
    {
        neededSpace = getSizeString(size - avail);
        return false;
    }
    return true;
}

QString SizeUtils::getSizeString( qlonglong size )
{
    double s = size;
    double scaledSize = s;
    QString suffix;
    bool skip = false;
    if ( s < 0 ) s = 0;
    if ( s < 1024 ) {
        suffix = QObject::tr("B","bytes");
        scaledSize = s;
        skip = true;
    }
    s /= 1024;
    if ( skip == false && s < 1024 ) {
        suffix = QObject::tr("KB","kilobytes");
        scaledSize = s;
        skip = true;
    }
    s /= 1024;
    if ( skip == false && s < 1024 ) {
        suffix = QObject::tr("MB","megabytes");
        scaledSize = s;
        skip = true;
    }
    s /= 1024;
    if ( skip == false && s < 1024 ) {
        suffix = QObject::tr("GB","gigabytes");
        scaledSize = s;
        skip = true;
    }
    return QString().sprintf("%0.2f",scaledSize ) + suffix;

}

qlonglong SizeUtils::parseInstalledSize( QString installedSize )
{
    QRegExp rx("^(\\d+\\.?\\d+)([km]?)$");
    installedSize = installedSize.trimmed().toLower();
    long multiplier;
    if ( rx.indexIn(installedSize) !=0 )
        return -1;

    QStringList captures = rx.capturedTexts();
    if ( !captures.count() == 3 )
        return -1;

    if ( captures[2].isEmpty()  )
        multiplier = 1;
    else if ( captures[2] == "k" )
        multiplier = 1024;
    else if ( captures[2] == "m" )
        multiplier = 1024 * 1024;
    else
        return -1;

    bool ok = false;
    double d = captures[1].toDouble( &ok );
    if ( !ok )
        return -1;

    double rounded = round( d );
    if ( floor(d) == rounded )
        rounded = rounded + 0.5;
    return (qlonglong)(rounded * multiplier) ;
}

bool LidsUtils::isLidsEnabled()
{
    return QFile::exists("/proc/sys/lids/locks");
}


/*
    Returns true if the maximum number of lids rules has been exceeded
*/
/*
    Implementation note: having a single number as the maximum number of
    lids rules is major simplification of how lids works.  Lids has
    limits on the number of subjects, number of objects, number of
    protected inodes.  For simplicity sake we use a single (configuration)
    value based on constants in the lids patch:
    CONFIG_LIDS_MAX_INODE
    CONFIG_LIDS_MAX_SACL
    CONFIG_LIDS_MAX_OACL
*/
bool LidsUtils::maxRulesExceeded()
{
    static int maxLidsRules=-1;

    if (maxLidsRules == -1)
    {
        QSettings conf( "Trolltech", "PackageManager" );
        conf.beginGroup("Configuration");
        bool b;
        maxLidsRules = conf.value("MaxLidsRules").toInt(&b);
        if (!b || maxLidsRules < 0)
        {
            maxLidsRules = 512;
            qWarning()  << "Invalid/Missing MaxLidsRules field in PackageManager settings file, "
                        << "using default value of " << maxLidsRules;
        }
    }

    QProcess lidsconf;
    lidsconf.start("lidsconf -L");
    if (lidsconf.waitForFinished())
    {
        QStringList output(QString(lidsconf.readAll()).split("\n"));
        if(!output.contains ("Killed"))
        {
            if ((output.count() - 5) > maxLidsRules)
                return true;
            else
                return false;
        }
    }

    //execution shouldn't get here
    //but return true to be on the "safe side"
    //(and not allow extra packages to be installed)
    return true;
}

namespace ScriptRunner
{
    void runScript(const QString &cmd)
    {
        QProcess process;
        QEventLoop eventLoop;
        QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)),
                    &eventLoop, SLOT(quit()));
        QObject::connect(&process, SIGNAL(error(QProcess::ProcessError)),
                    &eventLoop, SLOT(quit()));
        process.start(cmd);
        eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
    }
}
