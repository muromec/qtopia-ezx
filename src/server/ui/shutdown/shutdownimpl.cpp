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

#include "shutdownimpl.h"

#include <qtimer.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qtopiaapplication.h>

#include <stdio.h>

using namespace Ui;

ShutdownImpl::ShutdownImpl( QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl )
{
    setupUi(this);
    timer = new QTimer( this );
    connect( timer, SIGNAL(timeout()), this, SLOT(timeout()) );

    connect( reboot, SIGNAL(clicked()), this, SLOT(rebootClicked()));
    connect( restart, SIGNAL(clicked()), this, SLOT(restartClicked()));
    connect( quit, SIGNAL(clicked()), this, SLOT(quitClicked()));
    connect( shut, SIGNAL(clicked()), this, SLOT(shutdownClicked()));

//    connect( cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()) );

    progressBar->hide();
    QtopiaApplication::hideInputMethod();
#ifdef QT_QWS_SL5XXX
    QPushButton *sb = Shutdown::shutdown;
    sb->hide();
#endif
}

void ShutdownImpl::rebootClicked()
{
    operation = QtopiaServerApplication::RebootSystem;
    initiateAction();
}
void ShutdownImpl::restartClicked()
{
    operation = QtopiaServerApplication::RestartDesktop;
    initiateAction();
}
void ShutdownImpl::quitClicked()
{
    operation = QtopiaServerApplication::TerminateDesktop;
    initiateAction();
}
void ShutdownImpl::shutdownClicked()
{
    operation = QtopiaServerApplication::ShutdownSystem;
    initiateAction();
}

void ShutdownImpl::initiateAction()
{
    progress = 0;

    progressBar->show();
    timer->start( 300 );
    timeout();
}

/*
void ShutdownImpl::cancelClicked()
{
    progressBar->hide();
    if ( timer->isActive() )
        timer->stop();
    else
        close();
}
*/

void ShutdownImpl::timeout()
{
    if ( (progress+=2) > progressBar->maximum() ) {
        progressBar->hide();
        timer->stop();
        emit shutdown( operation );
        close();
    } else {
        progressBar->setValue( progress );
    }
}


