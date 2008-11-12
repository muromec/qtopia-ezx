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

#include <qmailmessage.h>
#include <qsoftmenubar.h>
#include <qtopiaapplication.h>

#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QAction>
#include <QTextEdit>
#include <QLabel>
#include <QLayout>
#include <QClipboard>
#include <QMenu>
#include <QSettings>
#include <QTextCursor>
#include <QContact>

#include "genericcomposer.h"
#include "templatetext.h"
#include <qtopia/serial/qgsmcodec.h>

#define SMS_CHAR_LIMIT 459

#ifndef QTOPIA_NO_SMS
//Qtmail sends either GSM or UCS2 encoded SMS messages
//If another encoding is ultimately used to send the message,
//these functions will return inaccurate results
void GenericComposer::smsLengthInfo(uint& estimatedBytes, bool& isUnicode)
{
    //calculate the number of consumed bytes
    //considering the gsm charset

    unsigned short c;
    unsigned short d;
    uint count = 0;
    QString unicodestr = text();
    for(int i = 0; i < unicodestr.length(); ++i)
    {
        c = unicodestr[i].unicode();
        if(c >= 256)
        {
            estimatedBytes = unicodestr.length() * 2;
            isUnicode = true;
            return;
        }
        else
        {
            d = QGsmCodec::twoByteFromUnicode(c);
            if(d >= 256)
                count += 2;
            else if(d == 0x10) //0x10 is unrecognised char
            {
                estimatedBytes = unicodestr.length() * 2; //non gsm char, so go unicode
                isUnicode = true;
                return;
            }
            else
                count += 1;
        }
    }
    isUnicode = false;
    estimatedBytes = count;
}
//estimates the number of messages that will be sent

int GenericComposer::smsCountInfo()
{
    bool isUnicode = false;
    uint numBytes = 0;
    int numMessages = 0;
    int len = text().length();

    smsLengthInfo(numBytes,isUnicode);

    if(isUnicode) //all 2 byte UCS2 so ok to use text length
    {
        if (len <= 70 ) {
            numMessages = 1;
        } else {
            // 67 = 70 - fragment_header_size (3).
            numMessages = ( len + 66 ) / 67;
        }
    }
    else
    {
        //use byte length instead of text length
        //as some GSM chars consume 2 bytes
        if ( numBytes <= 160 ) {
            numMessages = 1;
        } else {
        // 153 = 160 - fragment_header_size (7).
            numMessages = ( numBytes + 152 ) / 153;
        }
    }
    return numMessages;
}
#endif

class ComposerTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    ComposerTextEdit( QWidget *parent, const char *name = 0 );

    void limitedInsert( const QString &str);
#ifndef QT_NO_CLIPBOARD
    void limitedPaste();
#endif

signals:
    void finished();

private slots:
    void updateLabel();

protected:
    void keyPressEvent( QKeyEvent *e );
    void mousePressEvent( QMouseEvent *e );
    void inputMethodEvent( QInputMethodEvent *e );
};

ComposerTextEdit::ComposerTextEdit( QWidget *parent, const char *name )
    : QTextEdit( parent )
{
    setObjectName( name );
    setFrameStyle(NoFrame);
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    setLineWrapMode( QTextEdit::WidgetWidth );

    connect(this, SIGNAL(textChanged()), this, SLOT(updateLabel()));
    updateLabel();
}

void ComposerTextEdit::updateLabel()
{
    int charCount = toPlainText().length();

    if (charCount > 0) {
        QSoftMenuBar::setLabel(this, Qt::Key_Select, QSoftMenuBar::Next);
        if( Qtopia::mousePreferred() )
            QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Next);
        else
            QSoftMenuBar::clearLabel(this, Qt::Key_Back);
    } else {
        QSoftMenuBar::setLabel(this, Qt::Key_Select, QSoftMenuBar::NoLabel);
        QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Cancel);
    }
}

void ComposerTextEdit::keyPressEvent( QKeyEvent *e )
{
    int charCount = toPlainText().length();
    QChar c = e->text()[0];
    if( charCount >= SMS_CHAR_LIMIT &&
        (c.isLetterOrNumber() || c.isPunct() || c.isSpace()) )
        return;

#ifndef QT_NO_CLIPBOARD
    if( e->modifiers() & Qt::ControlModifier )
    {
        if( e->key() == Qt::Key_V )
        {
            limitedPaste();
            return;
        }
        else if( e->key() == Qt::Key_Y )
            return; //redo could be redo paste, and that could exceed limit
    }
#endif
    if (e->key() == Qt::Key_Select) {
        if (charCount == 0) {
            e->ignore();
        } else {
            e->accept();
            emit finished();
        }
        return;
    }

    if (e->key() == Qt::Key_Back) {
        if( Qtopia::mousePreferred() ) {
//            e->ignore();
            e->accept();
            emit finished();
            return;
        } else if (charCount == 0) {
            e->accept();
            emit finished();
            return;
        }
    }

    QTextEdit::keyPressEvent( e );
}

void ComposerTextEdit::limitedInsert( const QString &str)
{
    int curCharCount = toPlainText().length();
    if( curCharCount >= SMS_CHAR_LIMIT )
           return;
    QString strText = str;
    int strCharCount = strText.length();
    while( (strCharCount+curCharCount) > SMS_CHAR_LIMIT )
    {
        strText = strText.left( strText.length() -1 );
        strCharCount = strText.length();
    }
    if( !strText.isEmpty() )
    {
        textCursor().insertText( strText );
        ensureCursorVisible();
        emit textChanged();
    }

}

#ifndef QT_NO_CLIPBOARD
void ComposerTextEdit::limitedPaste()
{
    limitedInsert( QApplication::clipboard()->text() );
}
#endif

void ComposerTextEdit::mousePressEvent( QMouseEvent *e )
{
    if( e->button() == Qt::RightButton )
        return;
    QTextEdit::mousePressEvent( e );
}

void ComposerTextEdit::inputMethodEvent( QInputMethodEvent *e )
{
    if ( !e->commitString().isEmpty() ) {
        // Clear the commit string if it is makes the message longer
        // than the limit
        int charCount = toPlainText().length();
        if ( ( charCount + e->commitString().length() > SMS_CHAR_LIMIT ) ) {
            e->setCommitString( QString() );
        }
    }

    QTextEdit::inputMethodEvent( e );
}

GenericComposer::GenericComposer( QWidget *parent )
    : QWidget( parent ),
      m_vCard( false ),
      m_vCardData()
{
    QSettings cfg("Trolltech","qtmail");
    cfg.beginGroup( "GenericComposer" );

    m_showLimitAction = new QAction( tr("Show SMS Limit"), this );
    m_showLimitAction->setCheckable( true );
    m_showLimitAction->setChecked( cfg.value( "showSmsLimitIndicator", true ).toBool() );
    connect( m_showLimitAction, SIGNAL(triggered(bool)), this, SLOT(updateSmsLimitIndicator()) );

    m_templateTextAction = new QAction( tr("Insert template"), this );
    connect( m_templateTextAction, SIGNAL(triggered()), this, SLOT(templateText()) );

    m_smsLimitIndicator = new QLabel( this );
    m_smsLimitIndicator->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_textEdit = new ComposerTextEdit( this );
#ifdef QTOPIA4_TODO
    m_textEdit->setMaxLength( SMS_BYTE_LIMIT );
#endif
    connect( m_textEdit, SIGNAL(textChanged()), this, SLOT(textChanged()) );
    connect( m_textEdit, SIGNAL(textChanged()), this, SIGNAL(contentChanged()) );
    connect( m_textEdit, SIGNAL(finished()), this, SIGNAL(finished()));

    QVBoxLayout *l = new QVBoxLayout( this );
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    l->addWidget( m_smsLimitIndicator );
    l->addWidget( m_textEdit );

    setFocusProxy( m_textEdit );

    updateSmsLimitIndicator();
    textChanged();
}

GenericComposer::~GenericComposer()
{
    QSettings cfg("Trolltech","qtmail");
    cfg.beginGroup( "GenericComposer" );
    cfg.setValue( "showSmsLimitIndicator", m_showLimitAction->isChecked() );
}

void GenericComposer::addActions(QMenu* menu)
{
    menu->addAction(m_showLimitAction);
    menu->addAction(m_templateTextAction);
}

void GenericComposer::updateSmsLimitIndicator()
{
    if ( m_showLimitAction->isChecked() && !m_vCard ) {
        if( m_smsLimitIndicator->isHidden() )
            m_smsLimitIndicator->show();
    } else {
        if( !m_smsLimitIndicator->isHidden() )
            m_smsLimitIndicator->hide();
    }
}

void GenericComposer::textChanged()
{
    static bool rtl = QApplication::isRightToLeft();

    int charCount = m_textEdit->toPlainText().length();
    int remaining = SMS_CHAR_LIMIT - charCount; 

#ifndef QTOPIA_NO_SMS
    int numMessages = smsCountInfo();
#else
    int numMessages = 1;
#endif

    QString info = tr("%1/%2","e.g. 5/7").arg( rtl ? numMessages : remaining )
                                         .arg( rtl ? remaining : numMessages );
    m_smsLimitIndicator->setText( ' ' + info + ' ' );
}

void GenericComposer::templateText()
{
    TemplateTextDialog *templateTextDialog = new TemplateTextDialog( this, "template-text" );
    QtopiaApplication::execDialog( templateTextDialog );

    ComposerTextEdit *composer = qobject_cast<ComposerTextEdit *>( m_textEdit );
    if (templateTextDialog->result() && composer)
        composer->limitedInsert( templateTextDialog->text() );
    delete templateTextDialog;
}

void GenericComposer::setText( const QString &t, const QString &type )
{
#ifndef QTOPIA_NO_SMS
    if (type.contains(QLatin1String("text/x-vCard"), Qt::CaseInsensitive)) {
        QList<QContact> contacts = QContact::readVCard(t.toLatin1());

        if ( contacts.count() == 0 ) {
            // Invalid VCard data, so just show raw data
            m_textEdit->setPlainText( t );
        } else if ( contacts.count() == 1 ) {
            QString name = tr( "Message contains vCard for %1" );
            if ( !contacts[0].nickname().isEmpty() ) {
                m_textEdit->setPlainText( name.arg( contacts[0].nickname() ) );
            } else if ( !contacts[0].firstName().isEmpty() &&
                        !contacts[0].lastName().isEmpty() ) {
                m_textEdit->setPlainText( name.arg( contacts[0].firstName() +
                                                    " " +
                                                    contacts[0].lastName() ) );
            } else if ( !contacts[0].firstName().isEmpty() ) {
                m_textEdit->setPlainText( name.arg( contacts[0].firstName() ) );
            } else {
                m_textEdit->setPlainText(
                    tr( "Message contains vCard for a contact" ) );
            }
            m_vCard = true;
            m_vCardData = t;
        } else if ( contacts.count() > 1 ) {
            m_textEdit->setPlainText(
                tr( "Message contains vCard for multiple contacts" ) );
            m_vCard = true;
            m_vCardData = t;
        }
    } else
#else
    Q_UNUSED(type);
#endif
    {
        m_textEdit->setPlainText( t );
    }

    // Update GUI state
    m_templateTextAction->setVisible( !m_vCard );
    m_textEdit->setReadOnly( m_vCard );
    m_textEdit->setEditFocus( !m_vCard );
    if ( m_vCard ) {
        setFocusProxy( 0 );
    } else {
        setFocusProxy( m_textEdit );
    }
    updateSmsLimitIndicator();
}

QString GenericComposer::text() const
{
    if ( m_vCard )
        return m_vCardData;
    else
        return m_textEdit->toPlainText();
}

GenericComposerInterface::GenericComposerInterface( QWidget *parent )
    : QMailComposerInterface( parent )
{
    m_composer = new GenericComposer( parent );
    connect( m_composer, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()) );
    connect( m_composer, SIGNAL(finished()), this, SIGNAL(finished()) );
}

GenericComposerInterface::~GenericComposerInterface()
{
    delete m_composer;
}

bool GenericComposerInterface::isEmpty() const
{
    return m_composer->text().isEmpty();
}

void GenericComposerInterface::setMessage(const QMailMessage &mail )
{
    setText( mail.body().data(), mail.headerField("Content-Type").content() );
}

QMailMessage GenericComposerInterface::message() const
{
    QMailMessage mail;
    if( isEmpty() )
        return mail;

    QMailMessageContentType type( "text/plain; charset=UTF-8" );
    mail.setBody( QMailMessageBody::fromData( m_composer->text(), type, QMailMessageBody::Base64 ) );

    mail.setMessageType(QMailMessage::Sms);
    if (m_composer->isVCard())
        mail.setHeaderField(QLatin1String("Content-Type"), QLatin1String("text/x-vCard"));

    return mail;
}

void GenericComposerInterface::clear()
{
    m_composer->setText( QString(), QString() );
}

void GenericComposerInterface::setText( const QString &txt, const QString &type )
{
    m_composer->setText( txt, type );
}

QWidget *GenericComposerInterface::widget() const
{
    return m_composer;
}

void GenericComposerInterface::addActions(QMenu* menu) const
{
    m_composer->addActions(menu);
}

void GenericComposerInterface::attach( const QContent &, QMailMessage::AttachmentsAction )
{
    qWarning("Unimplemented function called %s %d, %s", __FILE__, __LINE__, __FUNCTION__ );
}

QTOPIA_EXPORT_PLUGIN( GenericComposerPlugin )

GenericComposerPlugin::GenericComposerPlugin()
    : QMailComposerPlugin()
{
}

QString GenericComposerPlugin::key() const
{
    return "GenericComposer";
}

QMailMessage::MessageType GenericComposerPlugin::messageType() const
{
    return QMailMessage::Sms;
}

QString GenericComposerPlugin::name() const
{
    return tr("Text message");
}

QString GenericComposerPlugin::displayName() const
{
    return tr("message");
}

QIcon GenericComposerPlugin::displayIcon() const
{
    static QIcon icon(":icon/txt");
    return icon;
}

bool GenericComposerPlugin::isSupported( QMailMessage::MessageType type ) const
{
    return (type == QMailMessage::Sms);
}

QMailComposerInterface* GenericComposerPlugin::create( QWidget *parent )
{
    return new GenericComposerInterface( parent );
}

#include "genericcomposer.moc"

