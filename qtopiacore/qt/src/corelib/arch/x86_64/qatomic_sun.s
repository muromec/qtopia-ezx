        .code64
        .globl q_atomic_test_and_set_ptr
        .type q_atomic_test_and_set_ptr, @function
        .section .text, "ax"
        .align 16
            
q_atomic_test_and_set_ptr:

	push 		%rbp
	movq		%rsp, %rbp
	
        movq            %rsi, %rax
        lock
        cmpxchgq        %rdx, (%rdi)
	movq		$0, %rax
        sete            %al
	
	movq		%rbp, %rsp
	pop		%rbp
        ret

        .size q_atomic_test_and_set_ptr, . - q_atomic_test_and_set_ptr

    
        .globl q_atomic_test_and_set_int
        .type q_atomic_test_and_set_int, @function
        .section .text, "ax"
        .align 16

q_atomic_test_and_set_int:
       
	push 		%rbp
	movq		%rsp, %rbp
	
        movl            %esi, %eax
        lock
        cmpxchgl        %edx, (%rdi)
	movl		$0, %eax
        sete            %al

	movq		%rbp, %rsp
	pop		%rbp
        ret
        
	.size q_atomic_test_and_set_int, . - q_atomic_test_and_set_int
        

        .globl q_atomic_fetch_and_add_int
        .type q_atomic_fetch_and_add_int, @function
        .section .text, "ax"
        .align 16

q_atomic_fetch_and_add_int:

        lock
        xaddl %esi,(%rdi)
        movl %esi, %eax
        ret

        .size q_atomic_fetch_and_add_int,.-q_atomic_fetch_and_add_int
