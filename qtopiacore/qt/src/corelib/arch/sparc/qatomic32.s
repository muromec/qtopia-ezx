	.section ".text"

	.align 4
	.type q_atomic_lock_int,#function
	.global q_atomic_lock_int
q_atomic_lock_int:
	sethi   %hi(-2147483648),%o3
.q_atomic_lock_int_try:
	mov %o3,%o2
        swap [%o0],%o2
	cmp %o2,%o3
	bne .q_atomic_lock_int_done
	nop
.q_atomic_lock_int_spin:
	ld [%o0],%o2
	cmp %o2,%o3
	be .q_atomic_lock_int_spin
	nop
	ba .q_atomic_lock_int_try
	nop
.q_atomic_lock_int_done:
	retl
	mov %o2,%o0
	.size q_atomic_lock_int,.-q_atomic_lock_int




	.align 4
	.type q_atomic_lock_ptr,#function
	.global q_atomic_lock_ptr
q_atomic_lock_ptr:
.q_atomic_lock_ptr_try:
	mov -1,%o2
        swap [%o0],%o2
        cmp %o2,-1
        bne .q_atomic_lock_ptr_done
        nop
.q_atomic_lock_ptr_spin:
        ld [%o0],%o2
        cmp %o2,-1
        be .q_atomic_lock_ptr_spin
        nop
	ba .q_atomic_lock_ptr_try
	nop
.q_atomic_lock_ptr_done:
	retl
        mov %o2,%o0
	.size q_atomic_lock_ptr,.-q_atomic_lock_ptr




	.align 4
	.type q_atomic_unlock,#function
	.global q_atomic_unlock
q_atomic_unlock:
	stbar
	retl
	st %o1,[%o0]
	.size q_atomic_unlock,.-q_atomic_unlock




	.align 4
	.type q_atomic_set_int,#function
	.global q_atomic_set_int
q_atomic_set_int:
	swap [%o0],%o1
	retl
	mov %o1,%o0
	.size q_atomic_set_int,.-q_atomic_set_int




	.align 4
	.type q_atomic_set_ptr,#function
	.global q_atomic_set_ptr
q_atomic_set_ptr:
	swap [%o0],%o1
	retl
	mov %o1,%o0
	.size q_atomic_set_ptr,.-q_atomic_set_ptr

