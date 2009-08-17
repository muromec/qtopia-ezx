        CODE32

        AREA text, CODE, READONLY

RATCoreMono PROC
        STMFD    sp!,{r4-r11,lr}
        MOV      r4,r0
        MOV      r0,r2
        LDR      r2,[r3,#0x18]
        LDR      r12,[r3,#0x2c]
        ADD      r2,r4,r2,LSL #1
        CMP      r2,r1
        LDR      lr,[r3,#0x30]
        BCS      L1_224
        ADD      r10,r3,#0x44
        ADD      r11,r3,#0x38
L1_44
        LDR      r9,[r3,#0xc]
        SUB      r5,r2,#2
        MOV      r6,#0x4000
        MOVS     r4,r9,ASR #1
        BEQ      L1_120
L1_64
        LDRSH    r7,[r2],#2
        LDRSH    r8,[lr],#2
        SUBS     r4,r4,#1
        MLA      r6,r8,r7,r6
        LDRSH    r7,[r2],#2
        LDRSH    r8,[lr],#2
        MLA      r6,r8,r7,r6
        LDRSH    r7,[r5],#-2
        LDRSH    r8,[r12],#2
        MLA      r6,r8,r7,r6
        LDRSH    r7,[r5],#-2
        LDRSH    r8,[r12],#2
        MLA      r6,r8,r7,r6
        BNE      L1_64
L1_120
        TST      r9,#1
        BEQ      L1_152
        LDRSH    r4,[r2],#2
        LDRSH    r7,[lr],#2
        LDRSH    r5,[r5,#0]
        MLA      r4,r7,r4,r6
        LDRSH    r7,[r12],#2
        MLA      r6,r7,r5,r4
L1_152
        MOV      r4,r6,ASR #15
        MOV      r5,r4,ASR #31
        CMP      r5,r4,ASR #15
        EORNE    r4,r5,#0x7f00
        EORNE    r4,r4,#0xff
        STRH     r4,[r0],#2
        LDR      r4,[r3,#0x34]
        CMP      r4,r12
        MOVCS    r4,r11
        MOVCC    r4,r10
        LDR      r5,[r4,#0]
        ADD      r12,r12,r5,LSL #1
        LDR      r5,[r4,#4]
        LDR      r4,[r4,#8]
        ADD      lr,lr,r5,LSL #1
        ADD      r2,r2,r4,LSL #1
        CMP      r2,r1
        BCC      L1_44
L1_224
        SUB      r1,r2,r1
        MOV      r1,r1,ASR #1
        STR      r1,[r3,#0x18]!
        STR      r12,[r3,#0x14]
        STR      lr,[r3,#0x18]
        LDMFD    sp!,{r4-r11,pc}
        ENDP

RATCoreStereo PROC
        STMFD    sp!,{r3-r11,lr}
        MOV      r12,r0
        LDR      lr,[r3,#0x18]
        MOV      r0,r2
        ADD      r6,r12,lr,LSL #1
        ADD      r12,r6,#2
        LDR      r2,[r3,#0x2c]
        LDR      r4,[r3,#0x30]
        CMP      r12,r1
        BCS      L1_564
        ADD      r12,r3,#0x44
        STR      r12,[sp,#0]
        ADD      r11,r3,#0x38
L1_300
        LDR      r10,[r3,#0xc]
        MOV      r5,#0x4000
        MOV      r7,r5
        SUB      lr,r6,#2
        MOVS     r12,r10,ASR #1
        BEQ      L1_412
L1_324
        LDRSH    r9,[r6],#2
        LDRSH    r8,[r4],#2
        SUBS     r12,r12,#1
        MLA      r7,r8,r9,r7
        LDRSH    r9,[r6],#2
        MLA      r8,r9,r8,r5
        LDRSH    r9,[r6],#2
        LDRSH    r5,[r4],#2
        MLA      r7,r5,r9,r7
        LDRSH    r9,[r6],#2
        MLA      r5,r9,r5,r8
        LDRSH    r9,[lr],#-2
        LDRSH    r8,[r2],#2
        MLA      r5,r8,r9,r5
        LDRSH    r9,[lr],#-2
        MLA      r7,r8,r9,r7
        LDRSH    r9,[lr],#-2
        LDRSH    r8,[r2],#2
        MLA      r5,r8,r9,r5
        LDRSH    r9,[lr],#-2
        MLA      r7,r8,r9,r7
        BNE      L1_324
L1_412
        TST      r10,#1
        BEQ      L1_464
        LDRSH    r12,[r6],#2
        LDRSH    r8,[r4],#2
        MLA      r12,r8,r12,r7
        LDRSH    r7,[r6],#2
        MLA      r5,r8,r7,r5
        SUB      r7,lr,#2
        LDRSH    r8,[lr,#0]
        LDRSH    lr,[r2],#2
        LDRSH    r7,[r7,#0]
        MLA      r5,lr,r8,r5
        MLA      r7,lr,r7,r12
L1_464
        MOV      r12,r7,ASR #15
        MOV      lr,r5,ASR #15
        MOV      r5,r12,ASR #31
        CMP      r5,r12,ASR #15
        EORNE    r12,r5,#0x7f00
        EORNE    r12,r12,#0xff
        MOV      r5,lr,ASR #31
        CMP      r5,lr,ASR #15
        EORNE    lr,r5,#0x7f00
        EORNE    lr,lr,#0xff
        STRH     r12,[r0],#2
        STRH     lr,[r0],#2
        LDR      r12,[r3,#0x34]
        CMP      r12,r2
        LDRCC    r12,[sp,#0]
        MOVCS    r12,r11
        LDR      lr,[r12,#0]
        ADD      r2,r2,lr,LSL #1
        LDR      lr,[r12,#4]
        LDR      r12,[r12,#8]
        ADD      r4,r4,lr,LSL #1
        ADD      r6,r6,r12,LSL #1
        ADD      r12,r6,#2
        CMP      r12,r1
        BCC      L1_300
L1_564
        SUB      r1,r6,r1
        MOV      r1,r1,ASR #1
        STR      r1,[r3,#0x18]!
        STR      r2,[r3,#0x14]
        STR      r4,[r3,#0x18]
        LDMFD    sp!,{r3-r11,pc}
        ENDP

ARBCoreMono PROC
        STMFD    sp!,{r0-r11,lr}
        LDR      r1,[r3,#0x18]
        LDR      r12,[r3,#0x2c]
        ADD      r2,r0,r1,LSL #1
        LDR      r0,[r3,#0x50]
        LDR      r10,[r3,#0x1c]
        LDR      lr,[r3,#0x30]
        ADD      r1,r3,#0x60
        SUB      sp,sp,#0x14
        STR      r1,[sp,#0x10]
        CMP      r0,r12
        ADD      r1,r3,#0x54
        STR      r1,[sp,#0xc]
        LDRHI    r0,[sp,#0xc]
        LDRLS    r0,[sp,#0x10]
        LDR      r1,[r0,#0]
        ADD      r6,r12,r1,LSL #1
        LDR      r1,[r0,#4]
        LDR      r0,[r0,#8]
        ADD      r7,lr,r1,LSL #1
        LDR      r1,[sp,#0x18]
        STR      r0,[sp,#0]
        ADD      r0,r2,r0,LSL #1
        CMP      r0,r1
        BCS      L1_1032
        ADD      r0,r3,#0x44
        STR      r0,[sp,#8]
        ADD      r0,r3,#0x38
        STR      r0,[sp,#4]
L1_704
        LDR      r0,[sp,#0]
        MOV      r4,#0x4000
        MOV      r8,r4
        LDR      r9,[r3,#0xc]
        SUB      r5,r2,#2
        CMP      r0,#0
        BNE      L1_792
        CMP      r9,#0
        BEQ      L1_856
L1_740
        LDRSH    r0,[r2],#2
        LDRSH    r11,[lr],#2
        LDRSH    r1,[r5],#-2
        MLA      r4,r11,r0,r4
        LDRSH    r11,[r7],#2
        SUBS     r9,r9,#1
        MLA      r0,r11,r0,r8
        LDRSH    r8,[r12],#2
        LDRSH    r11,[r6],#2
        MLA      r4,r8,r1,r4
        MLA      r8,r11,r1,r0
        BNE      L1_740
        B        L1_856
L1_792
        CMP      r9,#0
        BEQ      L1_856
L1_800
        LDRSH    r0,[r2],#2
        LDRSH    r11,[lr],#2
        LDRSH    r1,[r2,#0]
        MLA      r0,r11,r0,r4
        LDRSH    r4,[r7],#2
        LDRSH    r11,[r12],#2
        MLA      r8,r4,r1,r8
        LDRSH    r1,[r5,#2]
        LDRSH    r4,[r5],#-2
        SUBS     r9,r9,#1
        MLA      r4,r11,r4,r0
        LDRSH    r0,[r6],#2
        MLA      r8,r0,r1,r8
        BNE      L1_800
L1_856
        SUB      r1,r8,r4
        MOV      r0,r10,LSR #1
        SMULL    r5,r1,r0,r1
        ADD      r0,r1,r4,ASR #1
        MOV      r0,r0,ASR #14
        MOV      r1,r0,ASR #31
        CMP      r1,r0,ASR #15
        EORNE    r0,r1,#0x7f00
        LDR      r1,[sp,#0x1c]
        EORNE    r0,r0,#0xff
        STRH     r0,[r1,#0]
        ADD      r0,r1,#2
        STR      r0,[sp,#0x1c]
        LDR      r0,[r3,#0x20]
        ADDS     r10,r0,r10
        BCC      L1_936
        LDR      r0,[sp,#0]
        MOV      r12,r6
        MOV      lr,r7
        ADD      r2,r2,r0,LSL #1
L1_936
        LDR      r0,[r3,#0x34]
        CMP      r0,r12
        LDRCS    r0,[sp,#4]
        LDRCC    r0,[sp,#8]
        LDR      r1,[r0,#0]
        ADD      r12,r12,r1,LSL #1
        LDR      r1,[r0,#4]
        LDR      r0,[r0,#8]
        ADD      lr,lr,r1,LSL #1
        ADD      r2,r2,r0,LSL #1
        LDR      r0,[r3,#0x50]
        CMP      r0,r12
        LDRHI    r0,[sp,#0xc]
        LDRLS    r0,[sp,#0x10]
        LDR      r1,[r0,#0]
        ADD      r6,r12,r1,LSL #1
        LDR      r1,[r0,#4]
        LDR      r0,[r0,#8]
        ADD      r7,lr,r1,LSL #1
        LDR      r1,[sp,#0x18]
        STR      r0,[sp,#0]
        ADD      r0,r2,r0,LSL #1
        CMP      r0,r1
        BCC      L1_704
L1_1032
        LDR      r1,[sp,#0x18]
        ADD      r3,r3,#0x18
        SUB      r0,r2,r1
        MOV      r0,r0,ASR #1
        STMIA    r3!,{r0,r10}
        STR      r12,[r3,#0xc]
        STR      lr,[r3,#0x10]
        LDR      r0,[sp,#0x1c]
        ADD      sp,sp,#0x24
        LDMFD    sp!,{r4-r11,pc}
        ENDP

ARBCoreStereo PROC
        STMFD    sp!,{r0-r11,lr}
        LDR      r1,[r3,#0x2c]
        LDR      r2,[r3,#0x30]
        LDR      lr,[r3,#0x1c]
        SUB      sp,sp,#0x1c
        STR      lr,[sp,#0]
        LDR      r12,[r3,#0x18]
        ADD      lr,r3,#0x60
        ADD      r10,r0,r12,LSL #1
        LDR      r0,[r3,#0x50]
        STR      lr,[sp,#0x18]
        ADD      lr,r3,#0x54
        STR      lr,[sp,#0x14]
        CMP      r0,r1
        LDRHI    lr,[sp,#0x14]
        LDRLS    lr,[sp,#0x18]
        LDR      r0,[lr,#0]
        LDR      r12,[lr,#4]
        LDR      r6,[lr,#8]
        LDR      r4,[sp,#0x20]
        ADD      lr,r10,r6,LSL #1
        ADD      lr,lr,#2
        CMP      lr,r4
        ADD      r0,r1,r0,LSL #1
        ADD      r12,r2,r12,LSL #1
        STR      r6,[sp,#8]
        BCS      L1_1656
        ADD      lr,r3,#0x44
        STR      lr,[sp,#0x10]
        ADD      lr,r3,#0x38
        STR      lr,[sp,#0xc]
L1_1196
        MOV      r4,#0x4000
        LDR      r6,[sp,#8]
        MOV      r5,r4
        CMP      r6,#0
        LDR      r6,[r3,#0xc]
        MOV      r9,r4
        MOV      r8,r4
        SUB      lr,r10,#2
        BNE      L1_1328
        STR      r6,[sp,#4]
        CMP      r6,#0
        BEQ      L1_1424
L1_1244
        LDRSH    r6,[r10],#2
        LDRSH    r11,[r2],#2
        LDRSH    r7,[r10],#2
        MLA      r8,r11,r6,r8
        MLA      r9,r11,r7,r9
        LDRSH    r11,[r12],#2
        MLA      r5,r11,r7,r5
        LDRSH    r7,[lr],#-2
        MLA      r4,r11,r6,r4
        LDRSH    r11,[r0],#2
        LDRSH    r6,[lr],#-2
        MLA      r5,r11,r7,r5
        MLA      r4,r11,r6,r4
        LDRSH    r11,[r1],#2
        MLA      r8,r11,r6,r8
        LDR      r6,[sp,#4]
        MLA      r9,r11,r7,r9
        SUBS     r6,r6,#1
        STR      r6,[sp,#4]
        BNE      L1_1244
        B        L1_1424
L1_1328
        CMP      r6,#0
        BEQ      L1_1424
L1_1336
        LDRSH    r7,[r10],#2
        LDRSH    r11,[r2],#2
        SUBS     r6,r6,#1
        MLA      r7,r11,r7,r8
        LDRSH    r8,[r10],#2
        MLA      r8,r11,r8,r9
        LDRSH    r11,[r10,#0]
        LDRSH    r9,[r12],#2
        MLA      r4,r9,r11,r4
        LDRSH    r11,[r10,#2]
        MLA      r5,r9,r11,r5
        LDRSH    r11,[lr,#4]
        LDRSH    r9,[r0],#2
        MLA      r5,r9,r11,r5
        LDRSH    r11,[lr,#2]
        MLA      r4,r9,r11,r4
        LDRSH    r9,[lr],#-2
        LDRSH    r11,[r1],#2
        MLA      r9,r11,r9,r8
        LDRSH    r8,[lr],#-2
        MLA      r8,r11,r8,r7
        BNE      L1_1336
L1_1424
        LDR      lr,[sp,#0]
        SUB      r4,r4,r8
        MOV      lr,lr,LSR #1
        SMULL    r6,r4,lr,r4
        SUB      r5,r5,r9
        SMULL    r6,r5,lr,r5
        ADD      r4,r4,r8,ASR #1
        ADD      lr,r5,r9,ASR #1
        MOV      r4,r4,ASR #14
        MOV      r5,r4,ASR #31
        CMP      r5,r4,ASR #15
        EORNE    r4,r5,#0x7f00
        MOV      lr,lr,ASR #14
        MOV      r5,lr,ASR #31
        EORNE    r4,r4,#0xff
        CMP      r5,lr,ASR #15
        EORNE    lr,r5,#0x7f00
        LDR      r5,[sp,#0x24]
        EORNE    lr,lr,#0xff
        STRH     r4,[r5,#0]
        STRH     lr,[r5,#2]
        ADD      r5,r5,#4
        STR      r5,[sp,#0x24]
        LDR      lr,[r3,#0x20]
        LDR      r4,[sp,#0]
        ADD      r4,lr,r4
        STR      r4,[sp,#0]
        CMP      lr,r4
        BLS      L1_1556
        LDR      r6,[sp,#8]
        MOV      r1,r0
        MOV      r2,r12
        ADD      r10,r10,r6,LSL #1
L1_1556
        LDR      r0,[r3,#0x34]
        CMP      r0,r1
        LDRCS    r0,[sp,#0xc]
        LDRCC    r0,[sp,#0x10]
        LDR      r12,[r0,#0]
        ADD      r1,r1,r12,LSL #1
        LDR      r12,[r0,#4]
        LDR      r0,[r0,#8]
        ADD      r2,r2,r12,LSL #1
        ADD      r10,r10,r0,LSL #1
        LDR      r0,[r3,#0x50]
        CMP      r0,r1
        LDRHI    lr,[sp,#0x14]
        LDRLS    lr,[sp,#0x18]
        LDR      r0,[lr,#0]
        LDR      r12,[lr,#4]
        LDR      r6,[lr,#8]
        LDR      r4,[sp,#0x20]
        ADD      lr,r10,r6,LSL #1
        ADD      lr,lr,#2
        CMP      lr,r4
        ADD      r0,r1,r0,LSL #1
        ADD      r12,r2,r12,LSL #1
        STR      r6,[sp,#8]
        BCC      L1_1196
L1_1656
        LDR      r4,[sp,#0x20]
        SUB      r0,r10,r4
        MOV      r0,r0,ASR #1
        STR      r0,[r3,#0x18]
        STR      r1,[r3,#0x2c]
        STR      r2,[r3,#0x30]
        LDR      lr,[sp,#0]
        STR      lr,[r3,#0x1c]
        LDR      r0,[sp,#0x24]
        ADD      sp,sp,#0x2c
        LDMFD    sp!,{r4-r11,pc}
        ENDP



        EXPORT ARBCoreStereo
        EXPORT ARBCoreMono
        EXPORT RATCoreStereo
        EXPORT RATCoreMono



        END
