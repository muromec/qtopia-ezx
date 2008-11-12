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

#include <QFileInfo>
#include <QTextStream>
#include <QDir>

#include <qtopiasxe.h>

// requires vpath magic from qtopia build system
#include "qtransportauth_qws.h"
#include "qtransportauth_qws_p.h"

#include <qpackageregistry.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include <qdebug.h>
#include <qtopialog.h>

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

class SafeExecInstaller
{
public:
    SafeExecInstaller();
    ~SafeExecInstaller() {}
    int runScan();
    void setCallName( const QString &s ) { callName = s; }
    void setInstallRoot( const QString &s ) { progInfo.installRoot = s; }
    void setTarget( const QString & );
    void setDomainString( const QString &s ) { progInfo.domain = s; }
    void setRunDir();
    void usage( const QString & );
    QString domainString() { return progInfo.domain; }
    QString fileName(){ return progInfo.fileName; }
private:
    SxeProgramInfo progInfo;
    QString callName;
};

SafeExecInstaller::SafeExecInstaller()
    : callName( "sxe_installer" )
{
}

/*!
  \internal
  Set the target information for the install - this is the path
  to the target, relative to installDir, and its target name.

    eg:  "/bin/camera" -> "/bin/" - "camera"

  even though if quicklaunched it might be libcamera.so

  Note that the installRoot must be set before calling this
  method, so the install can be checked.
*/
void SafeExecInstaller::setTarget( const QString &s )
{
    Q_ASSERT( !progInfo.installRoot.isEmpty() );
    QFileInfo fi( progInfo.installRoot + s );
    progInfo.fileName = fi.fileName();
    progInfo.relPath = QFileInfo( s ).path();
    while( progInfo.relPath.startsWith( "/" ))
        progInfo.relPath.remove( 0, 1 );
    if ( !fi.exists() )
    {
        QFileInfo fi2( progInfo.installRoot + "/" + progInfo.relPath + "/lib" + progInfo.fileName + ".so" );
        if ( !fi2.exists() )
            usage( QString( "doesnt exist: target %1 ( %2 neither )" )
                .arg( fi.filePath() ).arg( fi2.filePath() ));
    }
}

/*!
  \internal

  Set the directory from which Qtopia will be run.  For a device build
  this is probably not going to be the same directory that the image is
  created in.

  This information is contained in the config.cache file created in the
  build directory.
*/
void SafeExecInstaller::setRunDir()
{
    int binDirPos = callName.lastIndexOf( "/bin/" );
    QString buildDir = callName.left( binDirPos );
    QFile configCache( buildDir + "/config.cache" );
    if ( !configCache.open( QIODevice::ReadOnly ))
    {
        qWarning( "\n\n***** sxe_installer could not open config.cache *****\n" );
        return;
    }
    QTextStream ts( &configCache );
    progInfo.runRoot.clear();
    while ( !ts.atEnd() && progInfo.runRoot.isEmpty() )
    {
        QString line = ts.readLine();
        if ( !line.startsWith( "opt.prefix.value=" ))
            continue;
        QStringList valuePair = line.split( "=" );
        progInfo.runRoot = valuePair[1];
    }
    if ( progInfo.runRoot.isEmpty() )
    {
        qWarning( "\n\n******* sxe_installer could not find \"opt.prefix.value=\" in config.cache *********\n" );
    }
}

/*!
  \internal

  The make install system - $QTOPIA_DEPOT_PATH/src/build/installs.prf is
  responsible for passing in the correct arguments in the correct order:

    domain.commands=$$QPEDIR/bin/sxe_installer $(PREFIX) $(INSTALL_ROOT) $${target.path}/$$TARGET "$${pkg.domain}"
*/
void SafeExecInstaller::usage( const QString &msg )
{
    qWarning() << "****** Safe exec install failed ********";
    qLog(SXE) << progInfo;
    qFatal( "%s ...exiting", qPrintable( msg ));
}

int SafeExecInstaller::runScan()
{
    if ( progInfo.installRoot.isEmpty() )
        usage( "install root not supplied" );
    if ( !QDir( progInfo.installRoot ).exists() )
        usage( QString( "install root %1 not a directory" ).arg( progInfo.installRoot ));
    if ( progInfo.domain.isEmpty() )
        progInfo.domain = "none";
    QPackageRegistry *p = QPackageRegistry::getInstance();
    int result = p->bootstrap( progInfo.installRoot );
    if ( result != 0 )
        return 1;
    p->registerBinary( progInfo );
    return 0;
}

int main( int argc, char** argv )
{
    if ( QSysInfo::ByteOrder == QSysInfo::BigEndian )
        qFatal( "Host tool sxe_installer does not currently support\n"
                "writing SXE database files on Big Endian architectures\n" );
    SafeExecInstaller se;
    if ( argc != 4 )
        se.usage( "wrong number of arguments" );
    se.setCallName( argv[0] );
    se.setInstallRoot( argv[1] );
    se.setTarget( argv[2] );
    se.setDomainString( argv[3] );

    if  ( se.fileName() != "qpe" && se.domainString() != "trusted" )
        qFatal( "ERROR: %s does not declare the domain: trusted\n"
                "All pre-installed programs must declare the trusted domain "
                "in their project(.pro) file", argv[0] );
    else if ( se.fileName() == "qpe" && se.domainString() != "qpe" )
        qFatal( "ERROR: qpe must declare the domain: qpe\n"
                "inside it's project(.pro) file" );
        //Note: qpe is a special case it must declare the qpe domain
        //all other pre-installed programs must declare trusted domain

    se.setRunDir();
    return se.runScan();
}
