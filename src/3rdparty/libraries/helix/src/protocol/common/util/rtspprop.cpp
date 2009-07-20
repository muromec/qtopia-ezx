/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspprop.cpp,v 1.3 2004/07/09 18:41:59 hubbe Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

//#include "hlxclib/stdlib.h"
//#include "hlxclib/stdio.h"

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "rtsputil.h"
#include "rtspprop.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


/*
 * RTSPProperty methods
 */

RTSPProperty::RTSPProperty(const char* pName, INT32 propType):
    m_propType(propType)
{
    m_pName = new char[strlen(pName)+1];
    strcpy(m_pName, pName); /* Flawfinder: ignore */
}

RTSPProperty::~RTSPProperty()
{
    delete[] m_pName;
}

const char*
RTSPProperty::getName()
{
    return m_pName;
}

INT32
RTSPProperty::getType()
{
    return m_propType;
}

RTSPIntegerProperty::RTSPIntegerProperty(const char* pName,
    INT32 value): 
    RTSPProperty(pName, PROP_INTEGER),
    m_value(value)
{
}

RTSPIntegerProperty::~RTSPIntegerProperty()
{
}

INT32
RTSPIntegerProperty::getValue()
{
    return m_value;
}
    
RTSPBufferProperty::RTSPBufferProperty(const char* pName,
    IHXBuffer* pBuffer):
    RTSPProperty(pName, PROP_BUFFER)
{
    pBuffer->AddRef();
    m_pValue = pBuffer;
}


RTSPBufferProperty::~RTSPBufferProperty()
{
    if(m_pValue)
    {
	m_pValue->Release();
    }
}

IHXBuffer*
RTSPBufferProperty::getValue()
{
    return m_pValue;
}

RTSPStringProperty::RTSPStringProperty(const char* pName,
    const char* pValue):
    RTSPProperty(pName, PROP_STRING),
    m_value(pValue)
{
}

RTSPStringProperty::~RTSPStringProperty()
{
}

const char*
RTSPStringProperty::getValue()
{
    return m_value;
}

RTSPPropertyList::RTSPPropertyList()
{
    m_pPropertyList = new CHXSimpleList;
}

RTSPPropertyList::~RTSPPropertyList()
{
    RTSPProperty* pProp = getFirstProperty();
    while(pProp)
    {
	delete pProp;
	pProp = getNextProperty();
    }
    m_pPropertyList->RemoveAll();
    delete m_pPropertyList;
}

void
RTSPPropertyList::addProperty(RTSPProperty* pProp)
{
    m_pPropertyList->AddTail(pProp);
}

RTSPProperty*
RTSPPropertyList::getProperty(const char* pName)
{
    CHXSimpleList::Iterator i;
    for(i=m_pPropertyList->Begin(); i!=m_pPropertyList->End(); ++i)
    {
	RTSPProperty* pProp = (RTSPProperty*)(*i);
	if(strcasecmp(pProp->getName(), pName) == 0)
	{
	    return pProp;
	}
    }
    return 0;
}

RTSPProperty*
RTSPPropertyList::getFirstProperty()
{
    m_listPos = m_pPropertyList->GetHeadPosition();
    if(m_listPos)
    {
	RTSPProperty* pProp = 
	    (RTSPProperty*)m_pPropertyList->GetNext(m_listPos);
	return pProp;
    }
    return 0;
}

RTSPProperty*
RTSPPropertyList::getNextProperty()
{
    RTSPProperty* pProp = 0;
    if(m_listPos)
    {
	pProp = (RTSPProperty*)m_pPropertyList->GetNext(m_listPos);
    }
    return pProp;
}
