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
#ifdef ENABLE_CONVERSION

#ifndef CONVERSIONIMPL_H
#define CONVERSIONIMPL_H

#include "../engine.h"
#include "../doubleinstruction.h"
#include "stdinputwidgets.h"

#include <QPushButton>
#include <QStackedWidget>
#include <QComboBox>

extern Engine *systemEngine;

class UnitConversionInstruction : public BaseDoubleInstruction {
public:
    UnitConversionInstruction():BaseDoubleInstruction(){
        name = "UnitConversion";
        retType = type = "Double";
        argCount = 1;
        precedence = 0;
    };
    ~UnitConversionInstruction(){};

   void doEvalI(DoubleData *acc) {
        DoubleData *result = new DoubleData();
        if (tempFrom != 'n' && tempTo != 'n') {
            double tmp = acc->get();

            if (tempFrom == 'k')
                tmp -= 273;
            if (tempFrom == 'f')
                tmp = (tmp - 32) * 5 / 9;
            if (tempTo == 'k')
                tmp += 273;
            if (tempTo == 'f')
                tmp = tmp / 5 * 9 + 32;

            result->set(tmp);
        } else {
            result->set(acc->get() * to / from);
        }
        systemEngine->putData(result);
    };

    static double from;
    static double to;
    static char tempFrom;
    static char tempTo;
};

struct conversionData {
    QString name;
    double factor;
    QPushButton *button;
};

struct conversionMode {
    QString name;
    QList<conversionData*> *dataList;
    conversionData *defaultType;
};

class FormConversion:public DecimalInputWidget {
    Q_OBJECT
public:
    FormConversion(QWidget * parent = 0);
    ~FormConversion();

    QString interfaceName() { return QString(tr("Conversion")); };

public slots:
    void celciusButtonClicked ();
    void fahrenheitButtonClicked ();
    void kelvinButtonClicked ();

protected:
    void showEvent ( QShowEvent * );
    void init(int fromRow, int fromCol);

private:
    QPushButton *PBC;
    QList<conversionMode*> *modeList;
    QComboBox *typeSelector;
    QStackedWidget *conversionStack;

    double currentFactor, newFactor;
    QPushButton *currentButton, *newButton;

    void doConversion();

public slots:
    void selectType(int);
    void conversionButtonClicked();

private slots:
    void CClicked();
};

#endif //CONVERSIONIMPL_H
#endif //ENABLE_CONVERSION
