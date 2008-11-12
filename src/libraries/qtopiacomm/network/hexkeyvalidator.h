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

#ifndef HEXKEY_VALIDATOR
#define HEXKEY_VALIDATOR

#include <QValidator>
#include <QWidget>
#include <qtopiaglobal.h>

class QTOPIACOMM_EXPORT HexKeyValidator : public QValidator {
public:
    explicit HexKeyValidator( QWidget* parent = 0, int numDigits = 0);
    ~HexKeyValidator() {};

    QValidator::State validate( QString& key, int& curs ) const;
private:
    const int neededNumDigits;
};

#endif //HEXKEY_VALIDATOR
