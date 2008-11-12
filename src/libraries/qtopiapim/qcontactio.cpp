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

#include "qcontactio_p.h"
#include <QSettings>
#include <QDebug>
#include <qglobal.h>

#include <qtranslatablesettings.h>
#include <qtopianamespace.h>
#include <qtopia/pim/qphonenumber.h>

#include <qtopiaipcenvelope.h>
#include <qfile.h>

#ifdef Q_OS_WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

/*!
  \class QContactIO
  \mainclass
  \module qpepim
  \ingroup pim
  \brief The QContactIO class provides an interface for accessing contact data stores such
  as xml, sql and sim.

  The QContactIO class provides an interface for accessing contact data stores such
  as xml, sql and sim.  This include functions for manipulating the data store,
  sorting and filtering, as well as code to aid in merging with other QContactIO objects.

  Functions that need to be implemented in a subclass are:

    void setField(QContactModel::Field k)
    QContactModel::Field field() const
    void setCategoryFilter(const QCategoryFilter &)
    QCategoryFilter categoryFilter() const
    QContact contact(const QUniqueId &) const
    QContact contact(int row) const
    int row(const QUniqueId &) const
    QUniqueId id(int row) const
    QVariant key(int row) const
    bool exists(const QUniqueId &) const
    bool contains(const QUniqueId &) const
    int count() const
    QVariant contactField(int row, QContactModel::Field k) const;
    bool setContactField(int row, QContactModel::Field k,  const QVariant &);
    bool removeContact(int row)
    bool removeContact(const QUniqueId & id)
    bool removeContact(const QContact &)
    bool removeContacts(const QList<int> &rows)
    bool removeContacts(const QList<QUniqueId> &ids)
    bool updateContact(const QContact &)
    QUniqueId addContact(const QContact &)

  QContactIO objects are created in QContactModel.
  \internal
*/

QList< QList<QVariant> > QContactIO::mFormat;
QList< QContactModel::Field > QContactIO::mFormatFieldOrder;
QList<QContactIO *> QContactIO::activeContacts;

/*!
  \internal
  Constructs a QContactIO with parent \a parent.
  access mode is set to \a m but otherwise is not used.
*/
QContactIO::QContactIO(QObject *parent)
    : QRecordIO(parent)
{
    // default format.
    // should make a qstringlist with tr's?
    //setFormat("firstname , _ lastname | lastname | firstname | company");
    QtopiaChannel *channel = new QtopiaChannel( "QPE/PIM",  this );

    connect( channel, SIGNAL(received(QString,QByteArray)),
            this, SLOT(pimMessage(QString,QByteArray)) );
    activeContacts.append(this);
}

/*!
  \internal
  Handles QCop messages sent to the QPE/PIM QCop Channel.

  Specifically the messages that notify of external process changing contacts.
*/
void QContactIO::pimMessage(const QString &message, const QByteArray &data)
{
    QDataStream ds(data);
    if (message == "addedContact(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId contact;
            ds >> contact;
            checkAdded(contact);
        }
    } else if (message == "removedContact(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId id;
            ds >> id;
            checkRemoved(id);
        }
    } else if (message == "removedContacts(int,QUuid,QList<QUniqueId>)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QList<QUniqueId> ids;
            ds >> ids;
            checkRemoved(ids);
        }
    } else if (message == "updatedContact(int,QUuid,QUniqueId)") {
        int pid;
        QUuid u;
        ds >> pid;
        ds >> u;
        if (pid != getpid() && u == contextId()) {
            QUniqueId contact;
            ds >> contact;
            checkUpdated(contact);
        }
    } else if (message == "reloadContacts()") {
        ensureDataCurrent();
    } else if (message == "reload(int)") {
        int force;
        ds >> force;
        ensureDataCurrent(force != 0);
    }
}

/*!
  \internal
  Destructs the QContactIO object.
  */
QContactIO::~QContactIO()
{
    activeContacts.removeAll(this);
}

/*!
  \internal
  Checks store for the added contact with identifer \a id.

  By default only calls the recordsUpdated() signal.

  Reimplement this function if the subclass caches data to force reload or if more efficient
  adding the contact for \a id to cache.
*/
void QContactIO::checkAdded(const QUniqueId &id)
{
    Q_UNUSED(id);
    emit recordsUpdated();
}

/*!
  \internal
  Checks store for removed contact with \a id.

  By default only calls the recordsUpdated() signal.

  Reimplement this function if the subclass caches data to force reload or if more efficient
  removing the contact for \a id from the cache.
*/
void QContactIO::checkRemoved(const QUniqueId &id)
{
    Q_UNUSED(id);
    emit recordsUpdated();
}

/*!
  \internal
  Checks store for removed contacts with identifiers in \a ids.

  By default only calls the recordsUpdated() signal.

  Reimplement this function if the subclass caches data to force reload or if more efficient
  to removing the list of contacts for \a ids from the cache.
*/
void QContactIO::checkRemoved(const QList<QUniqueId> &) { emit recordsUpdated(); }

/*!
  \internal
  Checks store for updates to contact with identifer \a id.

  By default only calls the recordsUpdated() signal.

  Reimplement this function if the subclass caches data to force reload or if more efficient
  to update \a contact in the cache.
*/
void QContactIO::checkUpdated(const QUniqueId &id)
{
    Q_UNUSED(id);
    emit recordsUpdated();
}

/*!
  \internal

  Ensures state of store is consistent with what is on permanent storage.  if \a force is true
  forces a reload of cache even if checks indicates is up to date.
*/
void QContactIO::ensureDataCurrent(bool force)
{
    Q_UNUSED(force);
    emit recordsUpdated();
}

/*!
  \internal

  Sends a message on the QPE/Pim QtopiaChannel indicating that this process has added the
  contact with identifer \a id to the data store.
*/
void QContactIO::notifyAdded(const QUniqueId &id)
{
    {
        QtopiaIpcEnvelope e("QPE/PIM", "addedContact(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }

    foreach(QContactIO *c, activeContacts) {
        if (c != this && c->contextId() == contextId())
            c->checkAdded(id);
    }
}

/*!
  \internal

  Sends a message on the QPE/Pim QtopiaChannel indicating that this process has updated the
  contact with identifer \a id in the data store.
*/
void QContactIO::notifyUpdated(const QUniqueId &id)
{
    {
        QtopiaIpcEnvelope e("QPE/PIM", "updatedContact(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }

    foreach(QContactIO *c, activeContacts) {
        if (c != this && c->contextId() == contextId())
            c->checkUpdated(id);
    }
}

/*!
  \internal

  Sends a message on the QPE/Pim QtopiaChannel indicating that this process has removed the
  contact for \a id from the data store.
*/
void QContactIO::notifyRemoved(const QUniqueId & id)
{
    {
        QtopiaIpcEnvelope e("QPE/PIM", "removedContact(int,QUuid,QUniqueId)");
        e << getpid();
        e << contextId();
        e << id;
    }

    foreach(QContactIO *c, activeContacts) {
        if (c != this && c->contextId() == contextId())
            c->checkRemoved(id);
    }
}

/*!
  \internal

  Sends a message on the QPE/Pim QtopiaChannel indicating that this process has removed the
  contacts for \a ids from the data store.
*/
void QContactIO::notifyRemoved(const QList<QUniqueId> &ids)
{
    {
        QtopiaIpcEnvelope e("QPE/PIM", "removedContacts(int,QUuid,QList<QUniqueId>)");
        e << getpid();
        e << contextId();
        e << ids;
    }

    foreach(QContactIO *c, activeContacts) {
        if (c != this && c->contextId() == contextId())
            c->checkRemoved(ids);
    }
}

/*!
  \internal

  Returns the value for field \a k of contact at \a row for the current filter and sort settings.
*/
QVariant QContactIO::contactField(int row, QContactModel::Field k) const
{
    return QContactModel::contactField(contact(row), k);
}

/*!
  \internal

  Sets the value for field \a k of contact at \a row for the current filter and sort settings
  to \a v.  Returns true if successfully updated, otherwise returns false.
*/
bool QContactIO::setContactField(int row, QContactModel::Field k,  const QVariant &v)
{
    QContact t = contact(row);
    if (QContactModel::setContactField(t, k, v))
        return updateContact(t);
    return false;
}

void QContactIO::initFormat()
{
    QSettings config( "Trolltech", "Contacts" );
    config.beginGroup( "formatting" );
    int curfmtn = config.value( "NameFormat" ).toInt();
    QString curfmt = config.value( "NameFormatFormat"+QString::number(curfmtn) ).toString();
    setFormat( curfmt );
    config.endGroup();
}

/*!
  Returns the label for \a contact based on the other fields set for \a contact
  and the current label format set.

  \sa setFormat()
*/
QString QContactIO::formattedLabel(const QContact &contact)
{
    if (mFormat.count() == 0)
        initFormat();
    foreach(QList<QVariant> f, mFormat) {
        QString value;
        QListIterator<QVariant> fit(f);
        bool match = true;
        while(fit.hasNext()) {
            QVariant v = fit.next();
            if (v.type() == QVariant::Int) {
                QContactModel::Field k = (QContactModel::Field)v.toInt();
                QString field = QContactModel::contactField(contact, k).toString();
                if (field.isEmpty()) {
                    match = false;
                    break;
                }
                value += field;
            } else if (v.type() == QVariant::String) {
                value += v.toString();
            }
        }
        if (match)
            return value;
    }
    return contact.firstName();
}

/*!
  Returns the label for the contact at the given \a row, based on the other fields
  and the current label format set.

  \sa setFormat()
*/
QString QContactIO::formattedLabel(int row)
{
    if (mFormat.count() == 0)
        initFormat();
    foreach(QList<QVariant> f, mFormat) {
        QString value;
        QListIterator<QVariant> fit(f);
        bool match = true;
        while(fit.hasNext()) {
            QVariant v = fit.next();
            if (v.type() == QVariant::Int) {
                QContactModel::Field k = (QContactModel::Field)v.toInt();
                QString field = contactField(row, k).toString();
                if (field.isEmpty()) {
                    match = false;
                    break;
                }
                value += field;
            } else if (v.type() == QVariant::String) {
                value += v.toString();
            }
        }
        if (match)
            return value;
    }
    return contactField(row, QContactModel::FirstName).toString();
}


/*!
  Sets the format for labels of contacts returned by the QContactModel to
  \a value.

  The format is a set of pattern separated by '|'s.  Each pattern is
  a set of space separated tokens.  A token can either be _ for a space,
  an identifier as from identifierKey(), or any string.  The format for label
  will the first pattern for which all fields specified are non null for the contact.

  For example:

  LastName , _ FirstName | LastName | FirstName | Company
*/
void QContactIO::setFormat(const QString &value) {
    QList< QList<QVariant> > newFormat;
    QList< QContactModel::Field > newFormatOrder;

    QList<QContactModel::Field> keys = labelKeys();
    QStringList tokens = value.split(' ');
    QList<QVariant> last;
    bool lastvalid = false;
    while(tokens.count() > 0) {
        QString token = tokens.takeFirst();
        QContactModel::Field key = QContactModel::identifierField(token);
        if (keys.contains(key)) {
            lastvalid = true;
            last.append(key);
            if (!newFormatOrder.contains(key) && key != QContactModel::NameTitle && key != QContactModel::Suffix)
                newFormatOrder.append(key);
        } else if (token == "|") {
            if (lastvalid)
                newFormat.append(last);
            lastvalid = false;
            last.clear();
        } else {
            token.replace("_", " ");
            last.append(token);
        }
    }
    if (lastvalid)
        newFormat.append(last);
    if (newFormat.count() > 0) {
        mFormat = newFormat;
        mFormatFieldOrder = newFormatOrder;

        foreach(QContactIO *c, activeContacts) {
            c->emitLabelFormatChanged();
        }
    }
}

void QContactIO::emitLabelFormatChanged()
{
    emit labelFormatChanged();
}

int QContactIO::formatCount()
{
    if (mFormat.count() == 0)
        initFormat();
    return mFormat.count();
}

QList<QVariant> QContactIO::format(int i)
{
    if (mFormat.count() == 0)
        initFormat();
    return mFormat[i];
}

QString QContactIO::format()
{
    if (mFormat.count() == 0)
        initFormat();
    QString expression;

    int fc = QContactIO::formatCount();

    for (int i = 0; i < fc; i++) {
        QList<QVariant> f = QContactIO::format(i);
        QListIterator<QVariant> fit(f);
        while(fit.hasNext()) {
            QVariant v = fit.next();
            if (v.type() == QVariant::Int) {
                QContactModel::Field k = (QContactModel::Field)v.toInt();
                if (k == QContactModel::Invalid || k == QContactModel::Label)
                    return QContactModel::fieldIdentifier(QContactModel::FirstName); // soft fail.
                expression += QContactModel::fieldIdentifier(k) + " ";
            } else if (v.type() == QVariant::String) {
                QString s = v.toString();
                if (v == " ")
                    expression += "_ ";
                else
                    expression += v.toString() + " ";
            }
            if (fit.hasNext())
                expression += "| ";
        }
    }
    return expression;
}

QList<QContactModel::Field> QContactIO::formatFieldOrder()
{
    if (mFormat.count() == 0)
        initFormat();
    return mFormatFieldOrder;
}

QList<QContactModel::Field> QContactIO::labelKeys()
{
    QList<QContactModel::Field> result;
    result << QContactModel::NameTitle;
    result << QContactModel::FirstName;
    result << QContactModel::MiddleName;
    result << QContactModel::LastName;
    result << QContactModel::Suffix;
    result << QContactModel::Company;
    result << QContactModel::Department;
    result << QContactModel::JobTitle;
    result << QContactModel::Office;
    return result;
}

QStringList QContactIO::labelIdentifiers()
{
    QList<QContactModel::Field> keys = labelKeys();
    QStringList result;
    foreach(QContactModel::Field k, keys) {
        result << QContactModel::fieldIdentifier(k);
    }
    return result;

}

// Note: this logic is also in QMailAddress.  Currently qtopiamail depends
// on qtopiapim, but not vice-versa, so a good way of sharing code will
// need to be determined.
static QString minimalEmailAddress(const QString& address) 
{
    QString result;

    // Email addresses can contained quoted sections (delimited with ""), or 
    // comments (including nested), delimited with parentheses.  Whitespace is
    // legal but removed to give the canonical form.

    int commentDepth = 0;
    bool quoted = false;
    bool escaped = false;

    // Remove any whitespace or comments from the email address
    const QChar* it = address.constData();
    const QChar* end = it + address.length();
    for ( ; it != end; ++it ) {
        if ( !escaped && ( *it == '\\' ) ) {
            escaped = true;
            continue;
        }

        if ( *it == '(' && !escaped && !quoted ) {
            commentDepth += 1;
        }
        else if ( *it == ')' && !escaped && !quoted && ( commentDepth > 0 ) ) {
            commentDepth -= 1;
        }
        else if ( quoted || !(*it).isSpace() ) {
            if ( !quoted && *it == '"' && !escaped ) {
                quoted = true;
            }
            else if ( quoted && *it == '"' && !escaped ) {
                quoted = false;
            }
            if ( commentDepth == 0 ) {
                result.append( *it );
            }
        }

        escaped = false;
    }

    return result;
}

QUniqueId QContactIO::matchEmailAddress(const QString &address, int &bestMatch) const
{
    bestMatch = 0;
    QList<QContactModel::Field> esf;
    esf << QContactModel::DefaultEmail;
    esf << QContactModel::Emails;
    const QString inputAddress(minimalEmailAddress(address));
    QContactModel::Field f;
    for (int i = 0; i < count(); ++i) {
        foreach(f, esf) {
            QStringList addresses = contactField(i, f).toStringList();
            foreach (QString element, addresses) {
                if (inputAddress.compare(minimalEmailAddress(element)) == 0) {
                    bestMatch = 100;
                    return QUniqueId(contactField(i, QContactModel::Identifier).toByteArray());
                }
            }
        }
    }
    return QUniqueId();
}

#ifdef QTOPIA_PHONE
QUniqueId QContactIO::matchPhoneNumber(const QString &phnumber, int &bestMatch) const
{
    bestMatch = 0;
    QUniqueId bestContact;
    QList<QContactModel::Field> nsf = phoneNumberSearchFields();
    QContactModel::Field f;
    for (int i = 0; i < count(); ++i) {
        foreach(f, nsf) {
            int match = QPhoneNumber::matchNumbers(phnumber, contactField(i, f).toString());
            if (match == 100) {
                bestMatch = match;
                return QUniqueId(contactField(i, QContactModel::Identifier).toByteArray());
            }

            if (match > bestMatch) {
                bestMatch = match;
                bestContact = QUniqueId(contactField(i, QContactModel::Identifier).toByteArray());
            }
        }
    }
    return bestContact;
}
#endif


#ifdef QTOPIA_PHONE
QMap<QChar, QString> QContactIO::mPhoneButtonText;
bool QContactIO::mPhoneButtonTextRead = false;

QMap<QChar, QString> QContactIO::phoneButtonText()
{
    if (mPhoneButtonTextRead)
        return mPhoneButtonText;

    QTranslatableSettings cfg(Qtopia::defaultButtonsFile(), QSettings::IniFormat); // No tr

    cfg.beginGroup("TextButtons");

    QString buttons = cfg.value("Buttons").toString(); // No tr

    for (int i = 0; i < (int)buttons.length(); i++) {
        QChar ch = buttons[i];
        if (!ch.isDigit())
            continue;

        QString tapfunc("Tap"); // No tr
        tapfunc += ch;

        QString fn = cfg.value(tapfunc).toString();

        if (fn[0] != '\'' && fn[0] != '"' )
            continue;

        fn = fn.mid(1);
        mPhoneButtonText.insert(ch, fn);
    }

    mPhoneButtonTextRead = true;
    return mPhoneButtonText;
}


QList<QContactModel::Field> QContactIO::labelSearchFields()
{
    QList<QContactModel::Field> l;
    l << QContactModel::FirstName;
    l << QContactModel::LastName;
    l << QContactModel::Company;
    return l;
}

QList<QContactModel::Field> QContactIO::phoneNumberSearchFields()
{
    QList<QContactModel::Field> l;
    l << QContactModel::OtherPhone;
    l << QContactModel::OtherMobile;
    l << QContactModel::OtherFax;
    l << QContactModel::OtherPager;
    l << QContactModel::HomePhone;
    l << QContactModel::HomeMobile;
    l << QContactModel::HomeFax;
    l << QContactModel::HomePager;
    l << QContactModel::BusinessPhone;
    l << QContactModel::BusinessMobile;
    l << QContactModel::BusinessFax;
    l << QContactModel::BusinessPager;
    return l;
}

#endif
