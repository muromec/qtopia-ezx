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

#include "fractioninstruction.h"

// Data type
void BaseFractionInstruction::eval() {
    FractionData *acc = (FractionData *)systemEngine->getData();
    if (argCount == 1)
        doEval(acc);
    else {
        FractionData *num = (FractionData *)systemEngine->getData();
        doEval(acc,num);
        delete num;
    }
    delete acc;
}
BaseFractionInstruction::BaseFractionInstruction()
    : Instruction() {
        type = retType = "Fraction"; // No tr
    }

// Factory
void iFractionFactory::eval() {
    FractionData *ret = new FractionData();
    ret->clear();
    systemEngine->putData(ret);
}
iFractionFactory::iFractionFactory():Instruction() {
    type = retType = "Fraction"; // No tr
    name = "Factory"; // No tr
};

// Copy
void iFractionCopy::eval() {
    FractionData *src = (FractionData *)systemEngine->getData();
    FractionData *tgt = new FractionData();
    tgt->clear();
    tgt->set(((FractionData *)src)->getNumerator(),
            ((FractionData *)src)->getDenominator());
    systemEngine->putData(src);
    systemEngine->putData(tgt);
};
iFractionCopy::iFractionCopy():BaseFractionInstruction() {
    name = "Copy"; // No tr
    retType = type = "Fraction"; // No tr
    argCount = 1;
};

// Conversion functions
void iConvertDoubleFraction::eval() {

    FractionData *ret = new FractionData();
    double target,tmp, diff;
    qlonglong upper,lower;

    DoubleData *doubleD = (DoubleData *)systemEngine->getData();
    target = doubleD->get();
    QString formattedOutput = doubleD->getFormattedOutput();
    bool ok=false;
    if (formattedOutput.contains('.') == 0) {
        ret->set(formattedOutput.toInt(&ok),1);
    }
    int maxCycles = 0;
    if (!ok) {
        diff = upper = lower = 1;
        tmp = 0;
        while ( (diff > 0.0000001 || diff < -0.0000001) &&  maxCycles < 100000) { // limited precision
            maxCycles++;
            if (tmp < target)
                upper++;
            else {
                lower++;
                upper = (qlonglong) target * lower;
            }
            tmp =  (double) upper / (double) lower;
            diff = tmp-target;
        }
        ret->set(upper,lower);
    }
    if (ok || maxCycles < 100000) {
        systemEngine->putData(ret);
        delete doubleD;
    } else {
        delete ret;
        systemEngine->putData(doubleD); // put data back on stack for recover
        systemEngine->setError(eOutOfRange, false);
    }
}
iConvertDoubleFraction::iConvertDoubleFraction():Instruction() {
    name = "Convert"; // No tr
    retType = "Fraction"; // No tr
    type = "Double"; // No tr
}

// Mathematical functions
void iAddFractionFraction::doEval (FractionData *f,FractionData *fractionNum) {
    FractionData *result = new FractionData();
    qlonglong nn,nd,fn,fd;
    nn = fractionNum->getNumerator();
    nd = fractionNum->getDenominator();
    fn = f->getNumerator();
    fd = f->getDenominator();
    result->set(nn * fd + nd * fn, nd * fd);
    systemEngine->putData(result);
}
void iSubtractFractionFraction::doEval (FractionData *f,FractionData *fractionNum) {
    FractionData *result = new FractionData();
    qlonglong nn = fractionNum->getNumerator();
    qlonglong nd = fractionNum->getDenominator();
    qlonglong fn = f->getNumerator();
    qlonglong fd = f->getDenominator();
    result->set(nn * fd - nd * fn, nd * fd);
    systemEngine->putData(result);
}
void iMultiplyFractionFraction::doEval (FractionData *f,FractionData *fractionNum) {
    FractionData *result = new FractionData();
    qlonglong nn = fractionNum->getNumerator();
    qlonglong nd = fractionNum->getDenominator();
    qlonglong fn = f->getNumerator();
    qlonglong fd = f->getDenominator();
    result->set(nn * fn, nd * fd);
    systemEngine->putData(result);
}
void iDivideFractionFraction::doEval (FractionData *f,FractionData *fractionNum) {
    FractionData *result = new FractionData();
    qlonglong nn = fractionNum->getNumerator();
    qlonglong nd = fractionNum->getDenominator();
    qlonglong fn = f->getNumerator();
    qlonglong fd = f->getDenominator();
    if (nd*fn == 0) {
        systemEngine->setError(eDivZero);
        return;
    }
    result->set(nn * fd, nd * fn);
    systemEngine->putData(result);
}

void iNegateFractionFraction::doEval (FractionData *f) {
    FractionData *result = new FractionData();
    result->set( -f->getNumerator(), f->getDenominator() );
    result->setEdited(true);
    systemEngine->putData(result);
}

iAddFractionFraction::iAddFractionFraction():BaseFractionInstruction() {
    name = "Add"; // No tr
    precedence = 10;
    displayString = "+";
    argCount = 2;
}
iSubtractFractionFraction::iSubtractFractionFraction():BaseFractionInstruction() {
    name = "Subtract"; // No tr
    precedence = 10;
    displayString = "-";
    argCount = 2;
}
iMultiplyFractionFraction::iMultiplyFractionFraction():BaseFractionInstruction() {
    name = "Multiply"; // No tr
    precedence = 15;
    displayString = "x";
    argCount = 2;
}
iDivideFractionFraction::iDivideFractionFraction():BaseFractionInstruction() {
    name = "Divide"; // No tr
    precedence = 15;
    displayString = "/";
    argCount = 2;
}
iNegateFractionFraction::iNegateFractionFraction():BaseFractionInstruction() {
    name = "Negate"; // No tr
    precedence = 0;
    argCount = 1;
}
#endif //ENABLE_FRACTION
