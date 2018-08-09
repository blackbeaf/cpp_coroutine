#include "coroutine.h"

#include <stdlib.h>
//////////////////////////////////////////////////////////////////////////

#define OFFSET(t, m) (&(((t*)0)->m))


/************************************************************************/
/* global var                                                                     */
/************************************************************************/
uint32_t g_stack_size = 100 * 1024;


//////////////////////////////////////////////////////////////////////////

Coroutine* CreateCoroutine(EntryCallback entry, void *arg)
{
    int size = g_stack_size + sizeof(struct stContext);
    struct stContext *c = calloc(size, 1);

    if (NULL == c)
    {
        return NULL;
    }

    uint8_t *start = (uint8_t*)c;
    c->arg = arg;
    //函数入口
    c->cpu_register.rip = (uint64_t)entry;
    //第一个参数
    c->cpu_register.rdi = (uint64_t)arg;
    //rbp 栈底
    //c->cpu_register.rbp = (uint64_t)(start + size);
    c->cpu_register.rbp = 0;
    //rsp 当前栈
    c->cpu_register.rsp = (uint64_t)(start + g_stack_size);
    //*(uint64_t*)c->cpu_register.rsp = 0x00007ffff72c7d5d;
    return c;
}


void DeleteCoroutine(Coroutine *ptr)
{
    free(ptr);
}

void SetStackSize(uint32_t size)
{
    g_stack_size = size;
}

