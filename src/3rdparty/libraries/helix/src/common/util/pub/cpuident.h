/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cpuident.h,v 1.7 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _CPUIDENT_H_
#define _CPUIDENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CPU architectures. Currently, only Intel architectures (x86) and ARM are
 * detected by this code.
 */

typedef enum
{
    ulArchitectureUNKNOWN = 0,
    ulArchitectureIntel   = 1,
    ulArchitectureARM_ARCH2 = 2,
    ulArchitectureARM_ARCH3 = 3,
    ulArchitectureARM_ARCH4 = 4,
    ulArchitectureARM_ARCH4T= 5,
    ulArchitectureARM_ARCH5 = 6,
    ulArchitectureARM_ARCH5T= 7,
    ulArchitectureARM_ARCH5TE=8
} CPUArchitecture;

/*
 * Intel Architecture processor families.
 * Do not change these values -- they match with the results of the cpuid()
 * instruction.
 * Refer to Intel document number 24319202, section 8.1.3.
 */

typedef enum
{
    ulFamilyUNKNOWN     =  0,
    ulFamily80386       =  3,
    ulFamily80486       =  4,
    ulFamilyPENTIUM     =  5,
    ulFamilyPENTIUM_PRO =  6,
    ulFamilyP4          =  15
} IntelCPUFamily;

/*
 * ARM architecture families
 */

typedef enum
{
    ulFamily_PREARM7 = ulFamilyUNKNOWN + 1,
    ulFamily_ARM7,
    ulFamily_POSTARM7
} ARMCPUFamily;

/*
 * Intel architecture specific features
 */
typedef struct
{
    IntelCPUFamily family ;
    int model ;
    int stepping ;

    int hasMMX; /* MMX instructions available? */
    int hasMSR; /* model specific registers (Pentium) available? */
    int hasTSC; /* rdtsc / Time stamp counter available? */
    int hasPMC; /* rdpmc / Performance monitoring counter available? */
    int hasSSE; /* Katmai new instructions (SSE) available? */
    int hasSSE_OSSupport; /* OS support for SSE floating point available? */
    int hasSSE2; /* SSE2 instructions available? */
} IntelCPUInformation ;

typedef struct
{
    int family ;
    //    int architecture ;
    int implementor ;
    char description[128] ;

    // only valid if family >= ARM7
    int variant ;
    int primaryPartNo ;
    int revision ;

    union
    {
        struct {
            int coreGeneration ;
            int coreRevision ;
            int productNumber ;
	} XScale ;
    } specific ;
} ARMCPUInformation ;

/*
 * ARM Implementors
 */

enum {
    ARM_IMPL_UNKNOWN = 0x00,
    ARM_IMPL_ARM     = 0x41, /* 'A' */
    ARM_IMPL_DEC     = 0x44, /* 'D' */
    ARM_IMPL_Intel   = 0x69  /* 'i' */
} ;

/*
 * CPUInformation
 */

typedef struct
{
    CPUArchitecture architecture ;
    union
    {
        IntelCPUInformation m_x86 ;
        ARMCPUInformation m_arm ;
    } specific ;
} CPUInformation ;

extern void CPUIdentify(CPUInformation* pInfo) ;

#ifdef __cplusplus
} // extern "C" {}
#endif

#endif /* _CPUIDENT_H_ */
