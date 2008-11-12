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
#include <qtopiaipcenvelope.h>

#include <qtopianamespace.h>
#include <qsoftmenubar.h>
#include <qtopialog.h>
#include <qperformancelog.h>

#include "emailclient.h"
#include "folder.h"
#include "searchview.h"
#include "accountlist.h"
#include "emailfolderlist.h"
#include "selectfolder.h"
#include "accountsettings.h"
#include "emailpropertysetter.h"
#include <qtopia/mail/qmailaddress.h>
#include <qtopia/mail/qmailcomposer.h>
#include <qtopia/mail/qmailtimestamp.h>
#include <QMailStore>

#include <qapplication.h>
#include <qmessagebox.h>
#include <qfile.h>
#include <qcursor.h>
#include <qstackedwidget.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qdesktopwidget.h>
#include <qtcpsocket.h>
#include <QHeaderView>
#include <QListWidget>
#include <QDebug>
#include <qtmailwindow.h>
#include <qtreewidget.h>
#include <QDSActionRequest>
#include <QDSData>
#include <QtopiaApplication>
#include <QWapAccount>
#include <QStack>
#include <longstream_p.h>
#ifndef QTOPIA_NO_SMS
#include <QSMSMessage>
#endif
#include <QtopiaIpcAdaptor>
#include <QtopiaServiceRequest>

#ifndef LED_MAIL
#define LED_MAIL 0
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

enum QueueStatus {
    Inactive = 0,
    Receiving = 1,
    Sending = 2
};

// Time in ms to show new message dialog.  0 == Indefinate
static const int NotificationVisualTimeout = 0;

// Number of messages required before we use a progress indicator
static const int MinimumForProgressIndicator = 20;
static const int SearchMinimumForProgressIndicator = 100;
static const int ProgressIndicatorUpdatePeriod = 500;

static QIcon* pm_folder = 0;
static QIcon* pm_trash = 0;

static void registerTask(const char* name)
{
    qLog(Messaging) << "Registering task:" << name;
    QtopiaApplication::instance()->registerRunningTask(QLatin1String(name));
}

static void unregisterTask(const char* name)
{
    qLog(Messaging) << "Unregistering task:" << name;
    QtopiaApplication::instance()->unregisterRunningTask(QLatin1String(name));
}

//paths for qtmail, is settings, inbox, enclosures
static QString getPath(const QString& fn, bool isdir=false)
{
    QString p = Qtopia::applicationFileName("qtmail",fn);
    if (isdir) {
        QDir dir(p);
        if ( !dir.exists() )
            dir.mkdir( dir.path() );
        p += "/";
    }
    return p;
}

class AcknowledgmentBox : public QMessageBox
{
    Q_OBJECT

public:
    static void show(const QString& title, const QString& text);

private:
    AcknowledgmentBox(const QString& title, const QString& text);
    ~AcknowledgmentBox();

    virtual void keyPressEvent(QKeyEvent* event);

    static const int _timeout = 3 * 1000;
};

AcknowledgmentBox::AcknowledgmentBox(const QString& title, const QString& text)
    : QMessageBox(0)
{
    setWindowTitle(title);
    setText(text);
    setIcon(QMessageBox::Information);
    setAttribute(Qt::WA_DeleteOnClose);

    QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Cancel);

    QDialog::show();

    QTimer::singleShot(_timeout, this, SLOT(accept()));
}

AcknowledgmentBox::~AcknowledgmentBox()
{
}

void AcknowledgmentBox::show(const QString& title, const QString& text)
{
    (void)new AcknowledgmentBox(title, text);
}

void AcknowledgmentBox::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Select) {
        event->accept();
        accept();
    } else {
        QMessageBox::keyPressEvent(event);
    }
}

// Keep track of where we are within the program
struct UILocation 
{
    UILocation(QWidget* widget, int widgetId = -1, EmailClient::MessageListContent content = EmailClient::Messages);
    UILocation(QWidget* widget, QMailId messageId, Folder* messageFolder, EmailClient::MessageListContent content);

    QWidget* widget;
    int widgetId;
    QMailId messageId;
    Folder* messageFolder;
    EmailClient::MessageListContent content;

private:
    friend class QVector<UILocation>;
    UILocation();
};

UILocation::UILocation()
    : widget(0), widgetId(-1), messageFolder(0), content(EmailClient::Messages)
{
}

UILocation::UILocation(QWidget* widget, int widgetId, EmailClient::MessageListContent content)
    : widget(widget), widgetId(widgetId), messageFolder(0), content(content)
{
}

UILocation::UILocation(QWidget* widget, QMailId messageId, Folder* messageFolder, EmailClient::MessageListContent content)
    : widget(widget), widgetId(-1), messageId(messageId), messageFolder(messageFolder), content(content)
{
}

QDebug& operator<< (QDebug& debug, const UILocation& location)
{
    return debug << '[' << location.widget << ':' << location.widgetId << ':' << location.messageId.toULongLong() 
                 << ':' << location.messageFolder << ':' << location.content << ']';
}


static QStack<UILocation> locationStack;

static void pushLocation(const UILocation& location)
{
    qLog(Messaging) << "pushLocation -" << locationStack.count() << ":" << location;
    locationStack.push(location);
}

static void popLocation()
{
    locationStack.pop();
    if (locationStack.count())
        qLog(Messaging) << "popLocation  -" << locationStack.count() - 1 << ":" << locationStack.top();
    else 
        qLog(Messaging) << "popLocation  - empty";
}

static bool haveLocation()
{
    return !locationStack.isEmpty();
}

static const UILocation& currentLocation()
{
    return locationStack.top();
}


// This is used regularly:
static const QMailMessage::MessageType nonEmailType = static_cast<QMailMessage::MessageType>(QMailMessage::Mms | 
                                                                                             QMailMessage::Sms | 
                                                                                             QMailMessage::System);

EmailClient::EmailClient( QWidget* parent, const QString name, Qt::WFlags fl )
    : QMainWindow( parent, fl ), accountIdCount(0), emailHandler(0),
    enableMessageActions(false), mb(0), fetchTimer(this),
    showMessageType(QMailAccount::SMS), autoDownloadMail(false),
    planeMode("/UI/Profile/PlaneMode"), newMessagesBox(0),
    messageCountUpdate("QPE/Messages/MessageCountUpdated"),
    initialAction(None)
{
    QPerformanceLog(appTitle.toLatin1().constData()) << " : " << "Begin emailclient constructor "
                      << qPrintable( QTime::currentTime().toString( "h:mm:ss.zzz" ) );
    setObjectName( name );
    appTitle = tr("Messages");
    waitingForNewMessage = false;
    newMessagesRequested = false;
    autoGetMail = false;
    mMailboxList = 0;
    accountList = 0;
    suspendMailCount = false;
    sending = false;
    receiving = false;
    previewingMail = false;
    mailIdCount = 1;
    allAccounts = false;
    closeAfterTransmissions = false;
    closeAfterWrite = false;
    mMessageView = 0;
    mFolderView = 0;
    mActionView = 0;
    folderId = -2;
    messageId = -3;
    queueStatus = Inactive;
    nosuspend = 0;
    filesRead = false;
    showMsgList = false;
    showMsgRetryCount = 0;
    lastSearch = 0;
    searchView = 0;
    preSearchWidgetId = -1;
    messageListFolder = 0;
    messageListContent = Messages;

    init();

    // Hook up the QCop service handlers.
    QtopiaAbstractService* svc;

    svc = new EmailService( this );

#ifndef QTOPIA_NO_SMS
    svc = new SMSService( this );
    connect(svc, SIGNAL(newMessages(bool)), this, SLOT(newMessages(bool)));
    connect(svc, SIGNAL(viewInbox()), this, SLOT(viewInbox()));
#ifndef QTOPIA_NO_MMS
    connect(svc, SIGNAL(mmsMessage(QDSActionRequest)), this, SLOT(mmsMessage(QDSActionRequest)));
#endif
#endif

    svc = new MessagesService( this );
    connect(svc, SIGNAL(newMessages(bool)), this, SLOT(newMessages(bool)));
    connect(svc, SIGNAL(message(QMailId)), this, SLOT(displayMessage(QMailId)));
    connect(svc, SIGNAL(compose(QMailMessage::MessageType, 
                                const QMailAddressList&, 
                                const QString&, 
                                const QString&, 
                                const QContentList&, 
                                QMailMessage::AttachmentsAction)), 
            this, SLOT(composeMessage(QMailMessage::MessageType, 
                                      const QMailAddressList&, 
                                      const QString&, 
                                      const QString&, 
                                      const QContentList&, 
                                      QMailMessage::AttachmentsAction)));
    connect(svc, SIGNAL(compose(QMailMessage)), this, SLOT(composeMessage(QMailMessage)));

    QTimer::singleShot(0, this, SLOT(delayedInit()) );
}

EmailClient::~EmailClient()
{
    delete pm_folder;
    delete pm_trash;
    EmailListItem::deletePixmaps();
    delete emailHandler;
}

void EmailClient::openFiles()
{
    QPerformanceLog(appTitle.toLatin1().constData()) << " : " << "Begin openFiles: "
                      << qPrintable( QTime::currentTime().toString( "h:mm:ss.zzz" ) );
    if ( filesRead ) {
        if ( cachedDisplayMailId.isValid() )
            displayCachedMail();

        return;
    }

    filesRead = true;

    readMail();
    
    if ( cachedDisplayMailId.isValid() ) {
        displayCachedMail();
    } else {
        //No default select for QTreeWidget
        Folder* folder = currentFolder();
        if(!folder)
            folderView()->changeToSystemFolder(MailboxList::InboxString);
        else
            folderSelected( folder );

        displayPreviousMail();
    }

    QPerformanceLog(appTitle.toLatin1().constData()) << " : " << "End openFiles: "
                      << qPrintable( QTime::currentTime().toString( "h:mm:ss.zzz" ) );
}

void EmailClient::displayPreviousMail()
{
    if (!mMessageView)
        return;

    QSettings mailconf("Trolltech","qtmail");
    mailconf.beginGroup("qtmailglobal");
	QMailId id(mailconf.value("currentmail").toULongLong());
    mailconf.endGroup();
    if ( id.isValid() ) {
        messageView()->setSelectedId(id);
    }
}

void EmailClient::displayFolder(const QString &mailbox)
{
    delayedInit();

    if (EmailFolderList *box = mailboxList()->mailbox(mailbox)) {
        if (box->mailbox() == MailboxList::InboxString) {
            // If we just entered the Inbox, we should reset the new message count
            if (emailHandler->newMessageCount())
                resetNewMessages();
        }
    }

    const Folder* folder = currentFolder();
    if (!folder || (folder->mailbox() != mailbox)) {
        folderView()->changeToSystemFolder(mailbox);
    }
    
    showMessageList();
}

void EmailClient::displayCachedMail()
{
    QMailMessage mail(cachedDisplayMailId, QMailMessage::Header);
    EmailFolderList *box = mailboxList()->mailbox(mail.parentFolderId());
    if ( box ) {
        folderView()->changeToSystemFolder( box->mailbox());
        showViewer(cachedDisplayMailId, currentFolder(), (mail.messageType() == QMailMessage::Email));
    }
    cachedDisplayMailId = QMailId();
}

void EmailClient::displayMessage(const QMailId &id)
{
    initialAction = EmailClient::View;
    delayedInit();

    if (!checkMailConflict(tr("Should this message be saved in Drafts before viewing the new message?"), 
                           tr("'View Mail' message will be ignored")) ) {
        cachedDisplayMailId = id;
        openFiles();
    }
}

void EmailClient::delayedShowMessage(QMailAccount::AccountType acct, QMailId id, bool userRequest)
{
    if (initialAction != None) {
        // Ensure we don't close while we're waiting for incoming data
        registerTask("display");
    }

    showMsgList = false;
    showMessageType = acct;
    showMsgId = id;

    newMessagesRequested = userRequest;
    emailHandler->synchroniseClients();
}

void EmailClient::displayRecentMessage()
{
    unregisterTask("display");
    if ( checkMailConflict(
        tr("Should this mail be saved in Drafts before viewing the new message?"),
        tr("'View Mail' message will be ignored")) )
        return;
    
    updateListViews();
    if (!showMsgList)
        queryItemSelected();
    else
        showMessageList();
}

// Ensure the folder list and message list are synchronized with the ReadMail widget
void EmailClient::updateListViews()
{
    openFiles();

    folderView()->changeToSystemFolder(MailboxList::InboxString);

    if (!showMsgList) {
        if (showMsgId.isValid()) {
            messageView()->setSelectedId(showMsgId);
        }
    }
}

bool EmailClient::cleanExit(bool force)
{
    bool result = true;

    if (sending || receiving) {
        if (force) {
            qLog(Messaging) << "EmailClient::cleanExit: forcing cancel to exit";
            cancel();   //abort all transfer
        }
        result = false;
    }

    if (!accountList)
        return result;

    saveSettings();
    accountList->saveAccounts();

    return result;
}

void EmailClient::closeAfterTransmissionsFinished()
{
    closeAfterWrite = false;
    if (!closeAfterTransmissions) {
        closeAfterTransmissions = true;
    }
}

bool EmailClient::isTransmitting()
{
    return nosuspend;
}

void EmailClient::createEmailHandler()
{
    if(emailHandler)
        return;

    //create the email handler
    emailHandler = new EmailHandler();

    //connect it up
    connect(emailHandler, SIGNAL(updateReceiveStatus(const Client*, QString)),
            this, SLOT(updateReceiveStatusLabel(const Client*, QString)) );
    connect(emailHandler, SIGNAL(updateSendStatus(const Client*, QString)),
            this, SLOT(updateSendStatusLabel(const Client*, QString)) );

    connect(emailHandler, SIGNAL(mailboxSize(int)),
            this, SLOT(setTotalPopSize(int)) );
    connect(emailHandler, SIGNAL(downloadedSize(int)),
            this, SLOT(setDownloadedSize(int)) );
    //smtp
    connect(emailHandler, SIGNAL(transferredSize(int)),
            this, SLOT(setTransferredSize(int)) );
    connect(emailHandler, SIGNAL(mailSendSize(int)),
            this, SLOT(setTotalSmtpSize(int)) );
    connect(emailHandler, SIGNAL(mailSent(int)),
            this, SLOT(mailSent(int)) );
    connect(emailHandler, SIGNAL(transmissionCompleted()),
            this, SLOT(transmissionCompleted()) );

    connect(emailHandler, SIGNAL(smtpError(int,QString&)), this,
            SLOT(smtpError(int,QString&)) );
    connect(emailHandler, SIGNAL(popError(int,QString&)), this,
            SLOT(popError(int,QString&)) );
#ifndef QTOPIA_NO_SMS
    connect(emailHandler, SIGNAL(smsError(int,QString&)), this,
            SLOT(smsError(int,QString&)) );
#endif
#ifndef QTOPIA_NO_MMS
    connect(emailHandler, SIGNAL(mmsError(int,QString&)), this,
            SLOT(mmsError(int,QString&)) );
#endif
    connect(emailHandler, SIGNAL(unresolvedUidlList(QString&,QStringList&)),
            this, SLOT(unresolvedUidlArrived(QString&,QStringList&)) );
    connect(emailHandler, SIGNAL(failedList(QStringList&)), this,
            SLOT(failedList(QStringList&)) );

    connect(emailHandler, SIGNAL(mailArrived(QMailMessage)), this,
            SLOT(mailArrived(QMailMessage)) );
    connect(emailHandler, SIGNAL(mailTransferred(int)), this,
            SLOT(allMailArrived(int)) );
    //imap
    connect(emailHandler, SIGNAL(serverFolders()), this,
            SLOT(imapServerFolders()) );
    connect(emailHandler, SIGNAL(nonexistentMessage(QMailId)), this,
            SLOT(nonexistentMessage(QMailId)) );
    connect(emailHandler, SIGNAL(expiredMessages(QStringList, QString, bool)), this,
            SLOT(expiredMessages(QStringList, QString, bool)) );

    connect(emailHandler, SIGNAL(allMessagesReceived()), this,
            SLOT(clientsSynchronised()) );

    //set relevant accounts
#if !defined(QTOPIA_NO_SMS) || !defined(QTOPIA_NO_MMS)
    QListIterator<QMailAccount*> it = accountList->accountIterator();
    while ( it.hasNext() ) {
        QMailAccount *account = it.next();
#ifndef QTOPIA_NO_SMS
        if ( account->accountType() == QMailAccount::SMS )
            emailHandler->setSmsAccount( account );
#endif
#ifndef QTOPIA_NO_MMS
        if ( account->accountType() == QMailAccount::MMS )
            emailHandler->setMmsAccount( account );
#endif
    }
#endif
}

void EmailClient::initActions()
{
    if (selectAccountMenu)
        return; // Already inited

    QMenu *actionContext = QSoftMenuBar::menuFor( mActionView );
    QMenu *folderContext = QSoftMenuBar::menuFor( folderView() );
    QMenu *messageContext = QSoftMenuBar::menuFor( messageView() );

    if (!pm_folder)
        pm_folder = new QIcon(":icon/folder");
    if (!pm_trash)
        pm_trash = new QIcon(":icon/trash");

    selectAccountMenu = new QMenu(mb);
    connect(selectAccountMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(selectAccount(QAction*)));

    getMailButton = new QAction( QIcon(":icon/getmail"), tr("Get all mail"), this );
    connect(getMailButton, SIGNAL(triggered()), this, SLOT(getAllNewMail()) );
    getMailButton->setWhatsThis( tr("Get new mail from all your accounts.") );
    setActionVisible(getMailButton, false);

    cancelButton = new QAction( QIcon(":icon/reset"), tr("Cancel transfer"), this );
    connect(cancelButton, SIGNAL(triggered()), this, SLOT(cancel()) );
    cancelButton->setWhatsThis( tr("Abort all transfer of mail.") );
    setActionVisible(cancelButton, false);

    /* Currently disabled:
    movePop = new QMenu(this);
    copyPop = new QMenu(this);
    connect(movePop, SIGNAL(triggered(QAction*)),
            this, SLOT(moveMailItem(QAction*)));
    connect(copyPop, SIGNAL(triggered(QAction*)),
            this, SLOT(copyMailItem(QAction*)));
    */

    composeButton = new QAction( QIcon(":icon/new"), tr("New"), this );
    connect(composeButton, SIGNAL(triggered()), this, SLOT(compose()) );
    composeButton->setWhatsThis( tr("Write a new message.") );

    lastSearch = new Search();
    QSettings mailconf("Trolltech","qtmail");
    mailconf.beginGroup("lastSearch");
    lastSearch->readSettings( &mailconf );
    mailconf.endGroup();

    searchButton = new QAction( QIcon(":icon/find"), tr("Search"), this );
    connect(searchButton, SIGNAL(triggered()), this, SLOT(search()) );
    searchButton->setWhatsThis( tr("Search for messages in your folders.") );

    settingsAction = new QAction( QIcon(":icon/settings"), tr("Account settings..."), this );
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(settings()));

    emptyTrashAction = new QAction( QIcon(":icon/trash"), tr("Empty trash"), this );
    connect(emptyTrashAction, SIGNAL(triggered()), this, SLOT(emptyTrashFolder()));
    setActionVisible(emptyTrashAction, false);

    moveAction = new QAction( this );
    connect(moveAction, SIGNAL(triggered()), this, SLOT(moveMessage()));
    setActionVisible(moveAction, false);

    copyAction = new QAction( this );
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copyMessage()));
    setActionVisible(copyAction, false);

    selectAllAction = new QAction( tr("Select all"), this );
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAll()));
    setActionVisible(selectAllAction, false);

    deleteMailAction = new QAction( this );
    deleteMailAction->setIcon( *pm_trash );
    connect(deleteMailAction, SIGNAL(triggered()), this, SLOT(deleteMailItem()));
    setActionVisible(deleteMailAction, false);

    actionContext->addAction( searchButton );
    actionContext->addAction( emptyTrashAction );
    actionContext->addAction( settingsAction );

    folderContext->addAction( composeButton );
    folderContext->addAction( getMailButton );
    folderContext->addAction( searchButton );
    folderContext->addAction( cancelButton );
    folderContext->addAction( emptyTrashAction );
    folderContext->addAction( settingsAction );

    messageContext->addAction( composeButton );
    messageContext->addAction( deleteMailAction );
    messageContext->addAction( moveAction );
    messageContext->addAction( copyAction );
    messageContext->addAction( selectAllAction );

    updateAccounts();
}

void EmailClient::updateActions()
{
    openFiles();
    
    // Ensure that the actions have been initialised
    initActions();

    // Only enable empty trash action if the trash has messages in it
    EmailFolderList *trash = mailboxList()->mailbox(MailboxList::TrashString);
    QMailMessage::MessageType type = QMailMessage::AnyType;
    if ( currentMailboxWidgetId() == actionId ) {
        type = nonEmailType;
    } else if (currentMailboxWidgetId() == folderId) {
        type = QMailMessage::Email;
    }
    
    int count = trash->messageCount(EmailFolderList::All, type);
    setActionVisible(emptyTrashAction, (count > 0));

    // Set the visibility for each action to whatever was last configured   
    QMap<QAction*, bool>::iterator it = actionVisibility.begin(), end = actionVisibility.end();
    for ( ; it != end; ++it)
        it.key()->setVisible(it.value());
}

void EmailClient::delayedInit()
{
    if (accountList)
        return; // delayedInit already done
    
    if (initialAction == None) {
        // We have been launched and raised by QPE - we'll start in the actionlist
        pushLocation(UILocation(this, actionId));
    }

    connect( &fetchTimer, SIGNAL(timeout()), this, SLOT(automaticFetch()) );
    accountList = new AccountList(this, "accountList");
    getPath("enclosures/", true);  //create directory enclosures

    sysMessagesChannel =  new QtopiaChannel("QPE/SysMessages", this);
    connect(sysMessagesChannel, SIGNAL(received(QString,QByteArray)),
             this, SLOT(handleSysMessages(QString,QByteArray)));
    
    connect(&showMessageTimer, SIGNAL(timeout()), this,
            SLOT(displayRecentMessage()));

    connect(accountList, SIGNAL(checkAccount(int)),
            this, SLOT(selectAccount(int)) );

    connect(&checkAccountTimer, SIGNAL(timeout()),
            this, SLOT(selectAccountTimeout()) );

    connect(&planeMode, SIGNAL(contentsChanged()),
            this, SLOT(planeModeChanged()) );

    accountList->readAccounts();
    createEmailHandler();
    readSettings();

    // Ideally would make actions functions methods and delay their
    // creation until context menu is shown.
    initActions(); 

    folderView()->setupFolders( accountList );

    QTimer::singleShot(0, this, SLOT(collectSysMessages()) );
    QTimer::singleShot(0, this, SLOT(openFiles()) );
}

void EmailClient::init()
{
    mReadMail = 0;
    mWriteMail = 0;
    selectAccountMenu = 0;
    getMailButton = 0;
    cancelButton = 0;
    /* Currently disabled:
    movePop = 0;
    copyPop = 0;
    */
    composeButton = 0;
    searchButton = 0;
    settingsAction = 0;
    emptyTrashAction = 0;
    moveAction = 0;
    copyAction = 0;
    selectAllAction = 0;
    deleteMailAction = 0;

    mailboxView = new QStackedWidget( this );
    mailboxView->setObjectName( "mailboxView" );

    mActionView = new ActionListView( mailboxView );
    mActionView->setObjectName( "actionView" );
    mActionView->setFrameStyle( QFrame::NoFrame );
    actionId = mailboxView->addWidget( mActionView );

    connect(mActionView, SIGNAL(composeMessage()), 
	    this, SLOT(compose()) );
    connect(mActionView, SIGNAL(emailSelected()), 
	    this, SLOT(showFolderList()) );
    connect(mActionView, SIGNAL(displayFolder(QString)),
	    this, SLOT(displayFolder(QString)) );
    connect(mActionView, SIGNAL(currentFolderChanged(QString)),
	    this, SLOT(currentActionViewChanged(QString)) );

    /* Create context menus for list of folders and messages */
    QMenu *actionContext = QSoftMenuBar::menuFor( mActionView );
    QMenu *folderContext = QSoftMenuBar::menuFor( folderView() );
    QMenu *messageContext = QSoftMenuBar::menuFor( messageView() );

    connect( actionContext, SIGNAL(aboutToShow()), this, SLOT(updateActions()) );
    connect( folderContext, SIGNAL(aboutToShow()), this, SLOT(updateActions()) );
    connect( messageContext, SIGNAL(aboutToShow()), this, SLOT(updateActions()) );

    QtopiaIpcAdaptor::connect(this, SIGNAL(messageCountUpdated()),
                              &messageCountUpdate, MESSAGE(changeValue()));

    setCentralWidget( mailboxView );

    setWindowTitle( appTitle );
}

void EmailClient::update()
{
    QTableWidgetItem *current = messageView()->currentItem();

    if ( current && messageView()->isItemSelected( current ) )
        messageView()->scrollToItem( current );

    //  In case user changed status of sent/unsent or read/unread messages
    if (mFolderView) {
        if ( Folder *folder = currentFolder() ) {
            updateFolderCount( folder->mailbox() );
            contextStatusUpdate();
        }
    }
}

void EmailClient::cancel()
{
    if ( !cancelButton->isEnabled() )
        return;

    emailHandler->cancel();

    emit clearStatus();

    isSending( false );
    isReceiving( false );
}

/*  Called when the user just exits the writemail window.  We don't know what he wanted
    to do, but we should be able to determine it
    Close event is handled by qtmailwindow, so no raise signal is necessary
*/
void EmailClient::autosaveMail(const QMailMessage& mail)
{
    // if uuid is not valid, it's a new mail
    bool isNew = !mail.id().isValid();

    //  Always autosave new messages to drafts folder
    if ( isNew ) {
        saveAsDraft( mail );
    } else {
        // update mail in same mailbox as it was previously stored
        if ( mailboxList()->mailbox(MailboxList::OutboxString)->contains( mail.id() ) ) {
            enqueueMail( mail );
        } else {
            saveAsDraft( mail );
        }
    }
}

/*  Enqueue mail must always store the mail in the outbox   */
void EmailClient::enqueueMail(const QMailMessage& mailIn)
{
    QMailMessage mail(mailIn);

    // if uuid is not valid , it's a new mail
    bool isNew = !mail.id().isValid();

    if ( isNew ) {
        mailResponded();

        if ( !mailboxList()->mailbox(MailboxList::OutboxString)->addMail(mail) ) {
            accessError(mailboxList()->mailbox(MailboxList::OutboxString) );
            return;
        }
    } else {
        // two possibilities, mail was originally from drafts but is now enqueued, or
        // the mail was in the outbox previously as well.
        
        EmailFolderList* draftsFolder = mailboxList()->mailbox(MailboxList::DraftsString);
        EmailFolderList* outboxFolder = mailboxList()->mailbox(MailboxList::OutboxString);

        if (draftsFolder->contains(mail.id()))
            if (!moveMailToFolder(mail.id(), draftsFolder, outboxFolder))
                return;
            
        //have to re-add since this updates any changes to ogl mail.
        //TODO refactor to an explicit update when emailfolderlist is refactored.
        if ( !mailboxList()->mailbox(MailboxList::OutboxString)->addMail( mail ) ) {
            accessError( mailboxList()->mailbox(MailboxList::OutboxString) );
            return;
        }
    }

    if (!closeAfterWrite)
        restoreView();

    if (planeMode.value().toBool()) {
        // Cannot send right now, in plane mode!
        QMessageBox::information(0, 
                                 tr("Airplane safe mode"),
                                 tr("Saved message to Outbox. Message will be sent after exiting Airplane Safe mode."));
    } else {
        sendAllQueuedMail(true);
    }

    if (closeAfterWrite) {
        closeAfterTransmissionsFinished();
        if (isTransmitting()) // prevents flicker
            QTMailWindow::singleton()->hide();
        else
            QTMailWindow::singleton()->close();
    }
}

/*  Simple, do nothing  */
void EmailClient::discardMail()
{
    // Reset these in case user chose reply but discarded message
    repliedFromMailId = QMailId();
    repliedFlags = 0;

    restoreView();

    if (closeAfterWrite) {
        closeAfterTransmissionsFinished();
        if (isTransmitting())
            QTMailWindow::singleton()->hide();
        else
            QTMailWindow::singleton()->close();
    }
}

void EmailClient::saveAsDraft(const QMailMessage& mailIn)
{
    QMailMessage mail(mailIn);

    // if uuid is not valid, it's a new mail
    bool isNew = !mail.id().isValid();

    if ( isNew ) {
        mailResponded();

        if ( !mailboxList()->mailbox(MailboxList::DraftsString)->addMail(mail) ) {
            accessError( mailboxList()->mailbox(MailboxList::DraftsString) );
            return;
        }
    } else {

        // two possibilities, mail was originally from outbox but is now a draft, or
        // the mail was in the drafts folder previously as well.
        
        EmailFolderList* outboxFolder = mailboxList()->mailbox(MailboxList::OutboxString);
        EmailFolderList* draftsFolder = mailboxList()->mailbox(MailboxList::DraftsString);
        
        if (outboxFolder->contains(mail.id()))
            if (!moveMailToFolder(mail.id(), outboxFolder, draftsFolder))
                return;

        //have to re-add since this updates any changes.
        //TODO refactor to an update when emailfolderlist is refactored.
        if( !mailboxList()->mailbox(MailboxList::DraftsString)->addMail( mail ) ) {
            accessError( mailboxList()->mailbox(MailboxList::DraftsString) );
            return;
        }
    }

    restoreView();
}

/*  Mark a message as replied/repliedall/forwarded  */
void EmailClient::mailResponded()
{
    if ( repliedFromMailId.isValid() ) {
        QString mailbox = MailboxList::InboxString;  //default search path
        Folder *folder = currentFolder();
        if ( folder )
            mailbox = folder->mailbox();    //could be trash, etc..

        QMailMessage replyMail(repliedFromMailId,QMailMessage::Header);
        replyMail.setStatus(replyMail.status() | repliedFlags);
        QMailStore::instance()->updateMessage(&replyMail);
    }
    repliedFromMailId = QMailId();
    repliedFlags = 0;
}

/*  Find an appropriate account for the mail and format
    the mail accordingly    */
QMailAccount* EmailClient::smtpForMail(QMailMessage& message)
{
    message.setReplyTo( QMailAddress() );

    /*  Let's see if we the emailAddress matches a SMTP account */
    QMailAddress fromAddress( message.from() );
    QMailAccount *account = accountList->getSmtpRefByMail( fromAddress.address() );
    if ( account != NULL ) {
        message.setFromAccount( account->id() );
        return account;
    }

    /*  Let's try using a default account instead */
    account = accountList->defaultMailServer();
    if ( account != NULL ) {
        return account;
    }

    /* No default either.  Try any and setup a reply-to */
    account = accountList->getSmtpRef();
    if ( account != NULL ) {
        message.setReplyTo( fromAddress );
        message.setFromAccount( account->id() );
        return account;
    }

    /*  No SMTP-account defined */
    return NULL;
}

// send all messages in outbox, by looping through the outbox, sending
// each message that belongs to the current found account
void EmailClient::sendAllQueuedMail(bool userRequest)
{
    if (planeMode.value().toBool()) {
        // Cannot send right now, in plane mode!
        return;
    }

    bool verifiedAccounts = false;
    bool haveValidAccount = false;
    QList<QMailMessage> queuedMessages;
    queuedMailIds.clear();
    smtpAccount = 0;

    EmailFolderList* outbox = mailboxList()->mailbox(MailboxList::OutboxString);
    QMailIdList outgoingIds = outbox->messages();

    int outgoingCount(outgoingIds.count());
    if (outgoingCount == 0)
        return;

    if (userRequest) {
        // Tell the user we're responding
        QString detail;
        if (outgoingCount == 1) {
            QMailMessage mail(*outgoingIds.begin(), QMailMessage::Header);
            detail = mailType(mail);
        } else {
            detail = tr("%1 messages", "%1 >=2").arg(outgoingCount);
        }
        AcknowledgmentBox::show(tr("Sending"), tr("Sending:") + " " + detail);
    }

    foreach(QMailId id, outgoingIds)
    {
        QMailMessage mail(id,QMailMessage::Header);

        // mail not previously sent, and has recipients defined, add to queue
        if ( !(mail.status() & QMailMessage::Sent) 
             && mail.hasRecipients() ) {

            if (mail.messageType() == QMailMessage::Email) {
                // Make sure we have a valid account
                if (!verifiedAccounts) {
                    haveValidAccount = verifyAccounts(true);
                    verifiedAccounts = true;
                    if (!haveValidAccount)
                        qWarning("Queued mail requires valid email accounts but none available.");
                }
                if (!haveValidAccount) {
                    // No valid account.  Move to Drafts and continue.
                    EmailFolderList *outbox = mailboxList()->mailbox(MailboxList::OutboxString);
                    EmailFolderList *drafts = mailboxList()->mailbox(MailboxList::DraftsString);
                    moveMailToFolder(mail.id(), outbox, drafts);
                    continue;
                }
            }

            /* The first mail determines which range of mails to first
               send.  As we allow use of several SMTP accounts we may
               need more than one connection, but the total number of connections
               needed will never exceed the number of SMTP accounts
            */
            if ( !smtpAccount ) {
                smtpAccount = smtpForMail( mail );
                queuedMessages.append(mail);
                queuedMailIds.append( mail.id() );
            } else if ( smtpForMail(mail) == smtpAccount ) {
                queuedMessages.append(mail);
                queuedMailIds.append( mail.id() );
            }
        }

    }

    if (queuedMessages.count() > 0) {
        emailHandler->setSmtpAccount(smtpAccount);
        sending = true;
        setActionVisible(cancelButton, true);
        if (!receiving)
            queueStatus = Sending;

        sendSingle = false;
        isSending(true);

        registerTask("transfer");
        emailHandler->sendMail(queuedMessages);
    } else {
        qWarning("no more messages to send");
    }
}

void EmailClient::sendSingleMail(const QMailMessage& message)
{
    if (sending) {
        qWarning("sending in progress, no action performed");
        return;
    }

    if (planeMode.value().toBool()) {
        // Cannot send right now, in plane mode!
        return;
    }

    bool needAccount = false;
    if ( message.messageType() == QMailMessage::Email )
        needAccount = true;

    if ( needAccount && !verifyAccounts(true) ) {
        qWarning("Mail requires valid email accounts but none available.");

        moveOutboxMailsToDrafts();
        return;
    }

    QList<QMailMessage> queuedMessages;
    queuedMailIds.clear();

    QMailMessage sendMessage = message;

    smtpAccount = smtpForMail( sendMessage);
    queuedMessages.append(sendMessage);
    queuedMailIds.append( sendMessage.id() );
    emailHandler->setSmtpAccount(smtpAccount);

    sending = true;
    setActionVisible(cancelButton, true);
    if (!receiving)
        queueStatus = Sending;

    sendSingle = true;
    isSending(true);

    registerTask("transfer");
    emailHandler->sendMail(queuedMessages);
}

bool EmailClient::verifyAccounts(bool outgoing)
{
    bool ok = true;

    if (accountList->count() == 0) {
        QMessageBox box(tr( "No account selected" ), tr("<qt>You must create an account</qt>"), QMessageBox::Warning,
                        QMessageBox::Ok | QMessageBox::Default , QMessageBox::NoButton, QMessageBox::NoButton );
        box.exec();
        ok = false;
    } else if (outgoing && accountList->getSmtpRef() == NULL) {
        QMessageBox box(tr("No SMTP Server"), tr("<qt>No valid SMTP server defined.<br><br>No emails could be sent.</qt>"), QMessageBox::Warning,
                        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton );
        box.exec();
        ok = false;
    } else if ( !outgoing && mailAccount == NULL ) {
        QMessageBox box(tr("No POP or IMAP accounts defined"), tr("<qt>Get mail only works with POP or IMAP</qt>"), QMessageBox::Warning,
                        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton );
        ok = false;
    }

    return ok;
}

//some mail are obviously sent, but are all mail in the outbox sent
void EmailClient::mailSent(int count)
{
    // We are no longer sending, although we won't update the UI yet, as we
    // may start again... moveMail depends on sending to update status correctly
    sending = false;

    if (count == -1) {
    }
    else {
        // Why doesn't this code check \a count?

        EmailFolderList *mailbox = mailboxList()->mailbox(MailboxList::OutboxString);

        QListIterator<QMailId> qit( queuedMailIds );

        foreach(QMailId id,queuedMailIds)
        { 
            if ( mailbox->contains(id) ) {
                QMailMessage mail(id,QMailMessage::Header);
                mail.setStatus(QMailMessage::Sent,true);
                QMailStore::instance()->updateMessage(&mail);
                if ( !moveMailToFolder(mail.id(), mailbox, mailboxList()->mailbox(MailboxList::SentString) ) )
                    break;      //no point continuing to move
            }
        }

        if ( !sendSingle ) {
            //loop through, if not all messages sent, start over
            QMailIdList outgoingIds = mailboxList()->mailbox(MailboxList::OutboxString)->messages();
            foreach(QMailId id, outgoingIds)
            {
                QMailMessage mail(id,QMailMessage::Header);
                if ( !(mail.status() & QMailMessage::Sent) ) {
                    // We are still sending
                    sending = true;
                    sendAllQueuedMail();
                    return;
                }
            }
        }

        queuedMailIds.clear();
    }

    transmissionCompleted();
    emit clearStatus();
}

void EmailClient::transmissionCompleted()
{
    setActionVisible(cancelButton, false);

    sending = false;
    isSending(false);
}

void EmailClient::addMailToDownloadList(const QMailMessage& mail)
{
    if ( !mailAccount )
	return; // mail check cancelled
    
    if ( mail.status() & QMailMessage::Downloaded 
         || mail.fromAccount() != mailAccount->id() )
        return;

    if ( (mailAccount->maxMailSize() > -1) && (mail.size() > static_cast<uint> ( mailAccount->maxMailSize() * 1024 ) ) )
        return;

    if ( mailAccount->accountType() == QMailAccount::IMAP ) {
        Mailbox *box = mailAccount->getMailboxRef( mail.fromMailbox() );
        if ( box ) {
            FolderSyncSetting fs = box->folderSync();
            if ( fs & Sync_OnlyHeaders ) {
                return;
            } else if ( fs & Sync_OnlyNew ) {
                if ( mail.status() & QMailMessage::ReadElsewhere )
                    return;
            }
        }
    }

    mailDownloadList.sizeInsert(mail.serverUid(), mail.size(), mail.id(), mail.fromMailbox() );
}

void EmailClient::getNewMail()
{
    if ( !verifyAccounts(false) )
        return;

    registerTask("transfer");

    receiving = true;
    previewingMail = true;
    updateGetMailButton(false);
    setActionVisible(cancelButton, true);
    selectAccountMenu->setEnabled(false);

    //get any previous mails not downloaded and add to queue
    mailDownloadList.clear();
    QMailIdList incomingIds = mailboxList()->mailbox(MailboxList::InboxString)->messages(QMailMessage::Downloaded,false);
    foreach(QMailId id, incomingIds){
        QMailMessage mail(id,QMailMessage::Header);
            addMailToDownloadList( mail );
    }

    emailHandler->setMailAccount(mailAccount);
    if (!sending) {
        queueStatus = Receiving;
        emit clearStatus();
    }

    // The retrieval operation can invalidate our cached message list data
    messageListFolder = 0;

    quitSent = false;
    emailHandler->getMailHeaders();
    isReceiving(true);
}

void EmailClient::getAllNewMail()
{
    allAccounts = true;
    accountIdCount = 0;
    mailAccount = accountList->at(accountIdCount);

    while ( mailAccount != NULL ) {
        if ( !mailAccount->canCollectMail() ) {
            accountIdCount++;
            mailAccount = accountList->at(accountIdCount);
        } else
            break;
    }

    getNewMail();
}

void EmailClient::getSingleMail(const QMailMessage& message)
{
    if (receiving) {
        QString user = mailAccount->id();
        if ( user == message.fromAccount() ) {
            mailDownloadList.append(message.serverUid(), message.size(), message.id(), message.fromMailbox() );
        } else {
            qWarning("receiving in progress, no action performed");
        }
        return;
    }
    mailAccount = accountList->getPopRefByAccount( message.fromAccount() );
    if (mailAccount == NULL) {
        QString temp = tr("<qt>Mail was retrieved from account %1<br>Redefine this account to get this mail</qt>").arg(message.fromAccount()) + "</qt>";
        QMessageBox::warning(0, tr("Account not defined"), temp, tr("OK"));
        return;
    }

    registerTask("transfer");

    receiving = true;
    previewingMail = false;
    allAccounts = false;
    updateGetMailButton(false);
    setActionVisible(cancelButton, true);
    selectAccountMenu->setEnabled(false);

    mailDownloadList.clear();
    mailDownloadList.sizeInsert(message.serverUid(), message.size(), message.id(), message.fromMailbox() );
    emailHandler->setMailAccount(mailAccount);
    quitSent = false;

    isReceiving(true);
    emailHandler->getMailByList(&mailDownloadList, true);
}

void EmailClient::unresolvedUidlArrived(QString &user, QStringList &list)
{
    QString msg = tr("<qt>%1<br>The following messages have been deleted "
                     "from the server by another email client and can not be completed:<br>").arg(user);

    QString mailList = "";

    QMailIdList accountIds = mailboxList()->mailbox(MailboxList::InboxString)->messagesFromAccount(*mailAccount);
    foreach(QMailId id, accountIds) {
        QMailMessage mail(id,QMailMessage::Header);
        if (  !(mail.status() & QMailMessage::Downloaded )) { 
            if ( (list.contains( mail.serverUid() ) ) ) {
                QMailAddress fromAddress(mail.from());
                mailList += fromAddress.name() + " - "  + mail.subject() + "<br>";
            }
        }
    }

    QMessageBox::warning(0, tr("Unresolved mail"), msg + mailList + "</qt>", tr("OK"));
}

void EmailClient::readReplyRequested(const QMailMessage& mail)
{
# ifndef QTOPIA_NO_MMS
    QString netCfg;
    QListIterator<QMailAccount*> it = accountList->accountIterator();
    while (it.hasNext()) {
        QMailAccount *account = it.next();
        if (account->accountType() == QMailAccount::MMS) {
            netCfg = account->networkConfig();
            break;
        }
    }
    if ( netCfg.isEmpty() )
        return;

    QWapAccount wapAccount( netCfg );
    if ( wapAccount.mmsDeliveryReport() ) {
        QString msg(tr("<qt>Do you wish to send a Read Reply?</qt>"));
        if (QMessageBox::information(0, tr("Multimedia Message"), msg,
                                     QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                QMailMessage rrmail;
                rrmail.setMessageType(QMailMessage::Mms);
                rrmail.setTo(mail.from());
                rrmail.setSubject(mail.subject());
                rrmail.setHeaderField("X-Mms-Message-Class", "Auto");
                rrmail.setHeaderField("X-Mms-Delivery-Report", "No");
                rrmail.setHeaderField("X-Mms-Read-Reply", "No");
                QString msg = tr("Sent MMS \"%1\" was read on: %2", "%1 = subject %2 = date");
                msg = msg.arg(mail.subject());
                msg = msg.arg(QDateTime::currentDateTime().toString());
                QMailMessagePart part;
                QMailMessageContentType type("text/plain; charset=ISO-8859-1");
                part.setBody(QMailMessageBody::fromData(msg, type, QMailMessageBody::EightBit));
                rrmail.appendPart(part);
                rrmail.setStatus(QMailMessage::Outgoing, true);
                rrmail.setStatus(QMailMessage::Downloaded, true);
                if ( !mailboxList()->mailbox(MailboxList::OutboxString)->addMail(rrmail) ) {
                    accessError(mailboxList()->mailbox(MailboxList::OutboxString) );
                    return;
                }
                sendSingleMail(rrmail);
            }
    }
# endif
    Q_UNUSED(mail);
}

/*  This function is basically here to ensure the header only
    mail is replaced so that the ListWidget remains the same */
void EmailClient::mailUpdated(const QMailId& id, const QString &mailbox)
{
    QMailMessage message(id,QMailMessage::Header);
    if(readMailWidget()->isVisible() )
        readMailWidget()->mailUpdated( message.id() );
    updateQuery( message, mailbox );

    updateFolderCount( mailbox );
    contextStatusUpdate();
}

void EmailClient::mailRemoved(const QMailId &uuid, const QString &mailbox)
{
    Folder *folder = currentFolder();
    if (!folder)
        return;

    if ( folder->mailbox() == mailbox || folderType(folder) == FolderTypeSearch) {
        if (EmailListItem *item = messageView()->getRef( uuid )) {
            int row = messageView()->row( item );
            messageView()->removeRow( row );

            if (int rowCount = messageView()->rowCount())
                if (QTableWidgetItem* nextItem = messageView()->item( qMin(row, (rowCount - 1)), 0))
                    messageView()->setSelectedItem( nextItem );

            messageSelectionChanged();
        }
    }

    updateFolderCount( mailbox );       // Need to update count of associated folder in folderlistview
    contextStatusUpdate();
}

/*  Mail arrived from server, treated a bit differently than from disk */
void EmailClient::mailArrived(const QMailMessage& m)
{   
    //make sure mailbox is connect at this point
    openFiles();
    
    QMailMessage mail(m);

    {
        QtopiaIpcEnvelope e(QLatin1String("QPE/TaskBar"), QLatin1String("setLed(int,bool)"));
        e << LED_MAIL << true;
    }

#ifndef QTOPIA_NO_MMS
    bool newMessages = false;
    bool getNow = false;
#endif

    /*  Test for get-this-mail activated on mail in trash.  Replace
        mail in trash for consistency sake  */
    if ( mail.status() & QMailMessage::Downloaded ) {
        if ( mailboxList()->mailbox(MailboxList::TrashString)->contains( mail.id() ) ) {
            if ( !mailboxList()->mailbox(MailboxList::TrashString)->addMail(mail) ) {
                accessError( mailboxList()->mailbox(MailboxList::TrashString) );
            }
            return;
        }
#ifndef QTOPIA_NO_MMS
        QString mmsType = mail.headerFieldText("X-Mms-Message-Type");
        if (mmsType.contains("m-delivery-ind")) {
            QString msg;
            QString mmsStatus = mail.headerFieldText("X-Mms-Status");
            qLog(Messaging) << "X-MMS-Status:" << mmsStatus;
            if (mmsStatus.contains("Retrieved")) {
                msg = tr("<qt>Multimedia message delivered to %1.</qt>");
            } else if (mmsStatus.contains("Rejected")) {
                msg = tr("<qt>Multimedia message rejected by %1.</qt>");
            } else if (mmsStatus.contains("Deferred")) {
                msg = tr("<qt>Multimedia message deferred by %1.</qt>");
            } else if (mmsStatus.contains("Expired")) {
                msg = tr("<qt>Multimedia message to %1 expired.</qt>");
            }
            QString to = mail.headerFieldText("To");
            if (to.isEmpty())
                to = tr("Unspecified", "MMS recipient");
            QMessageBox::information(0, tr("Multimedia message"), msg.arg(to),
                                     QMessageBox::Yes, QMessageBox::NoButton);
            return;
        } else if (mmsType.contains("m-send-req")) {
            if (QMailAccount *account = accountList->getAccountById(mail.fromAccount())) {
                // If the user has configured automatic download, we just retrieved this message immediately
                if (account->autoDownload()) {
                    unregisterTask("display");
                    newMessages = true;
                }
            }

            if (mail.id().isValid()) {
                // The date of this message is already recorded from the notification 
                QMailMessage existing(mail.id(), QMailMessage::Header);
                mail.setDate(existing.date());
            }
        }

#endif
    } else if (!mail.headerFieldText("X-Mms-Message-Type").trimmed().isEmpty()) {
#ifndef QTOPIA_NO_MMS
        // We will simply process this message, and allow the NewMessages code to 
        // deal with the handling
        emailHandler->acceptMail(mail);

        // Update the count of new MMS messages
        QSettings mailconf("Trolltech","qtmail");
        mailconf.beginGroup("MMS");
        int count = mailconf.value("newMmsCount").toInt() + 1;
        mailconf.setValue("newMmsCount", count);
        mailconf.endGroup();

        {
            QtopiaIpcEnvelope e("QPE/System", "newMmsCount(int)");
            e << count;
        }

        if (QMailAccount *account = accountList->getAccountById(mail.fromAccount())) {
            // If the user has configured automatic download, we should get this message immediately
            getNow = account->autoDownload();
        }
        newMessages = !getNow;
#endif
    }

    if ( !mailboxList()->mailbox(MailboxList::InboxString)->addMail(mail) ) {
        cancel();
        accessError( mailboxList()->mailbox(MailboxList::InboxString) );
    } else {
        Q_ASSERT(mail.id().isValid());

        if (mail.messageType() == QMailMessage::Sms)
            unreadSmsIds.append(mail.id());
    }

#ifndef QTOPIA_NO_MMS
    if (getNow) {
        registerTask("display");
        getSingleMail(mail);
    } else if (newMessages) {
        clientsSynchronised();
    }
#endif
     
    if ( previewingMail ) {
        addMailToDownloadList( mail );
    }
}

// Called two times.  After all headers are fetched and
// after all mails have been picked by list.
void EmailClient::allMailArrived(int)
{
    // not previewing means all mailtransfer has been done
    if (!previewingMail) {

        // close current connection
        if ( !quitSent) {
            quitSent = true;
            emailHandler->popQuit();
            return;
        }

        getNextNewMail();
        return;
    }

    // all headers downloaded from server, start downloading remaining mails
    accountList->saveAccounts();
    previewingMail = false;

    emailHandler->setMailAccount(mailAccount);
    emailHandler->getMailByList(&mailDownloadList, false);
}

void EmailClient::getNextNewMail()
{
    // must use a counter, since several other functions may mess
    // with the current item in accountlist
    accountIdCount++;
    if (accountList->count() <= accountIdCount) {
        mailAccount = 0;
    } else {
        mailAccount = accountList->at(accountIdCount);
        if ( !mailAccount->canCollectMail() ) {
            getNextNewMail();
            return;
        }
    }

    if ( (allAccounts) && (mailAccount != 0) ) {
        getNewMail();
    } else {
        allAccounts = false;
        receiving = false;
        autoGetMail = false;
        updateGetMailButton(true);
        setActionVisible(cancelButton, false);
        selectAccountMenu->setEnabled(true);

        if (queueStatus == Receiving) {
            emit clearStatus();
        }

        if ( Folder *folder = currentFolder() ) {
            updateFolderCount( folder->mailbox() );
            contextStatusUpdate();
        }

        isReceiving(false);
    }
}

void EmailClient::moveMailFront(const QMailMessage& message)
{
    if ( !(message.status() & QMailMessage::Incoming) 
         || (message.status() & QMailMessage::Downloaded) )
        return;

    if ( (receiving) && (message.fromAccount() == mailAccount->id() ) )
        mailDownloadList.moveFront( message.serverUid() );
}

static EmailClient::ErrorMap smtpErrorInit()
{
    // Create a map of (not-yet translated) error strings - should be stored in ROM data
    static const EmailClient::ErrorEntry map[] = 
    {
        { QAbstractSocket::ConnectionRefusedError, QT_TRANSLATE_NOOP( "EmailClient",  "Connection refused" ) },
        { QAbstractSocket::RemoteHostClosedError, QT_TRANSLATE_NOOP( "EmailClient",  "Remote host closed the connection" ) },
        { QAbstractSocket::HostNotFoundError, QT_TRANSLATE_NOOP( "EmailClient",  "Host not found" ) },
        { QAbstractSocket::SocketAccessError, QT_TRANSLATE_NOOP( "EmailClient",  "Permission denied" ) },
        { QAbstractSocket::SocketResourceError, QT_TRANSLATE_NOOP( "EmailClient",  "Insufficient resources" ) },
        { QAbstractSocket::SocketTimeoutError, QT_TRANSLATE_NOOP( "EmailClient",  "Operation timed out" ) },
        { QAbstractSocket::DatagramTooLargeError, QT_TRANSLATE_NOOP( "EmailClient",  "Datagram too large" ) },
        { QAbstractSocket::NetworkError, QT_TRANSLATE_NOOP( "EmailClient",  "Network error" ) },
        { QAbstractSocket::AddressInUseError, QT_TRANSLATE_NOOP( "EmailClient",  "Address in use" ) },
        { QAbstractSocket::SocketAddressNotAvailableError, QT_TRANSLATE_NOOP( "EmailClient",  "Address not available" ) },
        { QAbstractSocket::UnsupportedSocketOperationError, QT_TRANSLATE_NOOP( "EmailClient",  "Unsupported operation" ) },
        { QAbstractSocket::UnknownSocketError, QT_TRANSLATE_NOOP( "EmailClient",  "Unknown error" ) },
    };

    return qMakePair( static_cast<const EmailClient::ErrorEntry*>(map), ARRAY_SIZE(map) );
}

void EmailClient::smtpError(int code, QString &msg)
{
    // Create a map of error text strings, once only
    static ErrorMap errorMap(smtpErrorInit());

    QString temp = tr("<qt>Server: ") + smtpAccount->smtpServer() + "<br><br>";

    if (code == ErrUnknownResponse) {
        temp += tr("Unexpected response from server:<br><br>");
        QStringList list;
        list = msg.split(' ');
        int len = 0;
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
            if ( (*it).length() + len > 35 ) {
                temp +="\n";
                len = 0;
            }
            temp += *it + " ";
            len += (*it).length();
        }
    }

    qLog(Messaging) << "smtpError" << code << msg;

    // Add the error text, if configured
    appendErrorText(temp, code, errorMap);

    temp += "</qt>";

    if (code != ErrCancel) {
        QMessageBox::warning(0, tr("Sending error"), temp, tr("OK") );
        emit clearStatus();
    } else {
        emit updateStatus( tr("Aborted by user") );
    }

    sending = false;
    isSending(false);
    setActionVisible(cancelButton, false);
    queuedMailIds.clear();
}

static EmailClient::ErrorMap popErrorInit()
{
    // Create a map of (not-yet translated) error strings - should be stored in ROM data
    static const EmailClient::ErrorEntry map[] = 
    {
        { ErrLoginFailed, QT_TRANSLATE_NOOP( "EmailClient", "Login failed. Check user name and password") },
        { ErrFileSystemFull, QT_TRANSLATE_NOOP( "EmailClient", "Mail check failed.") },
        { ErrNonexistentMessage, QT_TRANSLATE_NOOP( "EmailClient", "Message deleted from server.") },
        { QAbstractSocket::ConnectionRefusedError, QT_TRANSLATE_NOOP( "EmailClient",  "Connection refused" ) },
        { QAbstractSocket::RemoteHostClosedError, QT_TRANSLATE_NOOP( "EmailClient",  "Remote host closed the connection" ) },
        { QAbstractSocket::HostNotFoundError, QT_TRANSLATE_NOOP( "EmailClient",  "Host not found" ) },
        { QAbstractSocket::SocketAccessError, QT_TRANSLATE_NOOP( "EmailClient",  "Permission denied" ) },
        { QAbstractSocket::SocketResourceError, QT_TRANSLATE_NOOP( "EmailClient",  "Insufficient resources" ) },
        { QAbstractSocket::SocketTimeoutError, QT_TRANSLATE_NOOP( "EmailClient",  "Operation timed out" ) },
        { QAbstractSocket::DatagramTooLargeError, QT_TRANSLATE_NOOP( "EmailClient",  "Datagram too large" ) },
        { QAbstractSocket::NetworkError, QT_TRANSLATE_NOOP( "EmailClient",  "Network error" ) },
        { QAbstractSocket::AddressInUseError, QT_TRANSLATE_NOOP( "EmailClient",  "Address in use" ) },
        { QAbstractSocket::SocketAddressNotAvailableError, QT_TRANSLATE_NOOP( "EmailClient",  "Address not available" ) },
        { QAbstractSocket::UnsupportedSocketOperationError, QT_TRANSLATE_NOOP( "EmailClient",  "Unsupported operation" ) },
        { QAbstractSocket::UnknownSocketError, QT_TRANSLATE_NOOP( "EmailClient",  "Unknown error" ) },
    };

    return qMakePair( static_cast<const EmailClient::ErrorEntry*>(map), ARRAY_SIZE(map) );
}

void EmailClient::popError(int code, QString &msg)
{
    static ErrorMap errorMap(popErrorInit());

    QString temp = tr("<qt>Server: ") + mailAccount->mailServer() + "<br><br>";

    if (code == ErrUnknownResponse) {
        temp += tr("Unexpected response from server:<br><br>" );
        QStringList list;
        list = msg.split(' ');
        int len = 0;
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
            if ( (*it).length() + len > 35 ) {
                temp +="\n";
                len = 0;
            }
            temp += *it + " ";
            len += (*it).length();
        }
    }

    // Add the error text, if configured
    appendErrorText(temp, code, errorMap);

    if (code == ErrFileSystemFull)
        temp += QLatin1String(" ") + LongStream::errorMessage();

    temp += "</qt>";

    if (code != ErrCancel) {
        if ( !autoGetMail ) {
            QMessageBox::warning(0, tr("Receiving error"), temp, tr("OK") );
        } else {
            emit updateStatus( tr("Automatic fetch failed") );
        }
    } else {
        emit updateStatus(tr("Aborted by user"));
    }

    getNextNewMail();
}

void EmailClient::smsError(int code, QString &msg)
{
    Q_UNUSED(code)
    QString temp(tr("<qt>Failed sending SMS: %1</qt>", "%1 will contain the reason for the failure"));
    QMessageBox::warning(0, tr("Sending error"), temp.arg(msg), tr("OK") );

    sending = false;
    isSending(false);
    setActionVisible(cancelButton, false);
    queuedMailIds.clear();
}

void EmailClient::mmsError(int code, QString &msg)
{
#ifndef QTOPIA_NO_MMS
    Q_UNUSED(code)
    if (sending) {
        QString temp(tr("<qt>Failed sending MMS: %1</qt>", "%1 will contain the reason for the failure"));
        QMessageBox::warning(0, tr("Sending error"), temp.arg(msg), tr("OK") );

        sending = false;
        isSending(false);
        queuedMailIds.clear();
    } else if (receiving) {
        QString temp(tr("<qt>Failed receiving MMS: %1</qt>", "%1 will contain the reason for the failure"));
        QMessageBox::warning(0, tr("Receiving error"),
                temp.arg(msg), tr("OK") );
        receiving = false;
        autoGetMail = false;
        isReceiving(false);
        updateGetMailButton(true);
        setActionVisible(cancelButton, false);
        selectAccountMenu->setEnabled(true);
    }
    setActionVisible(cancelButton, false);
#else
    Q_UNUSED(code)
    Q_UNUSED(msg)
#endif
}

QString EmailClient::mailType(const QMailMessage& message)
{
    QString key(QMailComposerFactory::defaultKey(message.messageType()));
    if (!key.isEmpty())
        return QMailComposerFactory::displayName(key);

    return tr("Message");
}

void EmailClient::queryItemSelected()
{
    EmailListItem *item = static_cast<EmailListItem*>(messageView()->currentItem());
    if (item == NULL)
        return;
    if (!messageView()->isItemSelected(item))
        return;

    if (messageView()->currentMailbox() == MailboxList::DraftsString) {
        QMailMessage message = QMailMessage(item->id(),QMailMessage::HeaderAndBody);
        modify(message);
        return;
    }

    showViewer(item->id(), currentFolder(), messageView()->showEmailsOnly());

    if (autoDownloadMail) {
        QMailMessage message = QMailMessage(item->id(),QMailMessage::HeaderAndBody);
        if (!(message.status() & QMailMessage::Downloaded))
            getSingleMail(message);
        autoDownloadMail = false;
    }

    QtopiaIpcEnvelope e( "QPE/TaskBar", "setLed(int,bool)" );
    e << LED_MAIL << false;
}

void EmailClient::resetNewMessages()
{
    // Reading a mail resets the new mail count
    QListIterator<QMailAccount*> it = accountList->accountIterator();
    while ( it.hasNext() ) {
        QMailAccount *acc = it.next();
        Client *client = emailHandler->clientFromAccount(acc);
        if (client) {
            client->resetNewMailCount();
        } else if (acc->accountType() == QMailAccount::System ) {
            QSettings mailconf("Trolltech", "qtmail");
            mailconf.beginGroup("SystemMessages");

            if (mailconf.value("newSystemCount").toInt()) {
                mailconf.setValue("newSystemCount", 0);

                QtopiaIpcEnvelope e("QPE/System", "newSystemCount(int)");
                e << static_cast<int>(0);
            }
        }
    }

    emit messageCountUpdated();
}

void EmailClient::displayNewMessage( const QMailMessage& message )
{
    QString accountId = message.fromAccount();
    QMailAccount *account = accountList->getAccountById( accountId );
	
    if (waitingForNewMessage
	    && showMessageType == QMailAccount::SMS // can't handle MMS fast yet
	    && account
	    && account->accountType() == showMessageType
	    && (showServerId.isEmpty() || 
	        (showServerId == message.serverUid().right(showServerId.length())))) {
    
        showServerId = QString();
        waitingForNewMessage = false;

        if ( checkMailConflict(
             tr("Should this message be saved in Drafts before viewing the new message?"),
             tr("'View Mail' message will be ignored")) )
            return;
        
        // Return to the inbox after viewing this message
        folderView()->changeToSystemFolder(MailboxList::InboxString);

        showMsgId = message.id();
        showViewer(showMsgId, currentFolder(), false);

        updateListViews();
        mReadMail->viewSelectedMail( messageView() );
        static_cast<EmailListItem *>(messageView()->currentItem())->updateState();
    }
}

void EmailClient::mailFromDisk(const QMailId& id, const QString &mailbox)
{
    // Don't create new messageView, i.e. if showing newly arrived message
    if (mMessageView)  {
        // Don't load the message details unless we need them
        if (Folder *folder = currentFolder()) {
            if (folder->mailbox() == mailbox || folderType(folder) == FolderTypeSearch) {
                QMailMessage message(id, QMailMessage::Header);
                updateQuery(message, mailbox);
            }
        }
    }
    
    updateFolderCount( mailbox );
    contextStatusUpdate();
}

void EmailClient::mailMoved(const QMailId& id, const QString& sourceBox, const QString& destBox)
{
    mailRemoved(id, sourceBox);

    // Don't load the message details unless we need them
    if (Folder *folder = currentFolder()) {
        if (folder->mailbox() == destBox || folderType(folder) == FolderTypeSearch) {
            QMailMessage message(id, QMailMessage::Header);
            updateQuery(message, destBox);
        }
    }

    updateFolderCount(destBox);
    contextStatusUpdate();
}

void EmailClient::mailMoved(const QMailIdList& list, const QString& sourceBox, const QString& destBox)
{
    int count(list.count());
    bool displayProgress(count >= MinimumForProgressIndicator);

    suspendMailCount = true;

    if (displayProgress) {
        QString caption;
        if ( count == 1 )
            caption = (tr("Moving 1 message"));
        else
            caption = tr("Moving %1 messages","number of messages always >=2").arg(count);
                
        emit updateProgress(0, count);
        emit updateStatus(caption);
    }

    QTime time;
    int progress = 0;
    foreach (const QMailId& id, list) {
        mailMoved(id, sourceBox, destBox);

        if (displayProgress) {
            ++progress;

            // We still need to process events during this loop
            if ((progress == 1) || (time.elapsed() > ProgressIndicatorUpdatePeriod)) {
                emit updateProgress(progress, count);
                qApp->processEvents();
                time.start();
            }
        }
    }

    if (displayProgress) {
        emit clearStatus();
    }

    suspendMailCount = false;

    updateFolderCount(sourceBox);
    updateFolderCount(destBox);
    contextStatusUpdate();

    messageSelectionChanged();
}

void EmailClient::readMail()
{
    mailboxList()->openMailboxes();

    EmailFolderList* outbox = mailboxList()->mailbox(MailboxList::OutboxString);
    if (outbox->messageCount(EmailFolderList::All)) {
        // There are messages ready to be sent
    	QTimer::singleShot( 0, this, SLOT(sendAllQueuedMail()) );
    }

    countList = mailboxList()->mailboxes();
    if (countList.count())
    	QTimer::singleShot( 0, this, SLOT(incrementalFolderCount()) );
}

void EmailClient::incrementalFolderCount()
{
    if (!countList.count()) {
        updateFolderCount( MailboxList::EmailString );
        contextStatusUpdate();
        return;
    }

    updateFolderCount( countList.takeFirst() );
    contextStatusUpdate();

    QTimer::singleShot( 0, this, SLOT(incrementalFolderCount()) );
}

void EmailClient::accessError(EmailFolderList *box)
{
    QString mailbox = "mailbox"; // No tr

    if ( box )
        mailbox = MailboxList::mailboxTrName( box->mailbox() );

    QString msg = tr("<qt>Cannot access %1. Either there is insufficient space, or another program is accessing the mailbox.</qt>").arg(mailbox);

    QMessageBox::critical( 0, tr("Save error"), msg );
}

void EmailClient::moveError(const EmailFolderList& source, const EmailFolderList& dest)
{
    QString mailbox1 = MailboxList::mailboxTrName( source.mailbox() );
    QString mailbox2 = MailboxList::mailboxTrName( dest.mailbox() );

    QString msg = tr("<qt>Cannot move message from %1 to %2. Either there is insufficient space, or another program is accessing the folders.</qt>").arg(mailbox1).arg(mailbox2);

    QMessageBox::critical( 0, tr("Move error"), msg );
}

void EmailClient::readSettings()
{
    int y;
    QSettings mailconf("Trolltech","qtmail");
    mailconf.beginGroup("qtmailglobal");

    if (( y = mailconf.value("mailidcount", -1).toInt()) != -1) {
        mailIdCount = y;
    }
    mailconf.endGroup();

    mailconf.beginGroup("settings");

    int val = mailconf.value("interval", -1 ).toInt();
    if ( val == -1 ) {
        fetchTimer.stop();
    } else {
        fetchTimer.start( val * 60 * 1000);
    }
    mailconf.endGroup();
}

bool EmailClient::saveSettings()
{
    QSettings mailconf("Trolltech","qtmail");

    mailconf.beginGroup("qtmailglobal");
    mailconf.remove("");
    mailconf.setValue("mailidcount", mailIdCount);
    mailconf.endGroup();

    mailconf.beginGroup("qtmailglobal");

    if ( mailboxView )
        mailconf.setValue( "currentpage", currentMailboxWidgetId() );

    messageView()->writeConfig( &mailconf );

    EmailListItem *item = static_cast<EmailListItem*>(messageView()->currentItem());
    if ( item ) {
        QMailId id = item->id();
        mailconf.setValue("currentmail", id.toULongLong() );
    }

    mailconf.endGroup();
    return true;
}

void EmailClient::selectAccount(int id)
{
    if ( queuedAccountIds.contains( id ) )
        return;
    if ( receiving ) {
        queuedAccountIds.append( id );
        checkAccountTimer.start( 1 * 60 * 1000 );
        return;
    }

    if (accountList->count() > 0) {
        accountIdCount = id;
        mailAccount = accountList->at(id);
        allAccounts = false;
        getNewMail();
    }
}

void EmailClient::selectAccount(QAction* action)
{
    if (actionMap.contains(action))
        selectAccount(actionMap[action]);
}

void EmailClient::selectAccountTimeout()
{
    if ( receiving )
        return;
    if ( queuedAccountIds.isEmpty() ) {
        checkAccountTimer.stop();
        return;
    }

    int accountId = queuedAccountIds.first();
    queuedAccountIds.erase( queuedAccountIds.begin() );
    selectAccount( accountId );
}

void EmailClient::editAccount(int id)
{
    Q_UNUSED( id );
}

void EmailClient::editAccount(QAction* action)
{
    if (actionMap.contains(action))
        editAccount(actionMap[action]);
}

void EmailClient::deleteAccount(int id)
{
    Q_UNUSED( id );
}

void EmailClient::updateGetMailButton(bool enable)
{
    bool visible(false);

    if (enable) {
        // Enable send mail account if SMTP account exists
        QListIterator<QMailAccount*> it = accountList->accountIterator();
        while ( it.hasNext() ) {
            QMailAccount *account = it.next();
            if ( account->accountType() < QMailAccount::SMS ) {
                // Enable send mail account if POP, IMAP, or Synchronized account exists
                visible = true;
                break;
            }
        }
    }

    setActionVisible(getMailButton, visible);
}

/*  Important:  If this method is called directly/indirectly from
    either configure or selectAccountMenu you will get a failure
    when mousemove/release/click events are tried being passed to
    invalid qmenudataitems. (invalid because this procedure clears them)
    Use QTimer:singleshot to dump the call after the mousevents
*/
void EmailClient::updateAccounts()
{
    queuedAccountIds.clear();
    newAccountId = -1;
    updateGetMailButton(true);

    // accounts has been changed, update writemailwidget if it's created
    if ( mWriteMail )
        mWriteMail->setAccountList( accountList );
    if ( mReadMail )
        mReadMail->setAccountList( accountList );
}

void EmailClient::showMessageStatus()
{
    if (suspendMailCount)
        return;

    if (!messageView()->singleColumnMode())
        return;

    if ( currentMailboxWidgetId() != messageId )
        return;

    if (QTMailWindow::singleton()->currentWidget() == this) {
        if (EmailListItem *item = static_cast<EmailListItem*>(messageView()->currentItem())) {
            QMailMessage message(item->id(), QMailMessage::Header);
            QString statusText( EmailListItem::dateToString( message.date().toLocalTime() ) );
            emit updateStatus(statusText);
        } else {
            emit clearStatus();
        }
    }
}

void EmailClient::deleteMail(const QMailMessage& deleteHeader, bool deleting, EmailFolderList* srcFolder)
{
    static EmailFolderList* const trashFolder = mailboxList()->mailbox( MailboxList::TrashString );

    QMailAccount *account = accountList->getAccountById( deleteHeader.fromAccount());

    if ( deleting ) {
        // Add it to queue of mails to be deleted from server
        if ( !deleteHeader.serverUid().isEmpty() ) {
            if ( account && account->deleteMail() ) {
                account->deleteMsg( deleteHeader.serverUid(), deleteHeader.fromMailbox() );
            }
        }

        trashFolder->removeMail( deleteHeader.id() );
    } else {
        if ( account ) {
            // If the client has "deleteImmediately" set, then do so now.
            if ( Client *client = emailHandler->clientFromAccount(account) ) {
                if ( client->hasDeleteImmediately() )
                    client->deleteImmediately( deleteHeader.serverUid() );
            }
        }

        // If mail is in queue for download, remove it from queue if possible
        mailDownloadList.remove( deleteHeader.serverUid() );

        moveMailToFolder( deleteHeader.id(), srcFolder, trashFolder );
    }
}

/*  handles two primary cases.  When a mail being deleted from inbox/outbox view
        it is transferred to trash, and if from trash it is expunged  */
bool EmailClient::deleteMail(EmailListItem *mailItem)
{
    static EmailFolderList* const trashFolder = mailboxList()->mailbox(MailboxList::TrashString);

    Folder *folder = currentFolder();
    if ( folder == NULL ) {
        qWarning("No folder selected, cannot delete mail");
        return false;
    }

    EmailFolderList* srcFolder;
    if (folderType(folder) == FolderTypeSearch)
        srcFolder = containingFolder(mailItem->id());
    else
        srcFolder = mailboxList()->mailbox(folder->mailbox());

    const bool deleting(srcFolder == trashFolder);

    QMailMessage message(mailItem->id(), QMailMessage::Header);
    deleteMail(message, deleting, srcFolder);

    return true;
}

bool EmailClient::deleteMailList(QList<EmailListItem*>& deleteList)
{
    static EmailFolderList* const trashFolder = mailboxList()->mailbox(MailboxList::TrashString);

    Folder *folder = currentFolder();
    if ( folder == NULL ) {
        qWarning("No folder selected, cannot delete mail");
        return false;
    }

    QMailIdList ids;
    foreach (EmailListItem* mailItem, deleteList)
        ids.append(mailItem->id());

    bool deleting;

    EmailFolderList* srcFolder = 0;
    if (folderType(folder) == FolderTypeSearch) {
        // We're only deleting if every message is already in the trash folder;
        // otherwise, we're moving to trash, and leaving those in trash alone
        deleting = true;
        foreach (const QMailId& id, ids) {
            if (!trashFolder->contains(id)) {
                deleting = false;
                break;
            }
        }
    } else {
        srcFolder = mailboxList()->mailbox(folder->mailbox());
        deleting = (srcFolder == trashFolder);
    }

    int count(deleteList.count());
    bool displayProgress(count >= MinimumForProgressIndicator);

    suspendMailCount = true;

    if (displayProgress) {
        QString caption;
        if (deleting) {
            if (deleteList.count() == 1 )
                caption = tr("Deleting message");
            else 
                caption = tr("Deleting messages");
        } else {
            if ( deleteList.count() == 1 )
                caption = tr("Moving message");
            else
                caption = tr("Moving messages");
        }

        emit updateProgress(0, count);
        emit updateStatus(caption);
    }

    QTime time;
    int progress = 0;

    QMailMessageList deleteHeaders = QMailStore::instance()->messageHeaders(QMailMessageKey(ids),
                                                                            QMailMessageKey::Id |
                                                                            QMailMessageKey::ServerUid |
                                                                            QMailMessageKey::FromAccount |
                                                                            QMailMessageKey::FromMailbox);

    foreach (const QMailMessage& deleteHeader, deleteHeaders){ 
        EmailFolderList* location(srcFolder);
        if (!location)
            location = containingFolder(deleteHeader.id());

        if (deleting || (location != trashFolder))
            deleteMail(deleteHeader, deleting, location);

        if (displayProgress) {
            ++progress;

            // We still need to process events during this loop
            if ((progress == 1) || (time.elapsed() > ProgressIndicatorUpdatePeriod)) {
                emit updateProgress(progress, count);
                qApp->processEvents();
                time.start();
            }
        }
    }

    if (displayProgress) {
        emit clearStatus();
    }

    suspendMailCount = false;

    return true;
}

bool EmailClient::moveMailToFolder(const QMailId& id, EmailFolderList *source, EmailFolderList *target)
{
    if ( source == target )
        return false;

    if(!source->moveMail(id,*target))
    {
        moveError(*source,*target);
        return false;
    }

    return true;
}

bool EmailClient::moveMailListToFolder(const QMailIdList& ids, EmailFolderList *target)
{
    QMap<EmailFolderList*, QMailIdList> locations;
    QList<EmailFolderList*> folders;

    foreach (const QString mailbox, mailboxList()->mailboxes()) {
        EmailFolderList* folder(mailboxList()->mailbox(mailbox));
        locations.insert(folder, QMailIdList());
        folders.append(folder);
    }

    // Partition the list of messages by their current location
    QList<EmailFolderList*>::const_iterator begin = folders.begin(), end = folders.end();
    foreach (const QMailId& id, ids) {
        for (QList<EmailFolderList*>::const_iterator it = begin; it != end; ++it)
            if ((*it)->contains(id)) {
                locations[(*it)].append(id);
                break;
            }
    }

    // Move the messages from each location
    QMap<EmailFolderList*, QMailIdList>::const_iterator lit = locations.begin(), lend = locations.end();
    for ( ; lit != lend; ++lit) {
        EmailFolderList* source = lit.key();
        const QMailIdList& ids = lit.value();

        if ((source != target) && !ids.isEmpty()) {
            if (!source->moveMailList(ids, *target)) {
                moveError(*source, *target);
        return false;
    }
        }
    }

    return true;
}

bool EmailClient::moveMailListToFolder(QList<EmailListItem*>& moveList, EmailFolderList *target)
{
    QMailIdList ids;
    foreach (EmailListItem* mailItem, moveList)
        ids.append(mailItem->id());
    return moveMailListToFolder( ids, target );
}

/* Currently disabled:
void EmailClient::showItemMenu(EmailListItem *item)
{
    Q_UNUSED( item );

    Folder *folder = currentFolder();

    if ( folder == NULL )
        return;

    QString mailbox = folder->mailbox();
    QStringList list = mailboxList()->mailboxes();
    list.removeAll( mailbox );
    list.removeAll( MailboxList::OutboxString );

    QMenu *popFolder = new QMenu(this);
    movePop->clear();
    copyPop->clear();
    moveMap.clear();
    QAction *action;
    uint pos = 0;
    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        if (!pm_folder)
            pm_folder = new QIcon(":icon/folder");

        action = movePop->addAction(*pm_folder, MailboxList::mailboxTrName(*it) );
        moveMap.insert(action, pos);
        action = copyPop->addAction(*pm_folder, MailboxList::mailboxTrName(*it) );
        moveMap.insert(action, pos);

        pos++;
    }
    movePop->setTitle( tr("Move to") );
    copyPop->setTitle( tr("Copy to") );
    popFolder->addMenu( movePop );
    popFolder->addMenu( copyPop );
    if (!pm_trash)
        pm_trash = new QIcon(":icon/folder");
    popFolder->addAction(*pm_trash, tr("Delete message"), this, SLOT(deleteMailItem()) );
    popFolder->popup( QCursor::pos() );
}
*/

bool EmailClient::confirmDeleteWithoutSIM(int deleteCount)
{
    QString text;
    if ( deleteCount == 1 )
         text = tr("The SIM card is not ready. Do you want to delete the message without removal from the SIM card?");
    else
         text = tr("The SIM card is not ready. Do you want to delete the messages without removal from the SIM card?");

    return (QMessageBox::warning(0,
                                 tr("SIM not ready"),
                                 text,
                                 QMessageBox::Yes, 
                                 QMessageBox::No) == QMessageBox::Yes);
}

void EmailClient::deleteMailItem()
{
    static EmailFolderList* const outboxFolder = mailboxList()->mailbox( MailboxList::OutboxString );
    static EmailFolderList* const trashFolder = mailboxList()->mailbox( MailboxList::TrashString );

    Folder *folder = currentFolder();
    if (!folder)
        return;

    // Do not delete messages from the outbox folder while we're sending
    if (locationList.contains(outboxFolder) && sending)
        return;

    bool hasSms = false;
    QList<EmailListItem*> deleteList;
    foreach (QTableWidgetItem* item, messageView()->selectedItems()) {
        EmailListItem* messageItem = static_cast<EmailListItem *>(item);
        deleteList.append( messageItem );
        hasSms |= (messageItem->type() == QMailMessage::Sms);
    }

    int deleteCount = deleteList.count();
    if ( deleteCount == 0)
        return;

    const bool deleting((locationList.count() == 1) && (locationList.first() == trashFolder));

    QString action;
    QString actionDetails;
    if ( deleting ) {
        QString item;
        if ( deleteCount == 1  )
            item = tr("1 message");
        else
            item = tr("%1 messages","%1>=2 ->use plural").arg(deleteCount);

        if ( !Qtopia::confirmDelete( this, tr("Email"), item ) )
            return;

        action = tr("Deleting");
        if ( deleteCount == 1 )
            actionDetails = tr("Deleting 1 message");
        else
            actionDetails = tr("Deleting %1 messages", "%1>=2 -> use plural").arg(deleteCount);
    } else {
        // Messages will be removed from SIM on move to Trash
        if (!emailHandler->smsReadyToDelete() && hasSms) {
            if (confirmDeleteWithoutSIM(deleteCount) == false)
                return;
        }

        action = tr("Moving");
        if (deleteCount == 1 )
            actionDetails = tr("Moving 1 message to Trash");
        else
            actionDetails = tr("Moving %1 messages to Trash","%1>=2 ->use plural").arg(deleteCount);
    }

    if (deleteList.count() < MinimumForProgressIndicator) {
        // Tell the user we're doing what they asked for
        AcknowledgmentBox::show(action, actionDetails);
    }

    deleteMailList(deleteList);

    updateFolderCount(folder->mailbox());
    updateFolderCount(trashFolder->mailbox());
    contextStatusUpdate();
}


void EmailClient::moveMailItem(EmailFolderList* destination)
{
    Folder *folder = currentFolder();
    if ( folder == NULL ) 
        return;

    QList<EmailListItem*> moveList;
    EmailListItem *item = 0;
    for (int i = 0; i < messageView()->rowCount(); ++i) {
       item = static_cast<EmailListItem *>(messageView()->item( i, 0 ));
       if ( messageView()->isItemSelected( item ) )
           moveList.append( item );
    }

    if ( moveList.isEmpty() )
        return;

    moveMailListToFolder( moveList, destination );

    updateFolderCount(folder->mailbox());
    updateFolderCount(destination->mailbox());
    contextStatusUpdate();
}

/* Currently disabled:
void EmailClient::moveMailItem(QAction *action)
{
    if (moveMap.contains(action))
        moveMailItem(moveMap[action]);
}
*/

bool EmailClient::copyMailToFolder(const QMailId& id, EmailFolderList *target)
{
    EmailFolderList* source(containingFolder(id));
    if (!source)
        return false;

    if ( source == target )
        return false;

    if (!source->copyMail(id, *target)) {
        accessError(target);
        return false;
    }

    return true;
}

bool EmailClient::copyMailListToFolder(const QList<EmailListItem*>& copyList, EmailFolderList *target)
{
    if (copyList.count() >= MinimumForProgressIndicator) {
        QString caption;
        if ( copyList.count() == 1 )
            caption = tr("Copying message");
        else 
            caption = tr("Copying messages","2 or more messages");
                
        emit updateProgress(0, copyList.count());
        emit updateStatus(caption);
        qApp->processEvents();
    }

    return foreachListElement(&EmailClient::copyMailToFolder, copyList, target);
}

void EmailClient::copyMailItem(EmailFolderList* destination)
{
    Folder *folder = currentFolder();
    if ( folder == NULL ) 
        return;

    QList<EmailListItem*> copyList;
    uint size = 0;

    foreach (QTableWidgetItem* item, messageView()->selectedItems()) {
        EmailListItem* messageItem = static_cast<EmailListItem *>(item);
        copyList.append(messageItem);

        QMailMessage message(messageItem->id(), QMailMessage::Header);
           size += message.size();
       }

    if ( copyList.isEmpty() )
        return;

    if (!LongStream::freeSpace( "", size + 1024*10 )) {
        QString title( tr("Copy error") );
        QString msg( "<qt>" + tr("Storage for messages is full.<br><br>Could not copy messages.") + "</qt>" );
        QMessageBox::warning(0, title, msg, tr("OK") );
        return;
    }

    copyMailListToFolder( copyList, destination );

    updateFolderCount(folder->mailbox());
    updateFolderCount(destination->mailbox());
    contextStatusUpdate();
}

/* Currently disabled:
void EmailClient::copyMailItem(QAction *action)
{
    if (moveMap.contains(action))
        copyMailItem(moveMap[action]);
}
*/

bool EmailClient::foreachListElement(bool (EmailClient::*func)(const QMailId&, EmailFolderList*), 
                                     const QMailIdList& list, EmailFolderList *target)
{
    bool result(true);

    int count(list.count());
    bool displayProgress(count >= MinimumForProgressIndicator);

    suspendMailCount = true;

    QTime time;
    int progress = 0;
    foreach (const QMailId& id, list) {
        result = ( result && (this->*func)( id, target ) );

        if (displayProgress) {
            ++progress;

            // We still need to process events during this loop
            if ((progress == 1) || (time.elapsed() > ProgressIndicatorUpdatePeriod)) {
                emit updateProgress(progress, count);
                qApp->processEvents();
                time.start();
            }
        }
    }

    if (displayProgress) {
        emit clearStatus();
    }

    suspendMailCount = false;

    return result;
}

bool EmailClient::foreachListElement(bool (EmailClient::*func)(const QMailId&, EmailFolderList*), 
                                     const QList<EmailListItem*>& list, EmailFolderList *target)
{
    QMailIdList ids;
    foreach (const EmailListItem* mailItem, list)
        ids.append(mailItem->id());

    return foreachListElement(func, ids, target);
}

void EmailClient::applyToSelectedMessages(void (EmailClient::*function)(EmailFolderList*))
{
    if (!locationList.isEmpty()) {
    QStringList list = mailboxList()->mailboxes();

        // If the message(s) are in a single location, do not permit that as a destination
        if (locationList.count() == 1)
            list.removeAll(locationList.first()->mailbox());

        // Also, do not permit messages to be copied/moved to the Outbox manually
        list.removeAll(MailboxList::OutboxString);

        SelectFolderDialog selectFolderDialog(list);
        QtopiaApplication::execDialog( &selectFolderDialog );

        if ((selectFolderDialog.result() == QDialog::Accepted) &&
            (selectFolderDialog.folder() != -1)) {
            int index = selectFolderDialog.folder();
            (this->*function)(mailboxList()->mailbox(list[index]));
        }
    }
}

void EmailClient::moveMessage()
{
    static EmailFolderList* const outboxFolder = mailboxList()->mailbox( MailboxList::OutboxString );

    // Do not move messages from the outbox folder while we're sending
    if (locationList.contains(outboxFolder) && sending)
        return;

    applyToSelectedMessages(&EmailClient::moveMailItem);
}

void EmailClient::copyMessage()
{
    applyToSelectedMessages(&EmailClient::copyMailItem);
}

/* Select all messages */
void EmailClient::selectAll()
{
    messageView()->selectAll();
}

bool EmailClient::emptyTrashItem(const QMailId& id, EmailFolderList*) 
{
    QMailMessage mail(id,QMailMessage::Header);

    if (mail.status() & QMailMessage::Incoming) {
        if (QMailAccount* account = accountList->getAccountById(mail.fromAccount())) {
            if (account->deleteMail())
                account->deleteMsg(mail.serverUid(), mail.fromMailbox());
        }
    }

    return true;
}

/*  currently only allowed for trash   */
void EmailClient::emptyTrashFolder()
{
    QMailMessage::MessageType type = QMailMessage::AnyType;
    if ( currentMailboxWidgetId() == actionId ) {
        type = nonEmailType;
    } else if (currentMailboxWidgetId() == folderId) {
        type = QMailMessage::Email;
        if (!currentFolder())
            return;
    }

    EmailFolderList* trashFolder = mailboxList()->mailbox(MailboxList::TrashString);

    QString strName = tr("all messages in the trash");
    if (Qtopia::confirmDelete(this, appTitle, strName)) {
        messageView()->clear();

        QMailIdList trashIds = trashFolder->messages(type);
        if (trashIds.count() >= MinimumForProgressIndicator) {
            QString caption;
            if ( trashIds.count() == 1 )
                caption = (tr("Moving 1 message"));
            else
                caption = tr("Moving %1 messages", "number of messages always >=2").arg(trashIds.count());
            
            emit updateProgress(0, trashIds.count());
            emit updateStatus(caption);
            qApp->processEvents();
        }

        trashFolder->empty(type);

        foreachListElement(&EmailClient::emptyTrashItem, trashIds, 0);

        updateFolderCount(MailboxList::TrashString);
        contextStatusUpdate();
    }

    update();
}

void EmailClient::setTotalSmtpSize(int size)
{
    if (queueStatus != Sending && !receiving)
        queueStatus = Sending;

    totalSize = size;
    if (queueStatus == Sending)
        emit updateProgress(0, totalSize);
}

void EmailClient::setStatusText(QString &txt)
{
    emit updateStatus(txt);
}

void EmailClient::setTotalPopSize(int size)
{
    if (queueStatus != Receiving && !sending)
        queueStatus = Receiving;

    totalSize = size;
    if (queueStatus == Receiving)
        emit updateProgress(0, totalSize);
}

void EmailClient::setDownloadedSize(int size)
{
    if (queueStatus != Receiving && !sending)
        queueStatus = Receiving;

    if (queueStatus == Receiving)
        emit updateProgress(size, totalSize);
}

void EmailClient::setTransferredSize(int size)
{
    if (queueStatus != Sending && !receiving)
        queueStatus = Sending;

    if (queueStatus == Sending)
        emit updateProgress(size, totalSize);
}

void EmailClient::updateQuery(const QMailMessage& message, const QString &mailbox)
{
    Folder *folder = currentFolder();
    if (folder == NULL)
        return;

    bool matches(false);
    if ( folder->mailbox() == mailbox ) {
        matches = folder->matchesEmail(message);
    } else {
        if ( folderType(folder) == FolderTypeSearch ) {
            matches = lastSearch->matches(message);
        } else {
        return;
        }
    }

    EmailListItem *item = messageView()->getRef( message.id() );
    if (item != NULL) {
        if ( matches ) {
            item->setId(message.id());
        } else {
            EmailListItem *newItem = item;

            if ( messageView()->isItemSelected(item) ) {
                int row = messageView()->row( item );
                if (row < messageView()->rowCount() - 1) //try below
                    newItem = static_cast<EmailListItem *>(messageView()->item(row + 1, 0));
                if (!newItem && row) //try above
                    newItem = static_cast<EmailListItem *>(messageView()->item(row - 1, 0));
            } else
                newItem = NULL;

            messageView()->removeRow( messageView()->row( item ) );
            if ( newItem ) {
                messageView()->setSelectedItem( newItem );
            }
            return;
        }
    } else if ( matches ) {
        if(messageView()->showEmailsOnly()) {
            if(message.messageType() & QMailMessage::Email)
                messageView()->treeInsert(message.id(), folder->menuLabel());
        } else {
            if(!(message.messageType()  & QMailMessage::Email))
                messageView()->treeInsert(message.id(), folder->menuLabel());
        }
    }
}

void EmailClient::updateReceiveStatusLabel(const Client* client, const QString &txt)
{
    if (queueStatus == Receiving) {
        QString status(txt);
        if (!status.isEmpty()) {
            if (QMailAccount* account = emailHandler->accountFromClient(client)) 
                if (!account->accountName().isEmpty())
                    status.prepend(account->accountName().append(" - "));
        }
        emit updateStatus(status);
    }
}

void EmailClient::updateSendStatusLabel(const Client* client, const QString &txt)
{
    if (queueStatus == Sending) {
        QString status(txt);
        if (!status.isEmpty()) {
            if (QMailAccount* account = emailHandler->accountFromClient(client))
                if (!account->accountName().isEmpty())
                    status.prepend(account->accountName().append(" - "));
        }
        emit updateStatus(status);
    }
}

/* Currently disabled:
void EmailClient::rebuildMoveCopyMenus(const Folder *folder)
{
    //  Rebuild mail move/copy menus as they don't include the currently selected folder
    initActions();
    movePop->clear();
    copyPop->clear();
    QMapIterator<QAction*, int> i(moveMap);
    moveMap.clear();

    QStringList list = mailboxList()->mailboxes();
    QString mailbox = folder->mailbox();
    list.removeAll( mailbox );
    list.removeAll( MailboxList::OutboxString);

    QAction *action;
    uint pos = 0;
    for ( QStringList::Iterator itList = list.begin(); itList != list.end(); ++itList ) {
	if (!pm_folder)
	    pm_folder = new QIcon(":icon/folder");
        action = movePop->addAction(*pm_folder, MailboxList::mailboxTrName(*itList) );
        moveMap.insert(action, pos);
        action = copyPop->addAction(*pm_folder, MailboxList::mailboxTrName(*itList) );
        moveMap.insert(action, pos);

        pos++;
    }
}
*/

void EmailClient::folderSelected(Folder *folder)
{
    if ( !folder )
        return;

    /* Currently disabled:
    rebuildMoveCopyMenus(folder);
    */
    messageView()->setCurrentMailbox( folder->mailbox() );
    messageView()->setSelectedRow( 0 );
    contextStatusUpdate();
}

void EmailClient::folderModified(Folder *folder)
{
    if ( !folder )
        return;

    updateFolderCount(folder->mailbox());
}

/*  make sure that the currently displayed item in readmail is the same
    after the folder has been deleted and added again
    If this should fail the current item will be the first  */
void EmailClient::imapServerFolders()
{
    EmailListItem *item = static_cast<EmailListItem *>(messageView()->currentItem());
    QMailId selected;

    if ( item && messageView()->isItemSelected(item) )
        selected = item->id();

    folderView()->updateAccountFolder(mailAccount);

    if ( selected.isValid() ) {
        messageView()->setSelectedId( selected );
    }
}

void EmailClient::failedList(QStringList &list)
{
    QMessageBox::warning(0, tr("<qt>The following commands failed:<br>%1</qt>").arg(list.join("<br>")), tr("OK"));
    return;
}

void EmailClient::cornerButtonClicked()
{
    if ( !messageView()->horizontalHeader()->isHidden() ) {
        messageView()->horizontalHeader()->hide();
    } else {
        messageView()->horizontalHeader()->show();
    }
}

QMailIdList EmailClient::findMatchingMessages(const Search* search)
{
    QMailIdList matchingIds;

    QMailMessage::MessageType messageType(messageView()->showEmailsOnly() ? QMailMessage::Email : nonEmailType);
    QString mailbox(search->mailbox());
    
    QMailIdList folderIds;
    if (mailbox.isEmpty()) {
        folderIds = MailboxList::messages(messageType, EmailFolderList::DescendingDate);
    } else {
        EmailFolderList* mailFolder = mailboxList()->mailbox(mailbox);
        if (!mailFolder) {
            if (Folder* folder = folderView()->systemFolder(mailbox))
                mailFolder = mailboxList()->mailbox(folder->mailbox());
        }
        folderIds = mailFolder->messages(messageType, EmailFolderList::DescendingDate);
    }

    if (!folderIds.isEmpty()) {
        bool searchBody(!search->getBody().isEmpty());
        int searchCount = folderIds.count();

        // We won't indicate progress if transmission is currently occurring
        //bool displayProgress((searchBody || (searchCount > SearchMinimumForProgressIndicator)) && 
        //                     (!receiving && !sending));

        // Always show progress for searches
        bool displayProgress(true);

        if (displayProgress) {
            emit updateProgress(0, searchCount);
            emit updateStatus(tr("Search") + "...");
        }

        QTime time;
        int progress = 0;
        foreach (const QMailId& id, folderIds) {
            // Only load the message body if we're searching it
            QMailMessage msg(id, (searchBody ? QMailMessage::HeaderAndBody : QMailMessage::Header)); 
            if (search->matches(msg))
                matchingIds.append(msg.id());

            if (displayProgress) {
                ++progress;

                // We still need to process events during this loop
                if ((progress == 1) || (time.elapsed() > ProgressIndicatorUpdatePeriod)) {
                    emit updateProgress(progress, searchCount);
                    qApp->processEvents();
                    time.start();
                }
            }
        }

        if (displayProgress)
            emit clearStatus();
    }

    return matchingIds;
}

void EmailClient::search()
{
    if (!searchView) {
        searchView = new SearchView(false, this);
        searchView->setObjectName("search"); // No tr
        searchView->setModal(true);

        connect(searchView, SIGNAL(finished(int)), this, SLOT(searchSelected(int)));
    }

    searchView->setSearch( lastSearch );
    QtopiaApplication::showDialog(searchView);

    QTimer::singleShot(0, this, SLOT(searchInitiated()));
}

void EmailClient::searchInitiated()
{
    preSearchWidgetId = currentMailboxWidgetId();

    // Clear the message list and make it current, so that we don't
    // return to the action list before displaying results
    messageListFolder = 0;
    messageView()->clear();

    if (preSearchWidgetId != messageId)
        setCurrentMailboxWidget( messageId );
}

void EmailClient::searchSelected(int result)
{
    if (result == QDialog::Accepted) {
        lastSearch = searchView->getSearch();

        if (!lastSearch->mailbox().isEmpty())
            folderView()->changeToSystemFolder(lastSearch->mailbox());
        else
            messageView()->setCurrentMailbox(MailboxList::LastSearchString);

        // Ensure that we refresh the message list content
        showMessageList(SearchResults, true);
        
        QSettings mailconf("Trolltech","qtmail");
        mailconf.beginGroup("lastSearch");
        lastSearch->saveSettings( &mailconf );
        mailconf.endGroup();
    } else {
        if (preSearchWidgetId != messageId)
            setCurrentMailboxWidget( preSearchWidgetId );
    }
}

void EmailClient::automaticFetch()
{
    if ( receiving )
        return;

    qWarning("get all new mail automatic");
    autoGetMail = true;
    getAllNewMail();
}

/*  Someone external are making changes to the mailboxes.  By this time
    we won't know what changes has been made (nor is it feasible to try
    to determine it).  Close all actions which can have become
    invalid due to the external edit.  A writemail window will as such close, but
    the information will be kept in memory (pasted when you reenter the
    writemail window (hopefully the external edit is done by then)
*/
void EmailClient::externalEdit(const QString &mailbox)
{
    cancel();
    showEmailView();
    folderSelected( currentFolder() );

    QString msg = MailboxList::mailboxTrName( mailbox ) + " "; //no tr
    msg += tr("was edited externally");
    emit updateStatus(msg);
}

int EmailClient::currentMailboxWidgetId() const
{
    if (!mailboxView)
        return -1;
    return mailboxView->currentIndex();
}

void EmailClient::setCurrentMailboxWidget(int id )
{
    if ( mailboxView && (id >= 0) ) {
        int oldId = currentMailboxWidgetId();
        mailboxView->setCurrentIndex( id );

        if (id == folderId) {
            if (oldId == actionId)
                folderView()->restoreCurrentFolder();
            messageView()->setShowEmailsOnly(true);
        } else if (id == messageId) {
            if (QTableWidgetItem* item = messageView()->currentItem())
                item->setSelected(true);
        } else if (id == actionId) {
            if (oldId == folderId)
                folderView()->rememberCurrentFolder();

            messageView()->setShowEmailsOnly(false);
            if (QListWidgetItem* item = mActionView->currentItem())
                item->setSelected(true);
            currentActionViewChanged(mActionView->currentFolder());
        }
    }
}

void EmailClient::showEmailView()
{
    update();
    showMessageList();
}

void EmailClient::showFolderList(bool recordLocation)
{
    delayedInit();

    setCurrentMailboxWidget( folderId );

    // Update the folder counts for email folders
    updateFolderCounts();
    
    // Updates the mailListView
    folderSelected( folderView()->currentFolder() );
    showWidget( this, tr("Email") );    

    if (recordLocation) {
        pushLocation(UILocation(this, folderId));
    }
}

void EmailClient::populateMessageView(MessageListContent content)
{
    if (const Folder* folder = currentFolder()) {
        if ((content == Messages) && (folderType(folder) != FolderTypeSystem)) {
            // Find the system folder this folder is a child of
            folder = folderView()->systemFolder(folder->mailbox());
        }

        if (folder != messageListFolder || content != messageListContent) {
            // We need to update the message list content
            messageView()->clear();

            if (messageView()->showEmailsOnly() && (content == Messages))
                messageView()->setShowEmailsOnly(false);
            if (!messageView()->showEmailsOnly() && (content == Emails))
                messageView()->setShowEmailsOnly(true);

            QMailIdList folderIds;
            if (content == SearchResults) {
                folderIds = findMatchingMessages(lastSearch);
            } else {
                if (EmailFolderList* mailFolder = mailboxList()->mailbox(folder->mailbox())) {
                    QMailMessage::MessageType messageType(messageView()->showEmailsOnly() ? QMailMessage::Email : nonEmailType);

                    int type(folderType(folder));
                    if (type == FolderTypeSystem) {
                        folderIds = mailFolder->messages(messageType, EmailFolderList::DescendingDate);  
                    } else if (type == FolderTypeAccount) {
                        folderIds = mailFolder->messagesFromAccount(*static_cast<const QMailAccount*>(folder),
                                                                    messageType,
                                                                    EmailFolderList::DescendingDate);  
                    } else if (type == FolderTypeMailbox) {
                        folderIds = mailFolder->messagesFromMailbox(*static_cast<const Mailbox*>(folder),
                                                                    messageType,
                                                                    EmailFolderList::DescendingDate);  
                    }
                }
            }

            // Insert the selected folder's messages into the message list
            messageView()->treeInsert(folderIds, folder->menuLabel());
            messageView()->setSelectedRow(0);

            messageListFolder = folder;
            messageListContent = content;
        }
    }
}

void EmailClient::showMessageList(MessageListContent content, bool recordLocation)
{
    populateMessageView( content );

    QString title;
    if (content == SearchResults) {
        int count = messageView()->rowCount();
        title = (count == 1 ? tr("1 message") : tr("%1 messages", "%1 >=2").arg(count));
    } else {
        if (Folder* folder = currentFolder()) {
            if ((content == Messages) && (folderType(folder) != FolderTypeSystem)) {
                // Find the system folder this folder is a child of
                folder = folderView()->systemFolder(folder->mailbox());
            }

            title = folder->displayName();
        }
    }

    setCurrentMailboxWidget( messageId );

    showWidget( this, title );

    if (recordLocation) {
        pushLocation(UILocation(this, messageId, content));
    }

    showMessageStatus();
}

void EmailClient::showMessageList(bool recordLocation)
{
    showMessageList(messageView()->showEmailsOnly() ? Emails : Messages, recordLocation);
}

void EmailClient::showActionList(bool recordLocation)
{
    setCurrentMailboxWidget( actionId );

    // Update the folder counts for non-email folders
    updateFolderCounts();
    
    showWidget( this, appTitle );
    if (recordLocation) {
        pushLocation(UILocation(this, actionId));
    }
}

void EmailClient::showComposer(bool recordLocation)
{
    showWidget(mWriteMail);

    if (recordLocation) {
        pushLocation(UILocation(mWriteMail));
    }
}

void EmailClient::showViewer(const QMailId& messageId, Folder* folder, bool email, bool recordLocation)
{
    if ((messageView()->showEmailsOnly() != email) ||
        (currentFolder() != folder)) {
        // Update the view for this folder's situation
        messageView()->clear();
        messageView()->setShowEmailsOnly(email);
        folderView()->setCurrentFolder(folder);
        folderSelected(folder);
    }

    // Ensure the message view has been populated
    MessageListContent content(email ? Emails : Messages);
    if (messageView()->rowCount() == 0)
        populateMessageView(content);

    messageView()->setSelectedId(messageId);
    readMailWidget()->viewSelectedMail(messageView());

    showWidget(mReadMail);

    if (recordLocation) {
        pushLocation(UILocation(mReadMail, messageId, folder, content));
    }
}

void EmailClient::restoreView()
{
    if (!haveLocation()) {
        // If we have never raised the app, we will have no locations
        return;
    }

    popLocation();

    if (!haveLocation()) {
        // We have finished
        closeAfterTransmissionsFinished();
        if (isTransmitting()) // prevents flicker
            QTMailWindow::singleton()->hide();
        else
            QTMailWindow::singleton()->close();
    } else {
        // Clear any status information remaining from the previous location
        emit clearStatus();

        UILocation restoreLocation(currentLocation());

        if (restoreLocation.widget == mWriteMail) {
            showComposer(false);
        } else if (restoreLocation.widget == mReadMail) {
            showViewer(restoreLocation.messageId, restoreLocation.messageFolder, (restoreLocation.content == Emails), false);
        } else {
            if (restoreLocation.widgetId == messageId) {
                showMessageList(restoreLocation.content, false);
            } else {
                if (restoreLocation.widgetId == actionId)
                    showActionList(false);
                else // (restoreLocation.widgetId == folderId)
                    showFolderList(false);
            }
        }
    }
}

bool EmailClient::checkMailConflict(const QString& msg1, const QString& msg2)
{
    if ( writeMailWidget()->isVisible() ) {
        QString message = tr("<qt>You are currently editing a message:<br>%1</qt>").arg(msg1);
        switch( QMessageBox::warning( 0, tr("Messages conflict"), message,
                                      tr("Yes"), tr("No"), 0, 0, 1 ) ) {

            case 0:
            {
                if ( !mWriteMail->saveChangesOnRequest() ) {
                    QMessageBox::warning(0, 
                                        tr("Autosave failed"), 
                                        tr("<qt>Autosave failed:<br>%1</qt>").arg(msg2));
                    return true;
                }
                break;
            }
            case 1: break;
        }
    }
    return false;
}

void EmailClient::writeMailAction(const QMap<QString, QString> propertyMap )
{
    if (isHidden() || !isVisible())
        closeAfterWrite = true;

    if ( checkMailConflict(
            tr("Should it be saved in Drafts before writing the new message?"),
            tr("'Write Mail' message will be ignored")) )
        return;

    QMailMessage mail;

    // Set all the properties defined in the supplied map
    EmailPropertySetter setter( mail );
    setter.setProperties( propertyMap );

    modify( mail );

    openFiles();
}

void EmailClient::smsVCard( const QDSActionRequest& request )
{
    writeSmsAction( QString(), QString(), request.requestData().toString(), true );
    QDSActionRequest( request ).respond();
}

void EmailClient::writeSmsAction(const QString&, const QString& number,
                                 const QString& body, bool vcard)
{
#ifndef QTOPIA_NO_SMS
    if (isHidden() || !isVisible())
        closeAfterWrite = true;

    if ( checkMailConflict(
            tr("Should this message be saved in Drafts before writing the new message?"),
            tr("'Write SMS' message will be ignored")) )
        return;

    if (writeMailWidget()->newMail( QMailComposerFactory::defaultKey( QMailMessage::Sms ), vcard)) {
        if (!number.isEmpty()) {
            writeMailWidget()->setSmsRecipient( number );
        }

        if (!body.isNull()) {
            writeMailWidget()->setBody(body, vcard ? QLatin1String("text/x-vCard")
                                                   : QLatin1String("text/plain"));
        }

        mWriteMail->setAccountList( accountList );
        showComposer();

        openFiles();
    }
#else
    Q_UNUSED(number);
    Q_UNUSED(body);
    Q_UNUSED(vcard);
#endif
}

void EmailClient::writeMailAction(const QString& name, const QString& email)
{
    if (isHidden() || !isVisible())
        closeAfterWrite = true;

    if ( checkMailConflict(
            tr("Should this message be saved in Drafts before writing the new message?"),
            tr("'Write Mail' message will be ignored")) )
        return;

    QString recipient;
    if ( !name.isEmpty() ) {
        recipient = "\"" + name + "\" <";
        if ( email.isEmpty() )
            recipient += "???";
        else
            recipient += email;
        recipient += ">";
    } else if ( !email.isEmpty() ) {
        recipient = "<"+email+">";
    }

    writeMailWidget()->newMail( QMailComposerFactory::defaultKey( QMailMessage::Email ) );
    if ( mWriteMail->composer().isEmpty() ) { 
        // failed to create new composer, maybe due to no email account 
        // being present. So hide/quit qtmail.
        if (isTransmitting()) // prevents flicker
            QTMailWindow::singleton()->hide();
        else
            QTMailWindow::singleton()->close();
        return;
    }
    mWriteMail->setRecipient( recipient );
    mWriteMail->setAccountList( accountList );
    showComposer();

    openFiles();
}

void EmailClient::emailVCard( const QDSActionRequest& request )
{
    QString leafname("email");

    QList<QContact> cardData( QContact::readVCard( request.requestData().data() ) );
    if (!cardData.isEmpty()) {
        const QContact& contact = cardData.first();
        QString name(contact.firstName() + contact.lastName());
        if (!name.isEmpty()) {
            // Remove any non-word chars to ensure we have a valid filename
            leafname = name.remove(QRegExp("\\W"));
        }
    }

    leafname += ".vcf";

    // Save the VCard data to a temporary document
    QString filename = Qtopia::tempDir() + leafname;
    {
        QFile temp( filename );
        if ( !temp.open( QIODevice::WriteOnly ) ) {
            qWarning() << "Unable to open path for write:" << filename;
            return;
        }

        temp.write( request.requestData().data() );
        temp.close();
    }

    QContent doc( filename );
    doc.setName( leafname );
    doc.setRole( QContent::Data );
    doc.commit();

    // write the Email
    composeMessage(QMailMessage::Email, 
                   QMailAddressList(),
                   QString(),
                   QString(),
                   (QContentList() << doc),
                   QMailMessage::CopyAndDeleteAttachments);

    // Respond to the request
    QDSActionRequest( request ).respond();
}

void EmailClient::flashSms( const QDSActionRequest& request )
{
#ifndef QTOPIA_NO_SMS
    // Extract the SMS message from the request payload.
    QByteArray data = request.requestData().data();
    QDataStream stream( data );
    QSMSMessage msg;
    stream >> msg;

    // Process the flash SMS message.
    // TODO
#endif

    // Respond to the request
    QDSActionRequest( request ).respond();
}

void EmailClient::collectSysMessages()
{
    QtopiaIpcEnvelope e ( "QPE/SysMessages", "collectMessages()" );
}

void EmailClient::setEnableMessageActions( bool enabled )
{
    if (!enabled)
        messageSelectionChanged();
}

void EmailClient::writeMessageAction( const QString &name,
                    const QString &addrStr, const QStringList &docAttachments,
                    const QStringList &fileAttachments,
                    int type)
{
    QMailMessage::MessageType diid = QMailMessage::AnyType;
    if ((type == QMailMessage::Sms) || (type == QMailMessage::Email) || (type == QMailMessage::Mms))
        diid = static_cast<QMailMessage::MessageType>(type);

    QList<QMailAddress> addresses;

    if (!addrStr.isEmpty()) {
        foreach (const QMailAddress& addr, QMailAddress::fromStringList(addrStr)) {
            if ((addr.isEmailAddress()) && 
                (addr.name() == addr.address()) &&
                !name.isEmpty()) {
                // This address has no specific name - use the supplied name
                addresses.append(QMailAddress(name, addr.address()));
            } else {
                addresses.append(addr);
            }
        }
    }

    QList<QContent> attachments;
    foreach (const QString& doc, docAttachments)
        attachments.append(QContent(doc, false));
    foreach (const QString& file, fileAttachments)
        attachments.append(QContent(file, false));

    composeMessage(diid, 
                   addresses,
                   QString(),
                   QString(),
                   attachments,
                   QMailMessage::LinkToAttachments);
}

void EmailClient::cleanupMessages( const QDate &removalDate, int removalSize )
{
    bool closeAfterCleanup = isHidden();

    openFiles();
    QStringList mboxList = mailboxList()->mailboxes();
    QStringList::Iterator it;
    for (it = mboxList.begin(); it != mboxList.end(); ++it)
    {
        EmailFolderList *box = mailboxList()->mailbox( *it );
        QMailIdList idList = box->messages();
        foreach(QMailId id, idList)
        {
            QMailMessage mail(id,QMailMessage::Header);
            QDate mailDate( mail.date().toLocalTime().date() );
            uint mailSize( mail.size() );
            if ((mailDate <= removalDate) && (static_cast<int>(mailSize) >= removalSize))
                box->removeMail( mail.id());
        }

    }

    if (closeAfterCleanup) {
        closeAfterTransmissionsFinished();
        if (isTransmitting())
            QTMailWindow::singleton()->hide();
        else
            QTMailWindow::singleton()->close();
    }
}

WriteMail *EmailClient::writeMailWidget()
{
    if ( !mWriteMail ) {
        mWriteMail = new WriteMail( this , "write-mail");
        if ( parentWidget()->inherits("QStackedWidget") )
            static_cast<QStackedWidget*>(parentWidget())->addWidget(mWriteMail);

        connect(mWriteMail, SIGNAL(enqueueMail(QMailMessage)), this,
                SLOT(enqueueMail(QMailMessage)) );
        connect(mWriteMail, SIGNAL(discardMail()), this,
                SLOT(discardMail()) );
        connect(mWriteMail, SIGNAL(saveAsDraft(QMailMessage)), this,
                SLOT(saveAsDraft(QMailMessage)) );
        connect(mWriteMail, SIGNAL(autosaveMail(QMailMessage)), this,
                SLOT(autosaveMail(QMailMessage)) );
        connect(mWriteMail, SIGNAL(noSendAccount(QMailMessage::MessageType)), this,
                SLOT(noSendAccount(QMailMessage::MessageType)) );

        mWriteMail->setAccountList( accountList );
    }

    return mWriteMail;
}

ReadMail *EmailClient::readMailWidget()
{   
    if ( !mReadMail ) {
        mReadMail = new ReadMail( this, "read-message");
        if ( parentWidget()->inherits("QStackedWidget") )
            static_cast<QStackedWidget*>(parentWidget())->addWidget(mReadMail);

        connect(mReadMail, SIGNAL(cancelView()), this, SLOT(restoreView()) );
        connect(mReadMail, SIGNAL(resendRequested(QMailMessage,int)), this,
                SLOT(resend(QMailMessage,int)) );

        connect(mReadMail, SIGNAL(modifyRequested(QMailMessage)),this,
                SLOT(modify(QMailMessage)));
        connect(mReadMail, SIGNAL(removeItem(EmailListItem*)), this,
                SLOT(deleteMailRequested(EmailListItem*)) );
        connect(mReadMail, SIGNAL(viewingMail(QMailMessage)), this,
                SLOT(moveMailFront(QMailMessage)));
        connect(mReadMail, SIGNAL(getMailRequested(QMailMessage)),this,
                SLOT(getSingleMail(QMailMessage)) );
        connect(mReadMail, SIGNAL(sendMailRequested(QMailMessage)),this,
                SLOT(sendSingleMail(QMailMessage)));
        connect(mReadMail, SIGNAL(mailto(QString)), this,
                SLOT(setDocument(QString)) );
        connect(mReadMail,SIGNAL(readReplyRequested(QMailMessage)),this,
                SLOT(readReplyRequested(QMailMessage)));
        connect(mReadMail, SIGNAL(viewingMail(QMailMessage)),
                emailHandler, SLOT(mailRead(QMailMessage)) );

        mReadMail->setAccountList( accountList );
    }

    return mReadMail;
}

void EmailClient::resend(const QMailMessage& message, int type)
{
    repliedFromMailId = message.id();

    if (type == ReadMail::Reply) {
        writeMailWidget()->setAction(WriteMail::Reply);
        repliedFlags = QMailMessage::Replied;
    } else if (type == ReadMail::ReplyToAll) {
        writeMailWidget()->setAction(WriteMail::ReplyToAll);
        repliedFlags = QMailMessage::RepliedAll;
    } else if (type == ReadMail::Forward) {
        writeMailWidget()->setAction(WriteMail::Forward);
        repliedFlags = QMailMessage::Forwarded;
    } else {
        return;
    }

    writeMailWidget()->reply(message, type);
    if ( mWriteMail->composer().isEmpty() ) { 
        // failed to create new composer, maybe due to no email account 
        // being present.
        return;
    }
    showComposer();
}

void EmailClient::modify(const QMailMessage& message)
{
    // Is this type editable?
    QString key(QMailComposerFactory::defaultKey(message.messageType()));
    if (!key.isEmpty()) {
        writeMailWidget()->modify(message);
        if ( mWriteMail->composer().isEmpty() ) { 
            // failed to create new composer, maybe due to no email account 
            // being present.
            return;
        }
        showComposer();
    } else {
        QMessageBox::warning(0,
                             tr("Error"),
                             tr("Cannot edit a message of this type."),
                             tr("OK"));
    }
}


void EmailClient::compose()
{
    delayedInit();

    if (writeMailWidget()->newMail())
        showComposer();
}

void EmailClient::setDocument(const QString &_address)
{
    // strip leading 'mailto:'
    QString address = _address;
    if (address.startsWith("mailto:"))
        address = address.mid(7);

    QMailMessage::MessageType addressType(QMailMessage::Email);
#ifndef QTOPIA_NO_SMS
    QMailAddress recipient(address);
    if (recipient.isPhoneNumber())
        addressType = QMailMessage::Sms; 
#endif

    if (writeMailWidget()->newMail( QMailComposerFactory::defaultKey( addressType ) )) {
        writeMailWidget()->setRecipient(address);
        showComposer();
    }
}

void EmailClient::deleteMailRequested(EmailListItem *item)
{
    if (!item || !item->id().isValid())
        return;

    Folder *folder = currentFolder();
    if ( folderType(folder) == FolderTypeSystem &&
         folder->mailbox() == MailboxList::OutboxString &&
         sending) {
        return; //don't delete when sending
    }

    QMailMessage message(item->id(),QMailMessage::Header);
    QString type = mailType(message);

    // Is the SIM card ready/detected?
    if ((message.messageType() == QMailMessage::Sms) && !emailHandler->smsReadyToDelete()) {
        if (confirmDeleteWithoutSIM(1) == false)
            return;
    }

    bool toTrash(true);
    if (folderType(folder) == FolderTypeSystem
        && folder->mailbox() == MailboxList::TrashString ) {
        if (!Qtopia::confirmDelete( this, appTitle, type ))
            return;

        toTrash = false;
    }

    deleteMail( item );

    // Tell the user we're doing what they asked for
    if (toTrash)
        AcknowledgmentBox::show(tr("Moving"), tr("Moving to Trash: %1", "%1=Email/Message/MMS").arg(type));
    else
        AcknowledgmentBox::show(tr("Deleting"), tr("Deleting: %1","%1=Email/Message/MMS").arg(type));

    restoreView();
}

void EmailClient::showEvent(QShowEvent* e)
{
    Q_UNUSED(e);
    closeAfterTransmissions = false;
}

void EmailClient::isSending(bool y)
{
    if ( y != (nosuspend&1) ) {
        nosuspend ^= 1;
        if ( nosuspend == 1 )
            suspendOk(false);
        else if ( nosuspend == 0 )
            suspendOk(true);
    }

    if (mReadMail)
        mReadMail->isSending(y);

    if (!y)
        moveOutboxMailsToDrafts();

    if (!isTransmitting() && closeAfterTransmissions)
        QTMailWindow::singleton()->close();
    if (!isTransmitting())
        unregisterTask("transfer");

    if (!y && (queueStatus == Sending))
        queueStatus = Inactive;
}

void EmailClient::isReceiving(bool y)
{
    if ( y != (nosuspend&2) ) {
        nosuspend ^= 2;
        if ( nosuspend == 2 )
            suspendOk(false);
        else if ( nosuspend == 0 )
            suspendOk(true);
    }

    if (mReadMail)
        mReadMail->isReceiving(y);

    if (!isTransmitting() && closeAfterTransmissions)
        QTMailWindow::singleton()->close();
    if (!isTransmitting())
        unregisterTask("transfer");

    if (!y && (queueStatus == Receiving))
        queueStatus = Inactive;
}

void EmailClient::suspendOk(bool y)
{
    QtopiaApplication::setPowerConstraint(y ? QtopiaApplication::Enable : QtopiaApplication::DisableSuspend);
}

void EmailClient::moveOutboxMailsToDrafts()
{
//  Move any messages stuck in the outbox to the drafts folder here
    EmailFolderList *outbox = mailboxList()->mailbox(MailboxList::OutboxString);
    EmailFolderList *drafts = mailboxList()->mailbox(MailboxList::DraftsString);
    QMailIdList outboxIds = outbox->messages();
    foreach(QMailId id,outboxIds)
    {
        if ( !moveMailToFolder(id,outbox, drafts ) )
            break;          //no point continuing to move
    }
}

void EmailClient::currentActionViewChanged(const QString &mailbox)
{
    if (mailbox.isEmpty()) {
        emit clearStatus();
    } else {
        updateFolderCount( mailbox );

        if (mailbox == MailboxList::EmailString) {
            emit updateStatus(emailStatusText);
        } else {
            if (mFolderView) {
            if (const Folder* folder = mFolderView->systemFolder(mailbox))
                showFolderStatus(folder);

                messageView()->setCurrentMailbox(mailbox);
            }
        }
    }
}

typedef QMap<const Folder*, QString> FolderStatusMap;
static FolderStatusMap folderStatusMap;

void EmailClient::setFolderStatus(const Folder* folder, const QString& status)
{
    folderStatusMap[folder] = status;
}

void EmailClient::contextStatusUpdate()
{
    if ( receiving || sending )
        return;
        
    if (suspendMailCount)
        return;

    if (mFolderView) {
        // Only update the status if we're the currently visible widget
        if (QTMailWindow::singleton()->currentWidget() == this) {
            int currentId = currentMailboxWidgetId();
            if ( currentId == folderId ) {
                // Show the status of the currently selected folder
                if (Folder* folder = currentFolder())
                    showFolderStatus(folder);
            } else if ( currentId == messageId ) {
                // Show the status of the selected message
                showMessageStatus();
            }
        }
    }
}

void EmailClient::showFolderStatus(const Folder* folder)
{
    FolderStatusMap::const_iterator it = folderStatusMap.find(folder);
    if (it != folderStatusMap.end())
        emit updateStatus(it.value());
}

static QString describeCounts(uint totalCount, uint unreadCount, bool excessTotal = false, bool excessUnread = false)
{
    static const QString excessIndicator(FolderListView::excessIndicator());

    QString countStr;

    if (totalCount) {
        countStr.append(' ');

        if (unreadCount)
            countStr.append(QString("%1%2/%3%4").arg(unreadCount)
                                                .arg(excessUnread ? excessIndicator : "")
                                                .arg(totalCount)
                                                .arg(excessTotal ? excessIndicator : ""));
        else
            countStr.append(QString("%1%2").arg(totalCount).arg(excessTotal ? excessIndicator : ""));
    }

    return countStr;
}

static QPair<uint, uint> accountMessageCounts(EmailFolderList* mb, QMailMessage::MessageType type, const QMailAccount& account)
{
    return qMakePair(mb->messageCount(EmailFolderList::All, type, account),
                     mb->messageCount(EmailFolderList::Unread, type, account));
}

static QPair<uint, uint> mailboxMessageCounts(EmailFolderList* mb, QMailMessage::MessageType type, const Mailbox& mailbox, bool subfolders)
{
    return qMakePair(mb->messageCount(EmailFolderList::All, type, mailbox, subfolders),
                     mb->messageCount(EmailFolderList::Unread, type, mailbox, subfolders));
}

QString EmailClient::describeFolderCount(uint all, uint sub, const QString& type)
{
    QString desc(QString::number(all));

    if (type == "new") {
        desc += " "; //no tr
        desc += tr( "(%1 new)", "%1 = number of new messages" ).arg(sub);
    } else if (type == "unsent" || type == "unfinished") {
        if (sub) {
            desc += " (";
            if (type == "unsent") {
                desc += tr("%1 unsent", "%1 = number of unsent mails" ).arg(sub);
            } else {
                desc += tr("%1 unfinished", "%1 = number of unfinished mails" ).arg(sub);
            }
            desc += ")";
        }
    } else {
        if (sub)
            desc = QString(" %1/%2").arg(sub).arg(all);
        else
            desc = QString(" %1").arg(all);
    }

    return desc;
}

void EmailClient::updateFolderCount(const QString &mailbox)
{
    if (suspendMailCount)
        return;

    if (!accountList)
        return;

    QMailMessage::MessageType messageType;
    uint allCount = 0;
    uint unreadCount = 0;
    QString statusBarText;

    const bool emailContext(currentMailboxWidgetId() == folderId);

    EmailFolderList* mb = mailboxList()->mailbox(mailbox);
    if (!mb) {
        if ( !emailContext && (mailbox == MailboxList::EmailString) ) {
            // Not a real mailbox - describe all emails in the Inbox
            messageType = QMailMessage::Email;
            allCount = MailboxList::messageCount(EmailFolderList::All, messageType);
            unreadCount = MailboxList::messageCount(EmailFolderList::Unread, messageType); 
            emailStatusText = describeFolderCount(allCount, unreadCount, "new"); // notr
        } else {
            return;
        }
    } else {
        messageType = emailContext ? QMailMessage::Email : nonEmailType;
        allCount = mb->messageCount(EmailFolderList::All, messageType);
        unreadCount = mb->messageCount(EmailFolderList::Unread, messageType); 

        // Update status bar text for this folder
        if (( mailbox == MailboxList::InboxString ) ||
            ( mailbox == MailboxList::TrashString )) {
            statusBarText = describeFolderCount(allCount, unreadCount, "new"); // notr
        } else if ( mailbox == MailboxList::DraftsString ) {
            int unsent = mb->messageCount(EmailFolderList::Unsent, messageType);
            if (unsent) {
                statusBarText = describeFolderCount(allCount, unsent, "unsent"); // notr
            } else {
                int unfinished = mb->messageCount(EmailFolderList::Unfinished, messageType);
                if (unfinished) {
                    statusBarText = describeFolderCount(allCount, unfinished, "unfinished"); // notr
                } else {
                    statusBarText = describeFolderCount(allCount);
                }
            }
        } else {
            statusBarText = describeFolderCount(allCount);
        }
    }

    // Leave the item status area blank if there are no messages
    const QString itemStatus = (allCount ? describeFolderCount(allCount, unreadCount) : QString());

    // Update the status of the folder item
    if (emailContext) {
        folderView()->updateFolderStatus( mailbox, itemStatus, (unreadCount > 0), NoIcon );

        // If this is the inbox folder, update any IMAP sub-folders
        if ( mailbox == MailboxList::InboxString ) {
            QListIterator<QMailAccount*> it = accountList->accountIterator();
            while ( it.hasNext() ) {
                const QMailAccount *account = it.next();

                // Update the account status
                QPair<uint, uint> count( accountMessageCounts( mb, messageType, *account ) );
                QString countText( describeCounts( count.first, count.second ) );
                folderView()->updateAccountStatus( account, countText, count.second, NoIcon );

                // Update the status bar text for this account
                if (const Folder* folder = folderView()->accountFolder( account ))
                    setFolderStatus(folder, describeFolderCount( count.first, count.second, "new" )); // notr

                // Update each individual mailbox count
                foreach (const Mailbox* box, account->mailboxes()) {
                    QPair<uint, uint> count( mailboxMessageCounts( mb, messageType, *box, false ) );
                    QString countText;

                    if (const QTreeWidgetItem* item = folderView()->accountFolderItem( account, box->pathName() )) {
                        if (item->childCount()) {
                            // We need to indicate if there are messages hidden beneath this folder
                            QPair<uint, uint> inclusiveCount( mailboxMessageCounts( mb, messageType, *box, true ) );
                            countText = QString( describeCounts( count.first, 
                                                                 count.second, 
                                                                 (inclusiveCount.first > count.first), 
                                                                 (inclusiveCount.second > count.second) ) );
                        }
                    }

                    if (countText.isEmpty())
                        countText = QString( describeCounts( count.first, count.second ) );

                    folderView()->updateAccountStatus( account, countText, count.second, NoIcon, box->pathName() );
                    
                    if (const Folder* folder = folderView()->accountFolder( account, box->pathName() ))
                        setFolderStatus(folder, describeFolderCount( count.first, count.second, "new" )); // notr
                }
            }
        }
    } else {
        mActionView->updateFolderStatus( mailbox, itemStatus, NoIcon );
    }

    // Update the folder status text for this folder
    if (const Folder* folder = folderView()->systemFolder(mailbox)) {
        setFolderStatus(folder, statusBarText);
    }

    // If this is the trash folder, then it can be emptied if it has at least 1 mail
    if (emptyTrashAction && mailbox == MailboxList::TrashString) {
        setActionVisible( emptyTrashAction, enableMessageActions );
    }
}

void EmailClient::updateFolderCounts()
{
    foreach (const QString& mailbox, mailboxList()->mailboxes())
        updateFolderCount(mailbox);
}

void EmailClient::settings()
{
    AccountSettings settings(accountList, this, "create-account", true);
    connect(&settings, SIGNAL(changedAccount(QMailAccount*)),
            this, SLOT(changedAccount(QMailAccount*)));
    connect(&settings, SIGNAL(deleteAccount(QMailAccount*)),
            this, SLOT(deleteAccount(QMailAccount*)));

    QListIterator<QMailAccount*> it = accountList->accountIterator();
    bool addAccount = true;
    while ( it.hasNext() ) {
        QMailAccount::AccountType accountType = it.next()->accountType();
        if ( accountType != QMailAccount::SMS ) {
            addAccount = false;
            break;
        }
    }
    if (addAccount)
        settings.addAccount();

    settings.showMaximized();
    QtopiaApplication::execDialog(&settings);
    QTimer::singleShot(0, this, SLOT(updateAccounts()) );
}

void EmailClient::changedAccount(QMailAccount *account)
{
    QTimer::singleShot(0, this, SLOT(updateAccounts()));
    folderView()->updateAccountFolder(account);
    accountList->saveAccounts();
}

bool EmailClient::removeMailFromFolder(const QMailId& id, EmailFolderList* location) 
{
    location->removeMail(id);
    return true;
}

void EmailClient::deleteAccount(QMailAccount *account)
{
    folderView()->deleteAccountFolder(account);

    // Remove the messages from this account in the Inbox
    EmailFolderList* inbox(mailboxList()->mailbox(MailboxList::InboxString));
    QMailIdList removedIds = inbox->messagesFromAccount(*account);
    if (removedIds.count() >= MinimumForProgressIndicator) {
        emit updateProgress(0, removedIds.count());
        emit updateStatus(tr("Deleting messages"));
        qApp->processEvents();
    }

    foreachListElement(&EmailClient::removeMailFromFolder, removedIds, inbox);
        
    accountList->remove(account);
    accountList->saveAccounts();

    QTimer::singleShot(0, this, SLOT(updateAccounts()));
}

FolderListView* EmailClient::folderView()
{
    if ( !mFolderView ) {
        mFolderView = new FolderListView(mailboxList(), mailboxView, "read-email");
        folderId = mailboxView->addWidget( mFolderView );

        connect(mFolderView, SIGNAL(viewMessageList()), this, SLOT(showMessageList()) );
        connect(mFolderView, SIGNAL(folderSelected(Folder*)), this, SLOT(folderSelected(Folder*)) );
        connect(mFolderView, SIGNAL(folderModified(Folder*)), this, SLOT(folderModified(Folder*)) );
        connect(mFolderView, SIGNAL(emptyFolder()), this, SLOT(emptyTrashFolder()) );
        connect(mFolderView, SIGNAL(finished()), this, SLOT(restoreView()) );
        
        /*  Folder and Message View specific init not related to placement  */
        QStringList columns;
        columns << tr( "Folders" );
        mFolderView->setColumnCount( columns.count() );
        mFolderView->setHeaderLabels( columns );
        mFolderView->setRootIsDecorated( false );

        QHeaderView *header = mFolderView->header();
        header->setMovable( false );
        header->setClickable( false );

#ifndef QTOPIA_PHONE
        QAction *fvWhatsThis = QWhatsThis::createAction( mFolderView );
        fvWhatsThis->setText( tr("A list of your folders.  You can tap Outbox "
                     "and then tap the Messages tab to see the "
                     "messages currently in the outbox.") );
#endif

        mFolderView->header()->resizeSection( 0, QApplication::desktop()->availableGeometry().width() );
        
        QSettings mailconf("Trolltech","qtmail");
        QFont font;
        mailconf.beginGroup("settings");
        if ( mailconf.value("font").toString() == "large") {
            font.setPointSize( font.pointSize() + 4 );      // 4 larger than default
        }
        mailconf.endGroup();
        mFolderView->setFont( font );

        QTimer::singleShot(0, this, SLOT(openFiles()) );
    }
    return mFolderView;
}

MailListView* EmailClient::messageView()
{
    if ( !mMessageView ) {
        mMessageView = new MailListView(mailboxView, "select-message" );
        connect(mMessageView, SIGNAL(itemClicked(QTableWidgetItem*)),
                this, SLOT(queryItemSelected()) );
        // Not sure how this is supposed to work - disable until UI is standardised:
        /*
        connect(mMessageView, SIGNAL(itemPressed(EmailListItem*)),
                this, SLOT(showItemMenu(EmailListItem*)) );
        */
        connect(mMessageView, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)),
                this, SLOT(showMessageStatus()) );
        connect(mMessageView, SIGNAL(enableMessageActions(bool)),
                this, SLOT(setEnableMessageActions(bool)) );
        connect(mMessageView, SIGNAL(itemSelectionChanged()),
                this, SLOT(messageSelectionChanged()) );
        connect(mMessageView, SIGNAL(backPressed()),
                this, SLOT(restoreView()) );

        messageId = mailboxView->addWidget(mMessageView);

        QSettings mailconf("Trolltech","qtmail");
        mailconf.beginGroup("qtmailglobal");
        messageView()->readConfig( &mailconf );
        mailconf.beginGroup("settings");

        QFont font;
        if ( mailconf.value("font").toString() == "large") {
            font.setPointSize( font.pointSize() + 4 );  // 4 larger than default
        }
        mailconf.endGroup();
        mMessageView->setFont( font );

        displayPreviousMail();
    }
    return mMessageView;
}

MailboxList* EmailClient::mailboxList()
{
    if ( !mMailboxList ) {
        mMailboxList = new MailboxList(this);

        connect(mMailboxList, SIGNAL(stringStatus(QString&)), this,
            SLOT(setStatusText(QString&)) );
        
        //connect after mail has been read to speed up reading */
        connect(mMailboxList, SIGNAL(mailAdded(QMailId,QString)), 
            this,SLOT(mailFromDisk(QMailId,QString)) );
        connect(mMailboxList, SIGNAL(mailUpdated(QMailId,QString)), 
            this,SLOT(mailUpdated(QMailId,QString)) );
        connect(mMailboxList, SIGNAL(mailRemoved(QMailId,QString)), 
            this,SLOT(mailRemoved(QMailId,QString)) );
        connect(mMailboxList,SIGNAL(mailMoved(QMailId,QString,QString)),
            this,SLOT(mailMoved(QMailId,QString,QString)));
        connect(mMailboxList,SIGNAL(mailMoved(QMailIdList,QString,QString)),
            this,SLOT(mailMoved(QMailIdList,QString,QString)));
        connect(mMailboxList, SIGNAL(externalEdit(QString)), 
            this,SLOT(externalEdit(QString)) );
    }
    return mMailboxList;
}

void EmailClient::handleSysMessages(const QString &message, const QByteArray &data)
{
    if (message == "postMessage(int,QDateTime,QString,QString)")
    {
        QDataStream ds(data);
        int messageId;
        QDateTime time;
        QString subject;
        QString text;
        ds >>  messageId >> time >> subject >> text;

        QMailMessage mail;
        mail.setMessageType( QMailMessage::System );
        mail.setStatus( QMailMessage::Downloaded, true );
        mail.setStatus( QMailMessage::Incoming, true );
        mail.setDate( QMailTimeStamp( time ) );
        mail.setSubject( subject );
        QMailMessageContentType type( "text/plain; charset=UTF-8" );
        mail.setBody( QMailMessageBody::fromData( text, type, QMailMessageBody::Base64 ) );
        mail.setFromAccount( "@System" );
        mail.setHeaderField("From", "System");
        mailArrived( mail );

        QtopiaIpcEnvelope e("QPE/SysMessages", "ackMessage(int)");
        e << messageId;
    }
}

void EmailClient::appendErrorText(QString& message, const int code, const ErrorMap& map)
{
    const ErrorEntry *it = map.first, *end = map.first + map.second; // ptr arithmetic!

    for ( ; it != end; ++it)
        if (it->code == code)
        {
            message += tr( it->text );
            return;
        }
}

void EmailClient::planeModeChanged() 
{
    if (planeMode.value().toBool() == false) {
        // We have left airplane mode
        qLog(Messaging) << "Leaving Airplane Safe Mode";

        EmailFolderList* outbox = mailboxList()->mailbox(MailboxList::OutboxString);
        if (outbox->messageCount(EmailFolderList::All)) {
            // Send any queued messages
            sendAllQueuedMail();
        }
    }
}

void EmailClient::messageSelectionChanged()
{
    static EmailFolderList* const trashFolder = mailboxList()->mailbox( MailboxList::TrashString );

    if (!moveAction)
        return; // initActions hasn't been called yet

    if (suspendMailCount)
        return;

    locationList.clear();

    uint count = messageView()->rowCount();

    QList<const EmailListItem*> selectedItems;
    foreach (const QTableWidgetItem* item, messageView()->selectedItems()) {
        const EmailListItem* messageItem = static_cast<const EmailListItem *>(item);
        selectedItems.append( messageItem );
    }

    uint selected = selectedItems.count();
    if (selected > 0) {
        if (Folder* folder = currentFolder()) {
            int type(folderType(folder));
            if (type == FolderTypeSystem) {
                locationList.append(mailboxList()->mailbox(folder->mailbox()));
            } else if (type == FolderTypeSearch) {
                foreach (const EmailListItem* item, selectedItems) {
                    EmailFolderList* location = containingFolder(item->id());
                    if (!locationList.contains(location))
                        locationList.append(location);
                }
            } else if (type == FolderTypeAccount || type == FolderTypeMailbox) {
                locationList.append(mailboxList()->mailbox(MailboxList::InboxString));
            }
        }

        // We can delete only if all selected messages are in the Trash folder
        if ((locationList.count() == 1) && (locationList.first() == trashFolder)) {
            if (selected == 1 )
                deleteMailAction->setText(tr("Delete message"));
            else 
                deleteMailAction->setText(tr("Delete messages"));
        } else {
            deleteMailAction->setText(tr("Move to Trash"));
        }
        if ( selected == 1 ) {
            moveAction->setText(tr("Move message...", ""));
            copyAction->setText(tr("Copy message...", ""));
        } else {
            moveAction->setText(tr("Move messages...", ">=2 messages"));
            copyAction->setText(tr("Copy messages...", ">=2 messages"));
        }
    } 

    // Ensure that the per-message actions are hidden, if not usable
    setActionVisible(selectAllAction, (count > 0 && count != selected));
    setActionVisible(deleteMailAction, (selected != 0));
    setActionVisible(moveAction, (selected != 0));
    setActionVisible(copyAction, (selected != 0));
}

void EmailClient::showWidget(QWidget* widget, const QString& title)
{
    emit statusVisible(widget == this);
    emit raiseWidget(widget, title);
}

void EmailClient::newMessages(bool userRequest)
{
    if (!QtopiaApplication::instance()->willKeepRunning()) {
        initialAction = IncomingMessages;
    }

    delayedInit();
    openFiles();
    showServerId = QString::null;

    delayedShowMessage(QMailAccount::SMS, QMailId(), userRequest);
}

static bool lessThanByTimestamp(const QMailId& lhs, const QMailId& rhs)
{
    QMailMessage lhsMessage(lhs, QMailMessage::Header);
    QMailMessage rhsMessage(rhs, QMailMessage::Header);

    return (lhsMessage.date() < rhsMessage.date());
}

void EmailClient::clientsSynchronised()
{
    // We now have an updated incoming message count
    emit messageCountUpdated();

    // Are these new SMS messages, or were they put on the SIM by a previous phone?
    int newSmsCount = emailHandler->unreadSmsCount();
    if (unreadSmsIds.count() > newSmsCount) {
        // We need to remove the n most recent
        qSort(unreadSmsIds.begin(), unreadSmsIds.end(), lessThanByTimestamp);
        while (newSmsCount-- > 0)
            unreadSmsIds.removeLast();

        foreach (const QMailId& id, unreadSmsIds) {
            // This is not really a new message, mark it as read
            QMailMessage oldMail(id, QMailMessage::Header);
            oldMail.setStatus(QMailMessage::Read, true);
            QMailStore::instance()->updateMessage(&oldMail);
        }
    }
    unreadSmsIds.clear();

    // Are we responding to a raise request from QPE?
    bool respondingToRaise(initialAction == IncomingMessages);

    int newMessageCount = emailHandler->newMessageCount();
    if (newMessageCount != 0 || respondingToRaise) {
        if (newMessagesRequested || newMessageCount == 0) {
            newMessagesRequested = false;

            // Just go to the next stage
            viewNewMessages(respondingToRaise);

            unregisterTask("display");
        } else {
            // Start the message ring
            QtopiaServiceRequest req("Ringtone", "startMessageRingtone()");
            req.send();

            QString text(newMessageCount == 1 ? tr("A new message has arrived. Do you wish to read it now?")
                                              : tr("%1 new messages have arrived. Do you wish to view them now?").arg(newMessageCount) );

            if (newMessagesBox) {
                // Update the text and restart the timer
                newMessagesBox->setText(text);
            } else {
                // Ask the user whether to view the message(s)
                newMessagesBox = new QMessageBox(QMessageBox::Information, tr("New message"), text, QMessageBox::Yes | QMessageBox::No);
                connect(newMessagesBox, SIGNAL(finished(int)), this, SLOT(newMessageAction(int)));
                QtopiaApplication::showDialog(newMessagesBox);

                connect(&newMessageResponseTimer, SIGNAL(timeout()), this, SLOT(abortViewNewMessages()));
            }

            if (NotificationVisualTimeout)
                newMessageResponseTimer.start(NotificationVisualTimeout);
        }
    }
}

void EmailClient::viewNewMessages(bool respondingToRaise)
{
    bool savedAsDraft(false);
    int newMessageCount = emailHandler->newMessageCount();

    // Having chosen to view new messages, we should reset the new message count
    resetNewMessages();

    if (!respondingToRaise) {
        // We were already operating when this new message notification arrived; if we 
        // are composing, we need to save as draft
        if (mWriteMail && 
            (QTMailWindow::singleton()->currentWidget() == mWriteMail)) {
            savedAsDraft = mWriteMail->forcedClosure();
        }
    }

    if (newMessageCount == 1) {
        // Find the newest incoming message and display it
        EmailFolderList* inbox(mailboxList()->mailbox(MailboxList::InboxString));

        QMailIdList unreadList = inbox->messages(QMailMessage::Read,
                                                 false,
                                                 nonEmailType, 
                                                 EmailFolderList::DescendingDate);
        if(!unreadList.isEmpty())
        {
            // We need to change to the correct message view, to control the context menu!
            folderView()->changeToSystemFolder(MailboxList::InboxString);
            showViewer(unreadList.first(), currentFolder(), false);
        }
    }
    else {
        viewInbox();
    }

    if (savedAsDraft) {
        // The composer had a partial message, now saved as a draft
        AcknowledgmentBox::show(tr("Saved to Drafts"), tr("Incomplete message has been saved to the Drafts folder"));
    }
}

void EmailClient::viewInbox()
{
    // Show the inbox with the new messages
    messageView()->setShowEmailsOnly(false);
    folderView()->changeToSystemFolder(MailboxList::InboxString);
    messageView()->setSelectedRow(0);

    showMessageList();
}

void EmailClient::newMessageAction(int choice)
{
    newMessageResponseTimer.stop();

    // Stop the message ring, if necessary
    QtopiaServiceRequest req("Ringtone", "stopMessageRingtone()");
    req.send();

    // Are we responding to a raise request from QPE?
    bool respondingToRaise(initialAction == IncomingMessages);
    if (respondingToRaise) {
        // If new messages arrive during this session, do not trigger the ringtone again
        initialAction = None;
    }

    if (choice == QMessageBox::Yes)
        viewNewMessages(respondingToRaise);

    unregisterTask("display");

    newMessagesBox->deleteLater();
    newMessagesBox = 0;
}

void EmailClient::abortViewNewMessages()
{
    newMessagesBox->setResult(QMessageBox::No);
    newMessagesBox->reject();
}

#ifndef QTOPIA_NO_MMS

void EmailClient::mmsMessage(const QDSActionRequest& request)
{
#ifndef QTOPIA_NO_SMS
    if (!QtopiaApplication::instance()->willKeepRunning())
        initialAction = IncomingMessages;

    delayedInit();

    emailHandler->pushMmsMessage(request);
#endif

    // Respond to the request
    QDSActionRequest( request ).respond();
}

#endif

void EmailClient::noSendAccount(QMailMessage::MessageType type)
{
    QString key(QMailComposerFactory::defaultKey(type));
    QString name(QMailComposerFactory::name(key));

    QMessageBox::warning(0,
                         tr("Send Error"), 
                         tr("%1 cannot be sent, because no account has been configured to send with.","%1=MMS/Email/TextMessage").arg(name),
                         QMessageBox::Ok);
}

void EmailClient::nonexistentMessage(const QMailId& id)
{
    // Mark this message as deleted
    QMailMessage deletedMail(id, QMailMessage::Header);
    deletedMail.setStatus(QMailMessage::Removed, true);
    QMailStore::instance()->updateMessage(&deletedMail);

    if (readMailWidget()->isVisible())
        mReadMail->mailUpdated(id);

    QMessageBox::warning(0,
                         tr("Message deleted"), 
                         tr("Message cannot be downloaded, because it has been deleted from the server."),
                         QMessageBox::Ok);
}

void EmailClient::expiredMessages(const QStringList& serverUids, const QString& mailbox, bool locationExists)
{
    if (mailAccount) {
        foreach (const QString& uid, serverUids) {
            QMailMessage deletedMail(uid, mailAccount->id(), QMailMessage::Header);

            if (deletedMail.id().isValid()) {
                if (locationExists) {
                    if (!(deletedMail.status() & QMailMessage::Removed)) {
                        // Mark this message as deleted
                        deletedMail.setStatus(QMailMessage::Removed, true);
                        QMailStore::instance()->updateMessage(&deletedMail);
                    }
                } else {
                    // Simply remove this message, if we have it
                    if (EmailFolderList* folder = mailboxList()->owner(deletedMail.id()))
                        folder->removeMail(deletedMail.id());
                }
            }
        }
    }

    Q_UNUSED(mailbox)
}

void EmailClient::setActionVisible(QAction* action, bool visible)
{
    actionVisibility[action] = visible;
}

void EmailClient::composeMessage(QMailMessage::MessageType type, 
                                 const QMailAddressList& to, 
                                 const QString& subject, 
                                 const QString& text, 
                                 const QContentList& attachments, 
                                 QMailMessage::AttachmentsAction action)
{
    if (isHidden() || !isVisible())
        closeAfterWrite = true;

    if (type == QMailMessage::AnyType) {
        // Some attachment types can be sent in an SMS
        bool textOnly(true);
        foreach (const QContent& attachment, attachments) {
            if ((attachment.type() != "text/plain") && 
                (attachment.type() != "text/x-vCalendar") && 
                (attachment.type() != "text/x-vCard")) {
                textOnly = false;
            }
        }

        // Determine what type of addresses we're sending to
        bool emailRecipients(false);
        bool phoneRecipients(false);
        foreach (const QMailAddress& address, to) {
            emailRecipients |= address.isEmailAddress();
            phoneRecipients |= address.isPhoneNumber();
        }

        if (!emailRecipients && textOnly) {
            type = QMailMessage::Sms;
        } else if (!phoneRecipients) {
            type = QMailMessage::Email;
        } else {
            type = QMailMessage::Mms;
        }
    }

    writeMailWidget()->newMail( QMailComposerFactory::defaultKey( type ) );
    if ( mWriteMail->composer().isEmpty() ) { 
        // failed to create new composer, maybe due to no email account 
        // being present. So hide/quit qtmail.
        if (isTransmitting()) // prevents flicker
            QTMailWindow::singleton()->hide();
        else
            QTMailWindow::singleton()->close();
        return;
    }

    mWriteMail->setRecipient( QMailAddress::toStringList(to).join(",") );
    mWriteMail->setSubject( subject );
    mWriteMail->setBody( text, "text/plain; charset=UTF-8" );

    foreach (const QContent& attachment, attachments)
        mWriteMail->attach( attachment, action );

    showComposer();
}

void EmailClient::composeMessage(const QMailMessage& message)
{
    if (isHidden() || !isVisible())
        closeAfterWrite = true;

    modify(message);
}

Folder* EmailClient::currentFolder() const
{
    static SystemFolder search(FolderTypeSearch, MailboxList::LastSearchString);

    if (!mFolderView)
        return 0;

    if (mMessageView && (mMessageView->currentMailbox() == MailboxList::LastSearchString))
        return &search;

    return mFolderView->currentFolder();
}

int EmailClient::folderType(const Folder* folder) const {
    int type = folder->folderType();
    if ((type == FolderTypeSystem) &&
        (static_cast<const SystemFolder*>(folder)->isSearch())) {
        type = FolderTypeSearch;
    }

    return type;
}

EmailFolderList* EmailClient::containingFolder(const QMailId& id)
{
    foreach (const QString& mailbox, mailboxList()->mailboxes()) {
        EmailFolderList* folder(mailboxList()->mailbox(mailbox));
        if (folder->contains(id))
            return folder;
    }

    return 0;
}


/*!
    \service EmailService Email
    \brief Provides the Qtopia Email service.

    The \i Email service enables applications to access features of
    the system's e-mail application.
*/

/*!
    \internal
*/
EmailService::~EmailService()
{
}

/*!
    Direct the \i Email service to interact with the user to compose a new
    e-mail message, and then, if confirmed by the user, send the message.

    This slot corresponds to the QCop service message
    \c{Email::writeMail()}.
*/
void EmailService::writeMail()
{
    qLog(Messaging) << "EmailService::writeMail()";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    parent->writeMailAction(QString(),QString());
}

/*!
    Direct the \i Email service to interact with the user to compose a new
    e-mail message, and then, if confirmed by the user, send the message.
    The message is sent to \a name at \a email.

    This slot corresponds to the QCop service message
    \c{Email::writeMail(QString,QString)}.
*/
void EmailService::writeMail( const QString& name, const QString& email )
{
    qLog(Messaging) << "EmailService::writeMail(" << name << "," << email << ")";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    parent->writeMailAction( name, email );
}

/*!
    \deprecated

    Clients of this service should instead use 
    \c{Messages::composeMessage(QMailMessage::MessageType, QMailAddressList, QString, QString, QContentList, QMailMessage::AttachmentsAction)}.

    Direct the \i Email service to interact with the user to compose a new
    e-mail message, and then, if confirmed by the user, send the message.
    The message is sent to \a name at \a email.  The initial body of
    the message will be based on \a docAttachments and \a fileAttachments.
    The resulting message will contains links to the files passed as
    attachments, unless the attached files are under the path returned by
    \c Qtopia::tempDir(). In this case, the data of the files is copied
    into the resulting message. Linked attachment files must remain accessible
    to qtmail until the message is transmitted.

    This message will choose the best message transport for the message,
    which may be e-mail, SMS, MMS, etc.  This is unlike writeMail(),
    which will always use e-mail.

    This slot corresponds to the QCop service message
    \c{Email::writeMessage(QString,QString,QStringList,QStringList)}.
*/
void EmailService::writeMessage( const QString& name, const QString& email,
                                 const QStringList& docAttachments,
                                 const QStringList& fileAttachments )
{
    qLog(Messaging) << "EmailService::writeMessage(" << name << "," << email << ", ... )";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    parent->writeMessageAction( name, email, docAttachments, fileAttachments );
}

/*!
    Direct the \i Email service to display the user's message boxes.

    This slot corresponds to the QCop service message
    \c{Email::viewMail()}.
*/
void EmailService::viewMail()
{
    qLog(Messaging) << "EmailService::viewMail()";
    parent->initialAction = EmailClient::View;
    parent->delayedInit();
}

/*!
    \deprecated

    Direct the \i Email service to display the message identified by
    \a id.

    This slot corresponds to the QCop service message
    \c{Email::viewMail(QMailId)}.
*/
void EmailService::viewMail( const QMailId& id )
{
    qLog(Messaging) << "EmailService::viewMail(" << id.toULongLong() << ")";
    parent->displayMessage(id);
}

/*!
    Direct the \i Email service to interact with the user to compose a new
    e-mail message for sending the vcard data in \a filename.  The
    \a description argument provides an optional descriptive text message.

    This slot corresponds to the QCop service message
    \c{Email::emailVCard(QString,QString)}.
*/
void EmailService::emailVCard( const QString& filename, const QString& )
{
    qLog(Messaging) << "EmailService::emailVCard(" << filename << ", )";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    parent->writeMessageAction( QString(), QString(), QStringList(),
                                QStringList( filename ),
                                QMailMessage::Email);
}

/*!
    Direct the \i Email service to interact with the user to compose a new
    e-mail message for sending the vcard data in \a request.

    This slot corresponds to a QDS service with a request data type of
    "text/x-vcard" and no response data.

    This slot corresponds to the QCop service message
    \c{Email::emailVCard(QDSActionRequest)}.
*/
void EmailService::emailVCard( const QDSActionRequest& request )
{
    qLog(Messaging) << "EmailService::emailVCard( QDSActionRequest )";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    parent->emailVCard( request );
}

/*!
    \internal
*/
void EmailService::emailVCard( const QString&, const QMailId&, const QString& filename, const QString& description )
{
    qLog(Messaging) << "EmailService::emailVCard( , ," << filename << "," << description << ")";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    // To be removed when the SetValue service is fixed.
    emailVCard( filename, description );
}

/*!
    \deprecated

    Direct the \i Email service to purge all messages which
    are older than the given \a date and exceed the minimal mail \a size.
    This is typically called by the cleanup wizard.

    This slot corresponds to the QCop service message
    \c{Email::cleanupMessages(QDate,int)}.
*/
void EmailService::cleanupMessages( const QDate& date, int size )
{
    qLog(Messaging) << "EmailService::cleanupMessages(" << date << "," << size << ")";
    parent->initialAction = EmailClient::Cleanup;
    parent->delayedInit();
    parent->cleanupMessages( date, size );
}

#ifndef QTOPIA_NO_SMS

/*!
    \service SMSService SMS
    \brief Provides the Qtopia SMS service.

    The \i SMS service enables applications to access features of
    the system's SMS application.
*/

/*!
    \internal
*/
SMSService::~SMSService()
{
}

/*!
    Direct the \i SMS service to interact with the user to compose a new
    SMS message, and then, if confirmed by the user, send the message.

    This slot corresponds to the QCop service message
    \c{SMS::writeSms()}.
*/
void SMSService::writeSms()
{
    qLog(Messaging) << "SMSService::writeSms()";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    parent->writeSmsAction(QString(),QString());
}

/*!
    Direct the \i SMS service to interact with the user to compose a new
    SMS message, and then, if confirmed by the user, send the message.
    The message is sent to \a name at \a number.

    This slot corresponds to the QCop service message
    \c{SMS::writeSms(QString,QString)}.
*/
void SMSService::writeSms( const QString& name, const QString& number )
{
    qLog(Messaging) << "SMSService::writeSms(" << name << "," << number << ")";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    parent->writeSmsAction( name, number );
}

/*!
    Direct the \i SMS service to interact with the user to compose a new
    SMS message, and then, if confirmed by the user, send the message.
    The message is sent to \a name at \a number.  The initial body of
    the message will be read from \a filename.  After the file is
    read, it will be removed.

    This slot corresponds to the QCop service message
    \c{SMS::writeSms(QString,QString,QString)}.
*/
void SMSService::writeSms( const QString& name, const QString& number,
                           const QString& filename )
{
    qLog(Messaging) << "SMSService::writeSms(" << name << "," << number << "," << filename << ")";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    QFile f( filename );
    if (! f.open(QIODevice::ReadOnly) ) {
        qWarning("could not open file: %s", filename.toLatin1().constData() );
    } else {
        QString body = QString::fromLocal8Bit( f.readAll() );
        f.close();
        f.remove();
        parent->writeSmsAction( name, number, body );
    }
}

/*!
    \fn SMSService::newMessages(bool)
    \internal 
*/

/*!
    \deprecated

    Clients of this service should instead use \c{Messages::viewNewMessages()}.

    Show the most recently received SMS message.

    This slot corresponds to the QCop service message
    \c{SMS::viewSms()}.
*/
void SMSService::viewSms()
{
    qLog(Messaging) << "SMSService::viewSms()";

    // Although this requests SMS specifically, we currently have only the 
    // facility to show the newest incoming message, or the combined inbox
    emit newMessages(false);
}

/*!
    \fn SMSService::viewInbox()
    \internal 
*/

/*!
    \deprecated

    Show the list of all received SMS messages.

    This slot corresponds to the QCop service message
    \c{SMS::viewSmsList()}.
*/
void SMSService::viewSmsList()
{
    qLog(Messaging) << "SMSService::viewSmsList";

    // Although this requests SMS specifically, we currently have only the 
    // facility to show the combined inbox
    emit viewInbox();
}

/*!
    Direct the \i SMS service to interact with the user to compose a new
    SMS message for sending the vcard data in \a filename.  The
    \a description argument provides an optional descriptive text message.

    This slot corresponds to the QCop service message
    \c{SMS::smsVCard(QString,QString)}.
*/
void SMSService::smsVCard( const QString& filename, const QString& description)
{
    qLog(Messaging) << "SMSService::smsVCard(" << filename << ", )";
    parent->initialAction = EmailClient::Compose;
    parent->delayedInit();
    QFile f( filename );
    if (! f.open(QIODevice::ReadOnly) ) {
        qWarning("could not open file: %s", filename.toLatin1().constData() );
    } else {
        QString body =  QString::fromLocal8Bit( f.readAll() );
        parent->writeSmsAction( QString(), QString(), body, true);
    }

    Q_UNUSED(description)
}

/*!
    Direct the \i SMS service to interact with the user to compose a new
    SMS message for sending the vcard data in \a request.

    This slot corresponds to a QDS service with a request data type of
    "text/x-vcard" and no response data.

    This slot corresponds to the QCop service message
    \c{SMS::smsVCard(QDSActionRequest)}.

*/
void SMSService::smsVCard( const QDSActionRequest& request )
{
    qLog(Messaging) << "SMSService::smsVCard( QDSActionRequest )";
    parent->initialAction = EmailClient::View;
    parent->delayedInit();
    parent->smsVCard( request );
}

/*!
    \deprecated

    Direct the \i SMS service to interact with the user to compose a new
    SMS message for sending the vcard data in \a filename.  The
    \a description argument provides an optional descriptive text message.
    The \a channel and \a id are not used.

    This slot corresponds to the QCop service message
    \c{SMS::smsVCard(QString,QString,QString,QString)}.
*/
void SMSService::smsVCard( const QString& channel, const QString& id, const QString& filename, const QString& description )
{
    qLog(Messaging) << "SMSService::smsVCard( , ," << filename << "," << description << ")";
    parent->initialAction = EmailClient::View;
    parent->delayedInit();
    // To be removed when the SetValue service is fixed.
    smsVCard( filename, description );

    Q_UNUSED(channel)
    Q_UNUSED(id)
}

/*!
    \fn SMSService::mmsMessage(const QDSActionRequest&)
    \internal 
*/

/*!
    Direct the \i SMS services to handle the MMS push notification \a request.

    This slot corresponds to a QDS service with request data containing
    the serialization of an \c MMSMessage, and no response data.

    This slot corresponds to the QCop service message
    \c{SMS::pushMmsMessage(QDSActionRequest)}.
*/
void SMSService::pushMmsMessage( const QDSActionRequest& request )
{
    qLog(Messaging) << "SMSService::pushMmsMessage( QDSActionRequest )";

#ifndef QTOPIA_NO_MMS
    emit mmsMessage(request);
#endif
}

/*!
    Direct the \i SMS service to process the flash SMS message
    within \a request.

    This slot corresponds to a QDS service with request data containing
    the serialization of a QSMSMessage, and no response data.

    This slot corresponds to the QCop service message
    \c{SMS::flashSms(QDSActionRequest)}.
*/
void SMSService::flashSms( const QDSActionRequest& request )
{
    qLog(Messaging) << "SMSService::flashSms( QDSActionRequest )";
    parent->initialAction = EmailClient::View;
    parent->delayedInit();
    parent->flashSms( request );
}

#endif // QTOPIA_NO_SMS

/*!
    \service MessagesService Messages
    \brief Provides the Qtopia Messages viewing service.

    The \i Messages service enables applications to request the display of messages of various types.
*/

/*! \internal */
MessagesService::MessagesService(EmailClient* parent)
    : QtopiaAbstractService( "Messages", parent )
{
    publishAll();
}

/*! \internal */
MessagesService::~MessagesService()
{
}

/*!
    \fn MessagesService::newMessages(bool)
    \internal 
*/

/*!
    Show the newly arrived messages.  If \a userRequest is true, the request will be treated
    as if arising from a direct user action; otherwise, the user will be requested to confirm 
    the action before proceeding.

    This slot corresponds to the QCop service message
    \c{Messages::viewNewMessages()}.
*/
void MessagesService::viewNewMessages(bool userRequest)
{
    qLog(Messaging) << "MessagesService::viewNewMessages(" << userRequest << ")";

    emit newMessages(userRequest);
}

/*!
    \fn MessagesService::message(QMailId)
    \internal 
*/

/*!
    Show the message with the supplied \a id.

    This slot corresponds to the QCop service message
    \c{Messages::viewMessage(QMailId)}.
*/
void MessagesService::viewMessage(QMailId id)
{
    qLog(Messaging) << "MessagesService::viewMessage(" << id << ")";
    if (!id.isValid()) {
        qWarning() << "viewMessage supplied invalid id:" << id.toULongLong();
        return;
    }

    emit message(id);
}

/*!
    \fn MessagesService::compose(QMailMessage::MessageType, const QMailAddressList&, const QString&, const QString&, const QContentList&, QMailMessage::AttachmentsAction)
    \internal 
*/

/*!
    Compose a message of type \a type, with the supplied properties. If \a type is
    \c QMailMessage::AnyType, the type will be determined by inspecting the types of 
    the addresses in \a to. The message will be addressed to each of the recipients 
    in \a to, the subject will be preset to \a subject, and the message text will be 
    preset to \a text.

    This slot corresponds to the QCop service message
    \c{Messages::composeMessage(QMailMessage::MessageType, QMailAddressList, QString, QString)}.
*/
void MessagesService::composeMessage(QMailMessage::MessageType type, QMailAddressList to, QString subject, QString text)
{
    qLog(Messaging) << "MessagesService::composeMessage(" << type << ',' << QMailAddress::toStringList(to).join(",") << ", <text> )";

    emit compose(type, to, subject, text, QContentList(), QMailMessage::LinkToAttachments);
}

/*!
    Compose a message of type \a type, with the supplied properties. If \a type is
    \c QMailMessage::AnyType, the type will be determined by inspecting the types of 
    the addresses in \a to, and by the existence of attachments. The message will be 
    addressed to each of the recipients in \a to, the message subject will be set to
    \a subject, and the message text will be preset to \a text.  All the documents 
    listed in \a attachments will be added to the message as attachments. If \a action 
    is \c MessagesService::LinkToAttachments, the attachments will be created as links 
    to the source document; otherwise, the data of the documents will be stored directly 
    in the message parts. If \a action is \c MessagesService::CopyAndDeleteAttachments, 
    the source document will be deleted after the data is copied.

    This slot corresponds to the QCop service message
    \c{Messages::composeMessage(QMailMessage::MessageType, QMailAddressList, QString, QString, QContentList, QMailMessage::AttachmentsAction)}.
*/
void MessagesService::composeMessage(QMailMessage::MessageType type, 
                                     QMailAddressList to, 
                                     QString subject, 
                                     QString text, 
                                     QContentList attachments, 
                                     QMailMessage::AttachmentsAction action)
{
    qLog(Messaging) << "MessagesService::composeMessage(" << type << ", ...)";

    emit compose(type, to, subject, text, attachments, action);
}

/*!
    \fn MessagesService::compose(const QMailMessage&)
    \internal 
*/

/*!
    Compose a message in the appropriate composer, where all composer fields are preset 
    with the data from the matching field of \a message.

    This slot corresponds to the QCop service message
    \c{Messages::composeMessage(QMailMessage)}.
*/
void MessagesService::composeMessage(QMailMessage message)
{
    qLog(Messaging) << "MessagesService::composeMessage(QMailMessage)";

    emit compose(message);
}

#include "emailclient.moc"

