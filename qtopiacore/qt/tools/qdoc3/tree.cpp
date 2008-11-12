/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
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

/*
  tree.cpp
*/

#include <QtCore>
#include <QDomDocument>

#include "atom.h"
#include "htmlgenerator.h"
#include "location.h"
#include "node.h"
#include "text.h"
#include "tree.h"

struct InheritanceBound
{
    Node::Access access;
    QStringList basePath;
    QString dataTypeWithTemplateArgs;

    InheritanceBound()
	: access(Node::Public) { }
    InheritanceBound( Node::Access access0, const QStringList& basePath0,
		      const QString &dataTypeWithTemplateArgs0)
	: access(access0), basePath(basePath0),
	  dataTypeWithTemplateArgs(dataTypeWithTemplateArgs0) { }
};

struct Target
{
    Node *node;
    Atom *atom;
    int priority;
};

typedef QMap<PropertyNode::FunctionRole, QString> RoleMap;
typedef QMap<PropertyNode *, RoleMap> PropertyMap;
typedef QMultiMap<QString, Node *> GroupMap;
typedef QMultiHash<QString, FakeNode *> FakeNodeHash;
typedef QMultiHash<QString, Target> TargetHash;

class TreePrivate
{
public:
    QMap<ClassNode *, QList<InheritanceBound> > unresolvedInheritanceMap;
    PropertyMap unresolvedPropertyMap;
    GroupMap groupMap;
    FakeNodeHash fakeNodesByTitle;
    TargetHash targetHash;
    QList<QPair<ClassNode*,QString> > basesList;
    QList<QPair<FunctionNode*,QString> > relatedList;
};

Tree::Tree()
    : roo( 0, "" )
{
    priv = new TreePrivate;
}

Tree::~Tree()
{
    delete priv;
}

Node *Tree::findNode(const QStringList &path, Node *relative, int findFlags)
{
    return const_cast<Node *>(const_cast<const Tree *>(this)->findNode(path, relative, findFlags));
}

const Node *Tree::findNode(const QStringList &path, const Node *relative, int findFlags) const
{
    if (!relative)
        relative = root();

    do {
        const Node *node = relative;
        int i;

        for (i = 0; i < path.size(); ++i) {
	    if (node == 0 || !node->isInnerNode())
	        break;

            const Node *next = static_cast<const InnerNode *>(node)->findNode(path.at(i));
            if (!next && (findFlags & SearchEnumValues) && i == path.size() - 1)
                next = static_cast<const InnerNode *>(node)->findEnumNodeForValue(path.at(i));

            if (!next && node->type() == Node::Class && (findFlags & SearchBaseClasses)) {
                NodeList baseClasses = allBaseClasses(static_cast<const ClassNode *>(node));
                foreach (const Node *baseClass, baseClasses) {
                    next = static_cast<const InnerNode *>(baseClass)->findNode(path.at(i));
                    if (!next && (findFlags & SearchEnumValues) && i == path.size() - 1)
                        next = static_cast<const InnerNode *>(baseClass)
                                        ->findEnumNodeForValue(path.at(i));
                    if (next)
                        break;
                }
            }
            node = next;
        }
        if (node && i == path.size()
                && (!(findFlags & NonFunction) || node->type() != Node::Function
                    || ((FunctionNode *)node)->metaness() == FunctionNode::MacroWithoutParams))
            return node;
        relative = relative->parent();
    } while (relative);

    return 0;
}

Node *Tree::findNode(const QStringList &path, Node::Type type, Node *relative, int findFlags)
{
    return const_cast<Node *>(const_cast<const Tree *>(this)->findNode(path, type, relative,
                                                                       findFlags));
}

const Node *Tree::findNode(const QStringList &path, Node::Type type, const Node *relative,
                           int findFlags) const
{
    const Node *node = findNode(path, relative, findFlags);
    if (node != 0 && node->type() == type)
	return node;
    return 0;
}

FunctionNode *Tree::findFunctionNode(const QStringList& path, Node *relative, int findFlags)
{
    return const_cast<FunctionNode *>(
                const_cast<const Tree *>(this)->findFunctionNode(path, relative, findFlags));
}

const FunctionNode *Tree::findFunctionNode(const QStringList &path, const Node *relative,
                                           int findFlags) const
{
    if (!relative)
        relative = root();
    do {
        const Node *node = relative;
        int i;

        for (i = 0; i < path.size(); ++i) {
	    if (node == 0 || !node->isInnerNode())
	        break;

            const Node *next;
            if (i == path.size() - 1)
                next = ((InnerNode *) node)->findFunctionNode(path.at(i));
            else
                next = ((InnerNode *) node)->findNode(path.at(i));

            if (!next && node->type() == Node::Class && (findFlags & SearchBaseClasses)) {
                NodeList baseClasses = allBaseClasses(static_cast<const ClassNode *>(node));
                foreach (const Node *baseClass, baseClasses) {
                    if (i == path.size() - 1)
                        next = static_cast<const InnerNode *>(baseClass)->
                                findFunctionNode(path.at(i));
                    else
                        next = static_cast<const InnerNode *>(baseClass)->findNode(path.at(i));

                    if (next)
                        break;
                }
            }

            node = next;
        }
        if (node && i == path.size() && node->type() == Node::Function) {
            // CppCodeParser::processOtherMetaCommand ensures that reimplemented
            // functions are private.
            const FunctionNode *func = static_cast<const FunctionNode*>(node);

            while (func->access() == Node::Private) {
                const FunctionNode *from = func->reimplementedFrom();
                if (from != 0) {
                    if (from->access() != Node::Private)
                        return from;
                    else
                        func = from;
                } else
                    break;
            }
            return func;
        }
        relative = relative->parent();
    } while (relative);

    return 0;
}

FunctionNode *Tree::findFunctionNode(const QStringList &parentPath, const FunctionNode *clone,
                                     Node *relative, int findFlags)
{
    return const_cast<FunctionNode *>(
		const_cast<const Tree *>(this)->findFunctionNode(parentPath, clone,
                                      				 relative, findFlags));
}

const FunctionNode *Tree::findFunctionNode(const QStringList &parentPath, const FunctionNode *clone,
                                           const Node *relative, int findFlags) const
{
    const Node *parent = findNode(parentPath, relative, findFlags);
    if ( parent == 0 || !parent->isInnerNode() ) {
	return 0;
    } else {
	return ((InnerNode *)parent)->findFunctionNode(clone);
    }
}

static const int NumSuffixes = 3;
static const char * const suffixes[NumSuffixes] = { "", "s", "es" };

const FakeNode *Tree::findFakeNodeByTitle(const QString &title) const
{
    for (int pass = 0; pass < NumSuffixes; ++pass) {
        FakeNodeHash::const_iterator i =
                priv->fakeNodesByTitle.find(Doc::canonicalTitle(title + suffixes[pass]));
        if (i != priv->fakeNodesByTitle.constEnd()) {
            FakeNodeHash::const_iterator j = i;
            ++j;
            if (j != priv->fakeNodesByTitle.constEnd() && j.key() == i.key()) {
                i.value()->doc().location().warning(
                    tr("Page '%1' defined in more than one location").arg(title));
                while (j != priv->fakeNodesByTitle.constEnd()) {
                    if (j.key() == i.key())
                        j.value()->doc().location().warning(tr("(defined here)"));
                    ++j;
                }
            }
            return i.value();
        }
    }
    return 0;
}

const Node *Tree::findUnambiguousTarget(const QString &target, Atom *&atom) const
{
    Target bestTarget = {0, 0, INT_MAX};
    int numBestTargets = 0;

    for (int pass = 0; pass < NumSuffixes; ++pass) {
        TargetHash::const_iterator i =
                priv->targetHash.find(Doc::canonicalTitle(target + suffixes[pass]));
        if (i != priv->targetHash.constEnd()) {
            TargetHash::const_iterator j = i;
            do {
                const Target &candidate = j.value();
                if (candidate.priority < bestTarget.priority) {
                    bestTarget = candidate;
                    numBestTargets = 1;
                } else if (candidate.priority == bestTarget.priority) {
                    ++numBestTargets;
                }
                ++j;
            } while (j != priv->targetHash.constEnd() && j.key() == i.key());

            if (numBestTargets == 1) {
                atom = bestTarget.atom;
                return bestTarget.node;
            }
        }
    }
    return 0;
}

Atom *Tree::findTarget(const QString &target, const Node *node) const
{
    for (int pass = 0; pass < NumSuffixes; ++pass) {
        QString key = Doc::canonicalTitle(target + suffixes[pass]);
        TargetHash::const_iterator i = priv->targetHash.find(key);

        if (i != priv->targetHash.constEnd()) {
            do {
                if (i.value().node == node)
                    return i.value().atom;
                ++i;
            } while (i != priv->targetHash.constEnd() && i.key() == key);
        }
    }
    return 0;
}

void Tree::addBaseClass( ClassNode *subclass, Node::Access access,
			 const QStringList &basePath,
			 const QString &dataTypeWithTemplateArgs )
{
    priv->unresolvedInheritanceMap[subclass].append(
	    InheritanceBound(access, basePath, dataTypeWithTemplateArgs));
}


void Tree::addPropertyFunction(PropertyNode *property, const QString &funcName,
			       PropertyNode::FunctionRole funcRole)
{
    priv->unresolvedPropertyMap[property].insert(funcRole, funcName);
}

void Tree::addToGroup(Node *node, const QString &group)
{
    priv->groupMap.insert(group, node);
}

void Tree::resolveInheritance(NamespaceNode *rootNode)
{
    if (!rootNode)
        rootNode = root();

    for ( int pass = 0; pass < 2; pass++ ) {
	NodeList::ConstIterator c = rootNode->childNodes().begin();
	while ( c != rootNode->childNodes().end() ) {
	    if ( (*c)->type() == Node::Class )
		resolveInheritance( pass, (ClassNode *) *c );
            else if ( (*c)->type() == Node::Namespace ) {
                NamespaceNode *ns = static_cast<NamespaceNode*>(*c);
                resolveInheritance( ns );
            }
	    ++c;
	}
        if (rootNode == root())
	    priv->unresolvedInheritanceMap.clear();
    }
}

void Tree::resolveProperties()
{
    PropertyMap::ConstIterator propEntry;

    propEntry = priv->unresolvedPropertyMap.begin();
    while (propEntry != priv->unresolvedPropertyMap.end()) {
	PropertyNode *property = propEntry.key();
        InnerNode *parent = property->parent();
	QString getterName = (*propEntry)[PropertyNode::Getter];
	QString setterName = (*propEntry)[PropertyNode::Setter];
	QString resetterName = (*propEntry)[PropertyNode::Resetter];

	NodeList::ConstIterator c = parent->childNodes().begin();
        while (c != parent->childNodes().end()) {
	    if ((*c)->type() == Node::Function) {
		FunctionNode *function = static_cast<FunctionNode *>(*c);
                if (function->status() == property->status()
                        && function->access() == property->access()) {
		    if (function->name() == getterName) {
	                property->addFunction(function, PropertyNode::Getter);
	            } else if (function->name() == setterName) {
	                property->addFunction(function, PropertyNode::Setter);
	            } else if (function->name() == resetterName) {
	                property->addFunction(function, PropertyNode::Resetter);
                    }
                }
	    }
	    ++c;
        }
	++propEntry;
    }

    propEntry = priv->unresolvedPropertyMap.begin();
    while (propEntry != priv->unresolvedPropertyMap.end()) {
	PropertyNode *property = propEntry.key();
        // redo it to set the property functions
        if (property->overriddenFrom())
            property->setOverriddenFrom(property->overriddenFrom());
	++propEntry;
    }

    priv->unresolvedPropertyMap.clear();
}

void Tree::resolveInheritance(int pass, ClassNode *classe)
{
    if ( pass == 0 ) {
	QList<InheritanceBound> bounds = priv->unresolvedInheritanceMap[classe];
	QList<InheritanceBound>::ConstIterator b = bounds.begin();
	while ( b != bounds.end() ) {
	    ClassNode *baseClass = (ClassNode *)findNode((*b).basePath, Node::Class);
	    if (baseClass)
		classe->addBaseClass((*b).access, baseClass, (*b).dataTypeWithTemplateArgs);
	    ++b;
	}
    } else {
	NodeList::ConstIterator c = classe->childNodes().begin();
	while ( c != classe->childNodes().end() ) {
	    if ( (*c)->type() == Node::Function ) {
		FunctionNode *func = (FunctionNode *) *c;
		FunctionNode *from = findVirtualFunctionInBaseClasses( classe, func );
		if ( from != 0 ) {
		    if ( func->virtualness() == FunctionNode::NonVirtual )
			func->setVirtualness( FunctionNode::ImpureVirtual );
		    func->setReimplementedFrom( from );
		}
	    } else if ((*c)->type() == Node::Property) {
                fixPropertyUsingBaseClasses(classe, static_cast<PropertyNode *>(*c));
            }
	    ++c;
	}
    }
}

void Tree::resolveGroups()
{
    GroupMap::const_iterator i;
    QString prevGroup;
    for (i = priv->groupMap.constBegin(); i != priv->groupMap.constEnd(); ++i) {
        if (i.value()->access() == Node::Private)
            continue;

        FakeNode *fake = static_cast<FakeNode *>(findNode(QStringList(i.key()), Node::Fake));
        if (fake && fake->subType() == FakeNode::Group) {
            fake->addGroupMember(i.value());
        } else {
            if (prevGroup != i.key())
                i.value()->doc().location().warning(tr("No such group '%1'").arg(i.key()));
        }

        prevGroup = i.key();
    }

    priv->groupMap.clear();
}

void Tree::resolveTargets()
{
    // need recursion

    foreach (Node *child, roo.childNodes()) {
        if (child->type() == Node::Fake) {
            FakeNode *node = static_cast<FakeNode *>(child);
            priv->fakeNodesByTitle.insert(Doc::canonicalTitle(node->title()), node);
        }

        if (child->doc().hasTableOfContents()) {
            const QList<Atom *> &toc = child->doc().tableOfContents();
            Target target;
            target.node = child;
            target.priority = 3;

            for (int i = 0; i < toc.size(); ++i) {
                target.atom = toc.at(i);
                QString title = Text::sectionHeading(target.atom).toString();
                if (!title.isEmpty())
                    priv->targetHash.insert(Doc::canonicalTitle(title), target);
            }
        }
        if (child->doc().hasKeywords()) {
            const QList<Atom *> &keywords = child->doc().keywords();
            Target target;
            target.node = child;
            target.priority = 1;

            for (int i = 0; i < keywords.size(); ++i) {
                target.atom = keywords.at(i);
                priv->targetHash.insert(Doc::canonicalTitle(target.atom->string()), target);
            }
        }
        if (child->doc().hasTargets()) {
            const QList<Atom *> &toc = child->doc().targets();
            Target target;
            target.node = child;
            target.priority = 2;

            for (int i = 0; i < toc.size(); ++i) {
                target.atom = toc.at(i);
                priv->targetHash.insert(Doc::canonicalTitle(target.atom->string()), target);
            }
        }
    }
}

void Tree::fixInheritance(NamespaceNode *rootNode)
{
    if (!rootNode)
        rootNode = root();

    NodeList::ConstIterator c = rootNode->childNodes().begin();
    while ( c != rootNode->childNodes().end() ) {
	if ( (*c)->type() == Node::Class )
	    static_cast<ClassNode *>(*c)->fixBaseClasses();
        else if ( (*c)->type() == Node::Namespace ) {
            NamespaceNode *ns = static_cast<NamespaceNode*>(*c);
            fixInheritance( ns );
        }
	++c;
    }
}

FunctionNode *Tree::findVirtualFunctionInBaseClasses(ClassNode *classe, FunctionNode *clone)
{
    QList<RelatedClass>::ConstIterator r = classe->baseClasses().begin();
    while ( r != classe->baseClasses().end() ) {
	FunctionNode *func;
        if ( ((func = findVirtualFunctionInBaseClasses((*r).node, clone)) != 0 ||
	      (func = (*r).node->findFunctionNode(clone)) != 0) ) {
	    if (func->virtualness() != FunctionNode::NonVirtual)
	        return func;
        }
 	++r;
    }
    return 0;
}

void Tree::fixPropertyUsingBaseClasses(ClassNode *classe, PropertyNode *property)
{
    QList<RelatedClass>::const_iterator r = classe->baseClasses().begin();
    while (r != classe->baseClasses().end()) {
	PropertyNode *baseProperty = static_cast<PropertyNode *>(r->node->findNode(property->name(), Node::Property));
        if (baseProperty) {
            fixPropertyUsingBaseClasses(r->node, baseProperty);
            property->setOverriddenFrom(baseProperty);
        } else {
            fixPropertyUsingBaseClasses(r->node, property);
        }
 	++r;
    }
}

NodeList Tree::allBaseClasses(const ClassNode *classe) const
{
    NodeList result;
    foreach (RelatedClass r, classe->baseClasses()) {
        result += r.node;
        result += allBaseClasses(r.node);
    }
    return result;
}

void Tree::readIndexes(const QStringList &indexFiles)
{
    foreach (QString indexFile, indexFiles)
        readIndexFile(indexFile);
}

void Tree::readIndexFile(const QString &path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        QDomDocument document;
        document.setContent(&file);
        file.close();

        QDomElement indexElement = document.documentElement();
        QString indexUrl = indexElement.attribute("url", "");
        priv->basesList.clear();
        priv->relatedList.clear();

        // Scan all elements in the XML file, constructing a map that contains
        // base classes for each class found.

        QDomElement child = indexElement.firstChildElement();
        while (!child.isNull()) {
            readIndexSection(child, root(), indexUrl);
            child = child.nextSiblingElement();
        }

        // Now that all the base classes have been found for this index,
        // arrange them into an inheritance hierarchy.

        resolveIndex();
    }
}

void Tree::readIndexSection(const QDomElement &element,
    InnerNode *parent, const QString &indexUrl)
{
    QString name = element.attribute("name");

    Node *section;
    Location location;

    if (element.nodeName() == "namespace") {
        section = new NamespaceNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + "/" + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");

    } else if (element.nodeName() == "class") {
        section = new ClassNode(parent, name);
        priv->basesList.append(QPair<ClassNode*,QString>(
            static_cast<ClassNode*>(section), element.attribute("bases")));

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + "/" + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");

    } else if (element.nodeName() == "page") {
        FakeNode::SubType subtype;
        if (element.attribute("subtype") == "example")
            subtype = FakeNode::Example;
        else if (element.attribute("subtype") == "header")
            subtype = FakeNode::HeaderFile;
        else if (element.attribute("subtype") == "file")
            subtype = FakeNode::File;
        else if (element.attribute("subtype") == "group")
            subtype = FakeNode::Group;
        else if (element.attribute("subtype") == "module")
            subtype = FakeNode::Module;
        else if (element.attribute("subtype") == "page")
            subtype = FakeNode::Page;
        else if (element.attribute("subtype") == "externalpage")
            subtype = FakeNode::ExternalPage;
        else
            return;

        FakeNode *fakeNode = new FakeNode(parent, name, subtype);
        fakeNode->setTitle(element.attribute("title"));

        if (element.hasAttribute("location"))
            name = element.attribute("location", "");

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + "/" + name);
        else if (!indexUrl.isNull())
            location = Location(name);

        section = fakeNode;

    } else if (element.nodeName() == "enum") {
        EnumNode *enumNode = new EnumNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + "/" + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

        QDomElement child = element.firstChildElement("value");
        while (!child.isNull()) {
            EnumItem item(child.attribute("name"), child.attribute("value"));
            enumNode->addItem(item);
            child = child.nextSiblingElement("value");
        }

        section = enumNode;

    } else if (element.nodeName() == "typedef") {
        section = new TypedefNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + "/" + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    } else if (element.nodeName() == "property") {
        section = new PropertyNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + "/" + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    } else if (element.nodeName() == "function") {
        FunctionNode::Virtualness virt;
        if (element.attribute("virtual") == "non")
            virt = FunctionNode::NonVirtual;
        else if (element.attribute("virtual") == "impure")
            virt = FunctionNode::ImpureVirtual;
        else if (element.attribute("virtual") == "pure")
            virt = FunctionNode::PureVirtual;
        else
            return;

        FunctionNode::Metaness meta;
        if (element.attribute("meta") == "plain")
            meta = FunctionNode::Plain;
        else if (element.attribute("meta") == "signal")
            meta = FunctionNode::Signal;
        else if (element.attribute("meta") == "slot")
            meta = FunctionNode::Slot;
        else if (element.attribute("meta") == "constructor")
            meta = FunctionNode::Ctor;
        else if (element.attribute("meta") == "destructor")
            meta = FunctionNode::Dtor;
        else if (element.attribute("meta") == "macro")
            meta = FunctionNode::MacroWithParams;
        else if (element.attribute("meta") == "macrowithparams")
            meta = FunctionNode::MacroWithParams;
        else if (element.attribute("meta") == "macrowithoutparams")
            meta = FunctionNode::MacroWithoutParams;
        else
            return;

        FunctionNode *functionNode = new FunctionNode(parent, name);
        functionNode->setReturnType(element.attribute("return"));
        functionNode->setVirtualness(virt);
        functionNode->setMetaness(meta);
        functionNode->setConst(element.attribute("const") == "true");
        functionNode->setStatic(element.attribute("static") == "true");
        functionNode->setOverload(element.attribute("overload") == "true");

        if (element.hasAttribute("relates")
            && element.attribute("relates") != parent->name()) {
            priv->relatedList.append(
                QPair<FunctionNode*,QString>(functionNode,
                                             element.attribute("relates")));
        }

        QDomElement child = element.firstChildElement("parameter");
        while (!child.isNull()) {
            Parameter parameter(child.attribute("left"),
                                child.attribute("right"),
                                child.attribute("name"),
                                child.attribute("default"));
            functionNode->addParameter(parameter);
            child = child.nextSiblingElement("parameter");
        }

        section = functionNode;

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + "/" + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    } else if (element.nodeName() == "variable") {
        section = new VariableNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + "/" + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    } else if (element.nodeName() == "keyword") {
        Target target;
        target.node = parent;
        target.priority = 1;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    } else if (element.nodeName() == "target") {
        Target target;
        target.node = parent;
        target.priority = 2;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    } else if (element.nodeName() == "contents") {
        Target target;
        target.node = parent;
        target.priority = 3;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    } else
        return;

    QString access = element.attribute("access");
    if (access == "public")
        section->setAccess(Node::Public);
    else if (access == "protected")
        section->setAccess(Node::Protected);
    else if (access == "private")
        section->setAccess(Node::Private);
    else
        section->setAccess(Node::Public);

    if (element.nodeName() != "page") {
        QString threadSafety = element.attribute("threadsafety");
        if (threadSafety == "non-reentrant")
            section->setThreadSafeness(Node::NonReentrant);
        else if (threadSafety == "reentrant")
            section->setThreadSafeness(Node::Reentrant);
        else if (threadSafety == "thread safe")
            section->setThreadSafeness(Node::ThreadSafe);
        else
            section->setThreadSafeness(Node::UnspecifiedSafeness);
    } else
        section->setThreadSafeness(Node::UnspecifiedSafeness);

    section->setModuleName(element.attribute("module"));
    section->setUrl(indexUrl);

    // Create some content for the node.
    QSet<QString> emptySet;

    Doc doc(location, " ", emptySet); // placeholder
    section->setDoc(doc);

    InnerNode *inner = dynamic_cast<InnerNode*>(section);
    if (inner) {
        QDomElement child = element.firstChildElement();

        while (!child.isNull()) {
            if (element.nodeName() == "class")
                readIndexSection(child, inner, indexUrl);
            else if (element.nodeName() == "page")
                readIndexSection(child, inner, indexUrl);
            else if (element.nodeName() == "namespace" && !name.isEmpty())
                // The root node in the index is a namespace with an empty name.
                readIndexSection(child, inner, indexUrl);
            else
                readIndexSection(child, parent, indexUrl);

            child = child.nextSiblingElement();
        }
    }
}

QString Tree::readIndexText(const QDomElement &element)
{
    QString text;
    QDomNode child = element.firstChild();
    while (!child.isNull()) {
        if (child.isText())
            text += child.toText().nodeValue();
        child = child.nextSibling();
    }
    return text;
}

void Tree::resolveIndex()
{
    QPair<ClassNode*,QString> pair;

    foreach (pair, priv->basesList) {
        foreach (QString base, pair.second.split(",")) {
            Node *baseClass = root()->findNode(base, Node::Class);
            if (baseClass) {
                pair.first->addBaseClass(Node::Public,
                                         static_cast<ClassNode*>(baseClass));
            }
        }
    }

    QPair<FunctionNode*,QString> relatedPair;

    foreach (relatedPair, priv->relatedList) {
        Node *classNode = root()->findNode(relatedPair.second, Node::Class);
        if (classNode)
            relatedPair.first->setRelates(static_cast<ClassNode*>(classNode));
    }
}

QDomElement Tree::generateIndexSection(QDomDocument &document, const Node *node) const
{
    if (!node->url().isEmpty())
        return QDomElement();

    QString nodeName;
    switch (node->type()) {
        case Node::Namespace:
            nodeName = "namespace";
            break;
        case Node::Class:
            nodeName = "class";
            break;
        case Node::Fake:
            nodeName = "page";
            break;
        case Node::Enum:
            nodeName = "enum";
            break;
        case Node::Typedef:
            nodeName = "typedef";
            break;
        case Node::Property:
            nodeName = "property";
            break;
        case Node::Function:
            nodeName = "function";
            break;
        case Node::Variable:
            nodeName = "variable";
            break;
        case Node::Target:
            nodeName = "target";
            break;
        default:
            return QDomElement();
    }

    QDomElement element = document.createElement(nodeName);

    QString access;
    switch (node->access()) {
        case Node::Public:
            access = "public";
            break;
        case Node::Protected:
            access = "protected";
            break;
        case Node::Private:     // Do not include private nodes in the index.
        default:
            return QDomElement();
    }
    element.setAttribute("access", access);

    if (node->type() != Node::Fake) {
        QString threadSafety;
        switch (node->threadSafeness()) {
            case Node::NonReentrant:
                threadSafety = "non-reentrant";
                break;
            case Node::Reentrant:
                threadSafety = "reentrant";
                break;
            case Node::ThreadSafe:
                threadSafety = "thread safe";
                break;
            case Node::UnspecifiedSafeness:
            default:
                threadSafety = "unspecified";
                break;
        }
        element.setAttribute("threadsafety", threadSafety);
    }

    QString objName = node->name();
    element.setAttribute("name", objName);
    element.setAttribute("href", fullDocumentName(node));
    element.setAttribute("location", node->location().fileName());

    if (node->type() == Node::Class) {

        // Classes contain information about their base classes.
        if (objName.isEmpty())
            return QDomElement();

        const ClassNode *classNode = static_cast<const ClassNode*>(node);
        QList<RelatedClass> bases = classNode->baseClasses();
        QStringList baseStrings;
        foreach (RelatedClass related, bases) {
            ClassNode *baseClassNode = related.node;
            baseStrings.append(baseClassNode->name());
        }
        element.setAttribute("bases", baseStrings.join(","));
        element.setAttribute("module", node->moduleName());
    }

    else if (node->type() == Node::Namespace) {
        element.setAttribute("module", node->moduleName());
    }

    // Fake nodes (such as manual pages) contain subtypes, titles and other
    // attributes.

    else if (node->type() == Node::Fake) {

        if (objName.isEmpty())
            return QDomElement();

        const FakeNode *fakeNode = static_cast<const FakeNode*>(node);
        switch (fakeNode->subType()) {
            case FakeNode::Example:
                element.setAttribute("subtype", "example");
                break;
            case FakeNode::HeaderFile:
                element.setAttribute("subtype", "header");
                break;
            case FakeNode::File:
                element.setAttribute("subtype", "file");
                break;
            case FakeNode::Group:
                element.setAttribute("subtype", "group");
                break;
            case FakeNode::Module:
                element.setAttribute("subtype", "module");
                break;
            case FakeNode::Page:
                element.setAttribute("subtype", "page");
                break;
            case FakeNode::ExternalPage:
                element.setAttribute("subtype", "externalpage");
                break;
            default:
                break;
        }
        element.setAttribute("title", fakeNode->title());
        element.setAttribute("fulltitle", fakeNode->fullTitle());
        element.setAttribute("subtitle", fakeNode->subTitle());
        element.setAttribute("location", fakeNode->doc().location().fileName());
    }

    // Function nodes contain information about the type of function being
    // described.

    else if (node->type() == Node::Function) {

        if (objName.isEmpty())
            return QDomElement();

        const FunctionNode *functionNode = static_cast<const FunctionNode*>(node);

        switch (functionNode->virtualness()) {
            case FunctionNode::NonVirtual:
                element.setAttribute("virtual", "non");
                break;
            case FunctionNode::ImpureVirtual:
                element.setAttribute("virtual", "impure");
                break;
            case FunctionNode::PureVirtual:
                element.setAttribute("virtual", "pure");
                break;
            default:
                break;
        }
        switch (functionNode->metaness()) {
            case FunctionNode::Plain:
                element.setAttribute("meta", "plain");
                break;
            case FunctionNode::Signal:
                element.setAttribute("meta", "signal");
                break;
            case FunctionNode::Slot:
                element.setAttribute("meta", "slot");
                break;
            case FunctionNode::Ctor:
                element.setAttribute("meta", "constructor");
                break;
            case FunctionNode::Dtor:
                element.setAttribute("meta", "destructor");
                break;
            case FunctionNode::MacroWithParams:
                element.setAttribute("meta", "macrowithparams");
                break;
            case FunctionNode::MacroWithoutParams:
                element.setAttribute("meta", "macrowithoutparams");
                break;
            default:
                break;
        }
        element.setAttribute("const", functionNode->isConst()?"true":"false");
        element.setAttribute("static", functionNode->isStatic()?"true":"false");
        element.setAttribute("overload", functionNode->isOverload()?"true":"false");
        if (functionNode->relates())
            element.setAttribute("relates", functionNode->relates()->name());
    }

    // Inner nodes and function nodes contain child nodes of some sort, either
    // actual child nodes or function parameters. For these, we close the
    // opening tag, create child elements, then add a closing tag for the
    // element. Elements for all other nodes are closed in the opening tag.

    const InnerNode *inner = dynamic_cast<const InnerNode*>(node);
    if (inner) {

        // For internal pages, we canonicalize the target, keyword and content
        // item names so that they can be used by qdoc for other sets of
        // documentation.
        // The reason we do this here is that we don't want to ruin
        // externally composed indexes, containing non-qdoc-style target names
        // when reading in indexes.

        if (inner->doc().hasTargets()) {
            bool external = false;
            if (inner->type() == Node::Fake) {
                const FakeNode *fakeNode = static_cast<const FakeNode *>(inner);
                if (fakeNode->subType() == FakeNode::ExternalPage)
                    external = true;
            }

            foreach (Atom *target, inner->doc().targets()) {
                QString targetName = target->string();
                if (!external)
                    targetName = Doc::canonicalTitle(targetName);

                QDomElement childElement = document.createElement("target");
                childElement.setAttribute("name", targetName);
                element.appendChild(childElement);
            }
        }
        if (inner->doc().hasKeywords()) {
            foreach (Atom *keyword, inner->doc().keywords()) {
                QDomElement childElement = document.createElement("keyword");
                childElement.setAttribute("name", Doc::canonicalTitle(keyword->string()));
                element.appendChild(childElement);
            }
        }
        if (inner->doc().hasTableOfContents()) {
            foreach (Atom *item, inner->doc().tableOfContents()) {
                QString title = Text::sectionHeading(item).toString();
                QDomElement childElement = document.createElement("contents");
                childElement.setAttribute("name", Doc::canonicalTitle(title));
                childElement.setAttribute("title", title);
                element.appendChild(childElement);
            }
        }

    } else if (node->type() == Node::Function) {

        const FunctionNode *functionNode = static_cast<const FunctionNode*>(node);
        foreach (Parameter parameter, functionNode->parameters()) {
            // Do not supply a default value for the parameter; it will only
            // cause disagreement when it is read from an index file later on.
            QDomElement childElement = document.createElement("parameter");
            childElement.setAttribute("left", parameter.leftType());
            childElement.setAttribute("right", parameter.rightType());
            childElement.setAttribute("name", parameter.name());
            //childElement.setAttribute("default", parameter.defaultValue());
            element.appendChild(childElement);
        }

    } else if (node->type() == Node::Enum) {

        const EnumNode *enumNode = static_cast<const EnumNode*>(node);
        foreach (EnumItem item, enumNode->items()) {
            QDomElement childElement = document.createElement("value");
            childElement.setAttribute("name", item.name());
            childElement.setAttribute("value", item.value());
            element.appendChild(childElement);
        }
    }

    return element;
}

QDomElement Tree::generateIndexSections(QDomDocument &document,
                                       const Node *node) const
{
    QDomElement element = generateIndexSection(document, node);

    if (!element.isNull() && node->isInnerNode()) {
        const InnerNode *inner = static_cast<const InnerNode *>(node);

        foreach (Node *child, inner->childNodes()) {
            // Recurse to generate a DOM element for this child node and all
            // its children.
            QDomElement childElement = generateIndexSections(document, child);
            element.appendChild(childElement);
        }
/*
        foreach (Node *child, inner->relatedNodes()) {
            QDomElement childElement = generateIndexSections(document, child);
            element.appendChild(childElement);
        }
*/
    }

    return element;
}

void Tree::generateIndex(const QString &fileName, const QString &url,
                         const QString &title) const
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return ;

    QTextStream out(&file);
    QDomDocument document("QDOCINDEX");
    QDomProcessingInstruction process = document.createProcessingInstruction(
        "xml", QString("version=\"1.0\" encoding=\"%1\"").arg("iso-8859-1"));

    QDomElement indexElement = document.createElement("INDEX");
    indexElement.setAttribute("url", url);
    indexElement.setAttribute("title", title);
    indexElement.setAttribute("version", version());

    document.appendChild(process);
    document.appendChild(indexElement);

    QDomElement rootElement = generateIndexSections(document, root());
    indexElement.appendChild(rootElement);

    out << document;
    out.flush();
}

void Tree::addExternalLink(const QString &url, const Node *relative)
{
    FakeNode *fakeNode = new FakeNode(root(), url, FakeNode::ExternalPage);
    fakeNode->setAccess(Node::Public);

    // Create some content for the node.
    QSet<QString> emptySet;
    Location location(relative->doc().location());
    Doc doc(location, " ", emptySet); // placeholder
    fakeNode->setDoc(doc);
}

QString Tree::fullDocumentName(const Node *node) const
{
    if (node->type() == Node::Namespace) {

        // The root namespace has no name - check for this before creating
        // an attribute containing the location of any documentation.

        if (!node->name().isEmpty())
            return node->name().toLower() + ".html";
        else
            return "";
    } else if (node->type() == Node::Fake) {
        QString base = node->name();
	base.replace(QRegExp("[^A-Za-z0-9.]+"), "");
	base = base.trimmed();
	base = base.toLower();
        return base;
    }

    QString parentName;
    Node *parentNode = 0;

    if ((parentNode = node->relates()))
        parentName = fullDocumentName(node->relates());
    else if ((parentNode = node->parent()))
        parentName = fullDocumentName(node->parent());

    switch (node->type()) {
        case Node::Class:
            if (parentNode && parentNode->type() == Node::Class)
                return parentNode->name().toLower()
                     + "-" + node->name().toLower()
                     + ".html";
            else
                return node->name().toLower() + ".html";
        case Node::Function:
            {
                // Functions can be overloaded.
                const FunctionNode *functionNode = static_cast<const FunctionNode *>(node);
                if (functionNode->overloadNumber() > 1)
                    return parentName + "#" + node->name()
                           + "-" + QString::number(functionNode->overloadNumber());
                return parentName + "#" + node->name();
            }

        case Node::Enum:
            return parentName + "#" + node->name() + "-enum";
        case Node::Typedef:
            return parentName + "#" + node->name() + "-typedef";
        case Node::Property:
            return parentName + "#" + node->name() + "-prop";
        case Node::Variable:
            return parentName + "#" + node->name() + "-var";
        case Node::Target:
            return parentName + "#" + node->name();
        case Node::Namespace:
        case Node::Fake:
        default:
            break;
    }

    return "";
}
