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

#include "motionpath_p.h"
#include <QDebug>
#include <QTimerEvent>

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

// define MotionClock
MotionClock::MotionClock()
: m_timerId(0)
{
}

QTime MotionClock::currentTime() const
{
    return m_currentTime;
}

void MotionClock::addTimeline(MotionTimeLine *t)
{
    m_timelines.insert(t);
    if(!m_timerId) {
        m_currentTime = QTime::currentTime();
        m_timerId = startTimer(15);
    }
}

void MotionClock::remTimeline(MotionTimeLine *t)
{
    m_timelines.remove(t);
    if(m_timelines.isEmpty() && m_timerId) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
}

void MotionClock::timerEvent(QTimerEvent *)
{
    emit tickStart();

    m_currentTime = QTime::currentTime();
    foreach(MotionTimeLine *line, m_timelines)
        line->setTime(m_currentTime);

    emit tick();
}

Q_GLOBAL_STATIC(MotionClock, motionClock);

// define MotionVelocity
MotionVelocity::MotionVelocity()
: m_lastPos(0.0f), m_velocity(0.0f), m_reset(true)
{
}

void MotionVelocity::reset()
{
    m_velocity = 0.0f;
    m_lastPos = 0.0f;
    m_reset = true;
}

void MotionVelocity::setPosition(qreal pos)
{
    if(m_reset) {

        m_lastPos = pos;
        m_timer.start();
        m_reset = false;

    } else {

        m_velocity = (pos - m_lastPos) / (qreal)m_timer.elapsed();
        m_lastPos = pos;
        m_timer.start();

    }
}

qreal MotionVelocity::velocity()
{
    return m_velocity;
}

// define MotionTimeLine
MotionTimeLine::MotionTimeLine(QObject *parent)
: QObject(parent), 
    m_totalTime(0),
    m_startTimeOffset(0),
    m_timelineTime(0),
    m_currentOp(0),
    m_currentOpBaseline(0.0f),
    m_isRunning(false),
    m_updateInterval(15),
    output(&m_dummy)
{
}

void MotionTimeLine::pause(int time)
{
    if(time <= 0) return;

    Op op = { Op::Pause, m_totalTime, time, 0.0f, 0.0f };
    m_ops.append(op);
    m_totalTime += time;
}

void MotionTimeLine::set(qreal value)
{
    Op op = { Op::Set, m_totalTime, 0, value, 0.0f };
    m_ops.append(op);
}

void MotionTimeLine::accel(qreal velocity, qreal accel)
{
    if((velocity > 0.0f) ==  (accel > 0.0f))
        accel = accel * -1.0f;

    int time = static_cast<int>(-1000 * velocity / accel);

    Op op = { Op::Accel, m_totalTime, time, velocity, accel };
    m_ops.append(op);
    m_totalTime += time;
}

// We meet the maxDistance requirement by reducing the accel
void MotionTimeLine::accel(qreal velocity, qreal accel, qreal maxDistance)
{
    Q_ASSERT(accel >= 0.0f && maxDistance >= 0.0f);

    qreal maxAccel = (3.0f * velocity * velocity) / (2.0f * maxDistance);
    if(maxAccel > accel)
        accel = maxAccel;

    if((velocity > 0.0f) ==  (accel > 0.0f))
        accel = accel * -1.0f;

    int time = static_cast<int>(-1000 * velocity / accel);

    Op op = { Op::Accel, m_totalTime, time, velocity, accel };
    m_ops.append(op);
    m_totalTime += time;
}

void MotionTimeLine::accelDistance(qreal velocity, qreal distance)
{
    Q_ASSERT((distance >= 0.0f) == (velocity >= 0.0f));

    int time = static_cast<int>(1000 * (2.0f * distance) / velocity);

    Op op = { Op::AccelDistance, m_totalTime, time, velocity, distance };
    m_ops.append(op);
    m_totalTime += time;
}

void MotionTimeLine::move(int time, qreal destination)
{
    if(time <= 0) return;
    Op op = { Op::Move, m_totalTime, time, destination, 0.0f };
    m_ops.append(op);
    m_totalTime += time;
}

void MotionTimeLine::moveBy(int time, qreal change)
{
    if(time <= 0) return;
    Op op = { Op::MoveBy, m_totalTime, time, change, 0.0f };
    m_ops.append(op);
    m_totalTime += time;
}

void MotionTimeLine::setValue(IValue *value)
{
    output = value;
}

int MotionTimeLine::endTime() const
{
    return m_totalTime;
}

int MotionTimeLine::currentTime() const
{
    return m_timelineTime;
}

int MotionTimeLine::remainingTime() const
{
    return endTime() - currentTime();
}

bool MotionTimeLine::isRunning() const
{
    return m_isRunning;
}

int MotionTimeLine::updateInterval() const
{
    return m_updateInterval;
}

void MotionTimeLine::setUpdateInterval(int interval)
{
    m_updateInterval = interval;
}

void MotionTimeLine::start(int time)
{
    stop();
    motionClock()->addTimeline(this);
    m_wallStartTime = motionClock()->currentTime();
    m_startTimeOffset = time;
    m_currentOpBaseline = output->value();
    m_isRunning = true;
    advanceTo(0);
}

void MotionTimeLine::start(int time, const QString &name)
{
    stop();
    m_name = name;
    start(time);
}

QString MotionTimeLine::runName() const
{
    return m_name;
}

void MotionTimeLine::stop(int time)
{
    if(!isRunning()) 
        return;

    m_isRunning = false;
    motionClock()->remTimeline(this);
    if(-1 != time) {
        advanceTo(time);
    }

    m_timelineTime = 0;
    m_currentOp = 0;
    m_currentOpBaseline = 0.0f;
    m_name = QString();
}

void MotionTimeLine::reset()
{
    stop();
    m_ops.clear();
    m_totalTime = 0;
}

MotionClock *MotionTimeLine::clock() 
{
    return motionClock();
}

void MotionTimeLine::setTime(const QTime &now)
{
    int newTimelineTime = m_wallStartTime.msecsTo(now) + m_startTimeOffset;
    
    advanceTo(newTimelineTime);
    
    if(m_currentOp == m_ops.count() && isRunning()) {
        // We're done
        stop();
        emit completed();
    }
}

void MotionTimeLine::advanceTo(int newTime)
{
    // Advance until we find the op in which it concludes
    while(m_currentOp < m_ops.count() &&
          (m_ops.at(m_currentOp).start + 
           m_ops.at(m_currentOp).length) <= newTime) {
        const Op &op = m_ops.at(m_currentOp);

        m_currentOpBaseline = value(op, op.start + op.length, 
                                    m_currentOpBaseline);
        ++m_currentOp;
    }

    bool changed = false;
    if(m_currentOp == m_ops.count()) {
        // If we have no more ops, set the output to the end of the last op and
        // complete
        changed = (output->value() != m_currentOpBaseline);
        output->setValue(m_currentOpBaseline);
    } else {
        // Otherwise set it to the partial value
        qreal newValue = 
            value(m_ops.at(m_currentOp), newTime, m_currentOpBaseline);
        changed = (output->value() != newValue);
        output->setValue(newValue);
    }

    m_timelineTime = newTime;
    
    if(changed)
        emit updated();
}

qreal MotionTimeLine::value(const Op &op, int time, qreal base) const
{
    Q_ASSERT(time >= op.start);
    Q_ASSERT(time <= (op.start + op.length));

    switch(op.type) {
        case Op::Pause:
            return base;
        case Op::Set:
            return op.value;
        case Op::Move:
            if(time == op.start) {
                return base;
            } else if(time == (op.start + op.length)) {
                return op.value;
            } else {
                qreal pTime = (qreal)(time - op.start) / (qreal)op.length;
                qreal delta = op.value - base;
                return base + delta * pTime;
            }
        case Op::MoveBy:
            if(time == op.start) {
                return base;
            } else if(time == (op.start + op.length)) {
                return base + op.value;
            } else {
                qreal pTime = (qreal)(time - op.start) / (qreal)op.length;
                qreal delta = op.value;
                return base + delta * pTime;
            }
        case Op::Accel:
            if(time == op.start) {
                return base;
            } else {
                qreal t = (qreal)(time - op.start) / 1000.0f;
                qreal delta = op.value * t + 0.5f * op.value2 * t * t;
                return base + delta;
            }
        case Op::AccelDistance:
            if(time == op.start) {
                return base;
            } else if(time == (op.start + op.length)) {
                return base + op.value2;
            } else {
                qreal t = (qreal)(time - op.start) / 1000.0f;
                qreal accel = -1.0f * 1000.0f * op.value / (qreal)op.length;
                qreal delta = op.value * t + 0.5f * accel * t * t;
                return base + delta;

            }
    }

    return base;
}

// define MotionValue
MotionValue::MotionValue()
: time(0.0f), start(0.0f), distance(0.0f), minimum(0.0f), maximum(0.0f)
{
}

MotionValue::MotionValue(const MotionValue &other)
: time(other.time), start(other.start),
  distance(other.distance), minimum(other.minimum), maximum(other.maximum),
  notifies(other.notifies)
{
}

MotionValue::MotionValue(qreal position)
: time(0.0f), start(position), distance(0.0f), minimum(0.0f), maximum(0.0f)
{
}

MotionValue & MotionValue::operator=(const MotionValue &other)
{
    time = other.time;
    start = other.start;
    distance = other.distance;
    minimum = other.minimum;
    maximum = other.maximum;
    notifies = other.notifies;
    return *this;
}

void MotionValue::shiftBy(qreal amount)
{
    start = start + amount;
}

void MotionValue::moveBy(qreal amount)
{
    start = value();
    distance = distance * (1.0f - time) + amount;
    time = 0.0f;
}

void MotionValue::setPosition(qreal position)
{
    start = position;
    distance = 0.0f;
    time = 0.0f;
}

void MotionValue::moveTo(qreal stop, Direction d)
{
    start = value();

    if(d == Any) {
        
        distance = stop - start;
        
    } else if(d == Up) {

        if(stop > start) {
            distance = stop - start;
        } else {
            distance = (maximum - start) + (stop - minimum);
        }

    } else if(d == Down) {

        if(stop < start) {
            distance = start - stop;
        } else {
            distance = (start - minimum) + (maximum - stop);
        }

        distance *= -1.0f;

    }

    time = 0.0f;
}

void MotionValue::setBounds(qreal minimum, qreal maximum)
{
    this->minimum = minimum;
    this->maximum = maximum;
}

void MotionValue::setTime(qreal time)
{
    Q_ASSERT(time >= 0.0f && time <= 1.0f);

    // Test for notifiers
    if(!notifies.isEmpty()) {
        qreal currentValue = value();
        qreal newValue = valueAt(time);

        for(int ii = 0; ii < notifies.count(); ++ii) {
            const Notify &notify = notifies.at(ii);

            if(maximum == 0.0f && minimum == 0.0f) {

                // Unbounded 
                if((currentValue >= notify.value &&
                    newValue < notify.value) ||
                   (currentValue <= notify.value &&
                    newValue > notify.value)) 
                    notify.callback(this, currentValue, newValue, 
                                    destinationDirection(), notify.id,
                                    notify.context);
            } else {

                // Bounded

                // May be outside range
                qreal unboundedValue = currentValue + 
                                       (time - this->time) * distance; 
                qreal range = maximum - minimum;
                qreal testingValue = currentValue;

                if(Up == destinationDirection()) {

                    Q_ASSERT(unboundedValue >= currentValue);
                    while(testingValue <= notify.value &&
                          unboundedValue > notify.value) {

                        notify.callback(this, currentValue, newValue, 
                                        Up, notify.id,
                                        notify.context);
                        unboundedValue -= range;
                        testingValue += range;
                    }
                }  else {

                    // Down == destinationDirection()
                    Q_ASSERT(unboundedValue <= currentValue);
                    while(testingValue >= notify.value && 
                          unboundedValue < notify.value) {

                        notify.callback(this, currentValue, newValue, 
                                        Down, notify.id,
                                        notify.context);

                        unboundedValue += range;
                        testingValue -= range; 
                    }
                }

            }

        }
    }

    this->time = time;
    if(time == 1.0f) {
        start = value();
        distance = 0.0f;
        time = 0.0f;
    }
}

qreal MotionValue::value() const
{
    return valueAt(time);
}

qreal MotionValue::valueAt(qreal t) const
{
    if(0.0f == distance)
        return start;

    qreal travelled = t * distance;

    qreal pos = 0.0f;

    pos = start + travelled;

    if(travelled < 0.0f) {
        if(pos < minimum && !(minimum == 0.0f && maximum == 0.0f)) {
            while(pos < minimum) 
                pos = maximum - (minimum - pos);
        } 
    } else {
        while(!(minimum == 0.0f && maximum == 0.0f) && 
              ((Down == destinationDirection() && pos > maximum) ||
               (Up == destinationDirection() && pos >= maximum)))
            pos = minimum + (pos - maximum);
    }

    return pos;
}

qreal MotionValue::value(qreal time) 
{
    setTime(time);
    return value();
}

MotionValue::Direction MotionValue::destinationDirection() const
{
    return (distance < 0.0f)?Down:Up;
}

qreal MotionValue::destination() const
{
    return valueAt(1.0f);
}

bool MotionValue::isDone() const
{
    return 0.0f == distance || time == 1.0f;
}

void MotionValue::addMotionNotify(qreal value, int id, Callback callback, 
                                 void *context)
{
    Notify n = { value, id, callback, context };
    notifies.append(n);
}


