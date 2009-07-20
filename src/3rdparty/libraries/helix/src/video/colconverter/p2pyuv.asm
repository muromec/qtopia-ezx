;
; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: p2pyuv.asm,v 1.3 2005/03/07 23:36:59 gwright Exp $
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
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Packed YUV To PlanarYUV MMX converters.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; YUY2ToPlanarYUV_MMX
%ifdef   COFF
        %define YUY2ToPlanarYUV_MMX _YUY2ToPlanarYUV_MMX
        %define UYVYToPlanarYUV_MMX _UYVYToPlanarYUV_MMX
%elifdef WIN32
        %define YUY2ToPlanarYUV_MMX _YUY2ToPlanarYUV_MMX
        %define UYVYToPlanarYUV_MMX _UYVYToPlanarYUV_MMX
%elifdef ELF
        %define YUY2ToPlanarYUV_MMX YUY2ToPlanarYUV_MMX
        %define UYVYToPlanarYUV_MMX UYVYToPlanarYUV_MMX
%elifdef AOUTB
        %define YUY2ToPlanarYUV_MMX YUY2ToPlanarYUV_MMX
        %define UYVYToPlanarYUV_MMX UYVYToPlanarYUV_MMX
%else
        %error linking format currently not supported by alphbablend.asm
%endif

        ;; Export the functions implemented here.
        global YUY2ToPlanarYUV_MMX
        global UYVYToPlanarYUV_MMX

;========================= DATA SEGMENT ============================
section .data
align 8        

MaskChroma
    dd  0xFF00FF00, 0xFF00FF00
        
MaskLuma
    dd  0x00FF00FF, 0x00FF00FF
        

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
        .d1:        resd 1  ;unsigned char* d1,      
        .d2:        resd 1  ;unsigned char* d2,      
        .du:        resd 1  ;unsigned char* du,      
        .dv:        resd 1  ;unsigned char* dv,      
        .dyPitch:   resd 1  ;INT32          dyPitch, 
        .duPitch:   resd 1  ;INT32          duPitch, 
        .dvPitch:   resd 1  ;INT32          dvPitch, 
        .dest_dx:   resd 1  ;INT32          dest_dx, 
        .dest_dy:   resd 1  ;INT32          dest_dy, 
        .s1:        resd 1  ;unsigned char* s1,      
        .s2:        resd 1  ;unsigned char* s2,      
        .src_pitch: resd 1  ;INT32          src_pitch

endstruc


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Our UYVY to Planar YUV MMX
;;                        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
UYVYToPlanarYUV_MMX:
        ;; Save some stuff...
        push ebx
        push edi
        push esi
        push ebp
        push ecx
        push edx

        ; Make room for temps on stack
        sub esp, numtemps*4;

        ;; Load our dest chroma pointers.
        movd  mm4, var(parms.du)      ; mm4 = du
        movd  mm5, var(parms.dv)      ; mm5 = dv
                
        ;; Set up the loops
        mov eax, var(parms.dest_dy)
        mov ecx, var(parms.dest_dx)
        shr eax, 1                    ; eax = dest_dy/2
        
        shr ecx, 2                    ; ecx = dest_dx/4
        jnc even1                      ; Was dest_dx divisible by 4?
odd1:            
        ; We have 1 macro pixel left over
        mov DWORD var(parms.tmp3), 1  ; Store the fact that we got odd marco pixels.
        jmp cont1
even1:   
        mov DWORD var(parms.tmp3), 0  ; We have even pixels...        
cont1:           
        mov var(parms.tmp1), ecx      ; save dx loop count
        mov var(parms.tmp2), eax      ; save dx loop count
        
        xor ecx, ecx

        mov edi, var(parms.s1)        ;s1
        mov esi, var(parms.s2)        ;s2
        mov ebx, var(parms.d1)        ;d1
        mov ebp, var(parms.d2)        ;d2

        pxor  mm7, mm7                ; mm7 = 00000000 00000000
        movq  mm6, [MaskLuma]         ; mm6 = Mask out Luma value.
        xor eax, eax
DYLOOP1: 
DXLOOP1:
        ;; Process 2 macro pixels at a time, 2 lines at a time.
        movq mm0, [edi+ecx*8]      ; mm0= y4v2y3u2 y2v1y1u1
        movq mm1, [esi+ecx*8]      ; mm1= y4v2y3u2 y2v1y1u1

        ;; These two lines are the only difference between the two
        ;; YUY2 and UYVY conversion routines. I really should make
        ;; time to combine the 2 routines to save a little download
        ;; space.
        ;; Byte swap the 2 MMX registers.
        ;; s1
        movq     mm2, mm0
        movq     mm3, mm0
        psrlq    mm2, 8            ; mm2 = 00y4v2y3 u2y2v1y1
        psllq    mm3, 8            ; mm3 = v2y3u2y2 v1y1u100
        pand     mm3, [MaskChroma] ; mm3 = v200u200 v100u100
        pand     mm2, mm6          ; mm2 = 00y400y3 00y200y1
        por      mm2, mm3          ; mm2 = v2y4u2y3 v1y2u1y1
        movq     mm0, mm2

        ;; s2
        movq     mm2, mm1
        movq     mm3, mm1
        psrlq    mm2, 8            ; mm2 = 00y4v2y3 u2y2v1y1
        psllq    mm3, 8            ; mm3 = v2y3u2y2 v1y1u100
        pand     mm3, [MaskChroma] ; mm3 = v200u200 v100u100
        pand     mm2, mm6          ; mm2 = 00y400y3 00y200y1
        por      mm2, mm3          ; mm2 = v2y4u2y3 v1y2u1y1
        movq     mm1, mm2
        
        ;; Store luma values in planar YUV space
        movq     mm2, mm0
        movq     mm3, mm1
        pand     mm2, mm6          ; mm2 = 00Y400Y3 00Y200Y1 of s1
        pand     mm3, mm6          ; mm3 = 00Y400Y3 00Y200Y1 of s1

        packuswb mm2, mm7          ; mm2 = 00000000 Y4Y3Y2Y1 of s1
        packuswb mm3, mm7          ; mm3 = 00000000 Y4Y3Y2Y1 of s2

        movd     [ebx+ecx*4], mm2  ; d1=s1
        psrlw    mm0, 8            ; mm0 = 00v200u2 00v100u1 of S1
        movd     [ebp+ecx*4], mm3  ; d2=s2

        ;;Compute averaged chroma values
        psrlw      mm1, 8            ; mm1 = 00v200u2 00v100u1 of S2
        paddw      mm0, mm1          ; mm0 = v2v2u2u2 v1v1u1u1 s1+s2
        psrlw      mm0, 1            ; mm0 = 00v200u2 00v100u1 (s1+s2)/2

        ;;
        ;;Store chromas in du and dv.
        ;;
        movd mm3, esi ;save esi

        ;; unpack and pack
        packuswb   mm0, mm0          ; mm0 = 00000000 v2u2v2u1
        punpcklbw  mm0, mm0          ; mm0 = v2v2u2u2 v1v1u1u1
        movd       esi, mm4          ; esi = dU
        movq       mm1, mm0
        psrlq      mm1, 32           ; mm1 = 0000000 v2v2u2u2
        punpcklbw  mm0, mm1          ; mm0 = v2v1v2v1 u2u1u2u1

        ;; Store the U data
        movd edx, mm0                ; edx = u1u2u1u2
        psrlq mm0, 32                ; mm0 = 00000000 v1v2v1v2

        mov  WORD [esi+ecx*2], dx    ; store 2 bytes of U data.

        ;; Store the V data
        movd  esi, mm5                ; esi = dV
        movd  edx, mm0                ; ecx = v1v2v1v2
        mov   WORD [esi+ecx*2], dx    ; store 2 bytes of V data.

        movd esi, mm3
        
        ;; inc DX counter and loop
        inc  ecx
        cmp  ecx, var(parms.tmp1)
        jne  NEAR DXLOOP1
        
        ;; Now we have to check for any pixels left over if dest_dx
        ;; is not divisible by 4. Since dest_dx must be at least even
        ;; we can only have 0 or two(1 macro pixel).
        mov ecx, var(parms.tmp3)
        jz  nextline1

        ;;
        ;; Do odd marco pixel here. ===========
        ;;
        movd mm0, [edi+ecx*8]      ; mm0= 00000000 v1y2u1y1
        movd mm1, [esi+ecx*8]      ; mm1= 00000000 v1y2u1y1

        ;; Store luma values in planar YUV space
        movq     mm2, mm0
        movq     mm3, mm1
        pand     mm2, mm6          ; mm2 = 00000000 00Y100Y2 of s1
        pand     mm3, mm6          ; mm3 = 00000000 00Y100Y2 of s1

        packuswb mm2, mm7          ; mm2 = 00000000 0000Y2Y1 of s1
        packuswb mm3, mm7          ; mm3 = 00000000 0000Y2Y1 of s2

        movd     edx, mm2              ; grab lower 32 bits of mm2
        mov      WORD [ebx+ecx*4], dx  ; Just store 16 bits of ecx
        movd     edx, mm3              ; grab lower 32 bits of mm3
        mov      WORD [ebp+ecx*4], dx  ; just store 16 bits of ecx

        ;;Compute averaged chroma values
        psrlw      mm0, 8            ; mm0 = 00000000 00v100u1 of S1
        psrlw      mm1, 8            ; mm1 = 00000000 00v100u1 of S2
        paddw      mm0, mm1          ; mm0 = 00000000 v1v1u1u1 s1+s2
        psrlw      mm0, 1            ; mm0 = 00000000 00v100u1 (s1+s2)/2

        ;;
        ;;Store chromas in du and dv.
        ;;
        movd mm3, esi ;save esi
        movd esi, mm4                ; esi = dU

        ;; unpack and pack
        movd edx, mm0    ; edx = 00v100u1
        
        ;; Store the U data
        mov  [esi+ecx*2], dl         ; store 1 byte of U data.
        movd  esi, mm5                ; esi = dV

        ;; Store the V data
        shr   edx, 16                 ; edx = 000000v1
        mov   [esi+ecx*2], dl         ; store 1 byte of V data.
        movd  esi, mm3
        
nextline1:               
        ;; Add pitches for next line
        mov ecx, var(parms.src_pitch)
        shl ecx, 1                     ; ecx = src_pitch*2
        add edi, ecx                   ; s1 = s1 + src_pitch*2
        add esi, ecx                   ; s2 = s2 + src_pitch*2
        mov ecx, var(parms.dyPitch)
        shl ecx, 1                     ; ecx = dyPitch*2
        add ebx, ecx                   ; d1 = d1+ dyPitch*2
        add ebp, ecx                   ; d2 = d2+ dyPitch*2
        mov   ecx, var(parms.dvPitch)  ; ecx = dvPitch
        movd  mm3, ecx
        paddd mm4, mm3                  ; dv = dv+ dvPitch
        mov   ecx, var(parms.duPitch)    ; ecx = 
        movd  mm3, ecx
        xor   ecx, ecx
        paddd mm5, mm3                  ; du = du+ duPitch
        
        ;; inc DY counter and loop
        inc eax
        cmp eax, var(parms.tmp2)
        jne NEAR DYLOOP1

        
end1: 
        ;; Free up stack temp var.
        add esp, numtemps*4
        
        ;; Pop off the stack....
        pop edx
        pop ecx
        pop ebp
        pop esi
        pop edi
        pop ebx
        emms
        
        ;; success
        xor eax, eax
        ret





        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Our YUY2 to Planar YUV MMX
;;                        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
YUY2ToPlanarYUV_MMX:
        ;; Save some stuff...
        push ebx
        push edi
        push esi
        push ebp
        push ecx
        push edx

        ; Make room for temps on stack
        sub esp, numtemps*4;

        ;; Load our chroma pointers.
        movd  mm4, var(parms.du)      ; mm4 = du
        movd  mm5, var(parms.dv)      ; mm5 = dv

        ;; Set up the loops
        mov eax, var(parms.dest_dy)
        mov ecx, var(parms.dest_dx)
        shr eax, 1                    ; eax = dest_dy/2
        
        shr ecx, 2                    ; ecx = dest_dx/4
        jnc even                      ; Was dest_dx divisible by 4?
odd:            
        ; We have 1 macro pixel left over
        mov DWORD var(parms.tmp3), 1  ; Store the fact that we got odd marco pixels.
        jmp cont
even:   
        mov DWORD var(parms.tmp3), 0  ; We have even pixels...        
cont:           
        mov var(parms.tmp1), ecx      ; save dx loop count
        mov var(parms.tmp2), eax      ; save dx loop count
        
        xor ecx, ecx

        mov edi, var(parms.s1)        ;s1
        mov esi, var(parms.s2)        ;s2
        mov ebx, var(parms.d1)        ;d1
        mov ebp, var(parms.d2)        ;d2

        pxor  mm7, mm7                ; mm7 = 00000000 00000000
        movq  mm6, [MaskLuma]         ; mm6 = Mask out Luma value.
        xor eax, eax
DYLOOP: 
DXLOOP:
        ;; Process 2 macro pixels at a time, 2 lines at a time.
        movq mm0, [edi+ecx*8]      ; mm0= v2y4u2y3 v1y2u1y1
        movq mm1, [esi+ecx*8]      ; mm1= v2y4u2y3 v1y2u1y1
        
        ;; Store luma values in planar YUV space
        movq     mm2, mm0
        movq     mm3, mm1
        pand     mm2, mm6          ; mm2 = 00Y400Y3 00Y200Y1 of s1
        pand     mm3, mm6          ; mm3 = 00Y400Y3 00Y200Y1 of s1

        packuswb mm2, mm7          ; mm2 = 00000000 Y4Y3Y2Y1 of s1
        packuswb mm3, mm7          ; mm3 = 00000000 Y4Y3Y2Y1 of s2

        movd     [ebx+ecx*4], mm2  ; d1=s1
        psrlw      mm0, 8            ; mm0 = 00v200u2 00v100u1 of S1
        movd     [ebp+ecx*4], mm3  ; d2=s2

        ;;Compute averaged chroma values
        psrlw      mm1, 8            ; mm1 = 00v200u2 00v100u1 of S2
        paddw      mm0, mm1          ; mm0 = v2v2u2u2 v1v1u1u1 s1+s2
        psrlw      mm0, 1            ; mm0 = 00v200u2 00v100u1 (s1+s2)/2

        ;;
        ;;Store chromas in du and dv.
        ;;
        movd mm3, esi ;save esi

        ;; unpack and pack
        packuswb   mm0, mm0          ; mm0 = 00000000 v2u2v2u1
        punpcklbw  mm0, mm0          ; mm0 = v2v2u2u2 v1v1u1u1
        movd       esi, mm4          ; esi = dU
        movq       mm1, mm0
        psrlq      mm1, 32           ; mm1 = 0000000 v2v2u2u2
        punpcklbw  mm0, mm1          ; mm0 = v2v1v2v1 u2u1u2u1

        ;; Store the U data
        movd edx, mm0                ; edx = u1u2u1u2
        psrlq mm0, 32                ; mm0 = 00000000 v1v2v1v2

        mov  WORD [esi+ecx*2], dx    ; store 2 bytes of U data.

        ;; Store the V data
        movd  esi, mm5                ; esi = dV
        movd  edx, mm0                ; ecx = v1v2v1v2
        mov   WORD [esi+ecx*2], dx    ; store 2 bytes of V data.

        movd esi, mm3
        
        ;; inc DX counter and loop
        inc  ecx
        cmp  ecx, var(parms.tmp1)
        jne  DXLOOP
        
        ;; Now we have to check for any pixels left over if dest_dx
        ;; is not divisible by 4. Since dest_dx must be at least even
        ;; we can only have 0 or two(1 macro pixel).
        mov ecx, var(parms.tmp3)
        jz  nextline

        ;;
        ;; Do odd marco pixel here. ===========
        ;;
        movd mm0, [edi+ecx*8]      ; mm0= 00000000 v1y2u1y1
        movd mm1, [esi+ecx*8]      ; mm1= 00000000 v1y2u1y1

        ;; Store luma values in planar YUV space
        movq     mm2, mm0
        movq     mm3, mm1
        pand     mm2, mm6          ; mm2 = 00000000 00Y100Y2 of s1
        pand     mm3, mm6          ; mm3 = 00000000 00Y100Y2 of s1

        packuswb mm2, mm7          ; mm2 = 00000000 0000Y2Y1 of s1
        packuswb mm3, mm7          ; mm3 = 00000000 0000Y2Y1 of s2

        movd     edx, mm2              ; grab lower 32 bits of mm2
        mov      WORD [ebx+ecx*4], dx  ; Just store 16 bits of ecx
        movd     edx, mm3              ; grab lower 32 bits of mm3
        mov      WORD [ebp+ecx*4], dx  ; just store 16 bits of ecx

        ;;Compute averaged chroma values
        psrlw      mm0, 8            ; mm0 = 00000000 00v100u1 of S1
        psrlw      mm1, 8            ; mm1 = 00000000 00v100u1 of S2
        paddw      mm0, mm1          ; mm0 = 00000000 v1v1u1u1 s1+s2
        psrlw      mm0, 1            ; mm0 = 00000000 00v100u1 (s1+s2)/2

        ;;
        ;;Store chromas in du and dv.
        ;;
        movd mm3, esi ;save esi
        movd esi, mm4                ; esi = dU

        ;; unpack and pack
        movd edx, mm0    ; edx = 00v100u1
        
        ;; Store the U data
        mov  [esi+ecx*2], dl         ; store 1 byte of U data.
        movd  esi, mm5                ; esi = dV

        ;; Store the V data
        shr   edx, 16                 ; edx = 000000v1
        mov   [esi+ecx*2], dl         ; store 1 byte of V data.
        movd  esi, mm3
        
nextline:               
        ;; Add pitches for next line
        mov ecx, var(parms.src_pitch)
        shl ecx, 1                     ; ecx = src_pitch*2
        add edi, ecx                   ; s1 = s1 + src_pitch*2
        add esi, ecx                   ; s2 = s2 + src_pitch*2
        mov ecx, var(parms.dyPitch)
        shl ecx, 1                     ; ecx = dyPitch*2
        add ebx, ecx                   ; d1 = d1+ dyPitch*2
        add ebp, ecx                   ; d2 = d2+ dyPitch*2
        mov   ecx, var(parms.dvPitch)  ; ecx = dvPitch
        movd  mm3, ecx
        paddd mm4, mm3                  ; dv = dv+ dvPitch
        mov   ecx, var(parms.duPitch)    ; ecx = 
        movd  mm3, ecx
        xor   ecx, ecx
        paddd mm5, mm3                  ; du = du+ duPitch
        
        ;; inc DY counter and loop
        inc eax
        cmp eax, var(parms.tmp2)
        jne NEAR DYLOOP

        
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
        emms
        
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
version: db '$(gfw) Copyright 2001 RealNetworks Inc. Revision:1.0 $',0




















