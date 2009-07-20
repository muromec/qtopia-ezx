;
; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: mmxlines.asm,v 1.4 2007/07/06 20:53:51 jfinnecy Exp $
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
;;  _MMX_lineI420toYUY2 and _MMX_lineI420toUYVY
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; YUY2ToPlanarYUV_MMX
%ifdef   COFF
        %define _MMX_lineI420toYUY2 __MMX_lineI420toYUY2
        %define _MMX_lineI420toUYVY __MMX_lineI420toUYVY
%elifdef WIN32
        %define _MMX_lineI420toYUY2 __MMX_lineI420toYUY2
        %define _MMX_lineI420toUYVY __MMX_lineI420toUYVY
%elifdef ELF
        %define _MMX_lineI420toYUY2 _MMX_lineI420toYUY2
        %define _MMX_lineI420toUYVY _MMX_lineI420toUYVY
%elifdef AOUTB
        %define _MMX_lineI420toYUY2 _MMX_lineI420toYUY2
        %define _MMX_lineI420toUYVY _MMX_lineI420toUYVY
%else
        %error linking format currently not supported by _MMX_lineI420to*
%endif

        ;; Export the functions implemented here.
        global _MMX_lineI420toYUY2
        global _MMX_lineI420toUYVY

;========================= DATA SEGMENT ============================
section .data
align 8        


;============================= CODE SEGMENT ========================                  
section .text

;;
;; This is our stack params definition. It is used for both 
;; YUY2 and UYVY functions.
;;                                        
%define var(a) [esp+a]       
                
struc parms
        ; Space for reg pushes and return address.
        .registers  resd 6  ;pushed registers
        .return     resd 1  ;return address

        ; input params
        .sy:        resd 1  ;unsigned char* sy,      
        .su:        resd 1  ;unsigned char* su,      
        .sv:        resd 1  ;unsigned char* sv,      
        .d:         resd 1  ;unsigned char* d,      
        .count:     resd 1  ;int            count
endstruc


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; I420 to YUY2...
;;                        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_MMX_lineI420toYUY2:
        ;; Save some stuff...
        push ebx
        push edi
        push esi
        push ebp
        push ecx
        push edx

;          load frame pointers
        mov     esi, var(parms.sy)
        mov     ebx, var(parms.su)
        mov     ecx, var(parms.sv)
        mov     edi, var(parms.d)

;          get # of pixels:  
        mov     ebp, var(parms.count)

;          copy misaligned pixels first: 
        sub     ebp, 2
        jl      cont_1
loop_1:
        test    edi,15
        jz      cont_1

        xor     eax, eax
        mov     al, [esi]      ; sy[0] 
        xor     edx, edx
        mov     dl, [esi+1]    ; sy[1] 
        mov     ah, [ebx]      ; su[0] 
        mov     dh, [ecx]      ; sv[0] 
        lea     esi, [esi+2]
        shl     edx, 16
        lea     ebx, [ebx+1]
        or      eax, edx
        lea     ecx, [ecx+1]
        mov    dword  [edi], eax
        lea     edi, [edi+4]

        sub     ebp, 2
        jge     loop_1

;          process main bulk of data: 
cont_1:
        sub     ebp,16-2
        jl      cont_2
loop_2:
;          load 8 bytes from each plane 
        movq    mm0, [esi]    ; mm0: y7..y0 
        movq    mm1, [ebx]    ; mm1: u7..u0 
        movq    mm2, [ecx]    ; mm2: v7..v0 

;          process first 8 pixels: 
        movq      mm3, mm1
        punpcklbw mm3, mm2      ; mm3: v3,u3,v2,u2,v1,u1,v0,u0 
        movq      mm4, mm0
        punpcklbw mm4, mm3      ; mm4: v1,y3,u1,y2,v0,y1,u0,y0 
        movq      [edi], mm4
        psrlq     mm3, 32
        psrlq     mm0, 32
        punpcklbw mm0, mm3      ; mm0: v3,y7,u3,y6,v2,y5,u2,y4 
        movq      [edi+8], mm0

;          process another 8 pixels: 
        movq      mm0, [esi+8]  ; mm0: y15..y8 
        psrlq     mm1, 32
        psrlq     mm2, 32
        movq      mm3, mm1
        punpcklbw mm3, mm2
        movq      mm4, mm0
        punpcklbw mm4, mm3
        movq      [edi+16], mm4
        psrlq     mm3, 32
        psrlq     mm0, 32
        punpcklbw mm0, mm3
        movq      [edi+24], mm0

        lea       esi, [esi+16]       ; sy 
        lea       ebx, [ebx+8]        ; su 
        lea       ecx, [ecx+8]        ; sv 
        lea       edi, [edi+32]       ; d 
  
        sub       ebp, 16
        jge       loop_2

;          copy the remaining pixels: 
cont_2:
        sub       ebp,2-16
        jl        done
loop_3:  
        xor       eax, eax
        mov       al, [esi]      ; sy[0] 
        xor       edx, edx
        mov       dl, [esi+1]    ; sy[1] 
        mov       ah, [ebx]      ; su[0] 
        mov       dh, [ecx]      ; sv[0] 
        lea       esi, [esi+2]
        shl       edx, 16
        lea       ebx, [ebx+1]
        or        eax, edx
        lea       ecx, [ecx+1]
        mov       dword [edi], eax
        lea       edi, [edi+4]
  
        sub       ebp, 2
        jge       loop_3
                
done:   
        ;; Pop off the stack....
        pop edx
        pop ecx
        pop ebp
        pop esi
        pop edi
        pop ebx

;;; No emms on purpose....
        
        ;; success
        xor eax, eax
        ret



        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; _MMX_lineI420toUYVY
;;                        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_MMX_lineI420toUYVY:
        
        ;; Save some stuff...
        push ebx
        push edi
        push esi
        push ebp
        push ecx
        push edx


        mov     esi, var(parms.sy)
        mov     ebx, var(parms.su)
        mov     ecx, var(parms.sv)
        mov     edi, var(parms.d)

        mov     ebp, var(parms.count)

        ; copy misaligned pixels first
        sub     ebp, 2
        jl      cont_11
loop_11:
        test    edi,15
        jz      cont_11

        xor     eax, eax
        mov     al, byte [ebx]  ; su
        xor     edx, edx
        mov     dl, byte [ecx]  ; sv
        mov     ah, byte [esi]  ; sy
        lea     ebx, [ebx+1]
        mov     dh, byte [esi+1]
        lea     ecx, [ecx+1]
        shl     edx, 16
        lea     esi, [esi+2]
        or      eax, edx
        sub     ebp, 2
        mov     dword [edi], eax
        lea     edi, [edi+4]
        jge     loop_11
        
        ; process main bulk of data
cont_11:
        sub     ebp,16-2
        jl      cont_22
loop_22:
        ; load chroma pixels
        movq    mm1, qword [ebx]    ; mm1: u7..u0 
        movq    mm2, qword [ecx]    ; mm2: v7..v0 

        ; process first 8 pixels
        movq      mm3, mm1
        punpcklbw mm3, mm2          ; mm3: v3,u3,v2,u2,v1,u1,v0,u0 
        movq      mm0, qword [esi]  ; mm0: y7,y6,y5,y4,y3,y2,y1,y0 
        movq      mm4, mm3
        punpcklbw mm4, mm0          ; m4: y3,v1,y2,u1,y1,v0,y0,u0 
        movq      qword [edi], mm4
        psrlq     mm3, 32
        psrlq     mm0, 32
        punpcklbw mm3, mm0          ; mm3: y7,v3,y6,u3,y5,v2,y4,u2 
        movq      qword [edi+8], mm3

        ; process another 8 pixels
        psrlq     mm1, 32
        psrlq     mm2, 32
        movq      mm3, mm1
        punpcklbw mm3, mm2
        movq      mm0, qword [esi+8]  ; mm0: y15..y8 
        movq      mm4, mm3
        punpcklbw mm4, mm0
        movq      qword [edi+16], mm4
        psrlq     mm3, 32
        psrlq     mm0, 32
        punpcklbw mm3, mm0
        movq      qword [edi+24], mm3

        lea       esi, [esi+16]       ;sy 
        lea       ebx, [ebx+8]        ;su 
        lea       ecx, [ecx+8]        ;sv 
        lea       edi, [edi+32]       ;d  
  
        sub       ebp, 16
        jge       loop_22

        ; copy the remaining pixels
cont_22:
        sub       ebp,2-16
        jl        done2
loop_32:  
        xor       eax, eax
        mov       al, byte [ebx]      ;su[0]
        xor       edx, edx
        mov       dl, byte [ecx]      ;sv[0] 
        mov       ah, byte [esi]      ;sy[0] 
        lea       ebx, [ebx+1]
        mov       dh, byte [esi+1]    ;sy[1] 
        lea       ecx, [ecx+1]
        shl       edx, 16
        lea       esi, [esi+2]
        or        eax, edx
        sub       ebp, 2
        mov       dword [edi], eax
        lea       edi, [edi+4]
        jge       loop_32

        
done2:
        ;; Pop off the stack....
        pop edx
        pop ecx
        pop ebp
        pop esi
        pop edi
        pop ebx

;;; No emms on purpose....
        
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


;;; Ident.
version: db '$(cl13n7c0r3) Helix Revision:1.0 $',0
