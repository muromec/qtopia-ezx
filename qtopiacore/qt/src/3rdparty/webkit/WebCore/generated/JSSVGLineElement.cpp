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
#include "JSSVGLineElement.h"

#include <wtf/GetPtr.h>

#include "CSSMutableStyleDeclaration.h"
#include "CSSStyleDeclaration.h"
#include "CSSValue.h"
#include "JSCSSStyleDeclaration.h"
#include "JSCSSValue.h"
#include "JSSVGAnimatedBoolean.h"
#include "JSSVGAnimatedLength.h"
#include "JSSVGAnimatedString.h"
#include "JSSVGAnimatedTransformList.h"
#include "JSSVGElement.h"
#include "JSSVGMatrix.h"
#include "JSSVGRect.h"
#include "JSSVGStringList.h"
#include "KURL.h"
#include "SVGElement.h"
#include "SVGLineElement.h"
#include "SVGStringList.h"

#include <runtime/Error.h>
#include <runtime/JSString.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSSVGLineElement)

/* Hash table */

static const HashTableValue JSSVGLineElementTableValues[16] =
{
    { "x1", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementX1, (intptr_t)0 },
    { "y1", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementY1, (intptr_t)0 },
    { "x2", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementX2, (intptr_t)0 },
    { "y2", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementY2, (intptr_t)0 },
    { "requiredFeatures", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementRequiredFeatures, (intptr_t)0 },
    { "requiredExtensions", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementRequiredExtensions, (intptr_t)0 },
    { "systemLanguage", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementSystemLanguage, (intptr_t)0 },
    { "xmllang", DontDelete, (intptr_t)jsSVGLineElementXmllang, (intptr_t)setJSSVGLineElementXmllang },
    { "xmlspace", DontDelete, (intptr_t)jsSVGLineElementXmlspace, (intptr_t)setJSSVGLineElementXmlspace },
    { "externalResourcesRequired", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementExternalResourcesRequired, (intptr_t)0 },
    { "className", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementClassName, (intptr_t)0 },
    { "style", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementStyle, (intptr_t)0 },
    { "transform", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementTransform, (intptr_t)0 },
    { "nearestViewportElement", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementNearestViewportElement, (intptr_t)0 },
    { "farthestViewportElement", DontDelete|ReadOnly, (intptr_t)jsSVGLineElementFarthestViewportElement, (intptr_t)0 },
    { 0, 0, 0, 0 }
};

static const HashTable JSSVGLineElementTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 1023, JSSVGLineElementTableValues, 0 };
#else
    { 35, 31, JSSVGLineElementTableValues, 0 };
#endif

/* Hash table for prototype */

static const HashTableValue JSSVGLineElementPrototypeTableValues[7] =
{
    { "hasExtension", DontDelete|Function, (intptr_t)jsSVGLineElementPrototypeFunctionHasExtension, (intptr_t)1 },
    { "getPresentationAttribute", DontDelete|Function, (intptr_t)jsSVGLineElementPrototypeFunctionGetPresentationAttribute, (intptr_t)1 },
    { "getBBox", DontDelete|Function, (intptr_t)jsSVGLineElementPrototypeFunctionGetBBox, (intptr_t)0 },
    { "getCTM", DontDelete|Function, (intptr_t)jsSVGLineElementPrototypeFunctionGetCTM, (intptr_t)0 },
    { "getScreenCTM", DontDelete|Function, (intptr_t)jsSVGLineElementPrototypeFunctionGetScreenCTM, (intptr_t)0 },
    { "getTransformToElement", DontDelete|Function, (intptr_t)jsSVGLineElementPrototypeFunctionGetTransformToElement, (intptr_t)1 },
    { 0, 0, 0, 0 }
};

static const HashTable JSSVGLineElementPrototypeTable =
#if ENABLE(PERFECT_HASH_SIZE)
    { 2047, JSSVGLineElementPrototypeTableValues, 0 };
#else
    { 17, 15, JSSVGLineElementPrototypeTableValues, 0 };
#endif

const ClassInfo JSSVGLineElementPrototype::s_info = { "SVGLineElementPrototype", 0, &JSSVGLineElementPrototypeTable, 0 };

JSObject* JSSVGLineElementPrototype::self(ExecState* exec)
{
    return getDOMPrototype<JSSVGLineElement>(exec);
}

bool JSSVGLineElementPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<JSObject>(exec, &JSSVGLineElementPrototypeTable, this, propertyName, slot);
}

const ClassInfo JSSVGLineElement::s_info = { "SVGLineElement", &JSSVGElement::s_info, &JSSVGLineElementTable, 0 };

JSSVGLineElement::JSSVGLineElement(PassRefPtr<Structure> structure, PassRefPtr<SVGLineElement> impl)
    : JSSVGElement(structure, impl)
{
}

JSObject* JSSVGLineElement::createPrototype(ExecState* exec)
{
    return new (exec) JSSVGLineElementPrototype(JSSVGLineElementPrototype::createStructure(JSSVGElementPrototype::self(exec)));
}

bool JSSVGLineElement::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSSVGLineElement, Base>(exec, &JSSVGLineElementTable, this, propertyName, slot);
}

JSValuePtr jsSVGLineElementX1(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedLength> obj = imp->x1Animated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGLineElementY1(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedLength> obj = imp->y1Animated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGLineElementX2(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedLength> obj = imp->x2Animated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGLineElementY2(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedLength> obj = imp->y2Animated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGLineElementRequiredFeatures(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->requiredFeatures()), imp);
}

JSValuePtr jsSVGLineElementRequiredExtensions(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->requiredExtensions()), imp);
}

JSValuePtr jsSVGLineElementSystemLanguage(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->systemLanguage()), imp);
}

JSValuePtr jsSVGLineElementXmllang(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    return jsString(exec, imp->xmllang());
}

JSValuePtr jsSVGLineElementXmlspace(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    return jsString(exec, imp->xmlspace());
}

JSValuePtr jsSVGLineElementExternalResourcesRequired(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedBoolean> obj = imp->externalResourcesRequiredAnimated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGLineElementClassName(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedString> obj = imp->classNameAnimated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGLineElementStyle(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->style()));
}

JSValuePtr jsSVGLineElementTransform(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    RefPtr<SVGAnimatedTransformList> obj = imp->transformAnimated();
    return toJS(exec, obj.get(), imp);
}

JSValuePtr jsSVGLineElementNearestViewportElement(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->nearestViewportElement()));
}

JSValuePtr jsSVGLineElementFarthestViewportElement(ExecState* exec, const Identifier&, const PropertySlot& slot)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(asObject(slot.slotBase()))->impl());
    return toJS(exec, WTF::getPtr(imp->farthestViewportElement()));
}

void JSSVGLineElement::put(ExecState* exec, const Identifier& propertyName, JSValuePtr value, PutPropertySlot& slot)
{
    lookupPut<JSSVGLineElement, Base>(exec, propertyName, value, &JSSVGLineElementTable, this, slot);
}

void setJSSVGLineElementXmllang(ExecState* exec, JSObject* thisObject, JSValuePtr value)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(thisObject)->impl());
    imp->setXmllang(value->toString(exec));
}

void setJSSVGLineElementXmlspace(ExecState* exec, JSObject* thisObject, JSValuePtr value)
{
    SVGLineElement* imp = static_cast<SVGLineElement*>(static_cast<JSSVGLineElement*>(thisObject)->impl());
    imp->setXmlspace(value->toString(exec));
}

JSValuePtr jsSVGLineElementPrototypeFunctionHasExtension(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGLineElement::s_info))
        return throwError(exec, TypeError);
    JSSVGLineElement* castedThisObj = static_cast<JSSVGLineElement*>(asObject(thisValue));
    SVGLineElement* imp = static_cast<SVGLineElement*>(castedThisObj->impl());
    const UString& extension = args.at(exec, 0)->toString(exec);


    JSC::JSValuePtr result = jsBoolean(imp->hasExtension(extension));
    return result;
}

JSValuePtr jsSVGLineElementPrototypeFunctionGetPresentationAttribute(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGLineElement::s_info))
        return throwError(exec, TypeError);
    JSSVGLineElement* castedThisObj = static_cast<JSSVGLineElement*>(asObject(thisValue));
    SVGLineElement* imp = static_cast<SVGLineElement*>(castedThisObj->impl());
    const UString& name = args.at(exec, 0)->toString(exec);


    JSC::JSValuePtr result = toJS(exec, WTF::getPtr(imp->getPresentationAttribute(name)));
    return result;
}

JSValuePtr jsSVGLineElementPrototypeFunctionGetBBox(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGLineElement::s_info))
        return throwError(exec, TypeError);
    JSSVGLineElement* castedThisObj = static_cast<JSSVGLineElement*>(asObject(thisValue));
    SVGLineElement* imp = static_cast<SVGLineElement*>(castedThisObj->impl());


    JSC::JSValuePtr result = toJS(exec, JSSVGStaticPODTypeWrapper<FloatRect>::create(imp->getBBox()).get(), imp);
    return result;
}

JSValuePtr jsSVGLineElementPrototypeFunctionGetCTM(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGLineElement::s_info))
        return throwError(exec, TypeError);
    JSSVGLineElement* castedThisObj = static_cast<JSSVGLineElement*>(asObject(thisValue));
    SVGLineElement* imp = static_cast<SVGLineElement*>(castedThisObj->impl());


    JSC::JSValuePtr result = toJS(exec, JSSVGStaticPODTypeWrapper<TransformationMatrix>::create(imp->getCTM()).get(), imp);
    return result;
}

JSValuePtr jsSVGLineElementPrototypeFunctionGetScreenCTM(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGLineElement::s_info))
        return throwError(exec, TypeError);
    JSSVGLineElement* castedThisObj = static_cast<JSSVGLineElement*>(asObject(thisValue));
    SVGLineElement* imp = static_cast<SVGLineElement*>(castedThisObj->impl());


    JSC::JSValuePtr result = toJS(exec, JSSVGStaticPODTypeWrapper<TransformationMatrix>::create(imp->getScreenCTM()).get(), imp);
    return result;
}

JSValuePtr jsSVGLineElementPrototypeFunctionGetTransformToElement(ExecState* exec, JSObject*, JSValuePtr thisValue, const ArgList& args)
{
    if (!thisValue->isObject(&JSSVGLineElement::s_info))
        return throwError(exec, TypeError);
    JSSVGLineElement* castedThisObj = static_cast<JSSVGLineElement*>(asObject(thisValue));
    SVGLineElement* imp = static_cast<SVGLineElement*>(castedThisObj->impl());
    ExceptionCode ec = 0;
    SVGElement* element = toSVGElement(args.at(exec, 0));


    JSC::JSValuePtr result = toJS(exec, JSSVGStaticPODTypeWrapper<TransformationMatrix>::create(imp->getTransformToElement(element, ec)).get(), imp);
    setDOMException(exec, ec);
    return result;
}


}

#endif // ENABLE(SVG)
