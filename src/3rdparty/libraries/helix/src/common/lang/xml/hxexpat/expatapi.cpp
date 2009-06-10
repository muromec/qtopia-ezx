/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: expatapi.cpp,v 1.3 2007/07/06 20:43:46 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */
//  $Id: expatapi.cpp,v 1.3 2007/07/06 20:43:46 jfinnecy Exp $

#include "hxtypes.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxxml.h"
#include "hxplugn.h"

#include "xmlparse.h"
#include "expatapi.h"
#include "expatprs.h"

void handleStartElement(void* parser, const XML_Char *name, const XML_Char **atts)
{
    ((HXExpatXMLParser*)parser)->handleStartElement(name, atts);
}

void handleEndElement(void* parser, const XML_Char *name)
{
    ((HXExpatXMLParser*)parser)->handleEndElement(name);
}

void handleCharacterData(void* parser, const XML_Char *s, int len)
{
    ((HXExpatXMLParser*)parser)->handleCharacterData(s, len);
}

void handleProcessingInstruction(void* parser, const XML_Char *target, const XML_Char *data)
{
    ((HXExpatXMLParser*)parser)->handleProcessingInstruction(target, data);
}

void handleComment(void* parser, const XML_Char *data)
{
    ((HXExpatXMLParser*)parser)->handleComment(data);
}

void handleStartCdataSection(void* parser)
{
    ((HXExpatXMLParser*)parser)->handleStartCDataSection();
}

void handleEndCdataSection(void* parser)
{
    ((HXExpatXMLParser*)parser)->handleEndCDataSection();
}

void handleDefault(void* parser, const XML_Char *s, int len)
{
    ((HXExpatXMLParser*)parser)->handleDefault(s, len);
}

void handleUnparsedEntityDecl(void* parser,
			    const XML_Char *entityName,
			    const XML_Char *base,
			    const XML_Char *systemId,
			    const XML_Char *publicId,
			    const XML_Char *notationName)
{
    ((HXExpatXMLParser*)parser)->handleUnparsedEntityDecl(entityName, base, systemId, publicId, notationName);
}

void handleNotationDecl(void* parser,
		      const XML_Char *notationName,
		      const XML_Char *base,
		      const XML_Char *systemId,
		      const XML_Char *publicId)
{
    ((HXExpatXMLParser*)parser)->handleNotationDecl(notationName, base, systemId, publicId);
}

void handleStartNamespaceDecl(void *parser, 
			      const XML_Char *prefix,
			      const XML_Char *uri)
{
    ((HXExpatXMLParser*)parser)->handleStartNamespaceDecl(prefix, uri);
}

void handleEndNamespaceDecl(void *parser, const XML_Char *prefix)
{
    ((HXExpatXMLParser*)parser)->handleEndNamespaceDecl(prefix);
}

int handleNotStandaloneHandler(void *parser)
{
    return ((HXExpatXMLParser*)parser)->handleNotStandalone();
}

void handleStartDoctypeDecl(void* parser, const XML_Char *doctypeName)
{
    ((HXExpatXMLParser*)parser)->handleStartDoctypeDecl(doctypeName);
}

void handleEndDoctypeDecl(XML_Parser parser)
{
    ((HXExpatXMLParser*)parser)->handleEndDoctypeDecl();
}
