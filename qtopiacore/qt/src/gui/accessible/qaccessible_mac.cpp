/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qaccessible.h"

#ifndef QT_NO_ACCESSIBILITY
#include "qhash.h"
#include "qset.h"
#include "qpointer.h"
#include "qapplication.h"
#include "qmainwindow.h"
#include "qtextdocument.h"
#include "qdebug.h"
#include "qabstractslider.h"
#include "qsplitter.h"

#include <private/qt_mac_p.h>
#include <CoreFoundation/CoreFoundation.h>

/*****************************************************************************
  Externals
 *****************************************************************************/
extern bool qt_mac_is_macsheet(const QWidget *w); //qwidget_mac.cpp
extern bool qt_mac_is_macdrawer(const QWidget *w); //qwidget_mac.cpp

/*****************************************************************************
  QAccessible Bindings
 *****************************************************************************/
//hardcoded bindings between control info and (known) QWidgets
struct QAccessibleTextBinding {
    int qt;
    CFStringRef mac;
    bool settable;
} text_bindings[][10] = {
    { { QAccessible::MenuItem, kAXMenuItemRole, false },
      { -1, 0, false }
    },
    { { QAccessible::MenuBar, kAXMenuBarRole, false },
      { -1, 0, false }
    },
    { { QAccessible::ScrollBar, kAXScrollBarRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Grip, kAXGrowAreaRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Window, kAXWindowRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Dialog, kAXWindowRole, false },
      { -1, 0, false }
    },
    { { QAccessible::AlertMessage, kAXWindowRole, false },
      { -1, 0, false }
    },
    { { QAccessible::ToolTip, kAXWindowRole, false },
      { -1, 0, false }
    },
    { { QAccessible::HelpBalloon, kAXWindowRole, false },
      { -1, 0, false }
    },
    { { QAccessible::PopupMenu, kAXMenuRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Application, kAXApplicationRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Pane, kAXGroupRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Grouping, kAXGroupRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Separator, kAXSplitterRole, false },
      { -1, 0, false }
    },
    { { QAccessible::ToolBar, kAXToolbarRole, false },
      { -1, 0, false }
    },
    { { QAccessible::PageTabList, kAXTabGroupRole, false },
      { -1, 0, false }
    },
    { { QAccessible::PageTab, kAXRadioButtonRole, false },
      { -1, 0, false }
    },
    { { QAccessible::ButtonMenu, kAXMenuButtonRole, false },
      { -1, 0, false }
    },
    { { QAccessible::ButtonDropDown, kAXPopUpButtonRole, false },
      { -1, 0, false }
    },
    { { QAccessible::SpinBox, kAXIncrementorRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Slider, kAXSliderRole, false },
      { -1, 0, false }
    },
    { { QAccessible::ProgressBar, kAXProgressIndicatorRole, false },
      { -1, 0, false }
    },
    { { QAccessible::ComboBox, kAXPopUpButtonRole, false },
      { -1, 0, false }
    },
    { { QAccessible::RadioButton, kAXRadioButtonRole, false },
      { -1, 0, false }
    },
    { { QAccessible::CheckBox, kAXCheckBoxRole, false },
      { -1, 0, false }
    },
    { { QAccessible::StaticText, kAXStaticTextRole, false },
      { QAccessible::Name, kAXValueAttribute, false },
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
      { QAccessible::Name, kAXDescriptionAttribute, false },
#endif
      { -1, 0, false }
    },
    { { QAccessible::Table, kAXTableRole, false },
      { -1, 0, false }
    },
    { { QAccessible::StatusBar, kAXStaticTextRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Column, kAXColumnRole, false },
      { -1, 0, false }
    },
    { { QAccessible::ColumnHeader, kAXColumnRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Row, kAXRowRole, false },
      { -1, 0, false }
    },
    { { QAccessible::RowHeader, kAXRowRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Cell, kAXUnknownRole, false },
      { -1, 0, false }
    },
    { { QAccessible::PushButton, kAXButtonRole, false },
      { -1, 0, false }
    },
    { { QAccessible::EditableText, kAXTextFieldRole, true },
      { -1, 0, false }
    },
    { { QAccessible::Link, kAXTextFieldRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Indicator, kAXValueIndicatorRole, false },
      { -1, 0, false }
    },
    { { QAccessible::Splitter, kAXSplitGroupRole, false },
      { -1, 0, false }
    },
    { { -1, 0, false } }
};


// The root of the Qt accessible hiearchy.
static QObject *rootObject = 0;

static EventHandlerUPP applicationEventHandlerUPP = 0;
static EventTypeSpec application_events[] = {
    { kEventClassAccessibility,  kEventAccessibleGetChildAtPoint },
    { kEventClassAccessibility,  kEventAccessibleGetNamedAttribute }
};

static CFStringRef kObjectQtAccessibility = CFSTR("com.trolltech.qt.accessibility");
static EventHandlerUPP objectCreateEventHandlerUPP = 0;
static EventTypeSpec objectCreateEvents[] = {
    { kEventClassHIObject,  kEventHIObjectConstruct },
    { kEventClassHIObject,  kEventHIObjectInitialize },
    { kEventClassHIObject,  kEventHIObjectDestruct },
    { kEventClassHIObject,  kEventHIObjectPrintDebugInfo }
};

static OSStatus accessibilityEventHandler(EventHandlerCallRef next_ref, EventRef event, void *data);
static EventHandlerUPP accessibilityEventHandlerUPP = 0;
static EventTypeSpec accessibilityEvents[] = {
    { kEventClassAccessibility,  kEventAccessibleGetChildAtPoint },
    { kEventClassAccessibility,  kEventAccessibleGetFocusedChild },
    { kEventClassAccessibility,  kEventAccessibleGetAllAttributeNames },
    { kEventClassAccessibility,  kEventAccessibleGetNamedAttribute },
    { kEventClassAccessibility,  kEventAccessibleSetNamedAttribute },
    { kEventClassAccessibility,  kEventAccessibleIsNamedAttributeSettable },
    { kEventClassAccessibility,  kEventAccessibleGetAllActionNames },
    { kEventClassAccessibility,  kEventAccessiblePerformNamedAction },
    { kEventClassAccessibility,  kEventAccessibleGetNamedActionDescription }
};

class QInterfaceItemBase
{
public:
    QInterfaceItemBase(QAccessibleInterface *interface);
    ~QInterfaceItemBase();

    QAccessibleInterface *interface;
    int refCount;
};

QInterfaceItemBase::QInterfaceItemBase(QAccessibleInterface *interface)
:interface(interface)
,refCount(1)
{}

QInterfaceItemBase::~QInterfaceItemBase()
{
    delete interface;
}

/*
    QInterfaceItem represents one accessiblity item. It hides the fact that
    one QAccessibleInterface may represent more than one item, and it also
    automates the memory management for QAccessibleInterfaces using a ref count.

    It has the same API as QAccessibleInterface, minus the child parameter
    in the funcitons.
*/
class QInterfaceItem : public QAccessible
{
public:
    QInterfaceItem();
    QInterfaceItem(QAccessibleInterface *interface, int child = 0);
    QInterfaceItem(QInterfaceItem item, int child);
    ~QInterfaceItem();

    QInterfaceItem(const QInterfaceItem &other);
    void operator=(const QInterfaceItem &other);
    bool operator==(const QInterfaceItem &other) const;

    inline QString actionText (int action, Text text) const
    { return base->interface->actionText(action, text, child); }

    int childAt(int x, int y) const
    {
        int foundChild = base->interface->childAt(x, y);
        if (child == 0)
            return foundChild;

        if (foundChild == -1)
            return -1;

        if (foundChild == child)
            return 0;
        return -1;
    }

    inline int childCount() const
    {
        if (child != 0)
            return 0;
        return base->interface->childCount();
    }

    inline void doAction(int action, const QVariantList &params = QVariantList())
    { base->interface->doAction(action, child, params); }

    QInterfaceItem navigate(RelationFlag relation, int entry) const;

    inline QObject * object() const
    { return base->interface->object(); }

    inline QObject * cachedObject() const
    { return m_cachedObject; }

    inline QRect rect() const
    { return base->interface->rect(child); }

    inline Role role() const
    { return base->interface->role(child); }

    inline void setText(Text t, const QString &text)
    { base->interface->setText(t, child, text); }

    inline State state() const
    { return base->interface->state(child); }

    inline QString text (Text text) const
    { return base->interface->text(text, child); }

    inline int userActionCount()
    { return base->interface->userActionCount(child); }

    inline QString className() const
    { return QLatin1String(base->interface->object()->metaObject()->className()); }

    inline bool isHIView() const
    { return (child == 0); }

    inline int id() const
    { return child; }

    inline bool isValid() const
    { return (base != 0 && base->interface->isValid()); }

    QInterfaceItem parent() const
    { return navigate(QAccessible::Ancestor, 1); }

protected:
    QInterfaceItemBase *base;
    QObject *m_cachedObject;
    int child;
};

/*
    Constructs an invalid interfaceitem, which can be checked for with
    isValid().
*/
QInterfaceItem::QInterfaceItem()
{
    base = 0;
}

/*
    QInterfaceItem takes ownership of the AaccessibleInterface and will delete
    it when it's no longer referensed by any QInterfaceItems
*/
QInterfaceItem::QInterfaceItem(QAccessibleInterface *interface, int child)
:base(new QInterfaceItemBase(interface))
,m_cachedObject(interface->object())
,child(child)
{ }

/*
    Constructs a QInterfaceItem that uses the QAccessibleInterface from another
    item, but with a different child.
*/
QInterfaceItem::QInterfaceItem(QInterfaceItem other, int child)
:base(other.base)
,m_cachedObject(other.m_cachedObject)
,child(child)
{
    ++base->refCount;
}

QInterfaceItem::QInterfaceItem(const QInterfaceItem &other)
:base(other.base)
,m_cachedObject(other.m_cachedObject)
,child(other.child)
{
    ++base->refCount;
}

void QInterfaceItem::operator=(const QInterfaceItem &other)
{
    if (other == *this)
        return;

    child = other.child;

    if (base != 0 && --base->refCount == 0)
        delete base;

    base = other.base;
    if (base != 0)
        ++base->refCount;
}

bool QInterfaceItem::operator==(const QInterfaceItem &other) const
{
    if (isValid() == false || other.isValid() == false) {
        return (isValid() && other.isValid());
    }
    return (object() == other.object() && id() == other.id());
}

uint qHash(QInterfaceItem item)
{
    if (item.isValid())
        return qHash(item.object()) + qHash(item.id());
    else
        return qHash(item.cachedObject()) + qHash(item.id());
}

QInterfaceItem::~QInterfaceItem()
{
    if (base == 0)
        return;

    if (--base->refCount == 0)
        delete base;
}

QInterfaceItem QInterfaceItem::navigate(RelationFlag relation, int entry) const
{
    // On a QAccessibleInterface that handles its own children we can short-circut
    // the navigation if this QInterfaceItem refers to one of the children:
    if (child != 0) {
        // The Ancestor interface will always be the same QAccessibleInterface with
        // a child value of 0.
        if (relation == QAccessible::Ancestor)
            return QInterfaceItem(*this, 0);

        // The child hiearchy is only one level deep, so navigating to a child
        // of a child is not possible.
        if (relation == QAccessible::Child)
            return QInterfaceItem();
    }
    QAccessibleInterface *child_iface = 0;
    const int status = base->interface->navigate(relation, entry, &child_iface);

    if (status == -1)
        return QInterfaceItem(); // not found;


    // Check if target is a child of this interface.
    if (!child_iface) {
        return QInterfaceItem(*this, status);
    } else {
        // Target is child_iface or a child of that (status decides).
        return QInterfaceItem(child_iface, status);
    }
}

/*
    QAXUIElement is a thin wrapper around an AXUIElementRef that automates
    the ref-counting.
*/
class QAXUIElement
{
public:
    QAXUIElement();
    explicit QAXUIElement(AXUIElementRef elementRef);
    QAXUIElement(const QAXUIElement &element);
    ~QAXUIElement();

    inline HIObjectRef object() const
    {
        return AXUIElementGetHIObject(elementRef);
    }

    inline int id() const
    {
        UInt64 id;
        AXUIElementGetIdentifier(elementRef, &id);
        return id;
    }

    inline AXUIElementRef element() const
    {
        return elementRef;
    }

    inline bool isValid() const
    {
        return (elementRef != 0);
    }

    void operator=(const QAXUIElement &other);
    bool operator==(const QAXUIElement &other) const;
private:
    AXUIElementRef elementRef;
};

QAXUIElement::QAXUIElement()
:elementRef(0)
{}

QAXUIElement::QAXUIElement(AXUIElementRef elementRef)
:elementRef(elementRef)
{
    CFRetain(elementRef);
    CFRetain(object());
}

QAXUIElement::QAXUIElement(const QAXUIElement &element)
:elementRef(element.elementRef)
{
    CFRetain(elementRef);
    CFRetain(object());
}


QAXUIElement::~QAXUIElement()
{
    if (elementRef != 0) {
        CFRelease(elementRef);
        CFRelease(object());
    }
}

void QAXUIElement::operator=(const QAXUIElement &other)
{
    if (*this == other)
        return;

    if (elementRef != 0) {
        CFRelease(object());
        CFRelease(elementRef);
    }

    elementRef = other.elementRef;

      if (elementRef != 0) {
        CFRetain(elementRef);
        CFRetain(object());
    }
}

bool QAXUIElement::operator==(const QAXUIElement &other) const
{
    return CFEqual(elementRef, other.elementRef);
}

uint qHash(QAXUIElement element)
{
    return qHash(element.object()) + qHash(element.id());
}

/*
    QAccessibleHierarchyManager bridges the Mac and Qt accessibility hierarchies.
    There is a one-to-one relationship between AXUIElementRefs on the Mac side
    and QInterFaceItems on the Qt side, and this class provies lookup funcitons
    that translates between these to items.

    The identity of a QInterfaceItem is determined by its QAccessibleInterface and
    child identifier, and the identity of a AXUIelementRef is determined by its
    HIObjectRef and identifier.

    QAccessibleHierarchyManager receives QObject::destroyed() signals and deletes
    the accessibility objects for destroyed objects.
*/
class QAccessibleHierarchyManager : public QObject
{
Q_OBJECT
public:
    QAccessibleHierarchyManager() {};
    ~QAccessibleHierarchyManager();
    void reset();

    QAXUIElement createElementForInterface(const QInterfaceItem &interface);
    void addElementInterfacePair(const QAXUIElement &element, const QInterfaceItem &interface);

    QInterfaceItem lookup(const AXUIElementRef element);
    AXUIElementRef lookup(const QInterfaceItem interface);
    QAXUIElement lookup(QObject * const object, int id);
private slots:
    void objectDestroyed();
private:
    typedef QMultiHash<QObject*, QInterfaceItem> QObjectToInterfacesHash;
    QObjectToInterfacesHash qobjectToInterfaces;

    typedef QHash<QAXUIElement, QInterfaceItem> QAXUIElementToInterfaceHash;
    QAXUIElementToInterfaceHash elementToInterface;

    typedef QHash<QInterfaceItem, QAXUIElement> InterfaceToQAXUIElementHash;
    InterfaceToQAXUIElementHash interfaceToElement;
};

QAccessibleHierarchyManager::~QAccessibleHierarchyManager()
{
    reset();
}

/*
    Reomves all accessibility info accosiated with the sender object from the hash.
*/
void QAccessibleHierarchyManager::objectDestroyed()
{
    QObject * const destroyed = sender();
    QObjectToInterfacesHash::iterator i = qobjectToInterfaces.find(destroyed);

    while (i != qobjectToInterfaces.end() && i.key() == destroyed) {
        const QInterfaceItem interface = i.value();
        ++i;
        const QAXUIElement element = interfaceToElement.value(interface);
        interfaceToElement.remove(interface);
        if (element.isValid() == false)
            continue;
        elementToInterface.remove(element);
    }
    qobjectToInterfaces.remove(destroyed);
}

/*
    Removes all stored items, deletes QInterfaceItems.
*/
void QAccessibleHierarchyManager::reset()
{
    qobjectToInterfaces.clear();
    elementToInterface.clear();
    interfaceToElement.clear();
}

/*
    Decides if a QAccessibleInterface is interesting from an accessibility users point of view.
*/
static bool isItInteresting(const QInterfaceItem interface)
{
    // If the object is 0 it's probably not that intersting.
    QObject * const object = interface.object();
    if (!object)
        return false;

    const QString className = QLatin1String(object->metaObject()->className());

    // VoiceOver focusing on tool tips can be confusing. The contents of the
    // tool tip is avalible through the description attribute anyway, so
    // we disable accessibility for tool tips.
    if (className == QLatin1String("QTipLabel"))
        return false;

   const QAccessible::Role role = interface.role();

   // Some roles are not interesting:
   if (role == QAccessible::Client ||    // QWidget
       role == QAccessible::Border ||    // QFrame
       role == QAccessible::Application || // We use the system-provided application element.
       role == QAccessible::MenuItem)      // The system also provides the menu items.
        return false;

    // It is probably better to access the toolbar buttons directly than having
    // to navigate through the toolbar.
    if (role == QAccessible::ToolBar)
        return false;

    // Mac accessibility does not have an attribute that corresponds to the Invisible/Offscreen
    // state, so we disable the interface here.
    const QAccessible::State state = interface.state();
    if (state & QAccessible::Invisible ||
        state & QAccessible::Offscreen )
        return false;

    return true;
}

/*
    Creates a QAXUIelement that corresponds to the given QInterfaceItem.
*/
QAXUIElement QAccessibleHierarchyManager::createElementForInterface(const QInterfaceItem &interface)
{
    if (interface.isValid() == false)
        return QAXUIElement();

    HIObjectRef hiobj = 0;
    bool weCreatedIt = false;

    if (interface.isHIView()) {
        QObject * const object = interface.object();

        if (object && qobjectToInterfaces.contains(object) == false)
            connect(object, SIGNAL(destroyed()), SLOT(objectDestroyed()));

        QWidget * const widget = qobject_cast<QWidget * const>(object);
        if (widget)
            hiobj = (HIObjectRef)widget->winId();
    }

    if (hiobj == 0) {
        const OSStatus err = HIObjectCreate(kObjectQtAccessibility, 0, &hiobj);
        if (err)
            qWarning("%s::%d: Qt internal error creating HIObject: %ld", __FILE__, __LINE__, err);
        weCreatedIt = true;
    }

    // If the interface is not interesting to the accessibility user we disable
    // accessibility for it it. This means that it won't show up for the user,
    // but it is still a part of the hierarcy.
    // This also gets rid of the "empty_widget" created in QEventDispatcherMac::processEvents().
    HIObjectSetAccessibilityIgnored(hiobj, !isItInteresting(interface));


    // Install accessibility event handler on object
    if (!accessibilityEventHandlerUPP)
        accessibilityEventHandlerUPP = NewEventHandlerUPP(accessibilityEventHandler);

    OSErr err = InstallHIObjectEventHandler(hiobj, accessibilityEventHandlerUPP,
                                        GetEventTypeCount(accessibilityEvents),
                                        accessibilityEvents, 0, 0);

    if (err)
        qDebug() << "qaccessible_mac: Could not install accessibility event handler";

    QAXUIElement qelement(AXUIElementCreateWithHIObjectAndIdentifier(hiobj, interface.id()));

    // If we created a hiobj we let the QAXUIElement own it.
    if (weCreatedIt)
        CFRelease(hiobj);

    return qelement;
}

/*
    Accociates the given AXIUIElement and QInterfaceItem with each other.
*/
void QAccessibleHierarchyManager::addElementInterfacePair(const QAXUIElement &element, const QInterfaceItem &interface)
{
    elementToInterface.insert(element, interface);
    interfaceToElement.insert(interface, element);
    qobjectToInterfaces.insert(interface.object(), interface);
}

QInterfaceItem QAccessibleHierarchyManager::lookup(const AXUIElementRef element)
{
    if (element == 0)
        return QInterfaceItem();
    return elementToInterface.value(QAXUIElement(element));
}

AXUIElementRef QAccessibleHierarchyManager::lookup(const QInterfaceItem interface)
{
    if (interface.isValid() == false)
        return 0;
    return interfaceToElement.value(interface).element();
}

QAXUIElement QAccessibleHierarchyManager::lookup(QObject * const object, int id)
{
    QObjectToInterfacesHash::iterator i = qobjectToInterfaces.find(object);

    while (i != qobjectToInterfaces.end() && i.key() == object) {
        const QInterfaceItem interface = i.value();
        ++i;
        if (interface.isValid() == false)
            continue;
        if (interface.id() != id)
            continue;
        const QAXUIElement element = interfaceToElement.value(interface);
        if (element.isValid() == false)
            continue;

        return element;
    }
    return QAXUIElement();
}

Q_GLOBAL_STATIC(QAccessibleHierarchyManager, accessibleHierarchyManager)

// Debug output helpers:
/*
static QString nameForEventKind(UInt32 kind)
{
    switch(kind) {
        case kEventAccessibleGetChildAtPoint:       return QString("GetChildAtPoint");      break;
        case kEventAccessibleGetAllAttributeNames:  return QString("GetAllAttributeNames"); break;
        case kEventAccessibleGetNamedAttribute:     return QString("GetNamedAttribute");    break;
        case kEventAccessibleSetNamedAttribute:     return QString("SetNamedAttribute");    break;
        case kEventAccessibleGetAllActionNames:     return QString("GetAllActionNames");    break;
        case kEventAccessibleGetFocusedChild:       return QString("GetFocusedChild");      break;
        default:
            return QString("Unknown accessibility event type: %1").arg(kind);
        break;
    };
}
*/
static bool qt_mac_append_cf_uniq(CFMutableArrayRef array, CFTypeRef value)
{
    if (value == 0)
        return false; 

    CFRange range;
    range.location = 0;
    range.length = CFArrayGetCount(array);
    if(!CFArrayContainsValue(array, range, value)) {
        CFArrayAppendValue(array, value);
        return true;
    }
    return false;
}

/*
    Gets the AccessibleObject parameter from an event.
*/
static inline AXUIElementRef getAccessibleObjectParameter(EventRef event)
{
    AXUIElementRef element;
    GetEventParameter(event, kEventParamAccessibleObject, typeCFTypeRef, 0,
                        sizeof(element), 0, &element);
    return element;
}

/*
    Returns an AXUIElementRef for the given child index of interface. Creates the element
    if necessary. Returns 0 if there is no child at that index. childIndex is 1-based.
*/
static AXUIElementRef lookupCreateChild(const QInterfaceItem interface, const int childIndex)
{
    const QInterfaceItem child_iface = interface.navigate(QAccessible::Child, childIndex);
    if (child_iface.isValid() == false)
        return 0;

    // The current mac accessibility bridge does not handle the case where two 
    // QAccessibleInterfaces in a parent-child relation share the the same object.
    if (child_iface.id() == 0 && child_iface.object() == interface.object())
        return 0;

    AXUIElementRef childElement = accessibleHierarchyManager()->lookup(child_iface);
    if (childElement)
        return childElement;

    QAXUIElement element = accessibleHierarchyManager()->createElementForInterface(child_iface);
    accessibleHierarchyManager()->addElementInterfacePair(element, child_iface);

    return element.element();
}

static void createElementsForRootInterface()
{
    QAccessibleInterface *appInterface = QAccessible::queryAccessibleInterface(rootObject ? rootObject : qApp);
    if (!appInterface)
        return;

    QInterfaceItem appQInterfaceItem(appInterface, 0);

    // Add the children of the root accessiblity interface
    const int childCount = appQInterfaceItem.childCount();
    for (int i = 0; i < childCount; ++i)
        lookupCreateChild(appQInterfaceItem, i + 1);
}

/*
    The application event handler makes sure that all top-level qt windows are registered
    before any accessibility events are handeled.
*/
static OSStatus applicationEventHandler(EventHandlerCallRef next_ref, EventRef event, void *data)
{
    Q_UNUSED(data);
    createElementsForRootInterface();
    return CallNextEventHandler(next_ref, event);
}

/*
    Installs the applicationEventHandler on the application
*/
static void installApplicationEventhandler()
{
    if (!applicationEventHandlerUPP)
        applicationEventHandlerUPP = NewEventHandlerUPP(applicationEventHandler);

    OSStatus err = InstallApplicationEventHandler(applicationEventHandlerUPP,
                            GetEventTypeCount(application_events), application_events,
                            0, 0);

    if (err && err != eventHandlerAlreadyInstalledErr)
        qDebug() << "qaccessible_mac: Error installing application event handler:" << err;
}

static void removeEventhandler(EventHandlerUPP eventHandler)
{
    if (eventHandler) {
        DisposeEventHandlerUPP(eventHandler);
        eventHandler = 0;
    }
}


/*
    Returns the value for element by combining the QAccessibility::Checked and
    QAccessibility::Mixed flags into an int value that the Mac accessibilty
    system understands. This works for check boxes, radio buttons, and the like.
    The return values are:
    0: unchecked
    1: checked
    2: undecided
*/
static int buttonValue(QInterfaceItem element)
{
    const QAccessible::State state = element.state();
    if (state & QAccessible::Mixed)
        return 2;
    else if(state & QAccessible::Checked)
        return 1;
    else
        return 0;
}

static QString getValue(QInterfaceItem interface)
{
    const QAccessible::Role role = interface.role();
    if (role == QAccessible::RadioButton || role == QAccessible::CheckBox)
        return QString::number(buttonValue(interface));
    else
        return interface.text(QAccessible::Value);
}

/*
    Translates a QAccessible::Role into a mac accessibility role.
*/
static CFStringRef macRole(QInterfaceItem interface)
{
    const QAccessible::Role qtRole = interface.role();

    // Qt accessibility:  QAccessible::Splitter contains QAccessible::Grip.
    // Mac accessibility: AXSplitGroup contains AXSplitter.
    if (qtRole == QAccessible::Grip) {
        const QInterfaceItem parent = interface.parent();
        if (parent.isValid() && parent.role() == QAccessible::Splitter)
            return kAXSplitterRole;
    }

    int i = 0;
    int testRole = text_bindings[i][0].qt;
    while (testRole != -1) {
        if (testRole == qtRole)
            return text_bindings[i][0].mac;
        ++i;
        testRole = text_bindings[i][0].qt;
    }

    return kAXUnknownRole;
}

/*
    Translates a QAccessible::Role and an attribute name into a QAccessible::Text, taking into
    account execptions listed in text_bindings.
*/
static int textForRoleAndAttribute(QAccessible::Role role, CFStringRef attribute)
{
     // Search for exception, return it if found.
    int testRole = text_bindings[0][0].qt;
    int i = 0;
    while (testRole != -1) {
        if (testRole == role) {
            int j = 1;
            int qtRole = text_bindings[i][j].qt;
            CFStringRef testAttribute = text_bindings[i][j].mac;
            while (qtRole != -1) {
                if (CFStringCompare(attribute, testAttribute, 0) == kCFCompareEqualTo) {
                    return (QAccessible::Text)qtRole;
                }
                ++j;
                testAttribute = text_bindings[i][j].mac;
                qtRole = text_bindings[i][j].qt;
            }
            break;
        }
        ++i;
        testRole = text_bindings[i][0].qt;
    }

    // Return default mappping
    if (CFStringCompare(attribute, kAXTitleAttribute, 0) == kCFCompareEqualTo)
        return QAccessible::Name;
    else if (CFStringCompare(attribute, kAXValueAttribute, 0) == kCFCompareEqualTo)
        return QAccessible::Value;
    else if (CFStringCompare(attribute, kAXHelpAttribute, 0) == kCFCompareEqualTo)
        return QAccessible::Help;
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
    else if (CFStringCompare(attribute, kAXDescriptionAttribute, 0) == kCFCompareEqualTo)
        return QAccessible::Description;
#endif
    else
        return -1;
}

/*
    Returns the subrole string constant for the interface if it has one,
    else returns an empty string.
*/
static QCFString subrole(QInterfaceItem interface)
{
    const QInterfaceItem parent = interface.parent();
    if (parent.isValid() == false)
        return QCFString();

    if (parent.role() == QAccessible::ScrollBar) {
        QCFString subrole;
        switch(interface.id()) {
            case 1: subrole = kAXDecrementArrowSubrole; break;
            case 2: subrole = kAXDecrementPageSubrole; break;
            case 4: subrole = kAXIncrementPageSubrole; break;
            case 5: subrole = kAXIncrementArrowSubrole; break;
            default:
            break;
        }
        return subrole;
    }
    return QCFString();
}

/*
    Returns the label (buddy) interface for interface, or 0 if it has none.
*/
/*
static QInterfaceItem findLabel(QInterfaceItem interface)
{
    return interface.navigate(QAccessible::Label, 1);
}
*/
/*
    Returns a list of interfaces this interface labels, or an empty list if it doesn't label any.
*/
/*
static QList<QInterfaceItem> findLabelled(QInterfaceItem interface)
{
    QList<QInterfaceItem> interfaceList;

    int count = 1;
    const QInterfaceItem labelled = interface.navigate(QAccessible::Labelled, count);
    while (labelled.isValid()) {
        interfaceList.append(labelled);
        ++count;
    }
    return interfaceList;
}
*/
/*
    Tests if the given QInterfaceItem has data for a mac attribute.
*/
static bool supportsAttribute(CFStringRef attribute, QInterfaceItem interface)
{
    const int text = textForRoleAndAttribute(interface.role(), attribute);

    // Special case: Static texts don't have a title.
    if (interface.role() == QAccessible::StaticText && attribute == kAXTitleAttribute)
        return false;

    // Return true if we the attribute matched a QAccessible::Role and we get text for that role from the interface.
    if (text != -1) {
        if (text == QAccessible::Value) // Special case for Value, see getValue()
            return !getValue(interface).isEmpty();
        else
            return !interface.text((QAccessible::Text)text).isEmpty();
    }

    if (CFStringCompare(attribute, kAXChildrenAttribute,  0) == kCFCompareEqualTo) {
        if (interface.childCount() > 0)
            return true;
    }

    if (CFStringCompare(attribute, kAXSubroleAttribute,  0) == kCFCompareEqualTo) {
        return (subrole(interface) != QCFString());
    }

    return false;
}

static void appendIfSupported(CFMutableArrayRef array, CFStringRef attribute, const QInterfaceItem interface)
{
    if (supportsAttribute(attribute, interface))
        qt_mac_append_cf_uniq(array, attribute);
}

/*
    Returns the names of the attributes the give QInterfaceItem supports.
*/
static OSStatus getAllAttributeNames(EventRef event, QInterfaceItem interface, EventHandlerCallRef next_ref)
{
    // Call system event handler.
    OSStatus err = CallNextEventHandler(next_ref, event);
    if(err != noErr && err != eventNotHandledErr)
        return err;
    CFMutableArrayRef attrs = 0;
    GetEventParameter(event, kEventParamAccessibleAttributeNames, typeCFMutableArrayRef, 0,
                      sizeof(attrs), 0, &attrs);

    if (!attrs)
        return eventNotHandledErr;

    // Append attribute names that are always supported.
    qt_mac_append_cf_uniq(attrs, kAXPositionAttribute);
    qt_mac_append_cf_uniq(attrs, kAXSizeAttribute);
    qt_mac_append_cf_uniq(attrs, kAXRoleAttribute);
    qt_mac_append_cf_uniq(attrs, kAXEnabledAttribute);
    qt_mac_append_cf_uniq(attrs, kAXWindowAttribute);
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
    qt_mac_append_cf_uniq(attrs, kAXTopLevelUIElementAttribute);
#endif

    // Append these names if the QInterafceItem returns any data for them.
    appendIfSupported(attrs, kAXTitleAttribute, interface);
    appendIfSupported(attrs, kAXValueAttribute, interface);
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
    appendIfSupported(attrs, kAXDescriptionAttribute, interface);
    appendIfSupported(attrs, kAXLinkedUIElementsAttribute, interface);
#endif
    appendIfSupported(attrs, kAXHelpAttribute, interface);
    appendIfSupported(attrs, kAXTitleUIElementAttribute, interface);
    appendIfSupported(attrs, kAXChildrenAttribute, interface);
    appendIfSupported(attrs, kAXSubroleAttribute, interface);

    // Append attribute names based on the interaface role.
    switch (interface.role())  {
        case QAccessible::Window:
            qt_mac_append_cf_uniq(attrs, kAXMainAttribute);
            qt_mac_append_cf_uniq(attrs, kAXMinimizedAttribute);
            qt_mac_append_cf_uniq(attrs, kAXCloseButtonAttribute);
            qt_mac_append_cf_uniq(attrs, kAXZoomButtonAttribute);
            qt_mac_append_cf_uniq(attrs, kAXMinimizeButtonAttribute);
            qt_mac_append_cf_uniq(attrs, kAXToolbarButtonAttribute);
            qt_mac_append_cf_uniq(attrs, kAXGrowAreaAttribute);
        break;
        case QAccessible::PageTabList:
            qt_mac_append_cf_uniq(attrs, kAXTabsAttribute);
        break;
        case QAccessible::RadioButton:
        case QAccessible::CheckBox:
            qt_mac_append_cf_uniq(attrs, kAXMinValueAttribute);
            qt_mac_append_cf_uniq(attrs, kAXMaxValueAttribute);
        break;
        case QAccessible::ScrollBar:
            qt_mac_append_cf_uniq(attrs, kAXOrientationAttribute);
        break;
        case QAccessible::Splitter:
            qt_mac_append_cf_uniq(attrs, kAXSplittersAttribute);
        break;
        default:
        break;
    }

    // Append attribute names based on the mac accessibility role.
    const QCFString mac_role = macRole(interface);
    if (mac_role == kAXSplitterRole) {
        qt_mac_append_cf_uniq(attrs, kAXPreviousContentsAttribute);
        qt_mac_append_cf_uniq(attrs, kAXNextContentsAttribute);
        qt_mac_append_cf_uniq(attrs, kAXOrientationAttribute);
    }

    return noErr;
}

/*
    Adds the children for interface to the global QAccessibleHirearchyManager.
*/
static QList<AXUIElementRef> mapChildrenForInterface(const QInterfaceItem interface)
{
    QList<AXUIElementRef> children;
    const int children_count = interface.childCount();
    for (int i = 0; i < children_count; ++i) {
        children.append(lookupCreateChild(interface, i + 1));
    }
    return children;
}

static void handleStringAttribute(EventRef event, QAccessible::Text text, QInterfaceItem interface)
{
    QString str = interface.text(text);
    if (str.isEmpty())
        return;

    // Remove any html markup from the text string, or VoiceOver will read the html tags.
    static QTextDocument document;
    document.setHtml(str);
    str = document.toPlainText();

    CFStringRef cfstr = QCFString::toCFStringRef(str);
    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFStringRef, sizeof(cfstr), &cfstr);
}

/*
    Handles the parent attribute for a interface.
    There are basically three cases here:
    1. interface is a HIView and has only HIView children.
    2. interface is a HIView but has children that is not a HIView
    3. interface is not a HIView.
*/
static OSStatus handleChildrenAttribute(EventHandlerCallRef next_ref, EventRef event, QInterfaceItem interface)
{
   // Add the children for this interface to the global QAccessibelHierachyManager.
    const QList<AXUIElementRef> children = mapChildrenForInterface(interface);
    const int childCount = children.count();

    OSStatus err = eventNotHandledErr;
    if (interface.isHIView())
        err = CallNextEventHandler(next_ref, event);

    CFMutableArrayRef array = 0;
    int arraySize = 0;
    if (err == noErr) {
        CFTypeRef obj = 0;
        err = GetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef, NULL , sizeof(obj), NULL, &obj);
        if (err == noErr && obj != 0) {
            array = (CFMutableArrayRef)obj;
            arraySize = CFArrayGetCount(array);
        }
    }

    if (array == 0)
        return err;

    // If the system event handler and the QInterafceItem agrees on the number of children
    // we trust the children returned by the system event handler.
    if (childCount == arraySize)
        return noErr;

    // Otherwise us the children reported by the QInterafceItem.
    CFArrayRemoveAllValues(array);
    for (int i = 0; i < childCount; ++i)  {
        qt_mac_append_cf_uniq(array, children.at(i));
    }

    return noErr;
}

/*

*/
static OSStatus handleParentAttribute(EventHandlerCallRef next_ref, EventRef event, QInterfaceItem interface)
{
    OSStatus err = eventNotHandledErr;
    if (interface.isHIView()) {
         err = CallNextEventHandler(next_ref, event);
    }
    if (err == noErr)
        return err;

    const QInterfaceItem parentInterface  = interface.navigate(QAccessible::Ancestor, 1);
    const AXUIElementRef parentElement = accessibleHierarchyManager()->lookup(parentInterface);

    if (parentElement == 0)
        return eventNotHandledErr;

    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef, sizeof(parentElement), &parentElement);
    return noErr;
}

struct IsWindowTest
{
    static inline bool test(const QInterfaceItem interface)
    {
        return (interface.role() == QAccessible::Window);
    }
};

struct IsWindowAndNotDrawerOrSheetTest
{
    static inline bool test(const QInterfaceItem interface)
    {
        QWidget * const widget = qobject_cast<QWidget*>(interface.object());
        return (interface.role() == QAccessible::Window &&
                widget && widget->isWindow() &&
                !qt_mac_is_macdrawer(widget) &&
                !qt_mac_is_macsheet(widget));
    }
};

/*
    Navigates up the iterfaces ancestor hierachy until a QAccessibleInterface that
    passes the Test is found. If we reach a interface that is a HIView we stop the
    search and call AXUIElementCopyAttributeValue.
*/
template <typename TestType>
OSStatus navigateAncestors(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface, CFStringRef attribute)
{
    if (interface.isHIView())
        return CallNextEventHandler(next_ref, event);

    QInterfaceItem current = interface;
    AXUIElementRef element = 0;
    while (current.isValid()) {
        if (TestType::test(interface)) {
            element = accessibleHierarchyManager()->lookup(current);
            break;
        }

        // If we reach an InterfaceItem that is a HiView we can hand of the search to
        // the system event handler. This is the common case.
        if (current.isHIView()) {
            CFTypeRef value = 0;
            const AXUIElementRef currentElement = accessibleHierarchyManager()->lookup(current);
            AXError err = AXUIElementCopyAttributeValue(currentElement, attribute, &value);
            if (err == noErr)
                element = (AXUIElementRef)value;
            break;
        }

        QInterfaceItem next = current.navigate(QAccessible::Ancestor, 1);
        if (next.isValid() == false)
            break;
        if (next == current)
            break;
        current = next;
    }

    if (element == 0)
        return eventNotHandledErr;

    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef,
                                      sizeof(element), &element);
    return noErr;
}

/*
    Returns the top-level window for an interface, which is the closest ancestor interface that
    has the Window role, but is not a sheet or a drawer.
*/
static OSStatus handleWindowAttribute(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface)
{
    return navigateAncestors<IsWindowAndNotDrawerOrSheetTest>(next_ref, event, interface, kAXWindowAttribute);
}

/*
    Returns the top-level window for an interface, which is the closest ancestor interface that
    has the Window role. (Can also be a sheet or a drawer)
*/
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
static OSStatus handleTopLevelUIElementAttribute(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface)
{
    return navigateAncestors<IsWindowTest>(next_ref, event, interface, kAXTopLevelUIElementAttribute);
}
#endif

/*
    Returns the tab buttons for an interface. To do this we look for children with the QAccessible::PageTab role.
*/
static OSStatus handleTabsAttribute(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface)
{
    Q_UNUSED(next_ref);
    CFMutableArrayRef array = CFArrayCreateMutable(0, 0, 0);

    const int numChildren = interface.childCount();
    for (int i = 1; i < numChildren + 1; ++i) {
        QInterfaceItem child = interface.navigate(QAccessible::Child, i);
        if (child.isValid() && child.role() == QAccessible::PageTab)
             qt_mac_append_cf_uniq(array, lookupCreateChild(interface, i));
    }

    OSStatus err = SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFArrayRef, sizeof(array), &array);
    CFRelease(array);
    if (err)
        return eventNotHandledErr;
    return noErr;
}

static OSStatus handlePositionAttribute(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface)
{
    if (interface.isHIView())
        return CallNextEventHandler(next_ref, event);

    QPoint qpoint(interface.rect().topLeft());
    HIPoint point;
    point.x = qpoint.x();
    point.y = qpoint.y();
    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeHIPoint, sizeof(point), &point);
    return noErr;
}

static OSStatus handleSizeAttribute(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface)
{
    if (interface.isHIView())
        return CallNextEventHandler(next_ref, event);

    QSize qSize(interface.rect().size());
    HISize size;
    size.width = qSize.width();
    size.height = qSize.height();
    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeHISize, sizeof(size), &size);
    return noErr;
}

static OSStatus handleSubroleAttribute(EventHandlerCallRef, EventRef event, const QInterfaceItem interface)
{
    const QCFString role = subrole(interface);
    CFStringRef rolestr = (CFStringRef)role;
    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef, sizeof(rolestr), &rolestr);
    return noErr;
}

static OSStatus handleOrientationAttribute(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface)
{
    QObject *const object = interface.object();
    Qt::Orientation orientation;
    if (QAbstractSlider * const scrollBar = qobject_cast<QAbstractSlider * const>(object)) {
        orientation = scrollBar->orientation();
    } else if (QSplitterHandle * const splitter = qobject_cast<QSplitterHandle * const>(object)) {
        // Qt reports the layout orientation, but we want the splitter handle orientation.
        orientation = (splitter->orientation() == Qt::Horizontal) ? Qt::Vertical : Qt::Horizontal;
    } else {
        return CallNextEventHandler(next_ref, event);
    }
    const CFStringRef orientationString = (orientation == Qt::Vertical)
        ? kAXVerticalOrientationValue : kAXHorizontalOrientationValue;
    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFStringRef, sizeof(orientationString), &orientationString);
    return noErr;
}

/*
    Figures out the next or previous contents for a splitter.
*/
static OSStatus handleSplitterContentsAttribute(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface, QCFString nextOrPrev)
{
    const QInterfaceItem parent = interface.parent();
    if (parent.isValid() == false)
        return CallNextEventHandler(next_ref, event);

    if (parent.role() != QAccessible::Splitter)
        return CallNextEventHandler(next_ref, event);

    const QSplitter * const splitter = qobject_cast<const QSplitter * const>(parent.object());
    if (splitter == 0)
        return CallNextEventHandler(next_ref, event);

    QWidget * const splitterHandle = qobject_cast<QWidget * const>(interface.object());
    const int splitterHandleIndex = splitter->indexOf(splitterHandle);
    const int widgetIndex = (nextOrPrev == QCFString(kAXPreviousContentsAttribute)) ? splitterHandleIndex - 1 : splitterHandleIndex;
    const AXUIElementRef contentsElement = accessibleHierarchyManager()->lookup(splitter->widget(widgetIndex), 0).element();

    QCFType<CFArrayRef> cfList = CFArrayCreate(0, (const void **)&contentsElement, 1, 0);
    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef, sizeof(cfList), &cfList);

    return noErr;
}

/*
    Creates a list of all splitter handles the splitter contains.
*/
static OSStatus handleSplittersAttribute(EventHandlerCallRef next_ref, EventRef event, const QInterfaceItem interface)
{
    const QSplitter * const splitter = qobject_cast<const QSplitter * const>(interface.object());
    if (splitter == 0)
        return CallNextEventHandler(next_ref, event);

    mapChildrenForInterface(interface);
    QAccessibleHierarchyManager * const manager = accessibleHierarchyManager();
    const int visibleSplitterCount = splitter->count() -1; // skip first handle, it's always invisible.

    CFMutableArrayRef cfList = CFArrayCreateMutable(0, 0, &kCFTypeArrayCallBacks);
    for (int i = 0; i < visibleSplitterCount; ++ i)
        CFArrayAppendValue(cfList, manager->lookup(splitter->handle(i + 1), 0).element());

    SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef, sizeof(cfList), &cfList);
    return noErr;
}

static OSStatus getNamedAttribute(EventHandlerCallRef next_ref, EventRef event, QInterfaceItem interface)
{
    CFStringRef var;
    GetEventParameter(event, kEventParamAccessibleAttributeName, typeCFStringRef, 0,
                              sizeof(var), 0, &var);

    if (CFStringCompare(var, kAXChildrenAttribute, 0) == kCFCompareEqualTo) {
        return handleChildrenAttribute(next_ref, event, interface);
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
    } else if(CFStringCompare(var, kAXTopLevelUIElementAttribute, 0) == kCFCompareEqualTo) {
        return handleTopLevelUIElementAttribute(next_ref, event, interface);
#endif
    } else if(CFStringCompare(var, kAXWindowAttribute, 0) == kCFCompareEqualTo) {
        return handleWindowAttribute(next_ref, event, interface);
    } else if(CFStringCompare(var, kAXParentAttribute, 0) == kCFCompareEqualTo) {
        return handleParentAttribute(next_ref, event, interface);
    } else if (CFStringCompare(var, kAXPositionAttribute, 0) == kCFCompareEqualTo) {
        return handlePositionAttribute(next_ref, event, interface);
    } else if (CFStringCompare(var, kAXSizeAttribute, 0) == kCFCompareEqualTo) {
        return handleSizeAttribute(next_ref, event, interface);
    } else  if (CFStringCompare(var, kAXRoleAttribute, 0) == kCFCompareEqualTo) {
        CFStringRef role = macRole(interface);
        QWidget * const widget = qobject_cast<QWidget *>(interface.object());
        if (role == kAXUnknownRole && widget && widget->isWindow())
            role = kAXWindowRole;

        SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFStringRef,
                          sizeof(role), &role);

    } else if (CFStringCompare(var, kAXEnabledAttribute, 0) == kCFCompareEqualTo) {
        Boolean val =  !((interface.state() & QAccessible::Unavailable))
                     && !((interface.state() & QAccessible::Invisible));
        SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                          sizeof(val), &val);
    } else if (CFStringCompare(var, kAXExpandedAttribute, 0) == kCFCompareEqualTo) {
        Boolean val = (interface.state() & QAccessible::Expanded);
        SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                          sizeof(val), &val);
    } else if (CFStringCompare(var, kAXSelectedAttribute, 0) == kCFCompareEqualTo) {
        Boolean val = (interface.state() & QAccessible::Selection);
        SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                          sizeof(val), &val);
    } else if (CFStringCompare(var, kAXFocusedAttribute, 0) == kCFCompareEqualTo) {
        Boolean val = (interface.state() & QAccessible::Focus);
        SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                          sizeof(val), &val);
    } else if (CFStringCompare(var, kAXSelectedChildrenAttribute, 0) == kCFCompareEqualTo) {
        const int cc = interface.childCount();
        QList<AXUIElementRef> sel;
        for (int i = 1; i <= cc; ++i) {
            const QInterfaceItem child_iface = interface.navigate(QAccessible::Child, i);
            if (child_iface.isValid() && child_iface.state() & QAccessible::Selected)
                sel.append(accessibleHierarchyManager()->lookup(child_iface));
        }
        AXUIElementRef *arr = (AXUIElementRef *)malloc(sizeof(AXUIElementRef) * sel.count());
        for(int i = 0; i < sel.count(); i++)
            arr[i] = sel[i];
        QCFType<CFArrayRef> cfList = CFArrayCreate(0, (const void **)arr, sel.count(), 0);
        free(arr);
        SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef,
                          sizeof(cfList), &cfList);
    } else if (CFStringCompare(var, kAXCloseButtonAttribute, 0) == kCFCompareEqualTo) {
        if(interface.object() && interface.object()->isWidgetType()) {
            Boolean val = true; //do we want to add a WState for this?
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                              sizeof(val), &val);
        }
    } else if (CFStringCompare(var, kAXZoomButtonAttribute, 0) == kCFCompareEqualTo) {
        if(interface.object() && interface.object()->isWidgetType()) {
            QWidget *widget = (QWidget*)interface.object();
            Boolean val = (widget->windowFlags() & Qt::WindowMaximizeButtonHint);
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                              sizeof(val), &val);
        }
    } else if (CFStringCompare(var, kAXMinimizeButtonAttribute, 0) == kCFCompareEqualTo) {
        if(interface.object() && interface.object()->isWidgetType()) {
            QWidget *widget = (QWidget*)interface.object();
            Boolean val = (widget->windowFlags() & Qt::WindowMinimizeButtonHint);
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                              sizeof(val), &val);
        }
    } else if (CFStringCompare(var, kAXToolbarButtonAttribute, 0) == kCFCompareEqualTo) {
        if(interface.object() && interface.object()->isWidgetType()) {
            QWidget *widget = (QWidget*)interface.object();
            Boolean val = qobject_cast<QMainWindow *>(widget) != 0;
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                              sizeof(val), &val);
        }
    } else if (CFStringCompare(var, kAXGrowAreaAttribute, 0) == kCFCompareEqualTo) {
        if(interface.object() && interface.object()->isWidgetType()) {
            Boolean val = true; //do we want to add a WState for this?
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                              sizeof(val), &val);
        }
    } else if (CFStringCompare(var, kAXMinimizedAttribute, 0) == kCFCompareEqualTo) {
        if (interface.object() && interface.object()->isWidgetType()) {
            QWidget *widget = (QWidget*)interface.object();
            Boolean val = (widget->windowState() & Qt::WindowMinimized);
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeBoolean,
                              sizeof(val), &val);
        }
    } else if (CFStringCompare(var, kAXSubroleAttribute, 0) == kCFCompareEqualTo) {
        return handleSubroleAttribute(next_ref, event, interface);
    } else if (CFStringCompare(var, kAXRoleDescriptionAttribute, 0) == kCFCompareEqualTo) {
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
        if (HICopyAccessibilityRoleDescription) {
            const CFStringRef roleDescription = HICopyAccessibilityRoleDescription(macRole(interface), 0);
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFStringRef,
                          sizeof(roleDescription), &roleDescription);
        } else
#endif
        {
            // Just use Qt::Description on 10.3
            handleStringAttribute(event, QAccessible::Description, interface);
        }
    } else if (CFStringCompare(var, kAXTitleAttribute, 0) == kCFCompareEqualTo) {
        const QAccessible::Role role = interface.role();
        const QAccessible::Text text = (QAccessible::Text)textForRoleAndAttribute(role, var);
        handleStringAttribute(event, text, interface);
    } else if (CFStringCompare(var, kAXValueAttribute, 0) == kCFCompareEqualTo) {
        const QAccessible::Role role = interface.role();
        const QAccessible::Text text = (QAccessible::Text)textForRoleAndAttribute(role, var);
        if (role == QAccessible::CheckBox || role == QAccessible::RadioButton) {
            int value = buttonValue(interface);
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeUInt32, sizeof(value), &value);
        } else {
            handleStringAttribute(event, text, interface);
        }
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
    } else if (CFStringCompare(var, kAXDescriptionAttribute, 0) == kCFCompareEqualTo) {
        const QAccessible::Role role = interface.role();
        const QAccessible::Text text = (QAccessible::Text)textForRoleAndAttribute(role, var);
        handleStringAttribute(event, text, interface);
    } else if (CFStringCompare(var, kAXLinkedUIElementsAttribute, 0) == kCFCompareEqualTo) {
        return CallNextEventHandler(next_ref, event);
#endif
    } else if (CFStringCompare(var, kAXHelpAttribute, 0) == kCFCompareEqualTo) {
        const QAccessible::Role role = interface.role();
        const QAccessible::Text text = (QAccessible::Text)textForRoleAndAttribute(role, var);
        handleStringAttribute(event, text, interface);
    } else if (CFStringCompare(var, kAXTitleUIElementAttribute, 0) == kCFCompareEqualTo) {
        return CallNextEventHandler(next_ref, event);
    } else if (CFStringCompare(var, kAXTabsAttribute, 0) == kCFCompareEqualTo) {
        return handleTabsAttribute(next_ref, event, interface);
    } else if (CFStringCompare(var, kAXMinValueAttribute, 0) == kCFCompareEqualTo) {
        if (interface.role() == QAccessible::RadioButton || interface.role() == QAccessible::CheckBox) {
            uint value = 0;
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeUInt32, sizeof(value), &value);
        } else {
            return CallNextEventHandler(next_ref, event);
        }
    } else if (CFStringCompare(var, kAXMaxValueAttribute, 0) == kCFCompareEqualTo) {
        if (interface.role() == QAccessible::RadioButton || interface.role() == QAccessible::CheckBox) {
            uint value = 2;
            SetEventParameter(event, kEventParamAccessibleAttributeValue, typeUInt32, sizeof(value), &value);
        } else {
            return CallNextEventHandler(next_ref, event);
        }
    } else if (CFStringCompare(var, kAXOrientationAttribute, 0) == kCFCompareEqualTo) {
        return handleOrientationAttribute(next_ref, event, interface);
    } else if (CFStringCompare(var, kAXPreviousContentsAttribute, 0) == kCFCompareEqualTo) {
        return handleSplitterContentsAttribute(next_ref, event, interface, kAXPreviousContentsAttribute);
    } else if (CFStringCompare(var, kAXNextContentsAttribute, 0) == kCFCompareEqualTo) {
        return handleSplitterContentsAttribute(next_ref, event, interface, kAXNextContentsAttribute);
    } else if (CFStringCompare(var, kAXSplittersAttribute, 0) == kCFCompareEqualTo) {
        return handleSplittersAttribute(next_ref, event, interface);
    } else {
        return CallNextEventHandler(next_ref, event);
    }
    return noErr;
}

static OSStatus isNamedAttributeSettable(EventRef event, QInterfaceItem interface)
{
    CFStringRef var;
    GetEventParameter(event, kEventParamAccessibleAttributeName, typeCFStringRef, 0,
                      sizeof(var), 0, &var);
    Boolean settable = false;
    if (CFStringCompare(var, kAXFocusedAttribute, 0) == kCFCompareEqualTo) {
        settable = true;
    } else {
        for (int r = 0; text_bindings[r][0].qt != -1; r++) {
            if(interface.role() == (QAccessible::Role)text_bindings[r][0].qt) {
                for (int a = 1; text_bindings[r][a].qt != -1; a++) {
                    if (CFStringCompare(var, text_bindings[r][a].mac, 0) == kCFCompareEqualTo) {
                        settable = text_bindings[r][a].settable;
                        break;
                    }
                }
            }
        }
    }
    SetEventParameter(event, kEventParamAccessibleAttributeSettable, typeBoolean,
                      sizeof(settable), &settable);
    return noErr;
}

static OSStatus getChildAtPoint(EventHandlerCallRef next_ref, EventRef event, QInterfaceItem interface)
{
    Q_UNUSED(next_ref);
    if (interface.isValid() == false)
        return eventNotHandledErr;

    // Add the children for this interface to the global QAccessibelHierachyManager.
    mapChildrenForInterface(interface);

    Point where;
    GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, 0, sizeof(where), 0, &where);
    const int child = interface.childAt(where.h, where.v);

    if(child == -1)
        return eventNotHandledErr;

    const AXUIElementRef element = lookupCreateChild(interface, child);

    SetEventParameter(event, kEventParamAccessibleChild, typeCFTypeRef,
                                  sizeof(element), &element);

    return noErr;
}

/*
    Returns a list of actions the given interface supports.
    Currently implemented by getting the interface role and deciding based on that.
*/
static QList<QAccessible::Action> supportedPredefinedActions(QInterfaceItem interface)
{
    QList<QAccessible::Action> actions;
    switch (interface.role()) {
        default:
            // Most things can be pressed.
            actions.append(QAccessible::Press);
        break;
    }

    return actions;
}

/*
    Translates a predefined QAccessible::Action to a Mac action constant.
    Returns an empty string if the Qt Action has no mac equivalent.
*/
static QCFString translateAction(const QAccessible::Action action)
{
    switch (action) {
        case QAccessible::Press:
            return kAXPressAction;
        break;
        case QAccessible::Increase:
            return kAXIncrementAction;
        break;
        case QAccessible::Decrease:
            return kAXDecrementAction;
        break;
        case QAccessible::Accept:
            return kAXConfirmAction;
        break;
        case QAccessible::Select:
            return kAXPickAction;
        break;
        case QAccessible::Cancel:
            return kAXCancelAction;
        break;
        default:
            return QCFString();
        break;
    }
}

/*
    Translates between a Mac action constant and a QAccessible::Action.
    Returns QAccessible::Default action if there is no Qt predefined equivalent.
*/
static QAccessible::Action translateAction(const CFStringRef actionName)
{
    if(CFStringCompare(actionName, kAXPressAction, 0) == kCFCompareEqualTo) {
        return QAccessible::Press;
    } else if(CFStringCompare(actionName, kAXIncrementAction, 0) == kCFCompareEqualTo) {
        return QAccessible::Increase;
    } else if(CFStringCompare(actionName, kAXDecrementAction, 0) == kCFCompareEqualTo) {
        return QAccessible::Decrease;
    } else if(CFStringCompare(actionName, kAXConfirmAction, 0) == kCFCompareEqualTo) {
        return QAccessible::Accept;
    } else if(CFStringCompare(actionName, kAXPickAction, 0) == kCFCompareEqualTo) {
        return QAccessible::Select;
    } else if(CFStringCompare(actionName, kAXCancelAction, 0) == kCFCompareEqualTo) {
        return QAccessible::Cancel;
    } else {
        return QAccessible::DefaultAction;
    }
}

/*
    Copies the translated names all supported actions for an interface into the kEventParamAccessibleActionNames
    event parameter.
*/
static OSStatus getAllActionNames(EventHandlerCallRef next_ref, EventRef event, QInterfaceItem interface)
{
    Q_UNUSED(next_ref);

    CFMutableArrayRef actions = 0;
    GetEventParameter(event, kEventParamAccessibleActionNames, typeCFMutableArrayRef, 0,
                      sizeof(actions), 0, &actions);

    // Add supported predefined actions.
    const QList<QAccessible::Action> predefinedActions = supportedPredefinedActions(interface);
    for (int i = 0; i < predefinedActions.count(); ++i) {
        const QCFString action = translateAction(predefinedActions.at(i));
        if (action != QCFString())
            qt_mac_append_cf_uniq(actions, action);
    }

    // Add user actions
    const int actionCount = interface.userActionCount();
    for (int i = 0; i < actionCount; ++i) {
        const QString actionName = interface.actionText(i, QAccessible::Name);
        qt_mac_append_cf_uniq(actions, QCFString::toCFStringRef(actionName));
    }

    return noErr;
}

/*
    Handles the perforNamedAction event.
*/
static OSStatus performNamedAction(EventHandlerCallRef next_ref, EventRef event, QInterfaceItem interface)
{
    Q_UNUSED(next_ref);

    CFStringRef act;
    GetEventParameter(event, kEventParamAccessibleActionName, typeCFStringRef, 0,
                      sizeof(act), 0, &act);

    const QAccessible::Action action = translateAction(act);

    // Perform built-in action
    if (action != QAccessible::DefaultAction) {
        interface.doAction(action, QVariantList());
        return noErr;
    }

    // Search for user-defined actions and perform it if found.
    const int actCount = interface.userActionCount();
    const QString qAct = QCFString::toQString(act);
    for(int i = 0; i < actCount; i++) {
        if(interface.actionText(i, QAccessible::Name) == qAct) {
            interface.doAction(i, QVariantList());
            break;
        }
    }
    return noErr;
}

static OSStatus setNamedAttribute(EventHandlerCallRef next_ref, EventRef event, QInterfaceItem interface)
{
    Q_UNUSED(next_ref);
    Q_UNUSED(event);

    CFStringRef var;
    GetEventParameter(event, kEventParamAccessibleAttributeName, typeCFStringRef, 0,
                      sizeof(var), 0, &var);
    if(CFStringCompare(var, kAXFocusedAttribute, 0) == kCFCompareEqualTo) {
        CFTypeRef val;
        if(GetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef, 0,
                             sizeof(val), 0, &val) == noErr) {
            if(CFGetTypeID(val) == CFBooleanGetTypeID() &&
               CFEqual(static_cast<CFBooleanRef>(val), kCFBooleanTrue)) {
                interface.doAction(QAccessible::SetFocus);
            }
        }
    } else {
        bool found = false;
        for(int r = 0; text_bindings[r][0].qt != -1; r++) {
            if(interface.role() == (QAccessible::Role)text_bindings[r][0].qt) {
                for(int a = 1; text_bindings[r][a].qt != -1; a++) {
                    if(CFStringCompare(var, text_bindings[r][a].mac, 0) == kCFCompareEqualTo) {
                        if(!text_bindings[r][a].settable) {
                        } else {
                            CFTypeRef val;
                            if(GetEventParameter(event, kEventParamAccessibleAttributeValue, typeCFTypeRef, 0,
                                                 sizeof(val), 0, &val) == noErr) {
                                if(CFGetTypeID(val) == CFStringGetTypeID())
                                    interface.setText((QAccessible::Text)text_bindings[r][a].qt,
                                                   QCFString::toQString(static_cast<CFStringRef>(val)));

                            }
                        }
                        found = true;
                        break;
                    }
                }
                break;
            }
        }
    }
    return noErr;
}

/*
    This is the main accessibility event handler.
*/
static OSStatus accessibilityEventHandler(EventHandlerCallRef next_ref, EventRef event, void *data)
{
    Q_UNUSED(next_ref)
    Q_UNUSED(data)

    // Return if this event is not a AccessibleGetNamedAttribute event.
    const UInt32 eclass = GetEventClass(event);
    if (eclass != kEventClassAccessibility)
        return eventNotHandledErr;

    // Get the AXUIElementRef and QInterfaceItem pointer
    AXUIElementRef element = 0;
    GetEventParameter(event, kEventParamAccessibleObject, typeCFTypeRef, 0, sizeof(element), 0, &element);
    const QInterfaceItem interface = accessibleHierarchyManager()->lookup(element);
    if (interface.isValid() == false)
        return eventNotHandledErr;

    const UInt32 ekind = GetEventKind(event);
    OSStatus status = noErr;
    switch (ekind) {
        case kEventAccessibleGetAllAttributeNames:
             status = getAllAttributeNames(event, interface, next_ref);
        break;
        case kEventAccessibleGetNamedAttribute:
             status = getNamedAttribute(next_ref, event, interface);
        break;
        case kEventAccessibleIsNamedAttributeSettable:
             status = isNamedAttributeSettable(event, interface);
        break;
        case kEventAccessibleGetChildAtPoint:
            status = getChildAtPoint(next_ref, event, interface);
        break;
        case kEventAccessibleGetAllActionNames:
            status = getAllActionNames(next_ref, event, interface);
        break;
        case kEventAccessibleGetFocusedChild:
            status = CallNextEventHandler(next_ref, event);
        break;
        case kEventAccessibleSetNamedAttribute:
            status = setNamedAttribute(next_ref, event, interface);
        break;
        case kEventAccessiblePerformNamedAction:
            status = performNamedAction(next_ref, event, interface);
        break;
        default:
            status = CallNextEventHandler(next_ref, event);
        break;
    };
    return status;
}

static OSStatus objectCreateEventHandler(EventHandlerCallRef next_ref, EventRef event, void *data)
{
    Q_UNUSED(data)
    Q_UNUSED(event)
    Q_UNUSED(next_ref)
    return noErr;
}

/*
    Registers the HIObject subclass used by the Qt mac accessibility framework.
*/
static void registerQtAccessibilityHIObjectSubclass()
{
    if (!objectCreateEventHandlerUPP)
        objectCreateEventHandlerUPP = NewEventHandlerUPP(objectCreateEventHandler);
    OSStatus err = HIObjectRegisterSubclass(kObjectQtAccessibility, 0, 0, objectCreateEventHandlerUPP,
                                         GetEventTypeCount(objectCreateEvents), objectCreateEvents, 0, 0);
    if (err && err != hiObjectClassExistsErr)
        qDebug() << "Error registreing subclass" << err;
}

void QAccessible::initialize()
{
    registerQtAccessibilityHIObjectSubclass();
    installApplicationEventhandler();
}

// Sets thre root object for the application
void QAccessible::setRootObject(QObject *object)
{
    // Call installed root object handler if we have one
    if (rootObjectHandler) {
        rootObjectHandler(object);
        return;
    }

    rootObject = object;
}

void QAccessible::cleanup()
{
    accessibleHierarchyManager()->reset();
    removeEventhandler(applicationEventHandlerUPP);
    removeEventhandler(objectCreateEventHandlerUPP);
    removeEventhandler(accessibilityEventHandlerUPP);
}

void QAccessible::updateAccessibility(QObject *object, int child, Event reason)
{
    // Call installed update handler if we have one.
    if (updateHandler) {
        updateHandler(object, child, reason);
        return;
    }

    // Return if the mac accessibility is not enabled.
    if(!AXAPIEnabled())
        return;

    const QAXUIElement element = accessibleHierarchyManager()->lookup(object, child);
    if (element.isValid() == false)
        return;

    CFStringRef notification = 0;
    if(object && object->isWidgetType() && reason == ObjectCreated) {
        notification = kAXWindowCreatedNotification;
    } else if(reason == ValueChanged) {
        notification = kAXValueChangedNotification;
    } else if(reason == MenuStart) {
        notification = kAXMenuOpenedNotification;
    } else if(reason == MenuEnd) {
        notification = kAXMenuClosedNotification;
    } else if(reason == LocationChanged) {
        notification = kAXWindowMovedNotification;
    } else if(reason == ObjectShow || reason == ObjectHide ) {
        // There is no equivalent Mac notification for ObjectShow/Hide, so we call HIObjectSetAccessibilityIgnored
        // and isItIntersting which will mark the HIObject accociated with the element as ignored if the
        // QAccessible::Invisible state bit is set.
        const QInterfaceItem interface = accessibleHierarchyManager()->lookup(element.element());
        HIObjectSetAccessibilityIgnored(element.object(), !isItInteresting(interface));

        // If the interface manages its own children, also check if we should ignore those.
        for (int i = 1; i <= interface.childCount(); ++i) {
            const QInterfaceItem childInterface = interface.navigate(QAccessible::Child, i);
            if (childInterface.isValid() && childInterface.isHIView() == false) {
                const AXUIElementRef element = accessibleHierarchyManager()->lookup(childInterface);
                if (element)
                    HIObjectSetAccessibilityIgnored(AXUIElementGetHIObject(element), !isItInteresting(childInterface));
            }
        }
    } else if(reason == Focus) {
        if(object && object->isWidgetType()) {
            QWidget *w = static_cast<QWidget*>(object);
            if(w->isWindow())
                notification = kAXFocusedWindowChangedNotification;
            else
                notification = kAXFocusedUIElementChangedNotification;
        }
    }

    if (!notification)
        return;

    AXNotificationHIObjectNotify(notification, element.object(), element.id());
}

#include "qaccessible_mac.moc"
#endif
