/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/*
 *
 *	CINTERL.h
 *
 *	A class to handle interleaving and error correcting basic RealAudio file types
 *	
 *	We currently use two different algorithms to interleave files for transfer over the Net. 
 *	
 *	INT3 is used with RA_FORMAT_3 files. RA_FORMAT_3 files are stored in non-interleaved
 *  format and are sent over the NET interleaved using the INT3 algorithm.
 *
 *  INT4 is used with RA_FORMAT_4 files. RA_FORMAT_4 files are stored in interleaved
 *  format and are sent over the NET interleaved using the INT4 algorithm. RA_FORMAT_4
 *  have have the first several blocks of data sent non-interleaved.
 *
 *
 */

#ifdef __MWERKS__
#pragma once
#endif


#ifndef	_CINTERL
#define _CINTERL

#include "hxtypes.h"
#include "hxresult.h"

#define RA_NO_INTERLEAVER         "Int0"         // no interleaver for this file
#define RA_INTERLEAVER_3_ID       "Int3"         // interleaver used for version 3 format
#define RA_INTERLEAVER_4_ID       "Int4"         // interleaver used for version 4 format
#define RA_INTERLEAVER_5_ID       "Int5"         // interleaver used for 05_6 codec
#define RA_INTERLEAVER_6_ID       "sipr"         // interleaver used for SIPRO codec
#define RA_INTERLEAVER_GENERIC_ID "genr"	 // generic interleaver
#define RA_INTERLEAVER_VBRS_ID    "vbrs"	 // Simple VBR interleaver
#define RA_INTERLEAVER_VBRF_ID    "vbrf"	 // Simple VBR interleaver (with possibly fragmenting)
#define MAX_INTER_LENGTH          5              // max length of interleave name

#define INTERPROP_INPLACE 0x01

class CInterleave 
{

public:

// Construct creates the correct instance of a CInterleave object depending
// on the requested interleaverName. If an error occurs it will be returned
// in the error and Construct will return a NULL pointer. If successful,
// Construct will return a pointer to a CInterleave object and set 
// theErr == HX_NO_ERROR

   static   CInterleave* Construct          (char *interleaverName, HX_RESULT *theErr, void *pParam);
   
   virtual               ~CInterleave      	(void);


	virtual HX_RESULT	Init				(UINT16 *frameSize,
										     UINT16 *blockSize, 
										     UINT16 *interleaveFactor,
											 UINT16 *pattern) = 0;

	virtual	HX_RESULT	Interleave			(unsigned char *inBuf,unsigned char *outBuf) = 0;

   virtual   HX_RESULT   Deinterleave      	(unsigned char    *InBuffer,
                                  UINT16       InLength,
                                  unsigned char    *OutBuffer,
                                  UINT16       *pOutLength,
                                  ULONG32      *present = 0) = 0;

   virtual   HX_RESULT    ErrorCorrection      	(ULONG32      granularity,
                                   ULONG32      *present,
                                   ULONG32      interleaveStartPos,
                                    char        *inData,
                                   char         *outData,
                                   INT32         *outLength,
                                   ULONG32      *received,
                                   UINT16       *lengths,
                                   UINT16       interleaveFactor,
                                   UINT16       period,
                                   UINT16       blockSize) = 0;

    virtual   HX_RESULT  ErrorCorrection2 (Byte* inData,
				    UINT16 InLength,
				    Byte* outData,
				    UINT16* pOutLength,
				    ULONG32* received,
				    ULONG32 ulGranularity) = 0;

    virtual   ULONG32 GetProperties( void );

    virtual   void    SetVBRDataSize(UINT32 ulSize) {};

protected:
   // constructor is protected. Use Construct to create the correct instance of CInterleave
                  CInterleave         (void);

         UINT16      mBlockSize;
         UINT16      mInterleaveFactor;
         ULONG32   mOutSuperBlockSize;
         ULONG32   mInSuperBlockSize;
};

#endif // _CHXDATAFILE_H_      




