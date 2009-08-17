#ifndef GET_DISK_FREE_H
#define GET_DISK_FREE_H

#if 0

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

HXBOOL __helix_GetDiskFreeSpaceExA(LPCSTR lpDirectoryName, 
				 PULARGE_INTEGER lpFreeBytesAvailableToCaller,
				 PULARGE_INTEGER lpTotalNumberOfBytes, 
				 PULARGE_INTEGER lpTotalNumberOfFreeBytes);

HXBOOL __helix_GetDiskFreeSpaceExW(LPCWSTR lpDirectoryName, 
				 PULARGE_INTEGER lpFreeBytesAvailableToCaller,
				 PULARGE_INTEGER lpTotalNumberOfBytes, 
				 PULARGE_INTEGER lpTotalNumberOfFreeBytes);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif // 0

#endif /* GET_DISK_FREE_H */
