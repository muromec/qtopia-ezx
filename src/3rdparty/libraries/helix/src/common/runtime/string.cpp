/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: string.cpp,v 1.11 2008/01/18 09:17:26 vkathuria Exp $
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

#include "hlxclib/string.h"
#include "hxassert.h"
#include "globals/hxglobals.h"

char * __helix_strrev(char * str)
{
    unsigned int SmallIndex = 0;
    unsigned int BigIndex = strlen(str) - (unsigned)1;
    
    while (SmallIndex < BigIndex) {
	char Temp = str[SmallIndex];
	
	str[SmallIndex] = str[BigIndex];
	str[BigIndex] = Temp;
	
	SmallIndex++;
	BigIndex--;
    }
    
    return str;
}

void __helix_strlwr(char *s)
{
    for (; *s; s++)
    {
        if ((*s <= 'Z') && (*s >= 'A'))
            *s += ('a' - 'A');
    }
}

void __helix_strupr(char *s)
{
    for (; *s; s++)
    {
        if ((*s <= 'z') && (*s >= 'a'))
            *s -= ('a' - 'A');
    }
}

#ifdef _WINCE
int strcasecmp(const char* str1, const char* str2)
{
	int f=0,l=0;
	do 
	{
		if ( ((f = (unsigned char) (*(str1++))) >= 'A') &&
				(f <= 'Z') )
			f -= 'A' - 'a';
			
		if ( ((l = (unsigned char) (*(str2++))) >= 'A') &&
				(l <= 'Z') )
			l -= 'A' - 'a';
	} while (f && (f == l) );
	return ( f - l );
}
#endif //_WINCE

#ifdef _SYMBIAN

#undef strtoul

unsigned long __helix_strtoul(const char*s, char**end, int base)
{
    /* Symbian doesn't like leading +/- signs. So, strip and apply
     * later.
     */
    int t=1;
    if(s && (*s=='-' || *s=='+'))
    {
        if(*s=='-')
            t=-1;
        ++s;
    }
    return t*strtoul(s, end, base);
}
#endif


/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  __helix_strnstr(sc, str, n)
 *
 *  PARAMETERS:
 *		sc			search string
 *		str			string to be found
 *		n			len of sc
 *
 *  DESCRIPTION:
 *	finds a string in a string with a length restriction
 *
 *  RETURNS
 *	points to position in sc of str. or NULL if not found
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
const char*
__helix_strnstr(const char* sc, const char* str, size_t n)
{
    if ( !sc || !*sc )
    {
	return NULL;
    }
    else if ( !str || !*str )
    {
	return sc;
    }

    size_t len = strlen(str);
    for ( size_t i = 0; i < n && strlen(sc) > len; i++, sc++ )
    {
	if ( !strncmp(sc, str, len) )
	{
	    return sc;
	}
    }
    return NULL;
}

/*___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  __helix_strnchr(sc, str, n)
 *
 *  PARAMETERS:
 *		sc			search string
 *		c			character to be found
 *		n			len of sc
 *
 *  DESCRIPTION:
 *		finds a character in a string with a length restriction
 *
 *	RETURNS
 *		points to position in sc of c. or NULL if not found
 *___________________________________________________________________________
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
const char*
__helix_strnchr(const char* sc, const char c, size_t n)
{
    for ( size_t i = 0; i < n && *sc; ++i, ++sc)
    {
	if ( *sc == c )
	{
	    return sc;
	}
    }
    return NULL;
}

#if defined(_BREW)

unsigned long __helix_strspn (const char* str1, const char* str2)
{
    int count = 0;
    while (str1=='\0')	
    {
	if (strchr(str2, *str1)!=0)
	{
	    break;
	}
	str1++;
    }
    return count;
}

unsigned long __helix_strcspn (const char* str1, const char* str2)
{
    int count = 0;
    char* sstr1 = STRLOWER((char*)str1);
    char* sstr2 = STRLOWER((char*)str2);
    while (sstr1=='\0')	
    {
	if (strchr(sstr2, *sstr1)!=0)
	{
	    break;
	}
	sstr1++;
    }
    return count;	
}

int __helix_tolower (int c)
{
    return (c+32*(c>='A' && c<='Z'));
}

int __helix_toupper ( int c )
{
    return (c-32*(c>='a' && c<='z'));
}

char* __helix_strncat ( char * destination, const char * source, size_t num )
{
    STRLCAT(destination, source, num);	
    return destination;
}	

static const  char* const g_lastcharsttok = NULL;
char * __helix_strtok ( char * str, const char * delimiters )
{
    char*& lp = (char*&)HXGlobalPtr::Get(&g_lastcharsttok);
    if (str)
    {
	char* k = strstr(str, delimiters);
	lp = k;
	if (lp)
	{
	    *lp = 0;
	    lp++;
	}
	return str;
    }
    else
    {
	char* lpo = lp;
	if (lp)
	{
	    lp = strstr(lp, delimiters);
	    if (lp)
	    {
		*lp = 0;
		lp++;
	    }
	}
	return lpo;
    }
    return NULL;
}

char* __helix_strpbrk (const char * str1, const char * str2 )
{
    HX_ASSERT(0);
    return NULL;
}

#endif //_BREW