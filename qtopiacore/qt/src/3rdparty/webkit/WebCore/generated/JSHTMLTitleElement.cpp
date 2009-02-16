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

#include "JSHTMLTitleElement.h"

#include <wtf/GetPtr.h>

#include "HTMLTitleElement.h"
#include "PlatformString.h"

using namespace KJS;

namespace WebCore {

/* Hash table */

static const HashEntry JSHTMLTitleElementTableEntries[] =
{
    { "text", JSHTMLTitleElement::TextAttrNum, DontDelete, 0, 0 },
    { "constructor", JSHTMLTitleElement::ConstructorAttrNum, DontDelete|DontEnum|ReadOnly, 0, 0 }
};

static const HashTable JSHTMLTitleElementTable = 
{
    2, 2, JSHTMLTitleElementTableEntries, 2
};

/* Hash table for constructor */

static const HashEntry JSHTMLTitleElementConstructorTableEntries[] =
{
    { 0, 0, 0, 0, 0 }
};

static const HashTable JSHTMLTitleElementConstructorTable = 
{
    2, 1, JSHTMLTitleElementConstructorTableEntries, 1
};

class JSHTMLTitleElementConstructor : public DOMObject {
public:
    JSHTMLTitleElementConstructor(ExecState* exec)
    {
        setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
        putDirect(exec->propertyNames().prototype, JSHTMLTitleElementPrototype::self(exec), None);
    }
    virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
    JSValue* getValueProperty(ExecState*, int token) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    virtual bool implementsHasInstance() const { return true; }
};

const ClassInfo JSHTMLTitleElementConstructor::info = { "HTMLTitleElementConstructor", 0, &JSHTMLTitleElementConstructorTable, 0 };

bool JSHTMLTitleElementConstructor::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSHTMLTitleElementConstructor, DOMObject>(exec, &JSHTMLTitleElementConstructorTable, this, propertyName, slot);
}

JSValue* JSHTMLTitleElementConstructor::getValueProperty(ExecState*, int token) const
{
    // The token is the numeric value of its associated constant
    return jsNumber(token);
}

/* Hash table for prototype */

static const HashEntry JSHTMLTitleElementPrototypeTableEntries[] =
{
    { 0, 0, 0, 0, 0 }
};

static const HashTable JSHTMLTitleElementPrototypeTable = 
{
    2, 1, JSHTMLTitleElementPrototypeTableEntries, 1
};

const ClassInfo JSHTMLTitleElementPrototype::info = { "HTMLTitleElementPrototype", 0, &JSHTMLTitleElementPrototypeTable, 0 };

JSObject* JSHTMLTitleElementPrototype::self(ExecState* exec)
{
    return KJS::cacheGlobalObject<JSHTMLTitleElementPrototype>(exec, "[[JSHTMLTitleElement.prototype]]");
}

const ClassInfo JSHTMLTitleElement::info = { "HTMLTitleElement", &JSHTMLElement::info, &JSHTMLTitleElementTable, 0 };

JSHTMLTitleElement::JSHTMLTitleElement(ExecState* exec, HTMLTitleElement* impl)
    : JSHTMLElement(exec, impl)
{
    setPrototype(JSHTMLTitleElementPrototype::self(exec));
}

bool JSHTMLTitleElement::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSHTMLTitleElement, JSHTMLElement>(exec, &JSHTMLTitleElementTable, this, propertyName, slot);
}

JSValue* JSHTMLTitleElement::getValueProperty(ExecState* exec, int token) const
{
    switch (token) {
    case TextAttrNum: {
        HTMLTitleElement* imp = static_cast<HTMLTitleElement*>(impl());

        return jsString(imp->text());
    }
    case ConstructorAttrNum:
        return getConstructor(exec);
    }
    return 0;
}

void JSHTMLTitleElement::put(ExecState* exec, const Identifier& propertyName, JSValue* value, int attr)
{
    lookupPut<JSHTMLTitleElement, JSHTMLElement>(exec, propertyName, value, attr, &JSHTMLTitleElementTable, this);
}

void JSHTMLTitleElement::putValueProperty(ExecState* exec, int token, JSValue* value, int /*attr*/)
{
    switch (token) {
    case TextAttrNum: {
        HTMLTitleElement* imp = static_cast<HTMLTitleElement*>(impl());

        imp->setText(valueToStringWithNullCheck(exec, value));
        break;
    }
    }
}

JSValue* JSHTMLTitleElement::getConstructor(ExecState* exec)
{
    return KJS::cacheGlobalObject<JSHTMLTitleElementConstructor>(exec, "[[HTMLTitleElement.constructor]]");
}

}