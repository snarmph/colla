#include "fs.h"

#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "tracelog.h"

#ifdef _WIN32
#define VC_EXTRALEAN
#include <windows.h>

#include <sys/stat.h>

static int _modeToType(unsigned int mode) {
    switch(mode & _S_IFMT) {
        case _S_IFDIR:  return FS_MODE_DIR;
        case _S_IFCHR:  return FS_MODE_CHARACTER_DEVICE;
        case _S_IFREG:  return FS_MODE_FILE;
        case _S_IFIFO:  return FS_MODE_FIFO;
        default:        return FS_MODE_UKNOWN;
    }
}

fs_stat_t fsStat(file_t fp) {
    TCHAR path[MAX_PATH];
    DWORD pathlen = GetFinalPathNameByHandle(
        (HANDLE)fp.handle,
        path,
        MAX_PATH,
        0);

    struct stat statbuf;
    int res = stat(path, &statbuf);
    if(res == 0) {
        return (fs_stat_t) {
            .type = _modeToType(statbuf.st_mode),
            .size = statbuf.st_size,
            .last_access = statbuf.st_atime,
            .last_modif = statbuf.st_mtime
        };
    }
    else {
        return (fs_stat_t) { 0 };
    }
}

fs_time_t fsAsTime(int64_t timer) {
    struct tm t;
    errno_t error = localtime_s(&t, &timer);
    
    if(error == 0) {
        return (fs_time_t) {
            .year = t.tm_year + 1900,
            .month = t.tm_mon + 1,
            .day = t.tm_mday,
            .hour = t.tm_hour,
            .minutes = t.tm_min,
            .seconds = t.tm_sec,
            .daylight_saving = t.tm_isdst > 0
        };
    }
    else {
        char buf[128];
        strerror_s(buf, sizeof(buf), error);
        err("%s", buf);
        return (fs_time_t) { 0 };
    }
}

#else
#include <sys/stat.h>

static int _modeToType(unsigned int mode) {
    switch(mode & __S_IFMT) {
        case __S_IFDIR:  return FS_MODE_DIR;
        case __S_IFCHR:  return FS_MODE_CHARACTER_DEVICE;
        case __S_IFREG:  return FS_MODE_FILE;
        case __S_IFIFO:  return FS_MODE_FIFO;
        default:         return FS_MODE_UKNOWN;
    }
}

fs_stat_t fsStat(file_t fp) {
    int fd = fileno((FILE*)fp.handle);
    struct stat statbuf;
    int res = fstat(fd, &statbuf);
    if(res == 0) {
        return (fs_stat_t) {
            .type = _modeToType(statbuf.st_mode),
            .size = statbuf.st_size,
            .last_access = statbuf.st_atime,
            .last_modif = statbuf.st_mtime
        };
    }
    else {
        return (fs_stat_t) { 0 };
    }
}

fs_time_t fsAsTime(int64_t timer) {
    struct tm *t = localtime(&timer);
    
    if(t) {
        return (fs_time_t) {
            .year = t->tm_year + 1900,
            .month = t->tm_mon + 1,
            .day = t->tm_mday,
            .hour = t->tm_hour,
            .minutes = t->tm_min,
            .seconds = t->tm_sec,
            .daylight_saving = t->tm_isdst > 0
        };
    }
    else {
        err("%s", strerror(errno));
        return (fs_time_t) { 0 };
    }
}
#endif