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



#include "readmail.h"
#include "accountlist.h"
#include "maillistview.h"
#include "emailfolderlist.h"
#include "smsclient.h"

#include <qtopiaapplication.h>
#include <qtopiaipcenvelope.h>
#include <qtopianamespace.h>
#include <qtopiaservices.h>
#include <qsoftmenubar.h>
#include <qthumbnail.h>

#include <qtopia/pim/qcontact.h>
#include <qtopia/pim/qcontactmodel.h>

#include <qtopia/mail/qmailviewer.h>

#include <qlabel.h>
#include <qimage.h>
#include <qaction.h>
#include <qfile.h>
#include <qtextbrowser.h>
#include <qtextstream.h>

#include <qcursor.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qregexp.h>
#include <qstackedwidget.h>
#include <qmessagebox.h>
#include <qboxlayout.h>
#include <qevent.h>
#include <qimagereader.h>
#include <qalgorithms.h>
#include <qtmailwindow.h>
#include <QContactSelector>
#include <QMailStore>
#include <QMailComposerFactory>
#include <QDrmContentPlugin>


ReadMail::ReadMail( QWidget* parent,  const QString name, Qt::WFlags fl )
    : QMainWindow(parent, fl)
{
    setObjectName( name );
    sending = false;
    receiving = false;
    initialized = false;
    firstRead = false;
    mailView = 0;
    accountList = 0;
    contactModel = 0;
    modelUpdatePending = false;

    init();
}

ReadMail::~ReadMail()
{
    delete contactModel;
}

void ReadMail::init()
{
#ifndef QTOPIA_NO_MMS
    smilView = 0;
#endif

    QDrmContentPlugin::initialize();

    getThisMailButton = new QAction( QIcon(":icon/getmail"), tr("Get message"), this );
    connect(getThisMailButton, SIGNAL(triggered()), this, SLOT(getThisMail()) );
    getThisMailButton->setWhatsThis( tr("Retrieve this message from the server.  You can use this option to retrieve individual messages that would normally not be automatically downloaded.") );

    sendThisMailButton = new QAction( QIcon(":icon/sendmail"), tr("Send message"), this );
    connect(sendThisMailButton, SIGNAL(triggered()), this, SLOT(sendThisMail()));
    sendThisMailButton->setWhatsThis(  tr("Send this message.  This option will not send any other messages in your outbox.") );

    replyButton = new QAction( QIcon(":icon/reply"), tr("Reply"), this );
    connect(replyButton, SIGNAL(triggered()), this, SLOT(reply()));
    replyButton->setWhatsThis( tr("Reply to sender only.  Select Reply all from the menu if you want to reply to all recipients.") );

    replyAllAction = new QAction( QIcon(":icon/replytoall"), tr("Reply all"), this );
    connect(replyAllAction, SIGNAL(triggered()), this, SLOT(replyAll()));

    forwardAction = new QAction(tr("Forward"), this );
    connect(forwardAction, SIGNAL(triggered()), this, SLOT(forward()));

    modifyButton = new QAction( QIcon(":icon/edit"), tr("Modify"), this );
    connect(modifyButton, SIGNAL(triggered()), this, SLOT(modify()));
    modifyButton->setWhatsThis( tr("Opens this message in the composer so that you can make modifications to it.") );

    previousButton = new QAction( QIcon( ":icon/up" ), tr( "Previous" ), this );
    connect( previousButton, SIGNAL(triggered()), this, SLOT( previous() ) );
    previousButton->setWhatsThis( tr("Read the previous message in the folder.") );

    nextButton = new QAction( QIcon( ":icon/down" ), tr( "Next" ), this );
    connect( nextButton, SIGNAL(triggered()), this, SLOT( next() ) );
    nextButton->setWhatsThis( tr("Read the next message in the folder.") );

    attachmentsButton = new QAction( QIcon( ":icon/attach" ), tr( "Attachments" ), this );
    connect( attachmentsButton, SIGNAL(triggered()), this,
            SLOT( viewAttachments() ) );
    attachmentsButton->setWhatsThis( tr("View the attachments in the message.") );

    deleteButton = new QAction( QIcon( ":icon/trash" ), tr( "Delete" ), this );
    connect( deleteButton, SIGNAL(triggered()), this, SLOT( deleteItem() ) );
    deleteButton->setWhatsThis( tr("Move this message to the trash folder.  If the message is already in the trash folder it will be deleted. ") );

    storeButton = new QAction( QIcon( ":icon/save" ), tr( "Save to Contacts" ), this );
    connect( storeButton, SIGNAL(triggered()), this, SLOT( storeContact() ) );
    
    views = new QStackedWidget(this);

    // Create a viewer for static content
    QString key = QMailViewerFactory::defaultKey( QMailViewerFactory::StaticContent );
    emailView = QMailViewerFactory::create( key, views );
    emailView->setObjectName( "read-message" );

    connect(emailView, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkClicked(QUrl)) );
    connect(emailView, SIGNAL(finished()), this, SLOT(closeView()) );

    QWidget* viewer = emailView->widget();
    viewer->setWhatsThis( tr("This view displays the contents of the message.") );

    views->addWidget(viewer);
    views->setCurrentWidget(viewer);

    setCentralWidget(views);

    context = QSoftMenuBar::menuFor( this );
}

void ReadMail::setAccountList(AccountList* list)
{
    accountList = list;
}

/*  We need to be careful here. Don't allow clicking on any links
    to automatically install anything.  If we want that, we need to
    make sure that the mail doesn't contain mailicious link encoding
*/
void ReadMail::linkClicked(const QUrl &lnk)
{
#ifdef QTOPIA4_TODO
    // Why does lnk have "&amp;" etc. in it?
    QString str = Qtopia::plainString(lnk.toString());
#else
    QString str = lnk.toString();
#endif

    int pos = str.indexOf(";");
    if ( pos != -1 ) {
        QString command = str.left(pos);
        QString param = str.mid(pos + 1);

        if ( command == "attachment" ) { // No tr
            if (param == "view") { // No tr
                viewAttachments();
            } else if ( param.startsWith("scrollto;") ) {
                emailView->scrollToAnchor( param.mid(9) );
            } else if (param == "play") {
                if (isMms)
                    viewMms();
            }
        } else if ( command == "dial" ) {
            dialNumber( param );
        }
    } else {
        if ( str.startsWith("mailto:") )  {
            emit mailto( str );
        } else if ( str.startsWith("http://") ) {
            QtopiaServiceRequest e( "WebAccess", "openURL(QString)" );
            e << str;
            e.send();
        } else if ( str == "download") {
            getThisMail();
        } else if( mail.messageType() == QMailMessage::System && str.startsWith( QLatin1String( "qtopiaservice:" ) ) ) {
            int commandPos  = str.indexOf( QLatin1String( "::" ) ) + 2;
            int argPos      = str.indexOf( '?' ) + 1;
            QString service = str.mid( 14, commandPos - 16 );
            QString command;
            QStringList args;

            if( argPos > 0 )
            {
                command = str.mid( commandPos, argPos - commandPos - 1 );
                args    = str.mid( argPos ).split( ',' );
            }
            else
                command = str.mid( commandPos );

            QtopiaServiceRequest e( service, command );

            foreach( QString arg, args )
                e << arg;

            e.send();
        }
    }
}

static QString displayName(const QMailMessage& mail)
{
    QString name(mail.from().displayName());
    if (name.isEmpty()) {
        // Use the type of this message as the title
        QString key(QMailComposerFactory::defaultKey(mail.messageType()));
        if (!key.isEmpty())
            name = QMailComposerFactory::displayName(key);
        else 
            name = qApp->translate("ReadMail", "Message");

        if (!name.isEmpty())
            name[0] = name[0].toUpper();
    }

    return name;
}

void ReadMail::updateView()
{
    if ( !lastMailId.isValid() )
        return;

    switchView(emailView->widget(), displayName(mail));
    if ( !(mail.status() & QMailMessage::Read) ) {
        mail.setStatus( QMailMessage::Read, true );
        QMailStore::instance()->updateMessage(&mail);
        firstRead = true;
    }
    else
        firstRead = false;

    //report currently viewed mail so that it will be
    //placed first in the queue of new mails to download.
    emit viewingMail(mail);

    emailView->clear();

    if (!isSmil && (mail.messageType() != QMailMessage::System)) {
        initImages(emailView);
    }

    emailView->setMessage(mail);
}

void ReadMail::keyPressEvent(QKeyEvent *e)
{
    switch( e->key() ) {
        case Qt::Key_A:
            if ( attachmentsButton->isEnabled() )
                viewAttachments();
            break;
        case Qt::Key_P:
            if ( previousButton->isEnabled() )
                previous();
            break;
        case Qt::Key_N:
            if ( nextButton->isEnabled() )
                next();
            break;
        case Qt::Key_Delete:
            deleteItem();
            break;
        case Qt::Key_R:
            reply();
            break;
        case Qt::Key_F:
            forward();
            break;
        case Qt::Key_E:
            if ( modifyButton->isEnabled() )
                modify();
        default:
            QMainWindow::keyPressEvent( e );
    }
}

//update view with current EmailListItem (item)
void ReadMail::viewSelectedMail(MailListView *view)
{
    mailView = view;
    EmailListItem *current = static_cast<EmailListItem *>( view->currentItem() );

    if ( !current || !view->isItemSelected( current ) ) {
        close();
        return;
    }

    showMail(current->id());

    QString mailbox = mailView->currentMailbox();

    context->clear();

    if ( hasGet(mailbox) )
        context->addAction( getThisMailButton );
    else if ( hasSend(mailbox) )
        context->addAction( sendThisMailButton );

    if ( hasReply(mailbox) ) {
        // You can't reply to a system message
        if (mail.messageType() != QMailMessage::System) {
            context->addAction( replyButton );
            context->addAction( replyAllAction );
        }

        context->addAction( forwardAction );
    }

    if ( hasEdit(mailbox) )
        context->addAction( modifyButton );

    deleteButton->setText( hasDelete(mailbox) ? tr("Delete") : tr("Move to Trash") );
    context->addAction( deleteButton );
    context->addAction( storeButton );

    context->addSeparator();
    emailView->addActions( context );

    updateButtons();
}

void ReadMail::buildMenu(const QString &mailbox)
{
    Q_UNUSED(mailbox);
}

void ReadMail::mailUpdated(const QMailId& id)
{
    if ( lastMailId == id ) {
        //reload the mail
        mail = QMailMessage(id,QMailMessage::HeaderAndBody);
        updateView();
    }
    
    updateButtons();
}

void ReadMail::showMail(const QMailId& id)
{
    mail = QMailMessage(id,QMailMessage::HeaderAndBody);
    lastMailId = id;

    forwardAction->setVisible(mail.messageType() == QMailMessage::Email);
    replyAllAction->setVisible(mail.messageType() == QMailMessage::Email);
    
    isMms = false;
    isSmil = false;

#ifndef QTOPIA_NO_MMS
    if (mail.messageType() == QMailMessage::Mms) {
        QString mmsType = mail.headerFieldText("X-Mms-Message-Type");
        if (mmsType.contains("m-retrieve-conf") || mmsType.contains("m-send-req")) {
            isMms = true;
            if (mail.contentType().content().toLower() == "multipart/related")
                isSmil = true;
        }
    }
#endif

    updateView();
}

void ReadMail::closeView()
{
    //check for read reply flag
#ifndef QTOPIA_NO_MMS
    QString mmsType = mail.headerFieldText("X-Mms-Message-Type");
    QString msgClass = mail.headerFieldText("X-Mms-Message-Class");
    QString readReply = mail.headerFieldText("X-Mms-Read-Reply");

    if (mmsType.contains("m-retrieve-conf") && !msgClass.contains("Auto")
        && readReply.contains("Yes") && firstRead) {
        emit readReplyRequested(mail);
        }

    if (smilView && views->currentWidget() == smilView->widget()) {
        switchView(emailView->widget(), displayName(mail));
        smilView->clear();
        return;
    }
#endif
    cleanup();
    emit cancelView();
}

void ReadMail::cleanup()
{
    emailView->clear();
}

//gets next item in listview, exits if there is no next
void ReadMail::next()
{
    EmailListItem *item = static_cast<EmailListItem *>( mailView->currentItem() );
    if (item && mailView->isItemSelected(item) &&
        mailView->row( item ) + 1 < mailView->rowCount() )
        item = static_cast<EmailListItem*>(mailView->item( mailView->row( item ) + 1, 0 ));
    else
       item = 0;

    if (item != NULL) {
        mailView->setSelectedItem(item);
        viewSelectedMail(mailView);
    }
}

//gets previous item in listview, exits if there is no previous
void ReadMail::previous()
{
    EmailListItem *item = static_cast<EmailListItem *>( mailView->currentItem() );
    if (item && mailView->isItemSelected(item) &&
        mailView->row( item ) > 0 )
        item = static_cast<EmailListItem*>(mailView->item( mailView->row( item ) - 1, 0 ));
    else
        item = 0;

    if (item != NULL) {
        mailView->setSelectedItem(item);
        viewSelectedMail(mailView);
    }
}

//deletes item, tries bringing up next or previous, exits if unsucessful
void ReadMail::deleteItem()
{
    EmailListItem *item = static_cast<EmailListItem *>( mailView->currentItem() );
    emit removeItem(item);
}

void ReadMail::updateButtons()
{
    EmailListItem *current = static_cast<EmailListItem *>( mailView->currentItem() );

    if ( !current || !mailView->isItemSelected( current ) ) {
        close();
        return;
    }

    /*  Safety precaution.  The mail might have been moved internally/externally
        away from the mailbox.  Verify that we actually still have access to the
        same mail   */
    if ( current->id() != lastMailId ) {
        viewSelectedMail( mailView );
        mail = QMailMessage(current->id(),QMailMessage::HeaderAndBody);
        return;
    }

    if ( (mail.status() & QMailMessage::Sent) || sending ) {
        sendThisMailButton->setVisible(false);
    } else {
        sendThisMailButton->setVisible(mail.hasRecipients());
    }

    modifyButton->setVisible( !((mail.status() & QMailMessage::Sent) || sending ) );

    bool needsDownload = (mail.status() & QMailMessage::Downloaded) || receiving;
    bool removed = (mail.status() & QMailMessage::Removed);
    getThisMailButton->setVisible( !needsDownload && !removed );

    if (!needsDownload) {
        // We can't really forward/reply/reply-to-all without the message content
        replyButton->setVisible(false);
        replyAllAction->setVisible(false);
        forwardAction->setVisible(false);
    } else {
        bool replyable(true);
        bool otherReplyTarget(!mail.cc().isEmpty() || mail.to().count() > 1);

        if (accountList) {
            if (QMailAccount* account = accountList->getAccountById(mail.fromAccount())) {
                QString accountAddress(account->emailAddress());
                // TODO: if we have no email address, we should try to get our own phone number...

                // We can't reply to messages from ourself
                if (mail.from().address() == accountAddress) {
                    replyable = false;
                } else {
                    // We can reply to all, if there are CC addresses or if the mail was sent
                    // an address other than ours (a list, probably)
                    const QList<QMailAddress>& toList(mail.to());
                    if (!toList.isEmpty() && (toList.first().address() != accountAddress))
                        otherReplyTarget = true;
                }
            }
        }

        replyButton->setVisible(replyable);
        replyAllAction->setVisible(replyable && otherReplyTarget);
        forwardAction->setVisible(true);
    }

    attachmentsButton->setVisible( mail.partCount() );

    nextButton->setVisible(mailView->row(current) + 1 < mailView->rowCount());
    previousButton->setVisible(mailView->row(current) > 0);

    // Show the 'Store to Contacts' action if we don't have a matching contact
    QMailAddress fromAddress(mail.from());
    bool unknownContact = !fromAddress.matchesExistingContact();
    storeButton->setVisible(unknownContact);

    if ( current )
        current->updateState();
}

void ReadMail::viewAttachments()
{
    ViewAtt dlg(&mail, (mail.status() & QMailMessage::Incoming));
    QtopiaApplication::execDialog(&dlg);
    QMailStore::instance()->updateMessage(&mail);
}

void ReadMail::viewMms()
{
#ifndef QTOPIA_NO_MMS
    if (!smilView) {
        // Create a viewer for SMIL content
        QString key = QMailViewerFactory::defaultKey( QMailViewerFactory::SmilContent );
        smilView = QMailViewerFactory::create( key, views );
        smilView->setObjectName( "smilView" );

        connect(smilView, SIGNAL(finished()), this, SLOT(mmsFinished()));

        QWidget* viewer = smilView->widget();
        views->addWidget(viewer);

        QSoftMenuBar::setLabel(viewer, QSoftMenuBar::menuKey(), QSoftMenuBar::NoLabel);
        QSoftMenuBar::setLabel(viewer, Qt::Key_Select, QSoftMenuBar::Next);
    }

    if (smilView->setMessage(mail)) {
        switchView(smilView->widget(), tr("MMS"));
    } else {
        QMessageBox::warning(this, tr("Cannot view MMS"),
            tr("<qt>Cannot play improperly formatted MMS</qt>"), QMessageBox::Ok, QMessageBox::NoButton);
    }
#endif
}

void ReadMail::mmsFinished()
{
    switchView(emailView->widget(), displayName(mail));
}

void ReadMail::reply()
{
    emit resendRequested(mail, Reply);
}

void ReadMail::replyAll()
{
    emit resendRequested(mail, ReplyToAll);
}

void ReadMail::forward()
{
    emit resendRequested(mail, Forward);
}

void ReadMail::setStatus(int id)
{
    QMailMessage::Status prevStatus(mail.status());
    QMailMessage::Status newStatus(prevStatus);

    switch( id ) {
        case 1:
            newStatus &= ~QMailMessage::Status( QMailMessage::Replied | 
                                                QMailMessage::RepliedAll | 
                                                QMailMessage::Forwarded |
                                                QMailMessage::Read );
            break;

        case 2:
            newStatus &= ~QMailMessage::Status( QMailMessage::RepliedAll | 
                                                QMailMessage::Forwarded );
            newStatus |= QMailMessage::Replied;
            break;

        case 3:
            newStatus &= ~QMailMessage::Status( QMailMessage::Replied | 
                                                QMailMessage::RepliedAll );
            newStatus |= QMailMessage::Forwarded;
            break;

        case 4: 
            newStatus |= QMailMessage::Sent;
            break;

        case 5: 
            newStatus &= ~QMailMessage::Sent;
            break;
    }

    if ( newStatus != prevStatus) {
        mail.setStatus( newStatus );
        QMailStore::instance()->updateMessage(&mail);
    }

    updateButtons();
}

void ReadMail::modify()
{
    emit modifyRequested(mail);
}

void ReadMail::getThisMail()
{
    emit getMailRequested(mail);
}

void ReadMail::sendThisMail()
{
    emit sendMailRequested(mail);
}

void ReadMail::isSending(bool on)
{
    sending = on;
    if ( isVisible() )
        updateButtons();
}

void ReadMail::isReceiving(bool on)
{
    receiving = on;
    if ( isVisible() )
        updateButtons();
}

void ReadMail::initImages(QMailViewerInterface* view)
{
    static QMap<QUrl, QVariant> resourceMap;

    if (!initialized) {
    // Add the predefined smiley images for EMS messages.
        resourceMap.insert( QUrl( "x-sms-predefined:ironic" ),
                            QVariant( QImage( ":image/smiley/ironic" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:glad" ),
                            QVariant( QImage( ":image/smiley/glad" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:skeptical" ),
                            QVariant( QImage( ":image/smiley/skeptical" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:sad" ),
                            QVariant( QImage( ":image/smiley/sad" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:wow" ),
                            QVariant( QImage( ":image/smiley/wow" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:crying" ),
                            QVariant( QImage( ":image/smiley/crying" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:winking" ),
                            QVariant( QImage( ":image/smiley/winking" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:laughing" ),
                            QVariant( QImage( ":image/smiley/laughing" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:indifferent" ),
                            QVariant( QImage( ":image/smiley/indifferent" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:kissing" ),
                            QVariant( QImage( ":image/smiley/kissing" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:confused" ),
                            QVariant( QImage( ":image/smiley/confused" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:tongue" ),
                            QVariant( QImage( ":image/smiley/tongue" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:angry" ),
                            QVariant( QImage( ":image/smiley/angry" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:glasses" ),
                            QVariant( QImage( ":image/smiley/glasses" ) ) );
        resourceMap.insert( QUrl( "x-sms-predefined:devil" ),
                            QVariant( QImage( ":image/smiley/devil" ) ) );
        initialized = true;
    }

    QMap<QUrl, QVariant>::iterator it = resourceMap.begin(), end = resourceMap.end();
    for ( ; it != end; ++it)
        view->setResource( it.key(), it.value() );
}

bool ReadMail::hasGet(const QString &mailbox)
{
    return (mailbox == MailboxList::InboxString); 
}

bool ReadMail::hasSend(const QString &mailbox)
{
    return (mailbox == MailboxList::OutboxString); 
}

bool ReadMail::hasEdit(const QString &mailbox)
{
    return (mailbox == MailboxList::DraftsString);
}

bool ReadMail::hasReply(const QString &mailbox)
{
    return (mailbox != MailboxList::OutboxString); 
}

bool ReadMail::hasDelete(const QString &mailbox)
{
    return (mailbox == MailboxList::TrashString); 
}

void ReadMail::dialNumber(const QString& number)
{
    if ( !number.isEmpty() ) {
        QtopiaServiceRequest req( "Dialer", "showDialer(QString)" );
        req << number;
        req.send();
    }
}

void ReadMail::switchView(QWidget* widget, const QString& title)
{
    QTMailWindow::singleton()->setWindowTitle(title);
    views->setCurrentWidget(widget);
}

void ReadMail::storeContact()
{
    QMailAddress fromAddress = mail.from();
    if (!fromAddress.isPhoneNumber() && !fromAddress.isEmailAddress()) {
        qWarning() << "Unable to store unknown address type:" << fromAddress.toString();
    } else {
        QString text = "<qt>" + tr("Create a new contact?") + "</qt>";
        bool newContact = (QMessageBox::warning(0,
                                                tr("Save to Contacts"), 
                                                text, 
                                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);

        if (!contactModel) {
            // Once we have registered the new contact, we need to update our message display
            contactModel = new QContactModel();
            connect(contactModel, SIGNAL(modelReset()), this, SLOT(contactModelReset()) );
        }

        modelUpdatePending = true;

        if (fromAddress.isPhoneNumber()) {
            QtopiaServiceRequest req( "Contacts", (newContact ? "createNewContact(QString)"
                                                              : "addPhoneNumberToContact(QString)") );
            req << fromAddress.toString();
            req.send();
        } else {
            // The Contacts app doesn't provide email address services at this time
            if (newContact) {
                QContact contact;
                contact.insertEmail(fromAddress.address());

                QtopiaServiceRequest req( "Contacts", "addAndEditContact(QContact)" );
                req << contact;
                req.send();
            } else {
                // For now, we need to do this ourselves
                QContactSelector selector;
                selector.setObjectName("select-contact");

                QContactModel model(&selector);

                QSettings config( "Trolltech", "Contacts" );
                config.beginGroup( "default" );
                if (config.contains("SelectedSources/size")) {
                    int count = config.beginReadArray("SelectedSources");
                    QSet<QPimSource> set;
                    for(int i = 0; i < count; ++i) {
                        config.setArrayIndex(i);
                        QPimSource s;
                        s.context = QUuid(config.value("context").toString());
                        s.identity = config.value("identity").toString();
                        set.insert(s);
                    }
                    config.endArray();
                    model.setVisibleSources(set);
                }

                selector.setModel(&model);
                selector.setAcceptTextEnabled(false);

                selector.showMaximized();
                if (QtopiaApplication::execDialog(&selector) == QDialog::Accepted) {
                    QContact contact(selector.selectedContact());
                    contact.insertEmail(fromAddress.address());

                    if (!model.updateContact(contact)) {
                        qWarning() << "Unable to update contact:" << contact.label();
                        return;
                    }
                } else {
                    return;
                }
            }
        }

        if (mailView) {
            // Force update of from address in GUI
            mailView->resetNameCaches();
        }
    }
}

void ReadMail::contactModelReset()
{
    if (modelUpdatePending) {
        // TODO: In fact, we can't ignore unrequested reset events, because the first reset after 
        // our update may not include the change we requested...
        //modelUpdatePending = false;

        updateView();
        updateButtons();
    }
}

