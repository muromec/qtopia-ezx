/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxopwavestdio.cpp,v 1.7 2004/07/09 18:20:57 hubbe Exp $
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
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hlxclib/limits.h"

// due to unsolved mysterey of assertion in OpDVPRINTF
// we currently disable it.
//#define OPENWAVE_PRINTF
extern "C" {

OpFsFd g_opfd;

int __helix_vprintf(const char  *format, va_list ap)
{
    int ret = 1;                // XXXSAB: what to return?
/// Turn OPENWAVE_PRINTF on needs DEBUG defined for 
/// Shibumi's OpDVPRINTF defined but that includes more
/// more duplicate definitions of other functions
#ifdef OPENWAVE_PRINTF
    OpNOTE(OpDVPRINTF(format, &ap));
#endif
    return ret;
}

//int __helix_fprintf(FILE* f, const char  *format, ...);
int __helix_printf(const char  *format, ...)
{
    int ret = 0;
#ifdef OPENWAVE_PRINTF 
    va_list args;
    va_start(args, format);
    OpDPRINTF(format, args);
    va_end(args);
#endif
    return ret;
}

int __helix_fprintf(FILE* f, const char  *format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}

/*
 *format supports only from string to ulong conversion in the form of
 * "%lu%lu%lu..."
 */
int __helix_sscanf(const char *buffer, const char *format, ...)
{
	
	int ret = 0;
	va_list args;
	va_start(args,format);
	
	if (strstr(format, "%lu") == NULL)
	{
		return ret;
	}
	while (*format != '\0')
	{
		char *pEnd = NULL;
		int val = strtoul(buffer,&pEnd, 16);
		if (val == ULONG_MAX || val == 0)
		{
			return ret;
		}
		unsigned long *pval = (unsigned long*)va_arg(args, unsigned long);
		if (pval)
		{
			*pval = val;
		}
		format += 3;
		buffer = pEnd + 1;
		ret++;
	}
	va_end(args);
	return ret;
}

FILE*	__helix_fopen(const char *filename, const char *mode)
{
	FILE *pf = NULL;
	OpFsFd fd = -1;
	OpFsFlags oflags= kOpFsFlagExcl;
	if (strcmp(mode, "r") == 0)
	{
		oflags = kOpFsFlagRdOnly;
	}
	else if (strcmp(mode,"w") == 0)
	{
		oflags = kOpFsFlagCreate | kOpFsFlagTrunc;
	}
	else if (strcmp(mode,"r+") == 0)
	{
		oflags = kOpFsFlagRdwr;
	}
	else if (strcmp(mode, "w+") == 0)
	{
		oflags = kOpFsFlagCreate | kOpFsFlagRdwr;
	}
	else if (strcmp(mode,"a") == 0)
	{
		oflags = kOpFsFlagCreate | kOpFsFlagWrOnly;
	}
	else if (strcmp(mode,"a+") == 0)
	{
		oflags = kOpFsFlagCreate | kOpFsFlagRdwr;
	}
	
	if ((fd = OpFsOpen( filename, oflags, 0644 )) != kOpFsErrAny)
        //if ((fd = OpFsOpen( filename, oflags, 0 )) != kOpFsErrAny)
	{
		g_opfd = fd;
		pf = (FILE*)&g_opfd;
	}
	return pf;
}

size_t	__helix_fread(void *buffer, size_t size, size_t count, FILE *stream)
{
	size_t numread = 0;
	if (stream)
	{
		if ((numread = OpFsRead((OpFsFd)*stream, buffer, size*count)) == kOpFsErrAny)
			numread = 0;
	}
	return numread;
}

size_t	__helix_fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
	size_t numwrite = 0;
	if (stream)
	{
		if (( numwrite = OpFsWrite( (OpFsFd)*stream, buffer, size*count)) == kOpFsErrAny)
		{
			numwrite = 0;
		}
	}
	return numwrite;
}

int	__helix_fseek(FILE *stream, long offset, int origin)
{
	int ret = 0;
	if (stream)
	{
		if (OpFsSeek((OpFsFd)*stream, (OpFsSize)offset, (U8CPU)origin) == kOpFsErrAny)
		{
			ret = -1;
		}
	}
	return ret;
}

int	__helix_fclose(FILE *stream)
{
	int ret = 0;
	if (stream)
	{
		if (OpFsClose((OpFsFd)*stream) == kOpFsErrAny)
		{
			ret = EOF;
		}
	}
	return ret;
}

int	__helix_feof(FILE *stream)
{
        int ret = 0;
	if (stream)
	{
		int numread = 0;
		char buffer[10];
		if ((numread = OpFsRead((OpFsFd)*stream, buffer, 1)) != 0)
		{
			// seek it back so we are not changing anything here
			if (numread == 1 && OpFsSeek((OpFsFd)*stream, -1, kOpFsSeekCur) != kOpFsErrAny)
			{
				ret = 1;
			}
			else
			{
				// some errors occur
				ret = EOF;
			}
		}
	}
	return ret;
}

long __helix_ftell(FILE *stream)
{
	long foffset = -1;
	if (stream)
	{
		foffset = OpFsSeek((OpFsFd)*stream, 0, kOpFsSeekCur);
	}
	return foffset;
}

char*	__helix_fgets(char *sbuf, int n, FILE *stream)
{
	char* rets = sbuf;
	if (stream)
	{
		/// use OpFsRead to implement. it is not the same as it is defined in c run-time 
		/// so it is not a correct implementation.
		if (OpFsRead((OpFsFd)*stream, sbuf, n) == kOpFsErrAny)
		{
			rets = NULL;
		}
	}
	return rets;
}

int __helix_fputc( int c, FILE *stream )
{
	int retc = EOF;
	if (stream)
	{
		if (OpFsWrite((OpFsFd)*stream, &c, 1) == 1)
		{
			retc = c;
		}
	}
	return retc;
}

int	__helix_ferror(FILE *stream)
{
	// not implemented so we suppose nothing can be said
	// about the error status but ok
	return 0;
}

int	__helix_fflush(FILE *stream)
{
	int ret = 0;
	if (stream)
	{
		if (OpFsFlush((OpFsFd)*stream) == kOpFsErrAny)
		{
			ret = EOF;
		}
	}
	return ret;
}

int	__helix_rename(const char *oldname, const char *newname)
{
	int ret = 0;
	if (OpFsRename(oldname, newname) == kOpFsErrAny)
	{
		ret = (int) kOpFsErrAny;
	}
	return ret;
}


FILE*	__helix_fdopen(int fd, const char *mode)
{
	// do nothing special but return the global OPENWAVE FILE 
	// cached by the global file descriptor
	return (FILE*)&g_opfd;
}


int     __helix_fileno(FILE* stream)
{
    int fd = 0;
    if (stream)
    {
        fd = (OpFsFd)*stream;
    }
    return fd;
}


}  // extern "C"
