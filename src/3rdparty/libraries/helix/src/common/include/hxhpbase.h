/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxhpbase.h,v 1.9 2007/07/06 20:43:41 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _INC_HXHPBASE_
#define _INC_HXHPBASE_

    /*
    //////////////////////////////////////////////////////////////////
    // HX_THIS_FILE
    //
    // this macro must be redefined in EVERY source module where new,
    // malloc, etc., are called as the following:
    //
    //	#ifdef _DEBUG
    //	#undef HX_THIS_FILE		
    //	static const char HX_THIS_FILE[] = __FILE__;
    //	#endif
    //
    // making this definition enables tracing of memory allocations to
    // module/line where allocation was performed
    */

#ifndef HX_THIS_FILE
#   define HX_THIS_FILE			"no file defined"
#endif

    /*
    /////////////////////////////////////////////////////////////////
    // HX_SET_HEAP_OPTION, HX_CLEAR_HEAP_OPTION
    //
    // Use these macros to change heap checking options.  They are
    // given null definitions here for cross platform support.
    // Platforms supporting these options should #undef, then #define
    // the supported options and flags.
    //
    // Typically, options will be set only once at the program entry point
    //
    // To set/clear heap options, pass in combinations of the
    // HX_HEAP_* flags OR'd together as follows:
    //
    // HX_CLEAR_HEAP_OPTION(HX_HEAP_NO_HEAP_CHECKS)
    // HX_SET_HEAP_OPTION(HX_HEAP_CHECK_ON_EXIT | HX_HEAP_DELAY_FREE_MEM)
    //
    // note that these macros are intended to OR the given option w/
    // the current state of heap options
    //
    //#define  HX_SET_HEAP_OPTION(a)
    //#define  HX_CLEAR_HEAP_OPTION(a)
    */

    /*
    ///////////////////////////////////////////////////////////////////
    // HX_HEAP_*
    //
    // These definitions include common options for heap debugging.
    // See platform specific sections for implementation details.  They
    // are all defined as 0 here for cross platform support.  Platforms
    // supporting these options should #undef, then #define the flags
    // using appropriate values.
    //
    //#define HX_HEAP_ENABLE_HEAP_CHECKS	0
    //#define HX_HEAP_CHECK_EVERY_TIME		0
    //#define HX_HEAP_DELAY_FREE_MEM		0
    //#define HX_HEAP_LEAK_CHECK_ON_EXIT	0
    */

    /*
    ///////////////////////////////////////////////////////////////////
    // HX_CHECK_MEMORY()
    //
    // Calls the runtime function triggering a heap check
    // #define HX_CHECK_MEMORY()
    */

    /*
    ///////////////////////////////////////////////////////////////////
    // HX_DUMP_LEAKS()
    //
    // Calls the runtime function triggering a dump of all the objects 
    // still in heap. 
    // #define HX_DUMP_LEAKS()
    */


#   ifdef _DEBUG	/* DEBUG definitions */

#if !defined(_WIN32) || defined(WIN32_PLATFORM_PSPC)

#	    define HX_SET_HEAP_OPTION(a)
#	    define HX_CLEAR_HEAP_OPTION(a)
#	    define HX_HEAP_ENABLE_HEAP_CHECKS	0
#	    define HX_HEAP_CHECK_EVERY_TIME	0
#	    define HX_HEAP_DELAY_FREE_MEM		0
#	    define HX_HEAP_LEAK_CHECK_ON_EXIT	0
#	    define HX_CHECK_MEMORY()
#	    define HX_DUMP_LEAKS()
#	    define HX_SET_BREAK_ALLOC(a)

#	endif

	/*
	// ------------------------ _WIN32 -------------------------
	*/
#if defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC)
#	    include <crtdbg.h>

#	    define HX_DEBUG_NEW_PARAMS		\
		unsigned int size,		\
		int block,			\
		const char * szfile,		\
		int line			\

#	    define HX_DEBUG_NEW		\
		new(_NORMAL_BLOCK, HX_THIS_FILE, __LINE__)

#	    define HX_DEBUG_MALLOC(n)	\
		_malloc_dbg(n,_NORMAL_BLOCK,HX_THIS_FILE, __LINE__)

#	    define HX_DEBUG_CALLOC(n,s)	\
		_calloc_dbg(n,s,_NORMAL_BLOCK,HX_THIS_FILE, __LINE__)

#	    define HX_DEBUG_REALLOC(p,s)\
		_realloc_dbg(p,s,_NORMAL_BLOCK,HX_THIS_FILE, __LINE__)

#	    undef HX_CHECK_MEMORY
#	    define HX_CHECK_MEMORY()	    _ASSERT(_CrtCheckMemory())

#	    undef  HX_DUMP_LEAKS
#	    define HX_DUMP_LEAKS()	_CrtDumpMemoryLeaks()

#	    define HX_SET_BREAK_ALLOC(a)    _CrtSetBreakAlloc(a)

#	    define HX_SET_HEAP_OPTION(a) \
		_CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

#	    define HX_CLEAR_HEAP_OPTION(a) \
		_CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

	    /*
	    // HX_HEAP_ENABLE_HEAP_CHECKS
	    //	ON by default.  If this option is turned off, subsequent
	    //	memory allocations will not be subject to standard heap checks
	    */
#	    define HX_HEAP_ENABLE_HEAP_CHECKS	_CRTDBG_ALLOC_MEM_DF

	    /*
	    // HX_HEAP_CHECK_EVERY_TIME
	    //	OFF by default.  If this option is turned on, every allocation
	    //	and release of memory will cause the entire heap to be checked
	    //	for integrity
	    */
#	    define HX_HEAP_CHECK_EVERY_TIME	_CRTDBG_CHECK_ALWAYS_DF

	    /*
	    // HX_HEAP_DELAY_FREE_MEM
	    //	OFF by default.
	    //	Causes freed memory to be marked as free, instead of actually
	    //	freed.  The freed blocks are all written over w/ DD and 
	    //	maintained on the heap.  This has 2 benefits:
	    //	1)	Simulate low memory conditions
	    //	2)	Detection of corruption of previously freed blocks.  
	    //		This occurrs when a pointer to freed memory is used
	    //		w/o reinitializing it
	    */
#	    define HX_HEAP_DELAY_FREE_MEM	_CRTDBG_DELAY_FREE_MEM_DF

	    /*
	    // HX_HEAP_LEAK_CHECK_ON_EXIT
	    //	OFF by default.  When the CRT shuts down, the heap linked
	    //	list is examined, and any remaining blocks not marked as
	    //	free generate a leakage report indicating the module and
	    //	line where the block was allocated.
	    */
#	    define HX_HEAP_LEAK_CHECK_ON_EXIT	_CRTDBG_LEAK_CHECK_DF


	/* ------------------------ _WIN32 ------------------------- */
#	endif

	/* ---------------------- MACINTOSH ---------------------- */
#	ifdef _MACINTOSH	

#	    define HX_DEBUG_NEW new

	/* ---------------------- MACINTOSH ---------------------- */
#	endif

#   else	/* end debug section */

#	define HX_SET_HEAP_OPTION(a)
#	define HX_CLEAR_HEAP_OPTION(a)
#	define HX_HEAP_ENABLE_HEAP_CHECKS	0
#	define HX_HEAP_CHECK_EVERY_TIME	0
#	define HX_HEAP_DELAY_FREE_MEM		0
#	define HX_HEAP_LEAK_CHECK_ON_EXIT	0
#	define HX_CHECK_MEMORY()
#	define HX_DUMP_LEAKS()

#   endif	/* end retail section */

#endif	/* _INC_HXHPBASE_ */
