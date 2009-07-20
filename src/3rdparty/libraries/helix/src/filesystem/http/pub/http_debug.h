/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: http_debug.h,v 1.7 2008/02/08 18:22:45 praveenkumar Exp $
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

#undef  NULLOK
#define NULLOK(x)     ((x) ? (x) : "Null")

#if !defined(_SYMBIAN)
#include "hlxclib/time.h"

#if defined(_UNIX) || defined (_MACINTOSH) || defined(__TCS__)
#  include <unistd.h>
#elif defined(_OPENWAVE)
#else
   static SYSTEMTIME  stTime;

#  define getpid                GetCurrentProcessId

#  define gettimeofday(x,y)     GetSystemTime (&stTime); \
                                    (x)->tv_sec = time(NULL); \
                                    (x)->tv_usec = stTime.wMilliseconds * 1000
#endif
#endif //_SYMBIAN
#if defined(DEBUG) && defined(ENABLE_LOGGING) && !defined(_OPENWAVE) && !defined(_SYMBIAN)
#  include "hlxclib/fcntl.h"

   static FILE*           g_fileLog  = NULL;
   static struct timeval  g_tv       = { 0, 0 };
   static struct tm*      g_tmp      = NULL;

   static char            szDbgMesg[2048]; /* Flawfinder: ignore */
   static char            szDbgTemp[2048]; /* Flawfinder: ignore */

#  define LOG(x) \
            if (!g_fileLog) { \
                unlink (LOG_FILE "_3"); \
                rename (LOG_FILE "_2", LOG_FILE "_3"); \
                rename (LOG_FILE "_1", LOG_FILE "_2"); \
                rename (LOG_FILE     , LOG_FILE "_1"); \
                g_fileLog = fopen (LOG_FILE, "w"); \
            } \
            if (g_fileLog) { \
                gettimeofday (&g_tv, NULL); \
                g_tmp = localtime (&g_tv.tv_sec); \
                SafeSprintf (szDbgMesg, 2048, "%02d:%02d:%02d.%05ld ", g_tmp->tm_hour, \
                         g_tmp->tm_min,  g_tmp->tm_sec, g_tv.tv_usec / 10); \
                SafeSprintf (szDbgMesg + strlen(szDbgMesg), 2048-strlen(szDbgMesg), "%05d %s\n", getpid() % 100000, x); \
                fputs (szDbgMesg, g_fileLog); \
            } \
            fflush (NULL)

#  define LOGX(x) \
            if (!g_fileLog) { \
                unlink (LOG_FILE "_3"); \
                rename (LOG_FILE "_2", LOG_FILE "_3"); \
                rename (LOG_FILE "_1", LOG_FILE "_2"); \
                rename (LOG_FILE     , LOG_FILE "_1"); \
                g_fileLog = fopen (LOG_FILE, "w"); \
            } \
            if (g_fileLog) { \
                gettimeofday (&g_tv, NULL); \
                g_tmp = localtime (&g_tv.tv_sec); \
                SafeSprintf (szDbgMesg, 2048, "%02d:%02d:%02d.%05ld ", g_tmp->tm_hour, \
                         g_tmp->tm_min,  g_tmp->tm_sec, g_tv.tv_usec / 10); \
                sprintf x; /* Flawfinder: ignore */ \
                SafeSprintf (szDbgMesg + strlen(szDbgMesg), 2048-strlen(szDbgMesg), "%05d %s\n", getpid() % 100000, szDbgTemp); \
                fputs (szDbgMesg, g_fileLog); \
            } \
            fflush (NULL)
#else
#  define g_fileLog     stderr
#  define szDbgTemp     NULL
#  define LOG(x)               
#  define LOGX(x)          
#endif	/* DEBUG && ENABLE_LOGGING && !_OPENWAVE */

