#include "file.h"

#include "tracelog.h"

#ifdef _WIN32
#define VC_EXTRALEAN
#include <windows.h>

static DWORD _toWin32Access(int mode) {
    switch(mode) {
        case FILE_READ:  return GENERIC_READ;
        case FILE_WRITE: return GENERIC_WRITE;
        case FILE_BOTH:  return GENERIC_READ | GENERIC_WRITE;
        default: fatal("unrecognized mode: %d", mode); return 0;
    }
}

static DWORD _toWin32Creation(int mode) {
    switch(mode) {
        case FILE_READ:  return OPEN_EXISTING;
        case FILE_WRITE: return OPEN_ALWAYS;
        case FILE_BOTH:  return OPEN_ALWAYS;
        default: fatal("unrecognized mode: %d", mode); return 0;
    }
}

file_t fileOpen(const char *fname, int mode) {
    return (file_t) {
        .handle = CreateFile(fname, 
                             _toWin32Access(mode), 
                             0, 
                             NULL, 
                             _toWin32Creation(mode), 
                             FILE_ATTRIBUTE_NORMAL, 
                             NULL)
    };
}

void fileClose(file_t *ctx) {
    if(ctx->handle) {
        CloseHandle((HANDLE)ctx->handle);
        ctx->handle = NULL;
    }
}

bool fileIsValid(file_t *ctx) {
    return ctx->handle != INVALID_HANDLE_VALUE;
}

bool filePutc(file_t *ctx, char c) {
    return fileWrite(ctx, &c, 1) == 1;
}

bool filePuts(file_t *ctx, const char *str) {
    size_t len = strlen(str);
    return fileWrite(ctx, str, len) == len;
}

bool filePutstr(file_t *ctx, str_t str) {
    return fileWrite(ctx, str.buf, str.len) == str.len;
}

bool filePutview(file_t *ctx, strview_t view) {
    return fileWrite(ctx, view.buf, view.len) == view.len;
}

size_t fileRead(file_t *ctx, void *buf, size_t len) {
    DWORD bytes_read = 0;
    BOOL result = ReadFile((HANDLE)ctx->handle, buf, (DWORD)len, &bytes_read, NULL);
    return result == TRUE ? (size_t)bytes_read : 0;
}

size_t fileWrite(file_t *ctx, const void *buf, size_t len) {
    DWORD bytes_read = 0;
    BOOL result = WriteFile((HANDLE)ctx->handle, buf, (DWORD)len, &bytes_read, NULL);
    return result == TRUE ? (size_t)bytes_read : 0;
}

bool fileSeekEnd(file_t *ctx) {
    return SetFilePointerEx((HANDLE)ctx->handle, (LARGE_INTEGER){0}, NULL, FILE_END) == TRUE;
}

void fileRewind(file_t *ctx) {
    SetFilePointerEx((HANDLE)ctx->handle, (LARGE_INTEGER){0}, NULL, FILE_BEGIN);
}

uint64_t fileTell(file_t *ctx) {
    LARGE_INTEGER tell;
    BOOL result = SetFilePointerEx((HANDLE)ctx->handle, (LARGE_INTEGER){0}, &tell, FILE_CURRENT);
    return result == TRUE ? (uint64_t)tell.QuadPart : 0;
}

#else
#include <stdio.h>
#include <errno.h>
#include <string.h>

const char *_toStdioMode(int mode) {
    switch(mode) {
        case FILE_READ: return "rb";
        case FILE_BOTH: return "r+b";
        case FILE_WRITE: return "wb";
        default: fatal("mode not recognized: %d", mode); return "";
    }
}

file_t fileOpen(const char *fname, int mode) {
    return (file_t) {
        .handle = (void*) fopen(fname, _toStdioMode(mode)),
    };
}

void fileClose(file_t *ctx) {
    if(ctx->handle) {
        fclose((FILE*)ctx->handle);
        ctx->handle = NULL;
    }
}

bool fileIsValid(file_t *ctx) {
    info("handle: %p", ctx->handle);
    return ctx->handle != NULL;
}

bool filePutc(file_t *ctx, char c) {
    return fputc(c, (FILE*)ctx->handle) == c;
}

bool filePuts(file_t *ctx, const char *str) {
    return fputs(str, (FILE*)ctx->handle) != EOF;
}

bool filePutstr(file_t *ctx, str_t str) {
    return fileWrite(ctx, str.buf, str.len) == str.len;
}

bool filePutview(file_t *ctx, strview_t view) {
    return fileWrite(ctx, view.buf, view.len) == view.len;
}

size_t fileRead(file_t *ctx, void *buf, size_t len) {
    return fread(buf, 1, len, (FILE*)ctx->handle);
}

size_t fileWrite(file_t *ctx, const void *buf, size_t len) {
    return fwrite(buf, 1, len, (FILE*)ctx->handle);
}

bool fileSeekEnd(file_t *ctx) {
    return fseek((FILE*)ctx->handle, 0, SEEK_END) == 0;
}

void fileRewind(file_t *ctx) {
    rewind((FILE*)ctx->handle);
}

uint64_t fileTell(file_t *ctx) {
    return (uint64_t)ftell((FILE*)ctx->handle);
}
#endif