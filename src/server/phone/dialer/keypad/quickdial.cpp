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

#include "numberdisplay.h"
#include "quickdial.h"
#include "dialercontrol.h"
#include "qtopiaserverapplication.h"

#include <qtopia/pim/qphonenumber.h>
#include <qsoftmenubar.h>
#include <qtopiaapplication.h>
#include <quniqueid.h>
#include <qtopia/pim/qcontactmodel.h>

#include <QDialog>
#include <QLayout>
#include <QDebug>
#include <QKeyEvent>
#include <QSet>

// declare QuickDialModel
class QuickDialModel : public CallContactModel
{
    Q_OBJECT
public:
    QuickDialModel( QCallList& callList, QObject *parent = 0 );

    void setWildcardNumberActive(bool);
    bool wildcardNumberActive() const;

public slots:
    void refresh();
    void populate();

private:
    QContactModel *clm;
    bool wcActive;
};

// declare QuickDialContactView
class QuickDialContactView : public CallContactListView
{
    Q_OBJECT
public:
    QuickDialContactView(QWidget *parent);

private slots:
    void selectedNumber(const QModelIndex& idx);

signals:
    void numberSelected(const QString&, const QUniqueId&);
};

QuickDialContactView::QuickDialContactView(QWidget *parent)
    : CallContactListView(parent)
{
    connect( this, SIGNAL(activated(QModelIndex)), this, SLOT(selectedNumber(QModelIndex)) );
}

void QuickDialContactView::selectedNumber(const QModelIndex& idx)
{
    /* sub view instead if QContact */
    CallContactItem* cci = cclm->itemAt(idx);
    if (cci && !cci->contactID().isNull()) {
        /* should be able to select from list of numbers , if there is more than one */
        /* open dialog listing phone numbers of selected contact */

        QContact ent = cci->contact();

        QMap<QContact::PhoneType, QString> numbers = ent.phoneNumbers();

#if !defined(QTOPIA_VOIP)
        // If we don't have VOIP, we can't dial VOIP numbers
        numbers.remove(QContact::HomeVOIP);
        numbers.remove(QContact::BusinessVOIP);
        numbers.remove(QContact::VOIP);
#endif

        if (numbers.count() == 1) {
            QMap<QContact::PhoneType, QString>::iterator it = numbers.begin();
            emit numberSelected(it.value(), ent.uid());
        } else {
            QPhoneTypeSelector s(ent, QString());
            if (QtopiaApplication::execDialog(&s) && !s.selectedNumber().isEmpty())
                emit numberSelected(s.selectedNumber(), ent.uid());
        }
    } else {
        emit numberSelected(cci->number(), QUniqueId());
    }
}

//---------------------------------------------------------------------------

QuickDialModel::QuickDialModel( QCallList& callList, QObject *parent )
    : CallContactModel( callList, parent ), clm(0), wcActive(false)
{
}

void QuickDialModel::setWildcardNumberActive(bool b)
{
    if (wcActive == b)
        return;
    wcActive = b;
    refresh();
}

bool QuickDialModel::wildcardNumberActive() const
{
    return wcActive;
}

void QuickDialModel::refresh()
{
    if (filter().isEmpty()) {
        CallContactModel::resetModel(); //delete existing items
        CallContactModel::refresh(); //reread CallListItems
        return;
    }

    if (wcActive) {
        populate();
        return;
    }

    if (!clm) {
        clm = new QContactModel(this);
        connect(clm, SIGNAL(modelReset()), this, SLOT(populate()));
    }

    QSettings config( "Trolltech", "Contacts" );

    // load SIM/No SIM settings.
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
        clm->setVisibleSources(set);
    }

    if (filter() == clm->filterText())
        populate();
    else
        clm->setFilter(filter(), QContactModel::ContainsPhoneNumber);
}

void QuickDialModel::populate()
{
    CallContactModel::resetModel(); //delete existing items
    CallContactModel::refresh(); //reread CallListItems

    if( clm->filterText().isEmpty() )
        return; //don't display anything if the filter is empty

    QList<QCallListItem> cl = mRawCallList;

    //create contacts that match - alphabetical order
    //const int numContacts = mAllContacts.count();
    const QString filStr = filter();
    const int filLen = filStr.length();


    int numMatchingEntries = 0; //limit number of entries in list
    int index = 0;
    if (!wcActive) {
        QUniqueId cntid = clm->id(index++);
        while (!cntid.isNull())
        {
            QCallListItem clItem;
            CallContactItem* newItem = 0;

            // assumed matched by label...
            newItem = new CallContactItem(clItem, this);
            newItem->setContact(clm, cntid);
            callContactItems.append(newItem);

            numMatchingEntries++;

            /* should only sow enough to fit on the screen,
               although perhaps have a way of searching out the rest
               later.  Scrolling through 3000+ item != quick */
            if (numMatchingEntries > 20)
                break;
            cntid = clm->id(index++);
        }
    }

    //create remaining calllist items
    if (filLen==0 || filLen >= 3) {
        foreach(QCallListItem clItem, cl)
        {
            if( clItem.isNull() )
                continue;

            QString number = clItem.number();
            if( filStr.isEmpty() || (filLen >= 3 &&
                            (QPhoneNumber::matchPrefix( clItem.number(), filStr ) != 0)) )
            {
                CallContactItem *newItem = new CallContactItem(clItem, this);
                callContactItems.append(newItem);
            }
        }
    }
    reset();
}

//---------------------------------------------------------------------------

/*!
  \class PhoneQuickDialerScreen
  \brief The PhoneQuickDialerScreen class implements a keypad based dialer UI.
  \ingroup QtopiaServer::PhoneUI

  This class is a Qtopia \l{QtopiaServerApplication#qtopia-server-widgets}{server widget}. 
  It is part of the Qtopia server and cannot be used by other Qtopia applications.

  \sa QAbstractServerInterface, QAbstractDialerScreen
  */


/*!
  \fn void PhoneQuickDialerScreen::numberSelected(const QString&, const QUniqueId&)

  \internal
*/

/*!
  Constructs a new PhoneQuickDialerScreen object with the specified
  \a parent and widget flags \a fl.
*/
PhoneQuickDialerScreen::PhoneQuickDialerScreen( QWidget *parent, Qt::WFlags fl )
    : QAbstractDialerScreen( parent, fl ), mSpeedDial( false )
{
    QCallList &callList = DialerControl::instance()->callList();
    QVBoxLayout *l = new QVBoxLayout( this );
    l->setContentsMargins(0, 0, 0, 0);

    mNumberDS = new NumberDisplay( this );
    l->addWidget(mNumberDS);
    QtopiaApplication::setInputMethodHint( mNumberDS, QtopiaApplication::AlwaysOff );

    // cadams - fix for bug 188035
    QSoftMenuBar::setLabel( this, Qt::Key_Select, "phone/calls" , tr( "Dial", "dial highlighted number" ) );
    QSoftMenuBar::setLabel( this, Qt::Key_Back, QSoftMenuBar::Cancel );
    mDialList = new QuickDialContactView( this );
    l->addWidget(mDialList);
    mDialList->setEmptyMessage( tr("Type in phone number.") );

    mDialModel = new QuickDialModel(callList, mDialList);
    mDialList->setModel(mDialModel);
    CallContactDelegate * delegate = new CallContactDelegate( mDialList );
    mDialList->setItemDelegate( delegate );

    QItemSelectionModel * sm = mDialList->selectionModel();
    connect( sm, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
        mDialList, SLOT(updateMenu()) );

    connect( mNumberDS, SIGNAL(numberChanged(QString)), this,
                                                    SLOT(rejectEmpty(QString)) );
    connect( mNumberDS, SIGNAL(numberChanged(QString)), mDialModel,
                                                    SLOT(setFilter(QString)) );
    connect( mNumberDS, SIGNAL(speedDialed(QString)), this,
                                    SIGNAL(speedDial(QString)) );
    connect( mNumberDS, SIGNAL(numberSelected(QString)), this,
                                    SLOT(selectedNumber(QString)) );
    connect( mNumberDS, SIGNAL(hangupActivated()), this, SLOT(close()) );

    connect( this, SIGNAL(numberSelected(QString,QUniqueId)), this,
                                            SIGNAL(requestDial(QString,QUniqueId)) );

    connect( mDialList, SIGNAL(requestedDial(QString,QUniqueId)), mDialList,
                                    SIGNAL(numberSelected(QString,QUniqueId)) );
    connect( mDialList, SIGNAL(numberSelected(QString,QUniqueId)), this,
                                    SLOT(selectedNumber(QString,QUniqueId)) );
    connect( mDialList, SIGNAL(hangupActivated()), this, SLOT(close()) );
    setWindowTitle( tr("Quick Dial") );

    mNumberDS->installEventFilter( this );
    mDialList->installEventFilter( this );
    // Set the dialog to the maximum possible size.
}

/*!
  Destroys the PhoneQuickDialerScreen object.
  */
PhoneQuickDialerScreen::~PhoneQuickDialerScreen()
{
}

/*! \internal */
void PhoneQuickDialerScreen::showEvent( QShowEvent *e )
{
    QAbstractDialerScreen::showEvent( e );
    if( mNumber.length() )
    {
        // append digits after show event
        mNumberDS->appendNumber( mNumber, mSpeedDial );
        mNumber = QString();
        mSpeedDial = false;
    }
}

/*! \internal */
void PhoneQuickDialerScreen::selectedNumber( const QString &num )
{
    selectedNumber( num, QUniqueId() );
}

/*! \internal */
void PhoneQuickDialerScreen::selectedNumber( const QString &num, const QUniqueId &cnt )
{
    if( num.isEmpty() )
    {
        close();
        return;
    }
    // Filter for special GSM key sequences.
    bool filtered = false;
    emit filterSelect( num, filtered );
    if ( filtered ) {
        mNumber = QString();
        close();
    } else if (num.contains(QChar('d'), Qt::CaseInsensitive) &&
               !num.contains(QChar(':')) && !num.contains(QChar('@'))) {
        mDialModel->setWildcardNumberActive(true);
        mNumberDS->setWildcardNumber(num);
        mNumberDS->setFocus();
    } else {
        mNumber = num;
        close();
        emit numberSelected( mNumber, cnt );
    }
}

/*! \internal */
bool PhoneQuickDialerScreen::eventFilter( QObject *o, QEvent *e )
{
    QEvent::Type t = e->type();
    if( t != QEvent::KeyPress )
        return false;
    QKeyEvent *ke = (QKeyEvent *)e;
    int key = ke->key();
    QChar ch( key );
    if( o == mDialList )
    {
        switch( key )
        {
            case Qt::Key_Up:
            {
                QItemSelectionModel * selectModel = mDialList->selectionModel();
                if( !mDialModel->rowCount() ||
                    selectModel->isSelected(mDialModel->index(0)) )
                {
                    selectModel->clear();
                    mNumberDS->setFocus();
                    mNumberDS->setEditFocus(true);
                    return true;
                }
                break;
            }
            case Qt::Key_Down:
            {
                QItemSelectionModel * selectModel = mDialList->selectionModel();
                if( !mDialModel->rowCount() ||
                        selectModel->isSelected(mDialModel->index(mDialModel->rowCount()-1)) )
                {
                    selectModel->clear();
                    mNumberDS->setFocus();
                    mNumberDS->setEditFocus(true);
                    return true;
                }
                break;
            }
            case Qt::Key_0:
            case Qt::Key_1:
            case Qt::Key_2:
            case Qt::Key_3:
            case Qt::Key_4:
            case Qt::Key_5:
            case Qt::Key_6:
            case Qt::Key_7:
            case Qt::Key_8:
            case Qt::Key_9:
            {
                mNumberDS->appendNumber( ke->text() );
                mNumberDS->setFocus();
                mNumberDS->setEditFocus(true);
                return true;
            }
            case Qt::Key_Backspace:
            {
                mNumberDS->setFocus();
                mNumberDS->setEditFocus(true);
                QString curNumber = mNumberDS->number();
                if( !curNumber.isEmpty() )
                    mNumberDS->backspace();
                return true;
            }
            default:
                break;
        }
    }
    else if( o == mNumberDS )
    {
        QModelIndex idx;
        if ( key == Qt::Key_Down || key == Qt::Key_Up ) {
            if ( !mDialModel->rowCount() )
                return true;    // Do not move focus if no matching number exists
            if ( key == Qt::Key_Down )
                idx = mDialModel->index(0);
            else
                idx = mDialModel->index(mDialModel->rowCount()-1);
        }
        if (idx.isValid()) {
            mDialList->setFocus();
            mDialList->setCurrentIndex(idx);
            mDialList->setEditFocus(true);
            return true;
        }
    }
    return false;
}

/*! \reimp */
void PhoneQuickDialerScreen::reset()
{
    mNumberDS->clear();
    mNumberDS->setFocus();
    mDialModel->setWildcardNumberActive(false);
    mNumber = QString();
    mDialList->setCurrentIndex(QModelIndex());
    mDialList->scrollToTop();
}

/*! \internal */
void PhoneQuickDialerScreen::rejectEmpty( const QString &t )
{
    if( t.isEmpty() && !isVisible() ) {
        close();
     } else {
        // Fitler special GSM key sequences that act immediately (e.g. *#06#).
        bool filtered = false;
        emit filterKeys( t, filtered );
        if ( filtered ) {
            mNumber = QString();
            close();
        }
    }
}

/*! \internal */
void PhoneQuickDialerScreen::appendDigits( const QString &digits, bool refresh,
                              bool speedDial )
{
    if( !refresh && isVisible() )
        qWarning("BUG: appending digits that will never be seen to quick dial");
    if( refresh )
        mNumberDS->appendNumber( digits, speedDial );
    else {
        mSpeedDial = mSpeedDial | speedDial;
        mNumber += digits;
    }
}

/*! \reimp */
QString PhoneQuickDialerScreen::digits() const
{
    if( mNumber.isEmpty() )
        return mNumberDS->number();
    return mNumber;
}

/*! \reimp */
void PhoneQuickDialerScreen::setDigits(const QString &digits)
{
    reset();
    appendDigits(digits, false, false);
}

/*! \reimp */
void PhoneQuickDialerScreen::appendDigits(const QString &digits)
{
    appendDigits(digits, false, true);
}

QTOPIA_REPLACE_WIDGET_WHEN(QAbstractDialerScreen, PhoneQuickDialerScreen, Keypad);

#include "quickdial.moc"

