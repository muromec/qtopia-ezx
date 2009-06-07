/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tbitpack.cpp,v 1.2 2004/07/09 18:31:01 hubbe Exp $
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
#include <stdlib.h>

#include "hxassert.h"
#include "bitstream.h"
#include "bitpack.h"

#define VALUE_CT 100001

#define BUFFER_SIZE VALUE_CT * 32 / 8


#define VALUE_CT2 1001
#define PACK_BUF_SIZE 100
#define BUFFER_SIZE2 VALUE_CT2 * PACK_BUF_SIZE

char ToHex(int i)
{
    static char z_hex[] = "0123456789ABCDEF";

    return z_hex[i];
}

void ShowBuffer(unsigned char* pData, int size)
{
    int lines = size >> 4;
    char buf[16 * 3 + 1];
    int i = 0;
    
    printf ("ShowBuffer(%p, %d)\n", pData, size);

    while (i < size)
    {
	char* pTmp = buf;
	int j = 0;

	while (j < 16)
	{
	    if (i >= size)
		break;

	    *pTmp++ = ToHex((*pData >> 4) & 0xf);
	    *pTmp++ = ToHex((*pData++) & 0xf);
	    *pTmp++ = ' ';
	    i++;
	    j++;
	}

	*pTmp = '\0';
	
	
	printf ("%s\n", buf);
	
    }
}

int GetValue(int i, int size)
{
    int ret = i;

    ret = rand();

    ret &= ((1 << size) - 1);

    return ret;
}

int GetBitCount(int maxSize)
{   
    int mask = 1;
    
    while (mask <  2 * maxSize)
	mask = (mask << 1) | 0x1;

    //int ret = MIN(24, maxSize);
    int ret = (maxSize * (rand() & mask) ) / mask;
    HX_ASSERT(ret <= maxSize);

    return ret;
}

void GenBuffer(UINT8* pBuf, int size)
{
    for (int i = 0; i < size; i++)
	pBuf[i] = (UINT8)(rand() & 0xff);
}

int GetOffset()
{
    return rand() & 0x7;
}

bool RunTest(int run)
{
    // This tests basic bit operations

    bool failed = false;

    printf ("RunTest(%d)\n", run);

    UINT8 buf[BUFFER_SIZE];
    BitPacker pack(buf, BUFFER_SIZE);

    srand(42 * run);

    for (int i = 0; i < VALUE_CT; i++)
    {
	int bitCount = GetBitCount(31);
	int value = GetValue(i, bitCount);
	pack.PackBits(value, bitCount);
    }
    
    srand(42 * run);

    Bitstream unpack;

    unpack.SetBuffer(buf);

    for (int j = 0; !failed && (j < VALUE_CT); j++)
    {
	int bitCount = GetBitCount(31);
	int expected = GetValue(j, bitCount);
	int peekVal = unpack.PeekBits(bitCount);
	int getVal = unpack.GetBits(bitCount);

	if (expected != peekVal)
	{
	    printf ("%d expect %08x peekVal %08x\n",
		    j, expected, peekVal);

	    failed = true;
	}
	else if (peekVal != getVal)
	{
	    printf ("%d peekVal %08x getVal %08x\n",
		    j, peekVal, getVal);

	    failed = true;
	}
    }

    return !failed;
}

bool RunTest2(int run)
{
    printf ("RunTest2(%d)\n", run);

    bool failed = false;

    // This tests the buffer GetBits() operation
    const int MaxBufferSize = 100;
    const int MaxBufferBits = 8 * MaxBufferSize;
    const int MaxByteCount = MaxBufferSize / 2;
    const int MaxBitCount = MaxByteCount * 8;

    UINT8 buf[MaxBufferSize];
    GenBuffer(buf, MaxBufferSize);

    UINT8 tmpBuffer[MaxBufferSize];

    Bitstream baseStream;
    Bitstream expectStream;

    baseStream.SetBuffer(buf);
    expectStream.SetBuffer(buf);
    

    int bitCount = 0;
    for (int bitsLeft = MaxBufferBits; !failed && bitsLeft; bitsLeft -= bitCount)
    {
	int maxBits = (bitsLeft > MaxBitCount) ? MaxBitCount : bitsLeft;

	bitCount = GetBitCount(maxBits);
	
	baseStream.GetBits(bitCount, tmpBuffer);

	int tmpBitCount = bitCount;
	for (int i = 0; !failed && tmpBitCount; i++)
	{
	    int a = (tmpBitCount > 8) ? 8 : tmpBitCount;
	    ULONG32 expect = expectStream.GetBits(a);
	    ULONG32 result = tmpBuffer[i];

	    if (a < 8)
		expect <<= (8 - a);

	    if (expect != result)
	    {
		printf ("byte %d expect %02x got %02x\n",
			i, expect, result);
		failed = true;
	    }
	    
	    tmpBitCount -= a;
	}

    }

    return !failed;
}

bool CompareBuffers(const UINT8* pBuf, const UINT8* pExpect, ULONG32 bitCount)
{
    bool failed = false;

    for (int i = 0; !failed && bitCount; i++)
    {
	int a = (bitCount > 8) ? 8 : bitCount;
	ULONG32 expect = pExpect[i];
	ULONG32 result = pBuf[i];
	
	if (a < 8)
	{
	    ULONG32 mask = ~((1 << (8 - a)) - 1);
	    expect &= mask;
	    result &= mask;
	}
	
	if (expect != result)
	{
	    printf ("byte %d expect %02x got %02x\n",
		    i, expect, result);
	    failed = true;
	}
	
	bitCount -= a;
    }

    return !failed;
}



bool RunTest3(int run)
{
    bool failed = false;

    printf ("RunTest3(%d)\n", run);
    
    const int MaxBufSize = 100;
    const int MaxBitCount = MaxBufSize * 8;

    UINT8 src[MaxBufSize];
    UINT8 dst[MaxBufSize];
    UINT8 expectBuf[MaxBufSize];

    srand(42 * run);

    for (int i = 0; !failed && (i < 1000); i++)
    {
	BitPacker pack(dst, MaxBufSize);
	
	GenBuffer(src, MaxBufSize);
	
	ULONG32 offset = 1; //GetOffset();
	int bitCount = GetBitCount(MaxBitCount - offset);
	
	pack.PackBits(0xbeef, offset);
	pack.PackBits(src, bitCount, 0);
	
	Bitstream expectStream;
	expectStream.SetBuffer(dst);
	
	expectStream.GetBits(offset);
	expectStream.GetBits(bitCount, expectBuf);
	
	failed = !CompareBuffers(src, expectBuf, bitCount);
    }

    return !failed;
}

int main (int argc, char* argv[])
{
    int ret = 0;

    for (int i = 1; i < 24; i++)
    {
	if (!RunTest(i) ||
	    !RunTest2(i) ||
	    !RunTest3(i))
	{
	    ret = -1;
	    break;
	}
    }

    if (ret == 0)
	printf("Unit Test PASSED\n");
    else
	printf("Unit Test FAILED\n");

    return ret;
}
