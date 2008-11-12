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

#include "storagemonitor.h"
#include <QMessageBox>
#include <QtopiaTimer>
#include <QtopiaService>
#include <qsoftmenubar.h>
#include <qvaluespace.h>
#include "qtopiaserverapplication.h"
#include <qtopiaipcenvelope.h>
#include <qtopiaservices.h>
#include <qstorage.h>
#include <QSettings>
#include <QTimer>

class StorageAlertBox : public QMessageBox
{
    public:
        StorageAlertBox(const QString &caption, const QString &text, Icon icon,
                int button0, int button1, int button2,
                QWidget *parent = 0, Qt::WFlags f = Qt::MSWindowsFixedSizeDialogHint)
           : QMessageBox(caption, text, icon, button0, button1, button2, parent, f)
        {
        };
    protected:
        void showEvent(QShowEvent *se) {
            QSoftMenuBar::setLabel(this, Qt::Key_Back, "trash", "");
            QMessageBox::showEvent(se);
        };
};

/*!
  \class StorageMonitor
  \ingroup QtopiaServer::Task
  \brief The StorageMonitor class periodically checks filesystems for available space and prompts to start the cleanup wizard if low.

  Periodically the StorageMonitor class examines the writable filesystems for
  free space.  The polling frequency can be controlled through configuration.
  If the amount of free space, summed across all writable filesystems, is found
  to be below a configurable limit, a dialog box is displayed prompting the
  user to delete unwanted documents.  If an application providing the 
  \c {CleanupWizard} service is installed, the user can also directly launch
  the cleanup wizard.

  As it is primarily a graphical feature, to preserve power the storage monitor
  suspends polling while the device's screen is turned off, on the rationale 
  that the user is probably not present or interacting with the device.

  The StorageMonitor class is configured through the \c {Trolltech/qpe} settings
  file.  The following keys control the configuration.

  \table
  \header \o Key \o Description
  \row \o \c {StorageMonitor/Enabled} \o Set to false if the storage monitor is to be disabled.  By default this value is true and the storage monitor is enabled.
  \row \o \c {StorageMonitor/UpdatePeriod} \o The rate, in seconds, at which the storage monitor polls the filesystems.  By default this is 60 seconds.
  \row \o \c {StorageMonitor/MinimalStorageLimit} \o When the total free space summed across all writable filesystems is below this value, in kilobytes, the storage monitor dialog will be triggered.  By default this is 20 kilobytes.
  \endtable

  Polls, by default, every 60 seconds, but pauses when screen is off or there
  are no writable file systems.  Checks for CleanupWizard service.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

/*! \internal */
StorageMonitor::StorageMonitor(QObject *o)
: QObject(o), sinfo(0), box(0), storageTimer(0),
  minimalStorageLimit(-1), pollFrequency(-1)
{
    QSettings cfg("Trolltech","qpe");
    cfg.beginGroup("StorageMonitor");
    if (!cfg.value("Enabled", true).toBool()) {
        QtopiaServerApplication::taskValueSpaceSetAttribute("StorageMonitor",
                                                            "InUse",
                                                            "No");
        return;
    } else {
        QtopiaServerApplication::taskValueSpaceSetAttribute("StorageMonitor",
                                                            "InUse",
                                                            "Yes");
    }
    pollFrequency = cfg.value("UpdatePeriod", 60).toInt() * 1000;

    sinfo = QStorageMetaInfo::instance();
    QtopiaChannel *channel = new QtopiaChannel("QPE/System", this);
    connect(channel, SIGNAL(received(QString,QByteArray)),
            this, SLOT(systemMsg(QString,QByteArray)) );


    storageTimer = new QtopiaTimer(this);
    storageTimer->setSingleShot(true);
    connect( storageTimer, SIGNAL(timeout()), this, SLOT(checkAvailStorage()));
    storageTimer->start(pollFrequency, QtopiaTimer::PauseWhenInactive);

    QObject::connect(sinfo, SIGNAL(disksChanged()), 
                     this, SLOT(availableDisksChanged()));
}

/*! \internal */
StorageMonitor::~StorageMonitor()
{
}

/*! \internal */
long StorageMonitor::fileSystemMetrics(const QFileSystem *fs)
{
    long mult = 0;
    long div = 0;
    if (fs == 0)
        return 0;

    if ( fs->blockSize() ) {
        mult = fs->blockSize() / 1024;
        div = 1024 / fs->blockSize();
    }
    if ( !mult ) mult = 1;
    if ( !div ) div = 1;

    return (fs->availBlocks() * mult / div);
}

/*! \internal */
void StorageMonitor::checkAvailStorage()
{
    bool haveWritableFS = false;
    long availStorage = 0;

    QFileSystemFilter fsf;
    fsf.documents = QFileSystemFilter::Set;
    foreach ( QFileSystem *fs, sinfo->fileSystems(&fsf) ) {
        if( fs->isWritable() ) {
            availStorage += fileSystemMetrics(fs);
            haveWritableFS = true;
        }
    }

    if( !haveWritableFS ) {
        return; // no writable filesystems, lack of free space is irrelevant
    }

    //for now read config file each time until we have notification in place
    if(minimalStorageLimit < 0) {
        QSettings cfg("Trolltech","qpe");
        cfg.beginGroup("StorageMonitor");
        minimalStorageLimit = cfg.value("MinimalStorageLimit", 20).toInt();
    }
    
    if(minimalStorageLimit >= 0 && availStorage <= minimalStorageLimit) {
        QString msg = tr("<qt>The device has no free storage space. "
                         "Please delete unwanted documents.</qt>");
        outOfSpace(msg);

        //suppress next msg for 30 mins to avoid annoying msgbox
        storageTimer->start(30 * 60 * 1000, QtopiaTimer::PauseWhenInactive);
    } else {
        storageTimer->start(pollFrequency, QtopiaTimer::PauseWhenInactive);
    }
}

/*! \internal */
void StorageMonitor::systemMsg(const QString &msg, const QByteArray &data)
{
    QDataStream stream( data );

    if ( msg == "outOfDiskSpace(QString)" ) {
        QString text;
        stream >> text;
        outOfSpace(text);
    } else if ( msg == "checkDiskSpace()" ) {
        checkAvailStorage();
    }
}

void StorageMonitor::outOfSpace(QString &msg)
{
    if(box)
        return; // Existing message box is being displayed

    bool cleanupWizard = !QtopiaService::apps("CleanupWizard").isEmpty();

    // start cleanup wizard
    box = new StorageAlertBox(tr("Out of storage space"), "", 
            QMessageBox::Critical,
            QMessageBox::Ok | QMessageBox::Default,
            cleanupWizard?QMessageBox::No:QMessageBox::NoButton, 
            QMessageBox::NoButton);

    if(cleanupWizard)
        box->setButtonText(QMessageBox::No, tr("Cleanup"));

    box->setText(msg);

    switch (QtopiaApplication::execDialog(box)) {
        case QMessageBox::Ok:
            break;

        case QMessageBox::No:
            showCleanupWizard();
            break;
    }

    delete box;
    box = 0;
}

void StorageMonitor::availableDisksChanged()
{
    checkAvailStorage();
}

void StorageMonitor::showCleanupWizard()
{
    QtopiaServiceRequest req("CleanupWizard", "showCleanupWizard()");
    req.send();
}

QTOPIA_TASK(StorageMonitor, StorageMonitor);
