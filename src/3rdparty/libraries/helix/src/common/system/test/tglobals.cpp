/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tglobals.cpp,v 1.5 2004/07/09 18:19:41 hubbe Exp $
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

#include "hxassert.h"
#include "globals/hxglobals.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/time.h"
#include "hxmsgs.h"  


#define TEST_SIZE	0x100
#define TEST_STRING	"This is a test, it is only a test."
#define TEST_STRING2	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

struct Foo
{
    char m_foo[TEST_SIZE];
};

const UINT32 g_uInt32 = 0;
const UINT32 g_uInt16 = 0;
const Foo* const g_pFoo = NULL;
const char* const g_pStr = NULL;
const CHXMapStringToString* const g_Str2Str = NULL;
const CHXSimpleList* const g_List = NULL;


const char* const kTestStrings[] =
{
    "foo",  "bar",
    "foo2", "bar2",
    "abc",  "xyz",
    "123",  "890",
    NULL
};


class HXGlobalStr
{
public:
    static char*& Get(GlobalID id, int size, const char* init)
    {
	HXGlobalManager* pGM = HXGlobalManager::Instance();

	GlobalPtr ptr = pGM->Get(id);
	if (!ptr)
	{
	    ptr = pGM->Add(id, (GlobalType)New(size, init), Delete);
	}
	return (char*&)*ptr;
    }

protected:
    static char* New(int size, const char* init)
    {
	char* p = new char[size];
	if (init)
	{
	    strncpy(p, init, size);
	    p[size] = 0;
	}
	return p;
    }

    static void Delete(GlobalType p)
    {
	delete[] (char*)p;
    }
};


int main()
{
    // randomize
    srand(time(NULL));

    fprintf( stderr, "global manager test running....\n" );

    ////////////////////////
    //
    // HXGlobalManager tests
    //
    ////////////////////////

    UINT32 uLast32 = 0;
    UINT16 uLast16 = 0;
    // test UINT32 accessor
    {
	UINT32& uInt32a = (UINT32&)HXGlobalInt32::Get(&g_uInt32);
	UINT32& uInt32b = (UINT32&)HXGlobalInt32::Get(&g_uInt32);
	if (&uInt32a != &uInt32b)
	{
	    fprintf( stderr, "FAILED: UINT32& global addresses don't match.\n" );
	    return 1;
	}

	for (UINT32 i = 0; i < 0x80000000; i += rand() % 0x1000)
	{
	    uInt32a = uLast32 = i;
	    if (uInt32b != uInt32a || uInt32a != uLast32)
	    {
		fprintf( stderr, "FAILED: UINT32& assignment failed.\n" );
		return 1;
	    }
	}
    }

    // test UINT16 accessor
    {
	UINT16& uInt16a = (UINT16&)HXGlobalInt16::Get(&g_uInt16);
	UINT16& uInt16b = (UINT16&)HXGlobalInt16::Get(&g_uInt16);
	if (&uInt16a != &uInt16b)
	{
	    fprintf( stderr, "FAILED: UINT16& global addresses don't match.\n" );
	    return 1;
	}

	for (UINT16 i = 0; i < 0x8000; i += rand() % 0x1000)
	{
	    uInt16a = uLast16 = i;
	    if (uInt16b != uInt16a || uInt16a != uLast16)
	    {
		fprintf( stderr, "FAILED: UINT16& assignment failed.\n" );
		return 1;
	    }
	}
    }

    // test pointer accessor
    {
	Foo*& pFooa = (Foo*&)HXGlobalPtr::Get(&g_pFoo);
	Foo*& pFoob = (Foo*&)HXGlobalPtr::Get(&g_pFoo);
	if (&pFooa != &pFoob)
	{
	    fprintf( stderr, "FAILED: void*& global addresses don't match.\n" );
	    return 1;
	}

	pFooa = new Foo;
	strcpy(pFooa->m_foo, TEST_STRING);
	if (strcmp(pFooa->m_foo, pFoob->m_foo) || strcmp(pFoob->m_foo, TEST_STRING))
	{
	    fprintf( stderr, "FAILED: void*& modification failed.\n" );
	    return 1;
	}
    }

    // test a vector
    {
	char*& pStra = (char*&)HXGlobalStr::Get(&g_pStr, TEST_SIZE, TEST_STRING);
	char*& pStrb = (char*&)HXGlobalStr::Get(&g_pStr, TEST_SIZE, TEST_STRING);
	if (&pStra != &pStrb)
	{
	    fprintf( stderr, "FAILED: char*& global addresses don't match.\n" );
	    return 1;
	}

	strcpy(pStra, TEST_STRING2);
	if (strcmp(pStra, pStrb) || strcmp(pStrb, TEST_STRING2))
	{
	    fprintf( stderr, "FAILED: char*& modification failed.\n" );
	    return 1;
	}
    }

    // test the ints again
    {
	UINT32& uInt32 = (UINT32&)HXGlobalInt32::Get(&g_uInt32);
	if (uInt32 != uLast32)
	{
	    fprintf( stderr, "FAILED: UINT32& match failed.\n" );
	    return 1;
	}

	UINT16& uInt16 = (UINT16&)HXGlobalInt16::Get(&g_uInt16);
	if (uInt16 != uLast16)
	{
	    fprintf( stderr, "FAILED: UINT16& match failed.\n" );
	    return 1;
	}
    }

    // test a map
    {
	CHXMapStringToString& g_Str2Str = HXGlobalMapStringToString::Get(&::g_Str2Str);
	for (const char* const* p = kTestStrings; *p != NULL; p += 2)
	{
	    g_Str2Str.SetAt(p[0], p[1]);
	}
    }

    // now check
    {
	CHXMapStringToString& g_Str2Str = HXGlobalMapStringToString::Get(&::g_Str2Str);
	for (const char* const* p = kTestStrings; *p != NULL; p += 2)
	{
	    CHXString value;
	    if (g_Str2Str.Lookup(p[0], value))
	    {
		if (strcmp(value, p[1]))
		{
		    fprintf( stderr, "FAILED: incorrect map entry.\n" );
		    return 1;
		}
	    }
	    else
	    {
		fprintf( stderr, "FAILED: map entry missing.\n" );
		return 1;
	    }
	}
    }

    // test a list
    {
	CHXSimpleList& g_List = HXGlobalList::Get(&::g_List);
	for (const char* const* p = kTestStrings; *p != NULL; ++p)
	{
	    g_List.AddTail((void*)*p);
	}
    }

    // now check
    {
	CHXSimpleList& g_List = HXGlobalList::Get(&::g_List);

	CHXSimpleList::Iterator i = g_List.Begin();
	for (const char* const* p = kTestStrings; *p != NULL; ++p, ++i)
	{
	    if ((void*)*p != *i)
	    {
		fprintf( stderr, "FAILED: incorrect list entry.\n" );
		return 1;
	    }
	}
    }

    fprintf( stderr, "PASSED!\n" ); 
    return 0;
}
