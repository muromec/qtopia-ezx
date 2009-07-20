/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxwinver.cpp,v 1.20 2008/06/25 00:36:50 ping Exp $
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

#include "hxtypes.h"

#ifdef _WINDOWS
#include "hlxclib/windows.h"
#include <stdlib.h>             // needed for _MAX_PATH
#include <string.h>             // needed for strXXX() functions
#endif

#ifdef _MACINTOSH
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MAC_MACHO
#include "OpenTransport.h"
#endif
//#include "productversion.r"
#endif

#if defined(HELIX_CONFIG_NOSTATICS)
# include "globals/hxglobals.h"
#endif
#include "hxassert.h"
#include "hxwinver.h"
#include "dbcs.h"
//#include "hlxclib/stdio.h"

#ifdef _WIN32
#include "hxdllldr.h"
#endif

#ifdef _UNIX
#include <stdlib.h>
#ifndef _VXWORKS
#include <sys/utsname.h>
#endif  // VXWORKS
#endif
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#ifdef _WINDOWS
// Helper function for Win32.
#if defined(_WIN32)
HXBOOL IsCoProcessorPresentInWinNT(void);
#else
extern "C" int FAR PASCAL Is586( void );
#endif
#if defined(_WIN32) || defined(_WINDOWS)
HXBOOL ExtractDistributionCode(char* pDistBuffer, HMODULE hModule);
#endif
#endif //_WINDOWS

// Helper functions for macintosh
#if defined(_MACINTOSH) || defined(_MAC_UNIX)
void    GetMacSystemVersion(UINT16      *major, UINT16  *minor, UINT16  *release);

#if !defined(_CARBON) && !defined(_MAC_UNIX)
HXBOOL    IsPPC(void);
HXBOOL    HasFPU(void);
HXBOOL    HasOpenTransport (void);
#endif
#endif

/*
** DWORD HXGetWinVer( HXVERSIONINFO *pVersionInfo )
*
*  PARAMETERS:
*              pVersionInfo : A pointer to the version info struct to receive the
*                             results.  (Can be NULL, in which case our only side
*                             effect is our return value).
*
*  DESCRIPTION:
*              Gets information on the Windows platform and version we are running on.
*
*  RETURNS:
*              A flag indicating the platform we are running on.
*
*              If this is a 16bit build of a Helix module, then
*              one of the following values is possible:
*
*                      HX_PLATFORM_WINNT
*                      HX_PLATFORM_WIN95
*                      HX_PLATFORM_WIN16
*
*              If this is a 32bit build of a Helix module, then
*              one of the following values is possible:
*
*                      HX_PLATFORM_WINNT
*                      HX_PLATFORM_WIN95
*                      HX_PLATFORM_WIN32S
*
*  NOTES:
*    The behavior of the GetVersion() API is totally different under
*    16bit and 32bit builds. As such we have been forced to implement
*    this function differently for each compiler version.
*
*/

ULONG32 HXGetWinVer( HXVERSIONINFO* lpVersionInfo )
{
    HXVERSIONINFO       rVersionInfo;

    // Initialize Defaults!
    rVersionInfo.dwPlatformId  = HX_PLATFORM_UNKNOWN;
    rVersionInfo.dwMachineType = HX_MACHINE_UNKNOWN;
    rVersionInfo.wMajorVersion = 0;
    rVersionInfo.wMinorVersion = 0;
    rVersionInfo.wReleaseVersion = 0;
    rVersionInfo.bFPUAvailable = FALSE;

#ifdef _WINDOWS ////////// WINDOWS SPECIFIC //////////

#ifdef _WIN32
    OSVERSIONINFO osVersionInfo;
    memset(&osVersionInfo, 0, sizeof(OSVERSIONINFO));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osVersionInfo);
    rVersionInfo.wMajorVersion = (UINT16)(osVersionInfo.dwMajorVersion);
    rVersionInfo.wMinorVersion = (UINT16)(osVersionInfo.dwMinorVersion);
    rVersionInfo.wReleaseVersion = (UINT16)(osVersionInfo.dwBuildNumber);
    HXBOOL bIsNT = (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
#else
    ULONG32 dwVersion = GetVersion();
    rVersionInfo.wMajorVersion = (WORD)(LOBYTE( LOWORD( dwVersion ) ));
    rVersionInfo.wMinorVersion = (WORD)(HIBYTE( LOWORD( dwVersion ) ));
    HXBOOL bIsNT = ((GetWinFlags() & 0x4000) == 0x4000);
#endif


    if (bIsNT)
    {
        // Windows NT
        rVersionInfo.dwPlatformId = HX_PLATFORM_WINNT;
    }

#ifdef _WIN32
    // In a 32bit build, we call GetVersionEx:
    // Win95 returns MajorVersion 4, MinorVersion 0
    // Win98 returns MajorVersion 4, MinorVersion 10
    //
    else if (rVersionInfo.wMajorVersion < 4)
    {
        // Win32s
        rVersionInfo.dwPlatformId = HX_PLATFORM_WIN32S;
    }
    else if (rVersionInfo.wMajorVersion == 4)
    {
        if (rVersionInfo.wMinorVersion < 10)
        {
            // Win95
            rVersionInfo.dwPlatformId = HX_PLATFORM_WIN95;
        }
        else        {
            // Windows 98
            rVersionInfo.dwPlatformId = HX_PLATFORM_WIN98;
        }
    }
#else
    // In a 16bit build, we call GetVersion:
    // Win95 returns MajorVersion 3, MinorVersion 95
    // Win98 returns MajorVersion ??, MinorVersion ??
    //
    else if (rVersionInfo.wMajorVersion == 3)
    {
        if (rVersionInfo.wMinorVersion < 95)
        {
            // Win16
            rVersionInfo.dwPlatformId = HX_PLATFORM_WIN16;
        }
        else if (rVersionInfo.wMinorVersion < 98)
        {
            // Windows 98
            rVersionInfo.dwPlatformId = HX_PLATFORM_WIN95;
        }
        // XXXBJP need to confirm minorVersion > 95 in 16-bit Win98
        else 
        {
            // Windows 98
            rVersionInfo.dwPlatformId = HX_PLATFORM_WIN98;
        }
    }
#endif

    // Don't bother with any of the rest of this code, if no
    // struct was passed in!!!!!

    if (lpVersionInfo)
    {
        // Determine processor and FPU capabilities...
#if defined(_WIN32)
        SYSTEM_INFO     sysInfo;
        GetSystemInfo(&sysInfo);

        switch (sysInfo.wProcessorArchitecture)
        {
           case PROCESSOR_ARCHITECTURE_INTEL:
           {
               rVersionInfo.dwMachineType = HX_MACHINE_UNKNOWN;

               // Only Newer versions of NT correctly supports wProcessorLevel.
               if       (
                   bIsNT
                   &&
                   (
                       (rVersionInfo.wMajorVersion > 3)
                       ||
                       (rVersionInfo.wMajorVersion == 3 && rVersionInfo.wMinorVersion >= 50)
                       )
                   )
               {
                   int nVerGreaterThan486 = (sysInfo.wProcessorLevel - 4);
                   if (nVerGreaterThan486 < 0)
                   {
                       rVersionInfo.dwMachineType = HX_MACHINE_TOOSLOW;
                   }
                   else if (nVerGreaterThan486 == 0)
                   {
                       rVersionInfo.dwMachineType = HX_MACHINE_486;
                   }
                   else if (nVerGreaterThan486 == 1)
                   {
                       rVersionInfo.dwMachineType = HX_MACHINE_586;
                   }
                   else if (nVerGreaterThan486 == 2)
                   {
                       rVersionInfo.dwMachineType = HX_MACHINE_686;
                   }
                   else
                   {
                       rVersionInfo.dwMachineType = HX_MACHINE_PENTIUM3PLUS;
                   }
               }
               else
               {
                   // Win95, Win32s, and old versions of NT don't correctly support
                   // Processor level, so instead we look at dwProcessorType
                   switch (sysInfo.dwProcessorType)
                   {
                      case PROCESSOR_INTEL_386:      rVersionInfo.dwMachineType = HX_MACHINE_TOOSLOW;  break;
                      case PROCESSOR_INTEL_486:      rVersionInfo.dwMachineType = HX_MACHINE_486;      break;
                      case PROCESSOR_INTEL_PENTIUM:  rVersionInfo.dwMachineType = HX_MACHINE_586;      break;
                      default:                       rVersionInfo.dwMachineType = HX_MACHINE_UNKNOWN;  break;
                   }
               }
           }
           break;

           case PROCESSOR_ARCHITECTURE_MIPS:
           {
               rVersionInfo.dwMachineType = HX_MACHINE_MIPS;
           }
           break;

           case PROCESSOR_ARCHITECTURE_ALPHA:
           {
               rVersionInfo.dwMachineType = HX_MACHINE_ALPHA;
           }
           break;

           case PROCESSOR_ARCHITECTURE_PPC:
           {
               rVersionInfo.dwMachineType = HX_MACHINE_PPC;
           }
           break;

           default:
           {
               rVersionInfo.dwMachineType = HX_MACHINE_UNKNOWN;
           }
           break;
        }

        rVersionInfo.bFPUAvailable = IsCoProcessorPresentInWinNT();

#else
        DWORD dwWinFlags = GetWinFlags();
        if      (
            (dwWinFlags & WF_CPU386)
            ||
            (dwWinFlags & WF_CPU286)
            ||
            (dwWinFlags & WF_CPU186)
            ||
            (dwWinFlags & WF_CPU086)
            )
        {
            rVersionInfo.dwMachineType = HX_MACHINE_TOOSLOW;
        }
        if (dwWinFlags & WF_CPU486)
        {
#ifdef _WIN16
            //=-=w16.3 Is586() asserts pentium.obj; must be fixed; #ifdefed to TRUE
            int nVerGreaterThan486 = TRUE;
#else
            int nVerGreaterThan486 = Is586();
#endif
            switch(nVerGreaterThan486)
            {
               case 0:  rVersionInfo.dwMachineType = HX_MACHINE_486;     break;
               case 1:  rVersionInfo.dwMachineType = HX_MACHINE_586;     break;
               case 2:  rVersionInfo.dwMachineType = HX_MACHINE_686;     break;
               default: rVersionInfo.dwMachineType = HX_MACHINE_UNKNOWN; break;
            }
        }
        if (dwWinFlags & WF_80x87)
        {
            rVersionInfo.bFPUAvailable = TRUE;
        }
#endif
    }


////////// END WINDOWS SPECIFIC //////////

////////// START MACINTOSH SPECIFIC CODE//////////
#elif defined(_MACINTOSH) || defined(_MAC_UNIX)


#if defined(_CARBON) || defined(_MAC_UNIX)
    rVersionInfo.dwPlatformId  = HX_PLATFORM_MACOSX;
#if defined(__i386__)
    rVersionInfo.dwMachineType = HX_MACHINE_686;
#else
    rVersionInfo.dwMachineType = HX_MACHINE_PPC;
#endif
    rVersionInfo.bFPUAvailable = FALSE;
#else
    if (HasOpenTransport())
        rVersionInfo.dwPlatformId  = HX_PLATFORM_MACOT;
    else
        rVersionInfo.dwPlatformId  = HX_PLATFORM_MACTCP;

    if (IsPPC())
        rVersionInfo.dwMachineType = HX_MACHINE_PPC;
    else
        rVersionInfo.dwMachineType = HX_MACHINE_68K;
    rVersionInfo.bFPUAvailable = HasFPU();
#endif
    GetMacSystemVersion(&rVersionInfo.wMajorVersion,
                        &rVersionInfo.wMinorVersion,
                        &rVersionInfo.wReleaseVersion);


////////// END MACINTOSH SPECIFIC CODE////////////
#elif defined(_UNIX)

    struct utsname osInfo;   // structure to hold uname info
    if (uname(&osInfo) != -1)
    {
        double nVersionNum = atof(osInfo.release);  // version number
        char *pPeriod = NULL;
        rVersionInfo.wMajorVersion = (UINT16)nVersionNum;
        if (pPeriod = strchr(osInfo.release,'.'))
            rVersionInfo.wMinorVersion = atoi(pPeriod+1);

        // CODE FOR LINUX
#ifdef _LINUX
        // These are defaults, they are not 100% correct but they shouldn't be needed
        rVersionInfo.dwPlatformId = HX_PLATFORM_LINUX;
        rVersionInfo.dwMachineType = HX_MACHINE_586;

        // CODE FOR SOLARIS
#elif _SOLARIS
        // These are defaults, they are not 100% correct but they shouldn't be needed
        rVersionInfo.dwPlatformId = HX_PLATFORM_SOLARIS;
        rVersionInfo.dwMachineType = HX_MACHINE_SPARC;

        // do platform mapping
        if (!strcasecmp("SunOS",osInfo.sysname))
        {
            if (nVersionNum > 5.0)
                rVersionInfo.dwPlatformId = HX_PLATFORM_SOLARIS;
            else
                rVersionInfo.dwPlatformId = HX_PLATFORM_SUNOS;
        }

        // do machine mapping
        if (!strcasecmp("sun4m",osInfo.machine))
            rVersionInfo.dwMachineType = HX_MACHINE_SPARC;

        // CODE FOR IRIX
#elif _IRIX
        rVersionInfo.dwPlatformId = HX_PLATFORM_IRIX;
        rVersionInfo.dwMachineType = HX_MACHINE_MIPS;

        // CODE FOR SUNOS
#elif _SUNOS
        // These are defaults, they are not 100% correct but they shouldn't be needed
        rVersionInfo.dwPlatformId = HX_PLATFORM_SUNOS;
        rVersionInfo.dwMachineType = HX_MACHINE_SPARC;

        // do platform mapping
        if (!strcasecmp("SunOS",osInfo.sysname))
        {
            if (nVersionNum > 5.0)
                rVersionInfo.dwPlatformId = HX_PLATFORM_SOLARIS;
            else
                rVersionInfo.dwPlatformId = HX_PLATFORM_SUNOS;
        }

        // do machine mapping
        if (!strcasecmp("sun4m",osInfo.machine))
            rVersionInfo.dwMachineType = HX_MACHINE_SPARC;

#else
        rVersionInfo.dwPlatformId = HX_PLATFORM_UNKNOWN;
        rVersionInfo.dwMachineType = HX_MACHINE_UNKNOWN;
#endif

    }
    rVersionInfo.bFPUAvailable = TRUE;

////////// END UNIX SPECIFIC CODE////////////
#elif defined(_SYMBIAN)
    
    rVersionInfo.dwPlatformId  = HX_PLATFORM_SYMBIAN;
#ifdef __WINS__    
    rVersionInfo.dwMachineType = HX_MACHINE_SYMEMULATOR;
#else
    rVersionInfo.dwMachineType = HX_MACHINE_ARM;
#endif
    //XXGFW need to not hard code this.
    rVersionInfo.wMajorVersion   = 6;
    rVersionInfo.wMinorVersion   = 1;
    rVersionInfo.wReleaseVersion = 0;
    rVersionInfo.bFPUAvailable   = FALSE;
    
#elif defined(_OPENWAVE)
    
    rVersionInfo.dwPlatformId  = HX_PLATFORM_OPENWAVE;
#ifdef _OPENWAVE_SIMULATOR
    rVersionInfo.dwMachineType = HX_MACHINE_OWEMULATOR;
#else
    rVersionInfo.dwMachineType = HX_MACHINE_ARM;
#endif

    // XXXSAB need to not hard code this.
    rVersionInfo.wMajorVersion   = 6;
    rVersionInfo.wMinorVersion   = 1;
    rVersionInfo.wReleaseVersion = 0;
    rVersionInfo.bFPUAvailable   = FALSE;
    
#elif defined(_BREW)
    rVersionInfo.dwPlatformId  = HX_PLATFORM_BREW;
#ifndef AEE_SIMULATOR
    rVersionInfo.dwMachineType = HX_MACHINE_BREWEMULATOR;
#else
    rVersionInfo.dwMachineType = HX_MACHINE_ARM;
#endif
    //XXGFW need to not hard code this.
    rVersionInfo.wMajorVersion   = 6;
    rVersionInfo.wMinorVersion   = 1;
    rVersionInfo.wReleaseVersion = 0;
    rVersionInfo.bFPUAvailable   = FALSE;
#else


#error Add your platform code here!

#endif

    if (lpVersionInfo)
    {
        *lpVersionInfo = rVersionInfo;
    }
    return( rVersionInfo.dwPlatformId );
}


/*
 *      const char* HXGetOSName( ULONG32 nPlatformID)
 *
 *  PARAMETERS:
 *              nPlatformID:    A platformID (defined in hxwinver.h)
 *
 *  DESCRIPTION:
 *              Returns an OS string. Indicates the OS, not the build of
 *      the player.
 *
 *  RETURNS:
 *              pointer to static const buffer containing OS string.
 */
const char* HXGetOSName(ULONG32 nPlatformID)
{
    const char* strResult = NULL;

    switch (nPlatformID)
    {
       case (HX_PLATFORM_WIN16) :
           strResult="Win16";
           break;

       case (HX_PLATFORM_WIN32S) :
           strResult="Win32S";
           break;

       case (HX_PLATFORM_WIN95) :
           strResult="Win95";
           break;

       case (HX_PLATFORM_WIN98) :
           strResult="Win98";
           break;

       case (HX_PLATFORM_WINNT) :
           strResult="WinNT";
           break;

       case (HX_PLATFORM_MACOT) :
           strResult="MacOT";
           break;

       case (HX_PLATFORM_MACTCP) :
           strResult="MacTCP";
           break;

       case (HX_PLATFORM_MACOSX) :
           strResult="MacOSX";
           break;

       case (HX_PLATFORM_LINUX) :
           strResult="Linux";
           break;

       case (HX_PLATFORM_SOLARIS) :
           strResult="Solaris";
           break;

       case (HX_PLATFORM_IRIX) :
           strResult="Irix";
           break;

       case (HX_PLATFORM_SUNOS) :
           strResult="SunOS";
           break;

       case (HX_PLATFORM_SYMBIAN):
           strResult="Symbian";
           break;

       case (HX_PLATFORM_UNKNOWN) :
       default:
           strResult="UNK";
           break;
    }

    return(strResult);
}


/*
 *      const char* HXGetMachName(ULONG32 nMachineType)
 *
 *  PARAMETERS:
 *              nMachineType:   A machine ID (defined in hxwinver.h)
 *
 *  DESCRIPTION:
 *              Returns a Machine string. Indicates the installed CPU,
 *      not the build of the player.
 *
 *  RETURNS:
 *              pointer to static const buffer containing Machine string.
 */
const char* HXGetMachName(ULONG32 nMachineType)
{
    const char* pProcName = NULL;

    switch (nMachineType)
    {
       case HX_MACHINE_486:
           pProcName = "486";
           break;

       case HX_MACHINE_586:
           pProcName = "586";
           break;

       case HX_MACHINE_686:
           pProcName = "686";
           break;

       case HX_MACHINE_PENTIUM3PLUS:
           pProcName = "PentiumIIIPlus";
           break;

       case HX_MACHINE_PPC:
           pProcName = "PPC";
           break;

       case HX_MACHINE_68K:
           pProcName = "68K";
           break;

       case HX_MACHINE_ALPHA:
           pProcName = "Alpha";
           break;

       case HX_MACHINE_MIPS:
           pProcName = "Mips";
           break;

       case HX_MACHINE_SPARC:
           pProcName = "Sparc";
           break;

       case HX_MACHINE_TOOSLOW:
           pProcName = "SLOW";
           break;

       case HX_MACHINE_ARM:
           pProcName = "ARM";
           break;

       case HX_MACHINE_SYMEMULATOR:
           pProcName = "SymbianEmulator";
           break;
 
       case HX_MACHINE_UNKNOWN:
       default:
           pProcName = "UNK";
           break;
    }

    return(pProcName);
}

/*
 *      const char* HXGetVerEncodedName( HXVERSIONINFO* pVersionInfo,
 *                                      const char* pProductName, const char* pProductVer,
 *                                      const char* pDistCode)
 *
 *  PARAMETERS:
 *              pVersionInfo :  A pointer to the version info struct to receive the
 *                                              results.
 *
 *              pProductName :  A pointer to the name of the product like play16 for
 *                                              the 16bit compiled player or plug32 for the 32bit
 *                                              compiled plugin.
 *
 *              pProductVer :   A pointer to the string form of the version of the
 *                                              product like 2.0b3.
 *
 *              pDistCode       :       A pointer to the string form of the version of the
 *                                              distribution code like PN01 for the player.
 *
 *  DESCRIPTION:    

 *              Returns a standard formatted encoded platform string.
 *
 *  RETURNS:
 *              pointer to temporary buffer containing the encoded string.
 */
const char* HXGetVerEncodedName
(
    HXVERSIONINFO* pVersionInfo,
    const char* pProductName,
    const char* pProductVer,
    const char* pLanguage,
    const char* pDistCode
    )
{
#if defined(HELIX_CONFIG_NOSTATICS)
    static const char _szEncodedName = '\0';
    char*& szEncodedName = HXGlobalCharArray::Get(&_szEncodedName, MAX_ENCODED_NAME, "" );
#else
    static char szEncodedName[MAX_ENCODED_NAME]; /* Flawfinder: ignore */
#endif        

    HX_ASSERT_VALID_READ_PTR(pVersionInfo);
    HX_ASSERT_VALID_READ_PTR(pProductName);
    HX_ASSERT_VALID_READ_PTR(pProductVer);
    HX_ASSERT_VALID_READ_PTR(pLanguage);
    HX_ASSERT_VALID_READ_PTR(pDistCode);

    const char* pOSName = "UNK";
    const char* pProcName = "UNK";

    // Calculate OS Name from
    pOSName = HXGetOSName(pVersionInfo->dwPlatformId);

    // CPU/Process/machine. same thing...
    pProcName = HXGetMachName(pVersionInfo->dwMachineType);

#ifdef _MACINTOSH
    // Macintosh OS version currently includes a wReleaseVersion number.
    SafeSprintf(szEncodedName,MAX_ENCODED_NAME,"%s_%d.%d.%d_%s_%s_%s_%s_%s%s",
                pOSName,
                pVersionInfo->wMajorVersion,
                pVersionInfo->wMinorVersion,
                pVersionInfo->wReleaseVersion,
                pProductVer,
                pProductName,
                pDistCode,
                pLanguage,
                pProcName,
                (pVersionInfo->bFPUAvailable ? "" : "_No-FPU")
                );
#else
    // WINDOWS and UNIX
    SafeSprintf(szEncodedName,MAX_ENCODED_NAME, "%s_%d.%d_%s_%s_%s_%s_%s%s",
                pOSName,
                pVersionInfo->wMajorVersion,
                pVersionInfo->wMinorVersion,
                pProductVer,
                pProductName,
                pDistCode,
                pLanguage,
                pProcName,
                (pVersionInfo->bFPUAvailable ? "" : "_No-FPU")
                );

#endif

    return szEncodedName;
};

//      Code From:
//
//      PSS ID Number: Q124207
//
//      Authored 21-Dec-1994                    Last modified 05-Jan-1995
//
//      The information in this article applies to:
//
//      - Microsoft Win32 Software Development Kit (SDK) for Windows NT
//      versions 3.1 and 3.5
#if defined(_WIN32) 
HXBOOL IsCoProcessorPresentInWinNT(void)
{
#if !defined(WIN32_PLATFORM_PSPC)
    HKEY hKey;
    SYSTEM_INFO SystemInfo;
    
    // return FALSE if we are not running under Windows NT
    // this should be expanded to cover alternative Win32 platforms
    
    if(!(GetVersion() & 0x7FFFFFFF))
    {
        return(FALSE); // We can't tell, assume it doesn't
    }
    
    // we return TRUE if we're not running on x86
    // other CPUs have built in floating-point, with no registry entry
    GetSystemInfo(&SystemInfo);
    if(SystemInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL)
    {
        return(TRUE);
    }

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "HARDWARE\\DESCRIPTION\\System\\FloatingPointProcessor",
                    0,
                    KEY_EXECUTE,
                    &hKey) != ERROR_SUCCESS)
    {

        // GetLastError() will indicate ERROR_RESOURCE_DATA_NOT_FOUND
        // if we can't find the key.  This indicates no coprocessor present
        return(FALSE);
    }

    RegCloseKey(hKey);
    return(TRUE);
#else /*!defined(WIN32_PLATFORM_PSPC) */
    return FALSE;
#endif /* !defined(WIN32_PLATFORM_PSPC) */
}
#endif

///////////////////////////////////////////////////////////////////////
//
//      FUNCTION:
//
//              HXExtractDistributionCode()
//
//      Description:
//
//              Extracts the distribution code resource from the version
//              information of the module.
//
//
HXBOOL HXExtractDistributionCode(char* pDistBuffer, UINT32 ulDistBufferLen, void* hMod)
{
    HXBOOL        bOk = FALSE;

#if (defined(_WIN32) || defined(_WINDOWS)) && !defined(WIN32_PLATFORM_PSPC)
    // load version.dll
    HINSTANCE               hLib = NULL;
    VERQUERYVALUE           _pVerQueryValue = NULL;
    GETFILEVERSIONINFO      _pGetFileVersionInfo = NULL;
    GETFILEVERSIONINFOSIZE  _pGetFileVersionInfoSize = NULL;

    if (hLib = HXLoadLibrary("version.dll"))
    {
        _pVerQueryValue = (VERQUERYVALUE)GetProcAddress(hLib, "VerQueryValueA");
        _pGetFileVersionInfo = (GETFILEVERSIONINFO)GetProcAddress(hLib, "GetFileVersionInfoA");
        _pGetFileVersionInfoSize = (GETFILEVERSIONINFOSIZE)GetProcAddress(hLib, "GetFileVersionInfoSizeA");
    }

    if (_pVerQueryValue         &&
        _pGetFileVersionInfo    &&
        _pGetFileVersionInfoSize)
    {
        HINSTANCE hModule = (HINSTANCE)(long)hMod;

        DWORD   dwVerInfoSize;
        DWORD   dwVerHnd;
        char    szFullPath[_MAX_PATH+1]; /* Flawfinder: ignore */

        if (GetModuleFileName(hModule,szFullPath, _MAX_PATH))
        {

            dwVerInfoSize = _pGetFileVersionInfoSize(szFullPath, &dwVerHnd);

            if (dwVerInfoSize)
            {
                LPSTR   lpstrVffInfo;             // Pointer to block to hold info
                HANDLE  hMem;                     // handle to mem alloc'ed

                // Get a block big enough to hold version info
                hMem          = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
                lpstrVffInfo  = (char *)GlobalLock(hMem);

                if(_pGetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo ))
                {
                    char    szGetName[_MAX_PATH]; /* Flawfinder: ignore */
                    UINT    VersionLen;
                    LPSTR   lpVersion;
                    HXBOOL    bRetCode;

                    SafeStrCpy(szGetName, "\\VarFileInfo\\Translation", _MAX_PATH);
                    bRetCode = _pVerQueryValue(lpstrVffInfo, szGetName,
                                               (void FAR* FAR*)&lpVersion, &VersionLen);

                    char TransNumber[10]; /* Flawfinder: ignore */
                    wsprintf(TransNumber, "%8lx", *(long *)lpVersion);
                    char *pSpace = HXFindChar(TransNumber, ' ');
                    while(pSpace)
                    {
                        *pSpace = '0';
                        pSpace = (char*)HXFindChar(TransNumber, ' ');
                    }

                    SafeStrCpy(szGetName, "\\StringFileInfo\\", _MAX_PATH);
                    SafeStrCat(szGetName, TransNumber + 4, _MAX_PATH);
                    TransNumber[4] = 0;
                    SafeStrCat(szGetName, TransNumber, _MAX_PATH);
                    SafeStrCat(szGetName, "\\DistCode", _MAX_PATH);

                    bRetCode = _pVerQueryValue(lpstrVffInfo, szGetName,
                                               (void FAR* FAR*)&lpVersion, &VersionLen);

                    if ( bRetCode && VersionLen && lpVersion)
                    {
                        SafeStrCpy(pDistBuffer,lpVersion, ulDistBufferLen);
                        bOk = TRUE;
                    }
                }

                // Let go of the memory
                GlobalUnlock(hMem);
                GlobalFree(hMem);
            }
        }

        if (!bOk)
        {
            SafeStrCpy(pDistBuffer,"UNK", ulDistBufferLen);
        }
    }

    HXFreeLibrary("version.dll");

#elif defined (_MACINTOSH)
    // dist code stored in STR 1000 resource of application
    INT16 strResId = 1000;
    StringHandle hStr = ::GetString(strResId);
    if (hStr)
    {
        if ((*hStr)[0] < ulDistBufferLen)
        {
            strncpy(pDistBuffer,(char*)*hStr+1,(*hStr)[0]); /* Flawfinder: ignore */
            pDistBuffer[(*hStr)[0]] = 0;
        }
        ::ReleaseResource((Handle)hStr);
        bOk = TRUE;
    }
    else
    {
        SafeStrCpy(pDistBuffer,"UNK", ulDistBufferLen);
    }
    
#endif // defined(_WIN32) || defined(_WINDOWS)

    return bOk;
}



#if defined(_MACINTOSH) || defined(_MAC_UNIX)

// Support functions for HXGetWinVer for Macintosh

#if !defined(_CARBON) && !defined(_MAC_UNIX) // Carbon code runs only on PPC processors with OT so runtime checks are pointless
HXBOOL
HasOpenTransport (void)
{
    OSErr   theErr = noErr;
    LONG32  result;
    Boolean hasOT = FALSE;

    theErr = Gestalt(gestaltOpenTpt, &result);
    hasOT = theErr == noErr &&
        (result & gestaltOpenTptPresentMask) != 0 &&
        (result & gestaltOpenTptTCPPresentMask) != 0;

    return hasOT;
}

HXBOOL
HasFPU(void)
{
    long    theFPU;

    /* Determine the coprocessor type */
    Gestalt(gestaltFPUType, &theFPU);

    if(theFPU == gestaltNoFPU)
        return FALSE;

    return TRUE;

}

HXBOOL
IsPPC(void) {
    long    theCPU;
    Gestalt(gestaltNativeCPUtype, &theCPU);
    if (theCPU >= gestaltCPU601) return(TRUE);
    return(FALSE);
}
#endif // !_CARBON

static UINT16 
BCDtoDecimal(UINT16 bcd)
{
    UINT16 decimal;
    decimal =            1 * ((bcd & 0x000F) >> 0)
        +   10 * ((bcd & 0x00F0) >> 4) 
        +  100 * ((bcd & 0x0F00) >> 8) 
        + 1000 * ((bcd & 0xF000) >> 12); 
    return decimal;
}

void
GetMacSystemVersion(UINT16 *major, UINT16 *minor, UINT16 *release)
{
    long    gestVer;
    UINT16  ver, bigRev, littleRev;

    OSErr err = Gestalt(gestaltSystemVersion, &gestVer); // returns a BCD number, like 0x00001015 for 10.1.5
    if (err == noErr)
    {
        ver =       BCDtoDecimal((gestVer & 0x0000FF00) >> 8);
        bigRev =    BCDtoDecimal((gestVer & 0x000000F0) >> 4);
        littleRev = BCDtoDecimal((gestVer & 0x0000000F));
    }
    else
    {
        ver = bigRev = littleRev = 0;
    }
        
//don't want to include mathlib in plugin
//      ::sprintf(s,"%ld.%ld.%ld",ver,majorRev,minorRev);
//      ver += 0x30; majorRev += 0x30; minorRev += 0x30;  // GR 7/4/02 why was this converting to ascii? (the conversion would fail for numbers over 9 anyway)

    if (major)      *major = ver;
    if (minor)      *minor = bigRev;
    if (release)    *release = littleRev;
}


#endif // _MACINTOSH

