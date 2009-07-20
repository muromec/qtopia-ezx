/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: loadinfo.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _LOADINFO_H_
#define _LOADINFO_H_

#include "proc.h"
#include "base_callback.h"

// This will temporarily disable the loadstate messages until
// they are better callibrated for 9.0.
#define DISABLE_LOADSTATE_MESSAGES

enum _ServerState { NormalLoad, HighLoad, ExtremeLoad };

class LoadInfo: public BaseCallback
{
public:
    LoadInfo(Process* pProc);

    STDMETHOD(Func)	(THIS);

    enum _ServerState	GetLoadState() { return m_LoadState; }
    void		StreamerForceSelect() { m_ulStreamerForceSelect++; }
    void		StreamerIteration() { m_ulStreamerIterations++; }
    void		Overload() { m_ulOverload++; }
    BOOL		NAKImplosion() { return m_bNAKImplosionState; }

private:
    Process*	    m_pProc;
    _ServerState    m_LoadState;
    UINT32	    m_ulStreamerForceSelect;
    UINT32	    m_ulStreamerIterations;
    UINT32	    m_ulFuncCounter;
    UINT32	    m_ulOverload;
    Timeval	    m_tLastForce;
    Timeval	    m_tLastOverload;
    UINT32	    m_ulExtremeStartedSeconds;
    BOOL	    m_bNAKImplosionState;
    UINT32	    m_ulNumberOfExtremeLoads;
    BOOL	    m_bReportedExtremeLoad;

};

#endif /* _LOADINFO_H_ */
