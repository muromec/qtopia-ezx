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
#include <QString>

#include "fractiondata.h"
#include "engine.h"

// Data type functions
bool FractionData::push(char c, bool commit) {
    if (formattedOutput.length() > 15)
        return false;
    if ( numerator == 0 && c == '0' )
        return !edited;
    if (edited && numerator == 0)
        edited = false;

    if (!edited) {
        numerator = 0;
        nString.truncate(0);
        dString.truncate(0);
        if (commit)
            edited = true;
            dEdited = false;
    }

    if (c == '/') {
        if (dEdited)
            return false;
        if (commit) {
            if (numerator == 0){
                numerator = 1;
                nString.setNum(1);
            }
            denominator = 0;
            dEdited = true;
            edited = true;
            dString.truncate(0);
            buildFormattedString();
        }
        return true;
    }

    // append char and test result
    bool ok = false;
    QString tmpString;
    qlonglong num;
    if (dEdited) {
        tmpString = dString;
        tmpString.append(QChar(c));
        num = tmpString.toLongLong(&ok);
        if (ok && commit) {
            dString = tmpString;
            denominator = num;
        }
    } else {
        tmpString = nString;
        tmpString.append(QChar(c));
        num = tmpString.toLongLong(&ok);
        if (ok && commit) {
            nString = tmpString;
            numerator = num;
            edited = true;
        }
    }
    if (ok && commit)
        buildFormattedString();
    return ok;
}
// puts dString and nString together
void FractionData::buildFormattedString() {
    formattedOutput.truncate(0);
    if (denominator < 0) {
        numerator = 0 - numerator;
        nString.truncate(0);
        nString.setNum(numerator);
        denominator = 0 - denominator;
        dString.truncate(0);
        dString.setNum(denominator);
        buildFormattedString();
        return;
    }

    formattedOutput.append(nString);

    if (dEdited && denominator != 1) {
        formattedOutput.append(QChar('/'));
        if (denominator)
            formattedOutput.append(dString);
    }
    if (!formattedOutput.length())
        formattedOutput.append(QChar('0'));
}
bool FractionData::del() {
    if (!edited)
        return true;

    if (dEdited) {
        if (dString.length())  {
            dString.truncate(dString.length()-1);
            bool ok = true;
            denominator=dString.toLongLong(&ok);
            if (!ok) {
                denominator = 1;
                dEdited = false;
            }
        } else {
            dEdited = false;
        }
    } else {
        if (formattedOutput.length() == 1 ||
                (formattedOutput.length() == 2) && formattedOutput[0] == '-') {
            nString.truncate(0);
            nString.append("0");
            edited = false;
            numerator = 0;
            return true;
        } else
            nString.truncate(nString.length()-1);
            numerator = numerator / 10;
    }

    buildFormattedString();
    return false;
}
void FractionData::clear() {
    edited = false;
    dEdited = false;
    nString.truncate(0);
    dString.truncate(0);
    formattedOutput.truncate(0);
    formattedOutput.append("0");
    numerator = 0;
    denominator = 1;
    buildFormattedString();
}

void FractionData::set(qlonglong n,qlonglong d) {
    clear();
    if (!d)
        d = 1;
    qlonglong GCM = findGCM(n,d);
    numerator = n / GCM;
    denominator = d / GCM;
    if (d == 1)
        dEdited = false;
    else
        dEdited = true;
    nString.setNum(numerator);
    dString.setNum(denominator);
    buildFormattedString();
}
// find greatest common d
qlonglong FractionData::doFindGCM ( qlonglong a, qlonglong b ) {
    qlonglong remainder = b % a;
    if ( remainder )
        return doFindGCM( remainder,a);
    return a;
}

// wrapper that makes sure that a and b
// are the right way round
qlonglong FractionData::findGCM ( qlonglong a, qlonglong b ) {
    if ( a == b )
        return a;
    if ( a > b )
        return doFindGCM ( a, b );
    else
        return doFindGCM ( b, a );
}

#endif //ENABLE_FRACTION
