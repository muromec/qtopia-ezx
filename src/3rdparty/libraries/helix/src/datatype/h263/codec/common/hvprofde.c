/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hvprofde.c,v 1.3 2004/07/09 18:31:58 hubbe Exp $
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



#include "hvdebtim.h"
#include "hvutilsw.h"

//#include "speed.h"
//#include "cpuid.h"


#define NBPROFTIMERS 16
struct CVvDebugTimer *	pVvProf[NBPROFTIMERS]={0};
unsigned long			pVvProfCount[NBPROFTIMERS]={0};

const char *cEventName[NBPROFTIMERS] = { /* Flawfinder: ignore */
	"YUV->RGB        ",
	"VvDecode        "
};

/* this is how to call it */
/*
	if(!pVvProf[0]) {
		pVvProf[0] = newCVvDebugTimer();
	}//memory leak on destruction

	StartTime(pVvProf[0]);
	StopAndAccuTime(pVvProf[0]);
 */

#define AWANKAID 1
/*
//S32 vvProf_nDisplayedFrames=0;//OK
S32 vvProf_nEncodedFrames=0;//OK
S32 vvProf_nEnc01Width=0;//OK
S32 vvProf_nEnc01Height=0;//OK
S32 vvProf_nDisWidth=0;//OK
S32 vvProf_nDisHeight=0;//OK
S32 vvProf_nBitrate=0;//OK
S32 vvProf_nDuration=0;
S32 vvProf_AudioDuration=0;//OK
*/

S32 vvProf_mcfunc[4][3];

extern void vvlogend();
extern void vvlog(U8 eventowner, U32 event, char *format, ...);


void  GetLogShutDownAllAccuTimes(void)
{
	double ptime;
	S32 ii=0;
	double dTimePerFrame;
	int	nFrames;
	double dDecodingTime=0.;
	double dPercentOfEncoding;
/*
	U32		nProcessorSpeed;
	short	nProcessor;
*/
	nFrames = pVvProfCount[0];
	vvlog(AWANKAID, 0xf400 | ii,	"%s \t \t %d\n", "Col Conv Frames  ", nFrames); 	ii++;

	vvlog(AWANKAID, 0xf400 | ii,	"%s \t \t %d\n", "DecodedFrames    ", pVvProfCount[1]); 	ii++;

	
	// get processor type and speed
	// CPUID
	/*
	nProcessor = wincpuid();
	nProcessorSpeed = cpurawspeed(0);
	vvlog(AWANKAID, 0xf300 | ii,	"%s \t \t %d\n", "Processor      ", nProcessor); 	ii++;
	vvlog(AWANKAID, 0xf300 | ii,	"%s \t \t %.0lf\n", "CPUSpeed       ", (double) nProcessorSpeed); 	ii++;
	*/

	//display all frame dependent times
	GetAccuTime(pVvProf[1], &ptime);
	dDecodingTime = (nFrames) ? (ptime/nFrames) : 0.;
	
	for(ii=0; ii<NBPROFTIMERS; ii++) {
		if(pVvProf[ii]) {
			GetAccuTime(pVvProf[ii], &ptime);
			dTimePerFrame = (pVvProfCount[ii]) ? (ptime/pVvProfCount[ii]) : 0.;
			dPercentOfEncoding = (dDecodingTime) ? (dTimePerFrame/dDecodingTime) : 0.;
			vvlog(AWANKAID, 0xf400 | ii, 
					"%s \t %.0lf\t%6.2lf\t%6.2lf\n", 
					cEventName[ii], 
					ptime, dTimePerFrame, 100*dPercentOfEncoding);
		}
	}

	for(ii=0; ii<NBPROFTIMERS; ii++) {
		if(pVvProf[ii]) {
			free(pVvProf[ii]);
		}
		pVvProf[ii] = 0;
		pVvProfCount[ii] = 0;

	}
	//memset(vvProf_mcfunc, 0, sizeof(vvProf_mcfunc));

	vvlogend();

	/* reset */
/*
	vvProf_nEncodedFrames=0;//OK
	vvProf_nEnc01Width=0;//OK
	vvProf_nEnc01Height=0;//OK
	vvProf_nDisWidth=0;//OK
	vvProf_nDisHeight=0;//OK
	vvProf_nBitrate=0;//OK
	vvProf_nDuration=0;
	vvProf_AudioDuration=0;//OK
*/

}



