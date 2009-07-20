/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: bitcheck.h,v 1.2 2003/01/23 23:42:54 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#if defined _LINUX && defined _DEBUG
#define GBITSCHECK(x) guard_bits_check((void*)x)

#define MEM_GUARD_SIZE 8

inline BOOL
guard_bits_check(void* p)
{
    char pDollar[] = "$$$$$$$$";
    char pAmp[] = "&&&&&&&&";
    p -= MEM_GUARD_SIZE;
    int size = *((int*)p);
    if (memcmp(p + sizeof(int), pDollar, MEM_GUARD_SIZE - sizeof(int)))
    {
	printf("GBC: 0x%x Before bits smashed!\n", p+MEM_GUARD_SIZE);
	return 0;
    }
    if (memcmp(p + size - MEM_GUARD_SIZE, pAmp, MEM_GUARD_SIZE))
    {
	printf("GBC: 0x%x After bits smashed!\n", p+MEM_GUARD_SIZE);
	return 0;
    }
    return 1; 
}
#else
#define GBITSCHECK(x) 1
#endif
