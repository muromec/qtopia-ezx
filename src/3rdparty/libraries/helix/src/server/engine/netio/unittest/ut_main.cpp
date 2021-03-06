/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_main.cpp,v 1.2 2006/10/19 20:08:04 tknox Exp $
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#include "hxtypes.h"
#include "hxassert.h"

//#define UT_DEFINE_GLOBALS
#include "ut_globals.h"

#include "hxmutexlock.h"

HX_MUTEX g_pServerMainLock;
UINT32 *g_pConcurrentOps;
UINT32 *g_pConcurrentMemOps;

#ifdef _WIN32
#include "windows.h"
#endif /* _WIN32 */

#include "cppunit/extensions/TestFactoryRegistry.h"
#include "cppunit/ui/text/TestRunner.h"

#include "leak_check.h"

int main( int argc, char **argv)
{
    HX_ADD_LEAK_CHECK;

    g_pConcurrentOps = new UINT32;
    g_pConcurrentMemOps = new UINT32;
    g_pServerMainLock = HXCreateMutex();

    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );

    delete g_pConcurrentOps;
    delete g_pConcurrentMemOps;
    HXDestroyMutex (g_pServerMainLock);
    
    return wasSuccessful;
}
