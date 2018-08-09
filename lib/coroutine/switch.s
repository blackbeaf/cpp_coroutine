.globl __GetStack__
__GetStack__:
    mov %rsp, (%rdi)
    mov %rbp, (%rsi)
    //简单处理返回
    ret

.globl __SwitchCoroutine__
__SwitchCoroutine__:
    //保存rsp
	mov %rsp, %rax
	//参数1移到rsp
    mov %rdi, %rsp
	//rsp指向context末尾
    add $136, %rsp
	//rax值是rsp,rsp的值进行内存寻址得到rip,也就是返回地址
    push (%rax)
	//加8得到上个函数栈帧的rsp
    add $8, %rax
    push %rax
    push %rbp
    push %r15
    push %r14
    push %r13
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %rbx
    push %rax
    mov %rsi, %rsp
    pop %rax
    pop %rbx
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rdi
    pop %r8
    pop %r9
    pop %r10
    pop %r11
    pop %r12
    pop %r13
    pop %r14
    pop %r15
    pop %rbp
    mov 8(%rsp), %rax
    pop %rsp
    jmp *%rax
