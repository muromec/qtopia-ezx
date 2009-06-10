/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: misc_proc.h,v 1.6 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _MISC_PROC_H_
#define _MISC_PROC_H_

#include "simple_callback.h"

class MiscProcInitPlug;
class LBLAcceptor;

class MiscProcessInitCallback : public SimpleCallback
{
public:
    enum Errors
    {
	NO_ERRORS,
	WRONG_INTERFACE,
	UNABLE_TO_SETUP,
	BAD_PLUGIN
    };
    void			func(Process* proc);
    Process*			m_proc;			// Main's Process Class
    PluginHandler::Plugin*	m_plugin;
    LBLAcceptor*		m_pAcceptor;
private:
				~MiscProcessInitCallback() {};
    Errors			AttemptFileSystemStart(Process* proc,
						       IUnknown* instance);
    Errors			AttemptBroadcastStart(Process* proc,
						      IUnknown* instance);

    Errors			AttemptAllowanceStart(Process* proc,
						      IUnknown* instance);
    Errors			AttemptPTAgentStart(Process* proc, 
                                                    IUnknown* instance);
            
    friend class MiscProcInitPlug;
};

class MiscProcInitPlug: public BaseCallback
{
public:
    MiscProcInitPlug(Process* pProc);

    STDMETHOD(Func)     (THIS);

    IUnknown*                           instance;
    IHXPlugin*                         plugin_interface;
    MiscProcessInitCallback*		pMPI;
    LBLAcceptor*			m_pAcceptor;

    Process* m_pProc;
private:

};

#endif /* _MISC_PROC_H_ */
