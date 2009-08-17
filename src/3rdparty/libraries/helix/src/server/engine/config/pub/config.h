/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: config.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _NEWCONFIG_H_
#define _NEWCONFIG_H_

#include <stdio.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxtime.h"
#include "hxslist.h"
#include "hxmap.h"
#include "servbuffer.h"

class Config;
class ConfigVariable;
class License;
class Server;
class Config_error;
class Engine;
class ConfigFile;
class ServerRegistry;
class Process;
class StructData;

const char* const CFGMSG_INVALID_ERROR_LOG_PATH
    = "could not open ErrorLogPath: ";
const char* const CFGMESG_INVALID_LICENSE
    = "The given license key is not valid";

typedef struct _HXPropList HXPropList;

#define MAXLINELEN 1024

#define CONFIG_BACKLOG 10
const UINT32 LIC_MAGIC_NUMBER = 0x04df12ef;
const UINT32 DT_LIC_MAGIC_NUMBER = 0x72fbc526;

const int CONSTANT_NEEDED_FDS = 6;
const int MAX_FDS_PER_PLAYER = 4;

enum CfgType
{
    CFGV_INT,
    CFGV_BOOL,
    CFGV_STRING,
    CFGV_PATH,
    CFGV_LIST,
    CFGV_DICT,
    CFGV_SPECIAL,
    CFGV_STRUCT,
    CFGV_NOTYPE
};

struct UserInfo 
{
    char*   id;
    char*   base_path;
    int     min_streams;
    int     max_streams;
    int     current_streams;
};

struct ConnectControl { 
    u_long32	addr;
    char*	addr_string;
    u_long32	mask;
    char*	mask_string;
    
    int     addr_check(char*);
};

class Config 
{
public:
			    	Config(Process* proc, char* file, 
				       char* key_name, ServerRegistry* registry);
			    	~Config();
    
    BOOL                        valid() { return m_valid;};
    /* accessor public functions */
    INT32			GetInt(Process* proc,
				       const char* name) const;
    const char*			GetString(Process* proc,
				          const char* name) const;
    HX_RESULT			GetComposite(Process* proc, const char* name,
					     IHXValues*& pValues) const;

private:
    friend class License;

    void			SetDefaults(Process* proc);
    Config_error   		load(Process* proc, char* file, char* key_name);
    ServerRegistry*		m_registry;
    BOOL                        m_valid;
};

#endif
