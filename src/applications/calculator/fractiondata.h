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

#ifndef CALC_FRACTIONDATA_H
#define CALC_FRACTIONDATA_H

#include "data.h"

class FractionData:public Data {
public:
    FractionData(){clear();};
    ~FractionData(){};
    QString getType(){return "Fraction";};
    void setEdited(bool edit) {
        edited = edit;
    };

    bool push(char,bool);
    bool del();
    void clear();

    void set(qlonglong,qlonglong);
    qlonglong getNumerator(){return numerator;};
    qlonglong getDenominator(){return denominator;};

protected:
    virtual void buildFormattedString();

private:
    qlonglong findGCM(qlonglong,qlonglong);
    qlonglong doFindGCM(qlonglong,qlonglong);
    qlonglong numerator,denominator;
    bool dEdited;
    bool edited;
    QString nString, dString;
};

#endif
#endif //ENABLE_FRACTION
