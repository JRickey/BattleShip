/**
 * coroutine_switch.cpp — Nintendo Switch (AArch64) coroutine implementation.
 *
 * Uses manual register save/restore for cooperative context switching with
 * per-coroutine stacks.  Avoids setjmp/longjmp which have undefined-behaviour
 * restrictions when the function containing setjmp is exited via longjmp.
 */
#ifdef __SWITCH__

#include "coroutine.h"
#include "port_watchdog.h"
#include "port_log.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CANARY_HEAD 0xDEADBEEFu
#define CANARY_TAIL 0xCAFEBABEu
#define MIN_STACK_SIZE 32768

/*
 * AArch64 context.  Offsets must match the stp/ldp pairs in context_switch.
 *
 * Offset  0.. 72: x19..x28   (10 registers)
 * Offset 80.. 88: x29 (fp), x30 (lr)
 * Offset 96     : sp
 * Offset 104    : x0 (first arg passed to trampoline)
 * Offset 112..168: d8..d15  (8 registers, 64-bit each)
 */
struct SwitchContext {
    uint64_t r[14];     /* x19-x28 (10), x29, x30, sp, x0 */
    uint64_t d[8];      /* d8-d15 (callee-saved SIMD/FP) */
};

struct PortCoroutine {
    unsigned int     canary_head;
    SwitchContext    ctx;
    SwitchContext    caller_ctx;
    void           (*entry)(void *);
    void            *arg;
    int              finished;
    int              started;
    char            *stack_mem;
    size_t           stack_size;
    unsigned int     canary_tail;
};

static PortCoroutine *sCurrentCoroutine = NULL;

/* Defined in port/switch/context_switch.s */
extern "C" void context_switch(SwitchContext *from, SwitchContext *to);

/* ========================================================================= */
/*  Internal helpers                                                         */
/* ========================================================================= */

static int validate_coroutine(PortCoroutine *co, const char *caller) {
    if (co == NULL) {
        fprintf(stderr, "SSB64 co [%s]: NULL coroutine\n", caller);
        return 0;
    }
    if (co->canary_head != CANARY_HEAD || co->canary_tail != CANARY_TAIL) {
        fprintf(stderr, "SSB64 co [%s]: corruption (head=0x%08X tail=0x%08X)\n",
                caller, co->canary_head, co->canary_tail);
        return 0;
    }
    return 1;
}

/* ========================================================================= */
/*  Public API                                                               */
/* ========================================================================= */

__attribute__((noinline, used))
static void coroutine_trampoline(PortCoroutine *co) {
    co->entry(co->arg);
    co->finished = 1;
    sCurrentCoroutine = NULL;
    context_switch(&co->ctx, &co->caller_ctx);
}

void port_coroutine_init_main(void) {
    /* No-op. */
}

PortCoroutine *port_coroutine_create(void (*entry)(void *), void *arg,
                                     size_t stack_size) {
    if (stack_size < MIN_STACK_SIZE) stack_size = MIN_STACK_SIZE;

    PortCoroutine *co = (PortCoroutine *)calloc(1, sizeof(PortCoroutine));
    if (co == NULL) return NULL;

    co->stack_mem = (char *)malloc(stack_size);
    if (co->stack_mem == NULL) { free(co); return NULL; }

    co->canary_head = CANARY_HEAD;
    co->canary_tail = CANARY_TAIL;
    co->stack_size  = stack_size;
    co->entry       = entry;
    co->arg         = arg;
    co->finished    = 0;
    co->started     = 0;

    return co;
}

void port_coroutine_destroy(PortCoroutine *co) {
    if (co == NULL) return;
    if (!validate_coroutine(co, "destroy")) return;
    if (co->stack_mem) { free(co->stack_mem); co->stack_mem = NULL; }
    co->canary_head = 0;
    co->canary_tail = 0;
    free(co);
}

void port_coroutine_resume(PortCoroutine *co) {
    if (!validate_coroutine(co, "resume")) return;
    if (co->finished) return;

    PortCoroutine *prev = sCurrentCoroutine;

    if (!co->started) {
        /*
         * First resume:  save the caller's state into a local context,
         * set up the coroutine's initial state (sp = top of allocated
         * stack, lr = trampoline address), then switch to it.
         */
        co->started = 1;
        sCurrentCoroutine = co;

        SwitchContext target = {};

        /* Coroutine's initial SP — top of allocated stack (grows down) */
        uintptr_t sp = (uintptr_t)(co->stack_mem + co->stack_size);
        sp &= ~15ULL;  /* 16-byte alignment */

        target.r[12] = sp;                                   /* sp */
        target.r[11] = (uint64_t)coroutine_trampoline;       /* lr (x30) */
        target.r[13] = (uint64_t)co;                         /* x0 -> trampoline arg */

        context_switch(&co->caller_ctx, &target);

        /* We return here when the coroutine yields or finishes. */
        sCurrentCoroutine = prev;
    } else {
        /*
         * Subsequent resumes:  save the caller's state, restore the
         * coroutine's saved state.
         */
        sCurrentCoroutine = co;

        SwitchContext caller;
        context_switch(&co->caller_ctx, &co->ctx);

        sCurrentCoroutine = prev;
    }
}

void port_coroutine_yield(void) {
    PortCoroutine *co = sCurrentCoroutine;

    if (co == NULL) {
        fprintf(stderr, "SSB64: port_coroutine_yield outside coroutine\n");
        return;
    }

    port_watchdog_note_yield();

    /*
     * We're on the coroutine's stack.  The caller's context is still
     * on the caller's stack (which is valid — the caller hasn't
     * returned from port_coroutine_resume yet).  context_switch
     * saves our state into co->ctx and restores the caller's state.
     *
     * The caller is whoever called port_coroutine_resume — their
     * local `caller` SwitchContext is on their stack frame, which
     * is still alive (context_switch doesn't return until we yield,
     * so the caller's frame is intact).
     */
    context_switch(&co->ctx, &co->caller_ctx);
}

int port_coroutine_is_finished(PortCoroutine *co) {
    if (co == NULL) return 1;
    return co->finished;
}

int port_coroutine_in_coroutine(void) {
    return sCurrentCoroutine != NULL;
}

#endif /* __SWITCH__ */
