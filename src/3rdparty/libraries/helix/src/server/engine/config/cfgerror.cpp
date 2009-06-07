/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cfgerror.cpp,v 1.2 2003/01/23 23:42:52 damonlan Exp $ 
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hxtypes.h"

#include "cfgerror.h"

const UINT32 Config_error::DEFAULT_ERROR_STR_LEN = 256;
const UINT32 Config_error::MAX_INT_AS_STRING_SIZE = 20;

Config_error::Core::Core()
{
    m_error_str = new char[Config_error::DEFAULT_ERROR_STR_LEN];
    m_error_str[0] = '\0';
    m_error_str_len = Config_error::DEFAULT_ERROR_STR_LEN;
    m_current_pos = m_error_str;
    m_ref_count = 0;
    m_code = 0;
}

Config_error::Core::~Core()
{
    delete[] m_error_str;
}

void
Config_error::Core::addRef()
{
    ++m_ref_count;
}

void
Config_error::Core::delRef()
{
    --m_ref_count;
    if (m_ref_count == 0)
    {
	delete this;
    }
}

void
Config_error::init()
{
    m_core = 0;
}

void
Config_error::init_core()
{
    if (m_core != 0)
    {
	return;
    }
    m_core = new Config_error::Core();
    m_core->addRef();
}

Config_error::Config_error()
{
    init();
}

Config_error::Config_error(int _code)
{
    init();
    init_core();
    m_core->m_code = _code;
}

Config_error::Config_error(int code, const char* description)
{
    init();
    init_core();
    m_core->m_code = code;
    (*this) << description;
}

Config_error::Config_error(const Config_error& rhs)
{
    init();
    (*this) = rhs;
}

Config_error::~Config_error()
{
    if (m_core)
    {
    	m_core->delRef();
    }
}

Config_error& 
Config_error::operator=(const Config_error& rhs)
{
    if (this == &rhs)
    {
	return *this;
    }

    if (m_core)
    {
    	m_core->delRef();
    }
    m_core = rhs.m_core;
    m_core->addRef();
    
    return *this;
}

Config_error&
Config_error::operator<<(const char* str)
{
    int str_len = strlen(str);

    ensure_space(str_len+1);
    strcpy(m_core->m_current_pos, str);
    m_core->m_current_pos += str_len;

    return *this;
}

Config_error&
Config_error::operator<<(INT32 n)
{
    ensure_space(Config_error::MAX_INT_AS_STRING_SIZE);
    m_core->m_current_pos += sprintf(m_core->m_current_pos, "%ld", n);

    return *this;
}

Config_error&
Config_error::operator<<(UINT32 num)
{
    ensure_space(Config_error::MAX_INT_AS_STRING_SIZE);
    m_core->m_current_pos += sprintf(m_core->m_current_pos, "%ld", num);

    return *this;
}

Config_error&
Config_error::operator<<(char c)
{
    ensure_space(1);
    *(m_core->m_current_pos) = c;
    ++m_core->m_current_pos;

    return *this;
}

/*
 * This trys to grow by DEFAULT_ERROR_STR_LEN, but will grow by space
 * if space is bigger than DEFAULT_ERROR_STR_LEN
 */

void
Config_error::ensure_space(UINT32 space)
{
    char* 	temp_str = 0;
    UINT32	new_len;

    init_core();

    if (len() + space > m_core->m_error_str_len)
    {
	if (len() + space > 
	    m_core->m_error_str_len + Config_error::DEFAULT_ERROR_STR_LEN)
	{
	    new_len = len() + space;
	}
	else
	{
	    new_len = m_core->m_error_str_len + 
		      Config_error::DEFAULT_ERROR_STR_LEN;
	}
	temp_str = new char[new_len];
	strcpy(temp_str, m_core->m_error_str);
	delete[] m_core->m_error_str;
	m_core->m_error_str = temp_str;
	m_core->m_error_str_len = new_len;
    }
}
