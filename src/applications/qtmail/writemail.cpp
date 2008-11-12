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

#include "writemail.h"
#include "accountlist.h"
#include "account.h"
#include "qtmailwindow.h"

#include <qtopia/mail/qmailaddress.h>
#include <qtopia/mail/qmailcomposer.h>
#include <qtopia/mail/qmailtimestamp.h>

#include <qtopiaapplication.h>
#include <QtopiaItemDelegate>
#include <QSettings>
#include <QMenu>
#include <qsoftmenubar.h>
#include <qdocumentselector.h>
#include <qtopiaglobal.h>
#include <QTabWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QStackedWidget>
#include <QMessageBox>
#include <QDateTime>
#include <QBoxLayout>
#include <QTextEdit>
#include <QDir>
#include <QFileInfo>
#include <QToolTip>
#include <QWhatsThis>
#include <QStackedWidget>
#include <QDrmContentPlugin>
#include <sys/vfs.h>  // sharp 1862

#include "detailspage.h"


class SelectListWidgetItem : public QListWidgetItem
{
public:
    SelectListWidgetItem(const QString& id, QListWidget* listWidget)
        : QListWidgetItem(QMailComposerFactory::name(id), listWidget),
          _key(id)
    {
        setIcon(QMailComposerFactory::displayIcon(_key));
    }

    const QString& key() const { return _key; }
    QMailMessage::MessageType type() { return type(_key); }

    static QMailMessage::MessageType type(const QString& key) { return QMailComposerFactory::messageType(key); }

private:
    QString _key;
};

class SelectListWidget : public QListWidget
{
    Q_OBJECT

public:
    SelectListWidget( QWidget* parent );

    void setKeys(const QStringList& keys);
    void setTypes(const QList<QMailMessage::MessageType>& types);
    void setSelected(const QString& selected);

    QString singularKey() const;

signals:
    void selected(const QString& key);
    void cancel();

protected slots:
    void accept(QListWidgetItem* item);

protected:
    void keyPressEvent(QKeyEvent *e);

    QStringList _keys;
};

SelectListWidget::SelectListWidget( QWidget *parent )
    : QListWidget( parent )
{
    setFrameStyle( QFrame::NoFrame );
    setContentsMargins(0, 0, 0, 0);
    setItemDelegate(new QtopiaItemDelegate);

    connect(this, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(accept(QListWidgetItem*)));
}

void SelectListWidget::setKeys(const QStringList& keys)
{
    _keys = keys;
}

void SelectListWidget::setTypes(const QList<QMailMessage::MessageType>& types)
{
    clear();

    foreach (const QString& key, _keys)
        if (types.contains(SelectListWidgetItem::type(key)))
            (void)new SelectListWidgetItem(key, this);
}

void SelectListWidget::setSelected(const QString& selected)
{
    int selectIndex = 0;

    if (!selected.isEmpty()) {
        for (int i = 1; i < count() && selectIndex == 0; ++i) {
            if (static_cast<SelectListWidgetItem*>(item(i))->key() == selected)
                selectIndex = i;
        }
    }

    setCurrentRow(selectIndex);
}

QString SelectListWidget::singularKey() const
{
    if (count() == 1)
        return static_cast<SelectListWidgetItem*>(item(0))->key();

    return QString();
}

void SelectListWidget::accept(QListWidgetItem* item)
{
    if (item) {
        // Allow this event to be acted upon before we return control to our parent
        emit selected(static_cast<SelectListWidgetItem*>(item)->key());
    }
}

void SelectListWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Back) {
        e->accept();

        // Allow this event to be acted upon before we return control to our parent
        emit cancel();
        return;
    }

    QListWidget::keyPressEvent( e );
}

//===========================================================================

WriteMail::WriteMail(QWidget* parent,  const char* name, Qt::WFlags fl )
    : QMainWindow( parent, fl ), m_composerInterface(0), m_composerWidget(0), 
      m_detailsPage(0), m_previousAction(0), m_cancelAction(0), m_draftAction(0), 
      widgetStack(0), _detailsOnly(false), _action(Create), _selectComposer(0),
      _composerList(0)
{
    setObjectName(name);

    /*
      because of the way QtMail has been structured, even though
      WriteMail is a main window, setting its caption has no effect
      because its parent is also a main window. have to set it through
      the real main window.
    */
    m_mainWindow = QTMailWindow::singleton();

    init();
}

WriteMail::~WriteMail()
{
    delete m_composerInterface;
    m_composerInterface = 0;
}

void WriteMail::setAccountList(AccountList *list)
{
    accountList = list;
    if (m_detailsPage )
        m_detailsPage->setAccountList( accountList );

    // Find what types of outgoing messages our accounts support
    _sendTypes.clear();
    QListIterator<QMailAccount*> itAccount = accountList->accountIterator();
    while (itAccount.hasNext()) {
        const QMailAccount* account = itAccount.next();

        if ((account->accountType() == QMailAccount::SMS) && 
            (!_sendTypes.contains(QMailMessage::Sms)))
            _sendTypes.append(QMailMessage::Sms);

        // We can only send an MMS if a WAP interface is configured
        if ((account->accountType() == QMailAccount::MMS) && 
#ifndef ENABLE_UNCONDITIONAL_MMS_SEND
            (!account->networkConfig().isEmpty()) &&
#endif
            (!_sendTypes.contains(QMailMessage::Mms)))
            _sendTypes.append(QMailMessage::Mms);

        // We can only send email if an account has SMTP details configured
        if (((account->accountType() == QMailAccount::POP) || (account->accountType() == QMailAccount::IMAP)) && 
            (!account->emailAddress().isEmpty()) && 
            (!_sendTypes.contains(QMailMessage::Email)))
            _sendTypes.append(QMailMessage::Email);
    }

    _composerList->setTypes(_sendTypes);
}

static int messageTypeValue(QMailMessage::MessageType type)
{
    if (type == QMailMessage::Sms) return 0;
    if (type == QMailMessage::Mms) return 1;
    if (type == QMailMessage::Email) return 2;
    if (type == QMailMessage::System) return 3;
    return 4;
}

static bool compareComposerByType(const QString& lhs, const QString& rhs)
{
    QMailMessage::MessageType lhsType = QMailComposerFactory::messageType(lhs);
    QMailMessage::MessageType rhsType = QMailComposerFactory::messageType(rhs);

    if (lhsType != rhsType)
        return (messageTypeValue(lhsType) < messageTypeValue(rhsType));

    return false;
}

void WriteMail::init()
{
    widgetStack = new QStackedWidget(this);
    setCentralWidget(widgetStack);

    QDrmContentPlugin::initialize();

    /*
    TODO  : composers should handle this
    wrapLines = new QAction( tr("Wrap lines"), QString::null, 0, this, 0);
    wrapLines->setToggleAction( true );
    connect(wrapLines, SIGNAL(activated()), this, SLOT(wrapToggled()) );
    */

    m_cancelAction = new QAction(QIcon(":icon/cancel"),tr("Cancel"),this);
    connect( m_cancelAction, SIGNAL(triggered()), this, SLOT(discard()) );

    m_draftAction = new QAction(QIcon(":icon/draft"),tr("Save in drafts"),this);
    connect( m_draftAction, SIGNAL(triggered()), this, SLOT(draft()) );
    m_draftAction->setWhatsThis( tr("Save this message as a draft.") );

    m_previousAction = new QAction( QIcon(":icon/i18n/previous"),tr("Previous"),this);
    connect( m_previousAction, SIGNAL(triggered()), this, SLOT(previousStage()) );

    _selectComposer = new QWidget(this);
    _selectComposer->setObjectName("selectComposer");
    widgetStack->addWidget(_selectComposer);
    QSoftMenuBar::setLabel(_selectComposer, Qt::Key_Back, QSoftMenuBar::Cancel);

    QList<QString> keys = QMailComposerFactory::keys();
    qSort(keys.begin(), keys.end(), compareComposerByType);

    _composerList = new SelectListWidget( _selectComposer );
    _composerList->setKeys(keys);
    _composerList->setFrameStyle(QFrame::NoFrame);
    connect(_composerList, SIGNAL(selected(QString)), this, SLOT(composerSelected(QString)));
    connect(_composerList, SIGNAL(cancel()), this, SLOT(selectionCanceled()));
    
    QVBoxLayout *l = new QVBoxLayout( _selectComposer );
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget( _composerList );
}

void WriteMail::nextStage()
{
    QWidget *curPage = widgetStack->currentWidget();
    if (!curPage) {
        composeStage();
    } else if (curPage == m_composerWidget) {
        if (m_composerInterface->isEmpty()) {
            discard();
        } else {
            detailsStage();
        }
    } else if (curPage == m_detailsPage) {
        sendStage();
    } else {
        qWarning("BUG: WriteMail::nextStage() called in unknown stage.");
    }
}

void WriteMail::sendStage()
{
    enqueue();
}

void WriteMail::detailsStage()
{
    if (!changed() && !_detailsOnly) {
        discard();
        return;
    }

    showDetailsPage();
}

void WriteMail::showDetailsPage()
{
    m_detailsPage->setType( m_composerInterface->messageType() );

    if (_detailsOnly) //fix resize problem if not showing composer
        widgetStack->setCurrentWidget(m_composerWidget);

    widgetStack->setCurrentWidget(m_detailsPage);
    
    m_mainWindow->setWindowTitle(m_composerInterface->displayName() + " " + tr("details"));
    m_previousAction->setVisible(!_detailsOnly);
}

void WriteMail::composeStage()
{
    if (composer().isEmpty())
        setComposer( QMailComposerFactory::defaultKey() );
    else
        m_composerInterface->setSignature( signature() );

    widgetStack->setCurrentWidget(m_composerWidget);

    QString task;
    if ((_action == Create) || (_action == Forward)) {
        task = (_action == Create ? tr("Create") : tr("Forward"));
        task += " " + m_composerInterface->displayName();
    } else if (_action == Reply) {
        task = tr("Reply");
    } else if (_action == ReplyToAll) {
        task = tr("Reply to all");
    }

    m_mainWindow->setWindowTitle(task);
    m_previousAction->setVisible(false);

    // Reset the action to default - create
    _action = Create;
}

bool WriteMail::readyToSend() const // Also check from field is not empty
{
    // Not ready to send if there's no 'from' address...
    if ((m_composerInterface
         && m_composerInterface->messageType() == QMailMessage::Email)
         && m_detailsPage->from().trimmed().isEmpty() )
        return false;

    return ( !m_detailsPage->to().trimmed().isEmpty()
             || !m_detailsPage->cc().trimmed().isEmpty()
             || !m_detailsPage->bcc().trimmed().isEmpty() );
}

bool WriteMail::isComplete() const
{
    return changed() && readyToSend();
}

bool WriteMail::saveChangesOnRequest()
{
    // ideally, you'd also check to see if the message is the same as it was
    // when we started working on it
    if (hasMessageChanged && 
        QMessageBox::warning(this, 
                             tr("Save to drafts"),
                             tr("Do you wish to save the message to drafts?"),
                             QMessageBox::Yes, 
                             QMessageBox::No) == QMessageBox::Yes) {
        draft();
    } else {
        discard();
    }

    return true;
}

void WriteMail::previousStage()
{
    QWidget *curPage = widgetStack->currentWidget();
    if (curPage == m_composerWidget)
        return; // no previous
    if (!curPage)
        composeStage();
    else if (curPage == m_detailsPage)
        composeStage();
    else
        qWarning("BUG: WriteMail::nextStage() called in unknown stage.");
}

bool WriteMail::buildMail()
{
    //retain the old mail id since it may have been a draft
    QMailId existingId = mail.id();
    
    // Ensure the signature of the selected account is used
    m_composerInterface->setSignature(signature());

    // Extract the message from the composer
    mail = m_composerInterface->message();
    m_detailsPage->getDetails( mail );

    mail.setDate( QMailTimeStamp::currentDateTime() );

    mail.setId(existingId);
    mail.setStatus( QMailMessage::Outgoing, true);
    mail.setStatus( QMailMessage::Downloaded, true);
    mail.setStatus( QMailMessage::Read,true);

    return true;
}

/*  TODO: connect this method to the input widgets so we can actually know whether
    the mail was changed or not */
bool WriteMail::changed() const
{
    if (!m_composerInterface || m_composerInterface->isEmpty())
        return false;

    return true;
}

// sharp 1839 to here
static void checkOutlookString(QString &str)
{
    int  pos = 0;
    int  newPos;
    QString  oneAddr;

    QStringList  newStr;
    if (str.indexOf(";") == -1) {
        // not Outlook style
        return;
    }

    while ((newPos = str.indexOf(";", pos)) != -1) {
        if (newPos > pos + 1) {
            // found some string
            oneAddr = str.mid(pos, newPos-pos);

            if (oneAddr.indexOf("@") != -1) {
                // not Outlook comment
                newStr.append(oneAddr);
            }
        }
        if ((pos = newPos + 1) >= str.length()) {
            break;
        }
    }

    str = newStr.join(", ");
}

void WriteMail::attach( const QContent &dl, QMailMessage::AttachmentsAction action )
{
    if (m_composerInterface) {
        m_composerInterface->attach( dl, action );
    } else {
        qWarning("WriteMail::attach called with no composer interface present.");
    }
}

void WriteMail::reply(const QMailMessage& replyMail, int type)
{
    const QString fwdIndicator(tr("Fwd"));
    const QString shortFwdIndicator(tr("Fw", "2 letter short version of Fwd for forward"));
    const QString replyIndicator(tr("Re"));

    const QMailAddress replyAddress(replyMail.from());
    const QString subject = replyMail.subject().toLower();

    QString toAddress;
    QString fromAddress;
    QString ccAddress;
    QString subjectText;

    //accountId stored in mail, but email address used for selecting
    //account, so loop through and find the matching account
    QListIterator<QMailAccount*> it = accountList->accountIterator();
    while (it.hasNext()) {
        QMailAccount* current = it.next();
        if (current->id() == replyMail.fromAccount()) {
            fromAddress = current->emailAddress();
            break;
        }
    }

    // work out the kind of mail to response
    // type of reply depends on the type of message
    // a reply to an mms is just a new mms message with the sender as recipient
    // a reply to an sms is a new sms message with the sender as recipient

    newMail(QMailComposerFactory::defaultKey( replyMail.messageType() ));
    if (composer().isEmpty())
        return;

    if (replyMail.messageType() == QMailMessage::Sms) {
        // SMS
        if (type == Forward) {
            QString bodyText;
            if (replyMail.status() & QMailMessage::Incoming)  {
                bodyText = replyAddress.displayName();
                bodyText += ":\n\"";
                bodyText += replyMail.body().data();
                bodyText += "\"\n--\n";
            } else {
                bodyText += replyMail.body().data();
            }

            QMailMessageContentType contentType("text/plain; charset=UTF-8");
            mail.setBody(QMailMessageBody::fromData(bodyText, contentType, QMailMessageBody::Base64));
            m_composerInterface->setMessage(mail);
        } else {
            QString from = replyAddress.address();
            if (!from.isEmpty()) {
                toAddress = from;
            }
        }
    } else if (replyMail.messageType() == QMailMessage::Mms) {
        // MMS
        if (type == Forward) {
            // Copy the existing mail
            mail = replyMail;
            mail.setId(QMailId());

            if ((subject.left(fwdIndicator.length() + 1) == (fwdIndicator.toLower() + ":")) ||
                (subject.left(shortFwdIndicator.length() + 1) == (shortFwdIndicator.toLower() + ":"))) {
                subjectText = replyMail.subject();
            } else {
                subjectText = fwdIndicator + ": " + replyMail.subject();
            }
            m_composerInterface->setMessage( mail );

            showDetailsPage();
        } else {
            if (subject.left(replyIndicator.length() + 1) == (replyIndicator.toLower() + ":")) {
                subjectText = replyMail.subject();
            } else {
                subjectText = replyIndicator + ": " + replyMail.subject();
            }
            toAddress = replyAddress.address();
        }
    } else if( replyMail.messageType() ==  QMailMessage::Email ) {
        // EMAIL
        QString originalText;
        int textPart = -1;

        // Find the body of this message
        if ( replyMail.hasBody() ) {
            originalText = replyMail.body().data();
        } else {
            for ( uint i = 0; i < replyMail.partCount(); ++i ) {
                const QMailMessagePart &part = replyMail.partAt(i);

                if (part.contentType().type().toLower() == "text") {
                    // This is the first text part, we will use as the forwarded text body
                    originalText = part.body().data();
                    textPart = i;
                    break;
                }
            }
        }

        if ( type == Forward ) {
            // Copy the existing mail
            mail = replyMail;
            mail.setId(QMailId());

            if ((subject.left(fwdIndicator.length() + 1) == (fwdIndicator.toLower() + ":")) ||
                (subject.left(shortFwdIndicator.length() + 1) == (shortFwdIndicator.toLower() + ":"))) {
                subjectText = replyMail.subject();
            } else {
                subjectText = fwdIndicator + ": " + replyMail.subject();
            }
        } else {
            if (subject.left(replyIndicator.length() + 1) == (replyIndicator.toLower() + ":")) {
                subjectText = replyMail.subject();
            } else {
                subjectText = replyIndicator + ": " + replyMail.subject();
            }

            QString str;
            if ( replyMail.replyTo().isNull() ) {
                str = replyAddress.address();
            } else {
                str = replyMail.replyTo().toString();
                mail.setReplyTo( QMailAddress() );
            }

            checkOutlookString(str);
            toAddress = str;

            QString messageId = mail.headerFieldText( "message-id" ).trimmed();
            if ( !messageId.isEmpty() )
                mail.setInReplyTo( messageId );
        }

        QString bodyText;
        if (type == Forward) {
            bodyText = "\n------------ Forwarded Message ------------\n";
            bodyText += "Date: " + replyMail.date().toString() + "\n";
            bodyText += "From: " + replyAddress.toString() + "\n";
            bodyText += "To: " + QMailAddress::toStringList(replyMail.to()).join(", ") + "\n";
            bodyText += "Subject: " + replyMail.subject() + "\n";
            bodyText += "\n" + originalText;
        } else {
            QDateTime dateTime = replyMail.date().toLocalTime();
            bodyText = "\nOn " + QTimeString::localYMDHMS(dateTime, QTimeString::Long) + ", ";
            bodyText += replyAddress.name() + " wrote:\n> ";

            int pos = bodyText.length();
            bodyText += originalText;
            while ((pos = bodyText.indexOf('\n', pos)) != -1)
                bodyText.insert(++pos, "> ");

            bodyText.append("\n");
        }

        // Whatever text subtype it was before, it's now plain...
        QMailMessageContentType contentType("text/plain; charset=UTF-8");

        if (mail.partCount() == 0) {
            // Set the modified text as the body
            mail.setBody(QMailMessageBody::fromData(bodyText, contentType, QMailMessageBody::Base64));
        } else if (textPart != -1) {
            // Replace the original text with our modified version
            QMailMessagePart& part = mail.partAt(textPart);
            part.setBody(QMailMessageBody::fromData(bodyText, contentType, QMailMessageBody::Base64));
        }

        if (type == ReplyToAll) {
            // Set the reply-to-all address list
            QList<QMailAddress> all;
            foreach (const QMailAddress& addr, replyMail.to() + replyMail.cc())
                if ((addr.address() != fromAddress) && (addr.address() != toAddress))
                    all.append(addr);

            QString cc = QMailAddress::toStringList(all).join(", ");
            checkOutlookString( cc );
            ccAddress = cc;
        }

        mail.removeHeaderField("From");
        m_composerInterface->setMessage( mail );
    }

    if (!toAddress.isEmpty())
        m_detailsPage->setTo( toAddress );
    if (!fromAddress.isEmpty())
        m_detailsPage->setFrom( fromAddress );
    if (!ccAddress.isEmpty())
        m_detailsPage->setCc( ccAddress );
    if (!subjectText.isEmpty())
        m_detailsPage->setSubject( subjectText );

    // ugh. we need to do this everywhere
    hasMessageChanged = false;
}

void WriteMail::modify(const QMailMessage& previousMessage)
{
    QString recipients = "";

    QString key( QMailComposerFactory::defaultKey( previousMessage.messageType() ) );
    if (key.isEmpty()) {
        qWarning() << "Cannot edit message of type:" << previousMessage.messageType();
    } else {
        newMail( QMailComposerFactory::defaultKey( previousMessage.messageType() ));
        if (composer().isEmpty())
            return;
        
        mail.setId( previousMessage.id() );
        mail.setFrom( previousMessage.from() );

        QMailAddress fromAddress( previousMessage.from() );
        m_detailsPage->setFrom( fromAddress.address() );
        m_detailsPage->setSubject( previousMessage.subject() );
        m_detailsPage->setTo( QMailAddress::toStringList(previousMessage.to()).join(", ") );
        m_detailsPage->setCc( QMailAddress::toStringList(previousMessage.cc()).join(", ") );
        m_detailsPage->setBcc( QMailAddress::toStringList(previousMessage.bcc()).join(", ") );

        m_composerInterface->setSignature( signature() );
        m_composerInterface->setMessage( previousMessage );

        // ugh. we need to do this everywhere
        hasMessageChanged = false;
    }
}

void WriteMail::setRecipient(const QString &recipient)
{
    if (m_detailsPage ) {
	m_detailsPage->setTo( recipient );
    } else {
        qWarning("WriteMail::setRecipient called with no composer interface present.");
    }
}

void WriteMail::setSubject(const QString &subject)
{
    if (m_detailsPage ) {
	m_detailsPage->setSubject( subject );
    } else {
        qWarning("WriteMail::setRecipient called with no composer interface present.");
    }
}

void WriteMail::setBody(const QString &text, const QString &type)
{
    if (!m_composerInterface)
        return;
    if (m_composerInterface->messageType() == QMailMessage::Mms)
        return;
    m_composerInterface->setText( text, type );
}

bool WriteMail::hasContent()
{
    // Be conservative when returning false, which means the message can
    // be discarded without user confirmation.
    if (!m_composerInterface)
        return true;
    return !m_composerInterface->isEmpty();
}

#ifndef QTOPIA_NO_SMS
void WriteMail::setSmsRecipient(const QString &recipient)
{
  m_detailsPage->setTo( recipient );
}
#endif

void WriteMail::setRecipients(const QString &emails, const QString & numbers)
{
    QString to;
    to += emails;
    to = to.trimmed();
    if (to.right( 1 ) != "," && !numbers.isEmpty()
        && !numbers.trimmed().startsWith( "," ))
        to += ", ";
    to +=  numbers;
    if (m_detailsPage ) {
        m_detailsPage->setTo( to );
    } else {
        qWarning("WriteMail::setRecipients called with no composer interface present.");
    }
}

void WriteMail::reset()
{
    mail = QMailMessage();

    if (m_composerInterface) {
        // Remove any associated widgets
        if (m_composerWidget) {
            widgetStack->removeWidget(m_composerWidget);
            m_composerWidget = 0;
        }

        // Remove and delete any existing details page
        if (m_detailsPage) {
            widgetStack->removeWidget(m_detailsPage);
            m_detailsPage->deleteLater();
            m_detailsPage = 0;
        }

        m_composerInterface->deleteLater();
        m_composerInterface = 0;
    }

    hasMessageChanged = false;
}

bool WriteMail::newMail(const QString& key, bool detailsOnly)
{
    bool success = true;

    reset();

    _detailsOnly = detailsOnly;

    if (key.isEmpty()) {
        // If there's just one composer option, select it immediately
        QString singularKey = _composerList->singularKey();
        if (!singularKey.isEmpty()) {
            success = composerSelected(singularKey);
        } else {
            _composerList->setSelected(composer());

            m_mainWindow->setWindowTitle( tr("Select type") );
            _selectComposer->setVisible(true);
            widgetStack->setCurrentWidget(_selectComposer);
        }
    } else {
        success = composerSelected(key);
    }

    return success;
}

void WriteMail::discard()
{
    emit discardMail();

    // Must delete the composer widget after emitting the discardMail signal
    reset();
}

void WriteMail::enqueue()
{
    if (!isComplete()) {
        if (!changed()) {
            QMessageBox::warning(qApp->activeWindow(),
                                 tr("Incomplete message"),
                                 tr("The unmodified message has been discarded without being sent"),
                                 tr("OK") );
            discard();
        } else {
            QString temp;
            QMessageBox::warning(qApp->activeWindow(),
                                 tr("Incomplete message"), 
                                 tr("The message could not be sent as no recipients have been entered. The message has been saved in the Drafts folder"),
                                 tr("OK") );
            draft();
        }
        return;
    }

    if (buildMail())
    {
        if(largeAttachments())
        {
            //prompt for action
            QMessageBox::StandardButton result;
            result = QMessageBox::question(qApp->activeWindow(),
                                           tr("Large attachments"),
                                           tr("The message has large attachments. Continue?"),
                                           QMessageBox::Yes | QMessageBox::No);
            if(result == QMessageBox::No)
            {
                draft();
                QMessageBox::warning(qApp->activeWindow(),
                                     tr("Message saved"), 
                                     tr("The message has been saved in the Drafts folder"),
                                     tr("OK") );

                return;
            }
        }
        emit enqueueMail(mail);
    }

    // Prevent double deletion of composer textedit that leads to crash on exit
    reset();
}

bool WriteMail::draft()
{
    bool result(false);

    if (changed()) {
        if (!buildMail()) {
            qWarning() << "draft() - Unable to buildMail for saveAsDraft!";
        } else {
            emit saveAsDraft( mail );
        }
        result = true;
    }

    // Since the existing details page may have hijacked the focus, we need to reset
    reset();

    return result;
}

void WriteMail::selectionCanceled()
{
    emit discardMail();

    _selectComposer->setVisible(false);
}

bool WriteMail::composerSelected(const QString &key)
{
    // We need to ensure that we can send for this composer
    QMailMessage::MessageType selectedType = QMailComposerFactory::messageType(key);

    if (!_sendTypes.contains(selectedType)) {
        _selectComposer->setVisible(false);
        emit noSendAccount(selectedType);
        return false;
    }

    setComposer(key);

    if(_detailsOnly) {
        detailsStage();
    } else {
        composeStage();
        QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Next);
    }

    _selectComposer->setVisible(false);
    return true;
}

void WriteMail::setComposer( const QString &key )
{
    if (m_composerInterface && m_composerInterface->key() == key)
        return;

    if (m_composerInterface) {
        // Remove any associated widgets
        if (m_composerWidget) {
            widgetStack->removeWidget(m_composerWidget);
            m_composerWidget = 0;
        }

        // Remove and delete any existing details page
        if (m_detailsPage) {
            widgetStack->removeWidget(m_detailsPage);
            delete m_detailsPage;
            m_detailsPage = 0;
        }

        delete m_composerInterface;
        m_composerInterface = 0;
    }

    m_composerInterface = QMailComposerFactory::create( key, this );
    if (!m_composerInterface)
        return;

    m_composerWidget = m_composerInterface->widget();
    connect( m_composerInterface, SIGNAL(contentChanged()), this, SLOT(messageChanged()) );
    connect( m_composerInterface, SIGNAL(finished()), this, SLOT(nextStage()), Qt::QueuedConnection );

    // Add standard actions to context menu for this composer (or its focus proxy)
    QWidget* focusWidget = m_composerWidget;
    if (focusWidget->focusProxy() != 0)
        focusWidget = focusWidget->focusProxy();

    QMenu* menu = QSoftMenuBar::menuFor(focusWidget);

    menu->addSeparator();
    m_composerInterface->addActions(menu);

    menu->addSeparator();
    menu->addAction(m_previousAction);
    menu->addAction(m_draftAction);
    menu->addAction(m_cancelAction);

    m_detailsPage = new DetailsPage( this, "send-message" );
    m_detailsPage->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    m_detailsPage->setType( m_composerInterface->messageType() );
    connect( m_detailsPage, SIGNAL(changed()), this, SLOT(detailsChanged()));
    connect( m_detailsPage, SIGNAL(sendMessage()), this, SLOT(sendStage()));
    connect( m_detailsPage, SIGNAL(cancel()), this, SLOT(saveChangesOnRequest()));

    if (accountList)
        m_detailsPage->setAccountList( accountList );

    m_composerInterface->setSignature( signature() );

    QMenu *detailsMenu = QSoftMenuBar::menuFor( m_detailsPage );
    detailsMenu->addSeparator();
    detailsMenu->addAction(m_previousAction);
    detailsMenu->addAction(m_draftAction);
    detailsMenu->addAction(m_cancelAction);

    // Can't save as draft until there has been a change
    m_draftAction->setVisible(false);

    widgetStack->addWidget(m_composerWidget);
    widgetStack->addWidget(m_detailsPage);
}

QString WriteMail::composer() const
{
    QString key;
    if (m_composerInterface)
        key = m_composerInterface->key();
    return key;
}

bool WriteMail::canClose()
{
    return widgetStack->currentWidget() == m_detailsPage;
}

void WriteMail::messageChanged()
{
    hasMessageChanged = true;

    if ( m_composerInterface ) {
        m_draftAction->setVisible( !m_composerInterface->isEmpty() );

        if ( m_composerInterface->isEmpty() ) 
            QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::RevertEdit);
    }
}

void WriteMail::detailsChanged()
{
    hasMessageChanged = true;
}

void WriteMail::setAction(ComposeAction action)
{
    _action = action;
}

bool WriteMail::forcedClosure()
{
    if (draft()) 
        return true;

    emit discardMail();
    return false;
}

bool WriteMail::largeAttachments() 
{
    //determine if the message attachments exceed our
    //limits

    quint64 totalAttachmentKB = 0;

    for(unsigned int i = 0; i < mail.partCount(); ++i)
    {
        const QMailMessagePart part = mail.partAt(i);
        if(!part.attachmentPath().isEmpty())
        {
            QFileInfo fi(part.attachmentPath());
            if(fi.exists())
                totalAttachmentKB += fi.size();
        }
    }

    totalAttachmentKB /= 1024;

    return (totalAttachmentKB > largeAttachmentsLimit());
}

uint WriteMail::largeAttachmentsLimit() const
{
    static uint limit = 0; 

    if(limit == 0)
    {
        const QString key("emailattachlimitkb");
        QSettings mailconf("Trolltech","qtmail");

        mailconf.beginGroup("qtmailglobal");
        if (mailconf.contains(key)) 
            limit = mailconf.value(key).value<uint>();
        else 
            limit = 2048; //default to 2MB
        mailconf.endGroup();
    }
    return limit;
}

QString WriteMail::signature() const 
{
    if (m_detailsPage)
        if (QMailAccount* account = m_detailsPage->fromAccount()) 
            if (account->useSig())
                return account->sig();

    return QString();
}

#include "writemail.moc"

