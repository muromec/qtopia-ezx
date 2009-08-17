/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bcngtypes.h,v 1.3 2004/03/08 20:07:57 tmarshall Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#ifndef _BCNGTYPES_H_
#define _BCNGTYPES_H_

#include "hlxclib/string.h"

#ifndef	PMC_PREDEFINED_TYPES
#define	PMC_PREDEFINED_TYPES

typedef char*	pmc_string;

struct buffer {
	 UINT32	len;
	 INT8*	data;
};
#endif/*PMC_PREDEFINED_TYPES*/

class ProtocolType
{
public:
    enum __PType { UDP_UNICAST, UDP_MCAST, TCP };
};

class BroadcastSecurity
{
public:
    enum __SecType { NONE, BASIC, PROXYPULL, RBS };
    BroadcastSecurity()
	: m_SecurityType(NONE)
	, m_pPassword(NULL)
    {
    }
    BroadcastSecurity(BroadcastSecurity& Security)
	: m_SecurityType(Security.m_SecurityType)
    {
	m_pPassword = Security.m_pPassword ?
	    new_string(Security.m_pPassword) : NULL;
    }
    ~BroadcastSecurity()
    {
	HX_VECTOR_DELETE(m_pPassword);
    }
    inline int operator =(BroadcastSecurity& Security)
    {
	m_SecurityType = Security.m_SecurityType;
	m_pPassword = Security.m_pPassword ?
	    new_string(Security.m_pPassword) : NULL;
	return 1;
    }
    __SecType		m_SecurityType;
    char*		m_pPassword;
};

#endif /* _BCNGTTYPES_H_ */
