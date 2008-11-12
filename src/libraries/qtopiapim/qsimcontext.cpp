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

#include <qsimcontext_p.h>
#include <qsiminfo.h>
#include <qcontactsqlio_p.h>
#include <qtopialog.h>

#include <QDebug>
/***************
 * CONTEXT
 **************/

const char *userEntries = "SM";
const char *serviceNumbers = "SN";

struct ExtensionMap
{
    QContactModel::Field type;
    const char * text;
};

// these are english versions.  should translate?
// also, first ones are Qtopia export extensions.  Could add
// another list for fuzzy matching.
// extensions the same length for label consistency
static const ExtensionMap SIMextensions[] = {
    { QContactModel::HomePhone, "/hp" }, // no tr
    { QContactModel::HomeMobile, "/hm" }, // no tr
    { QContactModel::HomeFax, "/hf" }, // no tr
    { QContactModel::BusinessPhone, "/bp" }, // no tr
    { QContactModel::BusinessMobile, "/bm" }, // no tr
    { QContactModel::BusinessFax, "/bf" }, // no tr
    { QContactModel::BusinessPager, "/bg" }, // no tr

    { QContactModel::HomePhone, "/h" }, // no tr
    { QContactModel::HomeMobile, "/m" }, // no tr
    { QContactModel::BusinessPhone, "/w" }, // no tr
    { QContactModel::BusinessFax, "/o" }, // no tr

    { QContactModel::Invalid, 0 }
};

struct RegExpExtensionMap
{
    QContactModel::Field type;
    QRegExp expression;
};

static const RegExpExtensionMap SIMRegExpExtensions[] = {
    { QContactModel::HomePhone, QRegExp("[ /](h|hp|home)$") }, // no tr
    { QContactModel::HomeMobile, QRegExp("[ /](m|hm|mob)$") }, // no tr
    { QContactModel::HomeFax, QRegExp("[ /](o|other)$") }, // no tr
    { QContactModel::BusinessPhone, QRegExp("[ /](b|w|wk|bp|wp|work)$") }, // no tr
    { QContactModel::BusinessFax, QRegExp("[ /](bo|wo)$") }, // no tr
    { QContactModel::BusinessMobile, QRegExp("[ /](bm|wm)$") }, // no tr
    { QContactModel::BusinessPager, QRegExp("[ /](bpa|wpa)$") }, // no tr
    { QContactModel::Invalid, QRegExp() }
};

class QContactSimSyncer : public QObject
{
    Q_OBJECT
public:
    QContactSimSyncer(ContactSqlIO *access, const QString &cardType, const QPimSource &source, QObject *parent = 0);

    QString simType() const { return mSimType; }
    int labelLimit() const { return SIMLabelLimit; }
    int numberLimit() const { return SIMNumberLimit; }
    int firstIndex() const { return SIMListStart; }
    int lastIndex() const { return SIMListEnd; }
    int count() const { return phoneData.count(); }

    QString card() const { return mActiveCard; };

    QUniqueId id(const QString &card, int index) const;

    bool done() const { return readState == PhoneBookRead; }

    QContact simContact(const QUniqueId &u) const;
signals:
    void simCardContactsUpdated(int contextId);

private slots:
    void simIdentityChanged(const QString &, const QDateTime &);
    void updatePhoneBook( const QString &store, const QList<QPhoneBookEntry> &list );
    void updatePhoneBookLimits( const QString &store, const QPhoneBookLimits &value );
    void updateSqlEntries();

private:
    enum ReadState {
        PhoneBookIdRead = 0x1,
        PhoneBookLimitsRead = 0x2,
        PhoneBookEntriesRead = 0x4,
        PhoneBookRead = PhoneBookIdRead | PhoneBookLimitsRead | PhoneBookEntriesRead
    };

    ContactSqlIO *mAccess;
    int readState;

    const QString mSimType;
    const QPimSource mSource;
    int SIMLabelLimit;
    int SIMNumberLimit;
    int SIMListStart;
    int SIMListEnd;
    QList<QPhoneBookEntry> phoneData;
    QString mActiveCard;
    QDateTime mInsertTime;
    bool initialReadComplete;

    mutable QPreparedSqlQuery addNameQuery;
    mutable QPreparedSqlQuery addNumberQuery;
    mutable QPreparedSqlQuery updateNameQuery;
    mutable QPreparedSqlQuery updateNumberQuery;
    mutable QPreparedSqlQuery removeNameQuery;
    mutable QPreparedSqlQuery removeNumberQuery;
    mutable QPreparedSqlQuery selectNameQuery;
    mutable QPreparedSqlQuery selectNumberQuery;

    mutable QPreparedSqlQuery selectIdQuery;
    mutable QPreparedSqlQuery insertIdQuery;
};

QContactSimContext::QContactSimContext(QObject *parent, QObject *access)
    : QContactContext(parent)
{
    mPhoneBook = new QPhoneBook( QString(), this );
    mSimInfo = new QSimInfo( QString(), this );

    mAccess = qobject_cast<ContactSqlIO *>(access);
    Q_ASSERT(mAccess);

    mSync = new QContactSimSyncer(mAccess, userEntries, defaultSource(), this);
    connect(mSync, SIGNAL(simCardContactsUpdated(int)), this, SLOT(notifyUpdate(int)));
    connect(this, SIGNAL(simIdentityUpdated(QString,QDateTime)), mSync, SLOT(simIdentityChanged(QString,QDateTime)));

    mServiceNumbers = new QContactSimSyncer(mAccess, serviceNumbers, serviceNumbersSource(), this);
    connect(mServiceNumbers, SIGNAL(simCardContactsUpdated(int)), this, SLOT(notifyUpdate(int)));
    connect(this, SIGNAL(simIdentityUpdated(QString,QDateTime)), mServiceNumbers, SLOT(simIdentityChanged(QString,QDateTime)));

    connect( mSimInfo, SIGNAL(inserted()), this, SLOT(readSimIdentity()) );
    connect( mSimInfo, SIGNAL(removed()), this, SLOT(readSimIdentity()) );

    connect( mPhoneBook, SIGNAL(entries(QString,QList<QPhoneBookEntry>)),
            mSync, SLOT(updatePhoneBook(QString,QList<QPhoneBookEntry>)) );
    connect( mPhoneBook, SIGNAL(limits(QString,QPhoneBookLimits)),
            mSync, SLOT(updatePhoneBookLimits(QString,QPhoneBookLimits)) );
    connect( mPhoneBook, SIGNAL(entries(QString,QList<QPhoneBookEntry>)),
            mServiceNumbers, SLOT(updatePhoneBook(QString,QList<QPhoneBookEntry>)) );
    connect( mPhoneBook, SIGNAL(limits(QString,QPhoneBookLimits)),
            mServiceNumbers, SLOT(updatePhoneBookLimits(QString,QPhoneBookLimits)) );

    mPhoneBook->getEntries();
    mPhoneBook->requestLimits();
    readSimIdentity();
}

void QContactSimContext::notifyUpdate(int context)
{
    QSet<int> filter = mAccess->contextFilter();
    if (!filter.contains(context))
        mAccess->invalidateCache();
}

QIcon QContactSimContext::icon() const
{
    static QIcon simicon(":icon/sim-contact");
    return simicon;
}

QString QContactSimContext::description() const
{
    return tr("SIM Card Contact Access");
}

QString QContactSimContext::title() const
{
    return tr("SIM Card Contact Access");
}

bool QContactSimContext::editable() const
{
    return mSync->done();
}

bool QContactSimContext::editable(const QUniqueId &id) const
{
    return (editable() && cardIndex(id) != -1 && source(id) == defaultSource());
}

QPimSource QContactSimContext::defaultSource() const
{
    QPimSource s;
    s.context = id();
    s.identity = "sim"; // compat with old code
    return s;
}

QPimSource QContactSimContext::serviceNumbersSource() const
{
    QPimSource s;
    s.context = id();
    s.identity = serviceNumbers;
    return s;
}

QSet<QPimSource> QContactSimContext::sources() const
{
    QSet<QPimSource> set;
    set.insert(defaultSource());
    set.insert(serviceNumbersSource());
    return set;
}

QUuid QContactSimContext::id() const
{
    static QUuid u("b63abe6f-36bd-4bb8-9c27-ece5436a5130");
    return u;
}

QString QContactSimContext::title(const QPimSource &source) const
{
    if (source == defaultSource())
        return tr("Active SIM Card");
    if (source == serviceNumbersSource())
        return tr("SIM Card Service Numbers");
    return QString();
}

/* TODO set mapping to int */
void QContactSimContext::setVisibleSources(const QSet<QPimSource> &set)
{
    QSet<int> show;
    QSet<int> hide;
    QSet<QPimSource> list = sources();
    foreach (QPimSource s, list) {
        int context = QPimSqlIO::sourceContext(s);
        if (set.contains(s)) {
            show.insert(context);
        } else {
            hide.insert(context);
        }
    }

    QSet<int> filter = mAccess->contextFilter();
    filter.unite(hide);
    filter.subtract(show);
    mAccess->setContextFilter(filter);
}

QSet<QPimSource> QContactSimContext::visibleSources() const
{
    QSet<int> filter = mAccess->contextFilter();
    QSet<QPimSource> result;

    QSet<QPimSource> list = sources();
    foreach (QPimSource s, list) {
        int context = QPimSqlIO::sourceContext(s);
        if (!filter.contains(context))
            result.insert(s);
    }
    return result;
}

bool QContactSimContext::exists(const QUniqueId &id) const
{
    return !source(id).isNull();
}

QPimSource QContactSimContext::source(const QUniqueId &id) const
{
    int itemContext = mAccess->context(id);
    QSet<QPimSource> list = sources();
    foreach(QPimSource s, list) {
        int context = QPimSqlIO::sourceContext(s);
        if (context == itemContext)
            return s;
    }
    return QPimSource();
}

bool QContactSimContext::updateContact(const QContact &contact)
{
    if (editable(contact.uid())) {
        QString label = contact.firstName();
        QString number = contact.homePhone();
        if (label.length() > mSync->labelLimit() || number.length() > mSync->numberLimit())
            return false;
        if (mAccess->updateContact(contact)) {
            int i = cardIndex(contact.uid());
            QPhoneBookEntry entry;
            entry.setIndex(i);
            entry.setText(label);
            entry.setNumber(number);
            mPhoneBook->update(entry);
            return true;
        }
    }
    return false;
}

bool QContactSimContext::removeContact(const QUniqueId &id)
{
    if (editable(id)) {
        if (mAccess->removeContact(id)) {
            int i = cardIndex(id);
            mPhoneBook->remove(i);
            mPhoneBook->flush();
        }
    }
    return false;
}

QUniqueId QContactSimContext::addContact(const QContact &contact, const QPimSource &source)
{
    if (source != defaultSource() || !editable())
        return QUniqueId();

    if (contact.firstName().length() > mSync->labelLimit() ||
            contact.homePhone().length() > mSync->numberLimit())
        return QUniqueId();

    QContact c;
    c.setFirstName(contact.firstName());
    c.setHomePhone(contact.homePhone());

    int i = nextFreeIndex();
    QUniqueId u = mSync->id(mSync->card(), i);
    c.setUid(u);
    // round trip via sim access?
    if (!mAccess->addContact(c, source, false).isNull()) {
        QPhoneBookEntry entry;
        entry.setIndex( i );
        entry.setNumber( c.homePhone() );
        entry.setText( c.firstName() );
        mPhoneBook->add(entry);
        return u;
    }
    return QUniqueId();
}

bool QContactSimContext::waitingOnSim() const
{
    return false;
}

QString QContactSimContext::card(const QUniqueId &u) const
{
    QPreparedSqlQuery q(QPimSqlIO::database());
    q.prepare("SELECT cardid FROM simcardidmap WHERE sqlid = :s");
    q.bindValue(":s", u.toUInt());
    q.exec();
    if (q.next())
        return q.value(0).toString();
    return QString();
}

int QContactSimContext::cardIndex(const QUniqueId &u) const
{
    QPreparedSqlQuery q(QPimSqlIO::database());
    q.prepare("SELECT cardindex FROM simcardidmap WHERE sqlid = :s");
    q.bindValue(":s", u.toUInt());
    q.exec();
    if (q.next())
        return q.value(0).toInt();
    return -1;
}

QUniqueId QContactSimSyncer::id(const QString &card, int index) const
{
    if (!selectIdQuery.prepare())
        return QUniqueId();

    selectIdQuery.bindValue(":c", card);
    selectIdQuery.bindValue(":i", index);
    selectIdQuery.exec();

    if (selectIdQuery.next()) {
        QUniqueId result = QUniqueId::fromUInt(selectIdQuery.value(0).toUInt());
        selectIdQuery.reset();
        return result;
    }
    selectIdQuery.reset();

    if (!insertIdQuery.prepare())
        return QUniqueId();

    static QUuid appScope("b63abe6f-36bd-4bb8-9c27-ece5436a5130");
    QUniqueIdGenerator g(appScope); // later, same scop method as xml
    QUniqueId u = g.createUniqueId();

    insertIdQuery.bindValue(":s", u.toUInt());
    insertIdQuery.bindValue(":c", card);
    insertIdQuery.bindValue(":i", index);

    insertIdQuery.exec();
    insertIdQuery.reset();
    return u;
}

int QContactSimContext::nextFreeIndex() const
{
    QPreparedSqlQuery q(QPimSqlIO::database());
    q.prepare("SELECT cardindex FROM simcardidmap WHERE cardid = :c ORDER BY cardindex");
    q.bindValue(":c", mSync->card());
    q.exec();

    int index = mSync->firstIndex();
    while (q.next()) {
        int pos = q.value(0).toInt();
        if (pos != index)
            break;
        index ++;
    }
    if (index > mSync->lastIndex())
        return -1;
    return index;
}

bool QContactSimContext::isSIMContactCompatible(const QContact &c) const
{
    if ( !c.homePhone().isEmpty() ||
         !c.homeMobile().isEmpty() ||
         !c.homeFax().isEmpty() ||
         !c.businessPhone().isEmpty() ||
         !c.businessMobile().isEmpty() ||
         !c.businessFax().isEmpty() ||
         !c.businessPager().isEmpty() )
        return true;
    return false;
}

QString QContactSimContext::typeToSIMExtension(QContactModel::Field type)
{
    const ExtensionMap *i = SIMextensions;
    while(i->type != QContactModel::Invalid) {
        if (i->type == type)
            return i->text;
        ++i;
    }
    return QString();
}

QContactModel::Field QContactSimContext::SIMExtensionToType(QString &label)
{
    // doesn't guess at ones we don't write.
    const ExtensionMap *i = SIMextensions;
    QString llabel = label.toLower();
    while(i->type != QContactModel::Invalid) {
        QString e(i->text);
        if (llabel.right(e.length()) == e) {
            label = label.left(label.length() - e.length());
            return i->type;
        }
        ++i;
    }

    // failed of the Qtopia map, try some regexp.
    const RegExpExtensionMap *r = SIMRegExpExtensions;
    while(r->type != QContactModel::Invalid) {
        if (r->expression.indexIn(llabel) != -1) {
            label = label.left(label.length() - r->expression.matchedLength());
            return r->type;
        }
        ++r;
    }

    return QContactModel::Invalid;
}

QString QContactSimContext::createSIMLabel(const QContact &c)
{
    QString label = c.firstName();
    if (label.isEmpty())
        label = c.label(); // should ask for 'short' label.
    return label;
}

QContactSimSyncer::QContactSimSyncer(ContactSqlIO *access, const QString &cardType, const QPimSource &source, QObject *parent)
    : QObject(parent), mAccess(access), readState(0), mSimType(cardType), mSource(source),
    SIMLabelLimit(20), SIMNumberLimit(60),
    SIMListStart(1), SIMListEnd(200), initialReadComplete(false),
    addNameQuery("INSERT INTO contacts (recid, firstname, context) VALUES (:i, :fn, :c)"),
    addNumberQuery("INSERT INTO contactphonenumbers (recid, phone_type, phone_number) VALUES (:i, 1, :pn)"),
    updateNameQuery("UPDATE contacts SET firstname = :fn WHERE recid = :i"),
    updateNumberQuery("UPDATE contactphonenumbers SET phone_number = :pn WHERE recid = :i AND phone_type = 1"),
    removeNameQuery("DELETE FROM contacts WHERE recid = :i"),
    removeNumberQuery("DELETE FROM contactphonenumbers WHERE recid = :i"),
    selectNameQuery("SELECT firstname FROM contacts WHERE recid = :i"),
    selectNumberQuery("SELECT phone_number from contactphonenumbers where recid=:id and phone_type=1"),
    selectIdQuery("SELECT sqlid FROM simcardidmap WHERE cardid = :c AND cardindex = :i"),
    insertIdQuery("INSERT INTO simcardidmap (sqlid, cardid, cardindex) VALUES (:s, :c, :i)")
{
}

void QContactSimContext::readSimIdentity()
{
    emit simIdentityUpdated(mSimInfo->identity(), mSimInfo->insertedTime());
}

void QContactSimSyncer::simIdentityChanged(const QString &value, const QDateTime &inserted)
{
    qLog(PIM) << "Notified of sim identity change" << value;
    mActiveCard = value;

    mInsertTime = inserted;

    readState |= PhoneBookIdRead;

    if (readState == PhoneBookRead)
        updateSqlEntries();
}

void QContactSimSyncer::updatePhoneBook( const QString &store, const QList<QPhoneBookEntry> &list )
{
    qLog(PIM) << "Notified of sim phonebook contents change" << list.count();
    if (store != mSimType)
        return;

    readState |= PhoneBookEntriesRead;

    phoneData = list;

    if (readState == PhoneBookRead)
        updateSqlEntries();
}

void QContactSimSyncer::updatePhoneBookLimits( const QString &store, const QPhoneBookLimits &value )
{
    qLog(PIM) << "Notified of sim phonebook limits change";
    if (store != mSimType)
        return;

    SIMNumberLimit = value.numberLength();
    SIMLabelLimit = value.textLength();
    SIMListStart = value.firstIndex();
    SIMListEnd = value.lastIndex();

    readState |= PhoneBookLimitsRead;

    if (readState == PhoneBookRead)
        updateSqlEntries();
}

void QContactSimSyncer::updateSqlEntries()
{
    qLog(PIM) << "Sync sim phonebook contents to SQL database";
    int context = QPimSqlIO::sourceContext(mSource);

    QDateTime syncTime;
    // we want to work in utc, siminfo works in local.  convert.
    if (mActiveCard.isEmpty())
        syncTime = QTimeZone::current().toUtc(QDateTime::currentDateTime());
    else
        syncTime = QTimeZone::current().toUtc(mInsertTime);

    // drop msecs from time.  We don't store it when storing a last sync time.
    syncTime = syncTime.addMSecs(-syncTime.time().msec());

    QDateTime lastSync = QPimSqlIO::lastSyncTime(mSource);

    if (lastSync.isValid() && lastSync >= syncTime) {
        if (!initialReadComplete) {
            initialReadComplete = true;
            emit simCardContactsUpdated(context); // since editable() possibly still changed
        }
        return;
    }

    if (mAccess->startSync(mSource, syncTime)) {
        QMap<QUniqueId, QContact> existing;
        QList<QContact> updated;
        QList<QContact> added;

        QPreparedSqlQuery q(QPimSqlIO::database());

        q.prepare("SELECT contacts.recid, firstname, phone_number FROM contacts JOIN contactphonenumbers ON contacts.recid = contactphonenumbers.recid where context = :c AND phone_type=1");

        q.bindValue(":c", context);
        q.exec();
        while(q.next()) {
            QContact c;
            c.setUid(QUniqueId::fromUInt(q.value(0).toUInt()));
            c.setFirstName(q.value(1).toString());
            c.setHomePhone(q.value(2).toString());
            existing.insert(c.uid(), c);
        }
        q.clear();

        foreach(QPhoneBookEntry entry, phoneData) {
            QContact c;

            c.setUid(id(mActiveCard, entry.index()));
            c.setFirstName(entry.text());
            c.setHomePhone(entry.number());
            c.setCustomField("sim_index", QString::number(entry.index()));

            QUniqueId u = c.uid();
            if (existing.contains(u)) {
                // only update if different.;
                QContact e = existing.value(u);
                existing.remove(u);
                if (e.firstName() != c.firstName() || e.homePhone() != c.homePhone())
                    updated.append(c);
            } else {
                added.append(c);
            }
        }

        QList<QUniqueId> removed = existing.uniqueKeys();
        foreach(QContact c, updated) {
            if (!mAccess->updateContact(c)) {
                mAccess->abortSync();
                return;
            }
        }
        foreach(QContact c, added) {
            if (mAccess->addContact(c, mSource, false).isNull()) {
                mAccess->abortSync();
                return;
            }
        }
        // covers cards not from this sim, will have different id's.
        foreach(QUniqueId u, removed) {
            if (!mAccess->removeContact(u)) {
                mAccess->abortSync();
                return;
            }
        }

        if (!mAccess->commitSync()) {
            mAccess->abortSync();
            return;
        }

        if (!initialReadComplete) {
            initialReadComplete = true;
            emit simCardContactsUpdated(context);
        }
    }
}

// Get only first name and home phone fields.
QContact QContactSimSyncer::simContact(const QUniqueId &u) const
{
    QContact t;
    if (u.isNull())
        return t;

    selectNameQuery.prepare();
    selectNumberQuery.prepare();
    if (!selectNameQuery.isValid() || !selectNumberQuery.isValid())
        return t;

    selectNameQuery.bindValue(":i", u.toUInt());

    if (!selectNameQuery.exec()) {
        return t;
    }

    if ( selectNameQuery.next() ) {
        selectNumberQuery.bindValue(":id", u.toUInt());
        selectNumberQuery.exec();
        if(selectNumberQuery.next()) {
            QString number;
            number = selectNumberQuery.value(0).toString();
            t.setPhoneNumber(QContact::HomePhone, number);
        }

        t.setUid(u);
        t.setFirstName(selectNameQuery.value(0).toString());
        // Don't bother caching this entry - no point caching enties that
        // won't be displayed.
    }
    selectNameQuery.reset();
    selectNumberQuery.reset();

    return t;
}

QUniqueId QContactSimContext::findLabel(const QString &test) const
{
    int context = QPimSqlIO::sourceContext(defaultSource());
    QPreparedSqlQuery q(QPimSqlIO::database());
    bool prepared = q.prepare("SELECT recid FROM contacts WHERE context = :c AND firstname = :fn");
    Q_ASSERT(prepared);
    prepared = prepared;
    q.bindValue(":c", context);
    q.bindValue(":fn", test);
    Q_ASSERT(q.exec());
    if (q.next())
        return QUniqueId::fromUInt(q.value(0).toUInt());
    return QUniqueId();
}

bool QContactSimContext::importContacts(const QPimSource &s, const QList<QContact> &list)
{
    if (!editable() || s != defaultSource())
        return false;

    // create io for exported context
    int context = QPimSqlIO::sourceContext(s);
    ContactSqlIO *exportAccess = new ContactSqlIO(0);
    QSet<int> set;
    set.insert(context);
    exportAccess->setContextFilter(set, QPimSqlIO::RestrictToContexts);
    int count = exportAccess->count();


    foreach(QContact contact, list) {
        // check isn't already a special contact
        QMap<QString, QString> newContacts;
        QList<QContact> updateContacts;

        bool untypedLabelUpdated = false;
        // separate out phone numbers, and add one contact per phone number.
        // if exists, will update, not add.
        QList<QContactModel::Field> pFields = QContactModel::phoneFields();
        foreach(QContactModel::Field f, pFields) {
            QString v = QContactModel::contactField(contact, f).toString().simplified();
            if (!v.isEmpty()) {

                // You just can't store a too long number (invalid to truncate it)
                if (v.length() > mSync->numberLimit())
                    return false;

                QString label;
                QUniqueId oldpos;
                if (contact.phoneNumbers().count() == 1) {
                    label = contact.label().left(mSync->labelLimit()-3);
                    oldpos = findLabel(label);
                } else {
                    label = contact.label().left(mSync->labelLimit()-3) + typeToSIMExtension(f);
                    oldpos = findLabel(label);
                    if (oldpos.isNull() && !untypedLabelUpdated) {
                        oldpos = findLabel(contact.label().left(mSync->labelLimit()-3));
                        if (!oldpos.isNull())
                            untypedLabelUpdated = true;
                    }
                }
                if (!oldpos.isNull()) {
                    QContact c = mAccess->contact(oldpos);
                    c.setFirstName(label);
                    c.setHomePhone(v);
                    updateContacts.append(c);
                } else {
                    newContacts.insert(label, v);
                }
                // check if update or add.
            }
        }

        int avail = mSync->lastIndex() - mSync->firstIndex() - count + 1;

        if (newContacts.count() > avail)
            return false;

        // assert, possible to do import.
        QMapIterator<QString, QString> ait(newContacts);
        while(ait.hasNext()) {
            ait.next();
            QContact c;
            c.setFirstName(ait.key());
            c.setHomePhone(ait.value());
            addContact(c, defaultSource());
        }
        foreach(QContact entry, updateContacts) {
            updateContact(entry);
        }
    }
    return true;
}

QContact QContactSimContext::exportContact(const QUniqueId &id, bool &ok) const
{
    QContact c;
    if (!exists(id)) {
        ok = false;
        return c;
    }

    c =  mAccess->contact(id);
    // how contacts are norally contructed.
    QString label = c.firstName();
    QString number = c.homePhone();

    QContactModel::Field key = SIMExtensionToType(label);
    if (key == QContactModel::Invalid)
        key = QContactModel::HomeMobile;

    c = QContact::parseLabel(label);

    QContactModel::setContactField(c, key, number);
    ok = true;
    return c;
}

QList<QContact> QContactSimContext::exportContacts(const QPimSource &s, bool &ok) const
{
    ok = true;
    // create io for exported context
    int context = QPimSqlIO::sourceContext(s);
    ContactSqlIO *exportAccess = new ContactSqlIO(0);
    QSet<int> set;
    set.insert(context);
    exportAccess->setContextFilter(set, QPimSqlIO::RestrictToContexts);

    QMap<QString, QContact> result;
    QList<QContactModel::Field> pfields = QContactModel::phoneFields();

    for (int i = 0; i < exportAccess->count(); i++) {
        QUniqueId u = exportAccess->id(i);
        QContact tmpcontact = mSync->simContact(u);
        QString label = tmpcontact.firstName();
        QString number = tmpcontact.homePhone();

        QContactModel::Field key = SIMExtensionToType(label);
        if (key == QContactModel::Invalid)
            key = QContactModel::HomeMobile;

        QContact c;

        if (result.contains(label))
            c = result[label];
        else
            c = QContact::parseLabel(label);

        if (!QContactModel::contactField(c, key).toString().isEmpty()) {
            // insert into first available phone number field.
            foreach(QContactModel::Field f, pfields) {
                if (QContactModel::contactField(c, f).toString().isEmpty()) {
                    QContactModel::setContactField(c, f, number);
                    break;
                }
            }
        } else {
            QContactModel::setContactField(c, key, number);
        }
        result.insert(label, c);
    }

    return result.values();
}

#include "qsimcontext.moc"
