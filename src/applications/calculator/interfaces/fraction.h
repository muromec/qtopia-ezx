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
#ifdef ENABLE_FRACTION

#ifndef FRACTIONIMPL_H
#define FRACTIONIMPL_H

#include <qtopianamespace.h>
#include <QPushButton>

#include "../engine.h"
#include "../doubledata.h"
#include "stdinputwidgets.h"

class FormFraction:public FractionInputWidget {
Q_OBJECT
public:
    FormFraction(QWidget * parent = 0, Qt::WFlags = 0);
    ~FormFraction(){};

    QString interfaceName() { return QString(tr("Fraction")); };

public slots:
    void showEvent ( QShowEvent * );

protected:
    void init(int fromRow, int fromCol);

private slots:
    void MPlusClicked();
    void MCClicked();
    void MRClicked();
    void CEClicked();
};

#endif
#endif //ENABLE_FRACTION
