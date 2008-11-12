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

#include "contactmessagehistorylist.h"

#include "qsoftmenubar.h"
#include "qcontactmodel.h"
#include "qcontactview.h"

#include "contactdetails.h"

#include "qtimestring.h"

#include "qtopiaservices.h"

#include "qmailmessage.h"
#include "qmailstore.h"
#include "qmailmessagekey.h"

#include <QValueSpaceItem>

#include <QKeyEvent>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPhoneNumber>

// -------------------------------------------------------------
// ContactMessageHistoryItem
// -------------------------------------------------------------

// Just to store the mailbox and message id
class ContactMessageHistoryItem : public QStandardItem
{
    public:
        ContactMessageHistoryItem( const QIcon& icon, const QString& str);
        virtual ~ContactMessageHistoryItem();

        int type() const {return QStandardItem::UserType;}

        QString mMessageMailbox;
        QMailId mMessageId;
};

ContactMessageHistoryItem::ContactMessageHistoryItem( const QIcon& icon, const QString& str)
    :  QStandardItem(icon, str)
{
}

ContactMessageHistoryItem::~ContactMessageHistoryItem()
{
}

// -------------------------------------------------------------
// ContactMessageHistoryModel
// -------------------------------------------------------------
class ContactMessageHistoryModel : public QStandardItemModel
{
    Q_OBJECT
    public:

        ContactMessageHistoryModel( QObject *parent = 0);
        virtual ~ContactMessageHistoryModel();

        virtual void refresh();

        void setContact( const QContact& contact )      {mContact = contact;refresh();}
        QContact contact() const                        {return mContact;}

        void setContactModel(QContactModel *model)      {mContactModel = model; refresh();}

    protected slots:
        void newMessageCountChanged();

    protected:
        void addRecord(bool sent, const QMailMessage& msg);

        QContact mContact;
        QContactModel* mContactModel;
        QValueSpaceItem *mVSItem;
};

ContactMessageHistoryModel::ContactMessageHistoryModel( QObject* parent )
    : QStandardItemModel(parent), mContactModel(0)
{
    mVSItem = new QValueSpaceItem("Communications/Messages/NewMessages");
    connect(mVSItem, SIGNAL(contentsChanged()), this, SLOT(newMessageCountChanged()));
}

ContactMessageHistoryModel::~ContactMessageHistoryModel()
{
}

void ContactMessageHistoryModel::newMessageCountChanged()
{
    refresh();
}

void ContactMessageHistoryModel::addRecord(bool sent, const QMailMessage &m)
{
    static QIcon sentMessageIcon(":icon/qtmail/sendmail");
    static QIcon receivedMessageIcon(":icon/qtmail/getmail");

    ContactMessageHistoryItem * newItem = new ContactMessageHistoryItem(sent ? sentMessageIcon : receivedMessageIcon, m.subject());
    switch(m.messageType()) {
        case QMailMessage::Mms:
            {
                static QIcon mmsIcon(":icon/multimedia");
                newItem->setData(mmsIcon, ContactHistoryDelegate::SecondaryDecorationRole);
            }
            break;
        case QMailMessage::Sms:
            {
                static QIcon smsIcon(":icon/txt");
                newItem->setData(smsIcon, ContactHistoryDelegate::SecondaryDecorationRole);
            }
            break;
        default:
            {
                static QIcon emailIcon(":icon/email");
                newItem->setData(emailIcon, ContactHistoryDelegate::SecondaryDecorationRole);
            }
            break;
    }

    QDateTime messageTime = m.date().toLocalTime();
    newItem->setData(messageTime, ContactHistoryDelegate::UserRole);

    QString desc = sent ? tr("Sent %1 %2", "Sent 4th July 17:42") : tr("Received %1 %2", "Received 4th July 17:42");
    QString subtext = desc.arg(QTimeString::localMD(messageTime.date())).arg(QTimeString::localHM(messageTime.time(), QTimeString::Short));

    newItem->setData(subtext, ContactHistoryDelegate::SubLabelRole);

    newItem->mMessageMailbox = m.fromMailbox();
    newItem->mMessageId = m.id();
    appendRow(newItem);
}

void ContactMessageHistoryModel::refresh()
{
    clear();

    QMap<QContact::PhoneType, QString> contactNumbers = mContact.phoneNumbers();
    QList<QString> emailAddresses = mContact.emailList();

    if (!contactNumbers.isEmpty() || !emailAddresses.isEmpty()) {
        // We do two queries - one for from, one for to.
        QMailMessageKey msgsTo;
        QMailMessageKey msgsFrom;

        // Phone numbers
        foreach(const QString &num, contactNumbers.values()) {
            QMailMessageKey to(QMailMessageKey::Recipients, num, QMailMessageKey::Contains);
            QMailMessageKey from(QMailMessageKey::Sender, num, QMailMessageKey::Equal);
            msgsTo |= to;
            msgsFrom |= from;
        }

        // Email addresses
        foreach(const QString &email, emailAddresses) {
            QMailMessageKey to(QMailMessageKey::Recipients,email,QMailMessageKey::Contains);
            QMailMessageKey from(QMailMessageKey::Sender,email,QMailMessageKey::Equal);
            msgsTo |= to;
            msgsFrom |= from;
        }

        // Now get the messages sent to this contact
        QMailIdList ml = QMailStore::instance()->queryMessages(msgsTo);
        foreach (QMailId id, ml) {
            addRecord(true, QMailMessage(id,QMailMessage::Header));
        }

        // And messages from
        ml = QMailStore::instance()->queryMessages(msgsFrom);
        foreach (QMailId id, ml) {
            addRecord(false, QMailMessage(id,QMailMessage::Header));
        }
    }

    // Make sure we sort on the right role
    setSortRole(ContactHistoryDelegate::UserRole);

    // and sort it
    sort(0, Qt::DescendingOrder);
}


// -------------------------------------------------------------
// ContactMessageHistoryList
// -------------------------------------------------------------
ContactMessageHistoryList::ContactMessageHistoryList( QWidget *parent )
    : QWidget( parent ), mInitedGui(false), mModel(0), mListView(0), mContactModel(0)
{
    setObjectName("cmhl");

    QSoftMenuBar::setLabel(this, Qt::Key_Back,
                           QSoftMenuBar::Back, QSoftMenuBar::AnyFocus);
    QSoftMenuBar::setLabel(this, Qt::Key_Select,
                           QSoftMenuBar::NoLabel, QSoftMenuBar::AnyFocus);
}

ContactMessageHistoryList::~ContactMessageHistoryList()
{

}

void ContactMessageHistoryList::init( const QContact &entry )
{
    ent = entry;
    if (!mModel)
        mModel = new ContactMessageHistoryModel(this);
    mModel->setContact(ent);
    mModel->setContactModel(mContactModel);

    /* Create our UI, if we haven't */
    if (!mInitedGui) {
        mInitedGui = true;

        QVBoxLayout *main = new QVBoxLayout();
        mListView = new QListView();
        mListView->setItemDelegate(new ContactHistoryDelegate(mListView));
        mListView->setResizeMode(QListView::Adjust);
        mListView->setLayoutMode(QListView::Batched);
        mListView->setSelectionMode(QAbstractItemView::SingleSelection);
        mListView->setModel(mModel);
        mListView->setFrameStyle(QFrame::NoFrame);
        mListView->installEventFilter(this);

        connect(mListView, SIGNAL(activated(QModelIndex)), this, SLOT(showMessage(QModelIndex)));
        connect(mListView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(updateItemUI(QModelIndex)));
        main->addWidget(mListView);
        main->setMargin(0);
        setLayout(main);
    }
}

void ContactMessageHistoryList::updateItemUI(const QModelIndex& idx)
{
    if (idx.isValid()) {
        QSoftMenuBar::setLabel(this, Qt::Key_Select,
                           QSoftMenuBar::View, QSoftMenuBar::AnyFocus);
    } else {
        QSoftMenuBar::setLabel(this, Qt::Key_Select,
                               QSoftMenuBar::NoLabel, QSoftMenuBar::AnyFocus);
    }

    if (idx.isValid())
        mListView->selectionModel()->select(idx, QItemSelectionModel::Select);
}

void ContactMessageHistoryList::showMessage(const QModelIndex &idx)
{
    if (idx.isValid()) {
        ContactMessageHistoryItem * cmhi = static_cast<ContactMessageHistoryItem*>(mModel->itemFromIndex(idx));
        if (cmhi) {
            QtopiaServiceRequest req( "Email", "viewMail(QMailId)" );
            req << cmhi->mMessageId;
            req.send();
        }
    }
}

void ContactMessageHistoryList::setModel(QContactModel *model)
{
    mContactModel = model;
    if (mModel)
        mModel->setContactModel(mContactModel);
}

bool ContactMessageHistoryList::eventFilter( QObject *o, QEvent *e )
{
    if(o == mListView && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = (QKeyEvent *)e;
        if (ke->key() == Qt::Key_Back ) {
            emit backClicked();
            return true;
        }
    }
    return false;
}

void ContactMessageHistoryList::keyPressEvent( QKeyEvent *e )
{
    switch(e->key())
    {
        case Qt::Key_Back:
            emit backClicked();
            return;
        case Qt::Key_Call:
        // TODO handleCall();
            return;
    }

    QWidget::keyPressEvent(e);
}

#include "contactmessagehistorylist.moc"
