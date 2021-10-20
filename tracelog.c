#include "tracelog.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef _WIN32 
    #pragma warning(disable:4996) // _CRT_SECURE_NO_WARNINGS.
#endif

#ifdef TLOG_VS
    #ifdef _WIN32
        #ifndef TLOG_NO_COLOURS
            #define TLOG_NO_COLOURS
        #endif

        #define VC_EXTRALEAN
        #include <windows.h>
    #else
        #error "can't use TLOG_VS if not on windows"
    #endif
#endif

#ifdef TLOG_NO_COLOURS
    #define BLACK   ""
    #define RED     ""
    #define GREEN   ""
    #define YELLOW  ""
    #define BLUE    ""
    #define MAGENTA ""
    #define CYAN    ""
    #define WHITE   ""
    #define RESET   ""
    #define BOLD    ""
#else
    #define BLACK   "\033[30m"
    #define RED     "\033[31m"
    #define GREEN   "\033[32m"
    #define YELLOW  "\033[33m"
    #define BLUE    "\033[22;34m"
    #define MAGENTA "\033[35m"
    #define CYAN    "\033[36m"
    #define WHITE   "\033[37m"
    #define RESET   "\033[0m"
    #define BOLD    "\033[1m"
#endif

#define MAX_TRACELOG_MSG_LENGTH 1024

bool use_newline = true;

void traceLog(LogLevel level, const char *fmt, ...) {
    char buffer[MAX_TRACELOG_MSG_LENGTH];
    memset(buffer, 0, sizeof(buffer));

    const char *beg;
    switch (level) {
        case LogTrace:   beg = BOLD WHITE  "[TRACE]: "   RESET; break; 
        case LogDebug:   beg = BOLD BLUE   "[DEBUG]: "   RESET; break; 
        case LogInfo:    beg = BOLD GREEN  "[INFO]: "    RESET; break; 
        case LogWarning: beg = BOLD YELLOW "[WARNING]: " RESET; break; 
        case LogError:   beg = BOLD RED    "[ERROR]: "   RESET; break; 
        case LogFatal:   beg = BOLD RED    "[FATAL]: "   RESET; break;        
        default: break;
    }

    size_t offset = strlen(beg);
    strncpy(buffer, beg, sizeof(buffer));

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, args);
    va_end(args);

#ifdef TLOG_VS
    OutputDebugStringA(buffer);
    if(use_newline) OutputDebugStringA("\n");
#else
    printf("%s", buffer);
    if(use_newline) puts("");
#endif

#ifndef TLOG_DONT_EXIT_ON_FATAL
    if (level == LogFatal) exit(1);
#endif
}

void traceUseNewline(bool newline) {
    use_newline = newline;
}