;
; ***** BEGIN LICENSE BLOCK *****
; Source last modified: $Id: yuvammx.asm,v 1.3 2005/03/07 23:36:59 gwright Exp $
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

;;--------------------------------------------
;; yuvammx.asm   
;;
;;  mmx alpha blender routines
;;
;;  NASM 0.98
;;  target machine = Pentium II
;----------------------------------------------------------
; Note:  Output buffer can be the same as one of the input
;        buffers for I420 and YV12 output only.  Common 
;        input/output buffer must have same pitch, lines,
;        width and height.
;=========================================================
;; data segment should be qword aligned for best performance

%define ALIGNMENT

%ifdef   COFF
    %define _I420andYUVAtoI420_MMX        _I420andYUVAtoI420_MMX        
    %define _I420andYUVAtoYV12_MMX        _I420andYUVAtoYV12_MMX        
    %define _I420andYUVAtoYUY2_MMX        _I420andYUVAtoYUY2_MMX        
    %define _I420andYUVAtoUYVY_MMX        _I420andYUVAtoUYVY_MMX        
    %define _I420andI420toI420_MMX_sub        _I420andI420toI420_MMX_sub        
    %error data segment possibly not qword aligned
%elifdef WIN32
    %define _I420andYUVAtoI420_MMX        _I420andYUVAtoI420_MMX        
    %define _I420andYUVAtoYV12_MMX        _I420andYUVAtoYV12_MMX        
    %define _I420andYUVAtoYUY2_MMX        _I420andYUVAtoYUY2_MMX        
    %define _I420andYUVAtoUYVY_MMX        _I420andYUVAtoUYVY_MMX        
    %define _I420andI420toI420_MMX_sub        _I420andI420toI420_MMX_sub        
    %define ALIGNMENT align=8
%elifdef ELF
    %define _I420andYUVAtoI420_MMX        I420andYUVAtoI420_MMX        
    %define _I420andYUVAtoYV12_MMX        I420andYUVAtoYV12_MMX        
    %define _I420andYUVAtoYUY2_MMX        I420andYUVAtoYUY2_MMX        
    %define _I420andYUVAtoUYVY_MMX        I420andYUVAtoUYVY_MMX        
    %define _I420andI420toI420_MMX_sub        I420andI420toI420_MMX_sub        
    %error data segment possibly not qword aligned
%elifdef AOUTB
    %define _I420andYUVAtoI420_MMX        I420andYUVAtoI420_MMX        
    %define _I420andYUVAtoYV12_MMX        I420andYUVAtoYV12_MMX        
    %define _I420andYUVAtoYUY2_MMX        I420andYUVAtoYUY2_MMX        
    %define _I420andYUVAtoUYVY_MMX        I420andYUVAtoUYVY_MMX        
    %define _I420andI420toI420_MMX_sub        I420andI420toI420_MMX_sub        
    %error data segment possibly not qword aligned
%else
        %error linking format currently not supported 
%endif



global _I420andYUVAtoI420_MMX
global _I420andYUVAtoYV12_MMX
global _I420andYUVAtoYUY2_MMX 
global _I420andYUVAtoUYVY_MMX 
global _I420andI420toI420_MMX_sub 

%assign CID_I420   0   ;;/* planar YCrCb 4:2:0 format (CCIR)     */
%assign CID_YV12   1   ;;/* planar YVU 4:2:0 (ATI)               */
%assign CID_YUY2   3   ;;/* packed YVU 4:2:2 (ATI,MATROX,etc.)   */
%assign CID_UYVY   4   ;;/* yet another packed 4:2:2 (ATI)       */

%macro  make_labels 1
%define exit       exit%1
%define fail_exit  fail_exit%1
%define a100       a100%1
%define a200       a200%1
%define a300       a300%1
%define a400       a400%1
%define y100       y100%1
%define two_pels   two_pels%1
%define line_done  line_done%1
%define y_by_fours y_by_fours%1
%define y_one_two_three y_one_two_three%1
%endmacro


;=========================================================
segment .data data ALIGNMENT

align 8

mask3 
    db  -1, 0, -1, 0, -1, 0, -1, 0
mask3b 
    db  0, -1, 0, -1, 0, -1, 0, -1

con1  
    dw 1, 1, 1, 1

;;_DATA ENDS
;=========================================================
;=========================================================
segment .text code 
;;==========================================================
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;	I420andYUVAtoYV12
;;
;;	This function alpha-blends two I420 buffers into a third
;;	YV12 buffer using the alpha info tacked to the 
;;	end of the second I420 buffer
;;
;;  yuva = top
;;  inverted alpha
;;  uv size computed as: uvpitch*uvlines = (pitch/2)*(lines/2)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_I420andYUVAtoYV12_MMX:
;;
;; int I420andYUVAtoYV12_MMX(
;;          unsigned char* src, int src_pels, int src_lines, int src_pitch
;;          int src_startx, int src_starty;
;;          unsigned char* yuva, int yuva_pels, int yuva yuva_lines, int yuva_pitch
;;          int yuva_startx, int yuva_starty;
;;          unsigned char* dst, int dst_pels, int dst_lines, int dst_pitch,
;;          int dst_startx, int dst_starty,
;;          int width,  int height);
;;
;; arguments
%define src            dword [esp+4*(1+npush)]
%define src_pels       dword [esp+4*(2+npush)]
%define src_lines      dword [esp+4*(3+npush)]
%define src_pitch      dword [esp+4*(4+npush)]
%define src_startx     dword [esp+4*(5+npush)]
%define src_starty     dword [esp+4*(6+npush)]

%define yuva           dword [esp+4*(7+npush)]
%define yuva_pels      dword [esp+4*(8+npush)]
%define yuva_lines     dword [esp+4*(9+npush)]
%define yuva_pitch     dword [esp+4*(10+npush)]
%define yuva_startx    dword [esp+4*(11+npush)]
%define yuva_starty    dword [esp+4*(12+npush)]

%define dst            dword [esp+4*(13+npush)]
%define dst_pels       dword [esp+4*(14+npush)]
%define dst_lines      dword [esp+4*(15+npush)]
%define dst_pitch      dword [esp+4*(16+npush)]
%define dst_startx     dword [esp+4*(17+npush)]
%define dst_starty     dword [esp+4*(18+npush)]

%define width          dword [esp+4*(19+npush)]
%define height         dword [esp+4*(20+npush)]

    push    ebp
    push    esi
    push    edi
    push    ecx
    push    ebx

;; tmp on stack
%assign ntmps 16
%assign npush (5+ntmps)
    sub esp, ntmps*4 

%define yuvay        dword [esp + 0*4]
%define yuvau        dword [esp + 1*4]
%define yuvaoffsetv  dword [esp + 2*4]
%define yuvaa        dword [esp + 3*4]
%define yuvaauv      dword [esp + 4*4]
%define yuvauvpitch  dword [esp + 5*4]

%define sy           dword [esp + 6*4]
%define su           dword [esp + 7*4]
%define soffsetv     dword [esp + 8*4]
%define suvpitch     dword [esp + 9*4]

%define dy           dword [esp + 10*4]
%define du           dword [esp + 11*4]
%define doffsetv     dword [esp + 12*4]
%define duvpitch     dword [esp + 13*4]

%define dtmp0        dword [esp + 14*4]
%define dtmp1        dword [esp + 15*4]


;; YV12
;;    [ Y Y ]
;;      [V]
;;      [U]
        
;; YV12 u v reversed from I420
;; compute reversed order pointers 
;; then comtinue with I420 routine

;;-----
    mov eax, dst_lines
    mov ecx, dst_pitch
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines
    shr ecx, 1              ;; pitch/2
    mov duvpitch, ecx
    mov esi, dst
    mov dy, esi
    add esi, eax

    shr edx, 1              ;;  lines/2
    imul    edx, ecx        ;; (pitch/2)*(lines/2)

    add esi, edx
    mov du, esi
    neg edx         ;; negative offset from u
    mov doffsetv, edx
;;-----
    jmp I420andYUVAtoI420_MMX_entry



;_I420andYUVAtoYV12_MMX endp
;=============================================================
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;	I420andYUVAtoI420
;;
;;	This function alpha-blends two I420 buffers into a third
;;	I420 buffer using the alpha info tacked to the 
;;	end of the second I420 buffer
;;
;;  yuva = top
;;  inverted alpha
;;  uv size computed as: uvpitch*uvlines = (pitch/2)*(lines/2)
;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_I420andYUVAtoI420_MMX:
;;
;; int I420andYUVAtoI420_MMX(
;;          unsigned char* src, int src_pels, int src_lines, int src_pitch
;;          int src_startx, int src_starty;
;;          unsigned char* yuva, int yuva_pels, int yuva yuva_lines, int yuva_pitch
;;          int yuva_startx, int yuva_starty;
;;          unsigned char* dst, int dst_pels, int dst_lines, int dst_pitch,
;;          int dst_startx, int dst_starty,
;;          int width,  int height);
;;
;%define exit      _I420andYUVAtoI420_MMX_exit
;%define fail_exit _I420andYUVAtoI420_MMX_fail_exit
;%define a100      _I420andYUVAtoI420_MMX_a100
;%define a200      _I420andYUVAtoI420_MMX_a200
;%define a300      _I420andYUVAtoI420_MMX_a300
;%define a400      _I420andYUVAtoI420_MMX_a400

make_labels _I420andYUVAtoI420_MMX

;; arguments
%define src            dword [esp+4*(1+npush)]
%define src_pels       dword [esp+4*(2+npush)]
%define src_lines      dword [esp+4*(3+npush)]
%define src_pitch      dword [esp+4*(4+npush)]
%define src_startx     dword [esp+4*(5+npush)]
%define src_starty     dword [esp+4*(6+npush)]

%define yuva           dword [esp+4*(7+npush)]
%define yuva_pels      dword [esp+4*(8+npush)]
%define yuva_lines     dword [esp+4*(9+npush)]
%define yuva_pitch     dword [esp+4*(10+npush)]
%define yuva_startx    dword [esp+4*(11+npush)]
%define yuva_starty    dword [esp+4*(12+npush)]

%define dst            dword [esp+4*(13+npush)]
%define dst_pels       dword [esp+4*(14+npush)]
%define dst_lines      dword [esp+4*(15+npush)]
%define dst_pitch      dword [esp+4*(16+npush)]
%define dst_startx     dword [esp+4*(17+npush)]
%define dst_starty     dword [esp+4*(18+npush)]

%define width          dword [esp+4*(19+npush)]
%define height         dword [esp+4*(20+npush)]

    push    ebp
    push    esi
    push    edi
    push    ecx
    push    ebx

;; tmp on stack
%assign ntmps 16
%assign npush (5+ntmps)
    sub esp, ntmps*4 

%define yuvay        dword [esp + 0*4]
%define yuvau        dword [esp + 1*4]
%define yuvaoffsetv  dword [esp + 2*4]
%define yuvaa        dword [esp + 3*4]
%define yuvaauv      dword [esp + 4*4]
%define yuvauvpitch  dword [esp + 5*4]

%define sy           dword [esp + 6*4]
%define su           dword [esp + 7*4]
%define soffsetv     dword [esp + 8*4]
%define suvpitch     dword [esp + 9*4]

%define dy           dword [esp + 10*4]
%define du           dword [esp + 11*4]
%define doffsetv     dword [esp + 12*4]
%define duvpitch     dword [esp + 13*4]

%define dtmp0        dword [esp + 14*4]
%define dtmp1        dword [esp + 15*4]

%define btmp0(x)     byte [esp + 14*4 + x]
%define btmp1(x)     byte [esp + 15*4 + x]
;;-----
    mov eax, dst_lines
    mov ecx, dst_pitch
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines
    shr ecx, 1              ;; pitch/2
    mov duvpitch, ecx
    mov esi, dst
    mov dy, esi
    add esi, eax
    mov du, esi
    shr edx, 1              ;; lines/2
    imul    edx, ecx        ;; (pitch/2)*(lines/2)
    mov doffsetv, edx
;;-----
;;  entry point for I420andYUVAtoYV12_MMX_entry
;;
I420andYUVAtoI420_MMX_entry:
;;
    mov eax, src_lines
    mov ecx, src_pitch
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines
    shr ecx, 1              ;; pitch/2
    mov suvpitch, ecx
    mov esi, src
    mov sy, esi
    add esi, eax
    mov su, esi
    shr edx, 1              ;; lines/2
    imul    edx, ecx        ;; (pitch/2)*(lines/2)
    mov soffsetv, edx
;;-----
    mov eax, yuva_lines
    mov ecx, yuva_pitch
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines
    shr ecx, 1              ;; pitch/2
    mov yuvauvpitch, ecx
    mov esi, yuva
    mov yuvay, esi
    add esi, eax
    mov yuvau, esi
    
    shr edx, 1              ;; lines/2
    imul   edx, ecx         ;; (pitch/2)*(lines/2)
    mov yuvaoffsetv, edx
    lea esi, [esi+edx*2]
    mov yuvaa, esi
    mov yuvaauv, esi        ;; duplicate yuvaa for uv use
;;------------------
;;------------------
;; pointer adjustment to (x,y)
    mov ecx, src_pitch
    mov eax, src_starty
    mov edx, eax
    mov ebx, src_startx
    imul    eax, ecx        ;; y*pitch
    mov esi, sy             ;;
    add esi, eax            ;;  sy + y*pitch
    add esi, ebx            ;;  sy + y*pitch + x
    mov sy, esi

    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    mov esi, su             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    add esi, ebx            ;;  su + (y/2)*(pitch/2) + x/2
    mov su, esi
;;
;; pointer adjustment to (x,y)
    mov ecx, dst_pitch
    mov eax, dst_starty
    mov edx, eax
    mov ebx, dst_startx
    imul    eax, ecx        ;; y*pitch
    mov esi, dy             ;;
    add esi, eax            ;;  dy + y*pitch
    add esi, ebx            ;;  dy + y*pitch + x
    mov dy, esi

    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    mov esi, du             ;;
    add esi, edx            ;;  du + (y/2)*(pitch/2)
    add esi, ebx            ;;  du + (y/2)*(pitch/2) + x/2
    mov du, esi
;;    
;; pointer adjustment to (x,y)
    mov ecx, yuva_pitch
    mov eax, yuva_starty
    mov edx, eax
    mov ebx, yuva_startx
    imul    eax, ecx        ;; y*pitch
    add eax, ebx            ;; y*pitch + x
    
    mov esi, yuvay          ;;
    add esi, eax            ;;  yuvay + y*pitch + x
    mov yuvay, esi
    
    mov edi, yuvaa          ;;
    add edi, eax            ;;  yuvaa + y*pitch + x
    mov yuvaa, edi
    mov yuvaauv, edi        ;; dup for uv use

    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    mov esi, yuvau             ;;
    add esi, edx            ;;  yuvau + (y/2)*(pitch/2)
    add esi, ebx            ;;  yuvau + (y/2)*(pitch/2) + x/2
    mov yuvau, esi
    
;;-----



;; yuva   P = top
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]
;; [A01A02][A03A04]
;; [A11A12][A13A04]


;; I420 src  Q = bot
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]

;; I420 dst
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]



;;-------
    mov esi, yuvay
    mov ecx, yuvaa
    mov edi, sy
    mov ebx, dy
    mov ebp, height      ;; y lines loop counter
    or  ebp, ebp
    jle near exit
y100:
    mov eax, width
    sub eax, 7
    jle  y_by_fours

    movq    mm7, [mask3]
    movq    mm6, [mask3b]
a100:
    ;; yuva 8 y's
    movq    mm0, [esi]          ;; p8 p7 p6 p5 p4 p3 p2 p1
    movq    mm1, mm0            ;; p8 p7 p6 p5 p4 p3 p2 p1
    psrlw   mm0, 8              ;; word p8 p6 p4 p2 

    ;; src I420 8 y's
    movq    mm2, [edi]          ;; q8 q7 q6 q5 q4 q3 q2 q1
    movq    mm3, mm2            ;; q8 q7 q6 q5 q4 q3 q2 q1
    psrlw   mm2, 8              ;; word q8 q6 q4 q2 
    psubw   mm2, mm0            ;; q-p

    ;; yuva 8 a's
    movq    mm4, [ecx]          ;; a8 a7 a6 a5 a4 a3 a2 a1
    movq    mm5, mm4            ;; a8 a7 a6 a5 a4 a3 a2 a1
    psrlw   mm4, 8              ;; word a8 a6 a4 a2

    pmullw  mm2, mm4            ;; word alpha*(q-p)  for y8 y6 y4 y2
    pand    mm2, mm6            ;; word hi-byte alpha*(q-p) for y8 y6 y4 y2
    ;;
    ;;
    movq    mm0, mm1            ;; p8 p7 p6 p5 p4 p3 p2 p1
    pand    mm1, mm7            ;; word p7 p5 p3 p1
    pand    mm3, mm7            ;; word q7 q5 q3 q1
    
    psubw   mm3, mm1            ;; q-p
    pand    mm5, mm7            ;; word a7 a5 a3 a1
    pmullw  mm3, mm5
    psrlw   mm3, 8              ;; alpha*(q-p) >> 8 for y7 y5 y3 y1
    
    por     mm2, mm3            ;; alpha*(q-p) >> 8 for y8 y7 y6 y5 y4 y3 y2 y1
    paddb   mm2, mm0            ;; blended y8 y7 y6 y5 y4 y3 y2 y1
    
    movq    [ebx], mm2
    
    add esi, 8
    add edi, 8
    add ecx, 8
    add ebx, 8
    sub eax, 8
    jg  a100
;;----------------------
y_by_fours:
    pxor    mm7, mm7        ;; set to zero for unpack
    add eax, 4
    jle  y_one_two_three
a200:
    ;; yuva 4 y's
    movd    mm0, [esi]          ;; byte p4 p3 p2 p1
    punpcklbw mm0, mm7          ;; p4 p3 p2 p1

    ;; src I420 4 y's
    movd    mm2, [edi]          ;; byte  q4 q3 q2 q1
    punpcklbw mm2, mm7          ;; q4 q3 q2 q1
    psubw   mm2, mm0            ;; q-p

    ;; yuva 4 a's
    movd    mm4, [ecx]          ;; byte a4 a3 a2 a1
    punpcklbw mm4, mm7          ;; a4 a3 a2 a1

    pmullw  mm2, mm4
    psrlw   mm2, 8              ;; alpha*(q-p) >> 8 for y4 y3 y2 y1
    paddb   mm2, mm0            ;; blended word y4 y3 y2 y1
    
    packuswb    mm2, mm7        ;; blended byte y4 y3 y2 y1 
    ;;
    movd    [ebx], mm2

    add esi, 4
    add edi, 4
    add ecx, 4
    add ebx, 4
    sub eax, 4      ;; not needed if doing by_eights
    jg  a200        ;; not needed if doing by_eights
;;
;;
;;----------------------
;; do one, two, or three odd bytes
y_one_two_three:
    add eax, 3      ;; number bytes to do
    jle  line_done
    mov edx, 4
    sub edx, eax    ;; 4-eax = numbers bytes offset
    ;;
    movd mm6, edx   ;; number bytes to shift
    psllq   mm6, 3              ;; number bits to shift 
    ;;
    ;; pointer adjustment, negative offset
    neg edx     
;;
;;
    ;; yuva 1-3 y's
    movd    mm0, [esi]          ;; byte xx p3 p2 p1
    punpcklbw mm0, mm7          ;; xx p3 p2 p1

    ;; src I420 1-3 y's
    movd    mm2, [edi]          ;; byte  xx q3 q2 q1
    punpcklbw mm2, mm7          ;; xx p3 p2 p1
    psubw   mm2, mm0            ;; q-p

    ;; yuva 1-3 a's
    movd    mm4, [ecx+edx]      ;; byte a3 a2 a1 xx
    psrlq   mm4, mm6            ;; byte xx a3 a2 a1 
    punpcklbw mm4, mm7          ;; xx a3 a2 a1

    pmullw  mm2, mm4
    psrlw   mm2, 8              ;; alpha*(q-p) >> 8 for xx y3 y2 y1
    paddb   mm2, mm0            ;; blended word xx y3 y2 y1
    
    packuswb    mm2, mm7        ;; blended byte xx y3 y2 y1 
    ;;
    movd    dtmp0, mm2

    ;; store result, byte by byte
    ;; eax = bytes to do
a300:
    mov dl, btmp0(eax-1)
    mov [ebx+eax-1], dl
    dec eax
    jg  a300
;;------
line_done:

    mov eax, yuva_pitch
    mov esi, yuvay
    add esi, eax
    mov yuvay, esi

    mov ecx, yuvaa
    add ecx, eax
    mov yuvaa, ecx
        
    mov eax, src_pitch
    mov edi, sy
    add edi, eax
    mov sy, edi
    
    mov eax, dst_pitch
    mov ebx, dy
    add ebx, eax
    mov dy, ebx

    dec ebp
    jg  near y100        ;; line loop
;;-----------------------------------------------
;;-----------------------------------------------
;; do u and v
;;
    mov esi, yuvau
    mov ecx, yuvaauv
    add ecx, yuva_pitch    ;; use a's from second line
    mov yuvaauv, ecx       ;; save for uv line loop
    mov edi, su
    mov ebx, du

    mov ebp, height      ;; uv lines loop counter
    shr ebp, 1
    jle near exit
uv100:
    mov eax, width
    shr eax, 1      ;; u v width = width/2
    sub eax, 3
    jle  near uv_one_two_three

b200:
    ;; yuva 4 u's
    movd    mm0, [esi]          ;; byte pu4 pu3 pu2 pu1
    punpcklbw mm0, mm7          ;; pu4 pu3 pu2 pu1

    ;; src I420 4 u's
    movd    mm2, [edi]          ;; byte  qu4 qu3 qu2 qu1
    punpcklbw mm2, mm7          ;; qu4 qu3 qu2 qu1
    psubw   mm2, mm0            ;; q-p

    ;; yuva 8 a's
    ;; We need to average the alpha values by 2's. In the C version
    ;; In the C version we do the average by the whole 2x2 block.
    ;;movq    mm4, [ecx]          ;; byte a8 a7 a6 a5 a4 a3 a2 a1
    ;;psrlw   mm4, 8              ;; word a8 a6 a4 a2
    movq    mm4, [ecx]
    movq    mm7, mm4          ;; mm7=mm4= a8 a7 a6 a5 a4 a3 a2 a1
    psllw   mm7, 8            ;; mm7    = a7 00 a5 00 a3 00 a1 00
    psrlw   mm4, 8            ;; mm4    = 00 a8 00 a6 00 a4 00 a2 
    psrlw   mm7, 8            ;; mm7    = 00 a7 00 a5 00 a3 00 a1
    paddw   mm4, mm7          ;; mm4    = (a8+a7) (a6+a5) (a4+a3) (a2+a1)
    pxor    mm7, mm7          ;; mm7    = 0...0
    psrlw   mm4, 1            ;; mm4    = (a8+a7)/2 (a6+a5)/2 (a4+a3)/2 (a2+a1)/2

    pmullw  mm2, mm4
    psrlw   mm2, 8             ;; alpha*(q-p) >> 8 for u4 u3 u2 u1
    paddb   mm2, mm0           ;; byte add -> blended word u4 u3 u2 u1
    
    packuswb    mm2, mm7       ;; blended byte u4 u3 u2 u1 

    mov edx, yuvaoffsetv
    ;; yuva 4 v's
    movd    mm0, [esi+edx]      ;; byte pv4 pv3 pv2 pv1
    punpcklbw mm0, mm7          ;; pv4 pv3 pv2 pv1

    mov edx, soffsetv
    ;; src I420 4 v's
    movd    mm3, [edi+edx]      ;; byte  qv4 qv3 qv2 qv1
    punpcklbw mm3, mm7          ;; qv4 qv3 qv2 qv1
    psubw   mm3, mm0            ;; q-p

    pmullw  mm3, mm4
    psrlw   mm3, 8              ;; alpha*(q-p) >> 8 for v4 v3 v2 v1
    paddb   mm3, mm0            ;; blended word v4 v3 v2 v1
    
    packuswb    mm3, mm7        ;; blended byte v4 v3 v2 v1 

    movd    [ebx], mm2          ;; postpone store for src buf = dest buf
    mov edx, doffsetv
    movd    [ebx+edx], mm3

    add esi, 4
    add edi, 4
    add ecx, 8
    add ebx, 4
    sub eax, 4
    jg  near b200
;;----------------------
;; do one, two, or three odd bytes
;; reads are unaligned
uv_one_two_three:
    add eax, 3
    jle  near uv_line_done
    mov edx, 4
    sub edx, eax    ;; 4-eax = numbers bytes offset
    ;;
    movd mm6, edx   ;; number bytes to shift
    psllq   mm6, 3              ;; number bits to shift 
    ;;
    ;; pointer adjustment, negative offset
    neg edx
    add esi, edx
    add edi, edx
    lea ecx, [ecx+edx*2]
;;
    ;; yuva 1-3 u's
    movd    mm0, [esi]      ;; byte pu3 pu2 pu1 xxx
    punpcklbw mm0, mm7          ;; pu3 pu2 pu1 xxx

    ;; src I420 1-3 u's
    movd    mm2, [edi]      ;; byte qu3 qu2 qu1 xxx
    punpcklbw mm2, mm7          ;; qu3 qu2 qu1 xxx
    psubw   mm2, mm0            ;; q-p

    ;; yuva 1-3 a's
    movq    mm4, [ecx]    ;; byte  a6 a5 a4 a3 a2 a1 xx xx
    psrlw   mm4, 8              ;; word  a6 a4 a2 xx 

    pmullw  mm2, mm4
    psrlw   mm2, 8             ;; alpha*(q-p) >> 8 for u3 u2 u1 xx
    paddb   mm2, mm0           ;; blended word u3 u2 u1 xx
    
    packuswb    mm2, mm7       ;; blended byte u3 u2 u1 xx
    movd    dtmp0, mm2

    mov edx, yuvaoffsetv
    ;; yuva 2 v's
    movd    mm0, [esi+edx]      ;; byte  pv3 pv2 pv1 xxx
    punpcklbw mm0, mm7          ;; pv3 pv2 pv1 xxx

    mov edx, soffsetv
    ;; src I420 1-3 u's
    movd    mm2, [edi+edx]      ;; byte  qv3 qv2 qv1 xx
    punpcklbw mm2, mm7          ;; qv3 qv2 qv1 xx
    psubw   mm2, mm0            ;; q-p

    pmullw  mm2, mm4
    psrlw   mm2, 8              ;; alpha*(q-p) >> 8 for v3 v2 v1 xx
    paddb   mm2, mm0            ;; blended word v3 v2 v1 xx
    
    packuswb    mm2, mm7        ;; blended byte v3 v2 v1 xx
    movd    dtmp1, mm2

    
    mov edx, doffsetv
    ;; store result, byte by byte
    ;; eax = bytes to do
    neg eax
b300:
    mov cl, btmp0(4+eax)   ;; u     ;; ecx trashed
    mov ch, btmp1(4+eax)   ;; v
    mov [ebx], cl                   
    mov [ebx+edx], ch
    inc ebx
    inc eax
    jl  b300
;;------
uv_line_done:

    mov eax, yuvauvpitch
    mov esi, yuvau
    add esi, eax
    mov yuvau, esi

    mov eax, yuva_pitch
    mov ecx, yuvaauv
    lea ecx, [ecx + eax*2]  ;; down two lines
    mov yuvaauv, ecx        ;; save for uv line loop

    mov eax, suvpitch
    mov edi, su
    add edi, eax
    mov su, edi
    
    mov eax, duvpitch
    mov ebx, du
    add ebx, eax
    mov du, ebx

    dec ebp
    jg  near uv100           ;; uv line loop


;;------
exit:
    xor eax, eax    ;; return success
exit2:
    add esp, ntmps*4
    pop ebx
    pop ecx
    pop edi
    pop esi
    pop ebp
    emms
    ret

fail_exit:
    mov eax, -1     ;; signal fail
    jmp exit2


;_I420andYUVAtoI420_MMX endp
;====================================
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;	I420andYUVAtoYUY2
;;
;;	This function alpha-blends two I420 buffers into a third
;;	YUY2 buffer using the alpha info tacked to the 
;;	end of the second I420 buffer
;;
;;  yuva = top
;;  inverted alpha
;;  uv size computed as: uvpitch*uvlines = (pitch/2)*(lines/2)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_I420andYUVAtoYUY2_MMX:
;;
;; int I420andYUVAtoYUY2_MMX(
;;    unsigned char* src,  int src_pels,    int src_lines,  int src_pitch,
;;                         int src_startx,  int src_starty,
;;    unsigned char* yuva, int yuva_pels,   int yuva_lines, int yuva_pitch,
;;                         int yuva_startx, int yuva_starty,
;;    unsigned char* dst,  int dst_pels,    int dst_lines,  int dst_pitch,
;;                         int dst_startx,  int dst_starty,
;;    int width,  int height);
;;


make_labels  _I420andYUVAtoYUY2_MMX

;;
;; arguments
%define src            dword [esp+4*(1+npush)]
%define src_pels       dword [esp+4*(2+npush)]
%define src_lines      dword [esp+4*(3+npush)]
%define src_pitch      dword [esp+4*(4+npush)]
%define src_startx     dword [esp+4*(5+npush)]
%define src_starty     dword [esp+4*(6+npush)]

%define yuva           dword [esp+4*(7+npush)]
%define yuva_pels      dword [esp+4*(8+npush)]
%define yuva_lines     dword [esp+4*(9+npush)]
%define yuva_pitch     dword [esp+4*(10+npush)]
%define yuva_startx    dword [esp+4*(11+npush)]
%define yuva_starty    dword [esp+4*(12+npush)]

%define dst            dword [esp+4*(13+npush)]
%define dst_pels       dword [esp+4*(14+npush)]
%define dst_lines      dword [esp+4*(15+npush)]
%define dst_pitch      dword [esp+4*(16+npush)]
%define dst_startx     dword [esp+4*(17+npush)]
%define dst_starty     dword [esp+4*(18+npush)]

%define width          dword [esp+4*(19+npush)]
%define height         dword [esp+4*(20+npush)]

    push    ebp
    push    esi
    push    edi
    push    ecx
    push    ebx

;; tmp on stack
%assign ntmps 15
%assign npush (5+ntmps)
    sub esp, ntmps*4 

%define yuvay        dword [esp + 0*4]
%define yuvau        dword [esp + 1*4]
%define yuvaoffsetv  dword [esp + 2*4]
%define yuvaoffseta  dword [esp + 3*4]
%define negyuvapitch dword [esp + 4*4]
%define yuvayinc     dword [esp + 5*4]
%define yuvauvinc    dword [esp + 6*4]

%define sy           dword [esp + 7*4]
%define su           dword [esp + 8*4]
%define soffsetv     dword [esp + 9*4]
%define syinc        dword [esp + 10*4]
%define suvinc       dword [esp + 11*4]

%define dy           dword [esp + 12*4]
%define width0       dword [esp + 13*4]
%define dyinc        dword [esp + 14*4]


;; 
    mov edi, width
    and edi, -4    ;; width truncated to multiple of 4 (width & ~ 3)
    mov ebp, edi   ;; truncated for address increment computation only
    shr ebp, 1              ;; (width & ~ 3)/2
;;-----
    mov eax, dst
    mov dy, eax

    mov ecx, dst_pitch
    add ecx, ecx
    sub ecx, edi
    sub ecx, edi
    mov dyinc, ecx          ;; inc = 2*pitch - 2*(width & ~ 3)
;;-----
    mov eax, src_lines
    mov ecx, src_pitch
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines

    mov esi, src
    mov sy, esi
    add esi, eax
    mov su, esi

    shr edx, 1              ;; lines/2
    shr ecx, 1              ;; pitch/2
    imul    edx, ecx        ;; (pitch/2)*(lines*2)
    mov soffsetv, edx
    
    sub ecx, ebp            ;; pitch/2 - width/2
    mov suvinc, ecx

    mov ecx, src_pitch
    add ecx, ecx
    sub ecx, edi
    mov syinc, ecx          ;; inc = 2*pitch - (width & ~ 3)
;;-----
    mov eax, yuva_lines
    mov ecx, yuva_pitch
    mov edx, ecx
    neg edx
    mov negyuvapitch, edx

    mov edx, eax            ;; lines
    imul    eax, ecx        ;; pitch*lines
    mov esi, yuva
    mov yuvay, esi
    add esi, eax
    mov yuvau, esi
    shr ecx, 1              ;; pitch/2
    shr edx, 1              ;; lines/2
    imul    edx, ecx        ;; (lines/2)*(pitch/2)
    mov yuvaoffsetv, edx
    lea edx, [eax+edx*2]
    mov yuvaoffseta, edx
    sub ecx, ebp
    mov yuvauvinc, ecx

    mov ecx, yuva_pitch
    add ecx, ecx
    sub ecx, edi
    mov yuvayinc, ecx          ;; inc = 2*pitch - (width & ~ 3)
;;-----
;;------------------
;; pointer adjustment to (x,y)
    mov ecx, src_pitch
    mov eax, src_starty
    mov edx, eax
    mov ebx, src_startx
    imul    eax, ecx        ;; y*pitch
    mov esi, sy             ;;
    add esi, eax            ;;  sy + y*pitch
    add esi, ebx            ;;  sy + y*pitch + x
    mov sy, esi

    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    mov esi, su             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    add esi, ebx            ;;  su + (y/2)*(pitch/2) + x/2
    mov su, esi
;;
;; pointer adjustment to (x,y)
    mov ecx, yuva_pitch
    mov eax, yuva_starty
    mov edx, eax
    mov ebx, yuva_startx
    imul    eax, ecx        ;; y*pitch
    add eax, ebx            ;; y*pitch + x
    
    mov esi, yuvay          ;;
    add esi, eax            ;;  yuvay + y*pitch + x
    mov yuvay, esi
    
    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    mov esi, yuvau          ;;
    add esi, edx            ;;  yuvau + (y/2)*(pitch/2)
    add esi, ebx            ;;  yuvau + (y/2)*(pitch/2) + x/2
    mov yuvau, esi

;; pointer adjustment to (x,y)
    mov ecx, dst_pitch
    mov eax, dst_starty
    mov ebx, dst_startx
    imul    eax, ecx        ;; y*pitch
    add ebx, ebx            ;; 2*x

    mov esi, dy             ;;
    add esi, eax            ;;  dy + y*pitch
    add esi, ebx            ;;  dy + y*pitch + 2*x
    mov dy, esi
;;-----



;; yuva   P = top
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]
;; [A01A02][A03A04]
;; [A11Y12][A13A04]


;; I420 src  Q = bot
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]

;; YUY2 dst 
;; [YUYV][YUYV]         ;; byte order [Y0][U][Y1][V]
;; [YUYV][YUYV]
;;-------

    pxor    mm7, mm7
    movq    mm6, [con1]

    ;; set up line loop
    mov eax, height
    dec eax
    mov height, eax
    jle near exit

    mov esi, yuvay
    mov ebx, yuvau
    mov edi, sy
    mov edx, su


y100:               ;; line loop

    mov eax, width
    sub eax, 3
    mov width0, eax
    jle near two_pels


a100:
    ;; do four pels per iteration
    ;;
    mov ecx, yuvaoffseta    
    mov eax, src_pitch
    ;; ------- first line --------
    ;; yuva 4 y's
    movd    mm0, [esi]          ;; p4 p3 p2 p1
    punpcklbw   mm0, mm7        ;; word p4 p3 p2 p1 

    ;; src I420 4 y's
    movq    mm2, [edi]          ;; q4 q3 q2 q1
    punpcklbw mm2, mm7          ;; word q4 q3 q2 q1 
    psubw   mm2, mm0            ;; q-p

    ;; yuva 4 a's
    movd    mm4, [esi+ecx]      ;; a4 a3 a2 a1
    punpcklbw mm4, mm7          ;; word a4 a3 a2 a1

    pmullw  mm2, mm4            ;; word alpha*(q-p)  for y4 y3 y2 y1
    psrlw   mm2, 8              ;; byte alpha*(q-p)  for 0 y4 0 y3 0 y2 0 y1
    paddb   mm2, mm0            ;; blended for 0 y4 0 y3 0 y2 0 y1

    pmaddwd mm4, mm6           ;; line 1 dword a4+a3 a2+a1
    ;; 
    ;;   mm2 = line 1 blended for 0 y4 0 y3 0 y2 0 y1
    ;;   mm4 = line 1 dword ave(a4,a3) ave(a2,a1)
    ;;
    ;; ------ second line ---------
    mov ebp, negyuvapitch
    sub esi, ebp            ;; point to line 2
    ;;add esi, yuva_pitch
    ;;;;add edi, src_pitch

    ;; yuva 4 y's
    movd    mm1, [esi]          ;; p4 p3 p2 p1
    punpcklbw   mm1, mm7        ;; word p4 p3 p2 p1 

    ;; src I420 4 y's
    movd    mm3, [edi+eax]          ;; q4 q3 q2 q1
    punpcklbw mm3, mm7          ;; word q4 q3 q2 q1 
    psubw   mm3, mm1            ;; q-p

    ;; yuva 4 a's
    movd    mm5, [esi+ecx]      ;; a4 a3 a2 a1
    punpcklbw mm5, mm7          ;; word a4 a3 a2 a1

    lea esi, [esi+ebp+4]        ;; back to line 1 and inc
    add edi, 4                  ;; inc sy

    pmullw  mm3, mm5            ;; word alpha*(q-p)  for y4 y3 y2 y1
    psrlw   mm3, 8              ;; byte alpha*(q-p)  for 0 y4 0 y3 0 y2 0 y1
    paddb   mm3, mm1            ;; blended for 0 y4 0 y3 0 y2 0 y1

    pmaddwd mm5, mm6           ;; line 2 dword a4+a3 a2+a1
    ;; 
    packssdw    mm4, mm5        ;; for u v sum a22 a21 a12 a11
    psrlw       mm4, 1          ;; for u v ave  a22 a21 a12 a11
    ;;
    ;;   mm2 = line 1 blended for 0 y4 0 y3 0 y2 0 y1
    ;;   mm3 = line 2 blended for 0 y4 0 y3 0 y2 0 y1 0
    ;;   mm4 = ave for line 2 line 1 =  a22 a21 a12 a11
    ;;
    ;; ----- U V --------
    ;;mov ebx, yuvau
    ;;mov edx, su
    mov ecx, yuvaoffsetv
    mov eax, soffsetv

    ;; --U--
    movd    mm0, [ebx]          ;; x x pu2 pu1
    punpcklwd    mm0, mm0       ;; byte pu2 pu1 pu2 pu1
    punpcklbw    mm0, mm7       ;; word pu2 pu1 pu2 pu1

    movd    mm1, [edx]          ;; x x qu2 qu1
    punpcklwd    mm1, mm1       ;; byte qu2 qu1 qu2 qu1
    punpcklbw    mm1, mm7       ;; word qu2 qu1 qu2 qu1

    psubw   mm1, mm0            ;; qu - pu
    pmullw  mm1, mm4            ;; alpha*(qu-pu)
    psrlw   mm1, 8
    paddb   mm1, mm0            ;; line 2 line 1 blended u2 u1 u2 u1 

    ;; --V--
    movd    mm0, [ebx+ecx]          ;; x x pv2 pv1
    punpcklwd    mm0, mm0       ;; byte pv2 pv1 pv2 pv1
    punpcklbw    mm0, mm7       ;; word pv2 pv1 pv2 pv1

    ;;movd    mm5, [edx+eax]            ;; x x qv2 qv1
    movzx   eax, word [edx+eax]     ;; prevent possible access vio
    movd    mm5, eax            ;;  0 0 qv2 qv1
    punpcklwd    mm5, mm5       ;; byte qv2 qv1 qv2 qv1
    punpcklbw    mm5, mm7       ;; word qv2 qv1 qv2 qv1

    psubw   mm5, mm0            ;; qv - pv

    add ebx, 2                  ;; inc yuvau address
    add edx, 2                  ;; inc su address

    pmullw  mm5, mm4            ;; alpha*(qv-pv)
    psrlw   mm5, 8
    paddb   mm5, mm0            ;; line 2 line 1 blended v2 v1 v2 v1 

    ;;   mm2 = line 1 blended for 0 y4 0 y3 0 y2 0 y1
    ;;   mm3 = line 2 blended for 0 y4 0 y3 0 y2 0 y1
    ;;   mm1 = line 2 line 1 blended 0 u2 0 u1 0 u2 0 u1 
    ;;   mm5 = line 2 line 1 blended 0 v2 0 v1 0 v2 0 v1 
    ;;
    mov ecx, dy
    mov eax, dst_pitch

    packuswb    mm2, mm3        ;; blended y24 y23 y22 y21 y14 y13 y12 y11 

    psllq   mm5, 8              ;;  v2 0 v1 0 v2 0 v1 0
    por mm1, mm5                ;;  v22 u22 v21 u21 v12 u12 v11 u11
    
    movq    mm3, mm2
    punpcklbw   mm3, mm1        ;;  v12 y14 u12 y13 v11 y12 u11 y11

    movq    [ecx], mm3          ;; line 1 result

    punpckhbw   mm2, mm1        ;;  v22 y24 u22 y23 v21 y22 u21 y21

    movq    [ecx+eax], mm2      ;; line 2 result

    add ecx, 8                  ;; inc dy address
    mov dy, ecx

    mov eax, width0             ;; pel loop
    sub eax, 4
    mov width0, eax
    jg  near a100


;;------------------------------
two_pels:
    ;; do two pels if any
    ;; remaining pels = eax+3
    ;; compute 2 pels if remaining pels = 2 or 3, 1 not computed
    add eax, 2
    jle near line_done
    ;;
    ;;
    mov ecx, yuvaoffseta    
    mov eax, src_pitch
    ;; ------- first line --------
    ;; yuva 2 y's
    movd    mm0, [esi]          ;; p2 p1
    punpcklbw   mm0, mm7        ;; word p2 p1 

    ;; src I420 2 y's
    movq    mm2, [edi]          ;; q2 q1
    punpcklbw mm2, mm7          ;; word q2 q1 
    psubw   mm2, mm0            ;; q-p

    ;; yuva 2 a's
    movd    mm4, [esi+ecx]      ;; a2 a1
    punpcklbw mm4, mm7          ;; word a2 a1

    pmullw  mm2, mm4            ;; word alpha*(q-p)  for y2 y1
    psrlw   mm2, 8              ;; byte alpha*(q-p)  for 0 y2 0 y1
    paddb   mm2, mm0            ;; blended for 0 y2 0 y1

    pmaddwd mm4, mm6           ;; line 1 dword a2+a1
    ;; 
    ;;   mm2 = line 1 blended for 0 y2 0 y1
    ;;   mm4 = line 1 ave(a2,a1)
    ;;
    ;; ------ second line ---------
    mov ebp, negyuvapitch
    sub esi, ebp            ;; point to line 2

    ;; yuva 2 y's
    movd    mm1, [esi]          ;; p2 p1
    punpcklbw   mm1, mm7        ;; word p2 p1 

    ;; src I420 2 y's
    movd    mm3, [edi+eax]          ;; q2 q1
    punpcklbw mm3, mm7          ;; word q2 q1 
    psubw   mm3, mm1            ;; q-p

    ;; yuva 2 a's
    ;;movd    mm5, [esi+ecx]      ;; a2 a1
    movzx   eax, word [esi+ecx]     ;; prevent possible access vio
    movd    mm5, eax            ;; a2 a1
    punpcklbw mm5, mm7          ;; word a2 a1

    lea esi, [esi+ebp]        ;; back to line 1,  no inc
    ;;add edi, 4                  ;; inc sy

    pmullw  mm3, mm5            ;; word alpha*(q-p)  for y2 y1
    psrlw   mm3, 8              ;; byte alpha*(q-p)  for 0 y2 0 y1
    paddb   mm3, mm1            ;; blended for 0 y2 0 y1

    pmaddwd mm5, mm6           ;; line 2 dword a2+a1
    ;; 
    packssdw    mm4, mm5        ;; for uv sum xx a21 xx a11
    psrlw   mm4, 1              ;; for uv ave xx a21 xx a11
    ;;
    ;;   mm2 = line 1 blended for 0 y2 0 y1
    ;;   mm3 = line 2 blended for 0 y2 0 y1
    ;;   mm4 = ave =  a21 a11
    ;;
    ;; ----- U V --------
    mov ecx, yuvaoffsetv
    mov eax, soffsetv

    ;; --U--
    ;;movd    mm0, [ebx]          ;; x x x pu1
    ;;punpcklwd    mm0, mm0       ;; byte x pu1 x pu1
    ;;punpcklbw    mm0, mm7       ;; word x pu1 x pu1
    movzx ebp, byte [ebx]
    movd    mm0, ebp                ;; 0 0 0 pu1
    punpckldq   mm0, mm0            ;; word 0 pu1 0 pu1

    ;;movd    mm1, [edx]          ;; x x qu2 qu1
    ;;punpcklwd    mm1, mm1       ;; byte qu2 qu1 qu2 qu1
    ;;punpcklbw    mm1, mm7       ;; word qu2 qu1 qu2 qu1
    movzx   ebp, byte [edx]
    movd    mm1, ebp                ;; 0 0 0 qu1          
    punpckldq   mm1, mm1            ;; word 0 qu1 0 qu1   
    
    psubw   mm1, mm0            ;; qu - pu
    pmullw  mm1, mm4            ;; alpha*(qu-pu)
    psrlw   mm1, 8
    paddb   mm1, mm0            ;; line 2 line 1 blended u2 u1 u2 u1 

    ;; --V--
    ;;movd    mm0, [ebx+ecx]          ;; x x pv2 pv1
    movzx   ebp, byte [ebx+ecx]     ;; prevent possible access vio
    movd    mm0, ebp            ;; 0 0 0 pv1
    punpckldq    mm0, mm0       ;; word 0 pv1 0 pv1

    ;;movd    mm5, [edx+eax]            ;; qv1
    movzx   eax, byte [edx+eax]     ;; prevent possible access vio
    movd    mm5, eax            ;;  0 0 0 qv1
    punpckldq    mm5, mm5       ;; word 0 qv1 0 qv1

    psubw   mm5, mm0            ;; qv - pv

    ;;add ebx, 2                  ;; inc yuvau address
    ;;add edx, 2                  ;; inc su address

    pmullw  mm5, mm4            ;; alpha*(qv-pv)
    psrlw   mm5, 8
    paddb   mm5, mm0            ;; line 2 line 1 blended v2 v1 v2 v1 

    ;;   mm2 = line 1 blended for 0 y2 0 y1
    ;;   mm3 = line 2 blended for 0 y2 0 y1
    ;;   mm1 = line 2 line 1 blended 0 u2 0 u1 
    ;;   mm5 = line 2 line 1 blended 0 v2 0 v1 
    ;;
    mov ecx, dy
    mov eax, dst_pitch

    packuswb    mm2, mm3        ;; x x y22 y21 x x y12 y11

    psllq   mm5, 8              ;;  v2 0 v1 0 v2 0 v1 0
    por mm1, mm5                ;;  v22 u22 v21 u21 v12 u12 v11 u11
    
    movq    mm3, mm2
    punpcklbw   mm2, mm1        ;;  v11 y12 u11 y11
    movd    [ecx], mm2          ;; line 1 result

    punpckhbw   mm3, mm1        ;;  v21 y22 u21 y21 

    movd    [ecx+eax], mm3      ;; line 2 result

;;-----------
;; line loop
line_done:

;;
;;
    mov eax, yuvayinc      ;; move down two lines
    add esi, eax
    
    mov eax, yuvauvinc
    add ebx, eax
    
    mov eax, syinc
    add edi, eax

    mov eax, suvinc
    add edx, eax
    
    mov eax, dyinc
    mov ecx, dy
    add ecx, eax
    mov dy,  ecx

    mov eax, height
    sub eax, 2
    mov height, eax
    jg  near y100
;;-----------
exit:

    xor eax, eax    ;; return success

    add esp, ntmps*4
    pop ebx
    pop ecx
    pop edi
    pop esi
    pop ebp

    emms

    ret

;_I420andYUVAtoYUY2_MMX endp
;====================================
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;	I420andYUVAtoUYVY
;;
;;	This function alpha-blends two I420 buffers into a third
;;	UYVY buffer using the alpha info tacked to the 
;;	end of the second I420 buffer
;;
;;  yuva = top
;;  inverted alpha
;;  uv size computed as: uvpitch*uvlines = (pitch/2)*(lines/2)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_I420andYUVAtoUYVY_MMX:
;;
;; int I420andYUVAtoUYVY_MMX(
;;    unsigned char* src,  int src_pels,    int src_lines,  int src_pitch,
;;                         int src_startx,  int src_starty,
;;    unsigned char* yuva, int yuva_pels,   int yuva_lines, int yuva_pitch,
;;                         int yuva_startx, int yuva_starty,
;;    unsigned char* dst,  int dst_pels,    int dst_lines,  int dst_pitch,
;;                         int dst_startx,  int dst_starty,
;;    int width,  int height);
;;
;;
make_labels _I420andYUVAtoUYVY_MMX


;; arguments
%define src            dword [esp+4*(1+npush)]
%define src_pels       dword [esp+4*(2+npush)]
%define src_lines      dword [esp+4*(3+npush)]
%define src_pitch      dword [esp+4*(4+npush)]
%define src_startx     dword [esp+4*(5+npush)]
%define src_starty     dword [esp+4*(6+npush)]

%define yuva           dword [esp+4*(7+npush)]
%define yuva_pels      dword [esp+4*(8+npush)]
%define yuva_lines     dword [esp+4*(9+npush)]
%define yuva_pitch     dword [esp+4*(10+npush)]
%define yuva_startx    dword [esp+4*(11+npush)]
%define yuva_starty    dword [esp+4*(12+npush)]

%define dst            dword [esp+4*(13+npush)]
%define dst_pels       dword [esp+4*(14+npush)]
%define dst_lines      dword [esp+4*(15+npush)]
%define dst_pitch      dword [esp+4*(16+npush)]
%define dst_startx     dword [esp+4*(17+npush)]
%define dst_starty     dword [esp+4*(18+npush)]

%define width          dword [esp+4*(19+npush)]
%define height         dword [esp+4*(20+npush)]

    push    ebp
    push    esi
    push    edi
    push    ecx
    push    ebx

;; tmp on stack
%assign ntmps 15
%assign npush (5+ntmps)
    sub esp, ntmps*4 

%define yuvay        dword [esp + 0*4]
%define yuvau        dword [esp + 1*4]
%define yuvaoffsetv  dword [esp + 2*4]
%define yuvaoffseta  dword [esp + 3*4]
%define negyuvapitch dword [esp + 4*4]
%define yuvayinc     dword [esp + 5*4]
%define yuvauvinc    dword [esp + 6*4]

%define sy           dword [esp + 7*4]
%define su           dword [esp + 8*4]
%define soffsetv     dword [esp + 9*4]
%define syinc        dword [esp + 10*4]
%define suvinc       dword [esp + 11*4]

%define dy           dword [esp + 12*4]
%define width0       dword [esp + 13*4]
%define dyinc        dword [esp + 14*4]


    mov edi, width
    and edi, -4         ;; pels truncated to multiple of 4 (width & ~ 3)
    mov ebp, edi
    shr ebp, 1              ;; (width & ~ 3)/2
;;-----
    mov eax, dst
    mov dy, eax

    mov ecx, dst_pitch
    add ecx, ecx
    sub ecx, edi
    sub ecx, edi
    mov dyinc, ecx          ;; inc = 2*pitch - 2*(width & ~ 3)
;;-----
    mov eax, src_lines
    mov ecx, src_pitch
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines

    mov esi, src
    mov sy, esi
    add esi, eax
    mov su, esi

    shr edx, 1              ;; lines/2
    shr ecx, 1              ;; pitch/2
    imul    edx, ecx        ;; (pitch/2)*(lines*2)
    mov soffsetv, edx
    
    sub ecx, ebp            ;; pitch/2 - width/2
    mov suvinc, ecx

    mov ecx, src_pitch
    add ecx, ecx
    sub ecx, edi
    mov syinc, ecx          ;; inc = 2*pitch - (width & ~ 3)
;;-----
    mov eax, yuva_lines
    mov ecx, yuva_pitch
    mov edx, ecx
    neg edx
    mov negyuvapitch, edx

    mov edx, eax            ;; lines
    imul    eax, ecx        ;; pitch*lines
    mov esi, yuva
    mov yuvay, esi
    add esi, eax
    mov yuvau, esi
    shr ecx, 1              ;; pitch/2
    shr edx, 1              ;; lines/2
    imul    edx, ecx        ;; (lines/2)*(pitch/2)
    mov yuvaoffsetv, edx
    lea edx, [eax+edx*2]
    mov yuvaoffseta, edx
    sub ecx, ebp
    mov yuvauvinc, ecx

    mov ecx, yuva_pitch
    add ecx, ecx
    sub ecx, edi
    mov yuvayinc, ecx          ;; inc = 2*pitch - (width & ~ 3)
;;-----
;;------------------
;; pointer adjustment to (x,y)
    mov ecx, src_pitch
    mov eax, src_starty
    mov edx, eax
    mov ebx, src_startx
    imul    eax, ecx        ;; y*pitch
    mov esi, sy             ;;
    add esi, eax            ;;  sy + y*pitch
    add esi, ebx            ;;  sy + y*pitch + x
    mov sy, esi

    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    mov esi, su             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    add esi, ebx            ;;  su + (y/2)*(pitch/2) + x/2
    mov su, esi
;;
;; pointer adjustment to (x,y)
    mov ecx, yuva_pitch
    mov eax, yuva_starty
    mov edx, eax
    mov ebx, yuva_startx
    imul    eax, ecx        ;; y*pitch
    add eax, ebx            ;; y*pitch + x
    
    mov esi, yuvay          ;;
    add esi, eax            ;;  yuvay + y*pitch + x
    mov yuvay, esi
    
    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    mov esi, yuvau          ;;
    add esi, edx            ;;  yuvau + (y/2)*(pitch/2)
    add esi, ebx            ;;  yuvau + (y/2)*(pitch/2) + x/2
    mov yuvau, esi

;; pointer adjustment to (x,y)
    mov ecx, dst_pitch
    mov eax, dst_starty
    mov ebx, dst_startx
    imul    eax, ecx        ;; y*pitch
    add ebx, ebx            ;; 2*x

    mov esi, dy             ;;
    add esi, eax            ;;  dy + y*pitch
    add esi, ebx            ;;  dy + y*pitch + 2*x
    mov dy, esi
;;-----



;; yuva   P = top
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]
;; [A01A02][A03A04]
;; [A11Y12][A13A04]


;; I420 src  Q = bot
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]

;; UYVY dst 
;; [UYVY][UYVY]         ;; byte order [U][Y0][V][Y1]
;; [UYVY][UYVY]
;;-------

    pxor    mm7, mm7
    movq    mm6, [con1]

    ;; set up line loop
    mov eax, height  
    dec eax
    mov height, eax
    jle near exit

    mov esi, yuvay
    mov ebx, yuvau
    mov edi, sy
    mov edx, su


y100:               ;; line loop

    mov eax, width
    sub eax, 3
    mov width0, eax
    jle near two_pels


a100:
    ;; do four pels per iteration
    ;;
    mov ecx, yuvaoffseta    
    mov eax, src_pitch
    ;; ------- first line --------
    ;; yuva 4 y's
    movd    mm0, [esi]          ;; p4 p3 p2 p1
    punpcklbw   mm0, mm7        ;; word p4 p3 p2 p1 

    ;; src I420 4 y's
    movq    mm2, [edi]          ;; q4 q3 q2 q1
    punpcklbw mm2, mm7          ;; word q4 q3 q2 q1 
    psubw   mm2, mm0            ;; q-p

    ;; yuva 4 a's
    movd    mm4, [esi+ecx]      ;; a4 a3 a2 a1
    punpcklbw mm4, mm7          ;; word a4 a3 a2 a1

    pmullw  mm2, mm4            ;; word alpha*(q-p)  for y4 y3 y2 y1
    psrlw   mm2, 8              ;; byte alpha*(q-p)  for 0 y4 0 y3 0 y2 0 y1
    paddb   mm2, mm0            ;; blended for 0 y4 0 y3 0 y2 0 y1

    pmaddwd mm4, mm6           ;; line 1 dword a4+a3 a2+a1
    ;; 
    ;;   mm2 = line 1 blended for 0 y4 0 y3 0 y2 0 y1
    ;;   mm4 = line 1 dword ave(a4,a3) ave(a2,a1)
    ;;
    ;; ------ second line ---------
    mov ebp, negyuvapitch
    sub esi, ebp            ;; point to line 2
    ;;add esi, yuva_pitch
    ;;;;add edi, src_pitch

    ;; yuva 4 y's
    movd    mm1, [esi]          ;; p4 p3 p2 p1
    punpcklbw   mm1, mm7        ;; word p4 p3 p2 p1 

    ;; src I420 4 y's
    movd    mm3, [edi+eax]          ;; q4 q3 q2 q1
    punpcklbw mm3, mm7          ;; word q4 q3 q2 q1 
    psubw   mm3, mm1            ;; q-p

    ;; yuva 4 a's
    movd    mm5, [esi+ecx]      ;; a4 a3 a2 a1
    punpcklbw mm5, mm7          ;; word a4 a3 a2 a1

    lea esi, [esi+ebp+4]        ;; back to line 1 and inc
    add edi, 4                  ;; inc sy

    pmullw  mm3, mm5            ;; word alpha*(q-p)  for y4 y3 y2 y1
    psrlw   mm3, 8              ;; byte alpha*(q-p)  for 0 y4 0 y3 0 y2 0 y1
    paddb   mm3, mm1            ;; blended for 0 y4 0 y3 0 y2 0 y1

    pmaddwd mm5, mm6           ;; line 2 dword a4+a3 a2+a1
    ;; 
    packssdw    mm4, mm5        ;; for u v sum a22 a21 a12 a11
    psrlw       mm4, 1          ;; for u v ave  a22 a21 a12 a11
    ;;
    ;;   mm2 = line 1 blended for 0 y4 0 y3 0 y2 0 y1
    ;;   mm3 = line 2 blended for 0 y4 0 y3 0 y2 0 y1 0
    ;;   mm4 = ave for line 2 line 1 =  a22 a21 a12 a11
    ;;
    ;; ----- U V --------
    ;;mov ebx, yuvau
    ;;mov edx, su
    mov ecx, yuvaoffsetv
    mov eax, soffsetv

    ;; --U--
    movd    mm0, [ebx]          ;; x x pu2 pu1
    punpcklwd    mm0, mm0       ;; byte pu2 pu1 pu2 pu1
    punpcklbw    mm0, mm7       ;; word pu2 pu1 pu2 pu1

    movd    mm1, [edx]          ;; x x qu2 qu1
    punpcklwd    mm1, mm1       ;; byte qu2 qu1 qu2 qu1
    punpcklbw    mm1, mm7       ;; word qu2 qu1 qu2 qu1

    psubw   mm1, mm0            ;; qu - pu
    pmullw  mm1, mm4            ;; alpha*(qu-pu)
    psrlw   mm1, 8
    paddb   mm1, mm0            ;; line 2 line 1 blended u2 u1 u2 u1 

    ;; --V--
    movd    mm0, [ebx+ecx]          ;; x x pv2 pv1
    punpcklwd    mm0, mm0       ;; byte pv2 pv1 pv2 pv1
    punpcklbw    mm0, mm7       ;; word pv2 pv1 pv2 pv1

    ;;movd    mm5, [edx+eax]            ;; x x qv2 qv1
    movzx   eax, word [edx+eax]     ;; prevent possible access vio
    movd    mm5, eax            ;;  0 0 qv2 qv1
    punpcklwd    mm5, mm5       ;; byte qv2 qv1 qv2 qv1
    punpcklbw    mm5, mm7       ;; word qv2 qv1 qv2 qv1

    psubw   mm5, mm0            ;; qv - pv

    add ebx, 2                  ;; inc yuvau address
    add edx, 2                  ;; inc su address

    pmullw  mm5, mm4            ;; alpha*(qv-pv)
    psrlw   mm5, 8
    paddb   mm5, mm0            ;; line 2 line 1 blended v2 v1 v2 v1 

    ;;   mm2 = line 1 blended for 0 y4 0 y3 0 y2 0 y1
    ;;   mm3 = line 2 blended for 0 y4 0 y3 0 y2 0 y1
    ;;   mm1 = line 2 line 1 blended 0 u2 0 u1 0 u2 0 u1 
    ;;   mm5 = line 2 line 1 blended 0 v2 0 v1 0 v2 0 v1 
    ;;
    mov ecx, dy
    mov eax, dst_pitch

    packuswb    mm2, mm3        ;; blended y24 y23 y22 y21 y14 y13 y12 y11 

    psllq   mm5, 8              ;;  v2 0 v1 0 v2 0 v1 0
    por mm1, mm5                ;;  v22 u22 v21 u21 v12 u12 v11 u11
    
    movq    mm3, mm1
    punpcklbw   mm1, mm2        ;;  y14 v12 y13 u12 y12 v11 y11 u11 

    movq    [ecx], mm1          ;; line 1 result

    punpckhbw   mm3, mm2        ;;  y24 v22 y23 u22 y22 v21 y21 u21

    movq    [ecx+eax], mm3      ;; line 2 result

    add ecx, 8                  ;; inc dy address
    mov dy, ecx

    mov eax, width0             ;; pel loop
    sub eax, 4
    mov width0, eax
    jg  near a100


;;------------------------------
two_pels:
    ;; do two pels if any
    ;; remaining pels = eax+3
    ;; compute 2 pels if remaining pels = 2 or 3, 1 not computed
    add eax, 2
    jle near line_done
    ;;
    ;;
    mov ecx, yuvaoffseta    
    mov eax, src_pitch
    ;; ------- first line --------
    ;; yuva 2 y's
    movd    mm0, [esi]          ;; p2 p1
    punpcklbw   mm0, mm7        ;; word p2 p1 

    ;; src I420 2 y's
    movq    mm2, [edi]          ;; q2 q1
    punpcklbw mm2, mm7          ;; word q2 q1 
    psubw   mm2, mm0            ;; q-p

    ;; yuva 2 a's
    movd    mm4, [esi+ecx]      ;; a2 a1
    punpcklbw mm4, mm7          ;; word a2 a1

    pmullw  mm2, mm4            ;; word alpha*(q-p)  for y2 y1
    psrlw   mm2, 8              ;; byte alpha*(q-p)  for 0 y2 0 y1
    paddb   mm2, mm0            ;; blended for 0 y2 0 y1

    pmaddwd mm4, mm6           ;; line 1 dword a2+a1
    ;; 
    ;;   mm2 = line 1 blended for 0 y2 0 y1
    ;;   mm4 = line 1 ave(a2,a1)
    ;;
    ;; ------ second line ---------
    mov ebp, negyuvapitch
    sub esi, ebp            ;; point to line 2

    ;; yuva 2 y's
    movd    mm1, [esi]          ;; p2 p1
    punpcklbw   mm1, mm7        ;; word p2 p1 

    ;; src I420 2 y's
    movd    mm3, [edi+eax]          ;; q2 q1
    punpcklbw mm3, mm7          ;; word q2 q1 
    psubw   mm3, mm1            ;; q-p

    ;; yuva 2 a's
    ;;movd    mm5, [esi+ecx]      ;; a2 a1
    movzx   eax, word [esi+ecx]     ;; prevent possible access vio
    movd    mm5, eax            ;; a2 a1
    punpcklbw mm5, mm7          ;; word a2 a1

    lea esi, [esi+ebp]        ;; back to line 1,  no inc
    ;;add edi, 4                  ;; inc sy

    pmullw  mm3, mm5            ;; word alpha*(q-p)  for y2 y1
    psrlw   mm3, 8              ;; byte alpha*(q-p)  for 0 y2 0 y1
    paddb   mm3, mm1            ;; blended for 0 y2 0 y1

    pmaddwd mm5, mm6           ;; line 2 dword a2+a1
    ;; 
    packssdw    mm4, mm5        ;; for uv sum xx a21 xx a11
    psrlw   mm4, 1              ;; for uv ave xx a21 xx a11
    ;;
    ;;   mm2 = line 1 blended for 0 y2 0 y1
    ;;   mm3 = line 2 blended for 0 y2 0 y1
    ;;   mm4 = ave =  a21 a11
    ;;
    ;; ----- U V --------
    mov ecx, yuvaoffsetv
    mov eax, soffsetv

    ;; --U--
    ;;movd    mm0, [ebx]          ;; x x x pu1
    ;;punpcklwd    mm0, mm0       ;; byte x pu1 x pu1
    ;;punpcklbw    mm0, mm7       ;; word x pu1 x pu1
    movzx ebp, byte [ebx]
    movd    mm0, ebp                ;; 0 0 0 pu1
    punpckldq   mm0, mm0            ;; word 0 pu1 0 pu1

    ;;movd    mm1, [edx]          ;; x x qu2 qu1
    ;;punpcklwd    mm1, mm1       ;; byte qu2 qu1 qu2 qu1
    ;;punpcklbw    mm1, mm7       ;; word qu2 qu1 qu2 qu1
    movzx   ebp, byte [edx]
    movd    mm1, ebp                ;; 0 0 0 qu1          
    punpckldq   mm1, mm1            ;; word 0 qu1 0 qu1   
    
    psubw   mm1, mm0            ;; qu - pu
    pmullw  mm1, mm4            ;; alpha*(qu-pu)
    psrlw   mm1, 8
    paddb   mm1, mm0            ;; line 2 line 1 blended u2 u1 u2 u1 

    ;; --V--
    ;;movd    mm0, [ebx+ecx]          ;; x x pv2 pv1
    movzx   ebp, byte [ebx+ecx]     ;; prevent possible access vio
    movd    mm0, ebp            ;; 0 0 0 pv1
    punpckldq    mm0, mm0       ;; word 0 pv1 0 pv1

    ;;movd    mm5, [edx+eax]            ;; qv1
    movzx   eax, byte [edx+eax]     ;; prevent possible access vio
    movd    mm5, eax            ;;  0 0 0 qv1
    punpckldq    mm5, mm5       ;; word 0 qv1 0 qv1

    psubw   mm5, mm0            ;; qv - pv

    ;;add ebx, 2                  ;; inc yuvau address
    ;;add edx, 2                  ;; inc su address

    pmullw  mm5, mm4            ;; alpha*(qv-pv)
    psrlw   mm5, 8
    paddb   mm5, mm0            ;; line 2 line 1 blended v2 v1 v2 v1 

    ;;   mm2 = line 1 blended for 0 y2 0 y1
    ;;   mm3 = line 2 blended for 0 y2 0 y1
    ;;   mm1 = line 2 line 1 blended 0 u2 0 u1 
    ;;   mm5 = line 2 line 1 blended 0 v2 0 v1 
    ;;
    mov ecx, dy
    mov eax, dst_pitch

    packuswb    mm2, mm3        ;; x x y22 y21 x x y12 y11

    psllq   mm5, 8              ;;  v2 0 v1 0 v2 0 v1 0
    por mm1, mm5                ;;  v22 u22 v21 u21 v12 u12 v11 u11
    
    movq    mm3, mm1
    punpcklbw   mm1, mm2        ;;  v11 y12 u11 y11
    movd    [ecx], mm1          ;; line 1 result

    punpckhbw   mm3, mm2        ;;  v21 y22 u21 y21 

    movd    [ecx+eax], mm3      ;; line 2 result

;;-----------
;; line loop
line_done:

    mov eax, yuvayinc      ;; move down two lines
    add esi, eax
    
    mov eax, yuvauvinc
    add ebx, eax
    
    mov eax, syinc
    add edi, eax

    mov eax, suvinc
    add edx, eax
    
    mov eax, dyinc
    mov ecx, dy
    add ecx, eax
    mov dy,  ecx

    mov eax, height
    sub eax, 2
    mov height, eax
    jg  near y100
;;-----------
exit:

    xor eax, eax    ;; return success

    add esp, ntmps*4
    pop ebx
    pop ecx
    pop edi
    pop esi
    pop ebp

    emms

    ret

;_I420andYUVAtoUYVY_MMX endp
;====================================
;====================================
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;int I420andI420toI420 (
;;  unsigned char *src1_ptr, int src1_pels, int src1_lines, int src1_pitch,
;;  unsigned char *src2_ptr, int src2_pels, int src2_lines, int src2_pitch,
;;  unsigned char *dest_ptr, int dest_pels, int dest_lines, int dest_pitch,
;;  int alpha)
;;
;;
;;  src2 = top
;;  inverted alpha
;;  uv size computed as: uvpitch*uvlines = (pitch/2)*(lines/2)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_I420andI420toI420_MMX_sub:
;;
;; int I420andI420toI420_MMX_sub(
;;  unsigned char *src1_ptr, int src1_pels,   int src1_lines,  int src1_pitch,
;;             int src1_startx, int src1_starty,
;;  unsigned char *src2_ptr, int src2_pels,   int src2_lines,  int src2_pitch,
;;             int src2_startx, int src2_starty,
;;  unsigned char *dest_ptr, int dest_pels,   int dest_lines,  int dest_pitch,
;;             int dest_startx, int dest_starty,
;;  int width,  int height,  int alpha );
;;
make_labels _I420andI420toI420_MMX_sub
;;
;; arguments
%define src            dword [esp+4*(1+npush)]
%define src_pels       dword [esp+4*(2+npush)]
%define src_lines      dword [esp+4*(3+npush)]
%define src_pitch      dword [esp+4*(4+npush)]
%define src_startx     dword [esp+4*(5+npush)]
%define src_starty     dword [esp+4*(6+npush)]

%define top            dword [esp+4*(7+npush)]
%define top_pels       dword [esp+4*(8+npush)]
%define top_lines      dword [esp+4*(9+npush)]
%define top_pitch      dword [esp+4*(10+npush)]
%define top_startx     dword [esp+4*(11+npush)]
%define top_starty     dword [esp+4*(12+npush)]

%define dst            dword [esp+4*(13+npush)]
%define dst_pels       dword [esp+4*(14+npush)]
%define dst_lines      dword [esp+4*(15+npush)]
%define dst_pitch      dword [esp+4*(16+npush)]
%define dst_startx     dword [esp+4*(17+npush)]
%define dst_starty     dword [esp+4*(18+npush)]

%define width          dword [esp+4*(19+npush)]
%define height         dword [esp+4*(20+npush)]

%define alpha          dword [esp+4*(21+npush)]


    push    ebp
    push    esi
    push    edi
    push    ecx
    push    ebx

;; tmp on stack
%assign ntmps 16
%assign npush (5+ntmps)
    sub esp, ntmps*4 


;; cycle through pointers to y then u the v indexed by ecx
;; structure below must agree
%define topptr       dword [esp + 0*4 + ecx*4]   ;; topy[ecx*4]
%define sptr         dword [esp + 4*4 + ecx*4]   ;; sy[ecx*4]
%define dptr         dword [esp + 8*4 + ecx*4]   ;; dy[ecx*4]
;; 
%define topy         dword [esp + 0*4]
%define topu         dword [esp + 1*4]
%define topv         dword [esp + 2*4]
%define toppitch     dword [esp + 3*4]

%define sy           dword [esp + 4*4]
%define su           dword [esp + 5*4]
%define sv           dword [esp + 6*4]
%define spitch       dword [esp + 7*4]

%define dy           dword [esp + 8*4]
%define du           dword [esp + 9*4]
%define dv           dword [esp + 10*4]
%define dpitch       dword [esp + 11*4]

%define lines        dword [esp + 12*4]
%define pels         dword [esp + 13*4]

%define dtmp0        dword [esp + 14*4]
%define dtmp1        dword [esp + 15*4]
%define btmp0(x)     byte [esp + 14*4 + x]
%define btmp1(x)     byte [esp + 15*4 + x]


;;-----
    mov eax, dst_lines
    mov ecx, dst_pitch
    mov dpitch, ecx
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines
    mov esi, dst
    mov dy, esi
    add esi, eax
    mov du, esi

    shr edx, 1              ;; lines/2  = uv lines
    shr ecx, 1              ;; pitch/2  = uvpitch
    imul    edx, ecx        ;; 

    add esi, edx
    mov dv, esi
;;-----
    mov eax, src_lines
    mov ecx, src_pitch
    mov spitch, ecx
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines
    mov esi, src
    mov sy, esi
    add esi, eax
    mov su, esi

    shr edx, 1              ;; lines/2  = uv lines
    shr ecx, 1              ;; pitch/2  = uvpitch
    imul    edx, ecx        ;; 

    add esi, edx
    mov sv, esi
;;-----
    mov eax, top_lines
    mov ecx, top_pitch
    mov toppitch, ecx
    mov edx, eax
    imul    eax, ecx        ;; pitch*lines
    mov esi, top
    mov topy, esi
    add esi, eax
    mov topu, esi

    shr edx, 1              ;; lines/2  = uv lines
    shr ecx, 1              ;; pitch/2  = uvpitch
    imul    edx, ecx        ;; 

    add esi, edx
    mov topv, esi
;;-----
    mov eax, height
    mov lines, eax
    mov eax, width
    mov pels, eax

;;------------------
;;------------------
;; pointer adjustment to (x,y)
    mov ecx, top_pitch
    mov eax, top_starty
    mov edx, eax
    mov ebx, top_startx
    imul    eax, ecx        ;; y*pitch
    mov esi, topy             ;;
    add esi, eax            ;;  sy + y*pitch
    add esi, ebx            ;;  sy + y*pitch + x
    mov topy, esi

    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    add edx, ebx        ;; (y/2)*(pitch/2) + x/2

    mov esi, topu             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    mov topu, esi

    mov esi, topv             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    mov topv, esi
;;
;; pointer adjustment to (x,y)
    mov ecx, src_pitch
    mov eax, src_starty
    mov edx, eax
    mov ebx, src_startx
    imul    eax, ecx        ;; y*pitch
    mov esi, sy             ;;
    add esi, eax            ;;  sy + y*pitch
    add esi, ebx            ;;  sy + y*pitch + x
    mov sy, esi

    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    add edx, ebx        ;; (y/2)*(pitch/2) + x/2

    mov esi, su             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    mov su, esi

    mov esi, sv             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    mov sv, esi
;;;
;; pointer adjustment to (x,y)
    mov ecx, dst_pitch
    mov eax, dst_starty
    mov edx, eax
    mov ebx, dst_startx
    imul    eax, ecx        ;; y*pitch
    mov esi, dy             ;;
    add esi, eax            ;;  sy + y*pitch
    add esi, ebx            ;;  sy + y*pitch + x
    mov dy, esi

    shr ecx, 1          ;; pitch/2
    shr edx, 1          ;; y/2
    imul    edx, ecx    ;; (y/2)*(pitch/2)
    shr ebx, 1          ;; x/2
    add edx, ebx        ;; (y/2)*(pitch/2) + x/2

    mov esi, du             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    mov du, esi

    mov esi, dv             ;;
    add esi, edx            ;;  su + (y/2)*(pitch/2)
    mov dv, esi
;;-----
;; I420 top   P = top
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]


;; I420 src  Q = bot
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]

;; I420 dst
;; [Y01Y02][Y03Y04]
;; [Y11Y12][Y13Y04]
;;   [U1]    [U3]
;;   [V1]    [V3]


    xor ecx, ecx            ;; y then u then v counter

;; load alpha
    movzx   eax, byte alpha
    movd    mm4, eax
    punpcklwd    mm4, mm4        ;;   0 0 a a 
    punpckldq    mm4, mm4        ;    a a a a 
;;
    movq    mm7, [mask3]
    movq    mm6, [mask3b]
    pxor    mm5, mm5

;;-------
yuv_loop:
    mov esi, topptr
    mov edi, sptr
    mov ebx, dptr

    mov ebp, lines      ;; y lines loop counter
    or  ebp, ebp
    jle near exit
y100:
    mov eax, pels
    sub eax, 7
    jle  y_by_fours

a100:
    ;;  mm4 = alpha
    ;; top 8 y's
    movq    mm0, [esi]          ;; p8 p7 p6 p5 p4 p3 p2 p1
    movq    mm1, mm0            ;; p8 p7 p6 p5 p4 p3 p2 p1
    psrlw   mm0, 8              ;; word p8 p6 p4 p2 

    ;; src I420 8 y's
    movq    mm2, [edi]          ;; q8 q7 q6 q5 q4 q3 q2 q1
    movq    mm3, mm2            ;; q8 q7 q6 q5 q4 q3 q2 q1
    psrlw   mm2, 8              ;; word q8 q6 q4 q2 
    psubw   mm2, mm0            ;; q-p

    pmullw  mm2, mm4            ;; word alpha*(q-p)  for y8 y6 y4 y2
    pand    mm2, mm6            ;; word hi-byte alpha*(q-p) for y8 y6 y4 y2
    ;;
    ;; 
    movq    mm0, mm1            ;; p8 p7 p6 p5 p4 p3 p2 p1
    pand    mm1, mm7            ;; word p7 p5 p3 p1
    pand    mm3, mm7            ;; word q7 q5 q3 q1
    
    psubw   mm3, mm1            ;; q-p
    pmullw  mm3, mm4
    psrlw   mm3, 8              ;; alpha*(q-p) >> 8 for y7 y5 y3 y1
    
    por     mm2, mm3            ;; alpha*(q-p) >> 8 for y8 y7 y6 y5 y4 y3 y2 y1
    paddb   mm2, mm0            ;; blended y8 y7 y6 y5 y4 y3 y2 y1
    
    movq    [ebx], mm2

    add esi, 8
    add edi, 8
    add ebx, 8

    sub eax, 8
    jg  a100
;;----------------------
y_by_fours:
    add eax, 4
    jle  y_one_two_three
a200:
    ;; top 4 y's
    movd    mm0, [esi]          ;; byte p4 p3 p2 p1
    punpcklbw mm0, mm5          ;; p4 p3 p2 p1

    ;; src I420 4 y's
    movd    mm2, [edi]          ;; byte  q4 q3 q2 q1
    punpcklbw mm2, mm5          ;; q4 q3 q2 q1
    psubw   mm2, mm0            ;; q-p

    pmullw  mm2, mm4
    psrlw   mm2, 8              ;; alpha*(q-p) >> 8 for y4 y3 y2 y1
    paddb   mm2, mm0            ;; blended word y4 y3 y2 y1
                        
    packuswb    mm2, mm5        ;; blended byte y4 y3 y2 y1 
    ;;
    movd    [ebx], mm2

    add esi, 4
    add edi, 4
    add ebx, 4
    sub eax, 4      ;; not needed if doing by_eights
    jg  a200        ;; not needed if doing by_eights
;;
;;
;;----------------------
;; do one, two, or three odd bytes
y_one_two_three:
    add eax, 3      ;; number bytes to do
    jle  line_done
    
    xor edx, edx    ;; zero offset on y
    or  ecx, ecx    ;; check for y (not u pr v)
    je  a333
    mov edx, 4
    sub edx, eax    ;; 4 - eax = numbers bytes offset
a333:

    movd mm1, edx   ;; number bytes to shift
    psllq   mm1, 3  ;; number bits to shift 
    ;;
    ;; pointer adjustment, negative offset
    neg edx     
;;
;;
    ;; top I420 1-3 y's
    movd    mm0, [esi+edx]          ;; byte p3 p2 p1 xx
    psrlq   mm0, mm1            ;; byte xx p3 p2 p1 
    punpcklbw mm0, mm5          ;; xx p3 p2 p1

    ;; src I420 1-3 y's
    movd    mm2, [edi+edx]          ;; byte  q3 q2 q1 xx
    psrlq   mm2, mm1            ;; byte xx q3 q2 q1 
    punpcklbw mm2, mm5          ;; xx q3 q2 q1
    psubw   mm2, mm0            ;; q-p

    pmullw  mm2, mm4
    psrlw   mm2, 8              ;; alpha*(q-p) >> 8 for xx y3 y2 y1
    paddb   mm2, mm0            ;; blended word xx y3 y2 y1
    
    packuswb    mm2, mm5        ;; blended byte xx y3 y2 y1 
    ;;
    movd    dtmp0, mm2

    ;; store result, byte by byte
    ;; eax = bytes to do
a300:
    mov dl, btmp0(eax-1)
    mov [ebx+eax-1], dl
    dec eax
    jg  a300
;;------
line_done:

    mov eax, toppitch
    mov esi, topptr
    add esi, eax
    mov topptr, esi

    mov eax, spitch
    mov edi, sptr
    add edi, eax
    mov sptr, edi
    
    mov eax, dpitch
    mov ebx, dptr
    add ebx, eax
    mov dptr, ebx

    dec ebp
    jg  near y100        ;; line loop

;;------
    
;;-----
;; y then u then v loop
;; adjust for u or v
    mov eax, top_pitch
    shr eax, 1
    mov toppitch, eax
 
    mov eax, src_pitch
    shr eax, 1
    mov spitch, eax
 
    mov eax, dst_pitch
    shr eax, 1
    mov dpitch, eax

    mov eax, height
    shr eax, 1
    mov lines, eax

    mov eax, width
    shr eax, 1
    mov pels, eax

    inc ecx
    cmp ecx, 3
    jl  near yuv_loop

;;-----


;;------
exit:
    xor eax, eax    ;; return success

    add esp, ntmps*4
    pop ebx
    pop ecx
    pop edi
    pop esi
    pop ebp

    emms

    ret


        
%ifdef ELF
;;
;; If we do not have this section here then the GNU
;; tools will believe we need an executable stack and
;; mark the DLL as such. 
section .note.GNU-stack
previous        
%endif

;_I420andI420toI420_MMX_sub endp
;====================================
;_TEXT ENDS
;====================================
          END
