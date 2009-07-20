/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcrt.cpp,v 1.10 2007/07/06 20:44:02 jfinnecy Exp $
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

#include <stdio.h>
#include <string.h> // for memset

#include "hxtypes.h"

#ifndef _MAC_MACHO

void*	__nwa(size_t size, char* file, int line);
//void*	__nw(size_t size, char* file, int line);

extern "C" void*	__nwa__FUlPcl(size_t size, char*, int);
//extern "C" void*	__nw__FUlPcl(size_t size, char* file, int line);

//extern "C" void*	operator new(unsigned long, char*, long);

#endif


extern "C" short HXCRT_ASSERT(char* message);

extern "C" {

void*	HXMM_NEWDEBUG(unsigned long size, char* file, unsigned long line);
void*	HXMM_NEW(unsigned long size);
void HXMM_DELETE(void* ptr);
void HXMM_INTERRUPTON(void);
void HXMM_INTERRUPTOFF(void);
HXBOOL HXMM_ATINTERRUPT(void);
void HXMM_COMPACT(void);

#ifndef _MAC_MACHO

void* malloc(size_t size);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);


char *__ttyname(long);

#endif

}

extern "C" short InstallConsole(short);
extern "C" void RemoveConsole(void);
extern "C" long WriteCharsToConsole(char*, long);
extern "C" long ReadCharsFromConsole(char*, long);

#ifndef _MAC_MACHO

void* operator new(unsigned long size, char*, long);
void* operator new(unsigned long size, char*, long)
{
return operator new(size);
}

//void*	operator new[](size_t size, char* file, int line)
void*	__nwa(size_t size, char* file, int line)
{
	void* retval = HXMM_NEWDEBUG(size, file, line);
	return retval;
}

void*	__nw(size_t size, char* file, int line);
void*	__nw(size_t size, char* file, int line)
{
	void* retval = HXMM_NEWDEBUG(size, file, line);
	return retval;
}

void*	__nwa(size_t size)
{
	void* retval = HXMM_NEW(size);
	return retval;
}

void*	__nw(size_t size)
{
	void* retval = HXMM_NEW(size);
	return retval;
}

void __dl(void*	ptr)
{
	HXMM_DELETE(ptr);
}


void __dla(void* ptr)
{
	HXMM_DELETE(ptr);
}


void*	__nwa__FUlPcl(size_t size, char*, int)
{
	void* retval = HXMM_NEW(size);
	return retval;
}

#endif

#pragma mark -

CFBundleRef systemBundle = NULL;
typedef void* (*MallocProcPtr) (size_t size);
typedef void* (*ReallocProcPtr) (void* oldPtr, size_t size);
typedef void* (*CallocProcPtr) (size_t nmemb, size_t size);
typedef void* (*FreeProcPtr) (void* ptr);

MallocProcPtr fpMalloc = NULL;
ReallocProcPtr fpRealloc = NULL;
CallocProcPtr fpCalloc = NULL;
FreeProcPtr fpFree = NULL;


static void LoadBundleIfNecessary()
{
	if (!systemBundle)
	{
		systemBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.Carbon"));
		
		fpMalloc = (MallocProcPtr) CFBundleGetFunctionPointerForName(systemBundle, CFSTR("malloc"));
		fpRealloc = (ReallocProcPtr) CFBundleGetFunctionPointerForName(systemBundle, CFSTR("realloc"));
		fpCalloc = (CallocProcPtr) CFBundleGetFunctionPointerForName(systemBundle, CFSTR("calloc"));
		fpFree = (FreeProcPtr) CFBundleGetFunctionPointerForName(systemBundle, CFSTR("free"));
	}
}

#pragma mark -

#define USE_NEW_POINTER_FOR_MALLOC 1

#ifndef _MAC_MACHO

void* malloc(size_t size)
{
	LoadBundleIfNecessary();
	if (fpMalloc)
	{
		return fpMalloc(size);
	}
	HXCRT_ASSERT("malloc function pointer not found");
	return NULL;
}

void* calloc(size_t nmemb, size_t size)
{
	if (fpCalloc)
	{
		return fpCalloc(nmemb, size);
	}
	HXCRT_ASSERT("calloc function pointer not found");
	return NULL;
}

void* realloc(void* ptr, size_t size)
{
	if (fpRealloc)
	{
		return fpRealloc(ptr, size);
	}
	HXCRT_ASSERT("realloc function pointer not found");
	return NULL;
}

void free(void* ptr)
{
	if (fpFree)
	{
		fpFree(ptr);
		return;
	}
	HXCRT_ASSERT("free function pointer not found");
	return;
}

#endif


#pragma mark -


short HXCRT_ASSERT(char* message)
{
	printf("ASSERTION: %s\n", message);
	return 0;
}


// Console stubs so stio.h stuff works

#ifndef _MAC_MACHO

short InstallConsole(short fd)
{
#pragma unused (fd)

	return 0;
}

/*
 *	extern void RemoveConsole(void);
 *
 *	Removes the console package.  It is called after all other streams
 *	are closed and exit functions (installed by either atexit or _atexit)
 *	have been called.  Since there is no way to recover from an error,
 *	this function doesn't need to return any.
 */

void RemoveConsole(void)
{
}

/*
 *	extern long WriteCharsToConsole(char *buffer, long n);
 *
 *	Writes a stream of output to the Console window.  This function is
 *	called by write.
 *
 *	char *buffer:	Pointer to the buffer to be written.
 *	long n:			The length of the buffer to be written.
 *	returns short:	Actual number of characters written to the stream,
 *					-1 if an error occurred.
 */

long WriteCharsToConsole(char *buffer, long n)
{
    while (n > 0)
    {
    
    	// try to break the string on carriage returns
    	// (some consoles honor the \r causing lines overwriting themselves)
		int lineLen = n;
		bool skipReturn = false;
    	for(int i=0;i<n;i++)
    	{
    		if (buffer[i] == '\r')
    		{
    			// found a CR -- print up to this point only 
    			lineLen = i;
    			skipReturn = true; // otherwise some consoles will print two CR's for a line
    			break;
    		}
    	}
    	
    	// and for sure break the line into blocks <= 200 bytes.
		Str255 a;
		long len = lineLen;
		if (len > 200) len = 200;

		a[0] = len;
		BlockMoveData(buffer, &a[1], len);

		DebugStr(a);

		if (skipReturn)
			len++;
					
		n -= len;
		buffer += len;
    }

	return 0;
}

/*
 *	extern long ReadCharsFromConsole(char *buffer, long n);
 *
 *	Reads from the Console into a buffer.  This function is called by
 *	read.
 *
 *	char *buffer:	Pointer to the buffer which will recieve the input.
 *	long n:			The maximum amount of characters to be read (size of
 *					buffer).
 *	returns short:	Actual number of characters read from the stream,
 *					-1 if an error occurred.
 */

long ReadCharsFromConsole(char *buffer, long n)
{
#pragma unused (buffer, n)

	return 0;
}

char *__ttyname(long)
{
	return (NULL);
}

#endif

void*	HXMM_NEW(unsigned long	size)
{
	LoadBundleIfNecessary();
	
	if (fpMalloc)
	{
		return fpMalloc(size);
	}
	HXCRT_ASSERT("HXMM_NEW: malloc function pointer not found");
	return NULL;
}


void*	HXMM_NEWDEBUG(unsigned long size, char* file, unsigned long line)
{
        return HXMM_NEW(size);
}


void	HXMM_DELETE(void* ptr)
{
	if (fpFree)
	{
		fpFree(ptr);
		return;
	}
	HXCRT_ASSERT("HXMM_DELETE: free function pointer not found");
}


#define kMaxInterruptEquivalentTaskIDs 20
MPTaskID gInterruptEquivalentTaskIDs[kMaxInterruptEquivalentTaskIDs];

long gCurrentInterruptEquivalentTaskID = 0;


void	HXMM_INTERRUPTON(void)
{
	long whichTask;
	MPTaskID curID = MPCurrentTaskID();
	bool needToAdd = true;
	for ( whichTask = 0; whichTask < gCurrentInterruptEquivalentTaskID; whichTask++ )
	{
		if ( !gInterruptEquivalentTaskIDs[whichTask] )
		{
			needToAdd = false;
			gInterruptEquivalentTaskIDs[whichTask] = curID;
			break;
		}
		else if ( gInterruptEquivalentTaskIDs[whichTask] == curID )
		{
			needToAdd = false;
		}
	}
	
	if (needToAdd)
	{
		// HX_ASSERT(gCurrentInterruptEquivalentTaskID < kMaxInterruptEquivalentTaskIDs);
		if (gCurrentInterruptEquivalentTaskID < kMaxInterruptEquivalentTaskIDs)
		{
			gInterruptEquivalentTaskIDs[gCurrentInterruptEquivalentTaskID] = curID;
			gCurrentInterruptEquivalentTaskID++;
		}
	}
}

void	HXMM_INTERRUPTOFF(void)
{
	long whichTask;
	MPTaskID curID = MPCurrentTaskID();
	bool needToAdd = true;
	for ( whichTask = 0; whichTask < gCurrentInterruptEquivalentTaskID; whichTask++ )
	{
		if ( gInterruptEquivalentTaskIDs[whichTask] == curID )
		{
			// xxxbobclark if we knew for sure that HXMM_INTERRUPTON()
			// would only be called at actual interrupt time (and not
			// occasionally at system time) then we wouldn't need to do
			// this. But since it's sometimes incorrectly called at system
			// time, we don't want to hold an array that thinks that
			// system time is interrupt time. Also since the embedded
			// player has "system time" on different threads we can't
			// just watch for the bad case of HXMM_INTERRUPTON() at
			// system time...
			gInterruptEquivalentTaskIDs[whichTask] = 0; // so it doesn't match later
		}
	}
}

HXBOOL	HXMM_ATINTERRUPT(void)
{
	long whichTask;
	MPTaskID curID = MPCurrentTaskID();
	for ( whichTask = 0; whichTask < gCurrentInterruptEquivalentTaskID; whichTask++ )
	{
		if ( gInterruptEquivalentTaskIDs[whichTask] == curID )
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

void HXMM_COMPACT(void)
{
	// do nothing
}





