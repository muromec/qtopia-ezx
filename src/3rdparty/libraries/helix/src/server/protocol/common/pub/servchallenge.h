/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servchallenge.h,v 1.2 2003/01/24 01:00:29 damonlan Exp $ 
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
#ifndef	_CHALLENGE_H_
#define	_CHALLENGE_H_

#define HX_SERV_COMPANY_ID_KEY_SIZE 16

long MakePNAChallengeKey(long k1);

void ServCalcCompanyIDKey(const char* starttime,
			  const char* guid,
			  const char* challenge,
			  UCHAR* outputKey);

struct Challenge {
			Challenge(Byte* ch, int len);
			Challenge(long k1, long k2, Byte* k3, Byte* k4);

static	long 		get_time_key(void);

	Byte*		response1(Byte* k1, Byte* k2, long k3, long k4);
	Byte*		response2(Byte* k1, Byte* k2, long k3, long k4);
        Byte*           response3(Byte* k1);
        Byte*           response4(Byte* k1);

	Byte		text[33];
	Byte		response[33];
};

struct RealChallenge
{
    RealChallenge();

    Byte* response1(Byte* k1);
    Byte* response2(Byte* k1);

    Byte		challenge[33];
    Byte		response[41];
    Byte		trap[9];
};

struct MidBoxChallenge
{
    MidBoxChallenge();

    Byte* response1(Byte* k1);
    Byte* response2(Byte* k1);

    Byte		challenge[33];
    Byte		response[41];
    Byte		trap[9];
};
#endif/*_CHALLENGE_H_*/
