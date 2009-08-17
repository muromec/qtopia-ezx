/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxmarsh.h,v 1.10 2007/02/28 05:45:11 gahluwalia Exp $
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

#ifndef _HXMARSH_H_
#define _HXMARSH_H_

#include "hlxclib/memory.h"

inline UINT8
getbyte(UINT8* data)
{
    return *data;
}

/* XXXSMP Should be UINT16 */
inline UINT16
getshort(UINT8* data)
{
    return ((UINT16)data[0])<<8|(UINT16)data[1];
}

inline INT32
getlong(UINT8* data)                           
{
    return (INT32) (((UINT32)getshort(data))<<16|(UINT32)getshort(data+2));
}

inline UINT64 getlonglong(UINT8* data)                           
{
    return (UINT64)( ((UINT64)getshort(data))<<48 | ((UINT64)getshort(data+2))<<32 |
                     ((UINT64)getshort(data+4))<<16 | (UINT64)getshort(data+8)
                     );
}

inline void
putbyte(UINT8* data, INT8 v)
{
    *data = v;
}

inline void
putshort(UINT8* data, UINT16 v)
{
    *data++ = (Byte)(v>>8);
    *data++ = (Byte)(v&0xff);
}

inline void
putshort(UINT8* data, int v)
{
    putshort(data,(unsigned short)v);
}

inline void
putshort(UINT8* data, INT16 v)
{
    putshort(data, (unsigned short)v);
}

inline void
putlong(UINT8* data, UINT32 v)
{
    *data++ = (Byte)(v>>24);
    *data++ = (Byte)((v>>16)&0xff);
    *data++ = (Byte)((v>>8)&0xff);
    *data++ = (Byte)(v&0xff);
}
#ifndef HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY
inline void putlonglong(UINT8* data, UINT64 v)
{
#if defined __SYMBIAN32__ && !defined(HELIX_CONFIG_SYMBIAN_PLATFORM_SECURITY)
    *data++ = (Byte)((v.High()>>24)&0xff);
    *data++ = (Byte)((v.High()>>16)&0xff);
    *data++ = (Byte)((v.High()>>8)&0xff);
    *data++ = (Byte)(v.High()&0xff);;
    *data++ = (Byte)((v.Low()>>24)&0xff);
    *data++ = (Byte)((v.Low()>>16)&0xff);
    *data++ = (Byte)((v.Low()>>8)&0xff);
    *data++ = (Byte)(v.Low()&0xff);
#else
    *data++ = (Byte)(v>>56);
    *data++ = (Byte)((v>>48)&0xff);
    *data++ = (Byte)((v>>40)&0xff);
    *data++ = (Byte)((v>>32)&0xff);
    *data++ = (Byte)((v>>24)&0xff);
    *data++ = (Byte)((v>>16)&0xff);
    *data++ = (Byte)((v>>8)&0xff);
    *data++ = (Byte)(v&0xff);
#endif /* __SYMBIAN32__ */
}
#endif
inline UINT8*
addbyte(UINT8* cp, UINT8 data)
{
    *cp = data;
    return cp+1;
}

inline UINT8*
addbyte(UINT8* cp, int data)
{
    *cp = data;
    return cp+1;
}

inline UINT8*
addshort(UINT8* cp, UINT16 data)
{
    putshort(cp, data);
    return cp + 2;
}

inline UINT8*
addlong(UINT8* cp, UINT32 data)
{
    putlong(cp, data);
    return cp + 4;
}

inline UINT8*
addstring(UINT8* cp, const UINT8* string, int len)
{
    memcpy(cp, string, len); /* Flawfinder: ignore */
    return cp + len;
}

inline UINT8*
addstring(UINT8* cp, const INT8* string, int len)
{
    memcpy(cp, string, len); /* Flawfinder: ignore */
    return cp + len;
}

#endif /* _HXMARSH_H_ */
