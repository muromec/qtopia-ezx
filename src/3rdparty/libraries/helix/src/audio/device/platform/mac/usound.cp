/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: usound.cp,v 1.4 2004/07/09 18:37:36 hubbe Exp $
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

//	Sound manager utilities

#include <stdio.h>
#ifndef _MAC_MACHO
#include <Traps.h>
#endif
#include "USound.h"

#if !defined( _CARBON ) && !defined( _MAC_UNIX )
// ---------------------------------------------------------------------------
//		¥ GetTrapType [static]
// ---------------------------------------------------------------------------
//
static TrapType GetTrapType(unsigned long theTrap)

{
    if (BitAnd(theTrap, 0x0800) > 0)
        return(ToolTrap);
    else
        return(OSTrap);
    }
#endif

// ---------------------------------------------------------------------------
//		¥ TrapAvailable [static]
// ---------------------------------------------------------------------------
//
static Boolean TrapAvailable(unsigned long trap)

{
#if defined( _CARBON ) || defined( _MAC_UNIX )

    return TRUE; // any trap we're likely to check for will exist in Carbon. xxxbobclark

#else

TrapType trapType = ToolTrap;
unsigned long numToolBoxTraps;

    if (NGetTrapAddress(_InitGraf, ToolTrap) == NGetTrapAddress(0xAA6E, ToolTrap))
        numToolBoxTraps = 0x200;
    else
        numToolBoxTraps = 0x400;

    trapType = GetTrapType(trap);
    if (trapType == ToolTrap) {
        trap = BitAnd(trap, 0x07FF);
        if (trap >= numToolBoxTraps)
            trap = _Unimplemented;
        }
    return(NGetTrapAddress(trap, trapType) != NGetTrapAddress(_Unimplemented, ToolTrap));

#endif
}


short	USound::sSMVersion = -1;
short	USound::sSMMinorVersion = -1;

// ---------------------------------------------------------------------------
//		¥ CheckSMVersion [static]
// ---------------------------------------------------------------------------
//
short 
USound::CheckSMVersion (void)

	{ /* begin CheckSMVersion */
		/* Check for SM 3.0 */
		if (sSMVersion < 0) {
#ifdef _MAC_MACHO
                        NumVersion vers = SndSoundManagerVersion();
                        sSMVersion = vers.majorRev;
                        sSMMinorVersion = vers.minorAndBugRev >> 4;
#else
			if (::TrapAvailable (_SoundDispatch)) {
				NumVersion	vers = SndSoundManagerVersion ();
				sSMVersion = vers.majorRev;
				sSMMinorVersion = vers.minorAndBugRev >> 4;
				} /* if */
			else sSMVersion = 0;
#endif
			} /* if */

		return sSMVersion;
		
	} /* end CheckSMVersion */

// ---------------------------------------------------------------------------
//		¥ CheckSMVersion [static]
// ---------------------------------------------------------------------------
//
ULONG32 
USound::GetSystemSampleRate (SndChannelPtr chan)

	{ /* begin GetSystemSampleRate */
	
		ULONG32 sampleRate = 0;
		OSErr theErr = noErr;
		
		/* Check for SM 3.1 */
		CheckSMVersion();

		if (chan && (sSMVersion >= 3 && sSMMinorVersion >= 1))
		{
			theErr = ::SndGetInfo(chan, siSampleRate, &sampleRate);
			if(!theErr)
				sampleRate >>= 16;
			else
				sampleRate = 0;
		}

		return sampleRate;
		
		
	} /* end GetSystemSampleRate */

// ---------------------------------------------------------------------------
//		¥ SetVolume [static]
// ---------------------------------------------------------------------------
//
void
USound::SetVolume (
	
	short	level)
	
	{ /* begin SetVolume */
		
		long	value = level;
		
#if 0 // OLDROUTINENAMES && !GENERATINGCFM
		if (CheckSMVersion () < 3)
			::SetSoundVol (level);
		else 
#endif
			::SetDefaultOutputVolume (value | value << 16);	
	
	} /* end SetVolume */
	
// ---------------------------------------------------------------------------
//		¥ GetVolume [static]
// ---------------------------------------------------------------------------
//

short
USound::GetVolume (void)
	
	{ /* begin GetVolume */
	
		short	level;
		
#if 0 // OLDROUTINENAMES && !GENERATINGCFM
		if (CheckSMVersion () < 3)
			::GetSoundVol (&level);
			
		else 
#endif
			{
			long	value;
			::GetDefaultOutputVolume (&value);
			level = value & 0x0FFFF;
			} /* else */
			
		return level;
		
	} /* end GetVolume */
	
// ---------------------------------------------------------------------------
//		¥ GetSoundHeaderOffset [static]
// ---------------------------------------------------------------------------
//
#pragma options align=mac68k

typedef struct Snd1Header {
	short	format;								/* format of resource */
	short	numSynths;							/* number of data types */
												/* synths, init option follow */
	} Snd1Header, *Snd1HdrPtr;
	
typedef struct Snd2Header {					/* format 2 'snd ' resource header */
	short	format;								/* format of resource */
	short	refCount;							/* for application use */
	} Snd2Header, *Snd2HdrPtr;

#pragma options align=reset

typedef short		*IntPtr;					/* for type coercion */
typedef SndCommand	*SndCmdPtr;					/* for type coercion */

typedef char	*ORD4;

OSErr 
USound::GetSoundHeaderOffset (
	
	SndListHandle	sndHdl,
	long			*offset)
	
	{  /* begin GetSoundHeaderOffset */
		
		if (CheckSMVersion () > 2) 
			return ::GetSoundHeaderOffset (sndHdl, offset);
			
		/* Fake it */
		Ptr		myPtr;								/* to navigate resource */
		long	myOffset;							/* offset into resource */
		short	numSynths;							/* info about resource */
		short	numCmds;							/* info about resource */
		Boolean	isDone;								/* are we done yet? */
		OSErr	myErr;

		/* Initialize variables. */
		myOffset = 0;													/* return 0 if no sound header found */
		myPtr = Ptr(*sndHdl);													/* point to start of resource data */
		isDone = FALSE;													/* haven't yet found sound header */
		myErr = noErr;
	
		/* Skip everything before sound commands. */
		switch (Snd1HdrPtr (myPtr)->format ) {
			case firstSoundFormat:												/* format 1 'snd ' resource */
															/* skip header start, synth ID, etc. */
				numSynths = Snd1HdrPtr(myPtr)->numSynths;
				myPtr += sizeof (Snd1Header);
				myPtr += numSynths * (sizeof (short) + sizeof (long));
				break;
	
			case secondSoundFormat:												/* format 2 'snd ' resource */
				myPtr += sizeof (Snd2Header);
				break;
				
			default:												/* unrecognized resource format */
				myErr = badFormat;
				isDone = TRUE;
				break;
			} /* switch */
	
		/* Find number of commands and move to start of first command. */
		numCmds = *IntPtr(myPtr);
		myPtr += sizeof (short);
	
		/* Search for bufferCmd or soundCmd to obtain sound header. */
		while ( (numCmds >= 1) && (! isDone) ) {
			short	cmd = *IntPtr(myPtr);
			if ((cmd == short (bufferCmd | dataOffsetFlag)) ||
				(cmd == short (soundCmd  | dataOffsetFlag))) {
															/* bufferCmd or soundCmd found */
															/* copy offset from sound command */
				myOffset = SndCmdPtr(myPtr)->param2;
				isDone = TRUE;											/* get out of loop */
				} /* if */
			
			else {												/* soundCmd or bufferCmd not found */
															/* move to next command */
				myPtr += sizeof (SndCommand);
				numCmds = numCmds - 1;
				} /* else */
			} /* while */
	
		*offset = myOffset;													/* return offset */
		
		return myErr;													/* return result code */
	
	} /* end GetSoundHeaderOffset */
	
