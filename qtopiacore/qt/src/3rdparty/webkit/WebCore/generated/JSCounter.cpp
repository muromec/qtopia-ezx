/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#include "JSCounter.h"

#include <wtf/GetPtr.h>

#include "Counter.h"
#include "KURL.h"

#include <runtime/JSNumberCell.h>
#include <runtime/JSString.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSCounter)

/* Hash table */

static const HashTableValue JSCounterTableValues[5] =
{
    { "identifier", DontDelete|ReadOnly, (intptr_t)jsCounterIdentifier, (intptr_t)0 },
    { "listStyle", DontDelete|ReadOnly, (intptr_t)jsCounterListStyle, (intptr_t)0 },
    { "separator", DontDelete|ReadOnly, (intptr_t)jsCounterSeparator, (intptr_t)0 },
    { "constructor", DontEnum|ReadOnly, (intptr_t)jsCounterConstructor, (intptr_t)0 },
    { 0, 0, 0, 0 }
};

static const HashTable JSCounterTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 63, JSCounterTableValues, 0 };
#else
    { 9, 7, JSCounterTableValues, 0 };
#endif

/* Hash table for constructor */

static const HashTableValue JSCounterConstructorTableValues[1] =
{
    { 0, 0, 0, 0 }
};

static const HashTable JSCounterConstructorTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 0, JSCounterConstructorTableValues, 0 };
#else
    { 1, 0, JSCounterConstructorTableValues, 0 };
#endif

class JSCounterConstructor : public DOMObject {
public:
    JSCounterConstructor(ExecState* exec)
        : DOMObject(JSCounterConstructor::createStructure(exec->lexicalGlobalObject()->objectPrototype()))
    {
        putDirect(exec->propertyNames().prototype, JSCounterPrototype::self(exec), None);
    }
    virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
    virtual const ClassInfo* classInfo() const { return &s_info; }
    static const ClassInfo s_info;

    static PassRefPtr<Structure> createStructure(JSValuePtr proto) 
    { 
        return Structure::create(proto, TypeInfo(ObjectType, ImplementsHasInstance)); 
    }
};

const ClassInfo JSCounterConstructor::s_info = { "CounterConstructor", 0, &JSCounterConstructorTable, 0 };

bool JSCounterConstructor::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSCounterConstructor, DOMObject>(exec, &JSCounterConstructorTable, this, propertyName, slot);
}

/* Hash table for prototype */

static const HashTableValue JSCounterPrototypeTableValues[1] =
{
    { 0, 0, 0, 0 }
};

static const HashTable JSCounterPrototypeTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 0, JSCounterPrototypeTableValues, 0 };
#else
    { 1, 0, JSCounterPrototypeTableValues, 0 };
#endif

const ClassInfo JSCounterPrototype::s_info = { "CounterPrototype", 0, &JSCounterPrototypeTable, 0 };

JSObject* JSCounterPrototype::self(ExecState* exec)
{
    return getDOMPrototype<JSCounter>(exec);
}

const ClassInfo JSCounter::s_info = { "Counter", 0, &JSCounterTable, 0 };

JSCounter::JSCounter(PassRefPtr<Structure> structure, PassRefPtr<Counter> impl)
    : DOMObject(structure)
    , m_impl(impl)
{
}

JSCounter::~JSCounter()
{
    forgetDOMObject(*Heap::heap(this)->globalData(), m_impl.get());

}

JSObject* JSCounter::createPrototype(ExecState* exec)
{
    return new (exec) JSCounterPrototype(JSCounterPrototype::createStructure(exec->lexicalGlobalObject()->objectPrototype()));
}

bool JSCounter::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSCounter, Base>(exec, &JSCounterTable, this, propertyName, slot);
}

JSValuePtr jsCounterIdentifier(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    Counter* imp = static_cast<Counter*>(static_cast<JSCounter*>(asObject(slot.slotBase()))->impl());
    return jsString(exec, imp->identifier());
}

JSValuePtr jsCounterListStyle(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    Counter* imp = static_cast<Counter*>(static_cast<JSCounter*>(asObject(slot.slotBase()))->impl());
    return jsString(exec, imp->listStyle());
}

JSValuePtr jsCounterSeparator(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    Counter* imp = static_cast<Counter*>(static_cast<JSCounter*>(asObject(slot.slotBase()))->impl());
    return jsString(exec, imp->separator());
}

JSValuePtr jsCounterConstructor(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    return static_cast<JSCounter*>(asObject(slot.slotBase()))->getConstructor(exec);
}
JSValuePtr JSCounter::getConstructor(ExecState* exec)
{
    return getDOMConstructor<JSCounterConstructor>(exec);
}

JSC::JSValuePtr toJS(JSC::ExecState* exec, Counter* object)
{
    return getDOMObjectWrapper<JSCounter>(exec, object);
}
Counter* toCounter(JSC::JSValuePtr value)
{
    return value->isObject(&JSCounter::s_info) ? static_cast<JSCounter*>(asObject(value))->impl() : 0;
}

}
