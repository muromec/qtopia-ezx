/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _DBCSUTIL_H_
#define _DBCSUTIL_H_

#include "dbcs.h"

// Function: dbcsStrCopy
//      
// Copies string from src to dest, up to maxlen chars,
// and NULL terminates dest. dest must be at least maxlen chars.
//
inline size_t dbcsStrCopy(char* dest, const char* src, size_t maxlen);

// Function: strSize
//
// Returns the length of str (in bytes), not including NULL terminator.
// If return value is equal to maxlen, there was no null terminator
// str buffer must be at least maxlen chars.
//
inline size_t dbcsStrSize(char* str, size_t maxlen);

// Function: dbcsFindChar
//
// Searches up to maxlen characters of str for c.
// returns pointer to first occurrence, or NULL if 
// not found. 
//
inline char*  dbcsFindChar(const char* str, char c, size_t maxlen);

inline size_t dbcsStrCopy(char* dest, const char* src, size_t maxlen)
{     
    size_t nByte;

    if(!HXIsDBCSEnabled())
    {
        for (nByte = 0; nByte < maxlen && src[nByte] != '\0'; nByte++)
        {
            dest[nByte] = src[nByte];
        }
    }
    else
    {
        for (nByte = 0; nByte < maxlen && src[nByte] != '\0'; nByte++)
        {
            dest[nByte] = src[nByte];
            if(HXIsLeadByte(dest[nByte]))
            {
                // If this is a Lead Byte, get the rest
                if(nByte+1 < maxlen && src[nByte+1] != '\0')
                {
                    nByte++;
                    dest[nByte] = src[nByte];
                }

                // If this is a Lead Byte and the last byte, leave it off
                else
                {
                    dest[nByte] = '\0';
                    return nByte;
                }
            }
        }
    }
    dest[nByte] = '\0';
    return nByte;
}

inline size_t dbcsStrSize(const char* str, size_t maxlen)
{
    size_t nSize;
    for(nSize=0; nSize<maxlen && str[nSize] != '\0'; nSize++) ;
    return nSize;
}

inline char* dbcsFindChar(const char* str, char c, size_t maxlen)
{
    size_t nByte;

    if(!HXIsDBCSEnabled())
    {
        for(nByte = 0; nByte < maxlen && str[nByte] != '\0'; nByte++)
        {
            if(str[nByte] == c)
            {
                return (char*)(&str[nByte]);
            }
        }
    }
    else
    {
        for(nByte = 0; nByte < maxlen && str[nByte] != '\0'; nByte++)
        {
            // If lead byte, skip the next one
            if(HXIsLeadByte(str[nByte]))
            {
                nByte++;
            }
            else if(str[nByte] == c)
            {
                return (char*)(&str[nByte]);
            }
        }
    }

    return NULL;
}

#endif //_DBCSUTIL_H_
