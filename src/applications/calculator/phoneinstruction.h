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
#ifndef CALC_PHONE_H
#define CALC_PHONE_H

#include "instruction.h"
#include "doubledata.h"

// Factory
class iPhoneDoubleFactory:public Instruction {
public:
    iPhoneDoubleFactory();
    ~iPhoneDoubleFactory(){};
    void eval();
};

// Copy
class iPhoneDoubleCopy:public Instruction {
public:
    iPhoneDoubleCopy();
    ~iPhoneDoubleCopy(){};
    void eval();
};

class iEvaluateLine:public Instruction {
public:
    iEvaluateLine();
    ~iEvaluateLine(){};
    QPixmap *draw();
    void eval(){qWarning("Error - iEvaluateLine should never be evaluated!");};
};

// Mathematical functions
class iPhoneAddDoubleDouble:public Instruction {
public:
    iPhoneAddDoubleDouble();
    ~iPhoneAddDoubleDouble(){};
    void eval();
};

class iPhoneSubtractDoubleDouble:public Instruction {
public:
    iPhoneSubtractDoubleDouble();
    ~iPhoneSubtractDoubleDouble(){};
    void eval();
};

class iPhoneMultiplyDoubleDouble:public Instruction {
public:
    iPhoneMultiplyDoubleDouble();
    ~iPhoneMultiplyDoubleDouble(){};
    void eval();
};

class iPhoneDivideDoubleDouble:public Instruction {
public:
    iPhoneDivideDoubleDouble();
    ~iPhoneDivideDoubleDouble(){};
    void eval();
};
#endif //CALC_PHONE_H
