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

#include "detailspage.h"
#include "addresslist.h"
#include "accountlist.h"

#include <qsoftmenubar.h>
#include <qtopiaapplication.h>
#include <qtopia/pim/qcontact.h>
#ifdef QTOPIA4_TODO
#include <qtopia/pim/private/contactfieldselector_p.h>
#endif
#include <qtopia/pim/qcontactview.h>

#include <qcheckbox.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtoolbutton.h>
#include <qmenu.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <QScrollArea>
#include <QTimer>

class DetailsLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    DetailsLineEdit(QWidget *parent = 0);

signals:
    void send();
    void done();

protected:
    void focusInEvent(QFocusEvent *);
    void keyPressEvent(QKeyEvent *);
    void inputMethodEvent(QInputMethodEvent *e);

protected slots:
    virtual void updateMenuBar(const QString &text);
};

DetailsLineEdit::DetailsLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    connect(this, SIGNAL(textChanged(QString)),
            this, SLOT(updateMenuBar(QString)));
}

void DetailsLineEdit::focusInEvent(QFocusEvent *e)
{
    QLineEdit::focusInEvent(e);
    setEditFocus(true);
    updateMenuBar(text());
}

void DetailsLineEdit::keyPressEvent(QKeyEvent *e)
{
    static const bool keypadAbsent(style()->inherits("QThumbStyle"));

    if (keypadAbsent) {
        if (e->key() == Qt::Key_Back) {
            if (text().isEmpty())
                emit done();
            else
                emit send();
        } else {
            QLineEdit::keyPressEvent(e);
        }
    } else {
        if (e->key() == Qt::Key_Select)
            emit send();
        else if (e->key() == Qt::Key_Back && text().isEmpty())
            emit done();
        else
            QLineEdit::keyPressEvent(e);
    }
}

void DetailsLineEdit::inputMethodEvent(QInputMethodEvent *e)
{
    QLineEdit::inputMethodEvent(e);
    updateMenuBar(text() + e->preeditString());
}

void DetailsLineEdit::updateMenuBar(const QString &text)
{
    static const bool keypadAbsent(style()->inherits("QThumbStyle"));

    if (keypadAbsent) {
        if (text.isEmpty())
            QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Cancel, QSoftMenuBar::EditFocus);
        else
            QSoftMenuBar::setLabel(this, Qt::Key_Back, ":icon/qtmail/enqueue", tr("Send"));
    } else {
        QSoftMenuBar::setLabel(this, Qt::Key_Select, ":icon/qtmail/enqueue", tr("Send"));

        if (text.isEmpty())
            QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::RevertEdit, QSoftMenuBar::EditFocus);
        else
            QSoftMenuBar::clearLabel(this, Qt::Key_Back);
    }
}

//===========================================================================

class RecipientEdit : public DetailsLineEdit
{
    Q_OBJECT
public:
    RecipientEdit(QWidget *parent = 0);
    ~RecipientEdit();

    void setPhoneNumbersAllowed(bool allow) {
        m_allowPhoneNumbers = allow;
    }
    void setEmailAllowed(bool allow) {
        m_allowEmails = allow;
    }
    void setMultipleAllowed(bool allow) {
        m_allowMultiple = allow;
    }

public slots:
    void editRecipients();

protected:
    virtual void keyPressEvent(QKeyEvent *);

protected slots:
    virtual void updateMenuBar(const QString &text);
    void recipientsChanged();
    void updateRecipients(int);

private:
    bool m_allowPhoneNumbers;
    bool m_allowEmails;
    bool m_allowMultiple;
    AddressPicker* m_picker;
    QDialog* m_pickerDialog;
};

RecipientEdit::RecipientEdit(QWidget *parent)
    : DetailsLineEdit(parent), m_allowPhoneNumbers(false),
      m_allowEmails(false), m_allowMultiple(false),
      m_picker(0), m_pickerDialog(0)
{
}

RecipientEdit::~RecipientEdit()
{
    delete m_pickerDialog;
}

void RecipientEdit::updateRecipients(int r)
{
    if (r == QDialog::Accepted)
        setText(m_picker->addressList().join(", "));
}

void RecipientEdit::recipientsChanged()
{
    if (m_picker->addressList().isEmpty() || !m_allowMultiple)
        m_pickerDialog->accept();
    else
        m_pickerDialog->setWindowTitle(tr("Recipients"));
}

void RecipientEdit::editRecipients()
{
    if (m_picker == 0) {
        m_picker = new AddressPicker;
        connect(m_picker, SIGNAL(selectionChanged()),
                this, SLOT(recipientsChanged()));

        if (m_allowPhoneNumbers && !m_allowEmails)
            m_picker->setFilterFlags(QContactModel::ContainsPhoneNumber);
        else if (m_allowEmails && !m_allowPhoneNumbers)
            m_picker->setFilterFlags(QContactModel::ContainsEmail);
        else 
            m_picker->resetFilterFlags();

        m_pickerDialog = new QDialog;
        m_pickerDialog->setObjectName("select-contact");

        QVBoxLayout *vbl = new QVBoxLayout;
        vbl->addWidget(m_picker);
        m_pickerDialog->setLayout(vbl);

        connect(m_pickerDialog, SIGNAL(finished(int)), 
                this, SLOT(updateRecipients(int)));
    }

    m_pickerDialog->showMaximized();
    QtopiaApplication::showDialog(m_pickerDialog);

    if (m_picker->isEmpty())
        m_picker->addAddress();
    else
        m_pickerDialog->setWindowTitle(tr("Recipients"));

#ifdef QTOPIA4_TODO
    QList<QContact::ContactFields> fields;
    if( m_allowPhoneNumbers )
    {
        fields.append( PimContact::HomeMobile );
        fields.append( PimContact::BusinessMobile );
    }
    if( m_allowEmails )
    {
        fields.append( PimContact::Emails );
    }
    ContactFieldSelector picker( fields, "", true, this, "addressPicker", true );
    QStringList curAddr = QStringList::split( ",", le->text() );
    for( uint i = 0 ; i < curAddr.count() ; ++i )
        curAddr[i] = curAddr[i].stripWhiteSpace();
    picker.setSelectedFields( curAddr );
    int r = QtopiaApplication::execDialog( &picker );
    if (  r == QDialog::Accepted ) {
        QStringList unknownFieldData = picker.unknownFieldData();
        QString userData;
        if (unknownFieldData.count())
            userData = unknownFieldData.join( ", " );
        QString selectedFields;
        QValueList<int> selectedIndices = picker.selected();
        for( QValueList<int>::Iterator it = selectedIndices.begin() ;
                                        it != selectedIndices.end() ; ++it )
        {
            if( !selectedFields.isEmpty() )
                selectedFields += ", ";
            selectedFields += picker.fieldDataAt( *it );
        }
        if( !userData.isEmpty() && !selectedFields.isEmpty() )
            userData += ", ";
        le->setText( userData + selectedFields );
    }
#endif
}

void RecipientEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Select && text().isEmpty())
        editRecipients();
    else
        DetailsLineEdit::keyPressEvent(e);
}

void RecipientEdit::updateMenuBar(const QString &text)
{
    static const bool keypadAbsent(style()->inherits("QThumbStyle"));

    if (keypadAbsent) {
        if (text.isEmpty())
            QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Cancel, QSoftMenuBar::EditFocus);
        else
            QSoftMenuBar::setLabel(this, Qt::Key_Back, ":icon/qtmail/enqueue", tr("Send"));
    } else {
        if (text.isEmpty()) {
            QSoftMenuBar::setLabel(this, Qt::Key_Select, ":icon/addressbook/AddressBook", tr("Search"));
            QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::RevertEdit, QSoftMenuBar::EditFocus);
        } else {
            QSoftMenuBar::setLabel(this, Qt::Key_Select, ":icon/qtmail/enqueue", tr("Send"));
            QSoftMenuBar::clearLabel(this, Qt::Key_Back);
        }
    }
}


class RecipientSelectorButton : public QToolButton
{
    Q_OBJECT

public:
    RecipientSelectorButton(QWidget *parent, QLayout *layout, RecipientEdit *sibling);

private:
    static QIcon s_icon;
};

RecipientSelectorButton::RecipientSelectorButton(QWidget *parent, QLayout *layout, RecipientEdit* sibling)
    : QToolButton(parent)
{
    setFocusPolicy( Qt::NoFocus );
    setText( tr( "..." ) );
    setIcon(QIcon(":icon/addressbook/AddressBook"));

    connect( this, SIGNAL(clicked()), sibling, SLOT(setFocus()) );
    connect( this, SIGNAL(clicked()), sibling, SLOT(editRecipients()) );

    layout->addWidget( this );
}


//===========================================================================


static const int MaximumDefaultSubjectLength = 40;

DetailsPage::DetailsPage( QWidget *parent, const char *name )
    : QWidget( parent ), m_type( -1 ), m_accountList( 0 )
{
    m_ignoreFocus = true;
    setObjectName( name );
    QIcon abicon(":icon/addressbook/AddressBook");
    QMenu *menu = QSoftMenuBar::menuFor( this );
    if( !Qtopia::mousePreferred() )
    {
        menu->addAction( abicon, tr("From contacts", "Find recipient's phone number or email address from Contacts application"),
                         this, SLOT(editRecipients()) );
        menu->addSeparator();
#ifndef QT_NO_CLIPBOARD	
        menu->addAction( QIcon(":icon/copy"), tr("Copy"),
                         this, SLOT(copy()) );
        menu->addAction( QIcon(":icon/paste"), tr("Paste"),
                         this, SLOT(paste()) );
#endif
    }

    const int margin = 2;
    setMaximumWidth( qApp->desktop()->width() - 2 * margin );
    QGridLayout *l = new QGridLayout( this );
    int rowCount = 0;

    m_toFieldLabel = new QLabel( this );
    m_toFieldLabel->setText( tr( "To" ) );
    m_toBox = new QHBoxLayout( );
    m_toField = new RecipientEdit( this );
    m_toBox->addWidget( m_toField );
    m_toFieldLabel->setBuddy(m_toField);
    connect( m_toField, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
    connect( m_toField, SIGNAL(send()), this, SIGNAL(sendMessage()) );
    connect( m_toField, SIGNAL(done()), this, SIGNAL(cancel()) );
    l->addWidget( m_toFieldLabel, rowCount, 0 );
    QSoftMenuBar::addMenuTo(m_toField, menu);

    m_toPicker = ( Qtopia::mousePreferred() ? new RecipientSelectorButton(this, m_toBox, m_toField) : 0 );
    l->addLayout( m_toBox, rowCount, 2 );
    ++rowCount;

    m_ccFieldLabel = new QLabel( this );
    m_ccFieldLabel->setText( tr( "CC" ) );
    m_ccBox = new QHBoxLayout( );
    m_ccField = new RecipientEdit( this );
    m_ccBox->addWidget( m_ccField );
    m_ccFieldLabel->setBuddy(m_ccField);
    connect( m_ccField, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
    connect( m_ccField, SIGNAL(send()), this, SIGNAL(sendMessage()) );
    connect( m_ccField, SIGNAL(done()), this, SIGNAL(cancel()) );
    l->addWidget( m_ccFieldLabel, rowCount, 0 );
    QSoftMenuBar::addMenuTo( m_ccField, menu );

    m_ccPicker = ( Qtopia::mousePreferred() ? new RecipientSelectorButton(this, m_ccBox, m_ccField) : 0 );
    l->addLayout( m_ccBox, rowCount, 2 );
    ++rowCount;

    m_bccFieldLabel = new QLabel( this );
    m_bccFieldLabel->setText( tr( "BCC" ) );
    m_bccBox = new QHBoxLayout( );
    m_bccField = new RecipientEdit( this );
    m_bccBox->addWidget( m_bccField );
    m_bccFieldLabel->setBuddy(m_bccField);
    connect( m_bccField, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
    connect( m_bccField, SIGNAL(send()), this, SIGNAL(sendMessage()) );
    connect( m_bccField, SIGNAL(done()), this, SIGNAL(cancel()) );
    l->addWidget( m_bccFieldLabel, rowCount, 0 );
    QSoftMenuBar::addMenuTo( m_bccField, menu );

    m_bccPicker = ( Qtopia::mousePreferred() ? new RecipientSelectorButton(this, m_bccBox, m_bccField) : 0 );
    l->addLayout( m_bccBox, rowCount, 2 );
    ++rowCount;

    m_subjectFieldLabel = new QLabel( this );
    m_subjectFieldLabel->setText( tr( "Subject" ) );
    m_subjectField = new DetailsLineEdit( this );
    m_subjectFieldLabel->setBuddy(m_subjectField);
    connect( m_subjectField, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
    connect( m_subjectField, SIGNAL(send()), this, SIGNAL(sendMessage()) );
    connect( m_subjectField, SIGNAL(done()), this, SIGNAL(cancel()) );
    l->addWidget( m_subjectFieldLabel, rowCount, 0 );
    l->addWidget( m_subjectField, rowCount, 2 );
    ++rowCount;
    QSoftMenuBar::addMenuTo( m_subjectField, menu );

    m_deliveryReportField = new QCheckBox( tr("Delivery report"), this );
    l->addWidget( m_deliveryReportField, rowCount, 0, 1, 3, Qt::AlignLeft );
    ++rowCount;

    m_readReplyField = new QCheckBox( tr("Read reply"), this );
    l->addWidget( m_readReplyField, rowCount, 0, 1, 3, Qt::AlignLeft );
    ++rowCount;

    m_fromFieldLabel = new QLabel( this );
    m_fromFieldLabel->setEnabled( true );
    m_fromFieldLabel->setText( tr( "From" ) );
    m_fromField = new QComboBox( this );
    m_fromFieldLabel->setBuddy(m_fromField);
    m_fromField->setEnabled( true );
    m_fromField->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum )); // Why not automatic?
    connect( m_fromField, SIGNAL(activated(int)), this, SIGNAL(changed()) );
    l->addWidget( m_fromFieldLabel, rowCount, 0 );
    l->addWidget( m_fromField, rowCount, 2 );
    ++rowCount;
    QSoftMenuBar::addMenuTo( m_fromField, menu );

    QSpacerItem* spacer1 = new QSpacerItem( 4, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    l->addItem( spacer1, rowCount, 1 );
    ++rowCount;

    QList<QWidget*> tabOrderList;

    tabOrderList.append( m_toField );
    if( Qtopia::mousePreferred() && m_toPicker)
        tabOrderList.append( m_toPicker );
    tabOrderList.append( m_ccField );
    if( Qtopia::mousePreferred() && m_ccPicker)
        tabOrderList.append( m_ccPicker );
    tabOrderList.append( m_bccField );
    if( Qtopia::mousePreferred() && m_bccPicker)
        tabOrderList.append( m_bccPicker );
    tabOrderList.append( m_subjectField );
    tabOrderList.append( m_fromField );

    QListIterator<QWidget*> it( tabOrderList );
    QWidget *prev = 0;
    QWidget *next;
    while ( it.hasNext() ) {
        next = it.next();
        if ( prev )
            setTabOrder( prev, next );
        prev = next;
    }
}

void DetailsPage::setAccountList( AccountList *list )
{
    m_accountList = list;

    m_fromField->clear();
    m_fromField->addItems( m_accountList->emailAccounts() );
    if ((m_fromField->count() < 2) || (m_type != QMailMessage::Email)) {
        m_fromField->hide();
        m_fromFieldLabel->hide();
    } else {
        m_fromField->show();
        m_fromFieldLabel->show();
    }
}

void DetailsPage::editRecipients()
{
    RecipientEdit *edit = 0;
    if (Qtopia::mousePreferred()) {
        if( sender() == m_toPicker )
            edit = m_toField;
        else if( sender() == m_ccPicker )
            edit = m_ccField;
        else if( sender() == m_bccPicker )
            edit = m_bccField;
    } else {
        QWidget *w = focusWidget();
        if( w && w->inherits("RecipientEdit") )
            edit = static_cast<RecipientEdit *>(w);
    }
    if (edit)
        edit->editRecipients();
    else
        qWarning("DetailsPage::editRecipients: Couldn't find line edit for recipients.");
}

void DetailsPage::setType( int t )
{
    QtopiaApplication::InputMethodHint imHint = QtopiaApplication::Normal;
    if( m_type != t )
    {
        m_allowPhoneNumbers = false;
        m_allowEmails = false;
        m_type = t;
        m_ccField->hide();
        if (m_ccPicker)
            m_ccPicker->hide();
        m_ccFieldLabel->hide();
        m_bccField->hide();
        if (m_bccPicker)
            m_bccPicker->hide();
        m_bccFieldLabel->hide();
        m_subjectField->hide();
        m_subjectFieldLabel->hide();
        m_fromField->hide();
        m_fromFieldLabel->hide();
        m_readReplyField->hide();
        m_deliveryReportField->hide();

        if( t == QMailMessage::Mms )
        {
            m_allowPhoneNumbers = true;
            m_allowEmails = true;
            m_ccFieldLabel->show();
            m_ccField->show();
            if (m_ccPicker)
                m_ccPicker->show();
            m_bccFieldLabel->show();
            m_bccField->show();
            if (m_bccPicker)
                m_bccPicker->show();
            m_subjectField->show();
            m_subjectFieldLabel->show();
            m_readReplyField->show();
            m_deliveryReportField->show();
        }
        else if( t == QMailMessage::Sms )
        {
            m_allowPhoneNumbers = true;

        }
        else if( t == QMailMessage::Email )
        {
            m_allowEmails = true;
            m_ccFieldLabel->show();
            m_ccField->show();
            if (m_ccPicker)
                m_ccPicker->show();
            m_bccFieldLabel->show();
            m_bccField->show();
            if (m_bccPicker)
                m_bccPicker->show();
            m_subjectField->show();
            m_subjectFieldLabel->show();
            if (m_fromField->count() >= 2) {
                m_fromField->show();
                m_fromFieldLabel->show();
            }
        }

        if( m_allowPhoneNumbers )
            imHint = QtopiaApplication::PhoneNumber;
        else if( m_allowEmails )
            imHint = QtopiaApplication::Words;

        foreach (RecipientEdit* field, (QList<RecipientEdit*>() << m_toField << m_ccField << m_bccField)) {
            if (imHint == QtopiaApplication::Words)
                QtopiaApplication::setInputMethodHint(field, QtopiaApplication::Named, "email noautocapitalization");
            else
                QtopiaApplication::setInputMethodHint(field, imHint);

            field->setMultipleAllowed(true);
            field->setPhoneNumbersAllowed(m_allowPhoneNumbers);
            field->setEmailAllowed(m_allowEmails);
        }
    }

    layout()->activate();
}

void DetailsPage::getDetails( QMailMessage &mail )
{
    mail.setTo( QList<QMailAddress>() << QMailAddress( to() ) );
    mail.setCc( QList<QMailAddress>() << QMailAddress( cc() ) );
    mail.setBcc( QList<QMailAddress>() << QMailAddress( bcc() ) );
    mail.setSubject( subject() );
    if( mail.subject().isEmpty() || (m_type & QMailMessage::Sms) ) {
        QString subjectText;
        if ( (m_type == QMailMessage::Email) || !mail.hasBody() ) {
            subjectText = tr("(no subject)");
        } else {
            if (mail.contentType().content().toLower() == "text/x-vcard") {
                // Much nicer to create a readable subject rather than show vcard data.
                QList<QContact> contacts = QContact::readVCard(mail.body().data(QMailMessageBody::Decoded));
                if ( contacts.count() == 0 ) {
                    // Invalid VCard data, so just show raw data
                } else if ( contacts.count() == 1 ) {
                    QString name = tr( "vCard describing %1", "%1 = Person's name" );
                    QContact& contact = contacts[0];
                    if ( !contact.nickname().isEmpty() ) {
                        subjectText = name.arg( contact.nickname() );
                    } else if ( !contact.firstName().isEmpty() && !contact.lastName().isEmpty() ) {
                        subjectText = name.arg( contact.firstName() + " " + contact.lastName() );
                    } else if ( !contact.firstName().isEmpty() ) {
                        subjectText = name.arg( contact.firstName() );
                    } else {
                        subjectText = tr( "vCard describing a contact" );
                    }
                } else if ( contacts.count() > 1 ) {
                    subjectText = tr( "vCard describing multiple contacts" );
                }
            } 
            if (subjectText.isEmpty()) {
                subjectText = mail.body().data();
                if (subjectText.length() > MaximumDefaultSubjectLength) {
                    // No point having a huge subject.
                    subjectText = subjectText.left(MaximumDefaultSubjectLength) + QLatin1String("...");
                }
            }
        }
        mail.setSubject(subjectText);
    }
    QString fromAddress( from() );
    if ( !fromAddress.isEmpty() ) {
        if ( QMailAccount* account = fromAccount() )
            mail.setFrom( QMailAddress( account->userName(), account->emailAddress() ) );
        else
            mail.setFrom( QMailAddress( fromAddress ) );
    }
    if( m_type == QMailMessage::Mms ) {
        if ( m_deliveryReportField->isChecked() )
            mail.setHeaderField( "X-Mms-Delivery-Report", "Yes" );
        if ( m_readReplyField->isChecked() )
            mail.setHeaderField( "X-Mms-Read-Reply", "Yes" );
    }
    if( m_type & QMailMessage::Sms ) {
        // For the time being limit sending SMS messages so that they can
        // only be sent to phone numbers and not email addresses
        QString number = to();
        QString n;
        QStringList numbers;
        for ( int posn = 0, len = number.length(); posn < len; ++posn ) {
            uint ch = number[posn].unicode();
            if ( ch >= '0' && ch <= '9' ) {
                n += QChar(ch);
            } else if ( ch == '+' || ch == '#' || ch == '*' ) {
                n += QChar(ch);
            } else if ( ch == '-' || ch == '(' || ch == ')' ) {
                n += QChar(ch);
            } else if ( ch == ' ' ) {
                n += QChar(ch);
            } else if ( ch == ',' ) {
                if (!n.isEmpty())
                    numbers.append( n );
                n = "";
            } // else ignore
        }
        if (!n.isEmpty())
            numbers.append( n );
        mail.setTo( QMailAddress::fromStringList( numbers ) );
    }
}

void DetailsPage::setBcc( const QString &a_bcc )
{
    m_bccField->setText( a_bcc );
}

QString DetailsPage::bcc() const
{
    QString text;
    if( !m_bccField->isHidden() )
        text = m_bccField->text();
    return text;
}


void DetailsPage::setCc( const QString &a_cc )
{
    m_ccField->setText( a_cc );
}

QString DetailsPage::cc() const
{
    QString text;
    if( !m_ccField->isHidden() )
        text = m_ccField->text();
    return text;
}

void DetailsPage::setTo( const QString &a_to )
{
    m_toField->setText( a_to );
}

QString DetailsPage::to() const
{
    return m_toField->text();
}

QString DetailsPage::subject() const
{
    return m_subjectField->text();
}

void DetailsPage::setSubject( const QString &sub )
{
    m_subjectField->setText( sub );
}

QString DetailsPage::from() const
{
    return m_fromField->currentText();
}

QMailAccount* DetailsPage::fromAccount() const
{
    if (m_accountList)
        return m_accountList->getSmtpRefByMail( from() );

    return 0;
}

void DetailsPage::setFrom( const QString &from )
{
    int i = 0;
    for( const int n = static_cast<int>(m_fromField->count()) ; i < n; ++i ) {
        if( m_fromField->itemText( i ) == from ) {
            m_fromField->setCurrentIndex( i );
            break;
        }
    }
}

void DetailsPage::copy()
{
#ifndef QT_NO_CLIPBOARD	
    QWidget *fw = focusWidget();
    if( !fw )
        return;
    if( fw->inherits( "QLineEdit" ) )
        static_cast<QLineEdit*>(fw)->copy();
    else if( fw->inherits( "QTextEdit" ) )
        static_cast<QTextEdit*>(fw)->copy();
#endif
}

void DetailsPage::paste()
{
#ifndef QT_NO_CLIPBOARD	
    QWidget *fw = focusWidget();
    if( !fw )
        return;
    if( fw->inherits( "QLineEdit" ) )
        static_cast<QLineEdit*>(fw)->paste();
    else if( fw->inherits( "QTextEdit" ))
        static_cast<QTextEdit*>(fw)->paste();
#endif
}

void DetailsPage::clear()
{
    m_toField->clear();
    m_ccField->clear();
    m_bccField->clear();
    m_subjectField->clear();
    m_readReplyField->setChecked( false );
    // don't clear from fields
}

#include "detailspage.moc"
