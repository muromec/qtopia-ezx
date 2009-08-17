/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hvdebtim.cpp,v 1.3 2004/07/09 18:31:58 hubbe Exp $
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

/////////////////////////////////////////////////////////////////////////////
// CVvDebugTimer --
/* How to use it: 

in C (here local creation of struct)

#include "hvdebtim.h"

struct CVvDebugTimer * pDebTim = newCVvDebugTimer();

ResetTime(pDebTim);//reset accu
StartTime(pDebTim);


Debugee();

StopAndAccuTime(pDebTim);
//AverageAndOutputAccuTime(pDebTim, "Resamp@22 kHz", 1);
OutputAccuTime(pDebTim, "Resamp@22 kHz", 1);




in C++:

#include "hvdebtim.h"

CVvDebugTimer DebTim01;
CVvDebugTimer DebTim02;
CVvDebugTimer DebTim03;

	DebTim01.StartTime();

	Debugee();
	
	DebTim01.StopAndAccuTime();

	//DebTim01.AverageAndOutputAccuTime("VvDecode", 1);
	DebTim01.OutputAccuTime("VvDecode  ", 1);
	DebTim01.ResetTime();//reset accu


*/


#include <stdio.h>
#include "hvutils.h"
#include "hvdebtim.h"
#include "hxstrutl.h"

/********************************/
/* time helper functions		*/
extern "C" {

#if TIMERTYPE!=WIN31_TIME
static LARGE_INTEGER	liFrequency = {0};
static S32				nFrequencyValid = FREQUENCY_NOTMEASURED;
#endif

/********************************/
void VvGetAbsCpuClocks(__int64 *time)
{
//	_asm RdTsc	;	Get count of cycles into EDX: EAX
_asm {
	_emit 0x0F	//0F 31 is RDTSC opcode: Reads Pentium CPU clock (costs only 6 clocks at Ring 0, 11 at Ring 3)
	_emit 0x31	//  into EDX:EAX
	mov ebx, time
	mov [ebx], eax;//Intel Little Endian
	mov [ebx+4], edx
	}
}

/********************************/
void VvZeroAbsCpuClocks(void)	//Force Pentium clock to 0  (Can be done at Ring 0 only)
{
	_asm{
		mov ECX, 0x10	//Machine-specific register 10h is the Time Stamp Counter
		xor EAX,EAX		//Timestamp we want to set goes
		xor EDX,EDX		//  into EDX:EAX
		_emit 0x0F	    //WRMSR opcode is 0F 30
		_emit 0x30
	}
	
}
/********************************/
#define TOLERANCE		1		// Number of MHz to allow
								//   samplings to deviate from
								//   average of samplings.
								//   Initially set to 2.
#define ROUND_THRESHOLD		6


U32	 GetRDTSCCpuSpeed()
{
	unsigned long in_cycles=0;	// Internal clock cycles during
								//   test
	unsigned long ex_ticks=0;		// Microseconds elapsed during 
								//   test
	unsigned long raw_freq=0;		// Raw frequency of CPU in MHz
	unsigned long norm_freq=0;	// Normalized frequency of CPU

	LARGE_INTEGER t0,t1;			// Variables for High-
									//   Resolution Performance
									//   Counter reads

	unsigned long  freq  =0;			// Most current frequ. calculation
	unsigned long  freq2 =0;			// 2nd most current frequ. calc.
	unsigned long  freq3 =0;			// 3rd most current frequ. calc.
	
	unsigned long  total;			// Sum of previous three frequency
							//   calculations

	int tries=0;			// Number of times a calculation has
							//   been made on this call to 
							//   cpuspeed

	unsigned long   total_cycles=0, cycles;	// Clock cycles elapsed 
									//   during test
	
	unsigned long   stamp0, stamp1;			// Time Stamp Variable 
									//   for beginning and end 
									//   of test

	unsigned long   total_ticks=0, ticks;	// Microseconds elapsed 
									//   during test
	
	LARGE_INTEGER count_freq;		// High Resolution 
									//   Performance Counter 
									//   frequency

#ifdef WIN32
	int iPriority;
	HANDLE hThread = GetCurrentThread();
#endif // WIN32;


	if ( !QueryPerformanceFrequency ( &count_freq ) ) 
		return raw_freq;//return 0
	
	// On processors supporting the Read 
	//   Time Stamp opcode, compare elapsed
	//   time on the High-Resolution Counter
	//   with elapsed cycles on the Time 
	//   Stamp Register.
	
	do {			// This do loop runs up to 20 times or
	   				//   until the average of the previous 
	   				//   three calculated frequencies is 
	   				//   within 1 MHz of each of the 
	   				//   individual calculated frequencies. 
					//   This resampling increases the 
					//   accuracy of the results since
					//   outside factors could affect this
					//   calculation
			
		tries++;		// Increment number of times sampled
						//   on this call to cpuspeed
			
		freq3 = freq2;	// Shift frequencies back to make
		freq2 = freq;	//   room for new frequency 
						//   measurement

    	QueryPerformanceCounter(&t0);	
    					// Get high-resolution performance 
    					//   counter time
			
		t1.LowPart = t0.LowPart;		// Set Initial time
		t1.HighPart = t0.HighPart;

#ifdef WIN32
		iPriority = GetThreadPriority(hThread);
		if ( iPriority != THREAD_PRIORITY_ERROR_RETURN )	{
			SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
		}
#endif // WIN32

   		while ( (unsigned long)t1.LowPart - (unsigned long)t0.LowPart<50) {	  
   						// Loop until 50 ticks have 
   						//   passed	since last read of hi-
						//	 res counter. This accounts for
						//   overhead later.

			QueryPerformanceCounter(&t1);

		__asm {
			_emit 0x0F	//0F 31 is RDTSC opcode: Reads Pentium CPU clock (costs only 6 clocks at Ring 0, 11 at Ring 3)
			_emit 0x31	//  into EDX:EAX
			MOV stamp0, EAX
			}
		}
			
			
		t0.LowPart = t1.LowPart;		// Reset Initial 
		t0.HighPart = t1.HighPart;		//   Time

   		while ((unsigned long)t1.LowPart-(unsigned long)t0.LowPart<1000 ) {
   						// Loop until 1000 ticks have 
   						//   passed	since last read of hi-
   						//   res counter. This allows for
						//   elapsed time for sampling.
   			
				
   			QueryPerformanceCounter(&t1);
   			

		__asm {
			_emit 0x0F	//0F 31 is RDTSC opcode: Reads Pentium CPU clock (costs only 6 clocks at Ring 0, 11 at Ring 3)
			_emit 0x31	//  into EDX:EAX
			MOV stamp1, EAX
			}
		}

			

#ifdef WIN32
		// Reset priority
		if ( iPriority != THREAD_PRIORITY_ERROR_RETURN )	{
			SetThreadPriority(hThread, iPriority);
		}
#endif // WIN32

       	cycles = stamp1 - stamp0;	// Number of internal 
        							//   clock cycles is 
        							//   difference between 
        							//   two time stamp 
        							//   readings.

    	ticks = (unsigned long) t1.LowPart - (unsigned long) t0.LowPart;	
								// Number of external ticks is
								//   difference between two
								//   hi-res counter reads.
	

		// Note that some seemingly arbitrary mulitplies and
		//   divides are done below. This is to maintain a 
		//   high level of precision without truncating the 
		//   most significant data. According to what value 
		//   ITERATIIONS is set to, these multiplies and
		//   divides might need to be shifted for optimal
		//   precision.

		ticks = ticks * 100000;	
							// Convert ticks to hundred
							//   thousandths of a tick
			
		ticks = ticks / ( count_freq.LowPart/10 );		
							// Hundred Thousandths of a 
							//   Ticks / ( 10 ticks/second )
							//   = microseconds (us)

		total_ticks += ticks;
		total_cycles += cycles;

		if ( ticks%count_freq.LowPart > count_freq.LowPart/2 )
			ticks++;			// Round up if necessary
			
		freq = cycles/ticks;	// Cycles / us  = MHz
        										
     	if ( cycles%ticks > ticks/2 )
       		freq++;				// Round up if necessary
          	
		total = ( freq + freq2 + freq3 );
							// Total last three frequency 
							//   calculations

	} while ( (tries < 3 ) || 		
	          (tries < 20)&&
	          ((abs(3 * freq -total) > 3*TOLERANCE )||
	           (abs(3 * freq2-total) > 3*TOLERANCE )||
	           (abs(3 * freq3-total) > 3*TOLERANCE )));	
					// Compare last three calculations to 
	          		//   average of last three calculations.		

	// Try one more significant digit.
	freq3 = ( total_cycles * 10 ) / total_ticks;
	freq2 = ( total_cycles * 100 ) / total_ticks;


	if ( freq2 - (freq3 * 10) >= ROUND_THRESHOLD )
		freq3++;

	raw_freq = total_cycles / total_ticks;
	norm_freq = raw_freq;

	freq = raw_freq * 10;
	if( (freq3 - freq) >= ROUND_THRESHOLD )
		norm_freq++;

	ex_ticks = total_ticks;
	in_cycles = total_cycles;

	return raw_freq*1000000;
}


/********************************/
void MeasureFrequency()
{
	nFrequencyValid = FREQUENCY_VALID;
#if TIMERTYPE==WIN31_TIME
	liFrequency.QuadPart = 1000;//Ticks per second
#elif TIMERTYPE==WIN95_TIME
	LARGE_INTEGER liPerfFreq;
	if(QueryPerformanceFrequency(&liPerfFreq)) {
		liFrequency = liPerfFreq.QuadPart;
	} else {
		nFrequencyValid = FREQUENCY_NOTVALID;
	}
#elif TIMERTYPE==PENT_TIME
	liFrequency.QuadPart = GetRDTSCCpuSpeed();
	if(liFrequency.QuadPart==0) {
		nFrequencyValid = FREQUENCY_NOTVALID;
	}
#endif
}


}


/********************************/
/* this is for cpp				*/


	CVvDebugTimer::CVvDebugTimer() {
		ResetTime();
		qwAverageAccuCount = 0;
		nSkipToNextOutput = 0;
		nDeltaHistoryPtr = 0;
		if(nFrequencyValid==FREQUENCY_NOTMEASURED) {
			MeasureFrequency();
		}
	}


	void CVvDebugTimer::ResetTime() {
		qwAccuCount = 0;		
		qwDeltaCount = 0;
		
	}

	/*************************/
	void CVvDebugTimer::StartTime() {
#if TIMERTYPE==WIN31_TIME
		dwStartCount = GetTickCount();
#elif TIMERTYPE==WIN95_TIME
		QueryPerformanceCounter(&liStartCount);
#elif TIMERTYPE==PENT_TIME
		VvGetAbsCpuClocks(&(liStartCount.QuadPart));
#endif		

	}
    
	/*************************/
	void CVvDebugTimer::StopAndAccuTime() {
#if TIMERTYPE==WIN31_TIME
		dwStopCount = GetTickCount();
		qwDeltaCount = (__int64) dwStopCount - (__int64) dwStartCount;
#else
	#if TIMERTYPE==WIN95_TIME
		QueryPerformanceCounter(&liStopCount);
	#elif TIMERTYPE==PENT_TIME
		VvGetAbsCpuClocks(&(liStopCount.QuadPart));
	#endif
		qwDeltaCount =	((__int64)liStopCount.QuadPart  
						-(__int64)liStartCount.QuadPart);

	#if TIMERTYPE==PENT_TIME
		/* magig number, assume that function calls etc. cost 160 cycles */
		qwDeltaCount -= 160;
	#endif
#endif		
		qwDeltaHistory[nDeltaHistoryPtr] = qwDeltaCount;
		nDeltaHistoryPtr++;
		if(nDeltaHistoryPtr >= NBDELTASHISTORY) 
			nDeltaHistoryPtr = 0;

		qwAccuCount += qwDeltaCount;
	}

	
	/*************************/
	void CVvDebugTimer::OutputDeltaTime(char *pcLabel, S32 nMaxSkip) {
		char string[80]; /* Flawfinder: ignore */
		nSkipToNextOutput++;//only every nMaxSkip time 
		if(nSkipToNextOutput >= nMaxSkip) {
			nSkipToNextOutput = 0;
#if TIMERTYPE==WIN31_TIME
				SafeSprintf(string, 80,"W31%s: %7.2lf ms\n", pcLabel, (double) qwDeltaCount);
#else
			if(nFrequencyValid!=FREQUENCY_VALID) {
				SafeSprintf(string, 80,"%s: %7.2lf ms\n", pcLabel, 
					1000.*(double)qwDeltaCount/(double) liFrequency.QuadPart);
			} else {
				SafeSprintf(string, 80,"%s: Frequency not valid\n", pcLabel);
			}
#endif
			OutputDebugString(string);
		}
	}

	/*************************/
	int CVvDebugTimer::GetAccuTime(double *ptime) {
		int result;

#if TIMERTYPE==WIN31_TIME
		*time = (double)qwAccuCount;
		result = 1
#else
		if(nFrequencyValid == FREQUENCY_VALID) {
			*ptime = 1000.*(double)qwAccuCount/((double) liFrequency.QuadPart);
			result = 2;
		} else {
			*ptime = 0;
			result = 0;
		}
#endif
		return(result);

	}
	
	
	/*************************/
	void CVvDebugTimer::OutputAccuTime(char *pcLabel, S32 nMaxSkip) {

		// we run through it only every nMaxSkip times
		char string[80]; /* Flawfinder: ignore */
		nSkipToNextOutput++;//only every nMaxSkip time 
		if(nSkipToNextOutput >= nMaxSkip) {
			nSkipToNextOutput = 0;
			double time;
			int result;
			result = GetAccuTime(&time);

			if(result==1) {
				SafeSprintf(string, 80, "W31%s: %7.2lf ms\n", pcLabel, time);
			} else if(result==2) {
				SafeSprintf(string, 80, "%s: %7.2lf ms\n", pcLabel, time);
			} else {
				SafeSprintf(string, 80,"%s: QueryPerformanceFrequency not supported\n", pcLabel);
			}
			OutputDebugString(string);
		}
	}

	/*************************/
	void CVvDebugTimer::LogAccuTime(char *pcLabel, S32 nMaxSkip) {

		// we run through it only every nMaxSkip times
		char string[80]; /* Flawfinder: ignore */
		nSkipToNextOutput++;//only every nMaxSkip time 
		if(nSkipToNextOutput >= nMaxSkip) {
			nSkipToNextOutput = 0;
#if TIMERTYPE==WIN31_TIME
			SafeSprintf(string, 80, "W31%s: %7.2lf ms\n", pcLabel, 
				(double)qwAccuCount);
#else
			if(nFrequencyValid == FREQUENCY_VALID) {
				SafeSprintf(string, 80,"%s: %7.2lf ms\n", pcLabel, 
					1000.*(double)qwAccuCount/((double) liFrequency.QuadPart));
			} else {
				SafeSprintf(string, 80,"%s: QueryPerformanceFrequency not supported\n", pcLabel);
			}
#endif

			dprintf(string);

		}
	}

#define VVDEBUGOUT(pcOut, string, nbMaxChar)  \
	if(pcOut) { (strncpy(pcOut, string, nbMaxChar)); } else {(OutputDebugString(string));} /* Flawfinder: ignore */

	/*************************/
	void CVvDebugTimer::AverageAndOutputAccuTime(char *pcLabel, S32 nMaxSkip, 
												 char *pcOut=0, S32 nbMaxChar=0) {

		// we run through it only every nMaxSkip times
		char string[800]; /* Flawfinder: ignore */

		qwAverageAccuCount += qwAccuCount;

		nSkipToNextOutput++;//only every nMaxSkip time 
		if(nSkipToNextOutput >= nMaxSkip) {
#if TIMERTYPE==WIN31_TIME
			SafeSprintf(string, 800, "W31%s: %7.2lf ms\n", pcLabel, 
				(double)qwAverageAccuCount/(double) nSkipToNextOutput);
#else

			if(nFrequencyValid == FREQUENCY_VALID) {
				S32 ii;
				char appendix[20]; /* Flawfinder: ignore */
				sprintf(string, "("); /* Flawfinder: ignore */
				for(ii=0; ii<nDeltaHistoryPtr; ii++) {
					SafeSprintf(appendix, 20,"%3.1lf ", 
								1000.*(double)qwDeltaHistory[ii]/
						((double) liFrequency.QuadPart));
					SafeStrCat(string,  appendix, 800);
				}
				SafeStrCat(string,  ")\n", 800);

				VVDEBUGOUT(pcOut, string, nbMaxChar);

				SafeSprintf(string, 800,"%s: %7.2lf ms\n", pcLabel, 
					1000.*(double)qwAverageAccuCount
					/((double) liFrequency.LowPart + 4294967296.*liFrequency.HighPart)
					/(double) nSkipToNextOutput);

				VVDEBUGOUT(pcOut, string, nbMaxChar);

			} else {
				SafeSprintf(string, 800,"%s: QueryPerformanceFrequency not supported\n", pcLabel);
				VVDEBUGOUT(pcOut, string, nbMaxChar);
			}
#endif

			qwAverageAccuCount = 0;
			nSkipToNextOutput = 0;
			nDeltaHistoryPtr = 0;

		}
	}



/********************************/
/* this is for c interface		*/
extern "C" {


    //  external member functions
	void ResetTime(CVvDebugTimer *p) {
		p->qwAccuCount = 0;		
		p->qwDeltaCount = 0;
		
	}

	// ctors, dtors, etc.
    CVvDebugTimer * newCVvDebugTimer () {
		CVvDebugTimer *Tim; 
		Tim = new CVvDebugTimer;

		Tim->ResetTime();

		Tim->qwAverageAccuCount = 0;
		Tim->nSkipToNextOutput = 0;

		if(nFrequencyValid==FREQUENCY_NOTMEASURED) {
			MeasureFrequency();
		}
		return Tim;
	}

	void AverageAndOutputAccuTime(CVvDebugTimer *p, char *pcLabel, S32 nMaxSkip) {
		if(p) 
			p->AverageAndOutputAccuTime(pcLabel, nMaxSkip, 0, 0);
	}

	void AverageAndOutputAccuTimeString(CVvDebugTimer *p, char *pcLabel, S32 nMaxSkip,
										char *pcOut=0, S32 nbMaxChar=0) {
		if(p) 
			p->AverageAndOutputAccuTime(pcLabel, nMaxSkip, pcOut, nbMaxChar);
	}

	int	GetAccuTime(CVvDebugTimer *p, double *ptime) {
		int result=0;
		*ptime = 0.;
		if(p) 
			result = p->GetAccuTime(ptime);
		return(result);
	}

	void OutputAccuTime(CVvDebugTimer *p, char *pcLabel, S32 nMaxSkip) {
		if(p) 
			p->OutputAccuTime(pcLabel, nMaxSkip);
	}

	void LogAccuTime(CVvDebugTimer *p, char *pcLabel, S32 nMaxSkip) {
		if(p) 
			p->LogAccuTime(pcLabel, nMaxSkip);
	}

	void OutputDeltaTime(CVvDebugTimer *p, char *pcLabel, S32 nMaxSkip) {
		if(p) 
			p->OutputDeltaTime(pcLabel, nMaxSkip);
	}
	
	void StopAndAccuTime(CVvDebugTimer *p) {
		if(p) 
			p->StopAndAccuTime();
	}
	void StartTime(CVvDebugTimer *p) {
		if(p) 
			p->StartTime();
	}


}

