#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Define any of this to turn on the option
 * -> TLOG_NO_COLOURS:         print without using colours
 * -> TLOG_VS:                 print to visual studio console, also turns on TLOG_NO_COLOURS
 * -> TLOG_DONT_EXIT_ON_FATAL: don't call 'exit(1)' when using LogFatal
*/

#include <stdbool.h>
#include <stdarg.h>

enum {
    LogAll, LogTrace, LogDebug, LogInfo, LogWarning, LogError, LogFatal
};

void traceLog(int level, const char *fmt, ...);
void traceLogVaList(int level, const char *fmt, va_list args);
void traceUseNewline(bool use_newline);

#define tall(...)  traceLog(LogAll, __VA_ARGS__)
#define trace(...) traceLog(LogTrace, __VA_ARGS__)
#define debug(...) traceLog(LogDebug, __VA_ARGS__)
#define info(...)  traceLog(LogInfo, __VA_ARGS__)
#define warn(...)  traceLog(LogWarning, __VA_ARGS__)
#define err(...)   traceLog(LogError, __VA_ARGS__)
#define fatal(...) traceLog(LogFatal, __VA_ARGS__)

#ifdef __cplusplus
} // extern "C"
#endif