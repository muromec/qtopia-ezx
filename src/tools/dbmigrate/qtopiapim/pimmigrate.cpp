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

#include "pimmigrate.h"

#include <QStringList>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QTimeZone>
#include <QDebug>
#include <QTextCodec>
#include <QFile>
#include <QSqlError>

#include "quniqueid.h"

#include "../migrateengine.h"

#include <sqlite3.h>
#include <QSqlDriver>
#include <netinet/in.h>

// migratoin tables
const QStringList &PimMigrate::tables() const
{
    static QStringList tables;

    if (tables.count() == 0) {
        // in order of when they should be processed.
        tables << "changelog";
        tables << "sqlsources";

        tables << "appointments";
        tables << "appointmentcategories";
        tables << "appointmentcustom";
        tables << "appointmentexceptions";
        tables << "contacts";
        tables << "contactaddresses";
        tables << "contactcategories";
        tables << "contactcustom";
        tables << "contactphonenumbers";
        tables << "emailaddresses";
        tables << "tasks";
        tables << "taskcategories";
        tables << "taskcustom";

        tables << "simcardidmap";

        tables << "googleid";

        tables << "pimdependencies";
    }

    return tables;
}


void convertRecIdFunc(sqlite3_context *context, int, sqlite3_value**values)
{
    int size = sqlite3_value_bytes(values[0]);
    if (size != 8)
        return;

    struct PairedUint {
        quint32 left;
        quint32 right;
    };
    // network byte order
    PairedUint *id = (PairedUint *)sqlite3_value_blob(values[0]);

    quint32 result = (ntohl(id->left) << 24) | (ntohl(id->right) & 0x00ffffff);

    sqlite3_result_int(context, result);
}


PimMigrate::~PimMigrate()
{}

//PimMigrate::PimMigrate() : syncTime(QTimeZone::current().toUtc(QDateTime::currentDateTime()))
PimMigrate::PimMigrate(QDBMigrationEngine *engine) : syncTime(QDateTime::currentDateTime().toUTC()), mi(engine)
{
}

bool PimMigrate::migrate()
{
    // 4.2.0 brings in the changelog table
    // 4.2.2 brings in a change to rec id's.
    // but because migrate only handles from 4.1.x,
    // the only relavant table version number is 110, the
    // number that maps to 4.2.2

    // first ensure changelog exists.
    CHECK(mi->ensureSchema("changelog"));
    CHECK(mi->setTableVersion("changelog", 110));

    const QSqlDatabase &db  = mi->database();

    // add function to migrate recid.  Since only db this affects is sqlite, use sqlite func
    QVariant v = db.driver()->handle();
    if (v.isValid() && strcmp(v.typeName(), "sqlite3*") == 0) {
        // v.data() returns a pointer to the handle
        sqlite3 *handle = *static_cast<sqlite3 **>(v.data());
        CHECK(handle);
        int result = sqlite3_create_function( handle, "convertRecId", 1, SQLITE_ANY, 0, convertRecIdFunc, 0, 0);
        CHECK(result == SQLITE_OK);
    }

    QStringList oldTables;
    oldTables << "taskcustom";
    oldTables << "taskcategories";
    oldTables << "tasks";
    oldTables << "emailaddresses";
    oldTables << "contactphonenumbers";
    oldTables << "contactcustom";
    oldTables << "contactcategories";
    oldTables << "contactaddresses";
    oldTables << "contacts";
    oldTables << "appointmentexceptions";
    oldTables << "appointmentcustom";
    oldTables << "appointmentcategories";
    oldTables << "appointments";

    // first we need to drop the old schema's in the right order.
    QStringList existingTables = db.tables();
    foreach(QString table, oldTables) {
        int v = mi->tableVersion(table);
        if (existingTables.contains(table) && v < 110) {
            // back it up and drop.
            CHECK(mi->copyTable(table, table+"_old"));
            QSqlQuery query(db);
            CHECK(query.exec("DROP TABLE "+table));
        }
    }

    foreach(QString table, tables()) {
        CHECK(migrate(db, table, mi->tableVersion(table)));
    }

    // only one version so far.. change this line if version changes
    if (!existingTables.contains("syncServers")) {
        // sync server identification
        QSqlQuery q(db);
        CHECK(q.exec("CREATE TABLE syncServers (serverIdentity VARCHAR(255), datasource VARCHAR(255), lastSyncAnchor TIMESTAMP, UNIQUE(serverIdentity, datasource))"));
        CHECK(mi->setTableVersion("syncServers", 111));
    }
    return true;
}


bool PimMigrate::migrate(const QSqlDatabase &db, const QString &table, int version)
{
    if (version >= 110)
        return true;
    else {
        CHECK(mi->ensureSchema(table));
        CHECK(mi->setTableVersion(table, 110));

        QStringList existingTables = db.tables();

        // if it was backed up
        if (existingTables.contains(table+"_old")) {
            QSqlQuery query(db);
            CHECK(query.prepare(queryText("copy", table)));
            CHECK(query.exec());

            if (table == "tasks" || table == "contacts" || table == "appointments") {
                QSqlQuery changelog(db);
                CHECK(changelog.prepare("INSERT INTO changelog (recid, created, modified, removed) SELECT recid, :dt1, :dt2, NULL FROM " + table + ";"));
                changelog.bindValue("dt1", syncTime);
                changelog.bindValue("dt2", syncTime);
                CHECK(changelog.exec());
            }

            //mi->dropTable(table+"_old");
            CHECK(query.exec("DROP TABLE "+table+"_old;"));
        }

        if (table == "pimdependencies") {
            CHECK(createContactEvents(db));
            CHECK(createTodoEvents(db));
        }
    }
    return true;
}

QString PimMigrate::queryText(const QString& queryType, const QString &table)
{
    const QSqlDatabase &db  = mi->database();

    QFile data(QLatin1String(":/QtopiaSql/") + queryType + QLatin1String("/") + db.driverName() + QLatin1String("/") + table);
    data.open(QIODevice::ReadOnly);
    QTextStream ts(&data);
    // read assuming utf8 encoding.
    ts.setCodec(QTextCodec::codecForName("utf8"));
    ts.setAutoDetectUnicode(true);

    // assumption, no comments or mult-statements.  those belong in rc file if anywhere.
    return ts.readAll();
}

bool PimMigrate::createContactEvents(const QSqlDatabase &db)
{
    bool ret = true;

    uint birthdayContext = QUniqueIdGenerator::mappedContext(QUuid("822d32bc-d646-4b36-b1fd-090b2199b725"));
    uint anniversaryContext = QUniqueIdGenerator::mappedContext(QUuid("5a72a3fe-f2a8-4cba-94bb-0880dac41520"));

    QSqlQuery createBirthdays(db);
    QSqlQuery createBirthdaysDeps(db);
    QSqlQuery createAnniversaries(db);
    QSqlQuery createAnniversariesDeps(db);

    // Birthdays
    CHECK(createBirthdaysDeps.prepare(queryText("generate", "contact_birthdays_deps")));
    createBirthdaysDeps.bindValue(":birthdaycontext", birthdayContext);
    CHECK(createBirthdaysDeps.exec());

    CHECK(createBirthdays.prepare(queryText("generate", "contact_birthdays")));
    createBirthdays.bindValue(":birthdaycontext", birthdayContext);
    createBirthdays.bindValue(":birthdaycontext2", birthdayContext);
    CHECK(createBirthdays.exec());

    // Anniversaries
    CHECK(createAnniversariesDeps.prepare(queryText("generate", "contact_anniversaries_deps")));
    createAnniversariesDeps.bindValue(":anniversarycontext", anniversaryContext);
    CHECK(createAnniversariesDeps.exec());

    CHECK(createAnniversaries.prepare(queryText("generate", "contact_anniversaries")));
    createAnniversaries.bindValue(":anniversarycontext", anniversaryContext);
    createAnniversaries.bindValue(":anniversarycontext2", anniversaryContext);
    CHECK(createAnniversaries.exec());

    return ret;
}

bool PimMigrate::createTodoEvents(const QSqlDatabase &db)
{
    bool ret = true;

    uint taskContext = QUniqueIdGenerator::mappedContext(QUuid("a2c69584-a85a-49c6-967b-6e2895f5c777"));

    QSqlQuery createTaskEvents(db);
    QSqlQuery createTaskEventsDeps(db);

    CHECK(createTaskEvents.prepare(queryText("generate", "task_duedates")));
    createTaskEvents.bindValue(":taskcontext", taskContext);
    createTaskEvents.bindValue(":taskcontext2", taskContext);
    CHECK(createTaskEvents.exec());

    CHECK(createTaskEventsDeps.prepare(queryText("generate", "task_duedates_deps")));
    createTaskEventsDeps.bindValue(":taskcontext", taskContext);
    CHECK(createTaskEventsDeps.exec());

    return ret;
}

