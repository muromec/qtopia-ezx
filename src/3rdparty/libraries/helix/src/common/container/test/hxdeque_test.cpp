/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdeque_test.cpp,v 1.4 2004/07/09 18:21:31 hubbe Exp $
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

#include <stdio.h>

#include "machdep.h"
#include "types.h"

#include "hxtypes.h"

#include "hxdeque.h"

void fail_tests();

int main()
{
//	fail_tests();

	HX_deque			test_deque;
	char*				temp_str;
	u_long32			i;
	HX_deque::Iterator	j;

	ASSERT(test_deque.empty());
	for (j = test_deque.begin(); j != test_deque.end(); ++j)
	{
	    ASSERT(0);
	}

	for (i = 0; i < 200; i++)
	{
		temp_str = new char[256];
		sprintf(temp_str, "hello %ld", i); /* Flawfinder: ignore */ //changed from %d to %ld for win16 
		test_deque.push_back(temp_str);
	}
	ASSERT(test_deque.size() == 200);

	i = 0;
	for (j = test_deque.begin(); j != test_deque.end(); ++j)
	{
		temp_str = (char*) (*j);
		printf("output %d: %s\n", i, temp_str);
		++i;
	}

	for (i = 0; i < 200; i++)
	{
	     temp_str = (char*) test_deque[i];
	     printf("array output %d: %s\n", i, temp_str);
	}


	for (i = 0; i < 50; ++i)
	{
		temp_str = (char*) test_deque.pop_front();
		printf("pop_front %d: %s\n", i, temp_str);
		delete[] temp_str;
	}
	ASSERT(test_deque.size() == 150);

	for (i = 0; i < 150; ++i)
	{
		temp_str = (char*) test_deque.pop_back();
		printf("pop_back %d: %s\n", i, temp_str);
		delete[] temp_str;
	}
	ASSERT(test_deque.size() == 0);
	ASSERT(test_deque.empty());
	for (j = test_deque.begin(); j != test_deque.end(); ++j)
	{
	    ASSERT(0);
	}

	for (i = 0; i < 25; ++i)
	{
		temp_str = new char[256];
		sprintf(temp_str, "hello2 %ld", i);/* Flawfinder: ignore */ //%d to %ld
		test_deque.push_front(temp_str);
	}
	
	for (i = 25; i < 1000000; ++i)
	{
		temp_str = new char[256];
		sprintf(temp_str, "hello3 %ld", i);/* Flawfinder: ignore */ // %d to %ld
		test_deque.push_front(temp_str);
		temp_str = (char*) test_deque.pop_back();
		printf("resend behaviour %d: %s\n", i, temp_str);
		delete[] temp_str;
	}
	ASSERT(test_deque.size() == 25);

	//Need some tests for boundary conditions
}

//Each one of these tests should fail with an assertion when compiled with 
//-DDEBUG
//Without -DDEBUG they should not die, but return undefined things.
void fail_tests()
{
    HX_deque	test_deque;
    char*	temp_str;
    int		i;

    ASSERT(test_deque.empty());
    temp_str = (char*) test_deque.pop_front();
    printf("pop front returned %s\n", temp_str);
    temp_str = (char*) test_deque.pop_back();
    printf("pop back returned %s\n", temp_str);
    temp_str = (char*) test_deque[50];
    printf("operator[50] returned %s\n", temp_str);

    for (i = 0; i < 25; ++i)
    {
	temp_str = new char[256];
	sprintf(temp_str, "fail %ld", i); /* Flawfinder: ignore */ //%d to %ld
	test_deque.push_back(temp_str);
    }
    for (i = 0; i < 25; ++i)
    {
	test_deque.pop_front();
    }

    ASSERT(test_deque.empty());
    temp_str = (char*) test_deque.pop_front();
    printf("pop front returned %s\n", temp_str);
    temp_str = (char*) test_deque.pop_back();
    printf("pop back returned %s\n", temp_str);
    temp_str = (char*) test_deque[50];
    printf("operator[50] returned %s\n", temp_str);
}

