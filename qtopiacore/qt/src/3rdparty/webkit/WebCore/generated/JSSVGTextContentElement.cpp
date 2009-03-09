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


#if ENABLE(SVG)

#include "SVGElement.h"
#include "JSSVGTextContentElement.h"

#include <wtf/GetPtr.h>

#include "CSSMutableStyleDeclaration.h"
#include "CSSStyleDeclaration.h"
#include "CSSValue.h"
#include "FloatPoint.h"
#include "JSCSSStyleDeclaration.h"
#include "JSCSSValue.h"
#include "JSSVGAnimatedBoolean.h"
#include "JSSVGAnimatedEnumeration.h"
#include "JSSVGAnimatedLength.h"
#include "JSSVGAnimatedString.h"
#include "JSSVGPoint.h"
#include "JSSVGRect.h"
#include "JSSVGStringList.h"
#include "KURL.h"
#include "SVGStringList.h"
#include "SVGTextContentElement.h"

#include <runtime/Error.h>
#include <runtime/JSNumberCell.h>
#include <runtime/JSString.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSSVGTextContentElement)

/* Hash table */

static const HashTableValue JSSVGTextContentElementTableValues[12] =
{
    { "textLength", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementTextLength, (intptr_t)0 },
    { "lengthAdjust", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementLengthAdjust, (intptr_t)0 },
    { "requiredFeatures", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementRequiredFeatures, (intptr_t)0 },
    { "requiredExtensions", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementRequiredExtensions, (intptr_t)0 },
    { "systemLanguage", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementSystemLanguage, (intptr_t)0 },
    { "xmllang", DontDelete, (intptr_t)jsSVGTextContentElementXmllang, (intptr_t)setJSSVGTextContentElementXmllang },
    { "xmlspace", DontDelete, (intptr_t)jsSVGTextContentElementXmlspace, (intptr_t)setJSSVGTextContentElementXmlspace },
    { "externalResourcesRequired", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementExternalResourcesRequired, (intptr_t)0 },
    { "className", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementClassName, (intptr_t)0 },
    { "style", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementStyle, (intptr_t)0 },
    { "constructor", DontEnum|ReadOnly, (intptr_t)jsSVGTextContentElementConstructor, (intptr_t)0 },
    { 0, 0, 0, 0 }
};

static const HashTable JSSVGTextContentElementTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 127, JSSVGTextContentElementTableValues, 0 };
#else
    { 34, 31, JSSVGTextContentElementTableValues, 0 };
#endif

/* Hash table for constructor */

static const HashTableValue JSSVGTextContentElementConstructorTableValues[4] =
{
    { "LENGTHADJUST_UNKNOWN", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementLENGTHADJUST_UNKNOWN, (intptr_t)0 },
    { "LENGTHADJUST_SPACING", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementLENGTHADJUST_SPACING, (intptr_t)0 },
    { "LENGTHADJUST_SPACINGANDGLYPHS", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementLENGTHADJUST_SPACINGANDGLYPHS, (intptr_t)0 },
    { 0, 0, 0, 0 }
};

static const HashTable JSSVGTextContentElementConstructorTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 7, JSSVGTextContentElementConstructorTableValues, 0 };
#else
    { 8, 7, JSSVGTextContentElementConstructorTableValues, 0 };
#endif

class JSSVGTextContentElementConstructor : public DOMObject {
public:
    JSSVGTextContentElementConstructor(ExecState* exec)
        : DOMObject(JSSVGTextContentElementConstructor::createStructure(exec->lexicalGlobalObject()->objectPrototype()))
    {
        putDirect(exec->propertyNames().prototype, JSSVGTextContentElementPrototype::self(exec), None);
    }
    virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
    virtual const ClassInfo* classInfo() const { return &s_info; }
    static const ClassInfo s_info;

    static PassRefPtr<Structure> createStructure(JSValuePtr proto) 
    { 
        return Structure::create(proto, TypeInfo(ObjectType, ImplementsHasInstance)); 
    }
};

const ClassInfo JSSVGTextContentElementConstructor::s_info = { "SVGTextContentElementConstructor", 0, &JSSVGTextContentElementConstructorTable, 0 };

bool JSSVGTextContentElementConstructor::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSSVGTextContentElementConstructor, DOMObject>(exec, &JSSVGTextContentElementConstructorTable, this, propertyName, slot);
}

/* Hash table for prototype */

static const HashTableValue JSSVGTextContentElementPrototypeTableValues[15] =
{
    { "LENGTHADJUST_UNKNOWN", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementLENGTHADJUST_UNKNOWN, (intptr_t)0 },
    { "LENGTHADJUST_SPACING", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementLENGTHADJUST_SPACING, (intptr_t)0 },
    { "LENGTHADJUST_SPACINGANDGLYPHS", DontDelete|ReadOnly, (intptr_t)jsSVGTextContentElementLENGTHADJUST_SPACINGANDGLYPHS, (intptr_t)0 },
    { "getNumberOfChars", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetNumberOfChars, (intptr_t)0 },
    { "getComputedTextLength", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetComputedTextLength, (intptr_t)0 },
    { "getSubStringLength", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetSubStringLength, (intptr_t)2 },
    { "getStartPositionOfChar", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetStartPositionOfChar, (intptr_t)1 },
    { "getEndPositionOfChar", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetEndPositionOfChar, (intptr_t)1 },
    { "getExtentOfChar", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetExtentOfChar, (intptr_t)1 },
    { "getRotationOfChar", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetRotationOfChar, (intptr_t)1 },
    { "getCharNumAtPosition", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetCharNumAtPosition, (intptr_t)1 },
    { "selectSubString", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionSelectSubString, (intptr_t)2 },
    { "hasExtension", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionHasExtension, (intptr_t)1 },
    { "getPresentationAttribute", DontDelete|Function, (intptr_t)jsSVGTextContentElementPrototypeFunctionGetPresentationAttribute, (intptr_t)1 },
    { 0, 0, 0, 0 }
};

static const HashTable JSSVGTextContentElementPrototypeTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 127, JSSVGTextContentElementPrototypeTableValues, 0 };
#else
    { 34, 31, JSSVGTextContentElementPrototypeTableValues, 0 };
#endif

const ClassInfo JSSVGTextContentElementPrototype::s_info = { "SVGTextContentElementPrototype", 0, &JSSVGTextContentElementPrototypeTable, 0 };

JSObject* JSSVGTextContentElementPrototype::self(ExecState* exec)
{
    return getDOMPrototype<JSSVGTextContentElement>(exec);
}

bool JSSVGTextContentElementPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticPropertySlot<JSSVGTextContentElementPrototype, JSObject>(exec, &JSSVGTextContentElementPrototypeTable, this, propertyName, slot);
}

const ClassInfo JSSVGTextContentElement::s_info = { "SVGTextContentElement", &JSSVGElement::s_info, &JSSVGTextContentElementTable, 0 };

JSSVGTextContentElement::JSSVGTextContentElement(PassRefPtr<Structure> structure, PassRefPtr<SVGTextContentElement> impl)
    : JSSVGElement(structure, impl)
{
}

JSObject* JSSVGTextContentElement::createPrototype(ExecState* exec)
{
    return new (exec) JSSVGTextContentElementPrototype(JSSVGTextContentElementPrototype::createStructure(JSSVGElementPrototype::self(exec)));
}

bool JSSVGTextContentElement::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSSVGTextContentElement, Base>(exec, &JSSVGTextContentElementTable, this, propertyName, slot);
}

JSValuePtr jsSVGTextContentElementTextLength(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedLength> obj = imp->textLengthAnimated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGTextContentElementLengthAdjust(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedEnumeration> obj = imp->lengthAdjustAnimated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGTextContentElementRequiredFeatures(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->requiredFeatures()), imp);
}

JSValuePtr jsSVGTextContentElementRequiredExtensions(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->requiredExtensions()), imp);
}

JSValuePtr jsSVGTextContentElementSystemLanguage(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->systemLanguage()), imp);
}

JSValuePtr jsSVGTextContentElementXmllang(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    return jsString(exec, imp->xmllang());
}

JSValuePtr jsSVGTextContentElementXmlspace(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    return jsString(exec, imp->xmlspace());
}

JSValuePtr jsSVGTextContentElementExternalResourcesRequired(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedBoolean> obj = imp->externalResourcesRequiredAnimated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGTextContentElementClassName(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedString> obj = imp->classNameAnimated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGTextContentElementStyle(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->style()));
}

JSValuePtr jsSVGTextContentElementConstructor(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    return static_cast<JSSVGTextContentElement*>(asObject(slot.slotBase()))->getConstructor(exec);
}
void JSSVGTextContentElement::put(ExecState* exec, const Identifier& propertyName, JSValuePtr value, PutPropertySlot& slot)
{
    lookupPut<JSSVGTextContentElement, Base>(exec, propertyName, value, &JSSVGTextContentElementTable, this, slot);
}

void setJSSVGTextContentElementXmllang(ExecState* exec, JSObject* thisObject, JSValuePtr value)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(thisObject)->impl());
    imp->setXmllang(value->toString(exec));
}

void setJSSVGTextContentElementXmlspace(ExecState* exec, JSObject* thisObject, JSValuePtr value)
{
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(static_cast<JSSVGTextContentElement*>(thisObject)->impl());
    imp->setXmlspace(value->toString(exec));
}

JSValuePtr JSSVGTextContentElement::getConstructor(ExecState* exec)
{
    return getDOMConstructor<JSSVGTextContentElementConstructor>(exec);
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetNumberOfChars(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());


    JSC::JSValuePtr result = jsNumber(exec, imp->getNumberOfChars());
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetComputedTextLength(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());


    JSC::JSValuePtr result = jsNumber(exec, imp->getComputedTextLength());
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetSubStringLength(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    ExceptionCode ec = 0;
    unsigned offset = args.at(exec, 0)->toInt32(exec);
    unsigned length = args.at(exec, 1)->toInt32(exec);


    JSC::JSValuePtr result = jsNumber(exec, imp->getSubStringLength(offset, length, ec));
    setDOMException(exec, ec);
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetStartPositionOfChar(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    ExceptionCode ec = 0;
    unsigned offset = args.at(exec, 0)->toInt32(exec);


    JSC::JSValuePtr result = toJS(exec, JSSVGStaticPODTypeWrapper<FloatPoint>::create(imp->getStartPositionOfChar(offset, ec)).get(), imp);
    setDOMException(exec, ec);
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetEndPositionOfChar(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    ExceptionCode ec = 0;
    unsigned offset = args.at(exec, 0)->toInt32(exec);


    JSC::JSValuePtr result = toJS(exec, JSSVGStaticPODTypeWrapper<FloatPoint>::create(imp->getEndPositionOfChar(offset, ec)).get(), imp);
    setDOMException(exec, ec);
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetExtentOfChar(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    ExceptionCode ec = 0;
    unsigned offset = args.at(exec, 0)->toInt32(exec);


    JSC::JSValuePtr result = toJS(exec, JSSVGStaticPODTypeWrapper<FloatRect>::create(imp->getExtentOfChar(offset, ec)).get(), imp);
    setDOMException(exec, ec);
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetRotationOfChar(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    ExceptionCode ec = 0;
    unsigned offset = args.at(exec, 0)->toInt32(exec);


    JSC::JSValuePtr result = jsNumber(exec, imp->getRotationOfChar(offset, ec));
    setDOMException(exec, ec);
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetCharNumAtPosition(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    FloatPoint point = toSVGPoint(args.at(exec, 0));


    JSC::JSValuePtr result = jsNumber(exec, imp->getCharNumAtPosition(point));
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionSelectSubString(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    ExceptionCode ec = 0;
    unsigned offset = args.at(exec, 0)->toInt32(exec);
    unsigned length = args.at(exec, 1)->toInt32(exec);

    imp->selectSubString(offset, length, ec);
    setDOMException(exec, ec);
    return jsUndefined();
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionHasExtension(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    const UString& extension = args.at(exec, 0)->toString(exec);


    JSC::JSValuePtr result = jsBoolean(imp->hasExtension(extension));
    return result;
}

JSValuePtr jsSVGTextContentElementPrototypeFunctionGetPresentationAttribute(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGTextContentElement::s_info))
        return throwError(exec, TypeError);
    JSSVGTextContentElement* castedThisObj = static_cast<JSSVGTextContentElement*>(asObject(thisValue));
    SVGTextContentElement* imp = static_cast<SVGTextContentElement*>(castedThisObj->impl());
    const UString& name = args.at(exec, 0)->toString(exec);


    JSC::JSValuePtr result = toJS(exec, WTF::getPtr(imp->getPresentationAttribute(name)));
    return result;
}

// Constant getters

JSValuePtr jsSVGTextContentElementLENGTHADJUST_UNKNOWN(ExecState* exec, const Identifier&, const PropertySlot&)
{
    return jsNumber(exec, static_cast<int>(0));
}

JSValuePtr jsSVGTextContentElementLENGTHADJUST_SPACING(ExecState* exec, const Identifier&, const PropertySlot&)
{
    return jsNumber(exec, static_cast<int>(1));
}

JSValuePtr jsSVGTextContentElementLENGTHADJUST_SPACINGANDGLYPHS(ExecState* exec, const Identifier&, const PropertySlot&)
{
    return jsNumber(exec, static_cast<int>(2));
}


}

#endif // ENABLE(SVG)
