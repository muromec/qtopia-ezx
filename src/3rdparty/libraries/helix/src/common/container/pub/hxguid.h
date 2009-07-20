/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxguid.h,v 1.9 2006/10/30 21:59:46 gwright Exp $
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

#ifndef _HXGUID_H
#define _HXGUID_H

#include "hxcom.h"

#if defined(HELIX_FEATURE_FULLGUID)
#if defined _MACINTOSH || defined _UNIX || defined _SYMBIAN || defined _OPENWAVE
extern const GUID GUID_NULL;
#endif
#endif

class CHXString;

class CHXGUID
{
public:

    CHXGUID(void);
    CHXGUID(REFGUID guid);
    CHXGUID(const char* pString);
    ~CHXGUID(void);

    HXBOOL Set(REFGUID guid);
    HXBOOL Set(const char* pString);

    HXBOOL Get(CHXString& string);
    HXBOOL Get(char* pBuffer, INT32 bufLen);

    virtual HXBOOL IsNull(void);
    GUID m_guid;
};

class CHXIID : public CHXGUID
{
public:

    CHXIID(void);
    CHXIID(REFIID iid);
    CHXIID(const char* pString);
    ~CHXIID(void);

    HXBOOL Set(REFIID iid);
};

class CHXCLSID : public CHXGUID
{
public:

    CHXCLSID(void);
    CHXCLSID(REFIID iid);
    CHXCLSID(const char* pString);
    ~CHXCLSID(void);

    HXBOOL Set(REFCLSID clsid);
};

inline CHXIID::CHXIID(void)
: CHXGUID()
{
}

inline CHXIID::CHXIID(REFIID iid)
: CHXGUID(iid)
{
}

inline CHXIID::CHXIID(const char* pString)
: CHXGUID(pString)
{
}

inline CHXIID::~CHXIID(void)
{
}

inline HXBOOL CHXIID::Set(REFIID iid)
{
    return(CHXGUID::Set(iid));
}

inline CHXCLSID::CHXCLSID(void)
: CHXGUID()
{
}

inline CHXCLSID::CHXCLSID(REFIID iid)
: CHXGUID(iid)
{
}

inline CHXCLSID::CHXCLSID(const char* pString)
: CHXGUID(pString)
{
}

inline CHXCLSID::~CHXCLSID(void)
{
}

inline HXBOOL CHXCLSID::Set(REFCLSID iid)
{
    return(CHXGUID::Set(iid));
}

#endif	
