/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mimehead.h,v 1.6 2007/07/06 20:51:33 jfinnecy Exp $
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

/*
 *
 * MIME header and helper classes 
 *
 */

#ifndef _MIMEHEAD_H_
#define _MIMEHEAD_H_

class CHXString;
class CHXSimpleList;

/*
 * Contains a single name/value pair
 */
class MIMEParameter
{
public:
    CHXString m_attribute;
    CHXString m_value;

    HXBOOL null() { return (m_value.GetLength() == 0); }
    void asString(CHXString& str);
};

/*
 * Contains a list of parameters for a single header value.
 * Syntax for a header value is:
 *
 *	parameter["=" value] *[;parameter["=" value]]
 */
class MIMEHeaderValue
{
public:
    MIMEHeaderValue();
    MIMEHeaderValue(const char* pAttribute);
    MIMEHeaderValue(const char* pAttribute, const char* pValue);
    MIMEHeaderValue(const MIMEHeaderValue& rhs);

    virtual ~MIMEHeaderValue();

    MIMEHeaderValue& operator= (const MIMEHeaderValue& rhs);

    void addParameter(const char* pAttribute, const char* pValue);
    void addParameter(const char* pAttribute);
    void addParameter(MIMEParameter* pParam);
    MIMEParameter* getParameter(const char* pAttribute);
    const char* getParameterValue(const char* pAttribute);
    MIMEParameter* getFirstParameter();
    MIMEParameter* getNextParameter();
    virtual void asString(CHXString& str);
    CHXString value();

private:
    void clearParameterList();

    CHXSimpleList m_parameters;
    LISTPOSITION m_listpos;
};

/*
 * Contains a name and a list of header values.
 * Syntax for a header is:
 *
 *	header-name ":" header-value *["," [header-value]]
 *
 */
class MIMEHeader
{
public:
    MIMEHeader(const char* pName);
    MIMEHeader(const MIMEHeader& rhs);

    virtual ~MIMEHeader();
    
    MIMEHeader& operator=(const MIMEHeader& rhs);

    const char* name() { return m_name; }
    void addHeaderValue(MIMEHeaderValue* pHeaderValue);
    void addHeaderValue(const char* pValue);
    MIMEHeaderValue* getHeaderValue(const char* pName);
    MIMEHeaderValue* getFirstHeaderValue();
    MIMEHeaderValue* getNextHeaderValue();

    virtual void asString(CHXString& msgStr);
    void makeString(CHXString& msgStr);
protected:
    void clearHeaderValueList();

    CHXString m_name;
    CHXSimpleList m_headerValues;
    LISTPOSITION m_listpos;
};

#endif /* _MIMEHEAD_H_ */
