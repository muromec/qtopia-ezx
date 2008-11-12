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

#include "callcontactlist.h"

#include <qtopia/pim/qphonenumber.h>

#include <qtimestring.h>
#include <qsoftmenubar.h>
#include <qtopiaservices.h>
#include <quniqueid.h>
#include <qtopiaipcenvelope.h>
#include <qthumbnail.h>
#include <qtopiaapplication.h>

#include <QAction>
#include <QTextDocument>
#include <QKeyEvent>
#include <QPainter>
#include <QDebug>
#include <QSettings>
#include <QMenu>

#include "savetocontacts.h"

CallContactItem::CallContactItem( QCallListItem cli, QObject *parent)
: QObject(parent), mFieldType(QContactModel::Invalid), mModel(0), clItem(cli)
{
}

QCallListItem CallContactItem::callListItem() const
{
    return clItem;
}

QContactModel::Field CallContactItem::fieldType() const
{
    if (mFieldType == QContactModel::Invalid && mModel) {
        QString number = contact().defaultPhoneNumber();
        mFieldType = contactNumberToFieldType(number);
    }
    return mFieldType;
}

QUniqueId CallContactItem::contactID() const
{
    return mId;
}

void CallContactItem::setContact( const QContact& cnt, const QString& number)
{
    mContact = cnt;
    mId = cnt.uid();
    mModel = 0;
    mFieldType = contactNumberToFieldType(number.simplified());
}

void CallContactItem::setContact( const QContactModel *m, const QUniqueId &id)
{
    mModel = m;
    mId = id;
    mFieldType = QContactModel::Invalid;
    mContact = QContact();
}

//return value undefined if (mFieldType == CallContactItem::Invalid)
QContact CallContactItem::contact() const
{
    if (mContact.uid() != mId && mModel)
        mContact = mModel->contact(mId);
    return mContact;
}

/*
   Most useful is the type of phone number of a contact
   so show that first.

   Next most useful is a contacts image, so fall back on that

   Finally, the type pixmap should be shown if nothing
   else is available
*/
QPixmap CallContactItem::decoration() const
{
    if (!mId.isNull())
        return contact().thumbnail();

    return typePixmap(clItem.type());
}

QPixmap CallContactItem::extraDecoration() const
{
    QIcon icon = QContactModel::fieldIcon(fieldType());
    return icon.isNull() ? QPixmap() : QPixmap(icon.pixmap(QSize(16,16)));
}

QString CallContactItem::text() const
{
    if (!mId.isNull())
        return contact().label();
    return clItem.number();
}

QString CallContactItem::extraInfoText( ) const
{
    if (!mId.isNull() && clItem.isNull())
    {
        return fieldTypeToContactDetail();
    }
    else if (!clItem.isNull())
    {
       QString desc;
       QCallListItem::CallType st = clItem.type();
       if ( st == QCallListItem::Dialed )
           desc = tr("Dialed");
       else if ( st == QCallListItem::Received )
           desc = tr("Received");
       else if ( st == QCallListItem::Missed )
           desc = tr("Missed");
       desc += " ";
       QDateTime dt = clItem.start();
       QDate callDate = dt.date();
       QTime callTime = dt.time();
       QString when("%1 %2");
       when = when.arg(QTimeString::localMD(callDate, QTimeString::Short))
           .arg(QTimeString::localHM(callTime, QTimeString::Medium));
       return desc + when;
    }
    else
    {
        qWarning("BUG: item is not contact and not in call list index");
        return QString();
    }
}

QString CallContactItem::number() const
{
    if (!mId.isNull() && clItem.isNull())
        return fieldTypeToContactDetail();
    else if (!clItem.isNull())
        return clItem.number();
    else
        return QString("");
}

QPixmap CallContactItem::typePixmap( QCallListItem::CallType type )
{
    QString typePixFileName;
    switch( type )
    {
        case QCallListItem::Dialed:
            typePixFileName = "phone/outgoingcall";
            break;
        case QCallListItem::Received:
            typePixFileName = "phone/incomingcall";
            break;
        case QCallListItem::Missed:
            typePixFileName = "phone/missedcall";
            break;
    }

    QIcon icon(":icon/"+typePixFileName);

    return icon.pixmap(QContact::thumbnailSize());
}

QContactModel::Field CallContactItem::contactNumberToFieldType(const QString& number) const
{
    QContact cnt = contact();
    static QList<QContactModel::Field> list;
    if ( list.count() == 0 ) {
        list = QContactModel::phoneFields();
        list.append(QContactModel::Emails);
    }

    int bestMatch = 0;
    QContactModel::Field bestField = QContactModel::Invalid;
    foreach(QContactModel::Field f, list) {
        QString candidate = QContactModel::contactField(cnt, f).toString();
        if ( candidate.isEmpty() )
            continue;
        int match = QPhoneNumber::matchNumbers(number, candidate);
        if (match > bestMatch) {
            bestField = f;
            bestMatch = match;
        }
    }
    return bestField;
}

QString CallContactItem::fieldTypeToContactDetail() const
{
    QContact cnt = contact();
    return QContactModel::contactField(cnt, fieldType()).toString();
}


QCallList::ListType CallContactItem::stateToType( QCallListItem::CallType st )
{
    if ( st == QCallListItem::Dialed )
        return QCallList::Dialed;
    else if ( st == QCallListItem::Missed )
        return QCallList::Missed;
    else if ( st == QCallListItem::Received )
        return QCallList::Received;
    else {
        qWarning("BUG: Invalid state passed to CallContactItem::stateToType");
        return QCallList::Dialed;
    }
}

//===================================================================

CallContactModel::CallContactModel( QCallList &callList, QObject *parent)
    :QAbstractListModel(parent), mCallList(callList), pk_matcher("text")
{
    mRawCallList = mCallList.allCalls();
}

CallContactModel::~CallContactModel() {}

CallContactItem * CallContactModel::itemAt( const QModelIndex & index ) const
{
    if (!index.isValid())
        return 0;

    return callContactItems.at(index.row());
}

int CallContactModel::findPattern(const QString &content) const
{
    const QString ctext = content.toLower();
    int idx = ctext.indexOf(mFilter);
    if (idx == -1)
        idx = pk_matcher.collate(ctext).indexOf(mFilter);
    return idx;
}

QVariant CallContactModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < rowCount())
    {
        CallContactItem * item = callContactItems.at(index.row());
        if (!item)
            return QVariant();
        QString text = item->text();
        switch(role)
        {
            case Qt::DecorationRole:
            case QContactModel::PortraitRole:
                return item->decoration();
            case Qt::DisplayRole:
                return text.isEmpty() ? item->extraInfoText() : text;
            case QContactModel::LabelRole:
                {
                    QString result = text.isEmpty() ? item->extraInfoText() : text;
                    return result.prepend("<b>").append("</b>");
                }
            case QContactModel::SubLabelRole:
                return text.isEmpty() ? QString() : item->extraInfoText();
            case QContactModel::StatusIconRole:
                return item->extraDecoration();
            default:
                break;
        }
    }
    return QVariant();
}

void CallContactModel::refresh()
{
    mRawCallList = mCallList.allCalls();
}

void CallContactModel::resetModel()
{
    while (!callContactItems.isEmpty())
        delete callContactItems.takeFirst();
}

void CallContactModel::setFilter(const QString& filter)
{
    bool ok = false;
    filter.toInt( &ok );
    if ( ok )
        mFilter = filter;
    else
        mFilter = pk_matcher.collate( filter );
    refresh();

    emit filtered( filter );
}

//==================================================================

CallContactDelegate::CallContactDelegate( QObject * parent)
    : QContactDelegate(parent)
{
}

CallContactDelegate::~CallContactDelegate() {}

QFont CallContactDelegate::secondaryFont(const QStyleOptionViewItem& o, const QModelIndex& idx) const
{
    QFont font = QContactDelegate::secondaryFont(o, idx);
    QFont f(font);
    f.setItalic(true);
    return f;
}


//==============================================================


CallContactListView::CallContactListView(QWidget * parent)
    :QListView(parent), addContactMsg(0)
{
    setFrameStyle(QFrame::NoFrame);

    mMenu = QSoftMenuBar::menuFor( this );

    QIcon addressbookIcon(":image/addressbook/AddressBook");

    mAddContact = mMenu->addAction(addressbookIcon, tr("Save to Contacts"), this, SLOT(addItemToContact()));
    mAddContact->setVisible(false);
    mOpenContact = mMenu->addAction(addressbookIcon, tr("Open Contact"), this, SLOT(openContact()));
    mOpenContact->setVisible(false);
    mSendMessage = mMenu->addAction(QIcon(":icon/txt"), tr("Send Message"), this, SLOT(sendMessageToItem()));
    mSendMessage->setVisible(false);
    mRelatedCalls = mMenu->addAction(QIcon(":icon/view"), tr("Related Calls"), this, SLOT(showRelatedCalls()));
    mRelatedCalls->setVisible(false);
    mAllCalls = mMenu->addAction(QIcon(":icon/view"), tr("All Calls"), this, SLOT(showAllCalls()));
    mAllCalls->setVisible(false);

    m_noResultMessage = tr("No matches.");
    setAlternatingRowColors(true);

    setSelectionMode(QAbstractItemView::SingleSelection);
}

CallContactListView::~CallContactListView() {}

void CallContactListView::paintEvent( QPaintEvent *pe )
{
    QListView::paintEvent(pe);
    if (cclm && !cclm->rowCount())
    {
        QWidget *vp = viewport();
        QPainter p( vp );
        QFont f = p.font();
        f.setBold(true);
        f.setItalic(true);
        p.setFont(f);
        p.drawText( 0, 0, vp->width(), vp->height(), Qt::AlignCenter,
                (cclm->filter().isEmpty() ? tr("No Items"): m_noResultMessage) );
    }
}

void CallContactListView::setModel(QAbstractItemModel* model)
{
    cclm  = qobject_cast<CallContactModel*>(model);
    if (!cclm)
    {
        qWarning("CallContactListView::setModel(): expecting model of type CallContactModel");
    }
    QListView::setModel(model);
}

void CallContactListView::addItemToContact()
{
    QModelIndex idx = selectionModel()->currentIndex();
    CallContactItem * cci = cclm->itemAt(idx);
    if (!cci)
        return;

    QString number = cci->text();
    if (cci->fieldType() == QContactModel::Invalid && !number.isEmpty())
        SavePhoneNumberDialog::savePhoneNumber(number);
}

void CallContactListView::openContact()
{
    QModelIndex idx = selectionModel()->currentIndex();
    CallContactItem* cci = cclm->itemAt(idx);
    if (!cci)
        return;

    QUniqueId cntID = cci->contactID();
    if (!cntID.isNull())
    {
        QtopiaServiceRequest req( "Contacts", "showContact(QUniqueId)" );
        req << cntID;
        req.send();
    }
}

void CallContactListView::sendMessageToItem()
{
    QModelIndex idx = selectionModel()->currentIndex();
    CallContactItem* cci = cclm->itemAt(idx);
    if (!cci)
        return;

    QString name = cci->text();
    QString number = cci->number();

    if (!number.isEmpty()) {
        QtopiaServiceRequest req( "SMS", "writeSms(QString,QString)");
        req << name << number;
        req.send();
        // XXX what about atachments
    }
}

void CallContactListView::showRelatedCalls()
{
    QModelIndex idx = selectionModel()->currentIndex();
    CallContactItem * cci = cclm->itemAt(idx);
    if ( !cci )
        return;
    if ( cci->contactID().isNull() )
        cclm->setFilter( cci->number() );
    else
        cclm->setFilter( cci->contact().label() );
}

void CallContactListView::showAllCalls()
{
    cclm->setFilter( QString() );
}

void CallContactListView::updateMenu()
{
    mSendMessage->setEnabled(false);
    mSendMessage->setVisible(false);
    mAddContact->setEnabled(false);
    mAddContact->setVisible(false);
    mOpenContact->setEnabled(false);
    mOpenContact->setVisible(false);
    mRelatedCalls->setEnabled(false);
    mRelatedCalls->setVisible(false);
    mAllCalls->setEnabled(false);
    mAllCalls->setVisible(false);

    CallContactItem* cci = cclm->itemAt(selectionModel()->currentIndex());
    if (!cci)
        return;

    QContactModel::Field fieldType = cci->fieldType();
    if ( fieldType == QContactModel::HomeMobile ||
         fieldType == QContactModel::BusinessMobile )
    {
        mSendMessage->setEnabled(true);
        mSendMessage->setVisible(true);
    }

    if ( !cci->contactID().isNull() ) {
        mOpenContact->setEnabled(true);
        mOpenContact->setVisible(true);
    }

    if ( fieldType == QContactModel::Invalid
        && !cci->number().trimmed().isEmpty() ) {
        mAddContact->setVisible(true);
        mAddContact->setEnabled(true);
    }

    if ( cci->number().trimmed().isEmpty() )
        return;

    if ( cclm->filter().isEmpty() ) {
        mRelatedCalls->setEnabled(true);
        mRelatedCalls->setVisible(true);
    } else {
        mAllCalls->setEnabled(true);
        mAllCalls->setVisible(true);
    }
}

QString CallContactListView::numberForIndex(const QModelIndex & idx) const
{
    QString number;
    CallContactItem* cci = cclm->itemAt(idx);
    if (!cci)
        return number;
    number = cci->number();
    return number;
}

QContact CallContactListView::contactForIndex(const QModelIndex & idx) const
{
    CallContactItem* cci = cclm->itemAt(idx);
    if (!cci)
        return QContact();
    return cci->contact();
}

void CallContactListView::reset()
{
    QListView::reset();
    mNumber = QString();
}

void CallContactListView::keyPressEvent( QKeyEvent *e )
{
    int key = e->key();
    if (key == Qt::Key_Call || key == Qt::Key_Yes) {
        QModelIndex idx = selectionModel()->currentIndex();
        if( idx.isValid() )
            emit requestedDial( numberForIndex(idx), contactForIndex(idx).uid() );
        else
            emit hangupActivated();
        e->accept();
    } else if (key == Qt::Key_Hangup  || key == Qt::Key_Back || key == Qt::Key_No) {
        emit hangupActivated();
        e->accept();
    } else if (key == Qt::Key_Flip) {
        QSettings cfg("Trolltech","Phone");
        cfg.beginGroup("FlipFunction");
        if (cfg.value("hangup").toBool()) {
            emit hangupActivated();
            e->accept();
        }
    } else {
        QListView::keyPressEvent( e );
    }
}

void CallContactListView::focusInEvent( QFocusEvent *focusEvent)
{
    QListView::focusInEvent( focusEvent );
    setEditFocus( true );
}

void CallContactListView::setEmptyMessage(const QString& newMessage)
{
    m_noResultMessage = newMessage;
}

