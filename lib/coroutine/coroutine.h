#pragma once
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////


typedef int64_t (*EntryCallback)(void*);

struct stRegister
{
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;

    uint64_t rsi;
    uint64_t rdi;

    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t rbp;
    uint64_t rsp;

    uint64_t rip;
};


struct stContext
{
    struct stRegister cpu_register;
    void *arg;
    uint8_t *stack;
};

typedef struct stContext Coroutine;

Coroutine* CreateCoroutine(EntryCallback entry, void *arg);
void DeleteCoroutine(Coroutine *ptr);

void SetStackSize(uint32_t size);

void __SwitchCoroutine__(Coroutine *cur, const Coroutine *next);

void __GetStack__(uint64_t *rsp, uint64_t *rbp);