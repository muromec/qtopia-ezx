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

#ifndef DIALER_H
#define DIALER_H

#include <themedview.h>
#include <quniqueid.h>

#include <QDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include "serverthemeview.h"
#include "qabstractdialerscreen.h"

class Dialer;
class PhoneTouchDialerScreen : public QAbstractDialerScreen
{
Q_OBJECT
public:
    PhoneTouchDialerScreen(QWidget *parent = 0, Qt::WFlags f = 0);

    virtual QString digits() const;
    virtual void reset();
    virtual void appendDigits(const QString &digits);
    virtual void setDigits(const QString &digits);

protected slots:
    void keyEntered(const QString &key);

private:
    Dialer *m_dialer;
};

#endif
