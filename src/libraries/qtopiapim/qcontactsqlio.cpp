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

#include <qtopia/pim/qcontact.h>
#include <qtopia/pim/qcontactmodel.h>

#include <qtopialog.h>
#include <qtopia/pim/qphonenumber.h>

#include <qtopiaipcenvelope.h>
#include "qcontactsqlio_p.h"
#include "qannotator_p.h"
#ifdef Q_OS_WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

// both... until I can get all the required widgets sorted out.
#include <qcategorymanager.h>
#include "qsqlpimtablemodel_p.h"

#include <QSettings>
#include <QString>
#include <QSqlError>
#include <QTimer>
#include <QDebug>
#include <QSqlField>
#include <QSqlDriver>
#include <QTcpSocket>

// For contact birthday/anniversary feature:
#include "qappointment.h"
#include "qappointmentsqlio_p.h"
#include "qdependentcontexts_p.h"
#include "qpimdependencylist_p.h"

static const char *contextsource = "default";

/*
   data fields

title firstname middlename lastname suffix default_phone default_email jobtitle department company b_webpage office profession assistant manager h_webpage spouse gender birthday anniversary nickname children portrait lastname_pronunciation firstname_pronunciation company_pronunciation

data bindings
:t, :fn, :mn, :ln, :su, :dp, :de, :jt, :d, :c, :bphone, :bfax, :bmobile, :bstreet, :bcity, :bstate, :bzip, :bcountry, :bpager, :bwebpage, :o, :profession, :assistant, :manager, :hphone, :hfax, :hmobile, :hstreet, :hcity, :hstate, :hzip, :hcountry, :hwebpage, :sp, :g, :b, :a, :nickname, :childern, :portrait, :lnp, :fnp, :cp
*/


// XXX because of the large number of fields.... will _HAVE_ to implement 'sub field retrival' as
// time taken to check and set that many strings will probably be too much.

class ContactSimpleQueryCache : public QPimQueryCache 
{
public:
    struct ContactRow
    {
        ContactRow(const QVariant &a, const QVariant &b, const QVariant &c,
                const QVariant &d, const QVariant &e, const QVariant &f,
                const QVariant &g, const QVariant &h, const QVariant &i,
                const QVariant &j)
            : recid(a), nameTitle(b), firstName(c), middleName(d),
                    lastName(e), suffix(f), phoneNumber(g), email(h),
                    company(i), portraitFile(j)
        {}

        QVariant recid;
        QVariant nameTitle;
        QVariant firstName;
        QVariant middleName;
        QVariant lastName;
        QVariant suffix;
        QVariant phoneNumber;
        QVariant email;
        QVariant company;
        QVariant portraitFile;
    };

    ContactSimpleQueryCache()
        : QPimQueryCache(), cache(25)
    {
    }

    QString fields() const
    {
        static const QString result("t1.title, t1.firstname, t1.middlename, t1.lastname, t1.suffix, t1.default_phone, t1.default_email, t1.company, t1.portrait");
        return result;

        //(select categoryid from contactcategories where contactcategories.recid = contacts.recid and categoryid = 'Business')
    }

    void cacheRow(int row, const QPreparedSqlQuery &q)
    {
        cache.insert(row, new ContactRow(q.value(0), q.value(2), q.value(3), q.value(4), q.value(5), q.value(6), q.value(7), q.value(8), q.value(9), q.value(10))); 
    }

    void setMaxCost(int m) { cache.setMaxCost(m); contactCache.setMaxCost(m); }
    void clear() { cache.clear(); contactCache.clear(); }

    QCache<int, ContactRow> cache;
    QCache<int, QContact> contactCache;

};

// may be better to over ride order text.
QStringList ContactSqlIO::sortColumns() const
{
    QList<QContactModel::Field> mOrder = formatFieldOrder();
    QStringList sc;
    foreach(QContactModel::Field f, mOrder) {
        sc << sqlColumn(f);
    }
    return sc;
}

QVariant ContactSqlIO::key(int row) const
{
    QList<QContactModel::Field> mOrder = formatFieldOrder();
    QStringList sc;
    foreach(QContactModel::Field f, mOrder) {
        sc << contactField(row, f).toString();
    }
    return sc;
}

QVariant ContactSqlIO::key(const QUniqueId &id) const
{
    QStringList keys = sortColumns();
    QString keystring = keys.join(", ");
    QPreparedSqlQuery q(database());
    q.prepare("SELECT " + keystring + " FROM contacts"
            " WHERE recid = :id");

    q.bindValue(":id", id.toUInt());

    if (q.exec() && q.next()) {
        QStringList sl;
        for (int i = 0; i < keys.count(); i++)
            sl << q.value(i).toString();
        return sl;
    }
    return QVariant();
}

QString ContactSqlIO::sqlColumn(QContactModel::Field k) const {
    switch(k) {
        default:
            return QString();
        case QContactModel::Label:
            return sqlLabelCache;
        case QContactModel::NameTitle:
            return "title";
        case QContactModel::FirstName:
            return "firstname";
        case QContactModel::MiddleName:
            return "middlename";
        case QContactModel::LastName:
            return "lastname";
        case QContactModel::Suffix:
            return "suffix";

        case QContactModel::JobTitle:
            return "jobtitle";
        case QContactModel::Department:
            return "department";
        case QContactModel::Company:
            return "company";

        case QContactModel::DefaultPhone:
            return "default_phone";

        // email
        case QContactModel::DefaultEmail:
        case QContactModel::Emails:
            return "default_email";

        // business
        case QContactModel::BusinessWebPage:
            return "b_webpage";

        case QContactModel::Office:
            return "office";
        case QContactModel::Profession:
            return "profession";
        case QContactModel::Assistant:
            return "assistant";
        case QContactModel::Manager:
            return "manager";

        // home
        case QContactModel::HomeWebPage:
            return "h_webpage";

        //personal
        case QContactModel::Spouse:
            return "spouse";
        case QContactModel::Gender:
            return "gender";
        case QContactModel::Birthday:
            return "birthday";
        case QContactModel::Anniversary:
            return "anniversary";
        case QContactModel::Nickname:
            return "nickname";
        case QContactModel::Children:
            return "children";

        // other
        case QContactModel::LastNamePronunciation:
            return "lastname_pronunciation";
        case QContactModel::FirstNamePronunciation:
            return "firstname_pronunciation";
        case QContactModel::CompanyPronunciation:
            return "company_pronunciation";

    }
    return QString();
}

QString ContactSqlIO::sqlLabel() const
{
    int fc = formatCount();
    QString expression = "(CASE ";
    for (int i = 0; i < fc; i++) {
        QList<QVariant> f = format(i);
        expression += "WHEN ";
        bool firstkey = true;
        QListIterator<QVariant> it(f);
        while(it.hasNext()) {
            QVariant v = it.next();
            if (v.type() == QVariant::String)
                continue;
            if (!firstkey)
                expression += "AND ";
            firstkey = false;
            QContactModel::Field k = (QContactModel::Field)v.toInt();
            if (k == QContactModel::Invalid || k == QContactModel::Label)
                return sqlColumn(QContactModel::FirstName); // soft fail.
            expression += sqlColumn(k) + " IS NOT NULL ";
        }
        expression += "THEN ";
        QListIterator<QVariant> fit(f);
        while(fit.hasNext()) {
            QVariant v = fit.next();
            if (v.type() == QVariant::Int) {
                QContactModel::Field k = (QContactModel::Field)v.toInt();
                if (k == QContactModel::Invalid || k == QContactModel::Label)
                    return sqlColumn(QContactModel::FirstName); // soft fail.
                expression += sqlColumn(k) + " ";
            } else if (v.type() == QVariant::String) {
                expression += "\"" + v.toString() + "\" ";
            }
            if (fit.hasNext())
                expression += "|| ";
        }
    }
    expression += "ELSE NULL END)";
    return expression;
}

ContactSqlIO::ContactSqlIO(QObject *parent, const QString &)
    : QContactIO(parent),
    QPimSqlIO( contextId(), "contacts", "contactcategories", "contactcustom",

            "title = :t, firstname = :fn, middlename = :mn, lastname = :ln, suffix = :su, "
            "default_phone = :dp, default_email = :de, jobtitle = :jt, department = :d, company = :c, "
            "b_webpage = :bwebpage, office = :o, profession = :profession, "
            "assistant = :assistant, manager = :manager, "
            "h_webpage = :hwebpage, spouse = :sp, "
            "gender = :g, birthday = :b, anniversary = :a, nickname = :nickname, "
            "children = :childern, portrait = :portrait, "
            "lastname_pronunciation = :lnp, firstname_pronunciation = :fnp, "
            "company_pronunciation = :cp",

            "(recid, context, title, firstname, middlename, lastname, suffix, default_phone, default_email, "
            "jobtitle, department, company, "
            "b_webpage, office, profession, assistant, manager, "
            "h_webpage, spouse, gender, birthday, anniversary, nickname, children, portrait, "
            "lastname_pronunciation, firstname_pronunciation, "
            "company_pronunciation)"
            " VALUES "
            "(:i, :context, :t, :fn, :mn, :ln, :su, :dp, :de, :jt, :d, :c, "
            ":bwebpage, :o, "
            ":profession, :assistant, :manager, "
            ":hwebpage, :sp, :g, :b, :a, :nickname, :childern, "
            ":portrait, :lnp, :fnp, :cp)"),

            orderKey(QContactModel::Label), contactByRowValid(false),
            tmptable(false),
            contactQuery("SELECT recid, title, firstname, " // 3
                    "middlename, lastname, suffix, " // 7
                    "default_phone, default_email, jobtitle, department, company, " // 11
                    "b_webpage, office, profession, assistant, manager, " // 16
                    "h_webpage, spouse, gender, birthday, anniversary, " // 22
                    "nickname, children, portrait, lastname_pronunciation, " // 26
                    "firstname_pronunciation, company_pronunciation " // 29
                    "FROM contacts WHERE recid = :i"),
            emailsQuery("SELECT addr from emailaddresses where recid=:id"),
            addressesQuery("SELECT addresstype, street, city, state, zip, country from contactaddresses where recid=:id"),
            phoneQuery("SELECT phone_type, phone_number from contactphonenumbers where recid=:id"),
            insertEmailsQuery("INSERT INTO emailaddresses (recid, addr) VALUES (:i, :a)"),
            insertAddressesQuery("INSERT INTO contactaddresses (recid, addresstype, street, city, state, zip, country) VALUES (:i, :t, :s, :c, :st, :z, :co)"),
            insertPhoneQuery("INSERT INTO contactphonenumbers (recid, phone_type, phone_number) VALUES (:i, :t, :ph)"),
            removeEmailsQuery("DELETE from emailaddresses WHERE recid = :i"),
            removeAddressesQuery("DELETE from contactaddresses WHERE recid = :i"),
            removePhoneQuery("DELETE from contactphonenumbers WHERE recid = :i")
{
    simpleCache = new ContactSimpleQueryCache;
    setSimpleQueryCache(simpleCache);

    QPimSqlIO::setOrderBy(sortColumns());

    sqlLabelCache = sqlLabel();
    initMaps();

    connect(this, SIGNAL(labelFormatChanged()), this, SLOT(updateSqlLabel()));
}

ContactSqlIO::~ContactSqlIO()
{
}

QUuid ContactSqlIO::contextId() const
{
    // generated with uuidgen
    static QUuid u("a7a2832c-cdb3-40b6-9d95-6cd31e05647d");
    return u;
}
void ContactSqlIO::bindFields(const QPimRecord& r, QPreparedSqlQuery &q) const
{
    const QContact &t = (const QContact &)r;
    q.bindValue(":t", t.nameTitle());
    q.bindValue(":fn", t.firstName());
    q.bindValue(":mn", t.middleName());
    q.bindValue(":ln", t.lastName());
    q.bindValue(":su", t.suffix());
    q.bindValue(":dp", t.defaultPhoneNumber());
    q.bindValue(":de", t.defaultEmail());
    q.bindValue(":jt", t.jobTitle());
    q.bindValue(":d", t.department());
    q.bindValue(":c", t.company());

    q.bindValue(":bwebpage", t.businessWebpage());
    q.bindValue(":o", t.office());
    q.bindValue(":profession", t.profession());
    q.bindValue(":assistant", t.assistant());
    q.bindValue(":manager", t.manager());

    q.bindValue(":hwebpage", t.homeWebpage());
    q.bindValue(":sp", t.spouse());
    q.bindValue(":g", t.gender());
    q.bindValue(":b", t.birthday());
    q.bindValue(":a", t.anniversary());
    q.bindValue(":nickname", t.nickname());
    q.bindValue(":childern", t.children());
    q.bindValue(":portrait", t.portraitFile());
    q.bindValue(":lnp", t.lastNamePronunciation());
    q.bindValue(":fnp", t.firstNamePronunciation());
    q.bindValue(":cp", t.companyPronunciation());
}

// by uid doesn't neeed caching... always fast and unlikely to be in order?
QContact ContactSqlIO::contact( const QUniqueId & u ) const
{
    if (u.isNull())
        return QContact();
    // Getting a whole contact can be expensive.  Cache if able.
    // We're very likely to require the previous contact again.
    if (contactByRowValid && u == lastContact.uid())
        return lastContact;
    // Maybe it is in our cache?

    if (!contactQuery.prepare())
        return QContact();

    contactQuery.bindValue(":i", u.toUInt());

    QContact t;

    // get common parts
    retrieveRecord(u.toUInt(), t);

    if (!contactQuery.exec()) {
        contactByRowValid = false;
        return t;
    }

    if ( contactQuery.next() ) {
        // XXX should check uid against u.
        QString defaultPhone;

        t.setUid(QUniqueId::fromUInt(contactQuery.value(0).toUInt()));
        t.setNameTitle(contactQuery.value(1).toString());
        t.setFirstName(contactQuery.value(2).toString());
        t.setMiddleName(contactQuery.value(3).toString());
        t.setLastName(contactQuery.value(4).toString());
        t.setSuffix(contactQuery.value(5).toString());

        defaultPhone = contactQuery.value(6).toString();

        t.setDefaultEmail(contactQuery.value(7).toString());
        t.setJobTitle(contactQuery.value(8).toString());
        t.setDepartment(contactQuery.value(9).toString());
        t.setCompany(contactQuery.value(10).toString());
        t.setBusinessWebpage(contactQuery.value(11).toString());
        t.setOffice(contactQuery.value(12).toString());
        t.setProfession(contactQuery.value(13).toString());
        t.setAssistant(contactQuery.value(14).toString());
        t.setManager(contactQuery.value(15).toString());
        t.setHomeWebpage(contactQuery.value(16).toString());
        t.setSpouse(contactQuery.value(17).toString());
        t.setGender((QContact::GenderType)contactQuery.value(18).toInt());
        t.setBirthday(contactQuery.value(19).toDate());
        t.setAnniversary(contactQuery.value(20).toDate());
        t.setNickname(contactQuery.value(21).toString());
        t.setChildren(contactQuery.value(22).toString());
        t.setPortraitFile(contactQuery.value(23).toString());
        t.setLastNamePronunciation(contactQuery.value(24).toString());
        t.setFirstNamePronunciation(contactQuery.value(25).toString());
        t.setCompanyPronunciation(contactQuery.value(26).toString());

        contactQuery.reset();

        // categories for this contact;
        if (emailsQuery.prepare()) {
            emailsQuery.bindValue(":id", u.toUInt());
            emailsQuery.exec();
            QStringList tlist;
            while(emailsQuery.next())
                tlist.append(emailsQuery.value(0).toString());
            t.setEmailList(tlist);
            emailsQuery.reset();
        }

        // and contact addresses
        if (addressesQuery.prepare()) {
            addressesQuery.bindValue(":id", u.toUInt());
            addressesQuery.exec();
            QStringList tlist;
            while(addressesQuery.next()) {
                QContactAddress a;
                QContact::Location l;
                l = (QContact::Location)addressesQuery.value(0).toInt();
                a.street = addressesQuery.value(1).toString();
                a.city = addressesQuery.value(2).toString();
                a.state = addressesQuery.value(3).toString();
                a.zip = addressesQuery.value(4).toString();
                a.country = addressesQuery.value(5).toString();
                t.setAddress(l, a);
            }
            addressesQuery.reset();
        }

        // and contact addresses
        if (phoneQuery.prepare()) {
            phoneQuery.bindValue(":id", u.toUInt());
            phoneQuery.exec();
            QString tlist;
            while(phoneQuery.next()) {
                QString number;
                QContact::PhoneType type;
                type = (QContact::PhoneType)phoneQuery.value(0).toInt();
                number = phoneQuery.value(1).toString();
                t.setPhoneNumber(type, number);
            }
            phoneQuery.reset();
        }

        if (!defaultPhone.isEmpty())
            t.setDefaultPhoneNumber(defaultPhone);

        lastContact = t;
        contactByRowValid = true;
    } else {
        contactQuery.reset();
        contactByRowValid = false;
    }

    return t;
}

void ContactSqlIO::setSortKey(QContactModel::Field s)
{
    if (orderKey != s) {
        orderKey = s;
        invalidateCache();
    }
}

void ContactSqlIO::setCategoryFilter(const QCategoryFilter &f)
{
    if (f != categoryFilter()) {
        QPimSqlIO::setCategoryFilter(f);
        emit filtersUpdated();
    }
}

void ContactSqlIO::setContextFilter(const QSet<int> &list, ContextFilterType type)
{
    if (list != contextFilter() || type != contextFilterType()) {
        QPimSqlIO::setContextFilter(list, type);
        invalidateCache();
    }
}

QContactModel::Field ContactSqlIO::sortKey() const
{
    return orderKey;
}

void ContactSqlIO::invalidateCache()
{
    QPimSqlIO::invalidateCache();
    contactByRowValid = false;
    mLocalNumberCache.clear();
    emit filtersUpdated();
}

// if filtering/sorting/contacts doesn't change.
QContact ContactSqlIO::contact(int row) const
{
    return contact(recordId(row));
}

void ContactSqlIO::updateSqlLabel()
{
    sqlLabelCache = sqlLabel();
    setOrderBy(sortColumns());
    invalidateCache();
}

// assumes storage is row based.
// override if more efficient to grap partial contact.
QVariant ContactSqlIO::contactField(int row, QContactModel::Field k) const
{
    if (!simpleCache->cache.contains(row))
        recordId(row);

    ContactSimpleQueryCache::ContactRow *cr = simpleCache->cache.object(row);
    if (cr) {
        switch(k) {
            case QContactModel::Identifier:
                return QUniqueId::fromUInt(cr->recid.toUInt()).toByteArray();
            case QContactModel::NameTitle:
                return cr->nameTitle;
            case QContactModel::FirstName:
                return cr->firstName;
            case QContactModel::MiddleName:
                return cr->middleName;
            case QContactModel::LastName:
                return cr->lastName;
            case QContactModel::Suffix:
                return cr->suffix;
            case QContactModel::Company:
                return cr->company;
            case QContactModel::DefaultEmail:
                return cr->email;
            case QContactModel::DefaultPhone:
                return cr->phoneNumber;
            case QContactModel::Portrait:
                return cr->portraitFile;
            case QContactModel::Label:
                {
                    QContact c = simpleContact(row);
                    return c.label();
                }
            default:
                break;
        }
    }

    return QContactIO::contactField(row, k);
}

QContact ContactSqlIO::simpleContact(int row) const
{
    if (simpleCache->contactCache.contains(row))
        return *(simpleCache->contactCache.object(row));

    if (!simpleCache->cache.contains(row))
        recordId(row);
    ContactSimpleQueryCache::ContactRow *cr = simpleCache->cache.object(row);
    if (cr) {
#ifdef GREENPHONE_EFFECTS
        bool isBus = false;
#else
        QPreparedSqlQuery q(QPimSqlIO::database());
        q.prepare("SELECT categoryid from contactcategories where recid = :r and categoryid = 'Business'");
        q.bindValue(":r", cr->recid);
        q.exec();
        bool isBus = q.next();
        q.clear();
#endif

        QContact *c = new QContact;
        c->setUid(QUniqueId::fromUInt(cr->recid.toUInt()));
        c->setNameTitle(cr->nameTitle.toString());
        c->setFirstName(cr->firstName.toString());
        c->setMiddleName(cr->middleName.toString());
        c->setLastName(cr->lastName.toString());
        c->setSuffix(cr->suffix.toString());
        c->setCompany(cr->company.toString());
        c->setPortraitFile(cr->portraitFile.toString());
        if (isBus)
            c->setCategories(QLatin1String("Business"));

        simpleCache->contactCache.insert(row, c);
        return *c;
    }
    return contact(row);
}

QMap<QContactModel::Field, QString> ContactSqlIO::mFields;
QMap<QContactModel::Field, bool> ContactSqlIO::mUpdateable;

void ContactSqlIO::initMaps()
{
    struct csiIM {
        QContactModel::Field k;
        const char *field;
        bool u;
    };

    static const csiIM i[] = {
        { QContactModel::NameTitle, "title", true },
        { QContactModel::FirstName, "firstname", true },
        { QContactModel::MiddleName, "middlename", true },
        { QContactModel::LastName, "lastname", true },
        { QContactModel::Suffix, "suffix", true },
        { QContactModel::Label, 0, false }, // meta field

        { QContactModel::JobTitle, "jobtitle", true },
        { QContactModel::Department, "department", true },
        { QContactModel::Company, "company", true },

        { QContactModel::DefaultPhone, "default_phone", true },

        // email
        { QContactModel::DefaultEmail, "default_email", true },
        { QContactModel::Emails, 0, true },

        // business
        { QContactModel::BusinessWebPage, "b_webpage", true },

        { QContactModel::Office, "office", true },
        { QContactModel::Profession, "profession", true },
        { QContactModel::Assistant, "assistant", true },
        { QContactModel::Manager, "manager", true },

        // home
        { QContactModel::HomeWebPage, "h_webpage", true },

        //personal
        { QContactModel::Spouse, "spouse", true },
        { QContactModel::Gender, "gender", true },
        { QContactModel::Birthday, "birthday", true },
        { QContactModel::Anniversary, "anniversary", true },
        { QContactModel::Nickname, "nickname", true },
        { QContactModel::Children, "children", true },

        // other
        { QContactModel::Portrait, "portrait", true },

        { QContactModel::LastNamePronunciation, "lastname_pronunciation", true },
        { QContactModel::FirstNamePronunciation, "firstname_pronunciation", true },
        { QContactModel::CompanyPronunciation, "company_pronunciation", true },
        { QContactModel::Invalid, 0, false }
    };

    const csiIM *item = i;
    while (item->k != QContactModel::Invalid) {
        if (item->field)
            mFields.insert(item->k, item->field);
        mUpdateable.insert(item->k, item->u);
        ++item;
    }

}

QString ContactSqlIO::sqlField(QContactModel::Field k) const
{
    // special for the label field
    if (k == QContactModel::Label)
        return sqlLabelCache;
    if (mFields.contains(k))
        return mFields[k];
    return QString();
}

QContact::PhoneType ContactSqlIO::fieldToPhoneType(QContactModel::Field f) const
{
    switch (f) {
        case QContactModel::BusinessPhone:
            return QContact::BusinessPhone;
        case QContactModel::BusinessMobile:
            return QContact::BusinessMobile;
        case QContactModel::BusinessVOIP:
            return QContact::BusinessVOIP;
        case QContactModel::BusinessFax:
            return QContact::BusinessFax;
        case QContactModel::BusinessPager:
            return QContact::BusinessPager;

        case QContactModel::OtherPhone:
            return QContact::OtherPhone;
        case QContactModel::OtherMobile:
            return QContact::Mobile;
        case QContactModel::OtherFax:
            return QContact::Fax;
        case QContactModel::OtherPager:
            return QContact::Pager;

        case QContactModel::HomePhone:
            return QContact::HomePhone;
        case QContactModel::HomeVOIP:
            return QContact::HomeVOIP;
        case QContactModel::HomeMobile:
            return QContact::HomeMobile;
        case QContactModel::HomeFax:
            return QContact::HomeFax;
        case QContactModel::HomePager:
            return QContact::HomePager;

        default:
            return QContact::OtherPhone;
    }
}

bool ContactSqlIO::canUpdate(QContactModel::Field k) const
{
    if (mUpdateable.contains(k))
        return mUpdateable[k];
    return false;
}


bool ContactSqlIO::removeContact(int row)
{
    QUniqueId u = recordId(row);
    return removeContact(u);
}

bool ContactSqlIO::removeContact(const QUniqueId & id)
{
    if (id.isNull())
        return false;

    if (QPimSqlIO::removeRecord(id)) {
        notifyRemoved(id);
        emit recordsUpdated();
        return true;
    }
    return false;
}

bool ContactSqlIO::removeContact(const QContact &t)
{
    return removeContact(t.uid());
}

bool ContactSqlIO::removeContacts(const QList<int> &rows)
{
    QList<QUniqueId> ids = recordIds(rows);
    return removeContacts(ids);
}

bool ContactSqlIO::removeContacts(const QList<QUniqueId> &ids)
{
    if (QPimSqlIO::removeRecords(ids)) {
        notifyRemoved(ids);
        emit recordsUpdated();
        return true;
    }
    return false;
}

bool ContactSqlIO::updateContact(const QContact &t)
{
    if (QPimSqlIO::updateRecord(t)) {
        notifyUpdated(t.uid());
        emit recordsUpdated();
        return true;
    }
    return false;
}

QUniqueId ContactSqlIO::addContact(const QContact &contact, const QPimSource &source, bool createuid)
{
    QPimSource s;
    s.identity = contextsource;
    s.context = contextId();
    QUniqueId i = addRecord(contact, source.isNull() ? s : source, createuid);
    if (!i.isNull()) {
        QContact added = contact;
        added.setUid(i);
        notifyAdded(i);
        invalidateCache();
    }
    return i;
}

bool ContactSqlIO::updateExtraTables(uint uid, const QPimRecord &r)
{
    if (!removeExtraTables(uid))
        return false;
    return insertExtraTables(uid, r);
}

bool ContactSqlIO::removeExtraTables(uint uid)
{
    removeEmailsQuery.prepare();
    if (!removeAddressesQuery.prepare() || !removePhoneQuery.prepare())
        return false;

    removeEmailsQuery.bindValue(":i", uid);
    if (!removeEmailsQuery.exec())
        return false;
    removeEmailsQuery.reset();

    removeAddressesQuery.bindValue(":i", uid);
    if (!removeAddressesQuery.exec())
        return false;
    removeAddressesQuery.reset();

    removePhoneQuery.bindValue(":i", uid);
    if (!removePhoneQuery.exec())
        return false;
    removePhoneQuery.reset();

    return true;
}

bool ContactSqlIO::insertExtraTables(uint uid, const QPimRecord &r)
{
    const QContact &c = (const QContact &)r;

    insertEmailsQuery.prepare();
    insertAddressesQuery.prepare();
    insertPhoneQuery.prepare();

    QStringList e = c.emailList();
    foreach(QString i, e) {
        // don't insert empties.
        if (!i.trimmed().isEmpty()) {
            insertEmailsQuery.bindValue(":i", uid);
            insertEmailsQuery.bindValue(":a", i);
            if (!insertEmailsQuery.exec())
                return false;
        }
    }
    insertEmailsQuery.reset();

    /* home address, business address */

    QMap<QContact::Location, QContactAddress> a = c.addresses();
    QMapIterator<QContact::Location, QContactAddress> i(a);
    while(i.hasNext()) {
        i.next();
        QContactAddress a = i.value();
        insertAddressesQuery.bindValue(":i", uid);
        insertAddressesQuery.bindValue(":t", i.key());
        insertAddressesQuery.bindValue(":s", a.street);
        insertAddressesQuery.bindValue(":c", a.city);
        insertAddressesQuery.bindValue(":st", a.state);
        insertAddressesQuery.bindValue(":z", a.zip);
        insertAddressesQuery.bindValue(":co", a.country);
        if (!insertAddressesQuery.exec())
            return false;
    }
    insertAddressesQuery.reset();

    /* all phone numbers */
    /* home address, business address */
    QMap<QContact::PhoneType, QString> ph = c.phoneNumbers();
    QMapIterator<QContact::PhoneType, QString> phi(ph);
    while(phi.hasNext()) {
        phi.next();
        insertPhoneQuery.bindValue(":i", uid);
        insertPhoneQuery.bindValue(":t", phi.key());
        insertPhoneQuery.bindValue(":ph", phi.value());
        if (!insertPhoneQuery.exec())
            return false;
    }
    insertPhoneQuery.reset();

    return true;
}

void ContactSqlIO::setFilter(const QString &text, int flags)
{
    /* assume text is not empty */
    QString mSearchFilter;

    QSqlDriver *driver = database().driver();

    QSqlField field("firstname", QVariant::String);

    field.setValue(text+"\%");
    QString escapedStartsWith = driver->formatValue(field);

    field.setValue("\% "+text+"\%");
    QString escapedWordStartsWith = driver->formatValue(field);

#ifdef QTOPIA_PHONE
    /* should do on construction? or in QContactIO construction */
    QMap<QChar, QString> pbt = phoneButtonText();

    QRegExp simIndex("^(\\d{1,3})#$");
    if (simIndex.exactMatch(text)) {
        QStringList sl; 
        sl.append("simcardidmap");
        QPimSqlIO::setJoins(sl);
        QPimSqlIO::setFilter("simcardidmap.cardindex = " + simIndex.cap(1));
        return;
    }
    bool allNumbers = true;
    for(int i = 0; i < text.length(); ++i) {
        if (!pbt.contains(text[i])) {
            allNumbers = false;
            break;
        }
    }
    if (allNumbers && !text.isEmpty()) {
        bool first = true;
        foreach (QContactModel::Field f, labelSearchFields()) {
            if (!first)
                mSearchFilter += " or ";
            else
                first = false;
            mSearchFilter += "(";
            QString fname = "t1."+sqlColumn(f);
            int i;
            /* Handle first letter with > and <, as will work on index and hence
               cut search results down much faster */
            QChar firstChar = text[0];
            mSearchFilter += fname + " >= '" + QString(pbt[firstChar][0]) + "'";
            if(firstChar.isDigit()) {
                int firstCharVal = firstChar.digitValue();
                QChar nextCharVal('0'+firstCharVal+1);
                if (firstCharVal < 9 && pbt.contains(nextCharVal))
                    mSearchFilter += " and " + fname + " < '" + QString(pbt[nextCharVal][0]) + "'";
            }
            for (i= 0; i < text.length(); i++) {
                mSearchFilter += " and ";
                /* changes based of db, Mimer for example is
                   substring(string from index for length)
                   */
#if 0
                mSearchFilter += "substring(" + fname + " from " + QString::number(i+1) + " for 1) in (";
#else
                mSearchFilter += "lower(substr(" + fname + ", " + QString::number(i+1) + ", 1)) in (";
#endif
                QString letters = pbt[text[i]];
                for (int pos = 0; pos < letters.length(); ++pos) {
                    //if (letters[pos] == '\'' || letters[pos] == '@')
                        //break;
                    if (pos != 0)
                        mSearchFilter += ", ";
                    if (letters[pos] == '\'')
                        mSearchFilter += "''''";
                    else
                        mSearchFilter += "'" + letters[pos] + "'";
                }
                mSearchFilter += ")";
            }
            mSearchFilter += ")";
        }
    } else
#endif
    if (!text.isEmpty()) {
        /* text fields as mere 'starts with' */
        bool first = true;
        foreach (QContactModel::Field f, labelSearchFields()) {
            if (!first)
                mSearchFilter += " or ";
            else
                first = false;

            QString fname = "t1."+sqlColumn(f);
            // starts with
            mSearchFilter += "lower("+fname+") like " + escapedStartsWith + " ";
            // or contains a word starting with
            mSearchFilter += "or lower("+fname+") like " + escapedWordStartsWith + " ";
        }
    }

    /* flags
       ContainsMobileNumber
       inner join contactphonenumbers on recid group by contacts.recid;
       ContainsEmail
       inner join emailaddresses on recid group by contacts.recid;
       ContainsMailing
       inner join contactaddresses on recid group by contacts.recid;

     */
    QStringList joins;
    if (flags & QContactModel::ContainsPhoneNumber)
        joins += "contactphonenumbers";
    if (flags & QContactModel::ContainsEmail)
        joins += "emailaddresses";
    if (flags & QContactModel::ContainsMailing)
        joins += "contactaddresses";

    if (!joins.isEmpty())
        QPimSqlIO::setJoins(joins);
    if (!mSearchFilter.isEmpty()) {
        mSearchFilter = "(" + mSearchFilter + ")";
        QPimSqlIO::setFilter(mSearchFilter);
    } else {
        QPimSqlIO::clearFilter();
    }
    emit filtersUpdated();
}

void ContactSqlIO::clearFilter()
{
    QPimSqlIO::setFilters(QStringList());
    QPimSqlIO::setJoins(QStringList());
    emit filtersUpdated();
}

int ContactSqlIO::predictedRow(const QVariant &k, const QUniqueId &id) const
{
    /* key is likely string list, turn into QList with id bytearray
       at end */
    if (k.type() == QVariant::StringList) {
        QStringList sl = k.toStringList();
        QStringListIterator it(sl);
        QList<QContactModel::Field> mOrder = formatFieldOrder();
        QList<QVariant> keys;

        for (int i= 0; i < mOrder.count(); i++) {
            if (it.hasNext())
                keys.append(it.next());
            else
                keys.append(QVariant());
        }
        keys.append(id.toUInt());

        return model.predictedRow(keys);
    }
    return count(); // all at end if don't match sort key
}

#ifdef QTOPIA_PHONE
QUniqueId ContactSqlIO::matchPhoneNumber(const QString &phnumber, int &bestMatch) const
{
    QString local = QPhoneNumber::localNumber(phnumber);
    if (local.isEmpty())
        return QUniqueId();

    /* For URL matching, strip off the protocol */
    int colonIdx = local.indexOf(':');
    if (colonIdx > 0 && colonIdx < local.length() - 1)
        local = local.mid(colonIdx + 1);

    /* Unfortunately we don't store the "localNumber" version of the entered number, so spaces, dashes etc
       cause an SQL LIKE clause to miss the number.. instead, maintain a cache of localNumber entries
       derived from the stored numbers. */
    if (mLocalNumberCache.isEmpty()) {
        QPreparedSqlQuery q(database());
        // Ensure numbers are added in a deterministic order
        q.prepare("SELECT recid, phone_number FROM contactphonenumbers ORDER BY recid"); 
        q.exec();

        while (q.next()) {
            QUniqueId numberId = QUniqueId::fromUInt(q.value(0).toUInt());
            QString phoneNumber = q.value(1).toString();
            QString localNumber = QPhoneNumber::localNumber(phoneNumber);

            if (!localNumber.isEmpty()) {
                colonIdx = localNumber.indexOf(':');
                if (colonIdx > 0 && colonIdx < localNumber.length() - 1)
                    localNumber = localNumber.mid(colonIdx + 1);
                mLocalNumberCache.insert(localNumber, qMakePair(numberId, phoneNumber));
            }
        }
    }

    bestMatch = 0;
    QUniqueId bestContact;

    LocalNumberCache::const_iterator it = mLocalNumberCache.find(local), end = mLocalNumberCache.end();
    if (it != end) {
        // We have at least one exact match on the local number - see if there is a better one
        for ( ; (bestMatch != 100) && (it != end) && (it.key() == local); ++it) {
            const QPair<QUniqueId, QString>& matched(it.value());

            int match = QPhoneNumber::matchNumbers(phnumber, matched.second);
            if (match > bestMatch) {
                bestMatch = match;
                bestContact = matched.first;
            }
        }
    }

    /* Is it possible to have two phone numbers that do not match in local form, but have a 
       meaningful match in their complete forms?  If so, we need to do a full cache search:
    if (bestMatch <= 5) {
        // We haven't found an acceptable match - try all numbers
        for (it = mLocalNumberCache.begin(); (bestMatch != 100) && (it != end); ++it) {
            const QPair<QUniqueId, QString>& entry(it.value());

            int match = QPhoneNumber::matchNumbers(phnumber, entry.second);
            if (match > bestMatch) {
                bestMatch = match;
                bestContact = entry.first;
            }
        }
    }
    */
    qLog(Sql) << "QContactSqlIO::matchPhoneNumber() result:" << bestMatch << bestContact.toString();
    return bestContact;
}
#endif

/***************
 * CONTEXT
 **************/
class QContactSync;
class QContactDefaultContextData
{
public:
    QContactDefaultContextData(QContactDefaultContext *parent) :
        importQuery("SELECT recid FROM contacts WHERE context = :cont AND ("
                "(title = :t OR (title IS NULL AND :t2 IS NULL)) AND "
                "(firstname = :fn OR (firstname IS NULL AND :fn2 IS NULL)) AND "
                "(middlename = :mn OR (middlename IS NULL AND :mn2 IS NULL)) AND "
                "(lastname = :ln OR (lastname IS NULL AND :ln2 IS NULL)) AND "
                "(suffix = :s OR (suffix IS NULL AND :s2 IS NULL))) "
                "OR (company = :c AND title IS NULL AND firstname IS NULL "
                "AND middlename IS NULL AND lastname IS NULL AND suffix IS NULL)"),
        syncer(0),
        mAppointmentAccess(0),
        mContactBirthdayContext(0),
        mContactAnniversaryContext(0),
        mDependencyList(0),
        q(parent)
    {}

    // Lazy
    QAppointmentSqlIO* appointmentAccess()
    {
        if (!mAppointmentAccess)
            mAppointmentAccess = new QAppointmentSqlIO(q);
        return mAppointmentAccess;
    }

    QContactBirthdayContext* contactBirthdayContext()
    {
        if (!mContactBirthdayContext)
            mContactBirthdayContext = new QContactBirthdayContext(q, appointmentAccess());
        return mContactBirthdayContext;
    }

    QContactAnniversaryContext* contactAnniversaryContext()
    {
        if (!mContactAnniversaryContext)
            mContactAnniversaryContext = new QContactAnniversaryContext(q, appointmentAccess());
        return mContactAnniversaryContext;
    }

    QString contactEventLabel(const QContact& contact)
    {
        QString description;
        if (!contact.firstName().isEmpty()) {
            if (contact.lastName().isEmpty())
                description = contact.firstName();
            else
                description = QContactDefaultContext::tr("%1 %2", "firstname lastname").arg(contact.firstName()).arg(contact.lastName());
        } else if (!contact.lastName().isEmpty())
            description = contact.lastName();
        else if (!contact.company().isEmpty())
            description = contact.company();
        return description;
    }

    // Helper functions
    bool addContactEvent(const QUniqueId& contactId, const QContact &contact, const QString& type)
    {
        bool ret = false;
        QAppointment a;
        QDate date;
        QDependentEventsContext *context;
        QString description = contactEventLabel(contact);
        QUniqueId newId;
        uint newContextId;
        if (type == "birthday") {
            date = contact.birthday();
            context = contactBirthdayContext();
            newContextId = context->mappedContext();
        } else {
            date = contact.anniversary();
            context = contactAnniversaryContext();
            newContextId = context->mappedContext();
        }

        // XXX this should go into QUniqueId
        newId = QUniqueId::fromUInt(newContextId << 24 | contactId.toUInt() & 0x00ffffff);


        a.setStart(QDateTime(date, QTime(0,0)));
        a.setEnd(QDateTime(date, QTime(23,59)));
        a.setAllDay(true);
        a.setRepeatRule(QAppointment::Yearly);
        a.setDescription(description);
        a.setUid(newId);

        QUniqueId added = context->addAppointment(a, context->defaultSource());
        if (!added.isNull()) {
            if (QPimDependencyList::addDependency(contactId, added, type))
                ret = true;
            else {
                // Sigh, remove the appointment (XXX transactions)
                context->removeAppointment(added);
            }
        }

        return ret;
    }

    ContactSqlIO *access;
    QPreparedSqlQuery importQuery;
    QContactSync *syncer;

    QAppointmentSqlIO *mAppointmentAccess;
    QContactBirthdayContext *mContactBirthdayContext;
    QContactAnniversaryContext *mContactAnniversaryContext;
    QPimDependencyList *mDependencyList;

    QContactDefaultContext* q;
};

QContactDefaultContext::QContactDefaultContext(QObject *parent, QObject *access)
    : QContactContext(parent)
{
    d = new QContactDefaultContextData(this);
    d->access = qobject_cast<ContactSqlIO *>(access);
    Q_ASSERT(d->access);
}

QIcon QContactDefaultContext::icon() const
{
    static QIcon conicon(":icon/contactgroup");
    return conicon;
}

QString QContactDefaultContext::description() const
{
    return tr("Default contact storage");
}

QString QContactDefaultContext::title() const
{
    return tr("Contacts");
}

bool QContactDefaultContext::editable() const
{
    return true;
}

QSet<QPimSource> QContactDefaultContext::sources() const
{
    QSet<QPimSource> list;
    list.insert(defaultSource());
    return list;
}

QPimSource QContactDefaultContext::defaultSource() const
{
    QPimSource s;
    s.context = d->access->contextId();
    s.identity = contextsource;
    return s;
}

QUuid QContactDefaultContext::id() const
{
    return d->access->contextId();
}

/* TODO set mapping to int */
void QContactDefaultContext::setVisibleSources(const QSet<QPimSource> &set)
{
    int context = QPimSqlIO::sourceContext(defaultSource());

    QSet<int> filter = d->access->contextFilter();

    if (set.contains(defaultSource()))
        filter.remove(context);
    else
        filter.insert(context);

    d->access->setContextFilter(filter);
}

QSet<QPimSource> QContactDefaultContext::visibleSources() const
{
    int context = QPimSqlIO::sourceContext(defaultSource());

    QSet<int> filter = d->access->contextFilter();
    if (!filter.contains(context))
        return sources();
    return QSet<QPimSource>();
}

bool QContactDefaultContext::exists(const QUniqueId &id) const
{
    int context = QPimSqlIO::sourceContext(defaultSource());
    return d->access->exists(id) && d->access->context(id) == context;
}

QPimSource QContactDefaultContext::source(const QUniqueId &id) const
{
    if (exists(id))
        return defaultSource();
    return QPimSource();
}

bool QContactDefaultContext::updateContact(const QContact &contact)
{
    bool ret = d->access->updateContact(contact);

    if (ret) {
        QList<QUniqueId> adeps = QPimDependencyList::typedChildrenRecords(contact.uid(), QString("anniversary"));
        QList<QUniqueId> bdeps = QPimDependencyList::typedChildrenRecords(contact.uid(), QString("birthday"));
        if (contact.anniversary().isValid()) {
            if (adeps.count() == 0) {
                d->addContactEvent(contact.uid(), contact, "anniversary");
            } else {
                foreach(QUniqueId id, adeps) {
                    QAppointment a = d->appointmentAccess()->appointment(id);
                    a.setDescription(d->contactEventLabel(contact));
                    a.setStart(QDateTime(contact.anniversary(), QTime(0,0)));
                    a.setEnd(QDateTime(contact.anniversary(), QTime(23,59)));
                    a.setAllDay(true);
                    d->contactAnniversaryContext()->updateAppointment(a);
                }
            }
        } else {
            foreach(QUniqueId id, adeps) {
                d->contactAnniversaryContext()->removeAppointment(id);
            }
        }
        if (contact.birthday().isValid()) {
            // Might need to add, if it was previously invalid
            if (bdeps.count() == 0) {
                d->addContactEvent(contact.uid(), contact, "birthday");
            } else {
                foreach(QUniqueId id, bdeps) {
                    QAppointment a = d->appointmentAccess()->appointment(id);
                    a.setDescription(d->contactEventLabel(contact));
                    a.setStart(QDateTime(contact.birthday(), QTime(0,0)));
                    a.setEnd(QDateTime(contact.birthday(), QTime(23,59)));
                    a.setAllDay(true);
                    d->contactBirthdayContext()->updateAppointment(a);
                }
            }
        } else {
            foreach(QUniqueId id, bdeps) {
                d->contactBirthdayContext()->removeAppointment(id);
            }
        }
    }
    return ret;
}

bool QContactDefaultContext::removeContact(const QUniqueId &id)
{
    bool ret = d->access->removeContact(id);

    if (ret) {
        QMap<QString, QUniqueId> deps = QPimDependencyList::childrenRecords(id);
        QMap<QString, QUniqueId>::const_iterator it = deps.constBegin();
        while(it != deps.constEnd()) {
            if (it.key() == "birthday") {
                if (d->contactBirthdayContext()->removeAppointment(it.value()))
                    QPimDependencyList::removeDependency(id, it.value(), it.key());
            } else if (it.key() == "anniversary") {
                if (d->contactAnniversaryContext()->removeAppointment(it.value()))
                    QPimDependencyList::removeDependency(id, it.value(), it.key());
            }
            ++it;
        }
    }

    return ret;
}

QUniqueId QContactDefaultContext::addContact(const QContact &contact, const QPimSource &source)
{
    if (source.isNull() || source == defaultSource()) {
        QUniqueId id = d->access->addContact(contact, defaultSource());
        if (!id.isNull()) {
            if (contact.birthday().isValid())
                d->addContactEvent(id, contact, "birthday");
            if (contact.anniversary().isValid())
                d->addContactEvent(id, contact, "anniversary");
        }
        return id;
    }

    return QUniqueId();
}

static void mergeContact(QContact &destination, const QContact &source)
{
    // initially for importing sim contacts.  hence 'merge' only applies to
    // phone numbers of the contact.
    QStringList destNumbers = destination.phoneNumbers().values();
    QStringList sourceNumbers = source.phoneNumbers().values();

    QList<QContact::PhoneType> types = QContact::phoneTypes();

    foreach(QContact::PhoneType t, types) {
        QString number = source.phoneNumber(t);
        QString dnumber = destination.phoneNumber(t);
        // continue if number is already stored.
        if (number.isEmpty() || destNumbers.contains(number))
            continue;
        // continue if storing a number already from this contact (so above statement doesn't invalidate)
        if (!dnumber.isEmpty() && sourceNumbers.contains(dnumber)) {
            continue;
        }
        // now, we have a genuine conflict.
        // hard to say if should favor dest or source.  For now, siding with dest
        // since contact is still on source hence favoring dest does not reduce
        // information stored overall.
        if (!dnumber.isEmpty())
            continue;
        destination.setPhoneNumber(t, number);
    }
}

/*
   needs to export just this filter, not all shown
   by access belong to this context
*/
QList<QContact> QContactDefaultContext::exportContacts(const QPimSource &, bool &ok) const
{
    int context = QPimSqlIO::sourceContext(defaultSource());
    // unfiltered? current filter?
    QList<QContact> result;
    // temporary to allow separate filters
    ContactSqlIO *exportAccess = new ContactSqlIO(0);
    exportAccess->setCategoryFilter(d->access->categoryFilter());
    QSet<int> set;
    set.insert(context);
    exportAccess->setContextFilter(set, QPimSqlIO::RestrictToContexts);

    // don't even try exporting more than this.  Exporting involves going via a list,
    // which can be expensive.  Since most sim cards only handle about 200 contacts,
    // this is mostly a 'don't try something silly' check.
    if (exportAccess->count() > 1000) {
        ok = false;
        delete exportAccess;
        return result;
    }
    for(int i = 0; i < exportAccess->count(); ++i) {
        result.append(exportAccess->contact(i));
    }
    ok = true;
    delete exportAccess;
    return result;
}

QContact QContactDefaultContext::exportContact(const QUniqueId &id, bool &ok) const
{
    int context = QPimSqlIO::sourceContext(defaultSource());
    ok = d->access->exists(id) && d->access->context(id) == context;
    if (ok)
        return d->access->contact(id);
    return QContact();
}

/* for each contact, check if an identical named one is in the list.  if so, update it rather than add.
   */
bool QContactDefaultContext::importContacts(const QPimSource &s, const QList<QContact> &list)
{
    if (list.isEmpty())
        return false;
    int context = QPimSqlIO::sourceContext(s);

    QDateTime syncTime = QTimeZone::current().toUtc(QDateTime::currentDateTime());

    if (!d->access->startSync(s, syncTime))
        return false;

    foreach(QContact c, list) {
        // contacts are pretty much uniquely identified by name.  hence do a search for uid on the name fields.
        d->importQuery.prepare();
        d->importQuery.bindValue(":cont", context);
        d->importQuery.bindValue(":t", c.nameTitle());
        d->importQuery.bindValue(":t2", c.nameTitle());
        d->importQuery.bindValue(":fn", c.firstName());
        d->importQuery.bindValue(":fn2", c.firstName());
        d->importQuery.bindValue(":mn", c.middleName());
        d->importQuery.bindValue(":mn2", c.middleName());
        d->importQuery.bindValue(":ln", c.lastName());
        d->importQuery.bindValue(":ln2", c.lastName());
        d->importQuery.bindValue(":s", c.suffix());
        d->importQuery.bindValue(":s2", c.suffix());
        d->importQuery.bindValue(":c", c.company());

        if (!d->importQuery.exec()) {
            d->access->abortSync();
            return false;
        }
        if (d->importQuery.next()) {
            QUniqueId u(d->importQuery.value(0).toByteArray());
            QContact current = d->access->contact(u);
            mergeContact(current, c); // merges phone number information from sim to phone.  favours phone.
            d->importQuery.reset();
            updateContact(current);
        } else {
            d->importQuery.reset();
            addContact(c, s);
        }
    }
    return d->access->commitSync();
}


struct SyncPacket
{
    char command[8];
    uint size;
    char *data;
};

// can be made generic, e.g. record(), etc.
// This is about as simple as sync gets.
class QContactSync : public QObject
{
    Q_OBJECT
public:
    QContactSync(const QPimSource &mSource, ContactSqlIO *mAccess)
        : s(0), source(mSource), access(mAccess),
        mStatus(QContactDefaultContext::NotStarted),
        expectedServerHeader("QtopiaServer:040200\n\n"),
        clientHeader("Qtopia:040200\n\n"),
        nextSyncCommand("next    "),
        lastSyncCommand("last    "),
        replaceCommand( "replace "),
        removeCommand(  "remove  "),
        createCommand(  "create  "),
        endCommand(     "end     ")
    {
        // for now, socket.  later, obex.
        s = new QTcpSocket;
        connect(s, SIGNAL(connected()), this, SLOT(sendClientHeader()));
        connect(s, SIGNAL(readyRead()), this, SLOT(processCommand()));
    }

    void startSync() {
        mStatus = QContactDefaultContext::InProgress;
        s->connectToHost("localhost", 7705);
    }

    QContactDefaultContext::Status status() const { return mStatus; }

public slots:
    void sendClientHeader()
    {
        QDataStream stream(s);
        nextSync = QTimeZone::current().toUtc(QDateTime::currentDateTime());
        lastSync = access->lastSyncTime(source);
        stream.writeRawData(clientHeader, 15);
        stream.writeRawData(nextSyncCommand, 8);
        stream << nextSync;
        stream.writeRawData(lastSyncCommand, 8);
        stream << lastSync;

        // next action is to recieve the server header
    }

    void processCommand()
    {
        if (serverNextSync.isNull() || serverLastSync.isNull())
            verifyServerHeader();
        else
            recieveServerDiff(); // should check that its sent the diff.
    }

    void verifyServerHeader()
    {
        QDataStream stream(s);
        if (!compare(stream, expectedServerHeader, 21)) {
            fail(QContactDefaultContext::BadAuthentication);
            return;
        }

        if (!compare(stream, nextSyncCommand, 8)) {
            fail(QContactDefaultContext::BadAuthentication);
            return;
        }
        stream >> serverNextSync;
        if (!compare(stream, lastSyncCommand, 8)) {
            fail(QContactDefaultContext::BadAuthentication);
            return;
        }
        stream >> serverLastSync;

        // later, should check that the server concepts
        // of sync time match the clients

        sendClientDiff();
    }

    void sendClientDiff()
    {
        // date stamps tell us if we are about to sync the right data;
        // use later;

        // first get and send diff.
        QDataStream stream(s);

        QPreparedSqlQuery q;
        q.prepare("SELECT recid, created, modified, removed FROM changelog JOIN contacts WHERE contacts.recid = changelog.recid AND modified > :m AND context = :c");
        q.bindValue(":m", serverLastSync);
        q.bindValue(":c", QPimSqlIO::sourceContext(source));
        if (!q.exec()) {
            fail(QContactDefaultContext::DataAccessError);
            return;
        }
        while(q.next()) {
            QUniqueId recid(q.value(0).toByteArray());
            QDateTime created(q.value(1).toDateTime());
            QDateTime modified(q.value(1).toDateTime());
            QDateTime removed(q.value(1).toDateTime());

            if (removed.isValid()) {
                // removed contact
                stream.writeRawData(removeCommand, 8);
                stream << recid;
            } else {
                // created or modifed contact
                QContact contact = access->contact(recid);
                if (created > serverLastSync) {
                    stream.writeRawData(createCommand, 8);
                } else {
                    stream.writeRawData(replaceCommand, 8);
                }
                stream << recid;
                writeContact(stream, contact);
            }
        }
        stream.writeRawData(endCommand, 8);
        s->waitForBytesWritten(100);

        // now wait for return diff from server
    }

    void recieveServerDiff()
    {
        QDataStream stream(s);

        // int the interests of not prolonging the lock of the db,
        // this function is not split into multiple reads nor is
        // is the startSync or locking of the database done before this point
        //
        access->startSync(source, nextSync);

        while(1) {
            QByteArray command;
            stream >> command;
            if (command == replaceCommand) {
                QContact contact;
                QUniqueId recid;
                stream >> recid;
                // recid used on server for merge info
                contact = readContact(stream);
                if (contact.uid().isNull()) {
                    fail(QContactDefaultContext::ParseError);
                    access->abortSync();
                    return;
                }
                access->updateContact(contact);
            } else if (command == createCommand) {
                QContact contact;
                QUniqueId recid;
                stream >> recid;
                contact = readContact(stream);
                if (contact.uid().isNull()) {
                    fail(QContactDefaultContext::ParseError);
                    access->abortSync();
                    return;
                }
                access->addContact(contact,source);
            } else if (command == removeCommand) {
                QUniqueId recid;
                stream >> recid;
                access->removeContact(recid);
            } else if (command == endCommand) {
                // done
                break;
            } else {
                // error
                fail(QContactDefaultContext::ParseError);
                access->abortSync();
                return;
            }
        }
        access->commitSync();
        s->disconnect();
        emit completed();
    }

signals:
    void error(int);
    void completed();

private:
    void fail(QContactDefaultContext::Status status)
    {
        mStatus = status;
        s->disconnect();
        emit error(mStatus);
    }

    bool compare(QDataStream &stream, const char *target, int size)
    {
        char buffer[128];
        stream.readRawData(buffer, size);
        buffer[size] = '\0';
        return strncmp(buffer, target, size) == 0;
    }

    static void writeContact(QDataStream &stream, const QContact &c)
    {
        QByteArray bytes;
        QDataStream arrayStream(&bytes, QIODevice::WriteOnly);
        c.writeVCard(&arrayStream);
        stream << bytes;
    }

    static QContact readContact(QDataStream &stream)
    {
        QByteArray bytes;
        stream >> bytes;
    
        QList<QContact> list = QContact::readVCard(bytes);
        if(list.count() == 1)
            return list[0];
        return QContact();
    }


    QTcpSocket *s;
    QPimSource source;
    ContactSqlIO *access;

    QContactDefaultContext::Status mStatus;

    QDateTime lastSync;
    QDateTime nextSync;
    QDateTime serverLastSync;
    QDateTime serverNextSync;

    const char *expectedServerHeader;
    const char *clientHeader;
    const char *nextSyncCommand;
    const char *lastSyncCommand;
    const char *replaceCommand;
    const char *removeCommand;
    const char *createCommand;
    const char *endCommand;
};

void QContactDefaultContext::startSync()
{
    if (!d->syncer)
        d->syncer = new QContactSync(defaultSource(), d->access);
    d->syncer->startSync();
}

bool QContactDefaultContext::syncing() const
{
    return status() == InProgress;
}

void QContactDefaultContext::syncProgress(int &amount, int &total) const
{
    //  can probably do better than this ;)
    if (syncing()) {
        amount = 0;
        total = 1;
    } else {
        amount = 1;
        total = 1;
    }
}

QContactDefaultContext::Status QContactDefaultContext::status() const
{
    return d->syncer->status();
}

QString QContactDefaultContext::statusMessage(Status status)
{
    switch(status) {
        case QContactDefaultContext::NotStarted:
        case QContactDefaultContext::Completed:
            return QString();
        case QContactDefaultContext::InProgress:
            return tr("Downloading appointments.");
        case QContactDefaultContext::ServiceUnavailable:
            return tr("Could not contact sync destination.");
        case QContactDefaultContext::ParseError:
            return tr("Error processing server sync information.");
        default:
            return tr("An unknown error has occurred.");
    }
}

#include "qcontactsqlio.moc"
