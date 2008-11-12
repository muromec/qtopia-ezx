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

#ifndef CALC_FRACTION_H
#define CALC_FRACTION_H

#include "instruction.h"
#include "fractiondata.h"
#include "doubledata.h"

//Fraction instruction base
class BaseFractionInstruction:public Instruction {
public:
    BaseFractionInstruction();
    ~BaseFractionInstruction(){};

    virtual void eval();
    virtual void doEval(FractionData *,FractionData *){};
    virtual void doEval(FractionData *){};
};

// Factory
class iFractionFactory:public Instruction {
public:
    iFractionFactory();
    ~iFractionFactory(){};
    void eval();
};

// Copy
class iFractionCopy:public BaseFractionInstruction {
public:
    iFractionCopy();
    ~iFractionCopy(){};
    void eval();
};

// Conversions
class iConvertDoubleFraction:public Instruction {
public:
    iConvertDoubleFraction();
    ~iConvertDoubleFraction(){};
    void eval();
};

// Mathematical functions
class iAddFractionFraction:public BaseFractionInstruction {
public:
    iAddFractionFraction();
    ~iAddFractionFraction(){};
    void doEval(FractionData *,FractionData *);
};

class iSubtractFractionFraction:public BaseFractionInstruction {
public:
    iSubtractFractionFraction ();
    ~iSubtractFractionFraction (){};
    void doEval(FractionData *,FractionData *);
};

class iMultiplyFractionFraction:public BaseFractionInstruction {
public:
    iMultiplyFractionFraction ();
    ~iMultiplyFractionFraction (){};
    void doEval(FractionData *,FractionData *);
};

class iDivideFractionFraction:public BaseFractionInstruction {
public:
    iDivideFractionFraction ();
    ~iDivideFractionFraction (){};
    void doEval(FractionData *,FractionData *);
};

class iNegateFractionFraction:public BaseFractionInstruction {
public:
    iNegateFractionFraction ();
    ~iNegateFractionFraction (){};
    void doEval(FractionData *);
};

#endif
#endif //ENABLE_FRACTION
