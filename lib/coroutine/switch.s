.globl __GetStack__
__GetStack__:
    mov %rsp, (%rdi)
    mov %rbp, (%rsi)
    //�򵥴�����
    ret

.globl __SwitchCoroutine__
__SwitchCoroutine__:
    //����rsp
	mov %rsp, %rax
	//����1�Ƶ�rsp
    mov %rdi, %rsp
	//rspָ��contextĩβ
    add $136, %rsp
	//raxֵ��rsp,rsp��ֵ�����ڴ�Ѱַ�õ�rip,Ҳ���Ƿ��ص�ַ
    push (%rax)
	//��8�õ��ϸ�����ջ֡��rsp
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
