/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: rartp.c,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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

#include <string.h>
#include "rartp.h"
#include "md5.h"

const unsigned char pRARTPMagic[] =
{
	0xab, 0x57, 0xac, 0xce,
	0xcd, 0x5d, 0x7d, 0xa9,
	0x38, 0x41, 0x64, 0x92,
	0x6f, 0x4f, 0x26, 0x21,
	0x85, 0xa, 0xe0, 0xd5,
	0xe, 0xad, 0xe1, 0x9,
	0x42, 0x2b, 0x52, 0xdb,
	0xe8, 0x95, 0x8a, 0xa2,
	0x92, 0xa6, 0xe8, 0xfa,
	0xe6, 0xcd, 0xf1, 0xf2,
	0x84, 0x6f, 0xc6, 0x60,
	0x86, 0x8, 0x9c, 0x2c,
	0x22, 0x51, 0x1f, 0x7b,
	0x85, 0x9b, 0x5f, 0x3f,
	0x90, 0xf6, 0x55, 0x7b,
	0xf6, 0xc1, 0xfc, 0xc1
};

int
RARTPResponse(char pResp[], char* pChallenge)
{
    unsigned char key[64];
    int i;
    memset(key, 0, 64);
    if (strlen(pChallenge) > 32)
    {
	return -1;
    }
    strcpy((char*)key, pChallenge);
    for (i = 0; i < 64; i++)
    {
	key[i] ^= pRARTPMagic[i];
    }
    MD5Data(pResp, key, 64);
    return 0;
}
