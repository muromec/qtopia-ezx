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

#include <QApplication>
#include <QSettings>
#include <QDebug>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QTextCodec>

#include <qtopiasql.h>
#include <qcontent.h>
#include <qtopianamespace.h>
#include <qtopialog.h>
#include "../dbmigrate/migrateengine.h"
#include <QCategoryManager>

#include <qdebug.h>

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

// because we can't guarantee cleanup by glibc's termination routines.
#define USE_EVIL_SIGNAL_HANDLERS

class QSystemSemaphore
{
public:
    QSystemSemaphore(const QString& fpath,  int n = 1);
    ~QSystemSemaphore();
    void acquire();
    void release();
    int available();
    bool tryAcquire();
    bool tryAcquire(int timeout);
    bool isValid() {return semdata != SEM_FAILED;}
    static bool clear(const QString &fpath);
    
private:
    sem_t *semdata;
#ifdef USE_EVIL_SIGNAL_HANDLERS
    static QList<sem_t *> semlist;
    friend void cleanUpSemaphoreSignalHandler(int sig);
#endif
    static QString mungePath(const QString& in);
};

#ifdef USE_EVIL_SIGNAL_HANDLERS
QList<sem_t *> QSystemSemaphore::semlist;

int signalList[14] = {
    SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGKILL,
    SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2, SIGBUS
};


typedef void (*sighandler_t)(int);
sighandler_t oldHandlers[sizeof(signalList)/sizeof(int)];


void cleanUpSemaphoreSignalHandler(int sig)
{
    foreach(sem_t *current, QSystemSemaphore::semlist)
        sem_post(current);
    for ( uint i = 0; i < sizeof(signalList)/sizeof(int); i++ )
        if ( signalList[i] == sig ) {
            if ( oldHandlers[i] )
                oldHandlers[i](sig);
            else
                abort();
        }
}
#endif // USE_EVIL_SIGNAL_HANDLERS

QSystemSemaphore::QSystemSemaphore(const QString& fpath, int n)
{
#ifdef USE_EVIL_SIGNAL_HANDLERS
    if(semlist.count() == 0) {
        for ( uint i = 0; i < sizeof(signalList)/sizeof(int); i++ ) {
            struct sigaction act;
            struct sigaction oldact;
            act.sa_handler = cleanUpSemaphoreSignalHandler;
            act.sa_flags = SA_ONESHOT | SA_ONSTACK;
            if ( sigaction(i, &act, &oldact) == 0 )
                oldHandlers[i] = oldact.sa_handler;
            else
                oldHandlers[i] = 0;
        }
    }
#endif // USE_EVIL_SIGNAL_HANDLERS
    
    semdata = sem_open(mungePath(fpath).toLocal8Bit(), 0);
    if(semdata == SEM_FAILED && errno == ENOENT)
    {
        semdata = sem_open(mungePath(fpath).toLocal8Bit(), O_CREAT, S_IRWXU, n);
        if(semdata == SEM_FAILED)
            qWarning() << "sem_open returned error(" << errno << ":" << strerror(errno) << ")";
    }
}

QSystemSemaphore::~QSystemSemaphore()
{
    if(semdata != SEM_FAILED)
    {
        if(sem_close(semdata) == -1)
            qWarning() << "sem_close returned error(" << errno << ":" << strerror(errno) << ")";
        // may need to test for/provide semaphore unlinking.
    }
}

void QSystemSemaphore::acquire()
{
    if(semdata != SEM_FAILED)
    {
        if(sem_wait(semdata) == -1)
            qWarning() << "sem_wait returned error(" << errno << ":" << strerror(errno) << ")";
#ifdef USE_EVIL_SIGNAL_HANDLERS
        else
            semlist.append(semdata);
#endif
    }
}

void QSystemSemaphore::release()
{
    if(semdata != SEM_FAILED)
    {
        if(sem_post(semdata) == -1)
            qWarning() << "sem_post returned error(" << errno << ":" << strerror(errno) << ")";
#ifdef USE_EVIL_SIGNAL_HANDLERS
        else
            semlist.removeAll(semdata);
#endif
    }
}

int QSystemSemaphore::available()
{
    int result=-1;
    if(semdata != SEM_FAILED)
    {
        if(sem_getvalue(semdata, &result) != -1)
            result = true;
        else
            qWarning() << "sem_getvalue returned error(" << errno << ":" << strerror(errno) << ")";
    }
    return result;
}

bool QSystemSemaphore::tryAcquire()
{
    bool result=false;
    if(semdata != SEM_FAILED)
    {
        if(sem_trywait(semdata) != -1)
            result = true;
        else
            qWarning() << "sem_trywait returned error(" << errno << ":" << strerror(errno) << ")";
    }
    return result;
}

bool QSystemSemaphore::tryAcquire(int timeout)
{
    bool result=false;
    if(semdata != SEM_FAILED)
    {
        timeval tv;
        timespec tstimeout;

        if(gettimeofday(&tv, NULL) != 0)
        {
            qWarning() << "gettimeofday returned error(" << errno << ":" << strerror(errno) << ")";
        }
        
        tstimeout.tv_sec = tv.tv_sec + timeout/1000;
        tstimeout.tv_nsec = tv.tv_usec * 1000 + (timeout%1000)*1000000L;
        
        if(sem_timedwait(semdata, &tstimeout) != -1)
            result = true;
        else
            qWarning() << "sem_timedwait returned error(" << errno << ":" << strerror(errno) << ")";
    }
    return result;
}

bool QSystemSemaphore::clear(const QString& fpath)
{
    if ( sem_unlink(mungePath(fpath).toLocal8Bit()) != -1 )
        return true;
    if ( errno == ENOENT ) {
        return true;
    } else {
        qWarning() << "sem_unlink returned error(" << errno << ":" << strerror(errno) << ")";
        return false;
    }
}

QString QSystemSemaphore::mungePath(const QString& in)
{
    // if you're curious about the below munging, see https://www.redhat.com/archives/phil-list/2003-January/msg00113.html
    QString mungedpath;
    if(in[0] != '/')
        mungedpath=in;
    else
        mungedpath=in.right(in.size()-1);
    mungedpath.replace(QString('/'), QString('_'));
    mungedpath = '/'+mungedpath;
    return mungedpath;
}

void sendQcop(const QString prefixPath, const QString dbPath, uint cId, int type)
{
    QFileInfo fi(prefixPath + "/bin/qcop");
    if (!fi.exists())
        return;

    QStringList args;
    args << "enum" << "QContent::ChangeType" << "--"
         << "send" << "QPE/DocAPI" << "contentChanged(QContentIdList,QContent::ChangeType)"
         << QString::number(qHash(dbPath)) + ':' + QString::number(cId) << QString::number(type);
    QProcess::execute(prefixPath + "/bin/qcop", args);
}

int main( int argc, char** argv )
{
    setenv( "LANG", "en_US.utf8", 1 ); //Force LANG to UTF-8 so correct bytes are written to the database.

    QApplication app(argc, argv, false);
    QStringList args = app.arguments();
    QString appName = args.takeFirst();
    bool removeFlag = false;
    bool qcopFlag = false;
    if ( args.count() && args.at(0) == "-remove" ) {
        removeFlag = true;
        args.removeFirst();
    }
    if (args.count() && args.at(0) == "-qcop") {
        qcopFlag = true;
        args.removeFirst();
    }
    if ( args.count() >= 2 && args.at(0) == "-clearlocks" ) {
        QSystemSemaphore::clear(args.at(1));
        return 0;
    }
    if ( args.count() < 5  ) {
        qWarning() << endl
                   << "Usage: " << QFileInfo(appName).baseName().toLocal8Bit().constData()
                                << "[-remove] [-qcop]"
                                << "database prefix destpath \"categories\" file/s" << endl
                   << "Usage: " << QFileInfo(appName).baseName().toLocal8Bit().constData()
                                << "-clearlocks database" << endl
                   << endl
                   << ".directory files can be passed to register system categories." << endl
                   << endl
                   << ".desktop files register content entries for apps or other files." << endl
                   << "Note that categories are ignored for .desktop files (edit the file" << endl
                   << "itself to add more categories)." << endl
                   << endl
                   << "Any other files will be added as regular files that are available" << endl
                   << "to apps but not visible in the Documents tab." << endl
                   << endl
                   << "Pass -remove to remove an existing content item from the database." << endl
                   << endl
                   << "Pass -qcop to inform running qtopia instances that the content item has changed." << endl
                   << endl
                   << "Pass -clearlocks to remove the system wide semaphore for a database" << endl
                   << "     (useful if content_installer is interrupted or crashes)." << endl;
        return 1;
    }

    QString dbPath( args.takeFirst() );
    QSystemSemaphore semaphore(dbPath);
    if(!semaphore.isValid())
    {
        QString errmsg = "Semaphore creation failed: Error " + QString::number(errno) + ", " + strerror(errno);
        qFatal( qPrintable(errmsg) );
    }
    if(!semaphore.tryAcquire(150000))
        qFatal( "Failed acquiring the semaphore. If this is because of a failed install, please run make install from the root directory again to clear out the failed lock" );

    // Add one or more .desktop files to the database
    QtopiaSql::instance()->loadConfig(QString ("QSQLITE"), dbPath, QString ());

    if(!QDBMigrationEngine().migrate(&QtopiaSql::instance()->systemDatabase()))
    {
        semaphore.release();
        qFatal( "Database creation/upgrade failed on database %s", qPrintable( dbPath ));
    }
    QString prefixPath = args.takeFirst();
    QString destPath = args.takeFirst();
    QStringList sharedCategories = args.takeFirst().split(" ", QString::SkipEmptyParts);
    while ( args.count() ) {
        QStringList categories = sharedCategories;
        QString sourceFile = args.takeFirst();
        QString destFile = prefixPath + destPath + QLatin1Char('/') + QFileInfo(sourceFile).fileName();
        if ( QFileInfo(sourceFile).suffix() == "directory" ) {
            QSettings settings( sourceFile, QSettings::IniFormat );
            settings.beginGroup( QLatin1String( "Desktop Entry" ) );
            QString name = settings.value( QLatin1String("Name[]") ).toString();
            QString icon = settings.value( QLatin1String("Icon") ).toString();
            categories = settings.value( QLatin1String("Categories") ).toString().split( QLatin1Char( ';' ), QString::SkipEmptyParts );
            if (categories.count() == 1)
                categories = categories.first().split(QLatin1Char(','), QString::SkipEmptyParts);
            settings.endGroup();
            settings.beginGroup( QLatin1String( "Translation" ) );
            QString translationFile = settings.value( QLatin1String( "File" ) ).toString();
            QString translationContext = settings.value( QLatin1String( "Context" ) ).toString();
            settings.endGroup();
            QCategoryManager catMan( QLatin1String( "Applications" ) );
            QString id = QFileInfo(destPath).baseName();
            destFile = QLatin1String( "Folder/" ) + id;
            QContentId oldId = QContent::execToContent( destFile );
            if (oldId != QContent::InvalidId) {
                QContent::uninstall(oldId);
                if (qcopFlag)
                    sendQcop(prefixPath, dbPath, oldId.second, 1);
            }
            if( removeFlag ) {
                catMan.remove( id );
                continue;
            }
            // Ensure the category id exists
            // For new code a more unique id should be used instead of using the untranslated text
            // eg. ensureSystemCategory("com.mycompany.myapp.mycategory", "My Category");
            catMan.ensureSystemCategory( id, name, icon );

            QContent content;
            content.setName( name );
            content.setType( destFile );
            content.setFile( destFile );
            content.setRole( QContent::Folder );
            content.setIcon( icon );
            content.setCategories( categories );
            content.setProperty( QLatin1String( "File" ), translationFile, QLatin1String( "Translation" ) );
            content.setProperty( QLatin1String( "Context" ), translationContext, QLatin1String( "Translation" ) );
            content.commit();
            if (qcopFlag)
                sendQcop(prefixPath, dbPath, content.id().second, 0);
        } else {
            // Remove the existing entry (we don't want duplication)
            QContent content(destFile, false);
            if (content.id() != QContent::InvalidId) {
                QContent::uninstall(content.id());
                if (qcopFlag)
                    sendQcop(prefixPath, dbPath, content.id().second, 1);
            }
            if ( removeFlag )
                continue;
            // Create a new entry
            content = QContent(sourceFile, false);
            if ( QFileInfo(sourceFile).suffix() == "desktop" ) {
                content.setLinkFile(destFile);
                categories = content.categories();
                categories.prepend(QFileInfo(destPath).baseName());
            }
            if( content.role() != QContent::Application && content.fileKnown() )
                if(QFileInfo(content.fileName()).absolutePath() == QFileInfo(sourceFile).absolutePath())
                    content.setFile(QDir::cleanPath(QFileInfo(destFile).absolutePath()+ QLatin1Char('/')+QFileInfo(content.fileName()).fileName()));
            if ( QFileInfo(sourceFile).suffix() != "desktop" ) {
                content.setName( QFileInfo(sourceFile).baseName() );
                content.setRole( QContent::Data );
            }
            // categories can come in via the .pro file (required for hint=content)
            if ( categories.count() != 0 )
                content.setCategories( categories );
            content.commit();
            if (qcopFlag)
                sendQcop(prefixPath, dbPath, content.id().second, 0);
        }
    }
    semaphore.release();
}
