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

#ifndef _SCREENCLICK_H_
#define _SCREENCLICK_H_

#include "qtopiaserverapplication.h"

class ScreenClick : public QObject,
                    public QtopiaServerApplication::QWSEventFilter
{
Q_OBJECT
public:
    ScreenClick();
    virtual ~ScreenClick();

protected:
    virtual bool qwsEventFilter( QWSEvent * );
    virtual void screenClick(bool) = 0;

private slots:
    void rereadVolume();

private:
    bool m_clickenabled;
    bool m_up;
};

#endif // _SCREENCLICK_H_
