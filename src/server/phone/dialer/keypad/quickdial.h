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

#ifndef HAVE_QUICKDIAL_H
#define HAVE_QUICKDIAL_H

#include <qtopiaservices.h>

#include "qabstractdialerscreen.h"
#include "callcontactlist.h"

class NumberDisplay;
class QuickDialModel;

class PhoneQuickDialerScreen : public QAbstractDialerScreen
{
    Q_OBJECT
public:
    PhoneQuickDialerScreen( QWidget *parent, Qt::WFlags fl = 0 );
    ~PhoneQuickDialerScreen();

    virtual void reset();
    virtual void setDigits(const QString &digits);
    virtual void appendDigits(const QString &digits);
    virtual QString digits() const;

protected:
    bool eventFilter( QObject *o, QEvent *e );

signals:
    void numberSelected(const QString&, const QUniqueId&);

protected slots:
    void rejectEmpty(const QString&);
    void selectedNumber( const QString &num );
    void selectedNumber( const QString &num, const QUniqueId &cnt );
    void showEvent( QShowEvent *e );

private:
    void appendDigits( const QString &digits, bool refresh,
                       bool speedDial = true );

    NumberDisplay *mNumberDS;
    CallContactListView *mDialList;
    QString mNumber;
    bool mSpeedDial;
    QuickDialModel *mDialModel;
};

#endif
