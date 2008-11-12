/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QDATETIME_P_H
#define QDATETIME_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qplatformdefs.h"
#include "QtCore/qatomic.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qlist.h"

#define QDATETIMEEDIT_TIME_MIN QTime(0, 0, 0, 0)
#define QDATETIMEEDIT_TIME_MAX QTime(23, 59, 59, 999)
#define QDATETIMEEDIT_DATE_MIN QDate(100, 1, 1)
#define QDATETIMEEDIT_COMPAT_DATE_MIN QDate(1752, 9, 14)
#define QDATETIMEEDIT_DATE_MAX QDate(7999, 12, 31)
#define QDATETIMEEDIT_DATETIME_MIN QDateTime(QDATETIMEEDIT_DATE_MIN, QDATETIMEEDIT_TIME_MIN)
#define QDATETIMEEDIT_COMPAT_DATETIME_MIN QDateTime(QDATETIMEEDIT_COMPAT_DATE_MIN, QDATETIMEEDIT_TIME_MIN)
#define QDATETIMEEDIT_DATETIME_MAX QDateTime(QDATETIMEEDIT_DATE_MAX, QDATETIMEEDIT_TIME_MAX)
#define QDATETIMEEDIT_DATE_INITIAL QDate(2000, 1, 1)


class QDateTimePrivate
{
public:
    enum Spec { LocalUnknown = -1, LocalStandard = 0, LocalDST = 1, UTC = 2 };

    QDateTimePrivate() : ref(1), spec(LocalUnknown) {}
    QDateTimePrivate(const QDateTimePrivate &other)
        : ref(1), date(other.date), time(other.time), spec(other.spec)
    {}

    QAtomic ref;
    QDate date;
    QTime time;
    Spec spec;

    Spec getLocal(QDate &outDate, QTime &outTime) const;
    void getUTC(QDate &outDate, QTime &outTime) const;
    static QDateTime addMSecs(const QDateTime &dt, qint64 msecs);
};

#ifndef QT_BOOTSTRAPPED
#include "QtCore/qvariant.h"

class Q_CORE_EXPORT QDateTimeParser
{
public:
    QDateTimeParser(QVariant::Type t)
        : currentSectionIndex(-1), display(0), cachedDay(-1), typ(t), fixday(false), allowEmpty(true)
    {
        first.type = FirstSection;
        first.pos = -1;
        first.count = -1;
        last.type = FirstSection;
        last.pos = -1;
        last.count = -1;
        none.type = NoSection;
        none.pos = -1;
        none.count = -1;
    }
    virtual ~QDateTimeParser() {}
    enum {
        Neither = -1,
        AM = 0,
        PM = 1,
        PossibleAM = 2,
        PossiblePM = 3,
        PossibleBoth = 4
    };

    enum {
        NoSectionIndex = -1,
        FirstSectionIndex = -2,
        LastSectionIndex = -3
    };

    enum Section {
        NoSection = 0x0000,
        AmPmSection = 0x0001,
        MSecSection = 0x0002,
        SecondSection = 0x0004,
        MinuteSection = 0x0008,
        Hour12Section   = 0x0010,
        Hour24Section   = 0x0020,
        TimeSectionMask = (AmPmSection|MSecSection|SecondSection|MinuteSection|Hour12Section|Hour24Section),
        Internal = 0x8000,
        DaySection = 0x0100,
        MonthSection = 0x0200,
        YearSection = 0x0400,
        DateSectionMask = (DaySection|MonthSection|YearSection),
        FirstSection = 0x1000|Internal,
        LastSection = 0x2000|Internal
    }; // duplicated from qdatetimeedit.h
    Q_DECLARE_FLAGS(Sections, Section)

        struct SectionNode {
            Section type;
            mutable int pos;
            int count;
        };

    enum State { // duplicated from QValidator
        Invalid,
        Intermediate,
        Acceptable
    };

    struct StateNode {
        QString input;
        State state;
        bool conflicts;
        QVariant value;
    };

    enum AmPm {
        AmText,
        PmText
    };

    enum Case {
        UpperCase,
        LowerCase
    };

#ifndef QT_NO_DATESTRING
    StateNode parse(const QString &input, const QVariant &currentValue, bool fixup) const;
#endif
    int sectionMaxSize(int index) const;
    int sectionSize(int index) const;
    int sectionMaxSize(Section s, int count) const;
    int sectionPos(int index) const;
    int sectionPos(const SectionNode &sn) const;
    bool isSpecial(const QChar &c) const;

    SectionNode sectionNode(int index) const;
    Section sectionType(int index) const;
    QString sectionText(const QString &text, int sectionIndex, int index) const;
    int getDigit(const QVariant &dt, int index) const;
    void setDigit(QVariant &t, int index, int newval) const;
    int parseSection(const QVariant &currentValue, int sectionIndex, QString &txt, int index,
                     QDateTimeParser::State &state, int *used = 0) const;
    int absoluteMax(int index, const QDateTime &value = QDateTime()) const;
    int absoluteMin(int index) const;
    bool parseFormat(const QString &format);
#ifndef QT_NO_DATESTRING
    QDateTimeParser::State checkIntermediate(const QDateTime &dt, const QString &str) const;
    bool fromString(const QString &text, QDate *date, QTime *time) const;
#endif

#ifndef QT_NO_TEXTDATE
    int findMonth(const QString &str1, int monthstart, int sectionIndex,
                  QString *monthName = 0, int *used = 0) const;
    int findDay(const QString &str1, int intDaystart, int sectionIndex,
                QString *dayName = 0, int *used = 0) const;
#endif
    int findAmPm(QString &str1, int index, int *used = 0) const;
    int maxChange(int s) const;
    int potentialValue(const QString &str, int min, int max, int index, const QVariant &currentValue, int insert) const;
    int potentialValueHelper(const QString &str, int min, int max, int size, int insert) const;

    QString sectionName(int s) const;
    QString stateName(int s) const;

    QString sectionFormat(int index) const;
    QString sectionFormat(Section s, int count) const;

    enum FieldInfoFlag {
        Numeric = 0x01,
        FixedWidth = 0x02,
        AllowPartial = 0x04,
        Fraction = 0x08
    };
    Q_DECLARE_FLAGS(FieldInfo, FieldInfoFlag)

    FieldInfo fieldInfo(int index) const;

    virtual QVariant getMinimum() const;
    virtual QVariant getMaximum() const;
    virtual int cursorPosition() const { return -1; }
    virtual QString displayText() const { return text; }
    virtual QString getAmPmText(AmPm ap, Case cs) const;

    mutable int currentSectionIndex;
    Sections display;
    mutable int cachedDay;
    mutable QString text;
    QList<SectionNode> sectionNodes;
    SectionNode first, last, none;
    QStringList separators;
    QString displayFormat;
    QVariant::Type typ;

    bool fixday;
    bool allowEmpty;

    static int dateTimeCompare(const QVariant &arg1, const QVariant &arg2);
};

Q_CORE_EXPORT bool operator==(const QDateTimeParser::SectionNode &s1, const QDateTimeParser::SectionNode &s2);

Q_DECLARE_OPERATORS_FOR_FLAGS(QDateTimeParser::Sections)
Q_DECLARE_OPERATORS_FOR_FLAGS(QDateTimeParser::FieldInfo)


#endif // QT_BOOTSTRAPPED

#endif // QDATETIME_P_H
