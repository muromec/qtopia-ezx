/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: thrdutil.cpp,v 1.2 2006/10/20 20:52:33 gwright Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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
#include "hxresult.h"
#include "hxassert.h"
#include "thrdutil.h"

//=================================================================
// In this section here include the OS specific headerfiles for
// each implementation.
//=================================================================
#if defined (_WIN32) || defined (_WINDOWS)
#include "hlxclib/windows.h"
#endif

#if defined( _LINUX ) || defined(_HPUX) || defined(_MAC_UNIX)
#include <pthread.h>
#include <semaphore.h>
#include "pthreadthreads.h"
#endif

#if defined( _SOLARIS )
#include <synch.h>
#include <thread.h>
#include "solaristhreads.h"
#endif

#if defined( _SYMBIAN )
#include <e32std.h>
#include <e32base.h>
#include "symbian_async_timer_imp.h"
#include "symbian_rtimer_at_imp.h"
#include "symbian_thread_at_imp.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

UINT32 HXGetCurrentThreadID()
{
#ifdef _WIN32
    return ::GetCurrentThreadId();
#elif defined (_MACINTOSH)
    #ifdef THREADS_SUPPORTED
	MPTaskID id = ::MPCurrentTaskID();
	return (UINT32)id;
    #else
	// HXMacThread is NOT implemented!!
	HX_ASSERT(FALSE);
	return 1;
	//return HXMacThread::GetCurrentThreadID();
    #endif
#elif defined( _UNIX_THREADS_SUPPORTED )
    #if defined( _LINUX ) || defined (_HPUX) || defined(_MAC_UNIX)
	return (UINT32) pthread_self();
    #elif defined( _SOLARIS )
	return thr_self();
    #else
	HX_ASSERT( "No unix thread for this platform" == NULL );
    #endif    
#elif defined _SYMBIAN
    return RThread().Id();
#else
    return 1;
#endif
}
