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

#ifndef OBSERVER_H
#define OBSERVER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore>

class Subject;

class Observer
{
public:
    virtual ~Observer() { };
    virtual void update( Subject* subject ) = 0;
};

class Subject
{
public:
    virtual ~Subject() { };

    virtual void attach( Observer* observer );
    virtual void detach( Observer* observer );
    virtual void notify();

protected:
    Subject() { };

private:
    QList<Observer*> m_observers;
};

inline void Subject::attach( Observer* observer )
{
    m_observers.append( observer );
}

inline void Subject::detach( Observer* observer )
{
    m_observers.removeAll( observer );
}

inline void Subject::notify()
{
    foreach( Observer *observer, m_observers ) {
        observer->update( this );
    }
}

#endif // OBSERVER_H
