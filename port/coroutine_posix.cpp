/**
 * coroutine_posix.cpp — POSIX ucontext-based coroutine implementation.
 *
 * Each coroutine uses a ucontext_t with a separate stack.
 * swapcontext() provides the resume/yield mechanism.
 */

#if !defined(_WIN32) && !defined(__SWITCH__)

/*
 * macOS marks the ucontext / swapcontext routines as deprecated and hides
 * their declarations unless _XOPEN_SOURCE is defined before <ucontext.h>.
 * Define it here (not via the target's compile definitions) so the rest of
 * the port layer still sees strictly-POSIX symbols.
 */
#if defined(__APPLE__) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 600
#endif

#include "coroutine.h"
#include "port_watchdog.h"

#include <ucontext.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_STACK_SIZE 32768

struct PortCoroutine {
	ucontext_t ctx;
	ucontext_t caller_ctx;
	void (*entry)(void *);
	void *arg;
	int finished;
	char *stack_mem;
	size_t stack_size;
};

static PortCoroutine *sCurrentCoroutine = NULL;

/* ========================================================================= */
/*  Internal: ucontext entry wrapper                                         */
/* ========================================================================= */

/*
 * makecontext requires a function with int parameters. We pass the
 * PortCoroutine pointer split into two ints for 64-bit compatibility.
 */
static void ucontext_entry(unsigned int lo, unsigned int hi)
{
	uintptr_t ptr = ((uintptr_t)hi << 32) | (uintptr_t)lo;
	PortCoroutine *co = (PortCoroutine *)ptr;

	co->entry(co->arg);

	co->finished = 1;
	sCurrentCoroutine = NULL;

	/* Return to caller via the linked context (set by swapcontext). */
}

/* ========================================================================= */
/*  Public API                                                               */
/* ========================================================================= */

void port_coroutine_init_main(void)
{
	/* No-op on POSIX — ucontext doesn't require main thread conversion. */
}

PortCoroutine *port_coroutine_create(void (*entry)(void *), void *arg,
                                     size_t stack_size)
{
	PortCoroutine *co;
	uintptr_t ptr;

	if (stack_size < MIN_STACK_SIZE) {
		stack_size = MIN_STACK_SIZE;
	}

	co = (PortCoroutine *)calloc(1, sizeof(PortCoroutine));
	if (co == NULL) {
		return NULL;
	}

	co->stack_mem = (char *)malloc(stack_size);
	if (co->stack_mem == NULL) {
		free(co);
		return NULL;
	}

	co->stack_size = stack_size;
	co->entry = entry;
	co->arg = arg;
	co->finished = 0;

	if (getcontext(&co->ctx) == -1) {
		free(co->stack_mem);
		free(co);
		return NULL;
	}

	co->ctx.uc_stack.ss_sp = co->stack_mem;
	co->ctx.uc_stack.ss_size = stack_size;
	co->ctx.uc_link = &co->caller_ctx; /* return to caller when entry returns */

	ptr = (uintptr_t)co;
	makecontext(&co->ctx, (void (*)(void))ucontext_entry, 2,
	            (unsigned int)(ptr & 0xFFFFFFFF),
	            (unsigned int)(ptr >> 32));

	return co;
}

void port_coroutine_destroy(PortCoroutine *co)
{
	if (co == NULL) {
		return;
	}
	if (co->stack_mem != NULL) {
		free(co->stack_mem);
		co->stack_mem = NULL;
	}
	free(co);
}

void port_coroutine_resume(PortCoroutine *co)
{
	if (co == NULL || co->finished) {
		return;
	}

	/* Save the current coroutine so nested resumes restore correctly.
	 * Example: main resumes Thread5, Thread5 resumes a GObj coroutine.
	 * When the GObj coroutine yields, sCurrentCoroutine must be restored
	 * to Thread5 (not NULL) so Thread5 can still yield later. */
	PortCoroutine *prev = sCurrentCoroutine;

	sCurrentCoroutine = co;
	swapcontext(&co->caller_ctx, &co->ctx);

	/* Restore the previous coroutine context for the caller. */
	sCurrentCoroutine = prev;
}

void port_coroutine_yield(void)
{
	PortCoroutine *co = sCurrentCoroutine;
	if (co == NULL) {
		fprintf(stderr, "SSB64: port_coroutine_yield called outside coroutine\n");
		return;
	}

	port_watchdog_note_yield();

	sCurrentCoroutine = NULL;
	swapcontext(&co->ctx, &co->caller_ctx);
	/* Returns here when resumed. */
}

int port_coroutine_is_finished(PortCoroutine *co)
{
	if (co == NULL) {
		return 1;
	}
	return co->finished;
}

int port_coroutine_in_coroutine(void)
{
	return sCurrentCoroutine != NULL;
}

#endif /* !_WIN32 */
