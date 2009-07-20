/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: stdio.h,v 1.11 2008/01/18 09:17:27 vkathuria Exp $
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

#ifndef HLXSYS_STDIO_H
#define HLXSYS_STDIO_H

#if defined(_OPENWAVE)
#include "platform/openwave/hx_op_debug.h"
#include "platform/openwave/hx_op_stdc.h"
#include "platform/openwave/hx_op_fs.h"
#include "hlxclib/sys/types.h"
#elif defined (_BREW)
#include "hxtypes.h"
#ifdef AEE_SIMULATOR
#define _WIN32
#endif 
#include "AEESio.h"
#include "AEEStdlib.h"
#ifdef AEE_SIMULATOR
#undef _WIN32
#else
#define HLX_INLINE __inline
#endif	
#else
#include <stdio.h>
#include <stdarg.h>
#endif

#if __cplusplus
extern "C" {
#endif
/* Make sure vsnprintf is defined for all platforms */

int __helix_snprintf(char *str, size_t size, const char  *format, ...);
int __helix_vsnprintf(char *str, size_t size, const char  *format, va_list ap);

#ifndef HLX_INLINE
#if (defined(_WINDOWS) || defined(_OPENWAVE)) || defined(_BREW) && \
    !defined(__cplusplus) && defined(_MSC_VER)
#define HLX_INLINE __inline
#else
#define HLX_INLINE inline
#endif
#endif

#if defined (_BREW)
typedef void* FILE;
#define stdin  (FILE*)0
#define stdout (FILE*)1
#define stderr (FILE*)2
FILE* __helix_fopen(const char *, const char *);
size_t __helix_fread(void *, size_t, size_t, FILE *);
size_t __helix_fwrite(const void *, size_t, size_t, FILE *);
int	__helix_fseek(FILE *, long, int);
int	__helix_fclose(FILE *);
long __helix_ftell(FILE *);
int __helix_fileno(FILE* );
int	__helix_ferror(FILE *);
int	__helix_feof(FILE *);

#define fopen __helix_fopen
#define fread __helix_fread
#define fseek __helix_fseek
#define fwrite __helix_fwrite
#define fclose __helix_fclose
#define ftell __helix_ftell
#define fileno __helix_fileno
#define ferror __helix_ferror
#define feof __helix_feof

#ifndef EOF
#define EOF ((size_t)-1)
#endif
int __helix_fprintf(FILE* f, const char  *format, ...);
int __helix_fflush(FILE *);

HLX_INLINE int
printf ( const char * format, ... )
{
    DBGPRINTF(format);
    return TRUE;
}

HLX_INLINE int32
vsprintf(char *buf,const char *cpszFormat, va_list list)
{
    return VSPRINTF(buf, cpszFormat, list);
}

HLX_INLINE int
vsnprintf (char *buf, size_t len, const char *cpszFormat, va_list list)
{
    return VSNPRINTF(buf, len, cpszFormat, list);
}

HLX_INLINE int
sprintf (char *buf,const char *cpszFormat, ...)
{
    return SPRINTF(buf, cpszFormat);
}

HLX_INLINE int
snprintf (char *buf,uint32 len,const char *cpszFormat, ...)
{
    return SNPRINTF(buf,len,cpszFormat);
}

HLX_INLINE int
vprintf(const char *cpszFormat, va_list list )
{
    return VSPRINTF(NULL, cpszFormat, list);
}

HLX_INLINE int
wprintf(const wchar_t* wide)
{
    DBGPRINTF("Not Implemented yet");
    return 0;
}

int __helix_sscanf(const char *buffer, const char *format, ...);
#define fprintf __helix_fprintf
#define vfprintf __helix_fprintf
#define fflush	__helix_fflush 
#define sscanf  __helix_sscanf

#endif // _BREW

#if defined(_OPENWAVE)
int __helix_printf(const char* format, ...);
int __helix_vprintf(const char  *format, va_list ap);

int __helix_sscanf(const char *buffer, const char *format, ...);

#define printf __helix_printf
#define vprintf __helix_vprintf
#define snprintf op_snprintf
#define vsnprintf __helix_vsnprintf
#define _vsnprintf __helix_vsnprintf
#define sscanf  __helix_sscanf
#define unlink OpFsRemove

typedef void* FILE;
#define stdin  (FILE*)0
#define stdout (FILE*)1
#define stderr (FILE*)2

#ifndef EOF
#define EOF ((size_t)-1)

FILE*	__helix_fopen(const char *, const char *);
size_t	__helix_fread(void *, size_t, size_t, FILE *);
size_t	__helix_fwrite(const void *, size_t, size_t, FILE *);
int		__helix_fseek(FILE *, long, int);
int		__helix_fclose(FILE *);
int		__helix_feof(FILE *);
long	__helix_ftell(FILE *);
char*	__helix_fgets(char*, int, FILE *);
int		__helix_fputc(int, FILE *);
int		__helix_ferror(FILE *);

int		__helix_fflush(FILE *);
int		__helix_rename(const char *oldname, const char *newname);

FILE*	__helix_fdopen(int, const char *);
int     __helix_fileno(FILE* );

int __helix_fprintf(FILE* f, const char *format, ...);
int __helix_vfprintf(FILE* f, const char  *format, va_list ap);
#define puts(x) printf("%s\n", (x))

#define fopen	__helix_fopen
#define fread	__helix_fread
#define fseek	__helix_fseek
#define fwrite	__helix_fwrite
#define fclose	__helix_fclose
#define feof	__helix_feof
#define ftell	__helix_ftell
#define fgets	__helix_fgets
#define fputc	__helix_fputc
#define putc	__helix_fputc
#define ferror  __helix_ferror
#define rewind(fp)  __helix_fseek(fp, 0, SEEK_SET)

#define fprintf __helix_fprintf
#define vfprintf __helix_fprintf

#define fflush	__helix_fflush             
#define rename  __helix_rename

#define _fdopen  __helix_fdopen

#define fileno  __helix_fileno

#endif	// end of _OPENWAVE

#elif defined(_WINDOWS)
#define snprintf _snprintf
#define vsnprintf _vsnprintf

#elif defined(_SYMBIAN) || defined(_WINCE) || defined(_IRIX)
#define snprintf __helix_snprintf
#define vsnprintf __helix_vsnprintf
#endif

#if	__cplusplus
}
#endif

#endif /* HLXSYS_STDIO_H */
