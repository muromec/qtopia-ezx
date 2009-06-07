#if 0

#include <stdlib.h>

/* 
 * Copy of required types from the WinCE header files.
 * Note: We CANNOT include any windows headers here because
 *       we will pull in the actual declaration for GetDiskFreeSpaceEx
 *       which is not what we want. This is a pain, but it is the only
 *       way to get declarations the way we need them so that things 
 *       link properly
 */

typedef long HXBOOL;
#define __RPC_FAR
typedef char CHAR;
typedef wchar_t WCHAR;
typedef /* [string] */ const CHAR __RPC_FAR *LPCSTR;
typedef /* [string] */ const WCHAR __RPC_FAR *LPCWSTR;
typedef unsigned __int64 ULONGLONG;
typedef struct  _ULARGE_INTEGER
    {
    ULONGLONG QuadPart;
    }	ULARGE_INTEGER;

typedef ULARGE_INTEGER *PULARGE_INTEGER;
#define WINAPI      __stdcall

#include "platform/wince/get_disk_free.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* 
 * Declare GetDiskFreeSpaceExA() and GetDiskFreeSpaceExW() the way
 * they should be in the WinCE header files.
 */
HXBOOL WINAPI GetDiskFreeSpaceExA(LPCSTR lpDirectoryName, 
				PULARGE_INTEGER lpFreeBytesAvailableToCaller,
				PULARGE_INTEGER lpTotalNumberOfBytes, 
				PULARGE_INTEGER lpTotalNumberOfFreeBytes);
    
HXBOOL WINAPI GetDiskFreeSpaceExW(LPCWSTR lpDirectoryName, 
				PULARGE_INTEGER lpFreeBytesAvailableToCaller,
				PULARGE_INTEGER lpTotalNumberOfBytes, 
				PULARGE_INTEGER lpTotalNumberOfFreeBytes);
#ifdef __cplusplus
};
#endif /* __cplusplus */

#ifdef UNICODE
HXBOOL __helix_GetDiskFreeSpaceExW(LPCWSTR lpDirectoryName, 
				 PULARGE_INTEGER lpFreeBytesAvailableToCaller,
				 PULARGE_INTEGER lpTotalNumberOfBytes, 
				 PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    return GetDiskFreeSpaceExW(lpDirectoryName,
			       lpFreeBytesAvailableToCaller,
			       lpTotalNumberOfBytes,
			       lpTotalNumberOfFreeBytes);
}
#else
HXBOOL __helix_GetDiskFreeSpaceExA(LPCSTR lpDirectoryName, 
				 PULARGE_INTEGER lpFreeBytesAvailableToCaller,
				 PULARGE_INTEGER lpTotalNumberOfBytes, 
				 PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    return GetDiskFreeSpaceExA(lpDirectoryName, 
			       lpFreeBytesAvailableToCaller,
			       lpTotalNumberOfBytes,
			       lpTotalNumberOfFreeBytes);
}
#endif
#endif // if 0

