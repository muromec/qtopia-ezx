/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: unwind.h,v 1.2 2003/01/23 23:42:52 damonlan Exp $ 
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



#ifndef UNWIND_HEADER_FILE
#define UNWIND_HEADER_FILE

typedef struct cframe_info {
  unsigned long cur_frsize; /* frame size */
  unsigned long cursp; /* stack pointer */
#ifndef __LP64__
  unsigned long currls; /* PC-space of CALLING routine */
#endif
  unsigned long currlo; /* PC-offset of CALLING routine */
  unsigned long curdp; /* data pointer */
  unsigned long toprp; /* return pointer */
  unsigned long topmrp; /* millicode return pointer */
#ifndef __LP64__
  unsigned long topsr0; /* sr0 */
  unsigned long topsr4; /* sr4 */
#endif
  unsigned long r3; /* gr3 */
#ifndef __LP64__
  unsigned long cur_r19; /* linkage-table pointer (gr19) - for PIC code */
#endif
  unsigned long r4; /* gr4 */ /* Added for alloca unwind interface */
#ifdef __LP64__
  unsigned long reserved[4];
#else
  unsigned long reserved;     /* Added for alloca unwind interface */
#endif
} cframe_info;

typedef struct pframe_info {
  unsigned long prev_frsize; /* frame size */
  unsigned long prevsp; /* stack pointer */
#ifndef __LP64__
  unsigned long prevrls; /* PC-space of CALLING routine */
#endif
  unsigned long prevrlo; /* PC-offset of CALLING routine */
  unsigned long prevdp; /* data pointer */
  unsigned int udescr0; /* first half of unwind descriptor */
  unsigned int udescr1; /* second half of unwind descriptor */
  unsigned int ustart; /* start of the unwind region */
  unsigned int uend; /* end of the unwind region */
  unsigned long uw_index; /* index into the unwind table */
#ifndef __LP64__
  unsigned long prev_r19; /* linkage-table pointer (gr19) - for PIC code */
#endif
  unsigned long PFIinitR3;  /* Added for alloca unwind interface */
  unsigned long PFIinitR4;  /* Added for alloca unwind interface */
#ifdef __LP64__
  unsigned long reserved[3];
#endif
} pframe_info;

extern int U_get_previous_frame_x(struct cframe_info*, struct pframe_info*, int size);
extern void U_prep_frame_rec_for_unwind(struct cframe_info*);
extern void U_init_frame_record(struct cframe_info*);

#ifdef __LP64__
void U_copy_frame_info(struct cframe_info *current, struct pframe_info *previous);
#endif

#endif /* UNWIND_HEADER_FILE */
