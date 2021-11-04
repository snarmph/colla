#include "file.h"

#include "tracelog.h"

#ifdef _WIN32
#define VC_EXTRALEAN
#include <windows.h>

static DWORD _toWin32Access(int mode) {
    if(mode & FILE_READ) return GENERIC_READ;
    if(mode & FILE_WRITE) return GENERIC_WRITE;
    if(mode & FILE_BOTH) return GENERIC_READ | GENERIC_WRITE;
    fatal("unrecognized access mode: %d", mode);
    return 0;
}

static DWORD _toWin32Creation(int mode) {
    if(mode & FILE_READ) return OPEN_EXISTING;
    if(mode == (FILE_WRITE | FILE_CLEAR)) return CREATE_ALWAYS;
    if(mode & FILE_WRITE) return OPEN_ALWAYS;
    if(mode & FILE_BOTH) return OPEN_ALWAYS;
    fatal("unrecognized creation mode: %d", mode);
    return 0;
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
#include <stdlib.h>
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

static fread_buf_t _readWholeInternal(file_t *ctx, bool null_terminated) {
    fread_buf_t contents = {0};
    uint64_t fsize = 0;

    if(!fileSeekEnd(ctx)) {
        err("file: couldn't read until end");
        goto failed;
    }

    fsize = fileTell(ctx);
    fileRewind(ctx);

    contents.len = fsize;
    contents.buf = malloc(fsize + null_terminated);
    if(!contents.buf) {
        err("file: couldn't allocate buffer");
        goto failed;
    }

    size_t read = fileRead(ctx, contents.buf, fsize);
    if(read != fsize) {
        err("file: read wrong amount of bytes: %zu instead of %zu", read, fsize);
        goto failed_free;
    }

    if(null_terminated) {
        contents.buf[contents.len] = '\0';
    }

failed:
    return contents;
failed_free:
    free(contents.buf);
    return contents;    
}

fread_buf_t fileReadWhole(const char *fname) {
    file_t fp = fileOpen(fname, FILE_READ);
    fread_buf_t contents = fileReadWholeFP(&fp);
    fileClose(&fp);
    return contents;
}

fread_buf_t fileReadWholeFP(file_t *ctx) {
    return _readWholeInternal(ctx, false);
}

str_t fileReadWholeText(const char *fname) {
    file_t fp = fileOpen(fname, FILE_READ);
    if(!fileIsValid(&fp)) {
        err("couldn't open file %s -> %d", fname);
        return strInit();
    }
    str_t contents = fileReadWholeFPText(&fp);
    fileClose(&fp);
    return contents;
}

str_t fileReadWholeFPText(file_t *ctx) {
    fread_buf_t contents = _readWholeInternal(ctx, true);
    return (str_t) {
        .buf = contents.buf,
        .len = contents.len
    };
}