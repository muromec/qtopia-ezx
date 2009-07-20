;
; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: y2touy.asm,v 1.4 2007/07/06 20:53:51 jfinnecy Exp $
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
; terms of the GNU General Public License Version 2 (the
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
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  YUY2 to UYVY MMX converter.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%ifdef   COFF
        %define YUY2ToUYVY_MMX _YUY2ToUYVY_MMX
%elifdef WIN32
        %define YUY2ToUYVY_MMX _YUY2ToUYVY_MMX
%elifdef ELF
        %define YUY2ToUYVY_MMX YUY2ToUYVY_MMX
%elifdef AOUTB
        %define YUY2ToUYVY_MMX YUY2ToUYVY_MMX
%else
        %error linking format currently not supported by alphbablend.asm
%endif

        ;; Export the functions implemented here.
        global YUY2ToUYVY_MMX

;========================= DATA SEGMENT ============================
section .data
align 8        

;============================= CODE SEGMENT =======================                        
section .text


;;
;; This is our stack params definition. It is used for both
;; YUY2 and UYVY routines as they both take the same parms.
;;                                        
%assign numtemps 4
%define var(a) [esp+a]       
                
struc parms
        ;Temps on stack
        .tmp1       resd 1  ;General DWORD temp.
        .tmp2       resd 1  ;General DWORD temp.
        .tmp3       resd 1  ;General DWORD temp.
        .tmp4       resd 1  ;General DWORD temp.
        
        ; Space for reg pushes and return address.
        .registers  resd 6  ;pushed registers
        .return     resd 1  ;return address

        ; input params
        .src:       resd 1  ;unsigned char*,      
        .dest:      resd 1  ;unsigned char*,      
        .dx:        resd 1  ;ULONG32,      

endstruc


YUY2ToUYVY_MMX:
        ;; Save some stuff...
        push ebx
        push edi
        push esi
        push ebp
        push ecx
        push edx

        ; Make room for temps on stack
        sub esp, numtemps*4;

        ;; Grab source and dest pointers...
        mov esi, var(parms.src)
        mov edi, var(parms.dest)

        ;; Preload our address add constant
        mov edx, 8
     
        mov ecx, var(parms.dx)
        shr ecx, 3              ; we do 4 macro pixels at a time.
        jnc loop1               ; Do we have a width that is a multiple of 8?
        ;; We have 2 macro pixels left over. Do them here
        
        movq  mm0, [esi]        ; grab 2 macro pixels from source
        movq  mm1, mm0          ; mm1=mm0= VVYYUUYY VVYYUUYY 
        psllw mm1, 8            ; mm1 = YY00YY00 YY00YY00
        psrlw mm0, 8            ; mm0 = 00VV00UU 00VV00UU
        por   mm0, mm1          ; mm0 = YYVVYYUU YYVVYYUU
        movq  [edi], mm0        ; store it.
        add   esi, edx
        add   edi, edx

        ;; This loops does 16 bytes at a time (4 macro pixels).
loop1:   
        movq  mm0, [esi]        ; grab 2 macro pixels from source
        add esi, edx

        movq  mm1, mm0          ; mm1=mm0= VVYYUUYY VVYYUUYY 
        psllw mm1, 8            ; mm1 = YY00YY00 YY00YY00
        psrlw mm0, 8            ; mm0 = 00VV00UU 00VV00UU
        movq  mm2, [esi]        ; grab 2 macro pixels from source

        por   mm0, mm1          ; mm0 = YYVVYYUU YYVVYYUU
        movq  mm3, mm2          ; mm1=mm0= VVYYUUYY VVYYUUYY 
        
        movq  [edi], mm0        ; store it.

        add edi, edx
        psllw mm3, 8            ; mm1 = YY00YY00 YY00YY00

        psrlw mm2, 8            ; mm0 = 00VV00UU 00VV00UU
        por   mm2, mm3          ; mm0 = YYVVYYUU YYVVYYUU
        add esi, edx
        
        movq  [edi], mm2        ; store it.
                
        ;; Now do the loop calcs....
        add edi, edx
        dec ecx
        jnz loop1
        
end: 
        ;; Free up stack temp var.
        add esp, numtemps*4
        
        ;; Pop off the stack....
        pop edx
        pop ecx
        pop ebp
        pop esi
        pop edi
        pop ebx
        ;; This emms is expensive. If we don't do it we wipe out the
        ;; floating ponts registers but that should be ok.
;         emms 
        
        ;; success
        xor eax, eax
        ret

%ifdef ELF
;;
;; If we do not have this section here then the GNU
;; tools will believe we need an executable stack and
;; mark the DLL as such. 
section .note.GNU-stack
previous        
%endif

;;; leave a trace
version: db '$(gfw) Helix Revision:1.0 $',0




















