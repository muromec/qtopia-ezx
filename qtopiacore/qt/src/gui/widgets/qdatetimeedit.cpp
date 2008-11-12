/****************************************************************************)
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <math.h>
#include <private/qabstractspinbox_p.h>
#include <private/qdatetime_p.h>
#include <private/qdatetimeedit_p.h>
#include <qabstractspinbox.h>
#include <qapplication.h>
#include <qdatetimeedit.h>
#include <qdesktopwidget.h>
#include <qdebug.h>
#include <qevent.h>
#include <qlineedit.h>
#include <private/qlineedit_p.h>
#include <qlocale.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qset.h>
#include <qstyle.h>

#ifndef QT_NO_DATETIMEEDIT

//#define QDATETIMEEDIT_QDTEDEBUG
#ifdef QDATETIMEEDIT_QDTEDEBUG
#  define QDTEDEBUG qDebug() << QString("%1:%2").arg(__FILE__).arg(__LINE__)
#  define QDTEDEBUGN qDebug
#else
#  define QDTEDEBUG if (false) qDebug()
#  define QDTEDEBUGN if (false) qDebug
#endif


class QDateTimeEditPrivate : public QAbstractSpinBoxPrivate, public QDateTimeParser
{
    Q_DECLARE_PUBLIC(QDateTimeEdit)
public:
    QDateTimeEditPrivate();

    void init();
    void readLocaleSettings();

    void emitSignals(EmitPolicy ep, const QVariant &old);
    QString textFromValue(const QVariant &f) const;
    QVariant valueFromText(const QString &f) const;
    virtual void _q_editorCursorPositionChanged(int oldpos, int newpos);
    virtual void interpret(EmitPolicy ep);

    QVariant validateAndInterpret(QString &input, int &, QValidator::State &state, bool fixup = false) const;
    void clearSection(int index);
    virtual QString displayText() const { return edit->displayText(); } // this is from QDateTimeParser

    int absoluteIndex(QDateTimeEdit::Section s, int index) const;
    int absoluteIndex(const SectionNode &s) const;
    void updateEdit();
    QVariant stepBy(int index, int steps, bool test = false) const;
    int sectionAt(int pos) const;
    int closestSection(int index, bool forward) const;
    int nextPrevSection(int index, bool forward) const;
    void setSelected(int index, bool forward = false);

    void updateCache(const QVariant &val, const QString &str) const;

    QVariant getMinimum() const { return minimum; }
    QVariant getMaximum() const { return maximum; }
    QString valueToText(const QVariant &var) const { return textFromValue(var); }
    QString getAmPmText(AmPm ap, Case cs) const;
    int cursorPosition() const { return edit ? edit->cursorPosition() : -1; }
    virtual QStyle::SubControl newHoverControl(const QPoint &pos);
    virtual void updateEditFieldGeometry();

    void _q_resetButton();
    void updateArrow(QStyle::StateFlag state);
    bool showCalendarPopup() const;

    bool isSeparatorKey(const QKeyEvent *k) const;

    static QDateTimeEdit::Sections convertSections(QDateTimeParser::Sections s);
    static QDateTimeEdit::Section convertToPublic(QDateTimeParser::Section s);

    void initCalendarPopup();
    void positionCalendarPopup();

    QDateTimeEdit::Sections sections;
    mutable bool cacheGuard;

    QString defaultDateFormat, defaultTimeFormat, unreversedFormat;
    Qt::LayoutDirection layoutDirection;
    mutable QVariant conflictGuard;
    bool hasHadFocus, formatExplicitlySet, calendarPopup;
    QStyle::StateFlag arrowState;
    QCalendarPopup *monthCalendar;

#ifdef QT_KEYPAD_NAVIGATION
    bool focusOnButton;
#endif
};

// --- QDateTimeEdit ---

/*!
  \class QDateTimeEdit qdatetimeedit.h
  \brief The QDateTimeEdit class provides a widget for editing dates and times.

  \ingroup basicwidgets
  \mainclass

  QDateTimeEdit allows the user to edit dates by using the keyboard or
  the arrow keys to increase and decrease date and time values. The
  arrow keys can be used to move from section to section within the
  QDateTimeEdit box. Dates and times appear in accordance with the
  format set; see setDisplayFormat().

  \code
  QDateTimeEdit *dateEdit = new QDateTimeEdit(QDate::currentDate());
  dateEdit->setMinimumDate(QDate::currentDate().addDays(-365));
  dateEdit->setMaximumDate(QDate::currentDate().addDays(365));
  dateEdit->setDisplayFormat("yyyy.MM.dd");
  \endcode

  Here we've created a new QDateTimeEdit object initialized with
  today's date, and restricted the valid date range to today plus or
  minus 365 days. We've set the order to month, day, year.

  The minimum value for QDateTimeEdit is 14 September 1752,
  and 2 January 4713BC for QDate. You can change this by calling
  setMinimumDate(), setMaximumDate(),  setMinimumTime(),
  and setMaximumTime().

  \table 100%
  \row \o \inlineimage windowsxp-datetimeedit.png Screenshot of a Windows XP style date time editing widget
       \o A date time editing widget shown in the \l{Windows XP Style Widget Gallery}{Windows XP widget style}.
  \row \o \inlineimage macintosh-datetimeedit.png Screenshot of a Macintosh style date time editing widget
       \o A date time editing widget shown in the \l{Macintosh Style Widget Gallery}{Macintosh widget style}.
  \row \o \inlineimage plastique-datetimeedit.png Screenshot of a Plastique style date time editing widget
       \o A date time editing widget shown in the \l{Plastique Style Widget Gallery}{Plastique widget style}.
  \endtable

  \sa QDateEdit, QTimeEdit, QDate, QTime
*/

/*!
  \enum QDateTimeEdit::Section

  \value NoSection
  \value AmPmSection
  \value MSecSection
  \value SecondSection
  \value MinuteSection
  \value HourSection
  \value DaySection
  \value MonthSection
  \value YearSection
  \omitvalue DateSections_Mask
  \omitvalue TimeSections_Mask
*/

/*!
  \fn void QDateTimeEdit::dateTimeChanged(const QDateTime &datetime)

  This signal is emitted whenever the date or time is changed. The
  new date and time is passed in \a datetime.
*/

/*!
  \fn void QDateTimeEdit::timeChanged(const QTime &time)

  This signal is emitted whenever the time is changed. The new time
  is passed in \a time.
*/

/*!
  \fn void QDateTimeEdit::dateChanged(const QDate &date)

  This signal is emitted whenever the date is changed. The new date
  is passed in \a date.
*/


/*!
  Constructs an empty date time editor with a \a parent.
*/

QDateTimeEdit::QDateTimeEdit(QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->value = QVariant(QDateTime(QDATETIMEEDIT_DATE_INITIAL, QDATETIMEEDIT_TIME_MIN));
    setDisplayFormat(d->defaultDateFormat + QLatin1String(" ") + d->defaultTimeFormat);
    d->init();
}

/*!
  Constructs an empty date time editor with a \a parent. The value
  is set to \a datetime.
*/

QDateTimeEdit::QDateTimeEdit(const QDateTime &datetime, QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->value = datetime.isValid() ? QVariant(datetime) : QVariant(QDateTime(QDATETIMEEDIT_DATE_INITIAL, QDATETIMEEDIT_TIME_MIN));
    setDisplayFormat(d->defaultDateFormat + QLatin1String(" ") + d->defaultTimeFormat);
    d->init();
}

/*!
  \fn QDateTimeEdit::QDateTimeEdit(const QDate &date, QWidget *parent)

  Constructs an empty date time editor with a \a parent.
  The value is set to \a date.
*/

QDateTimeEdit::QDateTimeEdit(const QDate &date, QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->value = QVariant(QDateTime(date.isValid() ? date : QDATETIMEEDIT_DATE_INITIAL, QDATETIMEEDIT_TIME_MIN));
    setDisplayFormat(d->defaultDateFormat);
    d->init();
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled())
        setCalendarPopup(true);
#endif
}

/*!
  \fn QDateTimeEdit::QDateTimeEdit(const QTime &time, QWidget *parent)

  Constructs an empty date time editor with a \a parent.
  The value is set to \a time.
*/

QDateTimeEdit::QDateTimeEdit(const QTime &time, QWidget *parent)
    : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
    Q_D(QDateTimeEdit);
    d->value = QVariant(QDateTime(QDATETIMEEDIT_DATE_INITIAL, time.isValid() ? time : QDATETIMEEDIT_TIME_MIN));
    setDisplayFormat(d->defaultTimeFormat);
    if (d->displayFormat.isEmpty()) {
        d->defaultDateFormat = QLatin1String("hh:mm:ss");
        setDisplayFormat(d->defaultTimeFormat);
    }
    d->init();
}

QDateTime QDateTimeEdit::dateTime() const
{
    Q_D(const QDateTimeEdit);
    return d->value.toDateTime();
}

void QDateTimeEdit::setDateTime(const QDateTime &datetime)
{
    Q_D(QDateTimeEdit);
    if (datetime.isValid()) {
        d->cachedDay = -1;
        d->clearCache();
        if (!(d->sections & DateSections_Mask))
            setDateRange(datetime.date(), datetime.date());
        d->setValue(QVariant(datetime), EmitIfChanged);
    }
}

/*!
  \property QDateTimeEdit::date
  \brief the QDate that is set in the QDateTimeEdit

  \sa time
*/

/*!
    \property QDateEdit::date
    \brief the QDate that is edited in the QDateEdit

*/

/*!
    \property QTimeEdit::time
    \brief the QTime that is edited in the QTimeEdit
*/

/*!
    Returns the date of the date time edit.
*/
QDate QDateTimeEdit::date() const
{
    Q_D(const QDateTimeEdit);
    return d->value.toDate();
}

void QDateTimeEdit::setDate(const QDate &date)
{
    Q_D(QDateTimeEdit);
    if (date.isValid()) {
        if (!(d->sections & DateSections_Mask))
            setDateRange(date, date);

        d->cachedDay = -1;
        d->clearCache();
        d->setValue(QVariant(QDateTime(date, d->value.toTime())), EmitIfChanged);
    }
}

/*!
  \property QDateTimeEdit::time
  \brief the QTime that is set in the QDateTimeEdit

  \sa date
*/

/*!
    Returns the time of the date time edit.
*/
QTime QDateTimeEdit::time() const
{
    Q_D(const QDateTimeEdit);
    return d->value.toTime();
}

void QDateTimeEdit::setTime(const QTime &time)
{
    Q_D(QDateTimeEdit);
    if (time.isValid()) {
        d->clearCache();
        d->cachedDay = -1;
        d->setValue(QVariant(QDateTime(d->value.toDate(), time)), EmitIfChanged);
    }
}

/*!
  \property QDateTimeEdit::dateTime
  \brief the QDateTime that is set in the QDateTimeEdit

  \sa minimumDate, minimumTime, maximumDate, maximumTime
*/

/*!
  \property QDateTimeEdit::minimumDate

  \brief the minimum date of the date time edit

  When setting this property the \l maximumDate is adjusted if
  necessary, to ensure that the range remains valid. If the date is
  not a valid QDate object, this function does nothing.

  \sa minimumTime, maximumTime, setDateRange()
*/

QDate QDateTimeEdit::minimumDate() const
{
    Q_D(const QDateTimeEdit);
    return d->minimum.toDate();
}

void QDateTimeEdit::setMinimumDate(const QDate &min)
{
    Q_D(QDateTimeEdit);
    if (min.isValid() && min >= QDATETIMEEDIT_DATE_MIN) {
        const QVariant m(QDateTime(min, d->minimum.toTime()));
        d->setRange(m, (d->variantCompare(d->maximum, m) > 0 ? d->maximum : m));
    }
}

void QDateTimeEdit::clearMinimumDate()
{
    setMinimumDate(QDATETIMEEDIT_COMPAT_DATE_MIN);
}

/*!
  \property QDateTimeEdit::maximumDate

  \brief the maximum date of the date time edit

  When setting this property the \l minimumDate is adjusted if
  necessary to ensure that the range remains valid. If the date is
  not a valid QDate object, this function does nothing.

  \sa minimumDate, minimumTime, maximumTime, setDateRange()
*/

QDate QDateTimeEdit::maximumDate() const
{
    Q_D(const QDateTimeEdit);
    return d->maximum.toDate();
}

void QDateTimeEdit::setMaximumDate(const QDate &max)
{
    Q_D(QDateTimeEdit);
    if (max.isValid()) {
        const QVariant m(QDateTime(max, d->maximum.toTime()));
        d->setRange((d->variantCompare(d->minimum, m) < 0 ? d->minimum : m), m);
    }
}

void QDateTimeEdit::clearMaximumDate()
{
    setMaximumDate(QDATETIMEEDIT_DATE_MAX);
}

/*!
  \property QDateTimeEdit::minimumTime

  \brief the minimum time of the date time edit

  When setting this property the \l maximumTime is adjusted if
  necessary, to ensure that the range remains valid. If the time is
  not a valid QTime object, this function does nothing.

  \sa maximumTime, minimumDate, maximumDate, setTimeRange()
*/

QTime QDateTimeEdit::minimumTime() const
{
    Q_D(const QDateTimeEdit);
    return d->minimum.toTime();
}

void QDateTimeEdit::setMinimumTime(const QTime &min)
{
    Q_D(QDateTimeEdit);
    if (min.isValid()) {
        const QVariant m(QDateTime(d->minimum.toDate(), min));
        d->setRange(m, (d->variantCompare(d->maximum, m) > 0 ? d->maximum : m));
    }
}

void QDateTimeEdit::clearMinimumTime()
{
    setMinimumTime(QDATETIMEEDIT_TIME_MIN);
}

/*!
  \property QDateTimeEdit::maximumTime

  \brief the maximum time of the date time edit

  When setting this property, the \l minimumTime is adjusted if
  necessary to ensure that the range remains valid. If the time is
  not a valid QTime object, this function does nothing.

  \sa minimumTime, minimumDate, maximumDate, setTimeRange()
*/
QTime QDateTimeEdit::maximumTime() const
{
    Q_D(const QDateTimeEdit);
    return d->maximum.toTime();
}

void QDateTimeEdit::setMaximumTime(const QTime &max)
{
    Q_D(QDateTimeEdit);
    if (max.isValid()) {
        const QVariant m(QDateTime(d->maximum.toDate(), max));
        d->setRange((d->variantCompare(d->minimum, m) < 0 ? d->minimum : m), m);
    }
}

void QDateTimeEdit::clearMaximumTime()
{
    setMaximumTime(QDATETIMEEDIT_TIME_MAX);
}

/*!
  Convenience function to set minimum and maximum date with one
  function call.

  \code
  setDateRange(min, max);
  \endcode

  is analogous to:

  \code
  setMinimumDate(min);
  setMaximumDate(max);
  \endcode

  If either \a min or \a max are not valid, this function does
  nothing.

  \sa setMinimumDate(), maximumDate(), setMaximumDate(),
  clearMinimumDate(), setMinimumTime(), maximumTime(),
  setMaximumTime(), clearMinimumTime(), QDate::isValid()
*/

void QDateTimeEdit::setDateRange(const QDate &min, const QDate &max)
{
    Q_D(QDateTimeEdit);
    if (min.isValid() && max.isValid()) {
        d->setRange(QVariant(QDateTime(min, d->minimum.toTime())),
                    QVariant(QDateTime(max, d->maximum.toTime())));
    }
}

/*!
  Convenience function to set minimum and maximum time with one
  function call.

  \code
  setTimeRange(min, max);
  \endcode

  is analogous to:

  \code
  setMinimumTime(min);
  setMaximumTime(max);
  \endcode

  If either \a min or \a max are not valid, this function does
  nothing.

  \sa setMinimumDate(), maximumDate(), setMaximumDate(),
  clearMinimumDate(), setMinimumTime(), maximumTime(),
  setMaximumTime(), clearMinimumTime(), QTime::isValid()
*/

void QDateTimeEdit::setTimeRange(const QTime &min, const QTime &max)
{
    Q_D(QDateTimeEdit);
    if (min.isValid() && max.isValid()) {
        d->setRange(QVariant(QDateTime(d->minimum.toDate(), min)),
                    QVariant(QDateTime(d->maximum.toDate(), max)));
    }
}

/*!
  \property QDateTimeEdit::displayedSections

  \brief the currently displayed fields of the date time edit

  Returns a bit set of the displayed sections for this format.
  \a setDisplayFormat(), displayFormat()
*/

QDateTimeEdit::Sections QDateTimeEdit::displayedSections() const
{
    Q_D(const QDateTimeEdit);
    return d->sections;
}

/*!
  \property QDateTimeEdit::currentSection

  \brief the current section of the spinbox
  \a setCurrentSection()
*/

QDateTimeEdit::Section QDateTimeEdit::currentSection() const
{
    Q_D(const QDateTimeEdit);
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled() && d->focusOnButton)
        return NoSection;
#endif
    return d->convertToPublic(d->sectionType(d->currentSectionIndex));
}

void QDateTimeEdit::setCurrentSection(Section section)
{
    Q_D(QDateTimeEdit);
    if (section == NoSection || !(section & d->sections))
        return;

    d->updateCache(d->value, d->displayText());
    const int size = d->sectionNodes.size();
    int index = d->currentSectionIndex + 1;
    for (int i=0; i<2; ++i) {
        while (index < size) {
            if (d->convertToPublic(d->sectionType(index)) == section) {
                d->edit->setCursorPosition(d->sectionPos(index));
                QDTEDEBUG << d->sectionPos(index);
                return;
            }
            ++index;
        }
        index = 0;
    }
}

/*!
  \since 4.3

  Returns the Section at \a index.

  If the format is 'yyyy/MM/dd', sectionAt(0) returns YearSection,
  sectionAt(1) returns MonthSection, and sectionAt(2) returns
  YearSection,
*/

QDateTimeEdit::Section QDateTimeEdit::sectionAt(int index) const
{
    Q_D(const QDateTimeEdit);
    if (index < 0 || index >= d->sectionNodes.size())
        return NoSection;
    return d->convertToPublic(d->sectionType(index));
}

/*!
  \since 4.3

  \property QDateTimeEdit::sectionCount

  \brief the number of sections displayed.
  If the format is 'yyyy/yy/yyyy', sectionCount returns 3
*/

int QDateTimeEdit::sectionCount() const
{
    Q_D(const QDateTimeEdit);
    return d->sectionNodes.size();
}


/*!
  \since 4.3

  \property QDateTimeEdit::currentSectionIndex

  \brief the current section index of the spinbox

  If the format is 'yyyy/MM/dd', the displayText is '2001/05/21' and
  the cursorPosition is 5 currentSectionIndex returns 1. If the
  cursorPosition is 3 currentSectionIndex is 0 etc.

  \a setCurrentSection()
  \sa currentSection()
*/

int QDateTimeEdit::currentSectionIndex() const
{
    Q_D(const QDateTimeEdit);
    return d->currentSectionIndex;
}

void QDateTimeEdit::setCurrentSectionIndex(int index)
{
    Q_D(QDateTimeEdit);
    if (index < 0 || index >= d->sectionNodes.size())
        return;
    d->edit->setCursorPosition(d->sectionPos(index));
}

/*!
  \since 4.2

  Selects \a section. If \a section doesn't exist in the currently
  displayed sections this function does nothing. If \a section is
  NoSection this function will unselect all text in the editor.
  Otherwise this function will move the cursor and the current section
  to the selected section.

  \sa currentSection()
*/

void QDateTimeEdit::setSelectedSection(Section section)
{
    Q_D(QDateTimeEdit);
    if (section == NoSection) {
        d->edit->setSelection(d->edit->cursorPosition(), 0);
    } else if (section & d->sections) {
        if (currentSection() != section)
            setCurrentSection(section);
        d->setSelected(d->currentSectionIndex);
    }
}



/*!
  \fn QString QDateTimeEdit::sectionText(Section section) const

  Returns the text from the given \a section.

  ### note about not working when not Acceptable

  \sa currentSection()
*/

QString QDateTimeEdit::sectionText(Section section) const
{
    Q_D(const QDateTimeEdit);
    if (section == QDateTimeEdit::NoSection || !(section & d->sections)) {
        return QString();
    }

    d->updateCache(d->value, d->displayText());
    const int sectionIndex = d->absoluteIndex(section, 0);
    if (sectionIndex < 0)
        return QString();

    return d->sectionText(d->displayText(), sectionIndex, d->sectionPos(sectionIndex));
}

/*!
  \property QDateTimeEdit::displayFormat

  \brief the format used to display the time/date of the date time edit

  This format is the same as the one used described in QDateTime::toString()
  and QDateTime::fromString()

  Example format strings(assuming that the date is 2nd of July 1969):

  \table
  \header \i Format \i Result
  \row \i dd.MM.yyyy    \i 02.07.1969
  \row \i MMM d yy \i Jul 2 69
  \row \i MMMM d yy \i July 2 69
  \endtable

  If you specify an invalid format the format will not be set.

  \sa QDateTime::toString(), displayedSections()
*/

QString QDateTimeEdit::displayFormat() const
{
    Q_D(const QDateTimeEdit);
    return isRightToLeft() ? d->unreversedFormat : d->displayFormat;
}

template<typename T> static inline QList<T> reverse(const QList<T> &l)
{
    QList<T> ret;
    for (int i=l.size() - 1; i>=0; --i)
        ret.append(l.at(i));
    return ret;
}

void QDateTimeEdit::setDisplayFormat(const QString &format)
{
    Q_D(QDateTimeEdit);
    if (d->parseFormat(format)) {
        d->unreversedFormat.clear();
        if (isRightToLeft()) {
            d->unreversedFormat = format;
            d->displayFormat.clear();
            for (int i=d->sectionNodes.size() - 1; i>=0; --i) {
                d->displayFormat += d->separators.at(i + 1);
                d->displayFormat += d->sectionFormat(i);
            }
            d->displayFormat += d->separators.at(0);
            d->separators = reverse(d->separators);
            d->sectionNodes = reverse(d->sectionNodes);
        }

        d->formatExplicitlySet = true;
        d->sections = d->convertSections(d->display);
        d->clearCache();

        d->currentSectionIndex = qMin(d->currentSectionIndex, d->sectionNodes.size() - 1);
        const bool timeShown = (d->sections & TimeSections_Mask);
        const bool dateShown = (d->sections & DateSections_Mask);
        Q_ASSERT(dateShown || timeShown);
        if (timeShown && !dateShown) {
            setDateRange(d->value.toDate(), d->value.toDate());
        } else if (dateShown && !timeShown) {
            setTimeRange(QDATETIMEEDIT_TIME_MIN, QDATETIMEEDIT_TIME_MAX);
            d->value = QVariant(QDateTime(d->value.toDate(), QTime()));
        }
        d->updateEdit();
        d->_q_editorCursorPositionChanged(-1, 0);
    }
}

/*!
    \property QDateTimeEdit::calendarPopup
    \brief the current calender popup showing mode.
    \since 4.2

    The calendar popup will be shown upon clicking the arrow button.
    This property is valid only if there is a valid date display format.

    \sa setDisplayFormat()
*/

bool QDateTimeEdit::calendarPopup() const
{
    Q_D(const QDateTimeEdit);
    return d->calendarPopup;
}

void QDateTimeEdit::setCalendarPopup(bool enable)
{
    Q_D(QDateTimeEdit);
    if (enable == d->calendarPopup)
        return;
    d->calendarPopup = enable;
#ifdef QT_KEYPAD_NAVIGATION
    if (!enable)
        d->focusOnButton = false;
#endif
    d->updateEditFieldGeometry();
    update();
}

/*!
  \reimp
*/

QSize QDateTimeEdit::sizeHint() const
{
    Q_D(const QAbstractSpinBox);
    ensurePolished();

    const QFontMetrics fm(fontMetrics());
    int h = d->edit->sizeHint().height();
    int w = 0;
    QString s;
    s = d->textFromValue(d->minimum) + QLatin1String("   ");
    w = qMax<int>(w, fm.width(s));
    s = d->textFromValue(d->maximum) + QLatin1String("   ");
    w = qMax<int>(w, fm.width(s));
    if (d->specialValueText.size()) {
        s = d->specialValueText;
        w = qMax<int>(w, fm.width(s));
    }
    w += 2; // cursor blinking space

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);
    QSize hint(w, h);
    QSize extra(35, 6);
    opt.rect.setSize(hint + extra);
    extra += hint - style()->subControlRect(QStyle::CC_SpinBox, &opt,
                                            QStyle::SC_SpinBoxEditField, this).size();
    // get closer to final result by repeating the calculation
    opt.rect.setSize(hint + extra);
    extra += hint - style()->subControlRect(QStyle::CC_SpinBox, &opt,
                                            QStyle::SC_SpinBoxEditField, this).size();
    hint += extra;

    opt.rect = rect();
    return style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
        .expandedTo(QApplication::globalStrut());
}

/*!
  \reimp
*/

bool QDateTimeEdit::event(QEvent *event)
{
    Q_D(QDateTimeEdit);
    switch (event->type()) {
    case QEvent::ApplicationLayoutDirectionChange: {
        const bool was = d->formatExplicitlySet;
        const QString oldFormat = d->displayFormat;
        d->displayFormat.clear();
        setDisplayFormat(oldFormat);
        d->formatExplicitlySet = was;
        break; }
    case QEvent::StyleChange:
#ifdef Q_WS_MAC
    case QEvent::MacSizeChange:
#endif
        d->setLayoutItemMargins(QStyle::SE_DateTimeEditLayoutItem);
        break;
    default:
        break;
    }
    return QAbstractSpinBox::event(event);
}

/*!
  \reimp
*/

void QDateTimeEdit::clear()
{
    Q_D(QDateTimeEdit);
    d->clearSection(d->currentSectionIndex);
}
/*!
  \reimp
*/

void QDateTimeEdit::keyPressEvent(QKeyEvent *event)
{
    Q_D(QDateTimeEdit);
    int oldCurrent = d->currentSectionIndex;
    bool select = true;
    bool inserted = false;

    switch (event->key()) {
#ifdef QT_KEYPAD_NAVIGATION
    case Qt::Key_NumberSign:    //shortcut to popup calendar
        if (QApplication::keypadNavigationEnabled() && d->showCalendarPopup()) {
            d->initCalendarPopup();
            d->positionCalendarPopup();
            d->monthCalendar->show();
            return;
        }
        break;
    case Qt::Key_Select:
        if (QApplication::keypadNavigationEnabled()) {
            if (hasEditFocus()) {
                if (d->focusOnButton) {
                    d->initCalendarPopup();
                    d->positionCalendarPopup();
                    d->monthCalendar->show();
                    d->focusOnButton = false;
                    return;
                }
                setEditFocus(false);
                selectAll();
            } else {
                setEditFocus(true);

                //hide cursor
                d->edit->d_func()->setCursorVisible(false);
                if (d->edit->d_func()->cursorTimer > 0)
                    killTimer(d->edit->d_func()->cursorTimer);
                d->edit->d_func()->cursorTimer = 0;

                d->setSelected(0);
            }
        }
        return;
#endif
    case Qt::Key_Enter:
    case Qt::Key_Return:
        d->interpret(AlwaysEmit);
        d->setSelected(d->currentSectionIndex, true);
        event->ignore();
        emit editingFinished();
        return;
    default:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled() && !hasEditFocus()
            && !event->text().isEmpty() && event->text().at(0).isLetterOrNumber()) {
            setEditFocus(true);
            
            //hide cursor
            d->edit->d_func()->setCursorVisible(false);
            if (d->edit->d_func()->cursorTimer > 0)
                killTimer(d->edit->d_func()->cursorTimer);
            d->edit->d_func()->cursorTimer = 0;

            d->setSelected(0);
            oldCurrent = 0;
        }
#endif
        if (!d->isSeparatorKey(event)) {
            inserted = select = !event->text().isEmpty() && event->text().at(0).isPrint() && !(event->modifiers() & ~Qt::ShiftModifier);
            break;
        }
    case Qt::Key_Left:
    case Qt::Key_Right:
        if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
#ifdef QT_KEYPAD_NAVIGATION
            if (!QApplication::keypadNavigationEnabled() || !hasEditFocus()) {
                select = false;
                break;
            }
#else
            if (!(event->modifiers() & Qt::ControlModifier)) {
                select = false;
                break;
            }
#ifdef Q_WS_MAC
            else {
                select = (event->modifiers() & Qt::ShiftModifier);
                break;
            }
#endif
#endif // QT_KEYPAD_NAVIGATION
        }
        // else fall through
    case Qt::Key_Backtab:
    case Qt::Key_Tab: {
        event->accept();
        if (d->specialValue()) {
            d->edit->setSelection(d->edit->cursorPosition(), 0);
            return;
        }
        const bool forward = event->key() != Qt::Key_Left && event->key() != Qt::Key_Backtab
                             && (event->key() != Qt::Key_Tab || !(event->modifiers() & Qt::ShiftModifier));
        int newSection = d->nextPrevSection(d->currentSectionIndex, forward);
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled()) {
            if (d->focusOnButton) {
                newSection = forward ? 0 : d->sectionNodes.size() - 1;
                d->focusOnButton = false;
                update();
            } else if (newSection < 0 && select && d->showCalendarPopup()) {
                setSelectedSection(NoSection);
                d->focusOnButton = true;
                update();
                return;
            }
        }
        // only allow date/time sections to be selected.
        if (newSection & ~(QDateTimeParser::TimeSectionMask | QDateTimeParser::DateSectionMask))
            return;
#endif
        d->edit->deselect();
        d->edit->setCursorPosition(d->sectionPos(newSection));
        QDTEDEBUG << d->sectionPos(newSection);

        if (select)
            d->setSelected(newSection, true);
        return; }
    }
    QAbstractSpinBox::keyPressEvent(event);
    if (select && !(event->modifiers() & Qt::ShiftModifier) && !d->edit->hasSelectedText()) {
        if (inserted && d->sectionAt(d->edit->cursorPosition()) == QDateTimeParser::NoSectionIndex) {
            QString str = d->displayText();
            int pos = d->edit->cursorPosition();
            QValidator::State state;
            d->validateAndInterpret(str, pos, state);
            if (state == QValidator::Acceptable
                && (d->sectionNode(oldCurrent).count != 1 || d->sectionSize(oldCurrent) == d->sectionMaxSize(oldCurrent))) {
                QDTEDEBUG << "Setting currentsection to" << d->closestSection(d->edit->cursorPosition(), true) << event->key()
                          << oldCurrent;
                const int tmp = d->closestSection(d->edit->cursorPosition(), true);
                if (tmp >= 0)
                    d->currentSectionIndex = tmp;
            }
        }
        if (d->currentSectionIndex != oldCurrent) {
            d->setSelected(d->currentSectionIndex);
        }
    }
    if (d->specialValue()) {
        d->edit->setSelection(d->edit->cursorPosition(), 0);
    }
}

/*!
  \reimp
*/

#ifndef QT_NO_WHEELEVENT
void QDateTimeEdit::wheelEvent(QWheelEvent *event)
{
    QAbstractSpinBox::wheelEvent(event);
}
#endif

/*!
  \reimp
*/

void QDateTimeEdit::focusInEvent(QFocusEvent *event)
{
    Q_D(QDateTimeEdit);
    QAbstractSpinBox::focusInEvent(event);
    QString *frm = 0;
    const int oldPos = d->edit->cursorPosition();
    if (!d->formatExplicitlySet) {
        if (d->displayFormat == d->defaultTimeFormat) {
            frm = &d->defaultTimeFormat;
        } else if (d->displayFormat == d->defaultDateFormat) {
            frm = &d->defaultDateFormat;
        }

        if (frm) {
            d->readLocaleSettings();
            if (d->displayFormat != *frm) {
                setDisplayFormat(*frm);
                d->formatExplicitlySet = false;
                d->edit->setCursorPosition(oldPos);
            }
        }
    }
    const bool oldHasHadFocus = d->hasHadFocus;
    d->hasHadFocus = true;
    bool first = true;
    switch (event->reason()) {
    case Qt::BacktabFocusReason:
        first = false;
        break;
    case Qt::MouseFocusReason:
    case Qt::PopupFocusReason:
        return;
    case Qt::ActiveWindowFocusReason:
        if (oldHasHadFocus)
            return;
    case Qt::ShortcutFocusReason:
    case Qt::TabFocusReason:
    default:
        break;
    }
    if (isRightToLeft())
        first = !first;
    d->updateEdit(); // needed to make it update specialValueText

    d->setSelected(first ? 0 : d->sectionNodes.size() - 1);
}

/*!
  \reimp
*/

bool QDateTimeEdit::focusNextPrevChild(bool next)
{
    Q_D(QDateTimeEdit);
    if (!focusWidget())
        return false;

    const int newSection = d->nextPrevSection(d->currentSectionIndex, next);
    switch (d->sectionType(newSection)) {
    case QDateTimeParser::NoSection:
    case QDateTimeParser::FirstSection:
    case QDateTimeParser::LastSection:
        break;
    default:
        return false;
    }
    return QAbstractSpinBox::focusNextPrevChild(next);
}

/*!
  \reimp
*/

void QDateTimeEdit::stepBy(int steps)
{
    Q_D(QDateTimeEdit);
#ifdef QT_KEYPAD_NAVIGATION
    // with keypad navigation and not editFocus, left right change the date/time by a fixed amount.
    if (QApplication::keypadNavigationEnabled() && !hasEditFocus()) {
        // if date based, shift by day.  else shift by 15min
        if (d->sections & DateSections_Mask)
            setDateTime(dateTime().addDays(steps));
        else {
            int minutes = time().hour()*60 + time().minute();
            int blocks = minutes/15;
            blocks += steps;
            /* rounding involved */
            if (minutes % 15) {
                if (steps < 0) {
                    blocks += 1; // do one less step;
                }
            }

            minutes = blocks * 15;

            /* need to take wrapping into account */
            if (!d->wrapping) {
                int max_minutes = d->maximum.toTime().hour()*60 + d->maximum.toTime().minute();
                int min_minutes = d->minimum.toTime().hour()*60 + d->minimum.toTime().minute();

                if (minutes >= max_minutes) {
                    setTime(maximumTime());
                    return;
                } else if (minutes <= min_minutes) {
                    setTime(minimumTime());
                    return;
                }
            }
            setTime(QTime(minutes/60, minutes%60));
        }
        return;
    }
#endif
    // don't optimize away steps == 0. This is the only way to select
    // the currentSection in Qt 4.1.x
    d->setValue(d->stepBy(d->currentSectionIndex, steps, false), EmitIfChanged);
    d->updateCache(d->value, d->displayText());

    d->setSelected(d->currentSectionIndex);
}

/*!
  This virtual function is used by the date time edit whenever it
  needs to display \a dateTime.

  If you reimplement this, you may also need to reimplement
  valueFromText() and validate().

  \sa dateTimeFromText(), validate()
*/
QString QDateTimeEdit::textFromDateTime(const QDateTime &dateTime) const
{
    Q_D(const QDateTimeEdit);
    return dateTime.toString(d->displayFormat);
}


/*!
  Returns an appropriate datetime for the given \a text.

  This virtual function is used by the datetime edit whenever it
  needs to interpret text entered by the user as a value.

  \sa textFromDateTime(), validate()
*/
QDateTime QDateTimeEdit::dateTimeFromText(const QString &text) const
{
    Q_D(const QDateTimeEdit);
    QString copy = text;
    int pos = d->edit->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    return d->validateAndInterpret(copy, pos, state).toDateTime();
}

/*!
  \reimp
*/

QValidator::State QDateTimeEdit::validate(QString &text, int &pos) const
{
    Q_D(const QDateTimeEdit);
    QValidator::State state;
    d->validateAndInterpret(text, pos, state);
    return state;
}

/*!
  \reimp
*/


void QDateTimeEdit::fixup(QString &input) const
{
    Q_D(const QDateTimeEdit);
    QValidator::State state;
    int copy = d->edit->cursorPosition();

    d->validateAndInterpret(input, copy, state, true);
}


/*!
  \reimp
*/

QDateTimeEdit::StepEnabled QDateTimeEdit::stepEnabled() const
{
    Q_D(const QDateTimeEdit);
    if (d->readOnly)
        return StepEnabled(0);
    if (d->specialValue()) {
        return (d->minimum == d->maximum ? StepEnabled(0) : StepEnabled(StepUpEnabled));
    }

    QAbstractSpinBox::StepEnabled ret = 0;

#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled() && !hasEditFocus()) {
        if (d->wrapping)
            return StepEnabled(StepUpEnabled | StepDownEnabled);
        // 3 cases.  date, time, datetime.  each case look
        // at just the relavant component.
        QVariant max, min, val;
        if (d->sections & DateSections_Mask == 0) {
            // time only, no date
            max = d->maximum.toTime();
            min = d->minimum.toTime();
            val = d->value.toTime();
        } else if (d->sections & TimeSections_Mask == 0) {
            // date only, no time
            max = d->maximum.toDate();
            min = d->minimum.toDate();
            val = d->value.toDate();
        } else {
            // both
            max = d->maximum;
            min = d->minimum;
            val = d->value;
        }
        if (val != min)
            ret |= QAbstractSpinBox::StepDownEnabled;
        if (val != max)
            ret |= QAbstractSpinBox::StepUpEnabled;
        return ret;
    }
#endif
    switch (d->sectionType(d->currentSectionIndex)) {
    case QDateTimeParser::NoSection:
    case QDateTimeParser::FirstSection:
    case QDateTimeParser::LastSection: return 0;
    default: break;
    }
    if (d->wrapping)
        return StepEnabled(StepDownEnabled|StepUpEnabled);

    QVariant v = d->stepBy(d->currentSectionIndex, 1, true);
    if (v != d->value) {
        ret |= QAbstractSpinBox::StepUpEnabled;
    }
    v = d->stepBy(d->currentSectionIndex, -1, true);
    if (v != d->value) {
        ret |= QAbstractSpinBox::StepDownEnabled;
    }

    return ret;
}


/*!
  \reimp
*/

void QDateTimeEdit::mousePressEvent(QMouseEvent *event)
{
    Q_D(QDateTimeEdit);
    if (!d->showCalendarPopup()) {
        QAbstractSpinBox::mousePressEvent(event);
        return;
    }
    d->updateHoverControl(event->pos());
    if (d->hoverControl == QStyle::SC_ComboBoxArrow) {
        d->updateArrow(QStyle::State_Sunken);
        d->initCalendarPopup();
        d->positionCalendarPopup();
        //Show the calendar
        d->monthCalendar->show();
        event->accept();
    }
    else {
        QAbstractSpinBox::mousePressEvent(event);
    }
}

/*!
  \class QTimeEdit
  \brief The QTimeEdit class provides a widget for editing times based on
  the QDateTimeEdit widget.

  \ingroup basicwidgets
  \mainclass

  Many of the properties and functions provided by QTimeEdit are implemented in
  QDateTimeEdit. The following properties are most relevant to users of this
  class:

  \list
  \o \l{QDateTimeEdit::time}{time} holds the date displayed by the widget.
  \o \l{QDateTimeEdit::minimumTime}{minimumTime} defines the minimum (earliest) time
     that can be set by the user.
  \o \l{QDateTimeEdit::maximumTime}{maximumTime} defines the maximum (latest) time
     that can be set by the user.
  \o \l{QDateTimeEdit::displayFormat}{displayFormat} contains a string that is used
     to format the time displayed in the widget.
  \endlist

  \table 100%
  \row \o \inlineimage windowsxp-timeedit.png Screenshot of a Windows XP style time editing widget
       \o A time editing widget shown in the \l{Windows XP Style Widget Gallery}{Windows XP widget style}.
  \row \o \inlineimage macintosh-timeedit.png Screenshot of a Macintosh style time editing widget
       \o A time editing widget shown in the \l{Macintosh Style Widget Gallery}{Macintosh widget style}.
  \row \o \inlineimage plastique-timeedit.png Screenshot of a Plastique style time editing widget
       \o A time editing widget shown in the \l{Plastique Style Widget Gallery}{Plastique widget style}.
  \endtable

  \sa QDateEdit, QDateTimeEdit
*/

/*!
  Constructs an empty time editor with a \a parent.
*/


QTimeEdit::QTimeEdit(QWidget *parent)
    : QDateTimeEdit(QDATETIMEEDIT_TIME_MIN, parent)
{
}

/*!
  Constructs an empty time editor with a \a parent. The time is set
  to \a time.
*/

QTimeEdit::QTimeEdit(const QTime &time, QWidget *parent)
    : QDateTimeEdit(time, parent)
{
}

/*!
  \class QDateEdit
  \brief The QDateEdit class provides a widget for editing dates based on
  the QDateTimeEdit widget.

  \ingroup basicwidgets
  \mainclass

  Many of the properties and functions provided by QDateEdit are implemented in
  QDateTimeEdit. The following properties are most relevant to users of this
  class:

  \list
  \o \l{QDateTimeEdit::date}{date} holds the date displayed by the widget.
  \o \l{QDateTimeEdit::minimumDate}{minimumDate} defines the minimum (earliest)
     date that can be set by the user.
  \o \l{QDateTimeEdit::maximumDate}{maximumDate} defines the maximum (latest) date
     that can be set by the user.
  \o \l{QDateTimeEdit::displayFormat}{displayFormat} contains a string that is used
     to format the date displayed in the widget.
  \endlist

  \table 100%
  \row \o \inlineimage windowsxp-dateedit.png Screenshot of a Windows XP style date editing widget
       \o A date editing widget shown in the \l{Windows XP Style Widget Gallery}{Windows XP widget style}.
  \row \o \inlineimage macintosh-dateedit.png Screenshot of a Macintosh style date editing widget
       \o A date editing widget shown in the \l{Macintosh Style Widget Gallery}{Macintosh widget style}.
  \row \o \inlineimage plastique-dateedit.png Screenshot of a Plastique style date editing widget
       \o A date editing widget shown in the \l{Plastique Style Widget Gallery}{Plastique widget style}.
  \endtable

  \sa QTimeEdit, QDateTimeEdit
*/

/*!
  Constructs an empty date editor with a \a parent.
*/

QDateEdit::QDateEdit(QWidget *parent)
    : QDateTimeEdit(QDATETIMEEDIT_DATE_INITIAL, parent)
{
}

/*!
  Constructs an empty date editor with a \a parent. The date is set
  to \a date.
*/

QDateEdit::QDateEdit(const QDate &date, QWidget *parent)
    : QDateTimeEdit(date, parent)
{
}


// --- QDateTimeEditPrivate ---

/*!
  \internal
  Constructs a QDateTimeEditPrivate object
*/


QDateTimeEditPrivate::QDateTimeEditPrivate()
    : QDateTimeParser(QVariant::DateTime)
{
    hasHadFocus = false;
    formatExplicitlySet = false;
    cacheGuard = false;
    fixday = true;
    allowEmpty = false;
    type = QVariant::DateTime;
    sections = 0;
    cachedDay = -1;
    currentSectionIndex = FirstSectionIndex;

    layoutDirection = QApplication::layoutDirection();
    first.type = FirstSection;
    last.type = LastSection;
    none.type = NoSection;
    first.pos = 0;
    last.pos = -1;
    none.pos = -1;
    sections = 0;
    calendarPopup = false;
    minimum = QVariant(QDATETIMEEDIT_COMPAT_DATETIME_MIN);
    maximum = QVariant(QDATETIMEEDIT_DATETIME_MAX);
    arrowState = QStyle::State_None;
    monthCalendar = 0;
    readLocaleSettings();

#ifdef QT_KEYPAD_NAVIGATION
    focusOnButton = false;
#endif
}


void QDateTimeEditPrivate::updateEdit()
{
    const QString newText = (specialValue() ? specialValueText : textFromValue(value));
    if (newText == displayText())
        return;
    int selsize = edit->selectedText().size();
    const bool sb = edit->blockSignals(true);

    edit->setText(newText);

    if (!specialValue()
#ifdef QT_KEYPAD_NAVIGATION
        && !(QApplication::keypadNavigationEnabled() && !edit->hasEditFocus())
#endif
            ) {
        int cursor = sectionPos(currentSectionIndex);
        QDTEDEBUG << "cursor is " << cursor << currentSectionIndex;
        cursor = qBound(0, cursor, displayText().size());
        QDTEDEBUG << cursor;
        if (selsize > 0) {
            edit->setSelection(cursor, selsize);
            QDTEDEBUG << cursor << selsize;
        } else {
            edit->setCursorPosition(cursor);
            QDTEDEBUG << cursor;

        }
    }
    edit->blockSignals(sb);
}


/*!
  \internal

  Selects the section \a s. If \a forward is false selects backwards.
*/

void QDateTimeEditPrivate::setSelected(int sectionIndex, bool forward)
{
    if ( specialValue()
#ifdef QT_KEYPAD_NAVIGATION
        || (QApplication::keypadNavigationEnabled() && !edit->hasEditFocus())
#endif
        ) {
        edit->selectAll();
    } else {
        const SectionNode &node = sectionNode(sectionIndex);
        if (node.type == NoSection || node.type == LastSection || node.type == FirstSection)
            return;

        updateCache(value, displayText());
        const int size = sectionSize(sectionIndex);
        if (forward) {
            edit->setSelection(sectionPos(node), size);
        } else {
            edit->setSelection(sectionPos(node) + size, -size);
        }
    }
}


/*!
  \internal

  Returns the section at index \a index or NoSection if there are no sections there.
*/

int QDateTimeEditPrivate::sectionAt(int pos) const
{
    if (pos < separators.first().size()) {
        return (pos == 0 ? FirstSectionIndex : NoSectionIndex);
    } else if (displayText().size() - pos < separators.last().size() + 1) {
        if (separators.last().size() == 0) {
            return sectionNodes.count() - 1;
        }
        return (pos == displayText().size() ? LastSectionIndex : NoSectionIndex);
    }
    updateCache(value, displayText());

    for (int i=0; i<sectionNodes.size(); ++i) {
        const int tmp = sectionPos(i);
        if (pos < tmp + sectionSize(i)) {
            return (pos < tmp ? -1 : i);
        }
    }
    return -1;
}

/*!
  \internal

  Returns the closest section of index \a index. Searches forward
  for a section if \a forward is true. Otherwise searches backwards.
*/

int QDateTimeEditPrivate::closestSection(int pos, bool forward) const
{
    Q_ASSERT(pos >= 0);
    if (pos < separators.first().size()) {
        return forward ? 0 : FirstSectionIndex;
    } else if (displayText().size() - pos < separators.last().size() + 1) {
        return forward ? LastSectionIndex : sectionNodes.size() - 1;
    }
    updateCache(value, displayText());
    for (int i=0; i<sectionNodes.size(); ++i) {
        const int tmp = sectionPos(sectionNodes.at(i));
        if (pos < tmp + sectionSize(i)) {
            if (pos < tmp && !forward) {
                return i-1;
            }
            return i;
        } else if (i == sectionNodes.size() - 1 && pos > tmp) {
            return i;
        }
    }
    qWarning("QDateTimeEdit: Internal Error: closestSection returned NoSection");
    return NoSectionIndex;
}

/*!
  \internal

  Returns a copy of the section that is before or after \a current, depending on \a forward.
*/

int QDateTimeEditPrivate::nextPrevSection(int current, bool forward) const
{
    Q_Q(const QDateTimeEdit);
    if (q->isRightToLeft())
        forward = !forward;

    switch (current) {
    case FirstSectionIndex: return forward ? 0 : FirstSectionIndex;
    case LastSectionIndex: return (forward ? LastSectionIndex : sectionNodes.size() - 1);
    case NoSectionIndex: return FirstSectionIndex;
    default: break;
    }
    Q_ASSERT(current >= 0 && current < sectionNodes.size());

    current += (forward ? 1 : -1);
    if (current >= sectionNodes.size()) {
        return LastSectionIndex;
    } else if (current < 0) {
        return FirstSectionIndex;
    }

    return current;
}

/*!
  \internal

  Clears the text of section \a s.
*/

void QDateTimeEditPrivate::clearSection(int index)
{
    const QLatin1Char space(' ');
    int cursorPos = edit->cursorPosition();
    bool blocked = edit->blockSignals(true);
    QString t = edit->text();
    const int pos = sectionPos(index);
    if (pos == -1) {
        qWarning("QDateTimeEdit: Internal error (%s:%d)", __FILE__, __LINE__);
        return;
    }
    const int size = sectionSize(index);
    t.replace(pos, size, QString().fill(space, size));
    edit->setText(t);
    edit->setCursorPosition(cursorPos);
    QDTEDEBUG << cursorPos;

    edit->blockSignals(blocked);
}


/*!
  \internal

  updates the cached values
*/

void QDateTimeEditPrivate::updateCache(const QVariant &val, const QString &str) const
{
    if (val != cachedValue || str != cachedText || cacheGuard) {
        cacheGuard = true;
        QString copy = str;
        int unused = edit->cursorPosition();
        QValidator::State unusedState;
        validateAndInterpret(copy, unused, unusedState);
        cacheGuard = false;
    }
}

/*!
  \internal

  parses and validates \a input
*/

QVariant QDateTimeEditPrivate::validateAndInterpret(QString &input, int &/*position*/,
                                                    QValidator::State &state, bool fixup) const
{
    if (input.isEmpty()) {
        if (sectionNodes.size() == 1) {
            state = QValidator::Intermediate;
        } else {
            state = QValidator::Invalid;
        }
        return getZeroVariant();
    } else if (cachedText == input && !fixup) {
        state = cachedState;
        return cachedValue;
    }
    if (!specialValueText.isEmpty() && input == specialValueText) {
        state = QValidator::Acceptable;
        return minimum;
    }
    StateNode tmp = parse(input, value, fixup);
    input = tmp.input;
    state = QValidator::State(int(tmp.state));
    if (state == QValidator::Acceptable) {
        if (tmp.conflicts && conflictGuard != tmp.value) {
            conflictGuard = tmp.value;
            clearCache();
            input = textFromValue(tmp.value);
            updateCache(tmp.value, input);
            conflictGuard.clear();
        } else {
            cachedText = input;
            cachedState = state;
            cachedValue = tmp.value;
        }
    } else {
        clearCache();
    }
    return (tmp.value.isNull() ? getZeroVariant() : tmp.value);
}


/*!
  \internal
  \reimp
*/

QString QDateTimeEditPrivate::textFromValue(const QVariant &f) const
{
    Q_Q(const QDateTimeEdit);
    return q->textFromDateTime(f.toDateTime());
}

/*!
  \internal
  \reimp
*/

QVariant QDateTimeEditPrivate::valueFromText(const QString &f) const
{
    Q_Q(const QDateTimeEdit);
    return QVariant(q->dateTimeFromText(f));
}


/*!
  \internal

  Internal function called by QDateTimeEdit::stepBy(). Also takes a
  Section for which section to step on and a bool \a test for
  whether or not to modify the internal cachedDay variable. This is
  necessary because the function is called from the const function
  QDateTimeEdit::stepEnabled() as well as QDateTimeEdit::stepBy().
*/

QVariant QDateTimeEditPrivate::stepBy(int sectionIndex, int steps, bool test) const
{
    Q_Q(const QDateTimeEdit);
    QVariant v = value;
    QString str = displayText();
    int pos = edit->cursorPosition();
    const SectionNode sn = sectionNode(sectionIndex);

    int val;
    // to make sure it behaves reasonably when typing something and then stepping in non-tracking mode
    if (!test && pendingEmit) {
        if (q->validate(str, pos) != QValidator::Acceptable) {
            v = value;
        } else {
            v = valueFromText(str);
        }
        val = getDigit(v, sectionIndex);
    } else {
        val = getDigit(value, sectionIndex);
    }

    val += steps;

    const int min = absoluteMin(sectionIndex);
    const int max = absoluteMax(sectionIndex, value.toDateTime());

    if (val < min) {
        val = (wrapping ? max - (min - val) + 1 : min);
    } else if (val > max) {
        val = (wrapping ? min + val - max - 1 : max);
    }


    const int tmp = v.toDate().day();

    setDigit(v, sectionIndex, val); // if this sets year or month it will make

    // sure that days are lowered if needed.

    // changing one section should only modify that section, if possible
    if (sn.type != AmPmSection && (variantCompare(v, minimum) < 0) || (variantCompare(v, maximum) > 0)) {
        const int localmin = getDigit(minimum, sectionIndex);
        const int localmax = getDigit(maximum, sectionIndex);

        if (wrapping) {
            // just because we hit the roof in one direction, it
            // doesn't mean that we hit the floor in the other
            if (steps > 0) {
                setDigit(v, sectionIndex, min);
                if (sn.type != DaySection && sections & DateSectionMask) {
                    const int daysInMonth = v.toDate().daysInMonth();
                    if (v.toDate().day() < tmp && v.toDate().day() < daysInMonth) {
                        const int adds = qMin(tmp, daysInMonth);
                        v = v.toDateTime().addDays(adds - v.toDate().day());
                    }
                }

                if (variantCompare(v, minimum) < 0) {
                    setDigit(v, sectionIndex, localmin);
                    if (variantCompare(v, minimum) < 0)
                        setDigit(v, sectionIndex, localmin + 1);
                }
            } else {
                setDigit(v, sectionIndex, max);
                if (sn.type != DaySection && sections & DateSectionMask) {
                    const int daysInMonth = v.toDate().daysInMonth();
                    if (v.toDate().day() < tmp && v.toDate().day() < daysInMonth) {
                        const int adds = qMin(tmp, daysInMonth);
                        v = v.toDateTime().addDays(adds - v.toDate().day());
                    }
                }

                if (variantCompare(v, maximum) > 0) {
                    setDigit(v, sectionIndex, localmax);
                    if (variantCompare(v, maximum) > 0)
                        setDigit(v, sectionIndex, localmax - 1);
                }
            }
        } else {
            setDigit(v, sectionIndex, (steps > 0 ? localmax : localmin));
        }
    }
    if (!test && tmp != v.toDate().day() && sn.type != DaySection) {
        // this should not happen when called from stepEnabled
        cachedDay = qMax<int>(tmp, cachedDay);
    }

    if (variantCompare(v, minimum) < 0) {
        if (wrapping) {
            QVariant t = v;
            setDigit(t, sectionIndex, steps < 0 ? max : min);
            int mincmp = variantCompare(t, minimum);
            int maxcmp = variantCompare(t, maximum);
            if (mincmp >= 0 && maxcmp <= 0) {
                v = t;
            } else {
                setDigit(t, sectionIndex, getDigit(steps < 0 ? maximum : minimum, sectionIndex));
                mincmp = variantCompare(t, minimum);
                maxcmp = variantCompare(t, maximum);
                if (mincmp >= 0 && maxcmp <= 0) {
                    v = t;
                }
            }
        } else {
            v = value;
        }
    } else if (variantCompare(v, maximum) > 0) {
        if (wrapping) {
            QVariant t = v;
            setDigit(t, sectionIndex, steps > 0 ? min : max);
            int mincmp = variantCompare(t, minimum);
            int maxcmp = variantCompare(t, maximum);
            if (mincmp >= 0 && maxcmp <= 0) {
                v = t;
            } else {
                setDigit(t, sectionIndex, getDigit(steps > 0 ? minimum : maximum, sectionIndex));
                mincmp = variantCompare(t, minimum);
                maxcmp = variantCompare(t, maximum);
                if (mincmp >= 0 && maxcmp <= 0) {
                    v = t;
                }
            }
        } else {
            v = value;
        }
    }

    const QVariant ret = bound(v, value, steps);
    return ret;
}


/*!
  \internal
  \reimp
*/

void QDateTimeEditPrivate::emitSignals(EmitPolicy ep, const QVariant &old)
{
    Q_Q(QDateTimeEdit);
    if (ep == NeverEmit) {
        return;
    }
    pendingEmit = false;

    const bool dodate = value.toDate().isValid() && (sections & DateSectionMask);
    const bool datechanged = (ep == AlwaysEmit || old.toDate() != value.toDate());
    const bool dotime = value.toTime().isValid() && (sections & TimeSectionMask);
    const bool timechanged = (ep == AlwaysEmit || old.toTime() != value.toTime());

    updateCache(value, displayText());

    if (datechanged || timechanged)
        emit q->dateTimeChanged(value.toDateTime());
    if (dodate && datechanged)
        emit q->dateChanged(value.toDate());
    if (dotime && timechanged)
        emit q->timeChanged(value.toTime());

}

/*!
  \internal
  \reimp
*/

void QDateTimeEditPrivate::_q_editorCursorPositionChanged(int oldpos, int newpos)
{
    if (ignoreCursorPositionChanged || specialValue())
        return;
    const QString oldText = displayText();
    updateCache(value, oldText);

    const bool allowChange = !edit->hasSelectedText();
    const bool forward = oldpos <= newpos;
    ignoreCursorPositionChanged = true;
    int s = sectionAt(newpos);
    if (s == NoSectionIndex && forward && newpos > 0) {
        s = sectionAt(newpos - 1);
    }

    int c = newpos;

    const int selstart = edit->selectionStart();
    const int selSection = sectionAt(selstart);
    const int l = selSection != -1 ? sectionSize(selSection) : 0;

    if (s == NoSectionIndex) {
        if (l > 0 && selstart == sectionPos(selSection) && edit->selectedText().size() == l) {
            s = selSection;
            if (allowChange)
                setSelected(selSection, true);
            c = -1;
        } else {
            int closest = closestSection(newpos, forward);
            c = sectionPos(closest) + (forward ? 0 : qMax<int>(0, sectionSize(closest)));

            if (allowChange) {
                edit->setCursorPosition(c);
                QDTEDEBUG << c;
            }
            s = closest;
        }
    }

    if (allowChange && currentSectionIndex != s) {
        interpret(EmitIfChanged);
    }
    if (c == -1) {
        setSelected(s, true);
    } else if (!edit->hasSelectedText()) {
        if (oldpos < newpos) {
            edit->setCursorPosition(displayText().size() - (oldText.size() - c));
        } else {
            edit->setCursorPosition(c);
        }
    }

    QDTEDEBUG << "currentSectionIndex is set to" << sectionName(sectionType(s))
              << oldpos << newpos
              << "was" << sectionName(sectionType(currentSectionIndex));

    currentSectionIndex = s;
    Q_ASSERT_X(currentSectionIndex < sectionNodes.size(),
               "QDateTimeEditPrivate::_q_editorCursorPositionChanged()",
               qPrintable(QString::fromAscii("Internal error (%1 %2)").
                          arg(currentSectionIndex).
                          arg(sectionNodes.size())));

    ignoreCursorPositionChanged = false;
}

/*!
  \internal

  Try to get the format from the local settings
*/
void QDateTimeEditPrivate::readLocaleSettings()
{
    const QLocale loc;
    defaultTimeFormat = loc.timeFormat(QLocale::ShortFormat);
    defaultDateFormat = loc.dateFormat(QLocale::ShortFormat);
}

QDateTimeEdit::Section QDateTimeEditPrivate::convertToPublic(QDateTimeParser::Section s)
{
    switch (s & ~Internal) {
    case AmPmSection: return QDateTimeEdit::AmPmSection;
    case MSecSection: return QDateTimeEdit::MSecSection;
    case SecondSection: return QDateTimeEdit::SecondSection;
    case MinuteSection: return QDateTimeEdit::MinuteSection;
    case DaySection: return QDateTimeEdit::DaySection;
    case MonthSection: return QDateTimeEdit::MonthSection;
    case YearSection: return QDateTimeEdit::YearSection;
    case Hour12Section:
    case Hour24Section: return QDateTimeEdit::HourSection;
    case FirstSection:
    case NoSection:
    case LastSection: break;
    }
    return QDateTimeEdit::NoSection;
}

QDateTimeEdit::Sections QDateTimeEditPrivate::convertSections(QDateTimeParser::Sections s)
{
    QDateTimeEdit::Sections ret = 0;
    if (s & QDateTimeParser::MSecSection)
        ret |= QDateTimeEdit::MSecSection;
    if (s & QDateTimeParser::SecondSection)
        ret |= QDateTimeEdit::SecondSection;
    if (s & QDateTimeParser::MinuteSection)
        ret |= QDateTimeEdit::MinuteSection;
    if (s & (QDateTimeParser::Hour24Section|QDateTimeParser::Hour12Section))
        ret |= QDateTimeEdit::HourSection;
    if (s & QDateTimeParser::AmPmSection)
        ret |= QDateTimeEdit::AmPmSection;
    if (s & QDateTimeParser::DaySection)
        ret |= QDateTimeEdit::DaySection;
    if (s & QDateTimeParser::MonthSection)
        ret |= QDateTimeEdit::MonthSection;
    if (s & QDateTimeParser::YearSection)
        ret |= QDateTimeEdit::YearSection;

    return ret;
}

/*!
    \reimp
*/

void QDateTimeEdit::paintEvent(QPaintEvent *event)
{
    Q_D(QDateTimeEdit);
    if (!d->showCalendarPopup()) {
        QAbstractSpinBox::paintEvent(event);
        return;
    }

    QStyleOptionSpinBox opt;
    initStyleOption(&opt);

    QStyleOptionComboBox optCombo;

    optCombo.init(this);
    optCombo.editable = true;
    optCombo.subControls = opt.subControls;
    optCombo.activeSubControls = opt.activeSubControls;
    optCombo.state = opt.state;

    QPainter p(this);
    style()->drawComplexControl(QStyle::CC_ComboBox, &optCombo, &p, this);
}

QString QDateTimeEditPrivate::getAmPmText(AmPm ap, Case cs) const
{
    if (ap == AmText) {
        return (cs == UpperCase ? QDateTimeEdit::tr("AM") : QDateTimeEdit::tr("am"));
    } else {
        return (cs == UpperCase ? QDateTimeEdit::tr("PM") : QDateTimeEdit::tr("pm"));
    }
}

int QDateTimeEditPrivate::absoluteIndex(QDateTimeEdit::Section s, int index) const
{
    for (int i=0; i<sectionNodes.size(); ++i) {
        if (convertToPublic(sectionNodes.at(i).type) == s && index-- == 0) {
            return i;
        }
    }
    return NoSectionIndex;
}

int QDateTimeEditPrivate::absoluteIndex(const SectionNode &s) const
{
    return sectionNodes.indexOf(s);
}

void QDateTimeEditPrivate::interpret(EmitPolicy ep)
{
    Q_Q(QDateTimeEdit);
    QString tmp = displayText();
    int pos = edit->cursorPosition();
    const QValidator::State state = q->validate(tmp, pos);
    if (state != QValidator::Acceptable
        && correctionMode == QAbstractSpinBox::CorrectToPreviousValue
        && (state == QValidator::Invalid || !(fieldInfo(currentSectionIndex) & AllowPartial))) {
        setValue(value, ep);
    } else {
        QAbstractSpinBoxPrivate::interpret(ep);
    }
}

/*!
    Initialize \a option with the values from this QDataTimeEdit. This method
    is useful for subclasses when they need a QStyleOptionSpinBox, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QDateTimeEdit::initStyleOption(QStyleOptionSpinBox *option) const
{
    if (!option)
        return;

    Q_D(const QDateTimeEdit);
    QAbstractSpinBox::initStyleOption(option);
    if (d->showCalendarPopup()) {
        option->subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxEditField
                              | QStyle::SC_ComboBoxArrow;
        if (d->arrowState == QStyle::State_Sunken)
            option->state |= QStyle::State_Sunken;
        else
            option->state &= ~QStyle::State_Sunken;
    }
}

void QDateTimeEditPrivate::init()
{
    setLayoutItemMargins(QStyle::SE_DateTimeEditLayoutItem);
}

void QDateTimeEditPrivate::_q_resetButton()
{
    updateArrow(QStyle::State_None);
}

void QDateTimeEditPrivate::updateArrow(QStyle::StateFlag state)
{
    Q_Q(QDateTimeEdit);

    if (arrowState == state)
        return;
    arrowState = state;
    if (arrowState != QStyle::State_None)
        buttonState |= Mouse;
    else {
        buttonState = 0;
        hoverControl = QStyle::SC_ComboBoxFrame;
    }
    q->update();
}

/*!
    \internal
    Returns the hover control at \a pos.
    This will update the hoverRect and hoverControl.
*/
QStyle::SubControl QDateTimeEditPrivate::newHoverControl(const QPoint &pos)
{
    if (!showCalendarPopup())
        return QAbstractSpinBoxPrivate::newHoverControl(pos);

    Q_Q(QDateTimeEdit);

    QStyleOptionComboBox optCombo;
    optCombo.init(q);
    optCombo.editable = true;
    optCombo.subControls = QStyle::SC_All;
    hoverControl = q->style()->hitTestComplexControl(QStyle::CC_ComboBox, &optCombo, pos, q);
    return hoverControl;
}

void QDateTimeEditPrivate::updateEditFieldGeometry()
{
    if (!showCalendarPopup()) {
        QAbstractSpinBoxPrivate::updateEditFieldGeometry();
        return;
    }

    Q_Q(QDateTimeEdit);

    QStyleOptionComboBox optCombo;
    optCombo.init(q);
    optCombo.editable = true;
    optCombo.subControls = QStyle::SC_ComboBoxEditField;
    edit->setGeometry(q->style()->subControlRect(QStyle::CC_ComboBox, &optCombo,
                                                 QStyle::SC_ComboBoxEditField, q));
}

bool QDateTimeEditPrivate::isSeparatorKey(const QKeyEvent *ke) const
{
    if (!ke->text().isEmpty() && currentSectionIndex + 1 < sectionNodes.size() && currentSectionIndex >= 0) {
        if (fieldInfo(currentSectionIndex) & Numeric) {
            if (ke->text().at(0).isNumber())
                return false;
        } else if (ke->text().at(0).isLetterOrNumber()) {
            return false;
        }
        return separators.at(currentSectionIndex + 1).contains(ke->text());
    }
    return false;
}

void QDateTimeEditPrivate::initCalendarPopup()
{
    Q_Q(QDateTimeEdit);
    if (!monthCalendar) {
        monthCalendar = new QCalendarPopup(q->date(), q);
        monthCalendar->setObjectName(QLatin1String("qt_datetimedit_calendar"));
        QObject::connect(monthCalendar, SIGNAL(newDateSelected(QDate)), q, SLOT(setDate(QDate)));
        QObject::connect(monthCalendar, SIGNAL(hidingCalendar(QDate)), q, SLOT(setDate(QDate)));
        QObject::connect(monthCalendar, SIGNAL(activated(QDate)), q, SLOT(setDate(QDate)));
        QObject::connect(monthCalendar, SIGNAL(activated(QDate)), monthCalendar, SLOT(close()));
        QObject::connect(monthCalendar, SIGNAL(resetButton()), q, SLOT(_q_resetButton()));
    }
    else
        monthCalendar->setDate(q->date());
    monthCalendar->setDateRange(q->minimumDate(), q->maximumDate());
}

void QDateTimeEditPrivate::positionCalendarPopup()
{
    Q_Q(QDateTimeEdit);
    QPoint pos = (q->layoutDirection() == Qt::RightToLeft) ? q->rect().bottomRight() : q->rect().bottomLeft();
    QPoint pos2 = (q->layoutDirection() == Qt::RightToLeft) ? q->rect().topRight() : q->rect().topLeft();
    pos = q->mapToGlobal(pos);
    pos2 = q->mapToGlobal(pos2);
    QSize size = monthCalendar->sizeHint();
    QRect screen = QApplication::desktop()->availableGeometry(pos);
    //handle popup falling "off screen"
    if (q->layoutDirection() == Qt::RightToLeft) {
        pos.setX(pos.x()-size.width());
        pos2.setX(pos2.x()-size.width());
        if (pos.x() < screen.left())
            pos.setX(qMax(pos.x(), screen.left()));
        else if (pos.x()+size.width() > screen.right())
            pos.setX(qMax(pos.x()-size.width(), screen.right()-size.width()));
    } else {
        if (pos.x()+size.width() > screen.right())
            pos.setX(screen.right()-size.width());
        pos.setX(qMax(pos.x(), screen.left()));
    }
    if (pos.y() + size.height() > screen.bottom())
        pos.setY(pos2.y() - size.height());
    else if (pos.y() < screen.top())
        pos.setY(screen.top());
    if (pos.y() < screen.top())
        pos.setY(screen.top());
    if (pos.y()+size.height() > screen.bottom())
        pos.setY(screen.bottom()-size.height());
    monthCalendar->move(pos);
}

bool QDateTimeEditPrivate::showCalendarPopup() const
{
    return (calendarPopup && (sections & (YearSection|MonthSection|DaySection)));
}

QCalendarPopup::QCalendarPopup(const QDate &date, QWidget * parent)
    : QWidget(parent, Qt::Popup), oldDate(date)
{
    setAttribute(Qt::WA_WindowPropagation);

    dateChanged = false;
    calendar = new QCalendarWidget(this);
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    calendar->setSelectedDate(date);
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled())
        calendar->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
#endif

    QVBoxLayout *widgetLayout = new QVBoxLayout(this);
    widgetLayout->setMargin(0);
    widgetLayout->setSpacing(0);
    widgetLayout->addWidget(calendar);

    connect(calendar, SIGNAL(activated(QDate)), this, SLOT(dateSelected(QDate)));
    connect(calendar, SIGNAL(clicked(QDate)), this, SLOT(dateSelected(QDate)));
    connect(calendar, SIGNAL(selectionChanged()), this, SLOT(dateSelectionChanged()));

    calendar->setFocus();
}

void QCalendarPopup::setDate(const QDate &date)
{
    oldDate = date;
    calendar->setSelectedDate(date);
}

void QCalendarPopup::setDateRange(const QDate &min, const QDate &max)
{
    calendar->setMinimumDate(min);
    calendar->setMaximumDate(max);
}

void QCalendarPopup::mousePressEvent(QMouseEvent *event)
{
    QDateTimeEdit *dateTime = qobject_cast<QDateTimeEdit *>(parentWidget());
    if (dateTime) {
        QStyleOptionComboBox opt;
        opt.init(dateTime);
        QRect arrowRect = dateTime->style()->subControlRect(QStyle::CC_ComboBox, &opt,
                                                            QStyle::SC_ComboBoxArrow, dateTime);
        arrowRect.moveTo(dateTime->mapToGlobal(arrowRect .topLeft()));
        if (arrowRect.contains(event->globalPos()) || rect().contains(event->pos()))
            setAttribute(Qt::WA_NoMouseReplay);
    }
    QWidget::mousePressEvent(event);
}

void QCalendarPopup::mouseReleaseEvent(QMouseEvent*)
{
    emit resetButton();
}

bool QCalendarPopup::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key()== Qt::Key_Escape)
            dateChanged = false;
    }
    return QWidget::event(event);
}

void QCalendarPopup::dateSelectionChanged()
{
    dateChanged = true;
    emit newDateSelected(calendar->selectedDate());
}
void QCalendarPopup::dateSelected(const QDate &date)
{
    dateChanged = true;
    emit activated(date);
    close();
}

void QCalendarPopup::hideEvent(QHideEvent *)
{
    emit resetButton();
    if (!dateChanged)
        emit hidingCalendar(oldDate);
}

#include "moc_qdatetimeedit.cpp"



#endif // QT_NO_DATETIMEEDIT
