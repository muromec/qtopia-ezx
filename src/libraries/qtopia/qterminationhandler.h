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

#ifndef _QTERMINATIONHANDLER_H_
#define _QTERMINATIONHANDLER_H_

#include <QObject>
#include <QtopiaServiceRequest>
#include <QString>

struct QTerminationHandlerPrivate;
struct QTerminationHandlerData;
class QTOPIA_EXPORT QTerminationHandler : public QObject
{
public:
    explicit QTerminationHandler(const QString &text,
                                 const QString &buttonText = QString(),
                                 const QString &icon = QString(),
                                 const QtopiaServiceRequest &action = QtopiaServiceRequest(),
                                 QObject *parent = 0);
    explicit QTerminationHandler(const QtopiaServiceRequest &action,
                                 QObject *parent = 0);
    ~QTerminationHandler();

private:
    void installHandler(const QTerminationHandlerData& data);
    static QTerminationHandlerPrivate* staticData();
};

#endif // _QTERMINATIONHANDLER_H_
