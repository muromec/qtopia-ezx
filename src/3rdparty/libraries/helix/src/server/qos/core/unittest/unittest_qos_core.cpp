/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: unittest_qos_core.cpp,v 1.6 2003/04/08 21:42:08 damonlan Exp $ 
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
#define INITGUID
#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxqossig.h"
#include "hxqos.h"
#include "hxccf.h"

#include "mutex.h"
#include "chxmapstringtoob.h"
#include "servlist.h"

#include "qos_sig_bus_ctl.h"
#include "qos_sig_bus.h"
#include "qos_test_sink.h"
#include "qos_test_engine.h"

UINT32*	g_pConcurrentOps = new UINT32;
UINT32*	g_pConcurrentMemOps = new UINT32;
HX_MUTEX  g_pServerMainLock;

#ifdef _UNIX
extern char** environ;
#endif

int
main(int argc, char* argv[])
{
    char** ep = environ;
    
    QoSTestEngine* pEngine = new QoSTestEngine();

    /* Verify the Controller */
     pEngine->TestController();

    /* Verify Signal Bus*/
    pEngine->TestSignalBus();

    /* Verify Profile Selection */
    pEngine->TestProfileManager();

    /* Cleanup */
    delete pEngine;
    pEngine = NULL;

    delete g_pConcurrentMemOps; 
    g_pConcurrentMemOps = NULL;

    delete g_pConcurrentOps;
    g_pConcurrentOps = NULL;
    
    exit(0);
    return 0;
}
