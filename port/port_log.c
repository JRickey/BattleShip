#include "port_log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static FILE *sLogFile = NULL;

#ifdef __SWITCH__
/* On Switch, SD-card fflush is slow (~5-20ms). Buffer in RAM and only
 * flush when the buffer is nearly full or on close. */
static char sLogBuffer[65536];
static size_t sLogBufUsed = 0;
#endif

void port_log_init(const char *path)
{
	if (sLogFile != NULL) return;
	sLogFile = fopen(path, "w");
}

void port_log_close(void)
{
	if (sLogFile != NULL) {
#ifdef __SWITCH__
		if (sLogBufUsed > 0) {
			fwrite(sLogBuffer, 1, sLogBufUsed, sLogFile);
			sLogBufUsed = 0;
		}
#endif
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
#ifdef __SWITCH__
	int n = vsnprintf(sLogBuffer + sLogBufUsed,
	                  sizeof(sLogBuffer) - sLogBufUsed, fmt, ap);
	if (n > 0) {
		sLogBufUsed += (size_t)n;
		if (sLogBufUsed >= sizeof(sLogBuffer) * 3 / 4) {
			fwrite(sLogBuffer, 1, sLogBufUsed, sLogFile);
			sLogBufUsed = 0;
		}
	}
#else
	vfprintf(sLogFile, fmt, ap);
	fflush(sLogFile);
#endif
	va_end(ap);
}
