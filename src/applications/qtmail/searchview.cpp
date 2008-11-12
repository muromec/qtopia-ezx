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



#include "searchview.h"
#include "accountlist.h"
#include "emailfolderlist.h"


#include <qtopiaapplication.h>
#include <qsoftmenubar.h>
#include <QMenu>
#include <QDesktopWidget>



SearchView::SearchView(bool query,  QWidget* parent, Qt::WFlags fl )
    : QDialog(parent, fl)
{
    setupUi( this );

    delete layout();
    QGridLayout *g = new QGridLayout(this);
    sv = new QScrollArea(this);
    sv->setFocusPolicy(Qt::NoFocus);
    sv->setWidgetResizable( true );
    sv->setFrameStyle(QFrame::NoFrame);
    sv->setWidget(searchFrame);
    sv->setFocusPolicy(Qt::NoFocus);
    g->addWidget(sv, 0, 0);

    int dw = QApplication::desktop()->availableGeometry().width();
    searchFrame->setMaximumWidth(dw - qApp->style()->pixelMetric(QStyle::PM_SliderLength) + 4 );
    sv->setMaximumWidth(dw);

    queryType = query;

    if ( queryType ) {
        setWindowTitle( tr("Query") );
        nameLabel->show();
        nameLine->show();
    } else {
        setWindowTitle( tr("Search") );
        nameLabel->hide();
        nameLine->hide();
    }

    init();
}

SearchView::~SearchView()
{
}

void SearchView::init()
{
    /*  Set up dateBookMonth popups */
    dateAfter = QDate::currentDate();
    dateAfterButton->setDate( dateAfter );
    connect( dateAfterButton, SIGNAL(dateChanged(QDate)),
            this, SLOT(dateAfterChanged(QDate)) );

    dateBefore = QDate::currentDate();
    dateBeforeButton->setDate( dateBefore );
    connect( dateBeforeButton, SIGNAL(dateChanged(QDate)),
            this, SLOT(dateBeforeChanged(QDate)) );

    /*  Fix tab order   */
    setTabOrder(mailbox, status);
    setTabOrder(status, fromLine);
    setTabOrder(fromLine, toLine);
    setTabOrder(toLine,subjectLine);
    setTabOrder(subjectLine, bodyLine);
    setTabOrder(bodyLine, dateAfterBox);
    setTabOrder(dateAfterBox, dateAfterButton);
    setTabOrder(dateAfterButton, dateBeforeBox);
    setTabOrder(dateBeforeBox, dateBeforeButton);
}

void SearchView::setQueryBox(QString box)
{
    if (box.isEmpty()) {
        mailbox->setCurrentIndex(0);
    } else {
        box = MailboxList::mailboxTrName(box).toLower();
        for (int i = 1; i < mailbox->count(); i++) {
            if (mailbox->itemText(i).toLower() == box ) {
                mailbox->setCurrentIndex(i);
                break;
            }
        }
    }

    mailbox->setEnabled(false);
}

// this function is assumed call directly after the constructor
void SearchView::setSearch(Search *in)
{
    QString str = in->mailbox();
    if ( str.isEmpty() )
        mailbox->setCurrentIndex(0);
    else if ( str == MailboxList::InboxString )
        mailbox->setCurrentIndex(1);
    else if ( str == MailboxList::OutboxString )
        mailbox->setCurrentIndex(2);
    else if ( str == MailboxList::DraftsString )
        mailbox->setCurrentIndex(3);
    else if ( str == MailboxList::SentString )
        mailbox->setCurrentIndex(4);
    else if ( str == MailboxList::TrashString )
        mailbox->setCurrentIndex(5);

    switch( in->status() ) {
        case Search::Read: {
        status->setCurrentIndex( 1 );
            break;
        }
        case Search::Unread: {
        status->setCurrentIndex( 2 );
            break;
        }
        case Search::Replied: {
        status->setCurrentIndex( 3 );
            break;
        }
    }

    fromLine->setText( in->getFrom() );
    toLine->setText( in->getTo() );
    subjectLine->setText( in->getSubject() );
    bodyLine->setText( in->getBody() );

    dateBefore = in->getBeforeDate();
    dateAfter = in->getAfterDate();

    if ( !dateAfter.isNull() ) {
        dateAfterBox->setChecked(true);
        dateAfterButton->setDate( dateAfter );

    } else {
        dateAfter = QDate::currentDate();
    }

    if ( !dateBefore.isNull() ) {
        dateBeforeBox->setChecked(true);
        dateBeforeButton->setDate( dateBefore );

    } else {
        dateBefore = QDate::currentDate();
    }

    nameLine->setText( in->name() );
}

// creates a new Search object, and returns.  Caller assumes
// ownership of search object
Search* SearchView::getSearch()
{
    Search *search = new Search();

    int i = mailbox->currentIndex();
    switch(i) {
        case 0: search->setMailbox(QString()); break;
        case 1: search->setMailbox(MailboxList::InboxString); break;
        case 2: search->setMailbox(MailboxList::OutboxString); break;
        case 3: search->setMailbox(MailboxList::DraftsString); break;
        case 4: search->setMailbox(MailboxList::SentString); break;
        case 5: search->setMailbox(MailboxList::TrashString); break;
    }

    search->setMailFrom( fromLine->text() );
    search->setMailTo( toLine->text() );
    search->setMailSubject( subjectLine->text() );
    search->setMailBody( bodyLine->text() );

    int statusNum = status->currentIndex();
    switch( statusNum ) {
        case 1: {
            search->setStatus( Search::Read );
            break;
        }
        case 2: {
            search->setStatus( Search::Unread );
            break;
        }
        case 3: {
            search->setStatus( Search::Replied );
            break;
        }
        default: search->setStatus( Search::Any );
    }

    if ( dateAfterBox->isChecked() ) {
        search->setAfterDate(dateAfter);
    }
    if ( dateBeforeBox->isChecked() ) {
        search->setBeforeDate(dateBefore);
    }

    if ( !nameLine->text().isEmpty() ) {
        search->setName( nameLine->text() );
    }

    return search;
}

void SearchView::dateAfterChanged(const QDate &ymd)
{
    dateAfter = ymd;
}

void SearchView::dateBeforeChanged(const QDate &ymd)
{
    dateBefore = ymd;
}
