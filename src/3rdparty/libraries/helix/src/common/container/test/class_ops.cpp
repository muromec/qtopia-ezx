/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: class_ops.cpp,v 1.6 2007/07/06 20:35:03 jfinnecy Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include "./class_ops.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "hxstring.h"

static const int StringSize = 10;

template<> 
class ClassOps<void*> 
{
public:
    void* Create() const;
    void* Null() const;
    void Destroy(void*& obj) const;
    char* Print(void* const & obj) const;
    void* Copy(void* const & obj) const;
};

void* ClassOps<void*>::Create() const
{    
    int* pRet = new int(rand() & 0x7fffffff);
    return pRet;
}

void* ClassOps<void*>::Null() const
{
    return 0;
}

void ClassOps<void*>::Destroy(void*& obj) const
{
    int* pTmp = (int*)obj;

    delete pTmp;
}

char* ClassOps<void*>::Print(void* const & obj) const
{
    char* pRet = new char[20];

    if (obj)
	sprintf(pRet, "%d", *((int*)obj)); /* Flawfinder: ignore */
    else
	sprintf(pRet, "(null)"); /* Flawfinder: ignore */

    return pRet;
}

void* ClassOps<void*>::Copy(void* const & obj) const
{
    int* pTmp = (int*)obj;

    int* pNew = 0;

    if (pTmp)
	pNew = new int(*pTmp);

    return pNew;
}

template<> 
class ClassOps<LONG32> 
{
public:
    LONG32 Create() const;
    LONG32 Null() const;
    void Destroy(LONG32& obj) const;
    char* Print(const LONG32& obj) const;
    LONG32 Copy(const LONG32& obj) const;
};

LONG32 ClassOps<LONG32>::Create() const
{    
    return (LONG32)(rand() & 0x7fffffff);
}

LONG32 ClassOps<LONG32>::Null() const
{
    return 0;
}

void ClassOps<LONG32>::Destroy(LONG32& /*obj*/) const
{
}

char* ClassOps<LONG32>::Print(const LONG32& obj) const
{
    char* pRet = new char[21];

    sprintf(pRet, "%ldL", obj); /* Flawfinder: ignore */

    return pRet;
}

LONG32 ClassOps<LONG32>::Copy(const LONG32& obj) const
{
    return obj;
}

template<> 
class ClassOps<GUID> 
{
public:
    GUID Create() const;
    GUID Null() const;
    void Destroy(GUID& obj) const;
    char* Print(const GUID& obj) const;
    GUID Copy(const GUID& obj) const;
};

GUID ClassOps<GUID>::Create() const
{    
    GUID ret;

#if defined(HELIX_FEATURE_FULLGUID)
    ret.Data1 = rand();
    ret.Data2 = rand() & 0xffff;
    ret.Data3 = rand() & 0xffff;

    for (int i = 0; i < 8; i++)
	ret.Data4[i] = rand() & 0xff;
#else
    UINT32 ulGUID = rand() * NUM_GUIDS / RAND_MAX;
    ret           = (GUID) ulGUID;
#endif /* #if defined(HELIX_FEATURE_FULLGUID) */

    return ret;
}

GUID ClassOps<GUID>::Null() const
{    
    GUID ret;
    
    memset(&ret, 0, sizeof(GUID));

    return ret;
}

void ClassOps<GUID>::Destroy(GUID& /*obj*/) const
{
}

char* ClassOps<GUID>::Print(const GUID& obj) const
{
    char* pRet = new char[40];

#if defined(HELIX_FEATURE_FULLGUID)
    sprintf(pRet, "%08lx-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x", /* Flawfinder: ignore */
	    obj.Data1,
	    obj.Data2,
	    obj.Data3,
	    obj.Data4[0],
	    obj.Data4[1],
	    obj.Data4[2],
	    obj.Data4[3],
	    obj.Data4[4],
	    obj.Data4[5],
	    obj.Data4[6],
	    obj.Data4[7]);
#else
    UINT32 ulGUID = (UINT32) obj;
    sprintf(pRet, "%lu", ulGUID);
#endif /* #if defined(HELIX_FEATURE_FULLGUID) */

    return pRet;
}

GUID ClassOps<GUID>::Copy(const GUID& obj) const
{
    return obj;
}

template<> 
class ClassOps<CHXString> 
{
public:
    CHXString Create() const;
    CHXString Null() const;
    void Destroy(CHXString& obj) const;
    char* Print(const CHXString& obj) const;
    CHXString Copy(const CHXString& obj) const;
};

CHXString ClassOps<CHXString>::Create() const
{
    CHXString ret;
    for (int i = 0; i < StringSize; i++)
    {
	int num = (rand() & 0x1f);
	
	if (num <= 0xf)
	    ret += 'a' + num;
	else
	    ret += 'A' + (num - 0x10);
    }

    return ret;
}

CHXString ClassOps<CHXString>::Null() const
{
    return CHXString();
}

void ClassOps<CHXString>::Destroy(CHXString& /*obj*/) const
{
    // We don't need to to anything here
}

char* ClassOps<CHXString>::Print(const CHXString& obj) const
{
    char* pRet = new char[obj.GetLength() + 1];
    strcpy(pRet, obj); /* Flawfinder: ignore */

    return pRet;
}

CHXString ClassOps<CHXString>::Copy(const CHXString& obj) const
{
    return obj;
}
