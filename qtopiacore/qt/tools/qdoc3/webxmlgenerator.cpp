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
  webxmlgenerator.cpp
*/

#include "codemarker.h"
#include "pagegenerator.h"
#include "webxmlgenerator.h"
#include "node.h"
#include "separator.h"
#include "tree.h"
#include <ctype.h>

#include <qdebug.h>
#include <qlist.h>
#include <qiterator.h>

#define COMMAND_VERSION                 Doc::alias("version")

WebXMLGenerator::WebXMLGenerator()
    : PageGenerator()
{
}

WebXMLGenerator::~WebXMLGenerator()
{
}

void WebXMLGenerator::initializeGenerator(const Config &config)
{
    Generator::initializeGenerator(config);
}

void WebXMLGenerator::terminateGenerator()
{
    PageGenerator::terminateGenerator();
}

QString WebXMLGenerator::format()
{
    return "WebXML";
}

QString WebXMLGenerator::fileExtension(const Node * /* node */)
{
    return "xml";
}

void WebXMLGenerator::generateTree(const Tree *tree, CodeMarker *marker)
{
    tre = tree;
    PageGenerator::generateTree(tree, marker);
}

void WebXMLGenerator::startText(const Node *relative, CodeMarker *marker)
{
    inLink = false;
    inContents = false;
    inSectionHeading = false;
    numTableRows = 0;
    sectionNumber.clear();
    PageGenerator::startText(relative, marker);
}

int WebXMLGenerator::generateAtom(const Atom *atom, const Node *relative, CodeMarker *marker)
{
    int skipAhead = 0;

    switch (atom->type()) {
    default:
        PageGenerator::generateAtom(atom, relative, marker);
    }
    return skipAhead;
}

void WebXMLGenerator::generateClassLikeNode(const InnerNode *inner, CodeMarker *marker)
{
    QDomDocument document("WebXML");
    QDomElement documentElement = document.createElement("document");
    //documentElement.setAttribute("version", tre->version());

    QDomElement contentElement = generateIndexSections(document, inner, marker);
    documentElement.appendChild(contentElement);

    QDomProcessingInstruction process = document.createProcessingInstruction(
        "xml", QString("version=\"1.0\" encoding=\"%1\"").arg("iso-8859-1"));
    document.appendChild(process);
    document.appendChild(documentElement);

    out() << document;
    out().flush();
}

void WebXMLGenerator::generateFakeNode( const FakeNode *fake, CodeMarker *marker )
{
    QDomDocument document("WebXML");
    QDomElement documentElement = document.createElement("document");
    //documentElement.setAttribute("version", tre->version());

    QDomElement contentElement = generateIndexSections(document, fake, marker);
    documentElement.appendChild(contentElement);

    QDomProcessingInstruction process = document.createProcessingInstruction(
        "xml", QString("version=\"1.0\" encoding=\"%1\"").arg("iso-8859-1"));
    document.appendChild(process);
    document.appendChild(documentElement);

    out() << document;
    out().flush();
}

QDomElement WebXMLGenerator::generateIndexSections(QDomDocument &document,
                                 const Node *node, CodeMarker *marker)
{
    QDomElement element = tre->generateIndexSection(document, node);

    if (node->isInnerNode()) {
        const InnerNode *inner = static_cast<const InnerNode *>(node);

        foreach (Node *child, inner->childNodes()) {
            // Recurse to generate a DOM element for this child node and all
            // its children.
            QDomElement childElement = generateIndexSections(document, child, marker);
            element.appendChild(childElement);
        }
/*
        foreach (Node *child, inner->relatedNodes()) {
            QDomElement childElement = generateIndexSections(document, child, marker);
            element.appendChild(childElement);
        }
*/
    }

    // Add documentation to this node if it exists.
    if (!node->doc().isEmpty()) {
        QDomElement descriptionElement = document.createElement("description");
        startText(node, marker);

        const Atom *atom = node->doc().body().firstAtom();
        while (atom)
            atom = addAtomElements(descriptionElement, atom, node, marker);
        element.appendChild(descriptionElement);
    }

    return element;
}

const Atom *WebXMLGenerator::addAtomElements(QDomElement &parent, const Atom *atom,
                                      const Node *relative, CodeMarker *marker)
{
    QDomElement atomElement;
    QDomDocument document = parent.ownerDocument();
    QDomText textNode;

    switch (atom->type()) {
    case Atom::AbstractLeft:
    case Atom::AbstractRight:
        break;
    case Atom::AutoLink:
        if (!inLink && !inContents && !inSectionHeading) {
            atomElement = document.createElement("link");
            atomElement.setAttribute("href", atom->string());
            textNode = document.createTextNode(atom->string());
            atomElement.appendChild(textNode);
        }
        break;
    case Atom::BaseName:
        break;
    case Atom::BriefLeft:

        atomElement = document.createElement("brief");
        if (relative->type() == Node::Property) {
            textNode = document.createTextNode("This property holds ");
            atomElement.appendChild(textNode);
        } else {
            textNode = document.createTextNode("This variable holds ");
            atomElement.appendChild(textNode);
        }
        atom = atom->next();
        while (atom && atom->type() != Atom::BriefRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        if (relative->type() == Node::Property || relative->type() == Node::Variable) {
            textNode = document.createTextNode(".");
            atomElement.appendChild(textNode);
        }
        break;

    case Atom::BriefRight:
        break;
    case Atom::C:
        atomElement = document.createElement("teletype");
        if (inLink)
            atomElement.setAttribute("type", "normal");
        else
            atomElement.setAttribute("type", "highlighted");

        textNode = document.createTextNode(plainCode(atom->string()));
        atomElement.appendChild(textNode);
        break;

    case Atom::Code:
        atomElement = document.createElement("code");
        textNode = document.createTextNode(trimmedTrailing(plainCode(atom->string())));
        atomElement.appendChild(textNode);
        break;

    case Atom::CodeBad:
        atomElement = document.createElement("badcode");
        textNode = document.createTextNode(trimmedTrailing(plainCode(atom->string())));
        atomElement.appendChild(textNode);
        break;

    case Atom::CodeNew:
        {
            QDomElement paragraphElement = document.createElement("para");
            QDomText paragraphText = document.createTextNode(
                                     "you can rewrite it as");
            paragraphElement.appendChild(paragraphText);
            parent.appendChild(paragraphElement);

            atomElement = document.createElement("newcode");
            textNode = document.createTextNode(trimmedTrailing(
                                               plainCode(atom->string())));
            atomElement.appendChild(textNode);
        }
        break;

    case Atom::CodeOld:
        {
            QDomElement paragraphElement = document.createElement("para");
            QDomText paragraphText = document.createTextNode(
                                     "For example, if you have code like");
            paragraphElement.appendChild(paragraphText);
            parent.appendChild(paragraphElement);

            atomElement = document.createElement("oldcode");
            textNode = document.createTextNode(trimmedTrailing(
                                               plainCode(atom->string())));
            atomElement.appendChild(textNode);
        }
        break;

    case Atom::FootnoteLeft:

        atomElement = document.createElement("footnote");
        atom = atom->next();
        while (atom && atom->type() != Atom::FootnoteRight)
            atom = addAtomElements(atomElement, atom, relative, marker);
        break;

    case Atom::FootnoteRight:
        break;
    case Atom::FormatElse:
    case Atom::FormatEndif:
    case Atom::FormatIf:
        break;
    case Atom::FormattingLeft:
/*        out() << formattingLeftMap()[atom->string()];
        if ( atom->string() == ATOM_FORMATTING_PARAMETER ) {
            if ( atom->next() != 0 && atom->next()->type() == Atom::String ) {
                QRegExp subscriptRegExp( "([a-z]+)_([0-9n])" );
                if ( subscriptRegExp.exactMatch(atom->next()->string()) ) {
                    out() << subscriptRegExp.cap( 1 ) << "<sub>"
                          << subscriptRegExp.cap( 2 ) << "</sub>";
                    skipAhead = 1;
                }
            }
        }*/
        break;
    case Atom::FormattingRight:
/*        if ( atom->string() == ATOM_FORMATTING_LINK ) {
            if (inLink) {
                if ( link.isEmpty() ) {
                    if (showBrokenLinks)
                        out() << "</i>";
                } else {
                    out() << "</a>";
                }
            }
            inLink = false;
        } else {
            out() << formattingRightMap()[atom->string()];
        }*/
        break;
    case Atom::GeneratedList:
/*        if (atom->string() == "annotatedclasses") {
            generateAnnotatedList(relative, marker, nonCompatClasses);
        } else if (atom->string() == "classes") {
            generateCompactList(relative, marker, nonCompatClasses);
        } else if (atom->string().contains("classesbymodule")) {
            QString arg = atom->string().trimmed();
            QString moduleName = atom->string().mid(atom->string().indexOf(
                "classesbymodule") + 15).trimmed();
            if (moduleClassMap.contains(moduleName))
                generateAnnotatedList(relative, marker, moduleClassMap[moduleName]);
        } else if (atom->string().contains("classesbyedition")) {
            QString arg = atom->string().trimmed();
            QString editionName = atom->string().mid(atom->string().indexOf(
                "classesbyedition") + 16).trimmed();
            if (editionModuleMap.contains(editionName)) {
                QMap<QString, const Node *> editionClasses;
                foreach (QString moduleName, editionModuleMap[editionName]) {
                    if (moduleClassMap.contains(moduleName))
                        editionClasses.unite(moduleClassMap[moduleName]);
                }
                generateAnnotatedList(relative, marker, editionClasses);
            }
        } else if (atom->string() == "classhierarchy") {
            generateClassHierarchy(relative, marker, nonCompatClasses);
        } else if (atom->string() == "compatclasses") {
            generateCompactList(relative, marker, compatClasses);
        } else if (atom->string() == "functionindex") {
            generateFunctionIndex(relative, marker);
        } else if (atom->string() == "legalese") {
            generateLegaleseList(relative, marker);
        } else if (atom->string() == "mainclasses") {
            generateCompactList(relative, marker, mainClasses);
        } else if (atom->string() == "services") {
            generateCompactList(relative, marker, serviceClasses);
        } else if (atom->string() == "overviews") {
            generateOverviewList(relative, marker);
        } else if (atom->string() == "namespaces") {
            generateAnnotatedList(relative, marker, namespaceIndex);
        } else if (atom->string() == "related") {
            const FakeNode *fake = static_cast<const FakeNode *>(relative);
            if (fake && !fake->groupMembers().isEmpty()) {
                QMap<QString, const Node *> groupMembersMap;
                foreach (Node *node, fake->groupMembers()) {
                    if (node->type() == Node::Fake)
                        groupMembersMap[node->name()] = node;
                }
                generateAnnotatedList(fake, marker, groupMembersMap);
            }
        }
*/
        break;
    case Atom::Image:
        atomElement = document.createElement("image");
        atomElement.setAttribute("href", atom->string());
        break;

    case Atom::InlineImage:
        atomElement = document.createElement("inlineimage");
        atomElement.setAttribute("href", atom->string());
        break;

    case Atom::ImageText:
        break;
    case Atom::LegaleseLeft:
        atomElement = document.createElement("legalese");
        atom = atom->next();
        while (atom && atom->type() != Atom::LegaleseRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::LegaleseRight:
        break;

    case Atom::Link:
        atomElement = document.createElement("link");
        atomElement.setAttribute("href", atom->string());
        //inLink = true;
        break;

    case Atom::LinkNode:
        atomElement = document.createElement("link");
        atomElement.setAttribute("href", atom->string());
        //inLink = true;
        break;

    case Atom::ListLeft:
        atomElement = document.createElement("list");
        if (atom->string() == ATOM_LIST_BULLET)
            atomElement.setAttribute("type", "bullet");
        else if (atom->string() == ATOM_LIST_TAG)
            atomElement.setAttribute("type", "definition");
        else if (atom->string() == ATOM_LIST_VALUE)
            atomElement.setAttribute("type", "enum");
        else {
            atomElement.setAttribute("type", "ordered");
            if (atom->string() == ATOM_LIST_UPPERALPHA)
                atomElement.setAttribute("start", "A");
            else if (atom->string() == ATOM_LIST_LOWERALPHA)
                atomElement.setAttribute("start", "a");
            else if (atom->string() == ATOM_LIST_UPPERROMAN)
                atomElement.setAttribute("start", "I");
            else if (atom->string() == ATOM_LIST_LOWERROMAN)
                atomElement.setAttribute("start", "i");
            else // (atom->string() == ATOM_LIST_NUMERIC)
                atomElement.setAttribute("start", "1");
        }

        atom = atom->next();
        while (atom && atom->type() != Atom::ListRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::ListItemNumber:
        break;

    case Atom::ListTagLeft:
        {
            atomElement = document.createElement("definition");

            QDomElement termElement = document.createElement("term");
            QDomText termTextNode = document.createTextNode(plainCode(
                        marker->markedUpEnumValue(atom->next()->string(), relative)));
            termElement.appendChild(termTextNode);
            atomElement.appendChild(termElement);

            atom = atom->next();
            while (atom && atom->type() != Atom::ListTagRight)
                atom = addAtomElements(atomElement, atom, relative, marker);
        }
        break;

    case Atom::ListTagRight:
        break;

    case Atom::ListItemLeft:
        atomElement = document.createElement("item");
        atom = atom->next();
        while (atom && atom->type() != Atom::ListItemRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::ListItemRight:
        break;

    case Atom::ListRight:
        break;

    case Atom::Nop:
        break;

    case Atom::ParaLeft:
        atomElement = document.createElement("para");
        atom = atom->next();
        while (atom && atom->type() != Atom::ParaRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::ParaRight:
        break;

    case Atom::QuotationLeft:
        atomElement = document.createElement("quote");
        atom = atom->next();
        while (atom && atom->type() != Atom::QuotationRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::QuotationRight:
        break;

    case Atom::RawString:
        textNode = document.createTextNode(atom->string());
        break;

    case Atom::SectionLeft:
        atomElement = document.createElement("section");
        textNode = document.createTextNode(
                   Doc::canonicalTitle(Text::sectionHeading(atom).toString()));
        atom = atom->next();
        while (atom && atom->type() != Atom::SectionRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::SectionRight:
        break;

    case Atom::SectionHeadingLeft:
        atomElement = document.createElement("heading");
        atomElement.setAttribute("level", atom->string().toInt()); // + hOffset(relative)

        inSectionHeading = true;
        atom = atom->next();
        while (atom && atom->type() != Atom::SectionHeadingRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::SectionHeadingRight:
        inSectionHeading = false;
        break;

    case Atom::SidebarLeft:
    case Atom::SidebarRight:
        break;

    case Atom::String:
        textNode = document.createTextNode(atom->string());
        break;

    case Atom::TableLeft:
        atomElement = document.createElement("table");

        numTableRows = 0;
        atom = atom->next();
        while (atom && atom->type() != Atom::TableRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::TableRight:
        break;

    case Atom::TableHeaderLeft:
        atomElement = document.createElement("header");

        atom = atom->next();
        while (atom && atom->type() != Atom::TableHeaderRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::TableHeaderRight:
        break;

    case Atom::TableRowLeft:
        atomElement = document.createElement("row");

        atom = atom->next();
        while (atom && atom->type() != Atom::TableRowRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::TableRowRight:
        break;

    case Atom::TableItemLeft:
        atomElement = document.createElement("item");
        atom = atom->next();
        while (atom && atom->type() != Atom::TableItemRight)
            atom = addAtomElements(atomElement, atom, relative, marker);

        break;

    case Atom::TableItemRight:
        break;

    case Atom::TableOfContents:
        atomElement = document.createElement("tableofcontents");
/*        {
            int numColumns = 1;
            const Node *node = relative;

            Doc::SectioningUnit sectioningUnit = Doc::Section4;
            QStringList params = atom->string().split(",");
            QString columnText = params.at(0);
            QStringList pieces = columnText.split(" ", QString::SkipEmptyParts);
            if (pieces.size() >= 2) {
                columnText = pieces.at(0);
                pieces.pop_front();
                QString path = pieces.join(" ").trimmed();
                node = findNodeForTarget(path, relative, marker, atom);
            }

            if (params.size() == 2) {
                numColumns = qMax(columnText.toInt(), numColumns);
                sectioningUnit = (Doc::SectioningUnit)params.at(1).toInt();
            }

            if (node)
                generateTableOfContents(node, marker, sectioningUnit, numColumns,
                                        relative);
        }*/
        break;

    case Atom::Target:
        atomElement = document.createElement("target");
        atomElement.setAttribute("name", Doc::canonicalTitle(atom->string()));
        break;

    case Atom::UnhandledFormat:
    case Atom::UnknownCommand:
        textNode = document.createTextNode(atom->string());
        break;
    default:
        break;
    }

/*    if (atom)
        qDebug() << atom->typeString();*/
    if (!atomElement.isNull())
        parent.appendChild(atomElement);
    else if (!textNode.isNull())
        parent.appendChild(textNode);

    if (atom)
        return atom->next();
    
    return 0;
}
/*
        QDomElement atomElement = document.createElement(atom->typeString().toLower());
        QDomText atomValue = document.createTextNode(atom->string());
        atomElement.appendChild(atomValue);
        descriptionElement.appendChild(atomElement);
*/
