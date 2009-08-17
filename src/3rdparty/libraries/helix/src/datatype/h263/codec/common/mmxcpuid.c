/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: mmxcpuid.c,v 1.2 2004/07/09 18:31:58 hubbe Exp $
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

#include "mmxcpuid.h"

/* GLOBALS */
long g_mmx_checked = CPUID_NOTINIT;
long g_mmx_detected = CPUID_MMX_OFF;

/* externs */
long VvGetPrefPrivateProfileInt(char *, long);


#ifdef COMPILE_MMX
#pragma message("Compiling MMX code, CPU detection enabled with cpuid_init()")
#endif

/*----------------------------------------------------------------------------*/ 
long __stdcall mmxsupport()
{
	int mmx_supported = 0;
	int *pmmx_supported = &mmx_supported;
	//NOTE: IMPORTANT
	//MSVC 5.0 Compiler with optimization "maximize speed" will obviously 
	// try to minimize registers saved for _asm blocks
	// but it does not have any way to tell that there is a CPUID instruction and that this instruction 
	// will affect eax, ebx, ecx, edx. 
	// be sure to use all the registers in the _asm block so that the compiler will save them before entering

	_asm {
		pushfd					//Save Eflag to stack
		pop eax					//Get Eflag from stack into eax
		mov ecx, eax			//Make another copy of Eflag in ecx
		xor eax, 0x200000		//Toggle ID bit in Eflag [i.e. bit(21)] 
		push eax				//Save modified Eflag back to stack

		popfd					//Restore modified value back to Eflag reg 
		pushfd					//Save Eflag to stack
		pop eax					//Get Eflag from stack
		xor eax, ecx			//Compare the new Eflag with the original Eflag
		jz NOT_SUPPORTED		//If the same, CPUID instruction is not supported,
								//skip following instructions and jump to
								//NOT_SUPPORTED label

		xor eax, eax			//Set eax to zero
					
		_asm _emit 0x0f			//CPUID instruction  (two bytes opcode)
		_asm _emit 0xa2 
		
		cmp eax, 1				//make sure eax return non-zero value
		jl NOT_SUPPORTED		//If eax is zero, mmx not supported

		xor eax, eax			//set eax to zero
		inc eax					//Now increment eax to 1.  This instruction is 
								//faster than the instruction "mov eax, 1"
		
		_asm _emit 0x0f			//CPUID instruction
		_asm _emit 0xa2 

		and edx, 0x00800000		//mask out all bits but mmx bit(24)
		cmp edx, 0				// 0 = mmx not supported
		jz	NOT_SUPPORTED		// non-zero = Yes, mmx IS supported

		mov ebx, pmmx_supported
		mov	[ebx], 1	//set return value to 1

NOT_SUPPORTED:
		nop
		//mov	eax, mmx_supported	//move return value to eax	

	}
	return mmx_supported;		
}



/*----------------------------------------------------------------------------*/ 
void cpuid_init() 
{
	long mmx = 0;
	
	long temp;//only word needed

	//mask all unnecessary floating point exceptions 
	//on fresh reboot, denorm is not masked at this point
	__asm {
		push	eax
		fclex			;//clear exceptions
		fstcw	temp;
		mov		eax, temp
		or		eax, 0x000f
		mov		temp, eax
		fldcw	temp;
		pop		eax
	}
	
	//VIVOGEN_BITRATE_BASE-style in vdefine.h would not work for .ini files
#if defined(_WIN32) || defined(_LINUX)
	g_mmx_detected = 1;
#else
	g_mmx_detected = 0;
#endif
	if(g_mmx_detected != 0) {//enabled, find out if we are on a MMX processor
		mmx = mmxsupport();
		if(mmx == 0) {//turned off because not on a MMX processor
			g_mmx_detected = CPUID_MMX_OFF;
		}
	} else {//turned off in ini file or registry
		g_mmx_detected = CPUID_MMX_OFF;
	}

	g_mmx_checked = CPUID_VALID;
}

