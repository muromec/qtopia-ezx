/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hvdebtim.h,v 1.2 2004/07/09 18:31:58 hubbe Exp $
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

/********************************/
/* this is for header			*/

#include "machine.h" 

//struct CHvDebugTimer;
#ifdef __cplusplus

struct CHvDebugTimer {


#define WIN31_TIME 0
#define WIN95_TIME 1
#define PENT_TIME 2

#define FREQUENCY_VALID 0
#define FREQUENCY_NOTMEASURED 1
#define FREQUENCY_NOTVALID 2

// choose timer type here:

#ifdef WIN16
//#define TIMERTYPE WIN31_TIME
#endif
#define TIMERTYPE PENT_TIME



#if TIMERTYPE==WIN31_TIME
	DWORD			dwStartCount;
	DWORD			dwStopCount;
#else
	LARGE_INTEGER	liStartCount;
	LARGE_INTEGER	liStopCount;
	//LARGE_INTEGER	liFrequency;
#endif

	__int64  qwDeltaCount;
	__int64  qwAccuCount;

	__int64  qwAverageAccuCount;

	S32		nSkipToNextOutput;
#define NBDELTASHISTORY 100
	__int64  qwDeltaHistory[NBDELTASHISTORY];
	S32		nDeltaHistoryPtr;


    //  external member functions
	CVvDebugTimer();
	void	ResetTime();
	void	StartTime();
	void	StopAndAccuTime();
	void	OutputDeltaTime(char *pcLabel, S32 nMaxSkip);
	int		GetAccuTime(double *ptime);
	void	OutputAccuTime(char *pcLabel, S32 nMaxSkip);
	void	LogAccuTime(char *pcLabel, S32 nMaxSkip);
	void	AverageAndOutputAccuTime(char *pcLabel, S32 nMaxSkip);
	void	AverageAndOutputAccuTime(char *pcLabel, S32 nMaxSkip, 
									 char *pcOut, S32 nbMaxChar);

	
};

#endif

/**************************************************/
#ifdef __cplusplus
extern "C" {
#endif
    struct CHvDebugTimer *newCHvDebugTimer ();
	void AverageAndOutputAccuTime(struct CHvDebugTimer *, 
			char *pcLabel, S32 nMaxSkip);
	void AverageAndOutputAccuTimeString(struct CHvDebugTimer *, 
			char *pcLabel, S32 nMaxSkip, char *pcOut, S32 nbMaxChar);
	int	GetAccuTime(struct CHvDebugTimer *p, double *ptime);

	void OutputAccuTime(struct CHvDebugTimer *p, 
					char *pcLabel, S32 nMaxSkip);
	void LogAccuTime(struct CHvDebugTimer *p, 
					char *pcLabel, S32 nMaxSkip);
	void OutputDeltaTime(struct CHvDebugTimer *p, 
					char *pcLabel, S32 nMaxSkip);
	void StopAndAccuTime(struct CHvDebugTimer *p);
	void StartTime(struct CHvDebugTimer *p);
	void ResetTime(struct CHvDebugTimer *p);

#ifdef __cplusplus
};
#endif





