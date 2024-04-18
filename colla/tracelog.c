#include "tracelog.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "format.h"

#if COLLA_WIN
    #if COLLA_MSVC
        #pragma warning(disable:4996) // _CRT_SECURE_NO_WARNINGS.
    #endif

    #include <handleapi.h>
    
    // avoid including windows.h
    
    #ifndef STD_OUTPUT_HANDLE
        #define STD_OUTPUT_HANDLE   ((DWORD)-11)
    #endif
    #ifndef CP_UTF8
        #define CP_UTF8             65001
    #endif

    typedef unsigned short WORD;
    typedef unsigned long DWORD;
    typedef unsigned int UINT;
    typedef int BOOL;
    WINBASEAPI HANDLE WINAPI GetStdHandle(DWORD nStdHandle);
    WINBASEAPI BOOL WINAPI SetConsoleTextAttribute(HANDLE hConsoleOutput, WORD wAttributes);
    WINBASEAPI BOOL WINAPI SetConsoleOutputCP(UINT wCodePageID);

    #ifndef TLOG_VS
        #define TLOG_WIN32_NO_VS
        #ifndef TLOG_NO_COLOURS
            #define TLOG_NO_COLOURS
        #endif
    #endif
#endif

#ifdef TLOG_VS
    #if COLLA_WIN
        #error "can't use TLOG_VS if not on windows"
    #endif
#endif

#ifdef TLOG_NO_COLOURS
    #define COLOUR_BLACK   ""
    #define COLOUR_RED     ""
    #define COLOUR_GREEN   ""
    #define COLOUR_YELLOW  ""
    #define COLOUR_BLUE    ""
    #define COLOUR_MAGENTA ""
    #define COLOUR_CYAN    ""
    #define COLOUR_WHITE   ""
    #define COLOUR_RESET   ""
    #define COLOUR_BOLD    ""
#else
    #define COLOUR_BLACK   "\033[30m"
    #define COLOUR_RED     "\033[31m"
    #define COLOUR_GREEN   "\033[32m"
    #define COLOUR_YELLOW  "\033[33m"
    #define COLOUR_BLUE    "\033[22;34m"
    #define COLOUR_MAGENTA "\033[35m"
    #define COLOUR_CYAN    "\033[36m"
    #define COLOUR_WHITE   "\033[37m"
    #define COLOUR_RESET   "\033[0m"
    #define COLOUR_BOLD    "\033[1m"
#endif

#define MAX_TRACELOG_MSG_LENGTH 1024

bool use_newline = true;

#ifdef TLOG_WIN32_NO_VS
static void setLevelColour(int level) {
    WORD attribute = 15;
    switch (level) {
        case LogDebug:   attribute = 1; break; 
        case LogInfo:    attribute = 2; break;
        case LogWarning: attribute = 6; break;
        case LogError:   attribute = 4; break;
        case LogFatal:   attribute = 4; break;
    }

    HANDLE hc = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hc, attribute);
}
#endif

void traceLog(int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);    traceLogVaList(level, fmt, args);
    va_end(args);
}

#ifdef TLOG_MUTEX
#include "cthreads.h"
static cmutex_t g_mtx = 0;
#endif

void traceLogVaList(int level, const char *fmt, va_list args) {
    #ifdef TLOG_MUTEX
    if (!g_mtx) g_mtx = mtxInit();
    mtxLock(g_mtx);
    #endif

    char buffer[MAX_TRACELOG_MSG_LENGTH];
    memset(buffer, 0, sizeof(buffer));

    const char *beg;
    switch (level) {
        case LogTrace:   beg = COLOUR_BOLD COLOUR_WHITE  "[TRACE]: "   COLOUR_RESET; break; 
        case LogDebug:   beg = COLOUR_BOLD COLOUR_BLUE   "[DEBUG]: "   COLOUR_RESET; break; 
        case LogInfo:    beg = COLOUR_BOLD COLOUR_GREEN  "[INFO]: "    COLOUR_RESET; break; 
        case LogWarning: beg = COLOUR_BOLD COLOUR_YELLOW "[WARNING]: " COLOUR_RESET; break; 
        case LogError:   beg = COLOUR_BOLD COLOUR_RED    "[ERROR]: "   COLOUR_RESET; break; 
        case LogFatal:   beg = COLOUR_BOLD COLOUR_RED    "[FATAL]: "   COLOUR_RESET; break;        
        default:         beg = "";                              break;
    }

    size_t offset = 0;

#ifndef TLOG_WIN32_NO_VS
    offset = strlen(beg);
    strncpy(buffer, beg, sizeof(buffer));
#endif

    fmtBufferv(buffer + offset, sizeof(buffer) - offset, fmt, args);

#if defined(TLOG_VS)
    OutputDebugStringA(buffer);
    if(use_newline) OutputDebugStringA("\n");
#elif defined(TLOG_WIN32_NO_VS)
    SetConsoleOutputCP(CP_UTF8);
    setLevelColour(level);
    printf("%s", beg);
    // set back to white
    setLevelColour(LogTrace);
    printf("%s", buffer);
    if(use_newline) puts("");
#else
    printf("%s", buffer);
    if(use_newline) puts("");
#endif

#ifndef TLOG_DONT_EXIT_ON_FATAL
    if (level == LogFatal) {
        abort();
        exit(1);
    }
#endif

    #ifdef TLOG_MUTEX
    mtxUnlock(g_mtx);
    #endif
}

void traceUseNewline(bool newline) {
    use_newline = newline;
}

void traceSetColour(colour_e colour) {
#ifdef TLOG_WIN32_NO_VS
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colour);
#else
    switch (colour) {
        case COL_RESET:   printf(RESET);   break;
        case COL_BLACK:   printf(BLACK);   break;
        case COL_BLUE:    printf(BLUE);    break;
        case COL_GREEN:   printf(GREEN);   break;
        case COL_CYAN:    printf(CYAN);    break;
        case COL_RED:     printf(RED);     break;
        case COL_MAGENTA: printf(MAGENTA); break;
        case COL_YELLOW:  printf(YELLOW);  break;
    }
#endif
}