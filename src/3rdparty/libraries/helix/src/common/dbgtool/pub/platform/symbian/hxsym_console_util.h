/*****************************************************************************
 * hxsym_console_util.h
 * ---------------
 *
 *
 * Target:
 * Symbian OS
 *
 *
 * (c) 1995-2003 RealNetworks, Inc. Patents pending. All rights reserved.
 *
 *****************************************************************************/

#if !defined(HXSYM_CONSOLE_UTIL_H__)
#define HXSYM_CONSOLE_UTIL_H__

#if defined (HELIX_FEATURE_DPRINTF) || !defined(HELIX_FEATURE_LOGLEVEL_NONE)

void PrintConsole(const char* pszText);

#endif

#endif // HXSYM_CONSOLE_UTIL_H__