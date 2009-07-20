/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mimehead.cpp,v 1.9 2007/07/06 20:51:32 jfinnecy Exp $
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

//#include "hlxclib/stdlib.h"
//#include "hlxclib/stdio.h"

#include "hxstring.h"
#include "hxslist.h"
#include "hxstrutl.h"
#include "hxtypes.h"
#include "mimehead.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


void
MIMEParameter::asString(CHXString& str)
{
    if(m_value.GetLength() > 0)
    {
    	str = m_attribute + "=" + m_value;
    }
    else
    {
	str = m_attribute;
    }
}

MIMEHeaderValue::MIMEHeaderValue()
{
}

MIMEHeaderValue::MIMEHeaderValue(const char* pAttribute)
{
    addParameter(pAttribute);
}

MIMEHeaderValue::MIMEHeaderValue(const char* pAttribute, const char* pValue)
{
    addParameter(pAttribute, pValue);
}

MIMEHeaderValue::MIMEHeaderValue(const MIMEHeaderValue& rhs)
{
    *this = rhs;
}

MIMEHeaderValue::~MIMEHeaderValue()
{
    clearParameterList();
}

MIMEHeaderValue& MIMEHeaderValue::operator= (const MIMEHeaderValue& rhs)
{
    if (&rhs != this)
    {
        clearParameterList();

        LISTPOSITION pos = rhs.m_parameters.GetHeadPosition();
        
        while(pos)
        {
            MIMEParameter* pParam =
                (MIMEParameter*)rhs.m_parameters.GetNext(pos);

            if (pParam)
            {
                MIMEParameter* pNewParam = new MIMEParameter(*pParam);
                
                if (pNewParam)
                {
                    addParameter(pNewParam);
                }
            }
        }
    }

    return *this;
}

void
MIMEHeaderValue::addParameter(const char* pAttribute, const char* pValue)
{
    MIMEParameter* pParam = new MIMEParameter;
    pParam->m_attribute = pAttribute;
    pParam->m_value = pValue;
    m_parameters.AddTail(pParam);
}

void
MIMEHeaderValue::addParameter(const char* pAttribute)
{
    addParameter(pAttribute, "");
}

void
MIMEHeaderValue::addParameter(MIMEParameter* pParam)
{
    m_parameters.AddTail(pParam);
}

MIMEParameter*
MIMEHeaderValue::getParameter(const char* pAttribute)
{
    LISTPOSITION pos = m_parameters.GetHeadPosition();
    while(pos)
    {
	MIMEParameter* pParam = (MIMEParameter*)m_parameters.GetNext(pos);
	if(strcasecmp(pParam->m_attribute, pAttribute) == 0)
	{
	    return pParam;
	}
    }
    return 0;
}

const char*
MIMEHeaderValue::getParameterValue(const char* pAttribute)
{
    MIMEParameter* pParam = getParameter(pAttribute);
    if(pParam)
	return pParam->m_attribute;
    return 0;
}

MIMEParameter*
MIMEHeaderValue::getFirstParameter()
{
    m_listpos = m_parameters.GetHeadPosition();
    if(m_listpos)
	return (MIMEParameter*)m_parameters.GetNext(m_listpos);
    return 0;
}

MIMEParameter*
MIMEHeaderValue::getNextParameter()
{
    if(m_listpos)
	return (MIMEParameter*)m_parameters.GetNext(m_listpos);
    return 0;
}

void
MIMEHeaderValue::clearParameterList()
{
    MIMEParameter* pParam = getFirstParameter();
    while(pParam)
    {
	delete pParam;
	pParam = getNextParameter();
    }
}

void
MIMEHeaderValue::asString(CHXString& str)
{
    HXBOOL bFirstParam = TRUE;
    MIMEParameter* pValueRep = getFirstParameter();
    while(pValueRep)
    {
	CHXString tmpStr;
	pValueRep->asString(tmpStr);
	if(bFirstParam)
	{
	    str = tmpStr;
	    bFirstParam = FALSE;
	}
	else
	{
	    str += ";" + tmpStr;
	}
	pValueRep = getNextParameter();
    }
}

CHXString
MIMEHeaderValue::value()
{
    CHXString str;
    asString(str);
    return str;
}

MIMEHeader::MIMEHeader(const char* pName):
    m_name(pName)
{
}

MIMEHeader::MIMEHeader(const MIMEHeader& rhs)
{
    *this = rhs;
}

MIMEHeader::~MIMEHeader()
{
    clearHeaderValueList();
}

MIMEHeader& MIMEHeader::operator=(const MIMEHeader& rhs)
{
    if (&rhs != this)
    {
        clearHeaderValueList();

        m_name = rhs.m_name;

        LISTPOSITION pos = rhs.m_headerValues.GetHeadPosition();

        while(pos)
        {
            MIMEHeaderValue* pHdrValue = 
                (MIMEHeaderValue*)rhs.m_headerValues.GetNext(pos);

            if (pHdrValue)
            {
                MIMEHeaderValue* pNewHdrValue = 
                    new MIMEHeaderValue(*pHdrValue);
                
                if (pNewHdrValue)
                {
                    addHeaderValue(pNewHdrValue);
                }
            }
        }
    }

    return *this;
}

void
MIMEHeader::addHeaderValue(MIMEHeaderValue* pHeaderValue)
{
    m_headerValues.AddTail(pHeaderValue);
}

void
MIMEHeader::addHeaderValue(const char* pValue)
{
    addHeaderValue(new MIMEHeaderValue(pValue));
} 

MIMEHeaderValue*
MIMEHeader::getHeaderValue(const char* pValue)
{
    LISTPOSITION pos = m_headerValues.GetHeadPosition();
    while(pos)
    {
	MIMEHeaderValue* pHeaderValue = 
	    (MIMEHeaderValue*)m_headerValues.GetNext(pos);
	if(strcasecmp(pHeaderValue->value(), pValue) == 0)
	{
	    return pHeaderValue;
	}
    }
    return 0;
}

MIMEHeaderValue*
MIMEHeader::getFirstHeaderValue()
{
    m_listpos = m_headerValues.GetHeadPosition();
    if(m_listpos)
	return (MIMEHeaderValue*)m_headerValues.GetNext(m_listpos);
    return 0;
}

MIMEHeaderValue*
MIMEHeader::getNextHeaderValue()
{
    if(m_listpos)
	return (MIMEHeaderValue*)m_headerValues.GetNext(m_listpos);
    return 0;
}

void
MIMEHeader::clearHeaderValueList()
{
    MIMEHeaderValue* pHeaderValue = getFirstHeaderValue();
    while(pHeaderValue)
    {
	delete pHeaderValue;
	pHeaderValue = getNextHeaderValue();
    }
}

void
MIMEHeader::asString(CHXString& msgStr)
{
    makeString(msgStr);
    msgStr += "\r\n";
}

void
MIMEHeader::makeString(CHXString& msgStr)
{
    MIMEHeaderValue* pHeaderValue = (MIMEHeaderValue*)getFirstHeaderValue();
    int firstValue = 1;
    while(pHeaderValue)
    {
	if(!firstValue)
	{
	    msgStr += ",";
	}
	else
	{
	    // msgStr += " ";
	    firstValue = 0;
	}
	CHXString tmpStr;
	pHeaderValue->asString(tmpStr);
	msgStr += tmpStr;
	pHeaderValue = (MIMEHeaderValue*)getNextHeaderValue();
    }
}
