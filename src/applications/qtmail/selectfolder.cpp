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

#include "selectfolder.h"
#include <qtopiaapplication.h>
#include <qlistwidget.h>
#include <qlayout.h>
#include "emailfolderlist.h"

SelectFolderDialog::SelectFolderDialog(const QStringList list,
                                       QWidget *parent)
    : QDialog( parent )
{
    QtopiaApplication::setMenuLike( this, true );
    setWindowTitle( tr( "Select folder" ) );
    QGridLayout *top = new QGridLayout( this );
    for( int i = 0; i < list.count(); i++ ) {
        mMailboxList.append(MailboxList::mailboxTrName(list[i]));
    }

    mFolderList = new QListWidget( this );
    top->addWidget( mFolderList, 0, 0 );
    getFolders();

    // Required for current item to be shown as selected(?)
    if (mFolderList->count())
        mFolderList->setCurrentRow( 0 );

    connect(mFolderList, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(selected()) );
}

SelectFolderDialog::~SelectFolderDialog()
{
}

int SelectFolderDialog::folder()
{
    return mFolderList->currentRow();
}

void SelectFolderDialog::getFolders()
{
    QStringList mboxList = mMailboxList;
    for (QStringList::Iterator it = mboxList.begin(); it != mboxList.end(); ++it)
        mFolderList->addItem( *it );
}

void SelectFolderDialog::selected()
{
    done(1);
}

