#include "file.h"

#include "tracelog.h"

#ifdef _WIN32
#include "win32_slim.h"
#include <stdlib.h>

static DWORD _toWin32Access(int mode) {
    if(mode & FILE_READ) return GENERIC_READ;
    if(mode & FILE_WRITE) return GENERIC_WRITE;
    if(mode & FILE_BOTH) return GENERIC_READ | GENERIC_WRITE;
    fatal("unrecognized access mode: %d", mode);
    return 0;
}

static DWORD _toWin32Creation(filemode_t mode) {
    if(mode & FILE_READ) return OPEN_EXISTING;
    if(mode == (FILE_WRITE | FILE_CLEAR)) return CREATE_ALWAYS;
    if(mode & FILE_WRITE) return OPEN_ALWAYS;
    if(mode & FILE_BOTH) return OPEN_ALWAYS;
    fatal("unrecognized creation mode: %d", mode);
    return 0;
}

bool fileExists(const char *fname) {
    return GetFileAttributesA(fname) != INVALID_FILE_ATTRIBUTES;
}

file_t fileOpen(const char *fname, filemode_t mode) {
    return (file_t)CreateFileA(
        fname, 
        _toWin32Access(mode), 
        0, 
        NULL, 
        _toWin32Creation(mode), 
        FILE_ATTRIBUTE_NORMAL, 
        NULL
    );
}

void fileClose(file_t ctx) {
    if (ctx) {
        CloseHandle((HANDLE)ctx);
    }
}

bool fileIsValid(file_t ctx) {
    return (HANDLE)ctx != INVALID_HANDLE_VALUE;
}

bool filePutc(file_t ctx, char c) {
    return fileWrite(ctx, &c, 1) == 1;
}

bool filePuts(file_t ctx, const char *str) {
    usize len = strlen(str);
    return fileWrite(ctx, str, len) == len;
}

bool filePutstr(file_t ctx, str_t str) {
    return fileWrite(ctx, str.buf, str.len) == str.len;
}

bool filePutview(file_t ctx, strview_t view) {
    return fileWrite(ctx, view.buf, view.len) == view.len;
}

usize fileRead(file_t ctx, void *buf, usize len) {
    DWORD bytes_read = 0;
    BOOL result = ReadFile((HANDLE)ctx, buf, (DWORD)len, &bytes_read, NULL);
    return result == TRUE ? (usize)bytes_read : 0;
}

usize fileWrite(file_t ctx, const void *buf, usize len) {
    DWORD bytes_read = 0;
    BOOL result = WriteFile((HANDLE)ctx, buf, (DWORD)len, &bytes_read, NULL);
    return result == TRUE ? (usize)bytes_read : 0;
}

bool fileSeekEnd(file_t ctx) {
    return SetFilePointerEx((HANDLE)ctx, (LARGE_INTEGER){0}, NULL, FILE_END) == TRUE;
}

void fileRewind(file_t ctx) {
    SetFilePointerEx((HANDLE)ctx, (LARGE_INTEGER){0}, NULL, FILE_BEGIN);
}

uint64 fileTell(file_t ctx) {
    LARGE_INTEGER tell;
    BOOL result = SetFilePointerEx((HANDLE)ctx, (LARGE_INTEGER){0}, &tell, FILE_CURRENT);
    return result == TRUE ? (uint64)tell.QuadPart : 0;
}

uint64 fileGetTime(file_t ctx) {
    uint64 fp_time = 0;
    GetFileTime((HANDLE)ctx, NULL, NULL, (FILETIME *)&fp_time);
    return fp_time;
}

#else
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

const char *_toStdioMode(filemode_t mode) {
    switch(mode) {
        case FILE_READ: return "rb";
        case FILE_BOTH: return "r+b";
        case FILE_WRITE: return "wb";
        default: fatal("mode not recognized: %d", mode); return "";
    }
}

bool fileExists(const char *fname) {
    return access(fname, F_OK) == 0;
}

file_t fileOpen(const char *fname, filemode_t mode) {
    return (file_t)(void*) fopen(fname, _toStdioMode(mode));
}

void fileClose(file_t ctx) {
    if(ctx) {
        fclose((FILE*)ctx);
    }
}

bool fileIsValid(file_t ctx) {
    return (FILE *)ctx != NULL;
}

bool filePutc(file_t ctx, char c) {
    return fputc(c, (FILE*)ctx) == c;
}

bool filePuts(file_t ctx, const char *str) {
    return fputs(str, (FILE*)ctx) != EOF;
}

bool filePutstr(file_t ctx, str_t str) {
    return fileWrite(ctx, str.buf, str.len) == str.len;
}

bool filePutview(file_t ctx, strview_t view) {
    return fileWrite(ctx, view.buf, view.len) == view.len;
}

usize fileRead(file_t ctx, void *buf, usize len) {
    return fread(buf, 1, len, (FILE*)ctx);
}

usize fileWrite(file_t ctx, const void *buf, usize len) {
    return fwrite(buf, 1, len, (FILE*)ctx);
}

bool fileSeekEnd(file_t ctx) {
    return fseek((FILE*)ctx, 0, SEEK_END) == 0;
}

void fileRewind(file_t ctx) {
    rewind((FILE*)ctx);
}

uint64 fileTell(file_t ctx) {
    return (uint64)ftell((FILE*)ctx);
}

uint64 fileGetTime(file_t ctx) {

}
#endif

static str_t _readWholeInternalStr(file_t ctx) {
    str_t contents = strInit();
    uint64 fsize = 0;
    usize read = 0;

    if(!fileSeekEnd(ctx)) {
        err("file: couldn't read until end");
        goto failed;
    }

    fsize = fileTell(ctx);
    fileRewind(ctx);

    contents.buf = (char *)malloc(fsize + 1);
    contents.len = fsize;
    if(!contents.buf) {
        err("file: couldn't allocate buffer");
        goto failed;
    }
    
    read = fileRead(ctx, contents.buf, fsize);
    if(read != fsize) {
        err("file: read wrong amount of bytes: %zu instead of %zu", read, fsize);
        goto failed_free;
    }

    contents.buf[contents.len] = '\0';

failed:
    return contents;
failed_free:
    strFree(contents);
    return strInit();   
}

static vec(uint8) _readWholeInternalVec(file_t ctx) {
    vec(uint8) contents = NULL;
    uint64 fsize = 0;
    usize read = 0;

    if(!fileSeekEnd(ctx)) {
        err("file: couldn't read until end");
        goto failed;
    }

    fsize = fileTell(ctx);
    fileRewind(ctx);

    vecReserve(contents, fsize);
    if(!contents) {
        err("file: couldn't allocate buffer");
        goto failed;
    }

    read = fileRead(ctx, contents, fsize);
    if(read != fsize) {
        err("file: read wrong amount of bytes: %zu instead of %zu", read, fsize);
        goto failed_free;
    }

    _veclen(contents) = read;

failed:
    return contents;
failed_free:
    vecFree(contents);
    return contents;    
}

vec(uint8) fileReadWhole(const char *fname) {
    file_t fp = fileOpen(fname, FILE_READ);
    if (!fileIsValid(fp)) return NULL;
    vec(uint8) contents = fileReadWholeFP(fp);
    fileClose(fp);
    return contents;
}

vec(uint8) fileReadWholeFP(file_t ctx) {
    return _readWholeInternalVec(ctx);
}

str_t fileReadWholeText(const char *fname) {
    file_t fp = fileOpen(fname, FILE_READ);
    if(!fileIsValid(fp)) {
        err("couldn't open file %s", fname);
        return strInit();
    }
    str_t contents = fileReadWholeTextFP(fp);
    fileClose(fp);
    return contents;
}

str_t fileReadWholeTextFP(file_t ctx) {
    return _readWholeInternalStr(ctx);
}

bool fileWriteWhole(const char *fname, filebuf_t data) {
    file_t fp = fileOpen(fname, FILE_WRITE);
    if (!fileIsValid(fp)) {
        err("couldn't open file %s", fname);
        return false;
    }
    bool res = fileWriteWholeFP(fp, data);
    fileClose(fp);
    return res;
}

bool fileWriteWholeFP(file_t ctx, filebuf_t data) {
    usize written = fileWrite(ctx, data.buf, data.len);
    return written == data.len;
}

bool fileWriteWholeText(const char *fname, strview_t string) {
    file_t fp = fileOpen(fname, FILE_WRITE);
    if (!fileIsValid(fp)) {
        err("couldn't open file %s", fname);
        return false;
    }
    bool res = fileWriteWholeTextFP(fp, string);
    fileClose(fp);
    return res;
}

bool fileWriteWholeTextFP(file_t ctx, strview_t string) {
    return fileWriteWholeFP(ctx, (filebuf_t){ (uint8 *)string.buf, string.len });
}

uint64 fileGetTimePath(const char *path) {
    file_t fp = fileOpen(path, FILE_READ);
    if (!fileIsValid(fp)) {
        return 0;
    }
    uint64 fp_time = fileGetTime(fp);
    fileClose(fp);
    return fp_time;
}