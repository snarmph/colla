#include "tracelog.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

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
    #define BLACK   "\x1B[30m"
    #define RED     "\x1B[31m"
    #define GREEN   "\x1B[32m"
    #define YELLOW  "\x1B[33m"
    #define BLUE    "\x1B[34m"
    #define MAGENTA "\x1B[35m"
    #define CYAN    "\x1B[36m"
    #define WHITE   "\x1B[37m"
    #define RESET   "\x1B[0m"
    #define BOLD    "\x1B[1m"
#endif

#define MAX_TRACELOG_MSG_LENGTH 1024

void traceLog(LogLevel level, const char *fmt, ...) {
    char buffer[MAX_TRACELOG_MSG_LENGTH];

    switch (level) {
        case LogTrace:   strcpy(buffer, BOLD WHITE  "[TRACE]: "   RESET); break; 
        case LogDebug:   strcpy(buffer, BOLD BLUE   "[DEBUG]: "   RESET); break; 
        case LogInfo:    strcpy(buffer, BOLD GREEN  "[INFO]: "    RESET); break; 
        case LogWarning: strcpy(buffer, BOLD YELLOW "[WARNING]: " RESET); break; 
        case LogError:   strcpy(buffer, BOLD RED    "[ERROR]: "   RESET); break; 
        case LogFatal:   strcpy(buffer, BOLD RED    "[FATAL]: "   RESET); break;        
        default: break;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

#ifdef TLOG_VS
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
#else
    puts(buffer);
#endif

#ifndef TLOG_DONT_EXIT_ON_FATAL
    if (level == LogFatal) exit(1);
#endif
}