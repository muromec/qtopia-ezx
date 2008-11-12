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

#ifndef _MOTIONPATH_P_H_
#define _MOTIONPATH_P_H_

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

#include <Qt>
#include <QList>
#include <QObject>
#include <QTime>
#include <QHash>
#include <QSet>

class IValue
{
public:
    virtual ~IValue() {}

    virtual qreal value() const = 0;
    virtual void setValue(qreal) = 0;

private:
    IValue *m_parent;
    QSet<IValue *> m_children;
};

class ConcreteValue : public IValue
{
public:
    ConcreteValue(qreal value = 0.0f) : v(value) {}
    ConcreteValue(const ConcreteValue &o) : IValue(), v(o.v) {}
    ConcreteValue &operator=(const ConcreteValue &o) { v = o.v; return *this; }
    ConcreteValue &operator=(qreal o) { v = o; return *this; }
    bool operator==(const ConcreteValue &other) { return v == other.v; }

    operator qreal () const { return v; }

    virtual qreal value() const { return v; }
    virtual void setValue(qreal value) { v = value; }

private:
    qreal v;
};

class OffsetValue : public IValue
{
public:
    OffsetValue(IValue *value) : v(value), off(0.0f) {}
    OffsetValue(IValue *value, qreal offset) : v(value), off(offset) {}
    OffsetValue(const OffsetValue &o) : IValue(), v(o.v), off(o.off) {}
    OffsetValue &operator=(const OffsetValue &o) { v = o.v; off = o.off; return *this; }

    qreal offset() const { return off; }
    void setOffset(qreal o) { off = o; }

    virtual qreal value() const { return v->value() - off; }
    virtual void setValue(qreal value) { v->setValue(value + off); }
private:
    IValue *v;
    qreal off;
};

class PivotValue : public IValue
{
public:
    PivotValue(IValue *out = 0) : v(out) {}
    PivotValue(const PivotValue &o) : IValue(), v(o.v) {}
    PivotValue &operator=(const PivotValue &o) { v = o.v; return *this; }

    IValue *out() const { return v; }
    void setOut(IValue *o) { v = o; }

    virtual qreal value() const { return v?v->value():0.0f; }
    virtual void setValue(qreal value) { if(v) v->setValue(value); }
private:
    IValue *v;
};

class MultiplyValue : public IValue
{
public:
    MultiplyValue(IValue *value) : v(value), mul(1.0f) {}
    MultiplyValue(IValue *value, qreal multiply) : v(value), mul(multiply) {}
    MultiplyValue(const MultiplyValue &o) : IValue(), v(o.v), mul(o.mul) {}
    MultiplyValue &operator=(const MultiplyValue &o) { v = o.v; mul = o.mul; return *this; }

    qreal multiply() const { return mul; }
    void setMultiply(qreal m) { mul = m; }

    virtual qreal value() const { return v->value() / mul; }
    virtual void setValue(qreal value) { v->setValue(value * mul); }

private:
    IValue *v;
    qreal mul;
};

class AdderValue
{
public:
    AdderValue(IValue *out) : v(out) {}
    ~AdderValue() { qDeleteAll(m_vals); }

    void setOuput(IValue *out) { v = out; recompute(); }
    IValue *output() const { return v; }

private:
    friend class AdderConcrete;
    void recompute()
    {
        if(!v) return;

        qreal val = 0.0f;
        for(Values::Iterator iter = m_vals.begin(); iter != m_vals.end(); ++iter)
            val += (*iter)->value();

        v->setValue(val);
    }

    class AdderConcrete : public ConcreteValue
    {
    public:
        AdderConcrete(AdderValue *a) : adder(a) {}
        virtual void setValue(qreal value) {
            ConcreteValue::setValue(value);
            adder->recompute();
        }

    private:
        AdderValue *adder;
    };

public:
    IValue *value(int id) const {
        IValue *rv = m_vals[id];
        if(!rv) {
            rv = new AdderConcrete(const_cast<AdderValue *>(this));
            m_vals[id] = static_cast<AdderConcrete *>(rv);
        }
        return rv;
    }

private:
    IValue *v;
    typedef QHash<int, AdderConcrete *> Values;
    mutable Values m_vals;
};

class ValueGroup : public IValue
{
public:
    ValueGroup() {}

    void addValue(IValue *v)
    {
        m_values.append(v);
        v->setValue(value());
    }

    void remValue(IValue *v)
    {
        m_values.removeAll(v);
    }

    virtual qreal value() const {
        if(m_values.isEmpty()) return 0.0f;
        return m_values.at(0)->value();
    }

    virtual void setValue(qreal v) {
        for(int ii = 0; ii < m_values.count(); ++ii)
            m_values[ii]->setValue(v);
    }

private:
    QList<IValue *> m_values;
};

class MotionVelocity
{
public:
    MotionVelocity();

    void reset();
    void setPosition(qreal);

    qreal velocity();

private:
    qreal m_lastPos;
    qreal m_velocity;
    QTime m_timer;
    bool m_reset;
};

class MotionClock;
class MotionTimeLine : public QObject
{
    Q_OBJECT
public:
    MotionTimeLine(QObject *parent = 0);

    void pause(int time);
    void set(qreal value);
    void accel(qreal velocity, qreal accel);
    void accel(qreal velocity, qreal accel, qreal maxDistance);
    void accelDistance(qreal velocity, qreal distance);

    void move(int time, qreal destination);
    void moveBy(int time, qreal change);

    void setValue(IValue *);

    int updateInterval() const;
    void setUpdateInterval(int);

    void start(int time = 0);
    void start(int time, const QString &name);
    QString runName() const;
    void stop(int time = -1);
    int currentTime() const;
    int endTime() const;
    int remainingTime() const;
    bool isRunning() const;

    void reset();

    static MotionClock *clock();
signals:
    void updated();
    void completed();

private:
    friend class MotionClock;
    void setTime(const QTime &now);

    struct Op {
        enum Type {
            Pause,
            Set,
            Move,
            MoveBy,
            Accel,
            AccelDistance
        };

        Type type;
        int start;
        int length;
        qreal value;
        qreal value2;
    };
    // List of operations
    QList<Op> m_ops;
    // Total length of time used in operations
    int m_totalTime;

    // The *wall* time at which we began timeline processing
    QTime m_wallStartTime;
    // The start time offset provided to start()
    int m_startTimeOffset;
    // The current timeline time
    int m_timelineTime;
    // The current op
    int m_currentOp;
    // The current value baseline for current op
    qreal m_currentOpBaseline;
    // Advance to the new time and update output and m_timelineTime
    void advanceTo(int newTime);

    // Returns the value for op at time
    qreal value(const Op &op, int time, qreal base) const;

    // True if running, false if not
    bool m_isRunning;

    // Interval that timer runs at
    int m_updateInterval;

    // Output location
    ConcreteValue m_dummy;
    IValue *output;

    QString m_name;
};

class MotionClock : public QObject
{
Q_OBJECT
public:
    MotionClock();

    QTime currentTime() const;

    void addTimeline(MotionTimeLine *);
    void remTimeline(MotionTimeLine *);

signals:
    void tickStart();
    void tick();

protected:
    virtual void timerEvent(QTimerEvent *);

private:
    int m_timerId;
    QTime m_currentTime;
    QSet<MotionTimeLine *> m_timelines;
};


// declare MotionValue
class MotionValue
{
public:
    MotionValue();
    MotionValue(const MotionValue &other);
    MotionValue(qreal position);

    MotionValue & operator=(const MotionValue &other);
    void setPosition(qreal position);
    enum Direction { Up, Down, Any };
    void moveTo(qreal position, Direction dir = Any);
    void moveBy(qreal amount);
    void shiftBy(qreal amount);

    // Setting to 0.0f and 0.0f disables bounds checking
    void setBounds(qreal minimum, qreal maximum);

    void setTime(qreal time);
    qreal value() const;
    qreal value(qreal time);
    qreal valueAt(qreal) const;

    Direction destinationDirection() const;
    qreal destination() const;

    bool isDone() const;

    typedef void (*Callback)(const MotionValue *, qreal, qreal, Direction, int, void *);
    void addMotionNotify(qreal value, int, Callback, void *);

private:
    qreal time;

    qreal start;
    qreal distance;

    qreal minimum;
    qreal maximum;

    struct Notify {
        qreal value;
        int id;
        Callback callback;
        void *context;
    };

    QList<Notify> notifies;
};

#endif // _MOTIONPATH_P_H_

