        CODE32

        AREA text, CODE, READONLY

RATCoreMono PROC
        STMFD    sp!,{r0-r11,lr}
        LDR      r1,[r3,#0x18]
        LDR      r6,[r3,#0x2c]
        LDR      r7,[r3,#0x30]
        MOV      r12,r0
        ADD      r5,r12,r1,LSL #1
        MOV      r0,r2
        SUB      sp,sp,#0xc
L1_32
        TST      r6,#3
        BNE      L1_32
L1_40
        TST      r7,#3
        BNE      L1_40
        LDR      r1,[sp,#0x10]
        CMP      r5,r1
        BCS      L1_248
        ADD      r1,r3,#0x44
        STR      r1,[sp,#8]
        ADD      r1,r3,#0x38
        STR      r1,[sp,#4]
L1_76
        LDR      r1,[r3,#0xc]
        SUB      r10,r5,#2
        MOV      r8,#0x4000
        STR      r1,[sp,#0]
        MOVS     r9,r1,ASR #1
        BEQ      L1_148
L1_100
        LDRH     r11,[r5],#2
        LDR      r1,[r7],#4
        LDRH     r12,[r5],#2
        LDRH     r4,[r10],#-2
        SMLABB   r8,r11,r1,r8
        LDR      r2,[r6],#4
        SMLABT   r1,r12,r1,r8
        LDRH     lr,[r10],#-2
        SMLABB   r1,r4,r2,r1
        SUBS     r9,r9,#1
        SMLABT   r8,lr,r2,r1
        BNE      L1_100
L1_148
        LDR      r1,[sp,#0]
        TST      r1,#1
        BEQ      L1_184
        LDRH     r1,[r5],#2
        LDRH     r2,[r7],#2
        LDRH     r12,[r10,#0]
        LDRH     lr,[r6],#2
        SMLABB   r1,r1,r2,r8
        SMLABB   r8,r12,lr,r1
L1_184
        QADD     r1,r8,r8
        MOV      r1,r1,ASR #16
        STRH     r1,[r0],#2
        LDR      r1,[r3,#0x34]
        CMP      r1,r6
        LDRCS    r1,[sp,#4]
        LDRCC    r1,[sp,#8]
        LDR      r2,[r1,#0]
        ADD      r6,r6,r2,LSL #1
        LDR      r2,[r1,#4]
        LDR      r1,[r1,#8]
        ADD      r7,r7,r2,LSL #1
        ADD      r5,r5,r1,LSL #1
        LDR      r1,[sp,#0x10]
        CMP      r5,r1
        BCC      L1_76
L1_248
        LDR      r1,[sp,#0x10]
        SUB      r1,r5,r1
        MOV      r1,r1,ASR #1
        STR      r1,[r3,#0x18]!
        STR      r6,[r3,#0x14]
        STR      r7,[r3,#0x18]
        ADD      sp,sp,#0x1c
        LDMFD    sp!,{r4-r11,pc}
        ENDP

RATCoreStereo PROC
        STMFD    sp!,{r0-r11,lr}
        LDR      r1,[r3,#0x18]
        LDR      r6,[r3,#0x2c]
        LDR      r7,[r3,#0x30]
        SUB      sp,sp,#0xc
        ADD      r5,r0,r1,LSL #1
L1_304
        TST      r6,#3
        BNE      L1_304
L1_312
        TST      r7,#3
        BNE      L1_312
L1_320
        TST      r5,#3
        BNE      L1_320
        LDR      r1,[sp,#0x10]
        ADD      r0,r5,#2
        CMP      r0,r1
        BCS      L1_588
        ADD      r0,r3,#0x44
        STR      r0,[sp,#8]
        ADD      r0,r3,#0x38
        STR      r0,[sp,#4]
L1_360
        LDR      r0,[r3,#0xc]
        MOV      r8,#0x4000
        MOV      r9,r8
        SUB      r11,r5,#4
        STR      r0,[sp,#0]
        MOVS     r10,r0,ASR #1
        BEQ      L1_452
L1_388
        LDR      r2,[r5],#4
        LDR      r0,[r7],#4
        LDR      r12,[r5],#4
        LDR      r4,[r11],#-4
        SMLABB   r8,r2,r0,r8
        SMLATB   r2,r2,r0,r9
        LDR      r1,[r6],#4
        SMLABT   r8,r12,r0,r8
        SMLATT   r0,r12,r0,r2
        LDR      lr,[r11],#-4
        SMLABB   r2,r4,r1,r8
        SMLATB   r0,r4,r1,r0
        SUBS     r10,r10,#1
        SMLABT   r8,lr,r1,r2
        SMLATT   r9,lr,r1,r0
        BNE      L1_388
L1_452
        LDR      r0,[sp,#0]
        TST      r0,#1
        BEQ      L1_496
        LDR      r0,[r5],#4
        LDRH     r1,[r7],#2
        LDR      r2,[r11,#0]
        LDRH     r12,[r6],#2
        SMLABB   lr,r0,r1,r8
        SMLATB   r0,r0,r1,r9
        SMLABB   r8,r2,r12,lr
        SMLATB   r9,r2,r12,r0
L1_496
        QADD     r0,r8,r8
        LDR      r2,[sp,#0x14]
        QADD     r1,r9,r9
        MOV      r0,r0,ASR #16
        STRH     r0,[r2,#0]
        MOV      r1,r1,ASR #16
        STRH     r1,[r2,#2]
        ADD      r0,r2,#4
        STR      r0,[sp,#0x14]
        LDR      r0,[r3,#0x34]
        CMP      r0,r6
        LDRCS    r0,[sp,#4]
        LDRCC    r0,[sp,#8]
        LDR      r1,[r0,#0]
        ADD      r6,r6,r1,LSL #1
        LDR      r1,[r0,#4]
        LDR      r0,[r0,#8]
        ADD      r7,r7,r1,LSL #1
        ADD      r5,r5,r0,LSL #1
        LDR      r1,[sp,#0x10]
        ADD      r0,r5,#2
        CMP      r0,r1
        BCC      L1_360
L1_588
        LDR      r1,[sp,#0x10]
        SUB      r0,r5,r1
        MOV      r0,r0,ASR #1
        STR      r0,[r3,#0x18]!
        STR      r6,[r3,#0x14]
        STR      r7,[r3,#0x18]
        LDR      r0,[sp,#0x14]
        ADD      sp,sp,#0x1c
        LDMFD    sp!,{r4-r11,pc}
        ENDP

ARBCoreMono PROC
        STMFD    sp!,{r0-r11,lr}
        LDR      r1,[r3,#0x2c]
        LDR      r2,[r3,#0x30]
        LDR      r4,[r3,#0x1c]
        SUB      sp,sp,#0x24
        ADD      lr,r3,#0x60
        STR      r4,[sp,#0]
        LDR      r12,[r3,#0x18]
        ADD      r11,r0,r12,LSL #1
        LDR      r0,[r3,#0x50]
        STR      lr,[sp,#0x20]
        ADD      lr,r3,#0x54
        STR      lr,[sp,#0x1c]
        CMP      r0,r1
        LDRHI    lr,[sp,#0x1c]
        LDRLS    lr,[sp,#0x20]
        LDR      r0,[lr,#0]
        LDR      r12,[lr,#4]
        LDR      r6,[lr,#8]
        ADD      r0,r1,r0,LSL #1
        ADD      r12,r2,r12,LSL #1
        STR      r6,[sp,#8]
L1_712
        TST      r1,#3
        BNE      L1_712
L1_720
        TST      r2,#3
        BNE      L1_720
L1_728
        TST      r0,#3
        BNE      L1_728
L1_736
        TST      r12,#3
        BNE      L1_736
        LDR      r6,[sp,#8]
        LDR      r4,[sp,#0x28]
        ADD      lr,r11,r6,LSL #1
        CMP      lr,r4
        BCS      L1_1288
        ADD      lr,r3,#0x44
        STR      lr,[sp,#0x18]
        ADD      lr,r3,#0x38
        STR      lr,[sp,#0x14]
L1_780
        LDR      r6,[sp,#8]
        MOV      r4,#0x4000
        MOV      r5,r4
        CMP      r6,#0
        LDR      r6,[r3,#0xc]
        SUB      lr,r11,#2
        BNE      L1_948
        STR      r6,[sp,#0x10]
        MOVS     r10,r6,ASR #1
        BEQ      L1_892
L1_820
        LDRH     r8,[r11],#2
        LDR      r9,[r2],#4
        LDR      r7,[r12],#4
        LDRH     r6,[r11],#2
        SMLABB   r5,r8,r9,r5
        SMLABB   r4,r8,r7,r4
        SMLABT   r8,r6,r9,r5
        SMLABT   r9,r6,r7,r4
        LDRH     r5,[lr],#-2
        LDRH     r4,[lr],#-2
        LDR      r6,[r1],#4
        LDR      r7,[r0],#4
        SUBS     r10,r10,#1
        SMLABB   r8,r5,r6,r8
        SMLABB   r9,r5,r7,r9
        SMLABT   r5,r4,r6,r8
        SMLABT   r4,r4,r7,r9
        BNE      L1_820
L1_892
        LDR      r6,[sp,#0x10]
        TST      r6,#1
        BEQ      L1_1112
        LDRH     r6,[r11],#2
        LDRH     r7,[r2],#2
        LDRH     r8,[r12],#2
        LDRH     lr,[lr,#0]
        SMLABB   r5,r6,r7,r5
        LDRH     r7,[r1],#2
        SMLABB   r4,r6,r8,r4
        LDRH     r6,[r0],#2
        SMLABB   r5,lr,r7,r5
        SMLABB   r4,lr,r6,r4
        B        L1_1112
L1_948
        STR      r6,[sp,#0xc]
        MOVS     r6,r6,ASR #1
        STR      r6,[sp,#4]
        BEQ      L1_1052
L1_964
        LDRH     r10,[r11],#2
        LDRH     r7,[r11],#2
        LDR      r9,[r2],#4
        LDR      r6,[r12],#4
        LDRH     r8,[r11,#0]
        SMLABB   r5,r10,r9,r5
        SMLABB   r4,r7,r6,r4
        SMLABT   r7,r7,r9,r5
        LDRH     r10,[lr,#2]
        SMLABT   r8,r8,r6,r4
        LDRH     r4,[lr],#-2
        LDRH     r9,[lr],#-2
        LDR      r5,[r1],#4
        LDR      r6,[r0],#4
        SMLABB   r7,r4,r5,r7
        SMLABB   r8,r10,r6,r8
        SMLABT   r5,r9,r5,r7
        SMLABT   r4,r4,r6,r8
        LDR      r6,[sp,#4]
        SUBS     r6,r6,#1
        STR      r6,[sp,#4]
        BNE      L1_964
L1_1052
        LDR      r6,[sp,#0xc]
        TST      r6,#1
        BEQ      L1_1112
        LDRH     r6,[r11],#2
        LDRH     r8,[r2],#2
        LDRH     r7,[r11,#0]
        LDRH     r9,[r12],#2
        SMLABB   r5,r6,r8,r5
        LDRH     r8,[r1],#2
        SMLABB   r4,r7,r9,r4
        LDRH     r7,[lr,#0]
        LDRH     r6,[lr,#2]
        LDRH     lr,[r0],#2
        SMLABB   r5,r7,r8,r5
        SMLABB   r4,r6,lr,r4
L1_1112
        SUB      lr,r4,r5
        LDR      r4,[sp,#0]
        MOV      r4,r4,LSR #1
        SMULL    r6,lr,r4,lr
        LDR      r4,[sp,#0x2c]
        QDADD    lr,r5,lr
        QADD     lr,lr,lr
        MOV      lr,lr,ASR #16
        STRH     lr,[r4],#2
        STR      r4,[sp,#0x2c]
        LDR      lr,[r3,#0x20]
        LDR      r4,[sp,#0]
        ADD      r4,lr,r4
        STR      r4,[sp,#0]
        CMP      lr,r4
        BLS      L1_1192
        LDR      r6,[sp,#8]
        MOV      r1,r0
        MOV      r2,r12
        ADD      r11,r11,r6,LSL #1
L1_1192
        LDR      r0,[r3,#0x34]
        CMP      r0,r1
        LDRCS    r0,[sp,#0x14]
        LDRCC    r0,[sp,#0x18]
        LDR      r12,[r0,#0]
        ADD      r1,r1,r12,LSL #1
        LDR      r12,[r0,#4]
        LDR      r0,[r0,#8]
        ADD      r2,r2,r12,LSL #1
        ADD      r11,r11,r0,LSL #1
        LDR      r0,[r3,#0x50]
        CMP      r0,r1
        LDRHI    lr,[sp,#0x1c]
        LDRLS    lr,[sp,#0x20]
        LDR      r0,[lr,#0]
        LDR      r12,[lr,#4]
        LDR      r6,[lr,#8]
        ADD      r0,r1,r0,LSL #1
        ADD      r12,r2,r12,LSL #1
        STR      r6,[sp,#8]
        LDR      r4,[sp,#0x28]
        ADD      lr,r11,r6,LSL #1
        CMP      lr,r4
        BCC      L1_780
L1_1288
        LDR      r4,[sp,#0x28]
        SUB      r0,r11,r4
        MOV      r0,r0,ASR #1
        STR      r0,[r3,#0x18]
        STR      r1,[r3,#0x2c]
        STR      r2,[r3,#0x30]
        LDR      r4,[sp,#0]
        STR      r4,[r3,#0x1c]
        LDR      r0,[sp,#0x2c]
        ADD      sp,sp,#0x34
        LDMFD    sp!,{r4-r11,pc}
        ENDP

ARBCoreStereo PROC
        STMFD    sp!,{r0-r11,lr}
        SUB      sp,sp,#0x1c
        LDR      r3,[sp,#0x28]
        LDR      r1,[r3,#0x2c]
        LDR      r2,[r3,#0x30]
        LDR      r3,[r3,#0x1c]
        STR      r3,[sp,#0]
        LDR      r3,[sp,#0x28]
        LDR      r3,[r3,#0x18]
        ADD      r10,r0,r3,LSL #1
        LDR      r3,[sp,#0x28]
        LDR      r0,[r3,#0x50]!
        ADD      r3,r3,#0x10
        STR      r3,[sp,#0x18]
        LDR      r3,[sp,#0x28]
        CMP      r0,r1
        ADD      r3,r3,#0x54
        STR      r3,[sp,#0x14]
        LDRHI    r3,[sp,#0x14]
        LDRLS    r3,[sp,#0x18]
        LDR      r0,[r3,#0]
        LDR      r12,[r3,#4]
        LDR      r3,[r3,#8]
        ADD      r0,r1,r0,LSL #1
        ADD      r12,r2,r12,LSL #1
        STR      r3,[sp,#8]
L1_1436
        TST      r1,#3
        BNE      L1_1436
L1_1444
        TST      r2,#3
        BNE      L1_1444
L1_1452
        TST      r0,#3
        BNE      L1_1452
L1_1460
        TST      r12,#3
        BNE      L1_1460
L1_1468
        TST      r10,#3
        BNE      L1_1468
        LDR      r3,[sp,#8]
        LDR      lr,[sp,#0x20]
        ADD      r3,r10,r3,LSL #1
        ADD      r3,r3,#2
        CMP      r3,lr
        BCS      L1_1968
        LDR      r3,[sp,#0x28]
        ADD      r3,r3,#0x44
        STR      r3,[sp,#0x10]
        LDR      r3,[sp,#0x28]
        ADD      r3,r3,#0x38
        STR      r3,[sp,#0xc]
L1_1524
        LDR      r3,[sp,#8]
        MOV      lr,#0x4000
        MOV      r7,lr
        CMP      r3,#0
        LDR      r3,[sp,#0x28]
        MOV      r6,lr
        MOV      r4,lr
        SUB      r5,r10,#4
        LDR      r3,[r3,#0xc]
        BNE      L1_1648
        CMP      r3,#0
        BEQ      L1_1748
L1_1572
        LDRH     r11,[r12],#2
        LDRH     r9,[r2],#2
        LDR      r8,[r10],#4
        SUBS     r3,r3,#1
        ORR      r9,r9,r11,LSL #16
        SMLABB   lr,r8,r9,lr
        SMLATB   r4,r8,r9,r4
        SMLABT   r6,r8,r9,r6
        SMLATT   r9,r8,r9,r7
        LDRH     r11,[r0],#2
        LDRH     r8,[r1],#2
        LDR      r7,[r5],#-4
        ORR      r8,r8,r11,LSL #16
        SMLABB   lr,r7,r8,lr
        SMLATB   r4,r7,r8,r4
        SMLABT   r6,r7,r8,r6
        SMLATT   r7,r7,r8,r9
        BNE      L1_1572
        B        L1_1748
L1_1648
        STR      r3,[sp,#4]
        CMP      r3,#0
        BEQ      L1_1748
L1_1660
        LDRH     r11,[r12],#2
        LDRH     r3,[r2],#2
        LDR      r9,[r10],#4
        LDR      r8,[r10,#0]
        ORR      r3,r3,r11,LSL #16
        SMLABB   lr,r9,r3,lr
        SMLABT   r6,r8,r3,r6
        SMLATT   r8,r8,r3,r7
        SMLATB   r4,r9,r3,r4
        LDRH     r11,[r0],#2
        LDRH     r3,[r1],#2
        LDR      r7,[r5,#4]
        LDR      r9,[r5],#-4
        ORR      r3,r3,r11,LSL #16
        SMLABT   r6,r7,r3,r6
        SMLATT   r7,r7,r3,r8
        SMLABB   lr,r9,r3,lr
        SMLATB   r4,r9,r3,r4
        LDR      r3,[sp,#4]
        SUBS     r3,r3,#1
        STR      r3,[sp,#4]
        BNE      L1_1660
L1_1748
        LDR      r3,[sp,#0]
        SUB      r5,r6,lr
        MOV      r3,r3,LSR #1
        SMULL    r6,r5,r3,r5
        QDADD    lr,lr,r5
        SUB      r5,r7,r4
        SMULL    r6,r5,r3,r5
        QADD     lr,lr,lr
        QDADD    r3,r4,r5
        LDR      r4,[sp,#0x24]
        QADD     r3,r3,r3
        MOV      lr,lr,ASR #16
        STRH     lr,[r4,#0]
        MOV      r3,r3,ASR #16
        STRH     r3,[r4,#2]
        LDR      r3,[sp,#0x28]
        ADD      r4,r4,#4
        STR      r4,[sp,#0x24]
        LDR      r3,[r3,#0x20]
        LDR      lr,[sp,#0]
        ADD      lr,r3,lr
        STR      lr,[sp,#0]
        CMP      r3,lr
        BLS      L1_1860
        LDR      r3,[sp,#8]
        MOV      r1,r0
        MOV      r2,r12
        ADD      r10,r10,r3,LSL #1
L1_1860
        LDR      r3,[sp,#0x28]
        LDR      r0,[r3,#0x34]
        CMP      r0,r1
        LDRCS    r0,[sp,#0xc]
        LDRCC    r0,[sp,#0x10]
        LDR      r3,[r0,#0]
        ADD      r1,r1,r3,LSL #1
        LDR      r3,[r0,#4]
        LDR      r0,[r0,#8]
        ADD      r2,r2,r3,LSL #1
        ADD      r10,r10,r0,LSL #1
        LDR      r3,[sp,#0x28]
        LDR      r0,[r3,#0x50]
        CMP      r0,r1
        LDRHI    r3,[sp,#0x14]
        LDRLS    r3,[sp,#0x18]
        LDR      r0,[r3,#0]
        LDR      r12,[r3,#4]
        LDR      r3,[r3,#8]
        LDR      lr,[sp,#0x20]
        ADD      r0,r1,r0,LSL #1
        STR      r3,[sp,#8]
        ADD      r12,r2,r12,LSL #1
        ADD      r3,r10,r3,LSL #1
        ADD      r3,r3,#2
        CMP      r3,lr
        BCC      L1_1524
L1_1968
        LDR      lr,[sp,#0x20]
        LDR      r3,[sp,#0x28]
        SUB      r0,r10,lr
        MOV      r0,r0,ASR #1
        STR      r0,[r3,#0x18]
        LDR      r3,[sp,#0x28]
        STR      r1,[r3,#0x2c]
        LDR      r3,[sp,#0x28]
        STR      r2,[r3,#0x30]
        LDR      r0,[sp,#0x28]
        LDR      r3,[sp,#0]
        STR      r3,[r0,#0x1c]
        LDR      r0,[sp,#0x24]
        ADD      sp,sp,#0x2c
        LDMFD    sp!,{r4-r11,pc}
        ENDP



        EXPORT ARBCoreStereo
        EXPORT ARBCoreMono
        EXPORT RATCoreStereo
        EXPORT RATCoreMono



        END
