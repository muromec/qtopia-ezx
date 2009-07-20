; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: spin.s,v 1.3 2004/07/09 18:22:57 hubbe Exp $
; 
; Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
; 
; The contents of this file, and the files included with this file,
; are subject to the current version of the RealNetworks Public
; Source License (the "RPSL") available at
; http://www.helixcommunity.org/content/rpsl unless you have licensed
; the file under the current version of the RealNetworks Community
; Source License (the "RCSL") available at
; http://www.helixcommunity.org/content/rcsl, in which case the RCSL
; will apply. You may also obtain the license terms directly from
; RealNetworks.  You may not use this file except in compliance with
; the RPSL or, if you have a valid RCSL with RealNetworks applicable
; to this file, the RCSL.  Please see the applicable RPSL or RCSL for
; the rights, obligations and limitations governing use of the
; contents of the file.
; 
; Alternatively, the contents of this file may be used under the
; terms of the GNU General Public License Version 2 or later (the
; "GPL") in which case the provisions of the GPL are applicable
; instead of those above. If you wish to allow use of your version of
; this file only under the terms of the GPL, and not to allow others
; to use your version of this file under the terms of either the RPSL
; or RCSL, indicate your decision by deleting the provisions above
; and replace them with the notice and other provisions required by
; the GPL. If you do not delete the provisions above, a recipient may
; use your version of this file under the terms of any one of the
; RPSL, the RCSL or the GPL.
; 
; This file is part of the Helix DNA Technology. RealNetworks is the
; developer of the Original Code and owns the copyrights in the
; portions it created.
; 
; This file, and the files included with this file, is distributed
; and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
; KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
; ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
; OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
; ENJOYMENT OR NON-INFRINGEMENT.
; 
; Technology Compatibility Kit Test Suite(s) Location:
;    http://www.helixcommunity.org/content/tck
; 
; Contributor(s):
; 
; ***** END LICENSE BLOCK *****
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;  spin.s:  Example assembly language routine for spinlock support.
;
;
; XXXDC -- Since the caller has aligned the pointer I simplified this
; routine by eliminating this code.  The pointer must be 16-byte aligned
; for a 32-bit environment.
;
; Also, couldn't we eliminate the nop's?  This is only needed for
; old hardware.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    .code
;   .level 2.0W                 ; use this option for 64-bit assembly
    .export load_and_clear,entry,priv_lev=3,rtnval=gr
    .proc
load_and_clear
    .callinfo no_calls
    .enter

; create a 16 byte aligned pointer to the load+clear word area
    addi        15,%arg0,%arg2  ; add 15 to pointer provided to round up

; Choose one of these statements and comment out the other:
    depi        0,31,4,%arg2    ; mask off the lower 4 bits (32-bit version)
;;   depdi       0,63,4,%arg2    ; mask off the lower 4 bits (64-bit version)

		
; load and clear the spinlock.  If locked, return 0
    stbys,e     0,(%arg2)       ; scrub cache; important for performance
    ldcws       (%arg2),%ret0   ; load and clear the spinlock word
    nop                         ; 3 No-Op instructions; needed for older
    nop                         ; HP-PA chips
    nop
    bv,n        (%r2)
    .leave
    .procend

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;OLD OLD OLD 
;;
; create a 16 byte aligned pointer to the load+clear word area
;    addi        15,%arg0,%arg2  ; add 15 to pointer provided to round up
;
; Choose one of these statements and comment out the other:
;    depi        0,31,4,%arg2    ; mask off the lower 4 bits (32-bit version)
;;   depdi       0,63,4,%arg2    ; mask off the lower 4 bits (64-bit version)
;
; load and clear the spinlock.  If locked, return 0
;    stbys,e     0,(%arg2)       ; scrub cache; important for performance
;    ldcws       (%arg2),%ret0   ; load and clear the spinlock word
;    nop                         ; 3 No-Op instructions; needed for older
;    nop                         ; HP-PA chips
;    nop
;    bv,n        (%r2)
;    .leave
;    .procend
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;





