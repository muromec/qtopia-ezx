/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: env.c,v 1.8 2007/07/06 20:53:51 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#define  ENV_MAIN   /* declare CPU-related variables here */
#include "env.h"    /* standard libraries & #defines */
#include "mmx_util.h"

#ifdef _MACINTOSH
#ifndef _MAC_MACHO
#include <gestalt.h>
#endif
#endif

/*
 * Check if current CPU supports Intel MMX/SSE instruction sets.
 * Use:
 *  int CheckCPUType (void);
 * Returns:
 *  !0 - MMX instruction set is supported
 *  0 - non-MMX or non-IA CPU.
 */ 
int CheckCPUType (void) 
{
#if defined(_M_IX86)  && !defined(WINCE_EMULATOR)
   _x86_MMX_Available = (checkMmxAvailablity()&CPU_HAS_MMX)?1:0;
   return _x86_MMX_Available;
#elif (defined(_MACINTOSH) || defined(_MAC_UNIX)) && !defined(__i386__)
	// Check for presence of AltiVec on Macintosh
	OSErr	err;
	long	processorAttributes;

	err = Gestalt(gestaltPowerPCProcessorFeatures, &processorAttributes);
	if (err == noErr)
		_AltiVec_Available = (1 << gestaltPowerPCHasVectorInstructions) & processorAttributes;
   return _AltiVec_Available;
#else
   return 0;
#endif
}

/* env.c -- end of file */
