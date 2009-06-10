/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: misc_plugin.h,v 1.5 2003/06/04 18:40:47 jgordon Exp $ 
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

#ifndef _MISC_PLUGIN_H_
#define _MISC_PLUGIN_H_

struct IHXFileSystemObject;
struct IHXPlayerConnectionAdviseSinkManager;
class BroadcastFactoryWrapper;
struct IHXPSSPTAgent;

class MiscPlugin
{
public:
    Process*	   m_pProc;
};

class FSPlugin : public MiscPlugin
{
public:
    IHXFileSystemObject*   	m_pFS;
};

class BroadcastPlugin : public FSPlugin
{
public:
//    BroadcastFactoryWrapper*	m_pBCastWrap;
};

class AllowancePlugin : public MiscPlugin
{
public:
    IHXPlayerConnectionAdviseSinkManager*   	m_pAdviseSinkManager;
};

class PTAgentPlugin : public MiscPlugin
{
public:
    IHXPSSPTAgent*  m_pPTAgent;
};
#endif /* _MISC_PLUGIN_H_ */
