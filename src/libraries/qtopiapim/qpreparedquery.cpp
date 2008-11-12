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

#include "qpreparedquery_p.h"
#include <sqlite3.h>
#include <qpimsqlio_p.h> // for db handle
#include <QSqlDriver>
#include <QDebug>

#include <qtopialog.h>

#ifdef SQLITE_DIRECT

QPreparedSqlQuery::QPreparedSqlQuery()
    : mHandle(0), mDBHandle(0), skip_step(false)
{
}

QPreparedSqlQuery::QPreparedSqlQuery(QSqlDatabase db)
    : mHandle(0), mDBHandle(*static_cast<sqlite3**>(db.driver()->handle().data())), skip_step(false)
{
}


// +1, hash being different from statement means not prepared.
QPreparedSqlQuery::QPreparedSqlQuery(const QString &statement)
    : mText(statement), mHandle(0), mDBHandle(0), skip_step(false)
{
}

QPreparedSqlQuery::~QPreparedSqlQuery()
{
    clear();
}

bool QPreparedSqlQuery::isValid() const
{
    return (mHandle != 0);
}

bool QPreparedSqlQuery::prepare()
{
    qLog(Sql) << "QPreparedSqlQuery::prepare()";
    if (mText.isEmpty()) {
        return false;
    }
    if (mHandle) {
        reset();
        return true; // e.g. once prepared, always prepared until cleared.
    }
    qLog(Sql) << "QPreparedSqlQuery::prepare() - parse statement:" << mText;

    if (!mDBHandle)
        mDBHandle = *static_cast<sqlite3**>(QPimSqlIO::database().driver()->handle().data());

    // could use this to 'parse for multiple statements'.
    // use v2 when it becomes available.
    switch(sqlite3_prepare(mDBHandle, mText.toUtf8().data(), -1, &mHandle, 0)) {
        case SQLITE_OK:
            qLog(Sql) << "prepare query succeeded";
            return true;
        default:
            mError.setDriverText(QString::fromUtf8(sqlite3_errmsg(mDBHandle)));
            qLog(Sql) << "prepare query error:" << mError.driverText();
            return false;
    }
}

bool QPreparedSqlQuery::prepare(const QString &text)
{
    if (mHandle && text != mText)
        clear();
    mText = text;
    return prepare();
}

void QPreparedSqlQuery::reset()
{
    qLog(Sql) << "QPreparedSqlQuery::reset() -" << mText;
    if (mHandle) {
        int res = sqlite3_reset(mHandle);
        if (res != SQLITE_OK)
            qLog(Sql) << "Failed to reset query:" << mText << "-" << sqlite3_errmsg(mDBHandle);
    }
    skip_step = false;
}

void QPreparedSqlQuery::clear()
{
    qLog(Sql) << "QPreparedSqlQuery::clear() - " << mText;
    int res;
    skip_step = false;
    if (mHandle) {
        res = sqlite3_finalize(mHandle);
        if (res != SQLITE_OK)
            qLog(Sql) << "Failed to finalize query:" << mText << "-" << sqlite3_errmsg(mDBHandle);
    }
    mHandle = 0;
}
void QPreparedSqlQuery::bindValue( const QString & placeholder, const QVariant & value, QSql::ParamType )
{
    if (mHandle) {
        int pos = sqlite3_bind_parameter_index(mHandle, placeholder.toUtf8().constData());
        if (pos)
            bindValue(pos-1, value);
    }

}

void QPreparedSqlQuery::bindValue( int pos, const QVariant & value, QSql::ParamType )
{
    qLog(Sql) << "QPreparedSqlQuery::bindValue(" << pos << value << ") -" << mText;
    if (!mHandle)
        return;
    pos++;
    int res;
    const QByteArray *ba;
    const QString *str;
    int attempt = 2;
    while (attempt--) {
        if (value.isNull()) {
            res = sqlite3_bind_null(mHandle, pos);
        } else {
            switch (value.type()) {
                case QVariant::ByteArray:
                    ba = static_cast<const QByteArray*>(value.constData());
                    res = sqlite3_bind_blob(mHandle, pos, ba->constData(),
                            ba->size(), SQLITE_TRANSIENT);
                    break;
                case QVariant::Int:
                    res = sqlite3_bind_int(mHandle, pos, value.toInt());
                    break;
                case QVariant::Double:
                    res = sqlite3_bind_double(mHandle, pos, value.toDouble());
                    break;
                case QVariant::UInt:
                case QVariant::LongLong:
                    res = sqlite3_bind_int64(mHandle, pos, value.toLongLong());
                    break;
                case QVariant::String:
                    str = static_cast<const QString*>(value.constData());
                    res = sqlite3_bind_text16(mHandle, pos, str->utf16(),
                            (str->size()) * sizeof(QChar), SQLITE_TRANSIENT);
                default:
                    {
                        QString str = value.toString();
                        res = sqlite3_bind_text16(mHandle, pos, str.utf16(),
                                (str.size()) * sizeof(QChar), SQLITE_TRANSIENT);
                    }
                    break;
            }
        }
        /* assume lack of step before rebind... */
        if (res == SQLITE_MISUSE)
            sqlite3_reset(mHandle);
    }

    if (res != SQLITE_OK) {
        mError.setDriverText(
                QString("Failed to bind parameter at %1: %2").arg(sqlite3_errmsg(mDBHandle)).arg(pos));
        qLog(Sql) << "bind value error:" << mError.driverText();
    }
}

QMap<QString, QVariant> QPreparedSqlQuery::boundValues() const
{
    qLog(Sql) << "boundValues not supported in QPreparedSqlQuery";
    return QMap<QString, QVariant>(); // sqlite doesn't let you query bound values
}

bool QPreparedSqlQuery::exec()
{
    qLog(Sql) << "QPreparedSqlQuery::exec()";
    // SQLITE doesn't need an explicit exec, rather it will just exec
    // on the next call to step.  Qt uses exec as a chance to bind
    // parameters and check errors while doing so.  This isn't
    // done here for performance and simplicity reasons.
    
    // doesn't unbind values.
    int res = sqlite3_step(mHandle);
    switch(res) {
        case SQLITE_ROW:
        case SQLITE_DONE:
            skip_step = true;
            step_res = res;
            qLog(Sql) << "exec succeeded";
            return true;
        default:
            mError.setDriverText( sqlite3_errmsg(mDBHandle) );
            qLog(Sql) << "exec error:" << res << mError.driverText();
            break;
    }
    return true;
}

bool QPreparedSqlQuery::isNull( int field ) const
{
    return sqlite3_column_type(mHandle, field) == SQLITE_NULL;
}

QSqlError QPreparedSqlQuery::lastError() const
{
    return mError;
}

QString QPreparedSqlQuery::lastQuery() const
{
    return mText;
}

bool QPreparedSqlQuery::next()
{
    if (skip_step) {
        skip_step = false;
        return step_res == SQLITE_ROW;
    } else {
        switch(sqlite3_step(mHandle))
        {
            case SQLITE_ROW:
                return true;
            case SQLITE_DONE:
                return false;
            case SQLITE_BUSY:
                mError.setDriverText( sqlite3_errmsg(mDBHandle) );
                qLog(Sql) << mError.driverText();
                return false;
            case SQLITE_ERROR:
                mError.setDriverText( sqlite3_errmsg(mDBHandle) );
                qLog(Sql) << mError.driverText();
                return false;
            case SQLITE_MISUSE:
                mError.setDriverText( sqlite3_errmsg(mDBHandle) );
                qLog(Sql) << mError.driverText();
                return false;
        }
    }
    return false;
}

//bool QPreparedSqlQuery::seek( int index, bool relative = false )
//{
//}

QVariant QPreparedSqlQuery::value( int index ) const
{
    switch(sqlite3_column_type(mHandle, index)) {
        case SQLITE_INTEGER:
            return QVariant(sqlite3_column_int64(mHandle, index));
        case SQLITE_FLOAT:
            return QByteArray(static_cast<const char *>(
                        sqlite3_column_blob(mHandle, index)),
                    sqlite3_column_bytes(mHandle, index));
        case SQLITE_BLOB:
            return QVariant(sqlite3_column_double(mHandle, index));
        case SQLITE_NULL:
            return QVariant();
        default:
        case SQLITE_TEXT:
            return QString::fromUtf16(static_cast<const ushort *>(sqlite3_column_text16(mHandle, index)),
                        sqlite3_column_bytes16(mHandle, index) / sizeof(ushort));
    }
}

#else // SQLITE_DIRECT

QPreparedSqlQuery::QPreparedSqlQuery()
    : mQuery(0)
{
}

QPreparedSqlQuery::QPreparedSqlQuery(QSqlDatabase db)
    : mQuery(new QSqlQuery(db))
{
    mQuery->setForwardOnly(true);
}


// +1, hash being different from statement means not prepared.
QPreparedSqlQuery::QPreparedSqlQuery(const QString &statement)
    : mText(statement), mQuery(0)
{
}

QPreparedSqlQuery::~QPreparedSqlQuery()
{
    clear();
}

bool QPreparedSqlQuery::isValid() const
{
    return mQuery && mQuery->isValid();
}

bool QPreparedSqlQuery::prepare()
{
    qLog(Sql) << "QPreparedSqlQuery::prepare()";
    bool res;
    if (!mText.isEmpty() && !mQuery)
    {
        mQuery = new QSqlQuery(QPimSqlIO::database());
        mQuery->setForwardOnly(true);
        qLog(Sql) << "QPreparedSqlQuery::prepare() - parse statement:" << mText;
        res =  mQuery->prepare(mText);
    } else if (mQuery) {
        res = true;
    } else
        res = false;
    if (res)
        qLog(Sql) << "prepare query succeeded";
    else
        qLog(Sql) << "prepare query error:" << mQuery->lastError().text();
    return res;
}

bool QPreparedSqlQuery::prepare(const QString &text)
{
    if (text != mText && !text.isEmpty())
    {
        clear();
        delete mQuery;
        mQuery = 0;
        mText = text;
        return prepare();
    } else if (text.isEmpty()) {
        qLog(Sql) << "prepare query error: no text";
        return false;
    }
    qLog(Sql) << "prepare query succeeded - already prepared as text";
    return true;
}

void QPreparedSqlQuery::reset()
{
    qLog(Sql) << "QPreparedSqlQuery::reset() -" << mText;
    if (mQuery)
        mQuery->finish();
}

void QPreparedSqlQuery::clear()
{
    qLog(Sql) << "QPreparedSqlQuery::clear() - " << mText;
    if (mQuery)
        mQuery->clear();
}

void QPreparedSqlQuery::bindValue( const QString & placeholder, const QVariant & value, QSql::ParamType type )
{
    qLog(Sql) << "QPreparedSqlQuery::bindValue(" << placeholder << value << ") -" << mText;
    if (mQuery)
        mQuery->bindValue(placeholder, value, type);
}

void QPreparedSqlQuery::bindValue( int pos, const QVariant & value, QSql::ParamType type)
{
    qLog(Sql) << "QPreparedSqlQuery::bindValue(" << pos << value << ") -" << mText;
    if (mQuery)
        mQuery->bindValue(pos, value, type);
}

QMap<QString, QVariant> QPreparedSqlQuery::boundValues() const
{
    if (mQuery)
        return mQuery->boundValues();
    return QMap<QString, QVariant>();
}

bool QPreparedSqlQuery::exec()
{
    qLog(Sql) << "QPreparedSqlQuery::exec()";
    if (mQuery) {
        bool res;
        res =  mQuery->exec();
        if (!res)
            qLog(Sql) << "QPreparedSqlQuery::exec() -" << mQuery->lastError();
        return res;
    } else
        qLog(Sql) << "QPreparedSqlQuery::exec() - query not prepared";
    return false;
}

bool QPreparedSqlQuery::isNull( int field ) const
{
    if (mQuery)
        return mQuery->isNull(field);
    return false;
}

QSqlError QPreparedSqlQuery::lastError() const
{
    if (mQuery)
        return mQuery->lastError();
    return QSqlError();
}

QString QPreparedSqlQuery::lastQuery() const
{
    if (mQuery)
        return mQuery->lastQuery();
    return QString();
}

bool QPreparedSqlQuery::next()
{
    if (mQuery)
        return mQuery->next();
    return false;
}

QVariant QPreparedSqlQuery::value( int index ) const
{
    if (mQuery)
        return mQuery->value(index);
    return QVariant();
}
#endif // SQLITE_DIRECT
