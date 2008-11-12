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

#include "qvaluespace.h"
#include <QSettings>
#include <QCoreApplication>
#include <QTranslator>
#include <QFileInfo>
#include <QDateTime>
#include <QSet>
#include <QTranslator>
#include "qfilemonitor.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <QStringList>
#include <QVariant>
#include <QRect>
#include <QSize>
#include <QPoint>
#include <QHash>
#include <QCache>
#include <QMap>
#include <QByteArray>
#include <QPair>
#include <QDir>
#include <qtopialog.h>


/*
   The INI Layer exposes regular QSettings-style INI files into the value space.
   INI files support change notification, and the creation or removal of INI
   files on disk is supported.

   How INI files are mapped into the Value Space is governed by a configuration
   file - itself an INI file.  The general form of this configuration file is:

   \code

   [General]
   Mappings=<Number of mappings that follow>

   [Mapping<x>]
   ValueSpacePath=<Value space location to map to>
   FileSystemPath=<File system location to map from>
   FileSystemPaths=<Optional count>
   FileSystemPath<x>=<Optional file system location to map from>
   FileSystemExtension=<Optional extension of files under FileSystemPath>
   DirectoryDepth=<Optional recursive depth>

   \endcode

   Example file format:

   [General]
   Mappings=2

   [Mapping0]
   ValueSpacePath=/Settings
   FileSystemPath=/home/akennedy/Settings
   FileSystemExtension=conf
   DirectoryDepth=1

   [Mapping1]
   ValueSpacePath=/Settings/Buttons
   FileSystemPath=/opt/Qtopia/etc/defaultbuttons.conf

 */


/*
   Following parsing, a raw copy of the key value is stored in this structure.
   When a client requests the value, it is decoded into the returnable variant
   at that time.  This reduces decoding time into just that which is necessary
   for the *used* values.
 */

// declare Ini
namespace Ini
{
    // declare Ini::Key
    struct Key {
        char * key;
        unsigned int len;

        void destroy() {
            delete [] key;
            key = 0;
            len = 0;
        }
    };

    // declare Ini::Value
    struct Value {
        Value() : var(0), rawValue(0), len(0) {}
        enum { Variant, Raw, RawTranslatable } type;
        QVariant * var;
        char * rawValue;
        unsigned int len;

        void destroy() {
            if(Variant == type)
                delete var;
            else
                delete [] rawValue;
            var = 0;
            rawValue = 0;
            len = 0;
        }
    };

    // declare Ini::Contents
    struct Contents {
        Contents() : cost(0) {}
        Contents(const Contents &other)
            : cost(other.cost),
              contents(other.contents) {}
        Contents & operator=(const Contents &other)
        {
            cost = other.cost;
            contents = other.contents;
            return *this;
        }

        unsigned int cost;
        QMap<Ini::Key, Ini::Value> contents;
    };

    static bool save(const Contents & contents, const QByteArray &file);
    static bool load(Contents & contents, const QByteArray &file);
    static void clear(QMap<Ini::Key, Ini::Value> * data);
    static Contents parseIni(const char * mem, unsigned int length);
    static void toVariant(Ini::Value & val);
    static QString &unescapedLeadingAt(QString &s);
    static QVariant stringListToVariantList(const QStringList &l);
    static QStringList splitArgs(const QString &s, int idx);
    static QVariant stringToVariant(const QString &s);
    static QStringList *unescapedStringList(const char *str, const char *to,
                                            QString &result);
    static void iniEscapedStringList(const QStringList &, QByteArray &);
    static void iniEscapedString(const QString &str, QByteArray &result);
    static QStringList variantListToStringList(const QVariantList &l);
    static QString variantToString(const QVariant &v);
    static QString &escapedLeadingAt(QString &s);
};

// define Ini
static inline char lower(char i)
{
    if(i >= 'A' && i <= 'Z')
        return i - 'A' + 'a';
    else
        return i;
}

static inline char fromhex(const char *i)
{
    unsigned char rv = 0;
    const char c1 = *i;
    const char c2 = *(i + 1);
    if(c1 >= '0' && c1 <= '9')
        rv += 16 * (c1 - '0');
    else if(c1 >= 'a' && c1 <= 'f')
        rv += 16 * (c1 - 'a' + 10);
    else if(c1 >= 'A' && c1 <= 'F')
        rv += 16 * (c1 - 'A' + 10);

    if(c2 >= '0' && c2 <= '9')
        rv += (c2 - '0');
    else if(c2 >= 'a' && c2 <= 'f')
        rv += (c2 - 'a' + 10);
    else if(c2 >= 'A' && c2 <= 'F')
        rv += (c2 - 'A' + 10);

    return (char)rv;
}

inline bool operator<(const Ini::Key &lhs, const Ini::Key &rhs)
{
    if(lhs.len != rhs.len)
        return lhs.len < rhs.len;

    return ::memcmp(lhs.key, rhs.key, lhs.len) < 0;
}

inline bool operator==(const Ini::Key &lhs, const Ini::Key &rhs)
{
    return lhs.len == rhs.len && 0 == ::memcmp(lhs.key, rhs.key, lhs.len);
}

Ini::Contents Ini::parseIni(const char * mem, unsigned int length)
{
    Contents rv;

    /* Format:

       <white>[<group>]<white><newline>
       <white><key><white>=<white><rawvalue><white><newline>

       */

    const char * const end = mem + length;
    enum { FreshLine, /* Nothing but white space has been seen since the lasta
                         newline */
           UntilFreshLine, /* Advance until a new line is seen */
           Group, /* Reading group tag until next ']' */
           Key  /* Reading key until next '=' */
           } state = FreshLine;

    char * currentGroup = 0;
    unsigned int currentGroupLen = 0;

    while(mem < end) {

        if(UntilFreshLine == state) {

            while('\n' != *mem && '\t' != *mem) {
                ++mem;
                if(mem >= end)
                    return rv;
            }
            ++mem;
            state = FreshLine;

        } else if(FreshLine == state) {

            while(' ' == *mem || '\t' == *mem ||
                  '\n' == *mem  || '\r' == *mem) {
                ++mem;
                if(mem >= end)
                    return rv;
            }

            if('[' == *mem) {
                state = Group;
                ++mem;
            } else if(';' == *mem) {
                state = UntilFreshLine;
                ++mem;
            } else {
                state = Key;
            }

        } else if(Group == state) {

            const char * groupStart = mem;
            while(']' != *mem && '\n' != *mem && '\r' != *mem) {
                ++mem;
                if(mem >= end)
                    return rv;
            }

            if(']' == *mem) {
                if(currentGroup)
                    delete [] currentGroup;

                if(*groupStart == '/')
                    ++groupStart;

                currentGroupLen = mem - groupStart;
                currentGroup = new char[currentGroupLen];
                for(int src = 0, dst = 0; src < mem - groupStart; ++dst){
                    const char val = groupStart[src];
                    if('%' == val) {
                        if(src + 2 < mem - groupStart)
                            currentGroup[dst] = fromhex(groupStart + src + 1);
                        src += 3;
                        currentGroupLen =
                            (currentGroupLen > 2)?(currentGroupLen - 2):0;
                    } else {
                        currentGroup[dst] = groupStart[src];
                        ++src;
                    }
                }

                ++mem;
                state = UntilFreshLine;
            } else {
                return rv;
            }

        } else if(Key == state) {

            const char * const keyStart = mem;
            const char * keyEnd = 0; // Guarenteed to be set in the first iteration
                                     // of do {} while() loop below.
            do {
                if(*mem != ' ' && *mem != '\t')
                    keyEnd = mem;
                else if(*mem == '\n' || *mem == '\r')
                    return rv;

                ++mem;
                if(mem >= end)
                    return rv;
            } while(*mem != '=');

            // Find value start
            do {
                ++mem;
                if(mem >= end)
                    return rv;
            } while(*mem == ' ' || *mem == '\t');

            const char * const valStart = mem;
            const char * valEnd = 0;
            while(mem < end && *mem != '\n' && *mem != '\r') {
                if(*mem != ' ')
                    valEnd = mem;
                ++mem;
            }

            Ini::Key key;
            bool translatable = false;
            key.len = currentGroupLen + (currentGroupLen?1:0) + (keyEnd - keyStart + 1);
            key.key = new char [key.len];
            if(currentGroupLen) {
                ::memcpy(key.key, currentGroup, currentGroupLen);
                key.key[currentGroupLen] = '/';
            }
            // We don't support case insensitive matching
            char * kd = key.key + currentGroupLen + (currentGroupLen?1:0);
            for(int src = 0, dst = 0; src < (keyEnd - keyStart + 1); ++dst){
                const char val = keyStart[src];
                if('%' == val) {
                    if(src + 2 < (keyEnd - keyStart + 1))
                        kd[dst] = fromhex(keyStart + src + 1);
                    src += 3;
                    key.len = (key.len > 2)?(key.len - 2):0;
                } else if('[' == val && src == (keyEnd - keyStart - 1) &&
                          keyStart[src + 1] == ']') {
                    translatable = true;
                    key.len -= 2;
                    src += 2;
                } else {
                    kd[dst] = keyStart[src];
                    ++src;
                }
            }

            // Copy raw key value into structure - will convert to qvariant on
            // demand
            Ini::Value ival;
            ival.type =
                translatable?Ini::Value::RawTranslatable:Ini::Value::Raw;
            ival.len = (valEnd == 0)?0:(valEnd - valStart + 1);
            ival.rawValue = new char[ival.len];
            if(ival.len)
                ::memcpy(ival.rawValue, valStart, ival.len);

            rv.cost += key.len + ival.len + 2 * sizeof(unsigned int);
            rv.contents.insert(key, ival);

            state = FreshLine;
        }
    }

    if(currentGroup)
        delete [] currentGroup;

    return rv;
}

//
// Begin from qsettings.cpp
//
static const char hexDigits[] = "0123456789ABCDEF";

QString &Ini::escapedLeadingAt(QString &s)
{
    if (s.length() > 0 && s.at(0) == QLatin1Char('@'))
        s.prepend(QLatin1Char('@'));
    return s;
}
QString Ini::variantToString(const QVariant &v)
{
    QString result;

    switch (v.type()) {
        case QVariant::Invalid:
            result = QLatin1String("@Invalid()");
            break;

        case QVariant::ByteArray: {
            QByteArray a = v.toByteArray();
            result = QLatin1String("@ByteArray(");
            result += QString::fromLatin1(a.constData(), a.size());
            result += QLatin1Char(')');
            break;
        }

        case QVariant::String:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::Bool:
        case QVariant::Double:
        case QVariant::KeySequence: {
            result = v.toString();
            result = escapedLeadingAt(result);
            break;
        }
#ifndef QT_NO_GEOM_VARIANT
        case QVariant::Rect: {
            QRect r = qvariant_cast<QRect>(v);
            result += QLatin1String("@Rect(");
            result += QString::number(r.x());
            result += QLatin1Char(' ');
            result += QString::number(r.y());
            result += QLatin1Char(' ');
            result += QString::number(r.width());
            result += QLatin1Char(' ');
            result += QString::number(r.height());
            result += QLatin1Char(')');
            break;
        }
        case QVariant::Size: {
            QSize s = qvariant_cast<QSize>(v);
            result += QLatin1String("@Size(");
            result += QString::number(s.width());
            result += QLatin1Char(' ');
            result += QString::number(s.height());
            result += QLatin1Char(')');
            break;
        }
        case QVariant::Point: {
            QPoint p = qvariant_cast<QPoint>(v);
            result += QLatin1String("@Point(");
            result += QString::number(p.x());
            result += QLatin1Char(' ');
            result += QString::number(p.y());
            result += QLatin1Char(')');
            break;
        }
#endif // !QT_NO_GEOM_VARIANT

        default: {
#ifndef QT_NO_DATASTREAM
            QByteArray a;
            {
                QDataStream s(&a, QIODevice::WriteOnly);
                s << v;
            }

            result = QLatin1String("@Variant(");
            result += QString::fromLatin1(a.constData(), a.size());
            result += QLatin1Char(')');
#else
            Q_ASSERT("QSettings: Cannot save custom types without QDataStream support");
#endif
            break;
        }
    }

    return result;
}

QStringList Ini::variantListToStringList(const QVariantList &l)
{
    QStringList result;
    QVariantList::const_iterator it = l.constBegin();
    for (; it != l.constEnd(); ++it)
        result.append(variantToString(*it));
    return result;
}

QVariant Ini::stringListToVariantList(const QStringList &l)
{
    QVariantList variantList;
    bool foundNonStringItem = false;
    bool foundEscapedStringItem = false;

    QStringList::const_iterator it = l.constBegin();
    for (; it != l.constEnd(); ++it) {
        QVariant variant = stringToVariant(*it);
        variantList.append(variant);

        if (variant.type() != QVariant::String)
            foundNonStringItem = true;
        else if (variant.toString() != *it)
            foundEscapedStringItem = true;
    }

    if (foundNonStringItem) {
        return variantList;
    } else if (foundEscapedStringItem) {
        return QVariant(variantList).toStringList();
    }
    return l;
}

void Ini::iniEscapedStringList(const QStringList &strs, QByteArray &result)
{
    if (strs.isEmpty()) {
        /*
            We need to distinguish between empty lists and one-item
            lists that contain an empty string. Ideally, we'd have a
            @EmptyList() symbol but that would break compatibility
            with Qt 4.0. @Invalid() stands for QVariant(), and
            QVariant().toStringList() returns an empty QStringList,
            so we're in good shape.
        */
        result += "@Invalid()";
    } else {
        for (int i = 0; i < strs.size(); ++i) {
            if (i != 0)
                result += ", ";
            iniEscapedString(strs.at(i), result);
        }
    }
}
void Ini::iniEscapedString(const QString &str, QByteArray &result)
{
    bool needsQuotes = false;
    bool escapeNextIfDigit = false;
    int i;
    int startPos = result.size();

    result.reserve(startPos+str.size()*3/2);
    for (i = 0; i < str.size(); ++i) {
        uint ch = str.at(i).unicode();
        if (ch == ';' || ch == ',' || ch == '=')
            needsQuotes = true;

        if (escapeNextIfDigit
                && ((ch >= '0' && ch <= '9')
                    || (ch >= 'a' && ch <= 'f')
                    || (ch >= 'A' && ch <= 'F'))) {
            result += "\\x";
            result += QByteArray::number(ch, 16);
            continue;
        }

        escapeNextIfDigit = false;

        switch (ch) {
        case '\0':
            result += "\\0";
            escapeNextIfDigit = true;
            break;
        case '\a':
            result += "\\a";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\v':
            result += "\\v";
            break;
        case '"':
        case '\\':
            result += '\\';
            result += (char)ch;
            break;
        default:
            if (ch <= 0x1F || ch >= 0x7F) {
                result += "\\x";
                result += QByteArray::number(ch, 16);
                escapeNextIfDigit = true;
            } else {
                result += (char)ch;
            }
        }
    }

    if (needsQuotes
            || (startPos < result.size() && (result.at(startPos) == ' '
                                                || result.at(result.size() - 1) == ' '))) {
        result.insert(startPos, '"');
        result += '"';
    }
}
QString &Ini::unescapedLeadingAt(QString &s)
{
    if (s.startsWith(QLatin1String("@@")))
        s.remove(0, 1);
    return s;
}

QStringList Ini::splitArgs(const QString &s, int idx)
{
    int l = s.length();
    Q_ASSERT(l > 0);
    Q_ASSERT(s.at(idx) == QLatin1Char('('));
    Q_ASSERT(s.at(l - 1) == QLatin1Char(')'));

    QStringList result;
    QString item;

    for (++idx; idx < l; ++idx) {
        QChar c = s.at(idx);
        if (c == QLatin1Char(')')) {
            Q_ASSERT(idx == l - 1);
            result.append(item);
        } else if (c == QLatin1Char(' ')) {
            result.append(item);
            item.clear();
        } else {
            item.append(c);
        }
    }

    return result;
}
QVariant Ini::stringToVariant(const QString &s)
{
    if (s.length() > 3
            && s.at(0) == QLatin1Char('@')
            && s.at(s.length() - 1) == QLatin1Char(')')) {

        if (s.startsWith(QLatin1String("@ByteArray("))) {
            return QVariant(s.toLatin1().mid(11, s.size() - 12));
        } else if (s.startsWith(QLatin1String("@Variant("))) {
#ifndef QT_NO_DATASTREAM
            QByteArray a(s.toLatin1().mid(9));
            QDataStream stream(&a, QIODevice::ReadOnly);
            QVariant result;
            stream >> result;
            return result;
#else
            Q_ASSERT("Ini: Cannot load custom types without QDataStream support");
#endif
#ifndef QT_NO_GEOM_VARIANT
        } else if (s.startsWith(QLatin1String("@Rect("))) {
            QStringList args = Ini::splitArgs(s, 5);
            if (args.size() == 4) {
                return QVariant(QRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt()));
            }
        } else if (s.startsWith(QLatin1String("@Size("))) {
            QStringList args = Ini::splitArgs(s, 5);
            if (args.size() == 2) {
                return QVariant(QSize(args[0].toInt(), args[1].toInt()));
            }
        } else if (s.startsWith(QLatin1String("@Point("))) {
            QStringList args = Ini::splitArgs(s, 6);
            if (args.size() == 2) {
                return QVariant(QPoint(args[0].toInt(), args[1].toInt()));
            }
#endif
        } else if (s == QLatin1String("@Invalid()")) {
            return QVariant();
        }
    }

    QString tmp = s;
    return QVariant(unescapedLeadingAt(tmp));
}

QStringList *Ini::unescapedStringList(const char *str, const char *to,
                                          QString &result)
{
    static const char escapeCodes[][2] =
    {
        { 'a', '\a' },
        { 'b', '\b' },
        { 'f', '\f' },
        { 'n', '\n' },
        { 'r', '\r' },
        { 't', '\t' },
        { 'v', '\v' },
        { '"', '"' },
        { '?', '?' },
        { '\'', '\'' },
        { '\\', '\\' }
    };
    static const int numEscapeCodes = sizeof(escapeCodes) / sizeof(escapeCodes[0]);

    QStringList *strList = 0;
    enum State { StNormal,
                 StSkipSpaces,
                 StEscape,
                 StHexEscapeFirstChar,
                 StHexEscape,
                 StOctEscape };
    State state = StSkipSpaces;

    int escapeVal = 0;
    bool inQuotedString = false;
    bool currentValueIsQuoted = false;

    while (str < to) {
        char ch = *str;

        switch (state) {
        case StNormal:
            switch (ch) {
            case '\\':
                state = StEscape;
                break;
            case '"':
                currentValueIsQuoted = true;
                inQuotedString = !inQuotedString;
                if (!inQuotedString)
                    state = StSkipSpaces;
                break;
            case ',':
                if (!inQuotedString) {
#if AKENNEDY
                    // Wouldn't we be in StSkipSpaces mode anyway?
                    if (!currentValueIsQuoted)
                        iniChopTrailingSpaces(&result);
#endif
                    if (!strList)
                        strList = new QStringList;
                    strList->append(result);
                    result.clear();
                    currentValueIsQuoted = false;
                    state = StSkipSpaces;
                    break;
                }
                // fallthrough
            default:
                result += QLatin1Char(ch);
            }
            ++str;
            break;
        case StSkipSpaces:
            if (ch == ' ' || ch == '\t')
                ++str;
            else
                state = StNormal;
            break;
        case StEscape:
            for (int j = 0; j < numEscapeCodes; ++j) {
                if (ch == escapeCodes[j][0]) {
                    result += QLatin1Char(escapeCodes[j][1]);
                    ++str;
                    state = StNormal;
                    goto end_of_switch;
                }
            }

            if (ch == 'x') {
                escapeVal = 0;
                state = StHexEscapeFirstChar;
            } else if (ch >= '0' && ch <= '7') {
                escapeVal = ch - '0';
                state = StOctEscape;
            } else {
                state = StNormal;
            }
            ++str;
            break;
        case StHexEscapeFirstChar:
            if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')
                    || (ch >= 'a' && ch <= 'f'))
                state = StHexEscape;
            else
                state = StNormal;
            break;
        case StHexEscape:
            if (ch >= 'a')
                ch -= 'a' - 'A';
            if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) {
                escapeVal <<= 4;
                escapeVal += strchr(hexDigits, ch) - hexDigits;
                ++str;
            } else {
                result += QChar(escapeVal);
                state = StNormal;
            }
            break;
        case StOctEscape:
            if (ch >= '0' && ch <= '7') {
                escapeVal <<= 3;
                escapeVal += ch - '0';
                ++str;
            } else {
                result += QChar(escapeVal);
                state = StNormal;
            }
        }
end_of_switch:
        ;
    }

    if (state == StHexEscape || state == StOctEscape)
        result += QChar(escapeVal);
#ifdef AKENNEDY
    if (!currentValueIsQuoted)
        iniChopTrailingSpaces(&result);
#endif
    if (strList)
        strList->append(result);
    return strList;
}
//
// End from qsettings.cpp
//

bool Ini::save(const Contents & contents, const QByteArray &file)
{
    qWarning("Saving to  '%s'", file.constData());
    // XXX - i18n and error condition unaware

    int fd = ::open(file.constData(), O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP);
    if(-1 == fd) {
        qWarning("XX1");
        return false;
    }

    typedef QMap<QByteArray, Ini::Value> SectionMap;
    typedef QMap<QByteArray, SectionMap> IniMap;
    IniMap iniMap;

    for(QMap<Ini::Key, Ini::Value>::ConstIterator iter =
            contents.contents.begin();
            iter != contents.contents.end();
            ++iter) {

        QByteArray section;
        QByteArray key(iter.key().key, iter.key().len);

        int slashPos;
        if ((slashPos = key.indexOf('/')) != -1) {
            section = key.left(slashPos);
            key.remove(0, slashPos + 1);
        }
        iniMap[section][key] = iter.value();
    }

    ::ftruncate(fd, 0);

    for(IniMap::ConstIterator iter = iniMap.begin();
            iter != iniMap.end();
            ++iter) {

        QByteArray section = iter.key();
        if(!section.isEmpty()) {
            section.prepend('[');
            section.append(']');
            section.append('\n');
            write(fd, section.constData(), section.length());
        }

        for(SectionMap::ConstIterator siter = iter->begin();
                siter != iter->end();
                ++siter) {

            QByteArray block(siter.key());
            block.append('=');

            if(Ini::Value::Variant == siter->type ) {
                if(siter->var->type() == QVariant::StringList ||
                   siter->var->type() == QVariant::List) {
                    iniEscapedStringList(variantListToStringList(siter->var->toList()), block);
                } else {
                    iniEscapedString(variantToString(*siter->var), block);
                }
            } else {
                block.append(QByteArray(siter->rawValue, siter->len));
            }
            block.append('\n');
            write(fd, block.constData(), block.length());
        }
    }

    ::close(fd);
    return true;
}

bool Ini::load(Contents & contents, const QByteArray &file)
{
    int fd = ::open(file.constData(), O_RDONLY);
    if(-1 == fd)
        return false;

    struct stat statbuf;
    if(-1 == ::fstat(fd, &statbuf)) {
        ::close(fd);
        return false;
    }

    void * mem = ::mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(MAP_FAILED == mem) {
        ::close(fd);
        return false;
    }

    if(!contents.contents.isEmpty())
        Ini::clear(&contents.contents);
    contents = Ini::parseIni((const char *)mem, statbuf.st_size);

    ::munmap(mem, statbuf.st_size);
    ::close(fd);

    return true;
}

void Ini::clear(QMap<Ini::Key, Ini::Value> * data)
{
    for(QMap<Ini::Key, Ini::Value>::ConstIterator iter = data->begin();
            iter != data->end();
            ++iter) {
        delete [] iter.key().key;
        if(iter.value().rawValue)
            delete [] iter.value().rawValue;
        if(iter.value().var)
            delete iter.value().var;
    }
    data->clear();
}

void Ini::toVariant(Ini::Value & val)
{
    if(val.type == Ini::Value::Variant)
        return;

    if(0 == val.len) {
        val.var = new QVariant();
    } else {
        QString strValue;
        strValue.reserve(val.len);
        QStringList *strListValue = unescapedStringList(val.rawValue,
                val.rawValue + val.len,
                strValue);
        if (strListValue) {
            val.var = new QVariant(stringListToVariantList(*strListValue));
            delete strListValue;
        } else {
            val.var = new QVariant(stringToVariant(strValue));
        }
    }
    val.type = Ini::Value::Variant;
    delete [] val.rawValue;
    val.rawValue = 0;
}

// declare IniLayer
class IniInstance;
class IniLayerTermData;
class IniLayer : public IValueSpaceLayer
{
Q_OBJECT
public:
    IniLayer();
    virtual ~IniLayer();

    // IValueSpaceLayer
    virtual QByteArray name(HANDLE handle);
    virtual bool value(HANDLE, QVariant *);
    virtual bool value(HANDLE, const QByteArray &, QVariant *);
    virtual QSet<QByteArray> children(HANDLE);
    virtual HANDLE item(HANDLE handle, const QByteArray &);
    virtual void remHandle(HANDLE);
    virtual void setProperty(HANDLE handle, Properties);
    virtual QString name() { return "Ini Layer"; }
    virtual bool startup(Type) { return true; }
    virtual bool restart() { return true; }
    virtual void shutdown() {}
    virtual QUuid id() {
        return QUuid(0x2ce1f9aa, 0x2897, 0x455f,
                     0xb9, 0x66, 0x42, 0x8c, 0x54, 0x8f, 0xb0, 0x21);
    }
    virtual unsigned int order() { return 0; }
    virtual bool remove(HANDLE);
    virtual bool remove(HANDLE, const QByteArray &);
    virtual bool setValue(HANDLE, const QVariant &);
    virtual bool setValue(HANDLE, const QByteArray &, const QVariant &);
    virtual bool syncChanges();

    // Other
    static IniLayer * instance();

signals:
    void languageChanged();

private slots:
    void languageMaybeChanged();
    void aboutToQuit();

private:
    bool initLanguage();

    // declare IniLayer::SearchBranch
    struct SearchNode;
    typedef QMap<QByteArray, SearchNode *> SearchBranch;
    // declare IniLayer::SearchTerm
    struct SearchTerm
    {
        SearchTerm() : depth(0) {}
        int depth; // -1 (exact), 0 (segment is file name), 1+ (paths)
        QList<QByteArray> searchPaths;
        QByteArray fileExtension;
    };
    // declare IniLayer::SearchNode
    struct SearchNode
    {
        SearchNode() : terminal(0), branch(0) {}
        SearchNode(SearchTerm * t) : terminal(t), branch(0) {}
        SearchNode(SearchBranch * b) : terminal(0), branch(b) {}

        SearchTerm * terminal;
        SearchBranch * branch;
    };
    SearchNode searchRoot;
    void initSearchTree();
    void destroySearchTree(SearchNode * node = 0);

    // declare IniLayer::ReadHandle
    // define IniLayer::ReadHandle
    struct ReadHandle {
        ReadHandle() : tcPathStart(-1),
                       tcPathSegs(0),
                       stPathStart(-1),
                       data(0),
                       properties((IniLayer::Properties)0) {}
        ReadHandle(const ReadHandle &other) : path(other.path),
                                              node(other.node),
                                              tcPathStart(other.tcPathStart),
                                              tcPathSegs(other.tcPathSegs),
                                              stPathStart(other.stPathStart),
                                              data(other.data) {}
        ReadHandle & operator=(const ReadHandle &other) {
            path = other.path;
            node = other.node;
            tcPathStart = other.tcPathStart;
            tcPathSegs = other.tcPathSegs;
            stPathStart = other.tcPathStart;
            data = other.data;
            return *this;
        };

        /* Full path of the handle */
        QByteArray path;
        /* SearchNode for which path terminates */
        SearchNode node;
        /* Offset in path which begins the terminal completion (or -1 if NA) */
        int tcPathStart;
        /* Count of terminal completion segments in path */
        int tcPathSegs;
        /* Offset in path which begins the sub terminal path (or -1 if NA) */
        int stPathStart;
        /* File data */
        IniLayerTermData * data;
        /* Current properties */
        IniLayer::Properties properties;
    };

    bool advReadHandle(ReadHandle &, const QByteArray &);

    // declare IniLayer::TermId
    typedef QPair<Ini::Key, SearchTerm *> TermId;
    friend class IniLayerTermData;
    friend uint qHash(TermId);

    QSet<IniLayerTermData *> dirtyTerminals;
    IniLayerTermData * termData(TermId);
    QMap<TermId, IniLayerTermData *> activeTerminals;
    QCache<TermId, IniLayerTermData> cachedTerminals;

    // i18n stuff
    QString currentLanguagePath;
    QByteArray currentLanguage;
    QValueSpaceItem * language;
    QString translatorPath;
    QByteArray languageItem;
};

uint qHash(IniLayer::TermId id)
{
    const uchar *p = (uchar *)id.first.key;
    int n = id.first.len;
    uint h = (uint)(ulong)id.second;
    uint g;

    while (n--) {
        h = (h << 4) + *p++;
        if ((g = (h & 0xf0000000)) != 0)
            h ^= g >> 23;
        h &= ~g;
    }
    return h;
}

// declare IniLayerTermData
// Must be separate class as moc doesn't support nested classes.  Logically part
// of IniLayer, though.
class IniLayerTermData : public QObject
{
    Q_OBJECT
public:
    IniLayerTermData(IniLayer *, IniLayer::TermId _me);
    ~IniLayerTermData();

    bool contentsDirty;
    Ini::Contents activeContents;
    QByteArray translationContext;
    QTranslator * translator();

    void AddRef();
    void Release();

    IniLayer::TermId id; // Mainly for cache memory management.  *Must* be self
                         // contined as the Ini::Key is deleted when the
                         // IniLayerTermData is.

    void addHandle(IniLayer::HANDLE h) { publishHandles.insert(h); }
    void remHandle(IniLayer::HANDLE h) { publishHandles.remove(h); }

    QByteArray writeFile();

public slots:
    void refresh();

private slots:
    void fileChanged(const QString &);

private:
    QTranslator * _translator;
    QByteArray postfilename;
    QList<QFileMonitor *> termMonitors;
    void syncActive(int);
    QByteArray filename(int);
    unsigned int refCount;
    IniLayer * layer;
    QSet<IniLayer::HANDLE> publishHandles;
};

// define IniLayerTermData
QTranslator * IniLayerTermData::translator()
{
    static const Ini::Key fileKey = { "Translation/File", 16 };
    static const Ini::Key contextKey = { "Translation/Context", 19 };

    if(_translator) return _translator;

    _translator = new QTranslator();

    QMap<Ini::Key, Ini::Value>::Iterator file = activeContents.contents.find(fileKey);
    QMap<Ini::Key, Ini::Value>::Iterator context = activeContents.contents.find(contextKey);
    if(file == activeContents.contents.end() ||
       context == activeContents.contents.end())
        return _translator; // Invalid translation information

    toVariant(*file);
    toVariant(*context);

    if(!_translator->load(file->var->toString(), layer->currentLanguagePath)) {
        qLog(IniLayer) << "Unable to load translation file:" << file->var->toString().toAscii();
    } else
        translationContext = context->var->toByteArray();

    QObject::connect(layer, SIGNAL(languageChanged()), this, SLOT(refresh()));
    return _translator;
}

/*!
  Does *NOT* own _me (will make an internally owned copy, though)
  */
IniLayerTermData::IniLayerTermData(IniLayer *l, IniLayer::TermId _me)
: contentsDirty(false), _translator(0), refCount(1), layer(l)
{
    Q_ASSERT(_me.second && (-1 == _me.second->depth || _me.first.key));

    if(_me.first.key) {
        id.first.key = new char[_me.first.len];
        id.first.len = _me.first.len;
        ::memcpy(id.first.key, _me.first.key, id.first.len);

        postfilename.append("/");
        postfilename.append(QByteArray(_me.first.key, _me.first.len));
        postfilename.append(".");
        postfilename.append(_me.second->fileExtension);
    }
    id.second = _me.second;

    syncActive(0);
}

IniLayerTermData::~IniLayerTermData()
{
    Q_ASSERT(0 == refCount);
    delete [] id.first.key;
    qDeleteAll(termMonitors);
    Ini::clear(&activeContents.contents);
    if(_translator)
        delete _translator;
}

void IniLayerTermData::AddRef()
{
    ++refCount;
}

void IniLayerTermData::Release()
{
    Q_ASSERT(0 != refCount);
    --refCount;
    if(!refCount) {
        Q_ASSERT(publishHandles.isEmpty());
        layer->activeTerminals.remove(id);
        Q_ASSERT(!layer->cachedTerminals.object(id));
        layer->cachedTerminals.insert(id, this, activeContents.cost);
    }
}

QByteArray IniLayerTermData::writeFile()
{
    return filename(0);
}

QByteArray IniLayerTermData::filename(int offset)
{
    Q_ASSERT(offset < id.second->searchPaths.count());

    return id.second->searchPaths.at(offset) + postfilename;
}

void IniLayerTermData::syncActive(int from)
{
    Q_ASSERT(from < id.second->searchPaths.count());
    Q_ASSERT(from <= termMonitors.count());
    Ini::clear(&activeContents.contents);
    if(_translator) {
        delete _translator;
        _translator = 0;
    }

    Q_ASSERT(from <= termMonitors.count());

    int found = -1;
    for(int ii = from; ii < termMonitors.count(); ++ii) {
        if(-1 != found) {
            delete termMonitors[ii];
        } else {
            QByteArray file = filename(ii);
            if(Ini::load(activeContents, file))
                found = ii;
        }
    }

    if(-1 != found) {
        for(int ii = found + 1; ii < termMonitors.count(); ++ii)
            termMonitors.removeLast();
        return;
    }

    for(int ii = termMonitors.count();
        ii < id.second->searchPaths.count();
        ++ii) {
        QByteArray file = filename(ii);
        QFileMonitor * monitor = new QFileMonitor(file);
        termMonitors.append(monitor);
        QObject::connect(monitor, SIGNAL(fileChanged(QString)),
                         this, SLOT(fileChanged(QString)));
        if(Ini::load(activeContents, file))
            return;
    }
}

void IniLayerTermData::refresh()
{
    if(!_translator)
        return;

    syncActive(termMonitors.count() - 1);
    contentsDirty = false;
    for(QSet<IniLayer::HANDLE>::ConstIterator iter = publishHandles.begin();
            iter != publishHandles.end();
            ++iter)
        emit layer->handleChanged(*iter);
}

void IniLayerTermData::fileChanged(const QString &)
{
    if(contentsDirty)
        return; // A change is in progress, we don't care what happened under us

    QFileMonitor * changed = (QFileMonitor *)sender();

    // Locate file monitor that changed
    bool found = false;
    for(int ii = 0; !found && ii < termMonitors.count(); ++ii) {
        if(termMonitors.at(ii) == changed) {
            syncActive(ii);
            found = true;
        }
    }
    Q_ASSERT(found && "IniLayer: Could not find file monitor that changed!");

    for(QSet<IniLayer::HANDLE>::ConstIterator iter = publishHandles.begin();
            iter != publishHandles.end();
            ++iter)
        emit layer->handleChanged(*iter);
}

// define IniLayer
IniLayer::IniLayer()
: cachedTerminals(8192), language(0)
{
    initSearchTree();
}

IniLayer::~IniLayer()
{
    destroySearchTree();
    if(language)
        delete language;
}

QByteArray IniLayer::name(HANDLE h)
{
    ReadHandle * handle = (ReadHandle *)h;
    return handle->path;
}

bool IniLayer::value(HANDLE h, QVariant *out)
{
    ReadHandle * handle = (ReadHandle *)h;
    if(!handle->data)
        return false;

    if(handle->stPathStart == -1)
        return false;

    Q_ASSERT(handle->data);
    Ini::Key key;
    key.key = (char *)(handle->path.constData() + handle->stPathStart);
    key.len = handle->path.length() - handle->stPathStart;
    if('/' == *key.key) {
        ++key.key;
        --key.len;
    }
    QMap<Ini::Key, Ini::Value>::Iterator iter =
        handle->data->activeContents.contents.find(key);
    if(iter == handle->data->activeContents.contents.end())
        return false;

    bool needTranslation = Ini::Value::RawTranslatable == iter->type;
    Ini::toVariant(*iter);
    if(needTranslation && initLanguage()) {
        // This needs translation!
        QTranslator * translator = handle->data->translator();
        if(translator) {
            *iter->var = translator->translate(handle->data->translationContext.constData(),
                    iter->var->toByteArray().constData());
        }
    }

    *out = *iter->var;
    return true;
}

bool IniLayer::value(HANDLE h, const QByteArray &key, QVariant *out)
{
    ReadHandle * handle = (ReadHandle *)h;
    if(key.isEmpty()) {
        return value(h, out);
    } else if(*key.constData() == '/') {
        ReadHandle rvhandle = *handle;
        if(!advReadHandle(rvhandle, key))
            return false;
        bool rv = value((HANDLE)&rvhandle, out);
        rvhandle.data->Release();
        return rv;
    } else {
        ReadHandle rvhandle = *handle;
        if(!advReadHandle(rvhandle, QByteArray("/") + key))
            return false;
        bool rv = value((HANDLE)&rvhandle, out);
        rvhandle.data->Release();
        return rv;
    }
}

QSet<QByteArray> IniLayer::children(HANDLE h)
{
    ReadHandle * handle = (ReadHandle *)h;
    QSet<QByteArray> rv;

    // Children come from:
    //    1) Branches in current non-terminal
    //    2) Ini file in satisfied terminal
    //    3) Directories or files in non-satisfied terminal

    // Branches in current non-terminal
    if(handle->node.branch) {
        for(SearchBranch::ConstIterator iter = handle->node.branch->begin();
                iter != handle->node.branch->end();
                ++iter)
            rv.insert(iter.key());
    }

    if(handle->data) {

        Q_ASSERT(-1 != handle->stPathStart);

        Ini::Key prekey;
        prekey.key = (char *)handle->path.constData() + handle->stPathStart;
        prekey.len = handle->path.length() - handle->stPathStart;
        if('/' == *prekey.key) { ++prekey.key; --prekey.len; }

        // Ini file in satisfied terminal
        for(QMap<Ini::Key, Ini::Value>::ConstIterator iter =
                handle->data->activeContents.contents.begin();
                iter != handle->data->activeContents.contents.end();
                ++iter) {

            const Ini::Key & curkey = iter.key();
            if(curkey.len > prekey.len &&
               (0 == prekey.len || '/' == *(curkey.key + prekey.len)) &&
               0 == ::memcmp(curkey.key, prekey.key, prekey.len)) {

                // Match, locate the segment end;
                const char * ptr = curkey.key + prekey.len + (prekey.len?1:0);
                const char * keyptr = ptr;
                while(keyptr < (curkey.key + curkey.len) &&
                      '/' != *keyptr && '\0' != *keyptr)
                    ++keyptr;
                rv.insert(QByteArray(ptr, keyptr - ptr));
            }
        }

    } else if(handle->node.terminal) {
        // Directories or files in non-satisfied terminal
        Q_ASSERT(-1 != handle->node.terminal->depth);

        QByteArray postpath("/");
        if(-1 != handle->tcPathStart)
            postpath.append(handle->path.mid(handle->tcPathStart));

        if(handle->tcPathSegs == handle->node.terminal->depth) {
            // Looking for files
            QStringList filters;
            filters.append("*." + handle->node.terminal->fileExtension);
            for(int ii = 0;
                ii < handle->node.terminal->searchPaths.count();
                ++ii) {

                QDir dir(handle->node.terminal->searchPaths.at(ii) + postpath);
                QStringList entrylist = dir.entryList(filters, QDir::Files);
                for(int ii = 0; ii < entrylist.count(); ++ii) {
                    QByteArray file = entrylist.at(ii).toAscii();
                    rv.insert(file.left(file.length() - handle->node.terminal->fileExtension.length() - 1));
                }

            }
        } else {
            // Looking for directories
            for(int ii = 0;
                ii < handle->node.terminal->searchPaths.count();
                ++ii) {

                QDir dir(handle->node.terminal->searchPaths.at(ii) + postpath);
                QStringList entrylist = dir.entryList(QDir::Dirs |
                                                      QDir::NoDotAndDotDot);
                for(int ii = 0; ii < entrylist.count(); ++ii)
                    rv.insert(entrylist.at(ii).toAscii());
            }

        }
    }

    return rv;
}

/*!
   Adds \a key to the handle position indicated by \a rvhandle.  Returns true
   if the new key (an updated rvhandle) is valid, false otherwise.

   Does NOT perform any INI file location etc. just advances the data members
   of \a rvhandle appropriately.
 */
bool IniLayer::advReadHandle(ReadHandle &rvhandle, const QByteArray &key)
{
    Q_ASSERT(!key.isEmpty());
    Q_ASSERT(key.constData()[0] == '/');
    Q_ASSERT(key.length() == 1 || key.constData()[key.length() - 1] != '/');

    const char * keydata = key.constData();

    const char * segstart = keydata + 1;
    const char * ptr = keydata + 1;
    if(!rvhandle.node.terminal && !rvhandle.node.branch)
        rvhandle.node = searchRoot;

    if('\0' == *ptr)
        return true;

    bool endOfString = false;
    while(!endOfString && -1 == rvhandle.stPathStart) {
        endOfString = (*ptr == '\0');

        if(*ptr == '/' || endOfString) {

            if(rvhandle.node.terminal && -1 != rvhandle.tcPathStart) {

                // We've already seen the terminal, and decided not to branch
                ++rvhandle.tcPathSegs;
                if(rvhandle.tcPathSegs == rvhandle.node.terminal->depth + 1) {
                    // Done!
                    rvhandle.stPathStart = (ptr - keydata) +
                                           rvhandle.path.length();
                }

            } else {

                if(rvhandle.node.branch) {

                    // Does this segment branch deeper?
                    QByteArray seg = QByteArray(segstart, ptr - segstart);
                    SearchBranch::ConstIterator iter =
                        rvhandle.node.branch->find(seg);
                    if(iter != rvhandle.node.branch->end()) {
                        // Yes, apparently
                        rvhandle.node = *(*iter);
                        ++ptr;
                        segstart = ptr;
                        continue;
                    }

                }

                if(rvhandle.node.terminal) {
                    if(-1 == rvhandle.node.terminal->depth) {
                        rvhandle.stPathStart = (segstart - keydata) +
                                               rvhandle.path.length();
                        rvhandle.tcPathStart = rvhandle.stPathStart;
                    } else {
                        rvhandle.tcPathStart = (segstart - keydata) +
                                               rvhandle.path.length();
                        rvhandle.tcPathSegs = 1;
                    }
                } else {
                    return false;
                }
            }
            segstart = ptr + 1;
        }
        ++ptr;
    }

    rvhandle.path.append(key);

    if(rvhandle.node.terminal &&
       -1 == rvhandle.node.terminal->depth &&
       -1 == rvhandle.tcPathStart) {
        rvhandle.stPathStart = rvhandle.path.length();
        rvhandle.tcPathStart = rvhandle.stPathStart;
    }

    if(rvhandle.node.terminal &&
       rvhandle.tcPathSegs == (rvhandle.node.terminal->depth + 1)) {
        TermId id;
        if(rvhandle.tcPathSegs) {
            id.first.key =
                (char *)(rvhandle.path.constData() + rvhandle.tcPathStart);
            id.first.len = rvhandle.stPathStart - rvhandle.tcPathStart;
        }
        id.second = rvhandle.node.terminal;
        rvhandle.data = termData(id);
        Q_ASSERT(rvhandle.data);
    }
    return true;
}

IniLayerTermData * IniLayer::termData(TermId tid)
{
    // Check active
    QMap<TermId, IniLayerTermData *>::Iterator iter = activeTerminals.find(tid);
    if(iter != activeTerminals.end()) {
        (*iter)->AddRef();
        return *iter;
    }

    // Check cache
    IniLayerTermData * cacheD = cachedTerminals.take(tid);
    if(cacheD) {
        cacheD->AddRef();
        activeTerminals.insert(cacheD->id, cacheD);
        return cacheD;
    }

    // Create a new one
    IniLayerTermData * newD = new IniLayerTermData(this, tid);
    activeTerminals.insert(newD->id, newD);
    return newD;
}

IValueSpaceLayer::HANDLE IniLayer::item(HANDLE h, const QByteArray &key)
{
    ReadHandle rvhandle;

    if(h != InvalidHandle)
        rvhandle = *(ReadHandle *)h;

    if(!advReadHandle(rvhandle, key))
        return InvalidHandle;

    return (HANDLE)(new ReadHandle(rvhandle));
}

void IniLayer::remHandle(HANDLE h)
{
    ReadHandle * handle = (ReadHandle *)h;
    if(handle->data) {
        if(handle->properties & Publish)
            handle->data->remHandle(h);
        handle->data->Release();
    }
    delete handle;
}

void IniLayer::setProperty(HANDLE h, Properties props)
{
    ReadHandle * handle = (ReadHandle *)h;
    if(!handle->data || props == handle->properties)
        return;

    handle->properties = props;
    if(props & Publish) {
        handle->data->addHandle(h);
    } else {
        handle->data->remHandle(h);
    }
}

bool IniLayer::remove(HANDLE h)
{
    ReadHandle * handle = (ReadHandle *)h;
    if(!handle->data)
        return false;

    if(handle->stPathStart == -1)
        return false;

    Q_ASSERT(handle->data);

    Ini::Key key;
    key.key = (char *)(handle->path.constData() + handle->stPathStart);
    key.len = handle->path.length() - handle->stPathStart;
    if('/' == *key.key) {
        ++key.key;
        --key.len;
    }

    for(QMap<Ini::Key, Ini::Value>::Iterator iter =
            handle->data->activeContents.contents.begin();
            iter != handle->data->activeContents.contents.end();
            ) {

        if(iter.key().len >= key.len &&
           0 == ::memcmp(iter.key().key, key.key, key.len)) {

            Ini::Key iKey = iter.key();
            Ini::Value iValue = iter.value();
            iter = handle->data->activeContents.contents.erase(iter);
            iKey.destroy();
            iValue.destroy();
        } else {
            ++iter;
        }
    }

    handle->data->contentsDirty = true;
    if(!dirtyTerminals.contains(handle->data)) {
        handle->data->AddRef();
        dirtyTerminals.insert(handle->data);
    }

    return true;
}

bool IniLayer::remove(HANDLE h, const QByteArray &key)
{
    ReadHandle * handle = (ReadHandle *)h;
    if(key.isEmpty()) {
        return remove(h);
    } else if(*key.constData() == '/') {
        ReadHandle rvhandle = *handle;
        if(!advReadHandle(rvhandle, key))
            return false;
        bool rv = remove((HANDLE)&rvhandle);
        rvhandle.data->Release();
        return rv;
    } else {
        ReadHandle rvhandle = *handle;
        if(!advReadHandle(rvhandle, QByteArray("/") + key))
            return false;
        bool rv = remove((HANDLE)&rvhandle);
        rvhandle.data->Release();
        return rv;
    }
}

bool IniLayer::setValue(HANDLE h, const QVariant &val)
{
    ReadHandle * handle = (ReadHandle *)h;
    if(!handle->data)
        return false;

    if(handle->stPathStart == -1)
        return false;

    Q_ASSERT(handle->data);
    Ini::Key key;
    key.key = (char *)(handle->path.constData() + handle->stPathStart);
    key.len = handle->path.length() - handle->stPathStart;
    if('/' == *key.key) {
        ++key.key;
        --key.len;
    }

    QMap<Ini::Key, Ini::Value>::Iterator iter =
        handle->data->activeContents.contents.find(key);

    if(iter == handle->data->activeContents.contents.end()) {
        Ini::Key newKey;
        newKey.len = key.len;
        newKey.key = new char[newKey.len];
        ::memcpy(newKey.key, key.key, newKey.len);

        Ini::Value newValue;
        newValue.type = Ini::Value::Variant;
        newValue.var = new QVariant(val);
        newValue.rawValue = 0;

        handle->data->activeContents.contents.insert(newKey, newValue);
    } else {
        if(Ini::Value::Variant == iter->type) {
            delete iter->var;
        } else {
            delete [] iter->rawValue;
            iter->rawValue = 0;
        }
        iter->type = Ini::Value::Variant;
        iter->var = new QVariant(val);
    }

    handle->data->contentsDirty = true;
    if(!dirtyTerminals.contains(handle->data)) {
        handle->data->AddRef();
        dirtyTerminals.insert(handle->data);
    }

    return true;
}

bool IniLayer::setValue(HANDLE h, const QByteArray &key,
                        const QVariant &val)
{
    ReadHandle * handle = (ReadHandle *)h;
    if(key.isEmpty()) {
        return setValue(h, val);
    } else if(*key.constData() == '/') {
        ReadHandle rvhandle = *handle;
        if(!advReadHandle(rvhandle, key))
            return false;
        bool rv = setValue((HANDLE)&rvhandle, val);
        rvhandle.data->Release();
        return rv;
    } else {
        ReadHandle rvhandle = *handle;
        if(!advReadHandle(rvhandle, QByteArray("/") + key))
            return false;
        bool rv = setValue((HANDLE)&rvhandle, val);
        rvhandle.data->Release();
        return rv;
    }
}

bool IniLayer::syncChanges()
{
    bool rv = true;

    QSet<IniLayerTermData *> dirty = dirtyTerminals;
    dirtyTerminals.clear();

    for(QSet<IniLayerTermData *>::ConstIterator iter = dirty.begin();
        iter != dirty.end(); ++iter) {

        if(!Ini::save((*iter)->activeContents, (*iter)->writeFile())) {
            qWarning("Unable to save!!");
            rv = false;
        }

        (*iter)->refresh();
        (*iter)->Release();
    }

    return rv;
}

void IniLayer::initSearchTree()
{
    // Makes me feel dirty, but so much easier to use QSettings here :)
    QSettings mapping("Trolltech", "IniValueSpace");

    unsigned int mappings = mapping.value("Mappings", 0).toUInt();
    translatorPath = mapping.value("Translations").toString();
    if(!translatorPath.isEmpty()) {
        languageItem = mapping.value("LanguageItem").toByteArray();
    }
    else {
        qLog(IniLayer) << "IniLayer: Translations not specified.  i18n disabled.";
    }

    unsigned int addedMappings = 0;

    for(unsigned int ii = 0; ii < mappings; ++ii) {
        QByteArray mappingName = "Mapping" + QByteArray::number(ii);
        mapping.beginGroup(mappingName);

        QByteArray vsp;
        QList<QByteArray> fsp;
        QByteArray fse;
        int rd;
        int fsps;
        int jj;
        SearchNode * currentNode;
        SearchNode * newNode;
        QList<QByteArray> vspSegs;

        vsp = mapping.value("ValueSpacePath").toByteArray();
        if(vsp.isEmpty() || vsp.endsWith("/") || !vsp.startsWith("/")) {
            qWarning("IniLayer: Ignoring mapping %d "
                     "(illegal ValueSpacePath: '%s')",
                     ii, vsp.constData());
            goto continue_loop;
        }

        fse = mapping.value("FileSystemExtension").toByteArray();
        rd = mapping.value("DirectoryDepth").toInt();
        if(rd < 0) {
            qWarning("IniLayer: Ignoring mapping %d "
                     "(cannot have negative DirectoryDepth)",
                     ii);
            goto continue_loop;
        }

        if(fse.isEmpty() && 0 != rd) {
            qWarning("IniLayer: Ignoring mapping %d "
                     "(cannot specify DirectoryDepth for absolute mapping)",
                     ii);
            goto continue_loop;
        }

        if(fse.isEmpty())
            rd = -1;

        fsps = mapping.value("FileSystemPaths", 0).toInt();
        if(fsps) {
            for(jj = 0; jj < fsps; ++jj) {
                fsp.append(mapping.value("FileSystemPath" +
                                         QByteArray::number(jj)).toByteArray());
                if(fsp.at(jj).isEmpty() || fsp.at(jj).endsWith("/")) {
                    qWarning("IniLayer: Ignoring mapping %d "
                             "(illegal FileSystemPath%d: '%s')",
                             ii, jj, fsp.at(jj).constData());
                    goto continue_loop;
                }
            }
        } else {
            fsp.append(mapping.value("FileSystemPath").toByteArray());
            if(fsp.at(0).isEmpty() || fsp.at(0).endsWith("/")) {
                qWarning("IniLayer: Ignoring mapping %d "
                         "(illegal FileSystemPath: '%s')",
                         ii, fsp.at(0).constData());
                goto continue_loop;
            }
        }

        // Locate the insertion point
        currentNode = &searchRoot;
        vspSegs = vsp.split('/');
        for(int jj = 1 /* Ignore empty first segment */;
            jj < vspSegs.count();
            ++jj) {

            if(!currentNode->branch)
                currentNode->branch = new SearchBranch;

            SearchBranch::Iterator iter =
                currentNode->branch->find(vspSegs.at(jj));
            if(iter == currentNode->branch->end()) {
                newNode = new SearchNode;
                currentNode->branch->insert(vspSegs.at(jj), newNode);
                currentNode = newNode;
                newNode = 0;
            } else {
                currentNode = *iter;
            }
        }
        Q_ASSERT(currentNode);
        if(currentNode->terminal) {
            qWarning("IniLayer: Ignoring mapping %d "
                     "(preexisting terminal at ValueSpacePath '%s')",
                     ii, vsp.constData());
            goto continue_loop;
        }

        currentNode->terminal = new SearchTerm;
        currentNode->terminal->depth = rd;
        currentNode->terminal->searchPaths = fsp;
        currentNode->terminal->fileExtension = fse;
        ++addedMappings;

continue_loop:
        mapping.endGroup();
    }

    qLog(IniLayer) << "Added" << addedMappings << "INI mappings";
}

void IniLayer::destroySearchTree(SearchNode * node)
{
    if(!node)
        node = &searchRoot;

    if(node->terminal) {
        delete node->terminal;
        node->terminal = 0;
    }

    if(node->branch) {
        for(SearchBranch::Iterator iter = node->branch->begin();
                iter != node->branch->end();
                ++iter) {

            destroySearchTree(*iter);
            delete *iter;
        }
        delete node->branch;
        node->branch = 0;
    }
}

void IniLayer::languageMaybeChanged()
{
    QByteArray newLanguage =
        language?language->value().toByteArray():QByteArray();
    if(newLanguage != currentLanguage) {
        currentLanguage = newLanguage;
        currentLanguagePath = translatorPath + "/" + currentLanguage;

        // Refresh all dependent ini files
        emit languageChanged();
    }
}

bool IniLayer::initLanguage()
{
    if(translatorPath.isEmpty()) return false;
    if(language) return true;

    static bool initing = false;

    if(initing)
        return false;
    initing = true;
    language = new QValueSpaceItem(languageItem);
    QObject::connect(language, SIGNAL(contentsChanged()),
                     this, SLOT(languageMaybeChanged()));
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
                     this, SLOT(aboutToQuit()));
    currentLanguagePath = translatorPath;
    initing = false;
    languageMaybeChanged();
    return true;
}

void IniLayer::aboutToQuit()
{
    if(language) {
        delete language;
        language = 0;
        languageMaybeChanged();
        QObject::disconnect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
                            this, SLOT(aboutToQuit()));
    }
}

Q_GLOBAL_STATIC(IniLayer, iniLayer);
IniLayer * IniLayer::instance()
{
    return iniLayer();
}
QVALUESPACE_AUTO_INSTALL_LAYER(IniLayer);

#include "inilayer.moc"
