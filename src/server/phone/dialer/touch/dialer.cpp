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

#include "dialer.h"
#include "qtopiaserverapplication.h"
#include "servercontactmodel.h"

#include <QLabel>
#include <QLayout>
#include <QAction>
#include <QMenu>
#include <QVBoxLayout>

#include <qtopiaservices.h>
#include <qtopiaipcenvelope.h>
#include <qsoftmenubar.h>
#include <qtopiaapplication.h>

#include <qtopia/pim/qcontactview.h>
#include "homescreencontrol.h"
#include "savetocontacts.h"
#include "themecontrol.h"
#include <QDebug>

// declare DialerLineEdit
class DialerLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    DialerLineEdit( QWidget *parent = 0 );

signals:
    void hangupActivated();
    void numberSelected(const QString&);

protected slots:
    void updateIcons();

protected:
    void keyPressEvent( QKeyEvent *e );
    void focusInEvent( QFocusEvent *e );
};

// declare Dialer
class Dialer : public PhoneThemedView
{
    Q_OBJECT

public:
    Dialer( QWidget *parent = 0, Qt::WFlags f = 0 );
    virtual ~Dialer();

    QString digits() const;
    void setDigits( const QString& digits );
    void appendDigits( const QString& digits );

public slots:
    void clear();

signals:
    void dial(const QString&, const QUniqueId&);
    void closeMe();
    void keyEntered(const QString&);
    void filterSelect(const QString&, bool&);

protected slots:
    virtual void dialerItemClicked( ThemeItem *item );
    void msgReceived(const QString&, const QByteArray&);

private slots:
    void character();
    void star();
    void pause();
    void wait();
    void plus();
    void numberSelected(const QString&);
    void numberSelected();
    void selectContact();
    void saveToContact();
    void sms();
    virtual void selectCallHistory();
    void updateIcons( const QString &text );

protected:
    virtual void themeLoaded( const QString &theme );
    void keyPressEvent(QKeyEvent*);
    QWidget *newWidget(ThemeWidgetItem* input, const QString& name);
    void generatePressEvent( int key, const QString &txt );

    QMenu *characterMenu();

    DialerLineEdit *display;

private:
    QActionGroup *m_actions;

    QMenu *charMenu;
    QAbstractMessageBox *addContactMsg;
};

// define DialerLineEdit
DialerLineEdit::DialerLineEdit( QWidget *parent )
    : QLineEdit( parent )
{
    QFont f = font();
    f.setBold( true );
    setFont( f );

    connect( this, SIGNAL(textChanged(QString)), this, SLOT(updateIcons()) );
    updateIcons();
    QtopiaApplication::setInputMethodHint( this, QtopiaApplication::AlwaysOff );
}

void DialerLineEdit::updateIcons()
{
    if( text().length() )
        QSoftMenuBar::setLabel( this, Qt::Key_Back, QSoftMenuBar::BackSpace,
                                                QSoftMenuBar::AnyFocus );
    else
        QSoftMenuBar::setLabel( this, Qt::Key_Back, QSoftMenuBar::Cancel,
                                                QSoftMenuBar::AnyFocus );
    if( text().trimmed().isEmpty() )
        QSoftMenuBar::setLabel( this, Qt::Key_Select, QSoftMenuBar::NoLabel,
                                                QSoftMenuBar::AnyFocus );
    else
        QSoftMenuBar::setLabel( this, Qt::Key_Select, "phone/answer", "Call",
                                                QSoftMenuBar::AnyFocus );
}

void DialerLineEdit::keyPressEvent( QKeyEvent *e )
{
    switch( e->key() )
    {
        case Qt::Key_Hangup:
        case Qt::Key_No:
        case Qt::Key_Flip:
        {
            emit hangupActivated();
            e->accept();
            break;
        }
        case Qt::Key_Yes:
        case Qt::Key_Call:
        case Qt::Key_Select:
        {
            if( text().length() )
                emit numberSelected( text() );
            else
                return;
            break;
        }
        case Qt::Key_Back:
        case Qt::Key_Backspace:
        {
            if( text().isEmpty() )
            {
                emit hangupActivated();
            }
            else
            {
                backspace();
            }
            e->accept();
            break;
        }
        default:
            QLineEdit::keyPressEvent( e );
            //let unhandled keys fall through to parent widget
            break;
    }
}

void DialerLineEdit::focusInEvent( QFocusEvent *e )
{
    if( !Qtopia::mousePreferred() )
    {
        if( !hasEditFocus() )
            setEditFocus( true );
        e->accept();
    } else {
        QLineEdit::focusInEvent( e );
    }
}

Dialer::Dialer( QWidget *parent, Qt::WFlags f )
    : PhoneThemedView( parent, f ), display(0), m_actions( 0 ), charMenu( 0 ), addContactMsg( 0 )
{
    setObjectName("Dialer");
    setWindowTitle( tr("Dialer") );
    connect( this, SIGNAL(itemClicked(ThemeItem*)), this,
                                        SLOT(dialerItemClicked(ThemeItem*)) );
    ThemeControl::instance()->registerThemedView(this, "Dialer");

    QtopiaChannel* channel = new QtopiaChannel( "Qtopia/Phone/TouchscreenDialer", this );
    connect( channel, SIGNAL(received(QString,QByteArray)), this, SLOT(msgReceived(QString,QByteArray)) );
}

Dialer::~Dialer()
{
}

void Dialer::keyPressEvent( QKeyEvent* e )
{
    if( characterMenu()->isVisible() )  {
        characterMenu()->setVisible(false);
        return;
    }
    switch( e->key() ) {
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
//      handled manually by dialerItemClicked
//      case Qt::Key_Asterisk:
        case Qt::Key_NumberSign:
            e->accept();
            appendDigits( e->text() );
        break;
    default:
            QWidget::keyPressEvent( e );
        break;
    }
}

void Dialer::msgReceived(const QString& str, const QByteArray&)
{
    if( str == "selectContact()" ) {
        selectContact();
    } else if( str == "showCallHistory()" ) {
        selectCallHistory();
    } else if( str == "sms()" ) {
        sms();
    } else if( str == "hangup()" ) {
        emit closeMe();
    } else if( str == "saveToContact()" ) {
        saveToContact();
    } else if( str == "dial()" ) {
        numberSelected();
    }
}

/* Reimplemented from ThemedView */
QWidget *Dialer::newWidget(ThemeWidgetItem* /*input*/, const QString& name)
{
    if( Qtopia::mousePreferred() && name == "dialernumber" )
        return new DialerLineEdit( this );
    return 0;
}

void Dialer::dialerItemClicked( ThemeItem *item )
{
    if( !item || !item->isInteractive() )
        return;

    QString n = item->itemName();
    if( n.isNull() )
        return;
    if( n == "star" )
        character();
}

void Dialer::themeLoaded( const QString & )
{
    ThemeWidgetItem *i = (ThemeWidgetItem *)findItem( "dialernumber", ThemedView::Widget );
    if( !i ) {
        qWarning("No input field available for dialer theme.");
        display = new DialerLineEdit( this );
    } else {
        display = qobject_cast<DialerLineEdit *>(i->widget());
    }

    connect( display, SIGNAL(textChanged(QString)), this, SLOT(updateIcons(QString)) );
    connect( display, SIGNAL(textChanged(QString)), this, SIGNAL(keyEntered(QString)) );
    display->setFocus();
    display->setFocusPolicy(Qt::StrongFocus);
    setFocusPolicy(Qt::NoFocus);

    if( m_actions )
        delete m_actions;
    m_actions = new QActionGroup( this );
    m_actions->setExclusive( false );
    // if any of these items aren't provided by the theme, put them in the contextmenu
    QAction *newAction = 0;
    int actionCount = 0;
    if( !findItem( "selectcontact" ) ) {
        newAction = new QAction( QIcon( ":icon/addressbook/AddressBook" ),
                tr("Select Contact"), m_actions );
        connect( newAction, SIGNAL(triggered()), this, SLOT(selectContact()) );
        ++actionCount;
    }
    if( !findItem( "callhistory" ) ) {
        newAction = new QAction( QPixmap( ":image/callhistory/CallHistory"),
                tr("Call History"), m_actions );
        connect( newAction, SIGNAL(triggered()), this, SLOT(selectCallHistory()) );
        ++actionCount;
    }
    if( !findItem( "messages" ) ) {
        newAction = new QAction( QIcon( ":icon/txt" ),
                tr("Send Message"), m_actions );
        connect( newAction, SIGNAL(triggered()), this, SLOT(sms()) );
        ++actionCount;
    }
    if( !findItem( "savecontact" ) ) {
        newAction = new QAction( QIcon( ":image/addressbook/AddressBook" ),
                tr( "Save to Contacts" ), m_actions );
        connect( newAction, SIGNAL(triggered()), this, SLOT(saveToContact()) );
        ++actionCount;
    }
    if( actionCount > 0 )
    {
        QMenu *menu = new QMenu(this);
        menu->addActions(m_actions->actions());
        QSoftMenuBar::addMenuTo(display, menu);
    }
    else
    {
        delete m_actions;
        m_actions = 0;
    }

    connect( display, SIGNAL(numberSelected(QString)), this, SLOT(numberSelected(QString)) );
    connect( display, SIGNAL(hangupActivated()), this, SIGNAL(closeMe()) );
}

void Dialer::updateIcons( const QString &text )
{

    if( m_actions ) {
        m_actions->setEnabled( !text.trimmed().isEmpty() );
        m_actions->setVisible( !text.trimmed().isEmpty() );
    }
}

void Dialer::saveToContact()
{
    if( !(display && !display->text().isEmpty()) )
        return;
    SavePhoneNumberDialog::savePhoneNumber(display->text());
}

void Dialer::selectContact()
{
    QContactSelector contactSelector( false, this );
    contactSelector.setModel(ServerContactModel::instance());
    contactSelector.showMaximized();
    if( QtopiaApplication::execDialog( &contactSelector ) && contactSelector.contactSelected() )
    {
        QContact cnt = contactSelector.selectedContact();
        QPhoneTypeSelector typeSelector(cnt, QString());
        if (QtopiaApplication::execDialog( &typeSelector ))
            if( display )
                display->setText( typeSelector.selectedNumber() );
    }
}

void Dialer::selectCallHistory()
{
    HomeScreenControl::instance()->homeScreen()->showCallHistory(false, display ? display->text() : QString());
}

void Dialer::sms()
{
    QtopiaServiceRequest req( "SMS", "writeSms(QString,QString)" );
    req << QString() << (display ? display->text() : QString());
    req.send();
}

void Dialer::numberSelected()
{
    numberSelected( (display ? display->text() : QString()) );
}

void Dialer::numberSelected( const QString &number )
{
    // Filter for special GSM key sequences.
    if (!number.isEmpty()) {
        bool filtered = false;
        emit filterSelect( number, filtered );
        if (!filtered)
            emit dial(number, QUniqueId());
        closeMe();
    }
}

void Dialer::generatePressEvent( int key, const QString &txt )
{
    if( display )
        QtopiaApplication::postEvent( display, new QKeyEvent( QEvent::KeyPress, key, QFlags<Qt::KeyboardModifier>(Qt::NoModifier), txt ) );
}

void Dialer::clear()
{
    if( display ) {
        display->setText( QString() );
        updateIcons( QString() );
    }
}

QString Dialer::digits() const
{
    return (display ? display->text() : QString());
}


void Dialer::setDigits( const QString& digits )
{
    if( display ) {
        display->setText( digits );
        updateIcons( digits );
    }
}


void Dialer::appendDigits( const QString& digits )
{
    if( display ) {
        display->setText( display->text()+digits );
        updateIcons( display->text() );
    }
}

void Dialer::star()
{
    generatePressEvent( Qt::Key_Asterisk, "*" );
}

void Dialer::plus()
{
    generatePressEvent( Qt::Key_Plus, "+" );
}

void Dialer::pause()
{
    generatePressEvent( Qt::Key_P, "p" );
}

void Dialer::wait()
{
    generatePressEvent( Qt::Key_W, "w" );
}

void Dialer::character()
{
    ThemeItem *i = findItem( "star" );
    if( !i )
        return;

    QMenu *menu = characterMenu();
    QPoint pos = QPoint( i->rect().x(), i->rect().y() );
    pos.setX( pos.x() + i->rect().width() );
    menu->popup( QWidget::mapToGlobal(pos) );
}

QMenu *Dialer::characterMenu()
{
    if( !charMenu )
    {
        charMenu = new QMenu( this );
        charMenu->addAction( "*", this, SLOT(star()) );
        charMenu->addAction( "+", this, SLOT(plus()) );
        charMenu->addAction( "P", this, SLOT(pause()) );
        charMenu->addAction( "W", this, SLOT(wait()) );
    }
    return charMenu;
}

/*!
  \class PhoneTouchDialerScreen
  \ingroup QtopiaServer::PhoneUI
  \brief The PhoneTouchDialerScreen class implements a touchscreen dialer.
  
  This class is a Qtopia \l{QtopiaServerApplication#qtopia-server-widgets}{server widget}. 
  It is part of the Qtopia server and cannot be used by other Qtopia applications.

  \sa QAbstractServerInterface, QAbstractDialerScreen
 */

// define PhoneTouchDialerScreen

/*!
  Construct a new PhoneTouchDialerScreen instance with the appropriate \a parent
  and \a flags.
 */
PhoneTouchDialerScreen::PhoneTouchDialerScreen(QWidget *parent, Qt::WFlags flags)
: QAbstractDialerScreen(parent, flags)
{
    setWindowTitle( tr("Dialer") );
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    setLayout(layout);
    m_dialer = new Dialer(this);
    layout->addWidget(m_dialer);

    QObject::connect(m_dialer,
                     SIGNAL(dial(QString,QUniqueId)),
                     this,
                     SIGNAL(requestDial(QString,QUniqueId)));
    QObject::connect(m_dialer, SIGNAL(closeMe()), this, SLOT(close()));
    QObject::connect(m_dialer, SIGNAL(keyEntered(QString)), this, SLOT(keyEntered(QString)));
    QObject::connect(m_dialer, SIGNAL(filterSelect(QString,bool&)),
                     this, SIGNAL(filterSelect(QString,bool&)));
}

/*! \reimp */
void PhoneTouchDialerScreen::reset()
{
    m_dialer->clear();
}

/*! \reimp */
void PhoneTouchDialerScreen::appendDigits(const QString &digits)
{
    m_dialer->appendDigits(digits);
}

/*! \reimp */
void PhoneTouchDialerScreen::setDigits(const QString &digits)
{
    m_dialer->setDigits(digits);
}

/*! \reimp */
QString PhoneTouchDialerScreen::digits() const
{
    return m_dialer->digits();
}

/*! \internal */
void PhoneTouchDialerScreen::keyEntered(const QString &key)
{
    bool filtered = false;
    emit filterKeys( key, filtered );
    if ( filtered ) {
        m_dialer->setDigits( QString() );
    }
}

QTOPIA_REPLACE_WIDGET_WHEN(QAbstractDialerScreen, PhoneTouchDialerScreen, Touchscreen);
#include "dialer.moc"
