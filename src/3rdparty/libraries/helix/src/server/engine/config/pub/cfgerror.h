/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cfgerror.h,v 1.2 2003/01/23 23:42:53 damonlan Exp $ 
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

#ifndef CFG_ERROR_H_
#define CFG_ERROR_H_

#include <stdarg.h>


class Config_error
{
public:
    enum ErrorCodes
    {
	INTERNAL_ERROR 	= -1,
	USER_ERROR 	= -2,
	WARNING 	= -3,
	NO_ERRORS	= 0
    };
			Config_error();
			Config_error(int _code);
			Config_error(int _code, const char* _description);
			Config_error(const Config_error& rhs);
			~Config_error();

    Config_error&	operator=(const Config_error& rhs);
    Config_error&	operator<<(const char* str);
    Config_error&	operator<<(INT32 num);
    Config_error&	operator<<(UINT32 num);
    Config_error&	operator<<(char c);

    INT32		code();
    const char*		errorMsg();
    void		set_code(INT32 code);

    class Core
    {
	friend 		class Config_error;

			Core();
	void		addRef();
	void		delRef();

    private:
			~Core();
	char*		m_error_str;
	UINT32		m_error_str_len;
	char*		m_current_pos;
	UINT32		m_ref_count;
	UINT32		m_code;
    };

    static const UINT32	DEFAULT_ERROR_STR_LEN;
    static const UINT32 MAX_INT_AS_STRING_SIZE;

private:
    void		init();
    void		init_core();
    void		ensure_space(UINT32 space);
    UINT32		len();
    Core*		m_core;
};

inline UINT32
Config_error::len()
{
    if (m_core)
    {
    	return m_core->m_current_pos - m_core->m_error_str;
    }
    else
    {
	return 0;
    }
}

inline INT32
Config_error::code()
{
    if (m_core)
    {
    	return m_core->m_code;
    }
    else
    {
	return 0;
    }
}

inline const char*
Config_error::errorMsg()
{
    if (m_core)
    {
    	return m_core->m_error_str;
    }
    else
    {
	return 0;
    }
}

inline void
Config_error::set_code(INT32 code)
{
    init_core();
    m_core->m_code = code;
}

#endif //CFG_ERROR_H_

