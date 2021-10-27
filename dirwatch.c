#include "dirwatch.h"

#include <stdint.h>
#include <assert.h>
#include "tracelog.h"

#ifdef _WIN32
#define VC_EXTRALEAN
#include <windows.h>
#include "str.h"

typedef struct {
    const char *path;
    dirwatch_cb_t on_event;
    BOOL should_continue;
    HANDLE stop_event;
} __dirwatch_internal_t;

static int watchDirThread(void *cdata) {
    __dirwatch_internal_t *desc = (__dirwatch_internal_t*)cdata;

    // stop_event is called from another thread when watchDirThread should exit 
    desc->stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);

    HANDLE file = CreateFile(
        desc->path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    assert(file != INVALID_HANDLE_VALUE);
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

    uint8_t change_buf[1024];
    BOOL success = ReadDirectoryChangesW(
        file, change_buf, sizeof(change_buf), TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME  |
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL, &overlapped, NULL
    );
    assert(success);

    HANDLE events[2] = {
        overlapped.hEvent,
        desc->stop_event
    };

    WCHAR old_name[MAX_PATH];
    dirwatch_file_t data = {0};

    // if the variable is 32bit aligned, a read/write is already atomice
    // https://docs.microsoft.com/en-us/windows/win32/sync/interlocked-variable-access
    while(desc->should_continue) {
        DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if(result == WAIT_OBJECT_0) {
            DWORD bytes_transferred;
            GetOverlappedResult(file, &overlapped, &bytes_transferred, FALSE);

            FILE_NOTIFY_INFORMATION *event = (FILE_NOTIFY_INFORMATION*)change_buf;

            for(;;) {
                DWORD name_len = event->FileNameLength / sizeof(wchar_t);

                switch(event->Action) {
                case FILE_ACTION_ADDED:
                    data.name = strFromWCHAR(event->FileName, name_len).buf;
                    data.oldname = NULL;
                    desc->on_event(desc->path, DIRWATCH_FILE_ADDED, data);
                    break;
                case FILE_ACTION_REMOVED:
                    data.name = strFromWCHAR(event->FileName, name_len).buf;
                    data.oldname = NULL;
                    desc->on_event(desc->path, DIRWATCH_FILE_REMOVED, data);
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    memcpy(old_name, event->FileName, event->FileNameLength);
                    old_name[event->FileNameLength] = '\0';
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    data.name = strFromWCHAR(event->FileName, name_len).buf;
                    data.oldname = strFromWCHAR(old_name, 0).buf;
                    desc->on_event(desc->path, DIRWATCH_FILE_RENAMED, data);
                    break;
                }

                if(data.name) free(data.name);
                if(data.oldname) free(data.oldname);

                data = (dirwatch_file_t){0};

                if(event->NextEntryOffset) {
                    *((uint8_t**)&event) += event->NextEntryOffset;
                }
                else {
                    break;
                }
            }

            success = ReadDirectoryChangesW(
                file, change_buf, sizeof(change_buf), TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME  |
                FILE_NOTIFY_CHANGE_LAST_WRITE,
                NULL, &overlapped, NULL 
            );
            assert(success);
        }
        // stop_event
        else if(result == WAIT_OBJECT_0 + 1) {
            warn("stopping");
            break;
        }
    }

    return 0;
}

dirwatch_t watchDir(dirwatch_desc_t desc) {
    dirwatch_t dir = {0};

    __dirwatch_internal_t *opts = HeapAlloc(
        GetProcessHeap(), 
        0, 
        sizeof(__dirwatch_internal_t)
    );

    opts->path = desc.path;
    opts->on_event = desc.on_event;
    opts->should_continue = true;

    dir.desc = (dirwatch_desc_t *)opts;

    dir.handle = thrCreate(watchDirThread, (void *)dir.desc);

    if(thrValid(dir.handle)) {
        info("watching %s", desc.path);
    }

    return dir;
}

void waitForWatchDir(dirwatch_t *ctx) {
    if(!thrValid(ctx->handle)) {
        err("not valid"); 
        return;
    }

    if(!thrJoin(ctx->handle, NULL)) {
        err("dirwatch: couldn't wait for thread");
    }
    info("waited");

    HeapFree(GetProcessHeap(), 0, ctx->desc);
}

void stopWatchDir(dirwatch_t *ctx, bool immediately) {
    (void)immediately;
    __dirwatch_internal_t *opts = (__dirwatch_internal_t *)ctx->desc;
    opts->should_continue = false;
    if(immediately) {
        if(!SetEvent(opts->stop_event)) {
            err("couldn't signal event stop_event: %d", GetLastError());
        }
    }
    if(!thrJoin(ctx->handle, NULL)) {
        err("dirwatch: couldn't wait for thread");
    }

    HeapFree(GetProcessHeap(), 0, ctx->desc);
}

#else
#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h> // malloc
#include <unistd.h> // read
#include <string.h>
#include <errno.h>
#include <linux/limits.h> // MAX_PATH
#include <stdatomic.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN    (1024 * (EVENT_SIZE + 16))

#define ERROR(str) { err(str ": %s", strerror(errno)); goto error; }

typedef struct {
    const char *path;
    dirwatch_cb_t on_event;
    atomic_bool should_continue;
    int fd;
    int wd;
} __dirwatch_internal_t;

static int watchDirThread(void *cdata) {
    __dirwatch_internal_t *desc = (__dirwatch_internal_t *)cdata;
    info("watching %s", desc->path);

    int length/*, fd, wd*/;
    char buffer[BUF_LEN];

    desc->fd = inotify_init();

    if(desc->fd < 0) {
        ERROR("inotify_init failed");
    }

    desc->wd = inotify_add_watch(desc->fd, desc->path, IN_MOVE | IN_CREATE | IN_DELETE);

    char old_path[PATH_MAX] = {0};
    dirwatch_file_t data = {0};

    while(desc->should_continue) {
        length = (int)read(desc->fd, buffer, BUF_LEN);

        if(length < 0) {
            ERROR("couldn't read");
        }

        for(int i = 0; i < length;) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if(event->len) {
                uint32_t e = event->mask;
                // bool is_dir = e & IN_ISDIR;

                if(e & IN_CREATE) {
                    data.name = event->name;
                    desc->on_event(desc->path, DIRWATCH_FILE_ADDED, data);
                }
                else if(e & IN_DELETE) {
                    data.name = event->name;
                    desc->on_event(desc->path, DIRWATCH_FILE_REMOVED, data);
                }
                else if(e & IN_MOVED_FROM) {
                    memcpy(old_path, event->name, event->len);
                    old_path[event->len] = '\0';
                }
                else if(e & IN_MOVED_TO) {
                    data.oldname = old_path;
                    data.name = event->name;
                    desc->on_event(desc->path, DIRWATCH_FILE_RENAMED, data);
                }
            }

            i += EVENT_SIZE + event->len;
        }
    }

    inotify_rm_watch(desc->fd, desc->wd);
    close(desc->fd);

    return 0;
error:
    return 1;
}

dirwatch_t watchDir(dirwatch_desc_t desc) {
    dirwatch_t dir = {0};

    __dirwatch_internal_t *opts = malloc(sizeof(__dirwatch_internal_t));
    opts->path = desc.path;
    opts->on_event = desc.on_event;
    opts->fd = 0;
    opts->wd = 0;
    opts->should_continue = true;

    dir.desc = (dirwatch_desc_t *)opts;

    dir.handle = thrCreate(watchDirThread, opts);
    
    return dir;
}

void waitForWatchDir(dirwatch_t *ctx) {
    thrJoin(ctx->handle, NULL);
    free(ctx->desc);
}

void stopWatchDir(dirwatch_t *ctx, bool immediately) {
    __dirwatch_internal_t *opts = (__dirwatch_internal_t *)ctx->desc;
    opts->should_continue = false;
    // this one might get called in the thread first, but it doesn't matter
    if(immediately) {
        inotify_rm_watch(opts->fd, opts->wd);
        close(opts->fd);
    }
    thrJoin(ctx->handle, NULL);
    free(opts);
}

#endif