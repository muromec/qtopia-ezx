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

#ifndef ELEMENT_H
#define ELEMENT_H

#include <qmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qcolor.h>
#include <qrect.h>

#include <qtopiaglobal.h>

class QXmlAttributes;
class QPainter;
class SmilSystem;
class SmilModuleAttribute;

class Duration
{
public:
    Duration(int v) : val(v), indefinite(false), unresolved(false) {}
    Duration() : val(0), indefinite(false), unresolved(true) {}

    void setIndefinite(bool i) { indefinite = i; unresolved = false; }
    bool isIndefinite() const { return indefinite; }

    void setUnresolved(bool u) { unresolved = u; indefinite = false; }
    bool isUnresolved() const { return unresolved; }

    void setDuration(int d) { val = d; indefinite = false; unresolved = false; }
    int duration() const { return val; }
    bool isValue() const { return !indefinite && !unresolved; }

    Duration operator*(const Duration &d);
    Duration operator+(const Duration &d);
    Duration operator-(const Duration &d);
    bool operator!=(const Duration &d);

private:
    int val;
    bool indefinite;
    bool unresolved;
};


bool operator<=(const Duration &d1, const Duration &d2);
bool operator<(const Duration &d1, const Duration &d2);
bool operator>(const Duration &d1, const Duration &d2);
Duration min(const Duration &d1, const Duration &d2);
Duration max(const Duration &d1, const Duration &d2);

//===========================================================================

class SmilEvent
{
public:
    enum Type {
        None = 0,
        Begin = 1,          // begin resolved
        End = 2,            // end resolved
    };

    SmilEvent(Type type) : t(type) {}
    virtual ~SmilEvent();

    Type type() const { return t; }

protected:
    Type t;
};

//===========================================================================

class QTOPIASMIL_EXPORT SmilElement
{
public:
    SmilElement(SmilSystem *sys, SmilElement *p, const QString &n, const QXmlAttributes &atts);
    virtual ~SmilElement();

    const QString &id() const { return eid; }
    const QString &name() const { return ename; }
    SmilElement *parent() const { return prnt; }

    void addChild(SmilElement *);
    SmilElement *findChild(const QString &id, const QString &nam=QString(), bool recurse=false) const;

    const QRect &rect() const { return r; }
    virtual void setRect(const QRect &r);

    const QColor &backgroundColor() const { return bg; }
    virtual void setBackgroundColor(const QColor &c);

    virtual void setData(const QByteArray &, const QString &type);

    SmilModuleAttribute *module(const QString &name) const;
    void addModule(SmilModuleAttribute *m);

    virtual Duration implicitDuration();
    virtual void addCharacters(const QString &ch);

    virtual void seek(int ms);
    virtual void reset();

    virtual void process();
    virtual void event(SmilElement *, SmilEvent *);

    virtual void paint(QPainter *p);
    void setVisible(bool v) { vis = v; }
    bool isVisible() const { return vis; }

    enum State { Idle, Startup, Waiting, Active, End, Post };

    virtual void setState(State s);
    State state() const { return currentState; }

    void setCurrentBegin(const Duration &begin);
    void setCurrentEnd(const Duration &end);

    void addListener(SmilElement *e);

    const QList<SmilElement*> &children() const { return chn; }
    SmilSystem *system() const { return sys; }

    Duration simpleDuration;
    Duration activeDuration;
    Duration currentBegin;
    Duration currentEnd;

protected:
    QRect r;
    QColor bg;
    SmilSystem *sys;
    SmilElement *prnt;
    QString ename;
    QString eid;
    QList<SmilModuleAttribute*> modules;
    QList<SmilElement*> chn;
    QList<SmilElement*> listeners;
    State currentState;
    bool vis;

protected:
    QColor parseColor(const QString &name);
    void sendEvent(SmilEvent &e);

    friend class SmilViewPrivate;
};

typedef QList<SmilElement*> SmilElementList;


#endif

