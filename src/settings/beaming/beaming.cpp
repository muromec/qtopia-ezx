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
#include "beaming.h"
#include "ircontroller.h"

#include <QSignalMapper>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTimer>
#include <QLayout>
#include <QListWidget>
#include <QStringList>
#include <QLabel>
#include <qtopianamespace.h>
#include <qtopia/qsoftmenubar.h>
#include <qirlocaldevice.h>

static int AUTO_QUIT_TIME = 0; // or 0 if auto-quitting is confusing.

Beaming::Beaming( QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl ), state( -1 ), protocol( -1 )
{
    setWindowTitle(tr("Beaming"));

    QVBoxLayout *vbl = new QVBoxLayout(this);
    vbl->setMargin(4);
    vbl->setSpacing(2);

    // Take care of the case where we have no Infrared adapters
    QStringList list = QIrLocalDevice::devices();
    if (list.size() == 0) {
        QLabel *label = new QLabel(tr("<P>No infrared devices found"));
        label->setWordWrap(true);
        vbl->addWidget(label);
        this->setLayout(vbl);
        showMaximized();
        return;
    }

    irc = new IRController(this);

    QButtonGroup* bg = new QButtonGroup(this);
    QSignalMapper* sm = new QSignalMapper(this);
    for (int i=0; i<=(int)IRController::LastState; i++) {
        QRadioButton *b = new QRadioButton(this);
        vbl->addWidget(b);
        bg->addButton(b);
        b->setText(IRController::stateDescription((IRController::State)i));
        if ( (IRController::State)i == irc->state() )
            b->setChecked(true);
        if ( irc->state() == IRController::Off ) {
            if ( (IRController::State)i == IRController::On1Item )
                b->setFocus();
        } else {
            if ( (IRController::State)i == IRController::Off )
                b->setFocus();
        }
        connect(b,SIGNAL(clicked()), sm, SLOT(map()));
        sm->setMapping(b,i);
    }
    connect(sm,SIGNAL(mapped(int)),this,SLOT(chooseState(int)));

    int pc = irc->protocolCount();
    if ( pc ) {
        lb = new QListWidget(this);
        vbl->addWidget(lb);
        for (int i=0; i<pc; i++) {
            QString n = irc->protocolName(i);
            QIcon ic = irc->protocolIcon(i);
            if ( ic.isNull() ) {
                lb->insertItem(i, n);
            } else {
                QListWidgetItem *item = new QListWidgetItem(n);
                item->setIcon(ic);
                lb->insertItem(i, item);
            }
        }
        lb->setItemSelected(lb->item(irc->currentProtocol()),true);
        connect(lb,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(chooseProtocol(QListWidgetItem*)));
    }

    // add context menu for help
    QSoftMenuBar::menuFor( this );

    this->setLayout(vbl);
}

void Beaming::chooseState(int c)
{
    state = c;
    if ( AUTO_QUIT_TIME )
        QTimer::singleShot( AUTO_QUIT_TIME, this, SLOT(accept()) );
}

void Beaming::chooseProtocol(QListWidgetItem *item)
{
    protocol = lb->row( item );
    if( !Qtopia::mousePreferred() )
        lb->setEditFocus(false);
}

Beaming::~Beaming()
{
}

void Beaming::accept()
{
    if ( state != -1 )
        irc->setState( (IRController::State) state );

    if ( protocol != -1 )
        irc->setProtocol( protocol );

    QDialog::accept();
}

