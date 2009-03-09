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

#include "JSXMLSerializer.h"

#include <wtf/GetPtr.h>

#include "JSNode.h"
#include "KURL.h"
#include "XMLSerializer.h"

#include <runtime/Error.h>
#include <runtime/JSNumberCell.h>
#include <runtime/JSString.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSXMLSerializer)

/* Hash table */

static const HashTableValue JSXMLSerializerTableValues[2] =
{
    { "constructor", DontEnum|ReadOnly, (intptr_t)jsXMLSerializerConstructor, (intptr_t)0 },
    { 0, 0, 0, 0 }
};

static const HashTable JSXMLSerializerTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 0, JSXMLSerializerTableValues, 0 };
#else
    { 2, 1, JSXMLSerializerTableValues, 0 };
#endif

/* Hash table for constructor */

static const HashTableValue JSXMLSerializerConstructorTableValues[1] =
{
    { 0, 0, 0, 0 }
};

static const HashTable JSXMLSerializerConstructorTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 0, JSXMLSerializerConstructorTableValues, 0 };
#else
    { 1, 0, JSXMLSerializerConstructorTableValues, 0 };
#endif

class JSXMLSerializerConstructor : public DOMObject {
public:
    JSXMLSerializerConstructor(ExecState* exec)
        : DOMObject(JSXMLSerializerConstructor::createStructure(exec->lexicalGlobalObject()->objectPrototype()))
    {
        putDirect(exec->propertyNames().prototype, JSXMLSerializerPrototype::self(exec), None);
    }
    virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
    virtual const ClassInfo* classInfo() const { return &s_info; }
    static const ClassInfo s_info;

    static PassRefPtr<Structure> createStructure(JSValuePtr proto) 
    { 
        return Structure::create(proto, TypeInfo(ObjectType, ImplementsHasInstance)); 
    }
    static JSObject* construct(ExecState* exec, JSObject*, const ArgList&)
    {
        return asObject(toJS(exec, XMLSerializer::create()));
    }
    virtual ConstructType getConstructData(ConstructData& constructData)
    {
        constructData.native.function = construct;
        return ConstructTypeHost;
    }
};

const ClassInfo JSXMLSerializerConstructor::s_info = { "XMLSerializerConstructor", 0, &JSXMLSerializerConstructorTable, 0 };

bool JSXMLSerializerConstructor::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSXMLSerializerConstructor, DOMObject>(exec, &JSXMLSerializerConstructorTable, this, propertyName, slot);
}

/* Hash table for prototype */

static const HashTableValue JSXMLSerializerPrototypeTableValues[2] =
{
    { "serializeToString", DontDelete|Function, (intptr_t)jsXMLSerializerPrototypeFunctionSerializeToString, (intptr_t)1 },
    { 0, 0, 0, 0 }
};

static const HashTable JSXMLSerializerPrototypeTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 0, JSXMLSerializerPrototypeTableValues, 0 };
#else
    { 2, 1, JSXMLSerializerPrototypeTableValues, 0 };
#endif

const ClassInfo JSXMLSerializerPrototype::s_info = { "XMLSerializerPrototype", 0, &JSXMLSerializerPrototypeTable, 0 };

JSObject* JSXMLSerializerPrototype::self(ExecState* exec)
{
    return getDOMPrototype<JSXMLSerializer>(exec);
}

bool JSXMLSerializerPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<JSObject>(exec, &JSXMLSerializerPrototypeTable, this, propertyName, slot);
}

const ClassInfo JSXMLSerializer::s_info = { "XMLSerializer", 0, &JSXMLSerializerTable, 0 };

JSXMLSerializer::JSXMLSerializer(PassRefPtr<Structure> structure, PassRefPtr<XMLSerializer> impl)
    : DOMObject(structure)
    , m_impl(impl)
{
}

JSXMLSerializer::~JSXMLSerializer()
{
    forgetDOMObject(*Heap::heap(this)->globalData(), m_impl.get());

}

JSObject* JSXMLSerializer::createPrototype(ExecState* exec)
{
    return new (exec) JSXMLSerializerPrototype(JSXMLSerializerPrototype::createStructure(exec->lexicalGlobalObject()->objectPrototype()));
}

bool JSXMLSerializer::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSXMLSerializer, Base>(exec, &JSXMLSerializerTable, this, propertyName, slot);
}

JSValuePtr jsXMLSerializerConstructor(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    return static_cast<JSXMLSerializer*>(asObject(slot.slotBase()))->getConstructor(exec);
}
JSValuePtr JSXMLSerializer::getConstructor(ExecState* exec)
{
    return getDOMConstructor<JSXMLSerializerConstructor>(exec);
}

JSValuePtr jsXMLSerializerPrototypeFunctionSerializeToString(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSXMLSerializer::s_info))
        return throwError(exec, TypeError);
    JSXMLSerializer* castedThisObj = static_cast<JSXMLSerializer*>(asObject(thisValue));
    XMLSerializer* imp = static_cast<XMLSerializer*>(castedThisObj->impl());
    ExceptionCode ec = 0;
    Node* node = toNode(args.at(exec, 0));


    JSC::JSValuePtr result = jsString(exec, imp->serializeToString(node, ec));
    setDOMException(exec, ec);
    return result;
}

JSC::JSValuePtr toJS(JSC::ExecState* exec, XMLSerializer* object)
{
    return getDOMObjectWrapper<JSXMLSerializer>(exec, object);
}
XMLSerializer* toXMLSerializer(JSC::JSValuePtr value)
{
    return value->isObject(&JSXMLSerializer::s_info) ? static_cast<JSXMLSerializer*>(asObject(value))->impl() : 0;
}

}
