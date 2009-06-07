/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: xmlregconfig.h,v 1.2 2003/01/23 23:42:53 damonlan Exp $ 
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
#ifndef _XMLREGCONFIG_H_
#define _XMLREGCONFIG_H_

#include "hxcfg.h"
#include "hxreg.h"
#include "xmlconfig.h"

class XMLServerConfig : public XMLConfig
{
public:
    XMLServerConfig():m_winregkey(NULL), m_proc(NULL), 
         m_pHXRegistry(NULL), m_lRefCount(0)
    {
    }

    XMLServerConfig(Process* proc, ServerRegistry* registry);
    ~XMLServerConfig();

    /*
     * Com stuff.
     */
    STDMETHOD(QueryInterface) (REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     * IHXRegConfig stuff.
     */
    STDMETHOD(WriteKey)	(THIS_
			const char* pKeyName);

    HX_RESULT Read(char* filename, char* pWinRegKey, 
		   char* pServRegKey);
    
//    HX_RESULT Reconfigure(IHXReconfigServerResponse* pResp);

private:
    INT32 m_lRefCount;
    char* m_winregkey;
    Process* m_proc;
    ServerRegistry* m_registry;
    HXRegistry* m_pHXRegistry;
    
};

#endif
