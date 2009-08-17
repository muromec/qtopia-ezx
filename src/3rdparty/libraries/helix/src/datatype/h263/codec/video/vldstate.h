/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vldstate.h,v 1.2 2004/07/09 18:32:17 hubbe Exp $
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
 
#include "h263plus.h"

#ifndef _INC_VLDSTATE
#define _INC_VLDSTATE	1

/* Define states for bitstream decoder */
#define ST_DIFF_QUANT         (-4)  /* Decrease state by 4 after decoding QUANT */
#define ST_DIFF_ESC_LEVEL     (-28) /* Decrease state by 28 after decoding ESC_LEVEL */
#define ST_MC_NOCBP_MVDX        1
#define ST_MC_NOCBP_MVDY        2
#define ST_MBA_STARTCODE        3
#define ST_NEXT_BLK_6           4
#define ST_FIRST_BLK_6          5
#define ST_NEXT_BLK_5           6
#define ST_FIRST_BLK_5          7
#define ST_NEXT_BLK_4           8
#define ST_FIRST_BLK_4          9
#define ST_NEXT_BLK_3           10
#define ST_FIRST_BLK_3          11
#define ST_NEXT_BLK_2           12
#define ST_FIRST_BLK_2          13
#define ST_NEXT_BLK_1           14
#define ST_FIRST_BLK_1          15
#define ST_INTRA_DC_BLK_6       16
#define ST_INTRA_AC_BLK_5       17
#define ST_INTRA_DC_BLK_5       18
#define ST_INTRA_AC_BLK_4       19
#define ST_INTRA_DC_BLK_4       20
#define ST_INTRA_AC_BLK_3       21
#define ST_INTRA_DC_BLK_3       22
#define ST_INTRA_AC_BLK_2       23
#define ST_INTRA_DC_BLK_2       24
#define ST_INTRA_AC_BLK_1       25
#define ST_INTRA_DC_BLK_1       26
#define ST_MC_CBP_MVDX          27
#define ST_MC_CBP_MVDY          28
#define ST_CBP                  29
#define ST_INTRA_MQUANT         (ST_INTRA_DC_BLK_1 - ST_DIFF_QUANT)
#define ST_MC_CBP_MQUANT        (ST_MC_CBP_MVDX - ST_DIFF_QUANT)
#define ST_ESC_BLK_6            (ST_NEXT_BLK_6 - ST_DIFF_ESC_LEVEL)
#define ST_INTER_MQUANT         (ST_CBP - ST_DIFF_QUANT)
#define ST_ESC_BLK_5            (ST_NEXT_BLK_5 - ST_DIFF_ESC_LEVEL)
#define ST_MTYPE                35
#define ST_ESC_BLK_4            (ST_NEXT_BLK_4 - ST_DIFF_ESC_LEVEL)
#define ST_GEI_PEI              37
#define ST_ESC_BLK_3            (ST_NEXT_BLK_3 - ST_DIFF_ESC_LEVEL)
#define ST_PTYPE                39
#define ST_ESC_BLK_2            (ST_NEXT_BLK_2 - ST_DIFF_ESC_LEVEL)
#define ST_GQUANT               41
#define ST_ESC_BLK_1            (ST_NEXT_BLK_1 - ST_DIFF_ESC_LEVEL)
#define ST_TR                   43
#define ST_AFTER_STARTCODE      44
#define ST_ESC_INTRA_5          (ST_INTRA_AC_BLK_5 - ST_DIFF_ESC_LEVEL)
                                /* State 46 is not used */
#define ST_ESC_INTRA_4          (ST_INTRA_AC_BLK_4 - ST_DIFF_ESC_LEVEL)
                                /* State 48 is not used */
#define ST_ESC_INTRA_3          (ST_INTRA_AC_BLK_3 - ST_DIFF_ESC_LEVEL)
                                /* State 50 is not used */
#define ST_ESC_INTRA_2          (ST_INTRA_AC_BLK_2 - ST_DIFF_ESC_LEVEL)
                                /* State 52 is not used */
#define ST_ESC_INTRA_1          (ST_INTRA_AC_BLK_1 - ST_DIFF_ESC_LEVEL)

// Definitions for H.263
#define ST263_BASE              (54)
#define ST263_DIFF_ESC_LEVEL    (-1)    // Decrease state by 1 after decoding ESC_LEVEL
#define ST263_DIFF_INTRA_DC     (-14)   // Decrease state by 14 after decoding INTRA-DC
#define ST263_DIFF_LAST         (-2)    // Decrease state by 2 after decoding TCOEF with LAST=1

#define ST263_FINISHED          (ST263_BASE + 0)
#define ST263_ESC_FINISHED      (ST263_FINISHED - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_6             (ST263_FINISHED - ST263_DIFF_LAST)
#define ST263_ESC_BLK6          (ST263_BLK_6 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_5             (ST263_BLK_6 - ST263_DIFF_LAST)
#define ST263_ESC_BLK5          (ST263_BLK_5 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_4             (ST263_BLK_5 - ST263_DIFF_LAST)
#define ST263_ESC_BLK4          (ST263_BLK_4 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_3             (ST263_BLK_4 - ST263_DIFF_LAST)
#define ST263_ESC_BLK3          (ST263_BLK_3 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_2             (ST263_BLK_3 - ST263_DIFF_LAST)
#define ST263_ESC_BLK2          (ST263_BLK_2 - ST263_DIFF_ESC_LEVEL)
#define ST263_BLK_1             (ST263_BLK_2 - ST263_DIFF_LAST)
#define ST263_ESC_BLK1          (ST263_BLK_1 - ST263_DIFF_ESC_LEVEL)
#define ST263_INTRA_DC_ONLY     (ST263_FINISHED - ST263_DIFF_INTRA_DC)
                                // State 69 is not used
#define ST263_INTRA_DC_AC       (ST263_BLK_6 - ST263_DIFF_INTRA_DC)

#ifdef DO_H263_PLUS

#define ST263PLUS_BASE              (ST263_INTRA_DC_AC + 1)

#define ST263PLUS_FINISHED          (ST263PLUS_BASE + 0)
#define ST263PLUS_ESC_FINISHED      (ST263PLUS_FINISHED - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_6             (ST263PLUS_FINISHED - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK6          (ST263PLUS_BLK_6 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_5             (ST263PLUS_BLK_6 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK5          (ST263PLUS_BLK_5 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_4             (ST263PLUS_BLK_5 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK4          (ST263PLUS_BLK_4 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_3             (ST263PLUS_BLK_4 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK3          (ST263PLUS_BLK_3 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_2             (ST263PLUS_BLK_3 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK2          (ST263PLUS_BLK_2 - ST263_DIFF_ESC_LEVEL)
#define ST263PLUS_BLK_1             (ST263PLUS_BLK_2 - ST263_DIFF_LAST)
#define ST263PLUS_ESC_BLK1          (ST263PLUS_BLK_1 - ST263_DIFF_ESC_LEVEL)

#define NUMSTATES               (ST263PLUS_BASE + 15)

#else
#define NUMSTATES               (ST263_BASE + 17)
#endif
#endif
