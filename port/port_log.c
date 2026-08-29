#include "port_log.h"

#include <stdio.h>
#include <stdarg.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

static FILE *sLogFile = NULL;

void port_log_init(const char *path)
{
	if (sLogFile != NULL) return;
	sLogFile = fopen(path, "w");
}

void port_log_close(void)
{
	if (sLogFile != NULL) {
		fclose(sLogFile);
		sLogFile = NULL;
	}
}

int port_log_get_fd(void)
{
	if (sLogFile == NULL) return -1;
	return fileno(sLogFile);
}

void port_log(const char *fmt, ...)
{
	if (sLogFile == NULL) return;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(sLogFile, fmt, ap);
	va_end(ap);
	/* fflush on every call costs seconds per frame on a slow drive when
	 * figatree watchdogs fire 28x per frame during a stuck APPEAR. Rely on
	 * stdio's buffer + OS-on-exit flush for normal logging; crash dumps
	 * have their own flush path. */
}

void port_exit_process(int code)
{
	if (sLogFile != NULL) fflush(sLogFile);
	fflush(NULL);
#ifdef _WIN32
	/* Not _exit()/ExitProcess(): those kill the other threads and then run
	 * every DLL's DLL_PROCESS_DETACH on this thread, which fail-fasts when a
	 * killed thread held one of their locks (seen as STATUS_FAIL_FAST_EXCEPTION
	 * at VS scene teardown). TerminateProcess skips all of that. */
	TerminateProcess(GetCurrentProcess(), (UINT)code);
#else
	_exit(code);
#endif
}
