#include "port_log.h"

#include <stdio.h>
#include <stdarg.h>

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
	/* Pre-init fallback: emit to stderr so very early diagnostics aren't
	 * silently lost. Without this, anything logged before port_log_init
	 * (CRT invalid-parameter / std::terminate handlers, signal handlers,
	 * or early static-init exceptions) disappears completely. */
	if (sLogFile == NULL) {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fflush(stderr);
		return;
	}

	/* Mirror every line to stderr alongside the file so users running
	 * from a terminal see boot progress + mod-load confirmation without
	 * having to find ssb64.log (~/Library/Application Support/BattleShip
	 * on macOS, %APPDATA%\BattleShip on Windows, $XDG_DATA_HOME/BattleShip
	 * on Linux). Prior behavior surfaced LUS spdlog lines on the console
	 * but dropped every SSB64: port_log into the file only, so questions
	 * like "did my mod load?" required digging through the OS app-data
	 * dir even when the binary was launched from a shell with stderr
	 * visible. */
	va_list ap;
	va_list ap2;
	va_start(ap, fmt);
	va_copy(ap2, ap);
	vfprintf(sLogFile, fmt, ap);
	fflush(sLogFile);
	vfprintf(stderr, fmt, ap2);
	fflush(stderr);
	va_end(ap2);
	va_end(ap);
}
