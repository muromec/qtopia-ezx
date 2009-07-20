/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: os.h,v 1.10 2007/07/06 22:00:32 jfinnecy Exp $
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

/*-----------------------------------------------------------------------------
 *
 *  NAME 
 *   Operating-system-dependent definitions.
 *   windows NT                             #define _NTWIN
 *   and 32 bits in general                 #define FLAT_OS
 *
 *  SYNOPSIS
 *    #include "os.h"
 *     ...
 *
 *  DESCRIPTION   
 *
 -----------------------------------------------------------------------------*/ 

#ifndef _OS_H_
#define _OS_H_

#if defined(XP_UNIX)
#define FOR_UNIX 1
#endif

#if defined(__MC68K__) || defined(__powerc)
#define FOR_MAC 1
#endif

#if defined(__MWERKS__) && defined(_WIN32)
// Compiling for Windows using the Mac Metrowerks compiler. This compiler doesn't
// support inline assembly.
#define FOR_WINMW
#endif

#if defined(_WIN32) || defined(FOR_MAC) || defined(FOR_UNIX)
#define  FLAT_OS
#endif


#ifdef FOR_MAC

#ifndef _MAC_MACHO
#include <Types.h>
#include <Memory.h>
#endif

#define UNREFERENCED_PARAMETER(x)

#define PASCAL
#define __declspec(X)

#ifdef _DEBUG
#define OutputDebugString(X) DebugStr((StringPtr) X)
#else
#define OutputDebugString(X) 
#endif

#define strcmpi strcasecmp

#elif defined (FOR_UNIX)

#define ASSERT assert
#define PASCAL
#define __declspec(X)

#define strcmpi strcasecmp
#define _fmemset memset

#define TRACE0

typedef long LONG;
typedef long LRESULT;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef char *LPSTR;
typedef unsigned char *LPBYTE;
typedef void         *LPVOID;

#else

#if defined(_WIN32) || defined(__WINDOWS_386__)
#define FOR_WINDOWS
#endif

/*
 * tell compiler to hush warnings about double slash comments.
 */
#if !defined(FOR_UNIX)
#pragma warning (disable: 4001)
#endif

#if !defined (_INC_WINDOWS) && !defined (FOR_UNIX) && !defined(_MACINTOSH)
#ifndef STDAFX_H
// If we haven't included MFC, we need to include windows.h
#include "hlxclib/windows.h"
#endif
#endif

#endif // FOR_MAC

/*
 * OS-Dependent macro for getting procedure pointers
 */
#define VVPROCPTR(name) (name)

/*
 * Use LOCALPROC as a modifier to indicate that a particular
 * function definition/declaration isn't visible outside the
 * present compilation unit.
 */
#define LOCALPROC static 
#ifdef NOLOCAL  //Useful for debugging with Softice/W
#undef LOCALPROC
#define LOCALPROC  //Not static, so debugger can see the name.
#endif

//Use PRIVATE as a modifier for variables not visible outside compilation unit
#define PRIVATE static 
#ifdef NOPRIVATE    //Useful for debugging with global-symbol-only debuggers
#undef PRIVATE
#define PRIVATE//Nothing    -- not static
#endif

/*
 * Definition of All-Purpose Section for getting 
 * initialization parameters from VIVOPROFILE
 */
#define GENERALPROFILE  "general"

#if defined(FLAT_OS)
#define mcLpStream              lpStream;
#define mcLpContext             lpContext;        
#define VvLpShadowFmLp(x)       (x)
#define PFMP1616(zType, p)      ((zType)(p))
#define mcLpData                lpData
#define mcLpFillBuffer          lpFillBuffer
#define mcLpHeader              lpHeader
#define CtxSwanFmPStm(pstm)     ((lpVvSwanContext)((pstm)->dwUserData))
#define mcLpLinearData          lpLinearData 
#define mcLpEchoData            lpEchoData
#define mclpEchoSuppressor      lpEchoSuppressor
#define mclpAttenuationTable    lpAttenuationTable
#define mclpInputGainTable      lpInputGainTable 
#define mclpLineGainTable       lpLineGainTable 
#define mclpCenterClipperTable  lpCenterClipperTable
#define mclpuLawToaLawTable     lpuLawToaLawTable
#define mclpaLawTouLawTable     lpaLawTouLawTable
#define mclpLinearTouLawTable   lpLinearTouLawTable
#define mclpuLawToLinearTable   lpuLawToLinearTable

// This is the callback to the Ring-3 Movieman audio  
#define mclpCallAddress           fpRing3Callback
#define mcSyncAudio              SyncRing3Audio
#define mcReadAudio              ReadRing3Audio
#define mcWriteAudio         WriteRing3Audio
//#define mcLpMvManAudioParms       lpMvManAudioParms
#define mcLpSilence              lpSilence
#define mcLpGarbage              lpGarbage 
#endif

#define AssertWindows() // Nothing
#define AssertMac() // Nothing

#if defined( _NTWIN) || defined(_WIN32) || defined(FOR_MAC)
#define _fmemset memset
#define _fmemcmp memcmp
#define _fstrlen strlen
#define _fstrchr strchr
#endif

#endif//ndef _OS_H_



