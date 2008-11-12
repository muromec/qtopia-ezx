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

#include <qtopiaapplication.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qgroupbox.h>
#include <qheaderview.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtabwidget.h>
#include <qtoolbutton.h>
#include <qscrollarea.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qvalidator.h>

#include "account.h"
#include "editaccount.h"

static const QMailAccount::AuthType authenticationType[] = {
    QMailAccount::Auth_NONE,
#ifndef QT_NO_OPENSSL
    QMailAccount::Auth_LOGIN,
    QMailAccount::Auth_PLAIN,
#endif
    QMailAccount::Auth_INCOMING
};

#ifndef QT_NO_OPENSSL
static int authenticationIndex(QMailAccount::AuthType type)
{
    const int numTypes = sizeof(authenticationType)/sizeof(QMailAccount::AuthType);
    for (int i = 0; i < numTypes; ++i)
        if (type == authenticationType[i])
            return i;

    return 0;
};
#endif


class PortValidator : public QValidator
{
public:
    PortValidator(QWidget *parent = 0, const char *name = 0)
        : QValidator(parent) {
      setObjectName(name);
    }

    State validate(QString &str, int &) const
    {
        // allow empty strings, as it's a bit awkward to edit otherwise
        if ( str.isEmpty() )
            return QValidator::Acceptable;

        bool ok = false;
        int i = str.toInt(&ok);
        if ( !ok )
            return QValidator::Invalid;

        if ( i <= 0 || i >= 65536 )
            return QValidator::Invalid;

        return QValidator::Acceptable;
    }
};

EditAccount::EditAccount( QWidget* parent, const char* name, Qt::WFlags fl )
    : QDialog(parent, fl)
    , tabWidget(0)
    , accountNameInput(new QLineEdit)
{
    setupUi(this);
    setObjectName(name);

    //connect custom slots
    connect(ToolButton2,SIGNAL(clicked()),SLOT(sigPressed()));
    connect(accountType,SIGNAL(activated(int)),SLOT(typeChanged(int)));
    connect(authentication,SIGNAL(activated(int)),SLOT(authChanged(int)));
    connect(emailInput,SIGNAL(textChanged(QString)),SLOT(emailModified()));
    //connect(mailboxButton,SIGNAL(clicked()),SLOT(configureFolders()));

    emailTyped = false;

    QtopiaApplication::setInputMethodHint(mailPortInput, QtopiaApplication::Number);
    QtopiaApplication::setInputMethodHint(smtpPortInput, QtopiaApplication::Number);

    const QString uncapitalised("email noautocapitalization");

    // These fields should not be autocapitalised
    QtopiaApplication::setInputMethodHint(mailUserInput, QtopiaApplication::Named, uncapitalised);
    QtopiaApplication::setInputMethodHint(mailServerInput, QtopiaApplication::Named, uncapitalised);

    QtopiaApplication::setInputMethodHint(emailInput, QtopiaApplication::Named, uncapitalised);
    QtopiaApplication::setInputMethodHint(smtpUsernameInput, QtopiaApplication::Named, uncapitalised);
    QtopiaApplication::setInputMethodHint(smtpServerInput, QtopiaApplication::Named, uncapitalised);

    // Too easy to mistype numbers in phone mode
    mailPasswInput->installEventFilter( this );
    accountNameInput->installEventFilter( this );
    defaultMailCheckBox->installEventFilter( this );
    installEventFilter(this);
    mailboxButton->hide();
    PortValidator *pv = new PortValidator(this);
    mailPortInput->setValidator(pv);
    smtpPortInput->setValidator(pv);
#ifdef QT_NO_OPENSSL
    encryption->hide();
    authentication->hide();
    lblEncryption->hide();
    lblAuthentication->hide();
    lblSmtpUsername->hide();
    lblSmtpPassword->hide();
    smtpUsernameInput->hide();
    smtpPasswordInput->hide();
    encryptionIncoming->hide();
#else
    authentication->addItem("INCOMING"); // notr
#endif
    typeChanged(1);
    createTabbedView();
    setLayoutDirection( qApp->layoutDirection() );
    mailPasswInput->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    smtpPasswordInput->setEchoMode(QLineEdit::PasswordEchoOnEdit);

    currentTabChanged(0);
}

void EditAccount::setAccount(QMailAccount *in, bool newOne)
{
    account = in;

    if (newOne) {
        accountNameInput->setText("");
        emailInput->setText("");
        mailUserInput->setText("");
        mailPasswInput->setText("");
        mailServerInput->setText("");
        smtpServerInput->setText("");
        syncCheckBox->setChecked(true);
        mailPortInput->setText("110");
        smtpPortInput->setText("25");
#ifndef QT_NO_OPENSSL
        smtpUsernameInput->setText("");
        smtpPasswordInput->setText("");
        encryption->setCurrentIndex(0);
        authentication->setCurrentIndex(0);
        smtpUsernameInput->setEnabled(false);
        smtpPasswordInput->setEnabled(false);
        encryptionIncoming->setCurrentIndex(0);
#endif
        setWindowTitle( tr("Create new account", "translation not longer than English") );

        typeChanged( 0 );
    } else {

        if ( account->accountType() == QMailAccount::POP ) {
            accountType->setCurrentIndex(0);
            typeChanged(0);
        } else if ( account->accountType() == QMailAccount::IMAP ) {
            accountType->setCurrentIndex(1);
            typeChanged(1);
            imapBaseDir->setText( account->baseFolder() );
        } else {
            accountType->setCurrentIndex(2);
            typeChanged(2);
        }

        accountNameInput->setText( account->accountName() );
        nameInput->setText( account->userName() );
        emailInput->setText( account->emailAddress() );
        mailUserInput->setText( account->mailUserName() );
        mailPasswInput->setText( account->mailPassword() );
        mailServerInput->setText( account->mailServer() );
        smtpServerInput->setText( account->smtpServer() );
        syncCheckBox->setChecked( account->synchronize() );
        deleteCheckBox->setChecked( account->deleteMail() );

        sigCheckBox->setChecked( account->useSig() );
        sig = account->sig();

        maxSize->setValue(account->maxMailSize());
        thresholdCheckBox->setChecked( account->maxMailSize() != -1 );
        mailPortInput->setText( QString::number( account->mailPort() ) );
        smtpPortInput->setText( QString::number( account->smtpPort() ) );
        defaultMailCheckBox->setChecked( account->defaultMailServer() );
#ifndef QT_NO_OPENSSL
        smtpUsernameInput->setText(account->smtpUsername());
        smtpPasswordInput->setText(account->smtpPassword());
        authentication->setItemText(3, accountType->currentText());
        authentication->setCurrentIndex(authenticationIndex(account->smtpAuthentication()));
        encryption->setCurrentIndex(static_cast<int>(account->smtpEncryption()));
        QMailAccount::AuthType type = authenticationType[authentication->currentIndex()];
        smtpUsernameInput->setEnabled(type == QMailAccount::Auth_LOGIN || type == QMailAccount::Auth_PLAIN);
        smtpPasswordInput->setEnabled(type == QMailAccount::Auth_LOGIN || type == QMailAccount::Auth_PLAIN);
        encryptionIncoming->setCurrentIndex(static_cast<int>(account->mailEncryption()));
#endif
    }

    nameInput->setText( account->userName() );
}

void EditAccount::createTabbedView()
{
    delete layout();

    QVBoxLayout* thelayout = new QVBoxLayout(this);
    thelayout->setMargin(0);
    thelayout->setSpacing(4);

    QHBoxLayout* formLayout = new QHBoxLayout;
    formLayout->setMargin(6);
    formLayout->setSpacing(4);
    formLayout->addWidget(new QLabel(tr("Acct")));
    formLayout->addWidget(accountNameInput);
    thelayout->addLayout(formLayout);

    QFrame* separator = new QFrame;
    separator->setFrameStyle(QFrame::HLine);
    thelayout->addWidget(separator);

    tabWidget = new QTabWidget(this);
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    thelayout->addWidget(tabWidget);

    QScrollArea* scroll = new QScrollArea(tabWidget);
    scroll->setFocusPolicy(Qt::NoFocus);
    scroll->setWidget(incomingFrame);
    scroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    scroll->setWidgetResizable(true);
    scroll->setFrameStyle(QFrame::NoFrame);
    incomingFrame->setLineWidth(0);
    incomingFrame->setMidLineWidth(0);
    incomingFrame->setFrameStyle(QFrame::NoFrame);
    tabWidget->addTab(scroll,tr("Incoming"));

    scroll = new QScrollArea(tabWidget);
    scroll->setFocusPolicy(Qt::NoFocus);
    scroll->setWidget(outgoingFrame);
    scroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    scroll->setWidgetResizable(true);
    scroll->setFrameStyle(QFrame::NoFrame);
    outgoingFrame->setLineWidth(0);
    outgoingFrame->setMidLineWidth(0);
    outgoingFrame->setFrameStyle(QFrame::NoFrame);
    tabWidget->addTab(scroll,tr("Outgoing"));

    updateGeometry();

    accountNameInput->setFocus();
}

void EditAccount::currentTabChanged(int index)
{
    // Change the name to select the relevant help page
    setObjectName(index == 0 ? "email-account-in" : "email-account-out");
}

// bool EditAccount::event(QEvent* e)
// {
//     if(e->type() == QEvent::LayoutRequest)
//     {
//         if(width() < (incomingFrame->sizeHint().width() + outgoingFrame->sizeHint().width()))
//         {
//             if(!tabWidget)
//             {
//                 createTabbedView();
//                 return QDialog::event(e);
//             }
//         }
//     }
//     return QDialog::event(e);
// }

bool EditAccount::eventFilter( QObject* o, QEvent *e )
{
    if ((o == defaultMailCheckBox) && (e->type() == QEvent::FocusIn)) {
        if (tabWidget && tabWidget->currentIndex() != 1)
            tabWidget->setCurrentIndex( 1 );
        return QWidget::eventFilter( o, e );
    }
    return QDialog::eventFilter(o,e);
}

//TODO fix, unused code, slot inaccessible since button not visible on form

// void EditAccount::configureFolders()
// {
//     MailboxSelector sel(this, "sel", account);
//     QtopiaApplication::execDialog(&sel);
// }

void EditAccount::sigPressed()
{
    if ( sigCheckBox->isChecked() ) {
      SigEntry sigEntry(this, "sigEntry", static_cast<Qt::WFlags>(1));

        if ( sig.isEmpty() ) {
            sigEntry.setEntry( "~~\n" + nameInput->text() );
        } else {
            sigEntry.setEntry( sig );
        }

        if ( QtopiaApplication::execDialog(&sigEntry) == QDialog::Accepted) {
            sig = sigEntry.entry();
        }
    }
}

void EditAccount::emailModified()
{
    emailTyped = true;
}

void EditAccount::authChanged(int index)
{
    bool enableFields;
#ifndef QT_NO_OPENSSL
    QMailAccount::AuthType type = authenticationType[index];
    enableFields = (type == QMailAccount::Auth_LOGIN || type == QMailAccount::Auth_PLAIN);
#else
    Q_UNUSED(index);
    enableFields = false;
#endif

    smtpUsernameInput->setEnabled(enableFields);
    smtpPasswordInput->setEnabled(enableFields);
    if (!enableFields) {
        smtpUsernameInput->clear();
        smtpPasswordInput->clear();
    }
}

void EditAccount::typeChanged(int)
{
#ifndef QT_NO_OPENSSL
    // Keep the authentication option in sync with the selected account type
    authentication->setItemText(3, accountType->currentText());
#endif

    if ( accountType->currentText() == tr("Sync") ) {
        imapSettings->hide();
        syncCheckBox->hide();

        mailboxButton->setEnabled( false );
        mailPortInput->setEnabled( false );

        smtpPortInput->setEnabled( false );
        smtpServerInput->setEnabled( false );
        deleteCheckBox->setEnabled( false );
        defaultMailCheckBox->setEnabled( false );
        thresholdCheckBox->setEnabled( false );

        return;
    } else {
        mailboxButton->setEnabled( true );
        mailPortInput->setEnabled( true );

        smtpPortInput->setEnabled( true );
        smtpServerInput->setEnabled( true );
        deleteCheckBox->setEnabled( true );
        defaultMailCheckBox->setEnabled( true );
        thresholdCheckBox->setEnabled( true );
    }

    if (accountType->currentText() == "POP") {
        mailPortInput->setText("110");

        imapSettings->hide();
        syncCheckBox->hide();
        deleteCheckBox->setChecked( false );
        
        if (encryptionIncoming->count() > 2)
            encryptionIncoming->removeItem(2);
    } else if (accountType->currentText() == "IMAP") {
        syncCheckBox->hide();
        imapSettings->show();
        mailboxButton->setEnabled( account->mailboxes().count() > 0 );
        mailPortInput->setText("143");
        deleteCheckBox->setChecked( true );
        
        if (encryptionIncoming->count() < 3)
            encryptionIncoming->addItem("TLS");
    }
}

void EditAccount::deleteAccount()
{
    done(2);
}

void EditAccount::accept()
{
    QString name = accountNameInput->text();
    if ( name.trimmed().isEmpty() ) {
        name = mailServerInput->text();
        if ( name.trimmed().isEmpty() )
            name = smtpServerInput->text();
    }

    if (name.trimmed().isEmpty()) {
        int ret = QMessageBox::warning(this, tr("Empty account name"),
                tr("<qt>Do you want to continue and discard any changes?</qt>"),
                QMessageBox::Yes, QMessageBox::No|QMessageBox::Default|QMessageBox::Escape);
        if (ret == QMessageBox::Yes)
            reject();
        return;
    }

    account->setAccountName( accountNameInput->text() );
    account->setUserName( nameInput->text() );
    account->setEmailAddress( emailInput->text() );
    account->setMailUserName( mailUserInput->text() );
    account->setMailPassword( mailPasswInput->text() );
    account->setMailServer( mailServerInput->text() );
    account->setSmtpServer( smtpServerInput->text() );
    account->setSynchronize( syncCheckBox->isChecked() );
    account->setDeleteMail( deleteCheckBox->isChecked() );

    if ( accountType->currentText() == "POP" ) {
        account->setAccountType( QMailAccount::POP );
    } else if ( accountType->currentText() == "IMAP") {
        account->setAccountType( QMailAccount::IMAP );
    } else {
        account->setAccountType( QMailAccount::Synchronized );
    }

    account->setUseSig( sigCheckBox->isChecked() );
    account->setSig( sig );
    account->setDefaultMailServer( defaultMailCheckBox->isChecked() );

    if ( thresholdCheckBox->isChecked() ) {
        account->setMaxMailSize( maxSize->value() );
    } else {
        account->setMaxMailSize( -1 );
    }
    account->setCheckInterval( -1 );
    if ( account->accountType() == QMailAccount::IMAP )
        account->setBaseFolder( imapBaseDir->text() );

    QString temp;
    bool result;
    temp = mailPortInput->text();
    account->setMailPort( temp.toInt(&result) );
    if ( (!result) ) {
        // should only happen when the string is empty, since we use a validator.
        if (accountType->currentText() == "POP")
            account->setMailPort( 110 );
        else
            account->setMailPort( 143 );
    }

    temp = smtpPortInput->text();
    account->setSmtpPort( temp.toInt(&result) );
    // should only happen when the string is empty, since we use a validator.
    if ( !result )
        account->setSmtpPort( 25 );

    //try to guess email address
    if ( (!emailTyped) && (account->emailAddress().isEmpty()) ) {
        QString address = account->smtpServer();

        if ( address.count('.')) {
            account->setEmailAddress( account->mailUserName() + "@" +
            address.mid( address.indexOf('.') + 1, address.length() ) );
        } else if (address.count('.') == 1) {
            account->setEmailAddress( account->mailUserName() + "@" + address );
        }
    }

    //set an accountname
    if ( account->accountName().isEmpty() ) {
        int pos = name.indexOf('.');
        if ( pos != -1) {
            name = name.mid( pos + 1, name.length() );

            pos = name.indexOf('.', pos);
            if (pos != -1)
                name = name.mid(0, pos);
        }

        account->setAccountName( name );
    }

#ifndef QT_NO_OPENSSL
    account->setSmtpUsername(smtpUsernameInput->text());
    account->setSmtpPassword(smtpPasswordInput->text());
    account->setSmtpAuthentication(authenticationType[authentication->currentIndex()]);
    account->setSmtpEncryption(static_cast<QMailAccount::EncryptType>(encryption->currentIndex()));
    account->setMailEncryption(static_cast<QMailAccount::EncryptType>(encryptionIncoming->currentIndex()));
#endif

    QDialog::accept();
}

SigEntry::SigEntry(QWidget *parent, const char *name, Qt::WFlags fl )
    : QDialog(parent,fl)
{
    setObjectName(name);
    setWindowTitle( tr("Signature") );

    QGridLayout *grid = new QGridLayout(this);
    input = new QTextEdit(this);
    grid->addWidget(input, 0, 0);
}

// /*  class MailboxView    */
// MailboxView::MailboxView(QWidget *parent, const char* name)
//     : QListWidget(parent)
// {
//     setObjectName(name);
//     pop = new QMenu(this);
//     connect(pop, SIGNAL(triggered(QAction*)), this,
//      SLOT(changeMessageSettings(QAction*)) );
//
//     pop->addAction( tr("All"));
//     pop->addAction( tr("Only recent"));
//     pop->addAction( tr("None (only headers)"));
//
//     connect( &menuTimer, SIGNAL(timeout()), SLOT(showMessageOptions()) );
//     connect( this, SIGNAL(itemChanged()), SLOT(cancelMenuTimer()) );
// }
//
// void MailboxView::changeMessageSettings(QAction* id)
// {
//     QListWidgetItem *item = currentItem();
//     if ( !item )
//  return;
//
//     item->setText(id->text());
// }
//
// void MailboxView::showMessageOptions()
// {
//     pop->popup( QCursor::pos() );
// }
//
// void MailboxView::mousePressEvent( QMouseEvent * e )
// {
//     QListView::mousePressEvent( e );
//     menuTimer.start( 500);
//     menuTimer.setSingleShot(true);
// }
//
// void MailboxView::mouseReleaseEvent( QMouseEvent * e )
// {
//     QListView::mouseReleaseEvent( e );
//     menuTimer.stop();
// }
//
// void MailboxView::cancelMenuTimer()
// {
//     if( menuTimer.isActive() )
//     menuTimer.stop();
// }
//
// /*  Class MailboxSelector   */
// MailboxSelector::MailboxSelector(QWidget *parent, const char *name, QMailAccount *account)
//     : QDialog(parent)
// {
//     setObjectName(name);
//     _account = account;
//
//     setWindowTitle( tr("Configure IMAP folders") );
//
//     QGridLayout *grid = new QGridLayout(this);
// //    grid->setSpacing(4);
// //    grid->setMargin(4);
//
//     view = new MailboxView(this, "mview");
//     QAction* what = QWhatsThis::createAction(view);
//     what->setText(tr("Lists your IMAP mailboxes.  Tick the mailboxes which you want to access on your device."));
//
//    // what = QWhatsThis::createAction(view->header());
//    // what->setText(tr("For selected mailboxes, you can choose which type of messages to download."));
//
//     view->addColumn(tr("Keep local copy"), 120);
//     view->addColumn(tr("Complete messages"), 115);
//     view->setAllColumnsShowFocus( true );
//     grid->addMultiCellWidget(view, 0, 0, 0, 3);
//
//     QPushButton *button = new QPushButton( tr("Select all"), this);
//
//     what = QWhatsThis::createAction(button);
//     what->setText(tr("Select all mailboxes in your account."));
//     connect(button, SIGNAL(clicked()), this, SLOT(selectAll()) );
//     grid->addWidget(button, 1, 0);
//
//     button = new QPushButton( tr("Deselect all"), this);
//
//     what = QWhatsThis::createAction(button);
//     what->setText(tr("Deselect all mailboxes in your account."));
//     connect(button, SIGNAL(clicked()), this, SLOT(clearAll()) );
//     grid->addWidget(button, 1, 1);
//
//     QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
// //     grid->addItem(spacer, 1, 2);
//
//     button = new QPushButton( tr("Change message type"), this );
//     what = QWhatsThis::createAction(button);
//     what->setText(tr("Choose the message types you want to download for the selected mailbox."));
//     connect(button, SIGNAL(clicked()), this, SLOT(itemSelected()) );
//     grid->addWidget(button, 1, 3);
//
//     Mailbox *box;
//     QCheckListItem *item;
//     uint row = 0;
//     for (box = _account->mailboxes.first(); box != NULL;
//  box = _account->mailboxes.next() ) {
//
//  item = new QCheckListItem(view, box->pathName(), QCheckListItem::CheckBox);
//  item->setOn( box->localCopy() );
//
//  QString sync = tr("All");
//  FolderSyncSetting fs = box->folderSync();
//  if ( fs == Sync_OnlyNew)
//      sync = tr("Only recent");
//  if ( fs == Sync_OnlyHeaders)
//      sync = tr("None (only headers)");
//
//  item->setText(1, sync );
//
//  row++;
//     }
//
// }
//
// void MailboxSelector::accept()
// {
//     QCheckListItem *item;
//     QListViewItemIterator it(view);
//     uint row = 0, localCopy = 0;
//
//     for ( ; it.current(); ++it ) {
//      item = (QCheckListItem *) it.current();
//  if ( item->isOn() )
//      localCopy++;
//     }
//
//     if ( localCopy == 0 && view->childCount() > 0 ) {
//  QString message = tr("<qt>No local copies? You must select the mailboxes you want to access mail from</qt>");
//
//  switch( QMessageBox::warning( this, tr("Email"), message,
//      tr("I know"), tr("Whoops"), 0, 0, 1 ) ) {
//
//      case 0: break;
//      case 1: return;
//  }
//
//     }
//
//     localCopy = 0;
//     it = view->firstChild();
//     for ( ; it.current(); ++it) {
//      item = (QCheckListItem *) it.current();
//  Mailbox *box =  _account->getMailboxRef( item->text(0) );
//  if ( box ) {
//      box->setLocalCopy( item->isOn() );
//      if ( item->isOn() )
//      localCopy++;
//
//      FolderSyncSetting fs = Sync_AllMessages;
//      QString sync = item->text(1);
//      if ( sync == tr("Only recent") ) {
//      fs = Sync_OnlyNew;
//      } else if ( sync == tr("None (only headers)") ) {
//      fs = Sync_OnlyHeaders;
//      }
//      box->setFolderSync( fs );
//  }
//
//  row++;
//     }
//
//     QDialog::accept();
// }
//
// void MailboxSelector::selectAll()
// {
//     QListViewItemIterator it(view);
//     for ( ; it.current(); ++it) {
//  ( (QCheckListItem *) it.current())->setOn(true);
//     }
// }
//
// void MailboxSelector::clearAll()
// {
//     QListViewItemIterator it(view);
//     for ( ; it.current(); ++it) {
//  ( (QCheckListItem *) it.current())->setOn(false);
//     }
// }
//
// void MailboxSelector::itemSelected()
// {
//     QListViewItem *item = view->selectedItem();
//     if ( item )
//  view->showMessageOptions();
// }
