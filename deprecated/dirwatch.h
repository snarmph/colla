#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Basic usage:
 * 
 * // this function will be called from another thread every time
 * // something happens to 'path'
 * void onEvent(const char *path, int action, dirwatch_file_t file) {
 *     switch(action) {
 *         case DIRWATCH_FILE_ADDED:   [do something] break;
 *         case DIRWATCH_FILE_REMOVED: [do something] break;
 *         case DIRWATCH_FILE_RENAMED: [do something] break;
 *     }
 * }
 * 
 * int main() {
 *     dirwatch_t dir = watchDir((dirwatch_desc_t){
 *         .dirname = "watch/",
 *         .on_event = onEvent
 *     });
 * 
 *     waitForWatchDir(&dir);
 * }
 */

#include <stdbool.h>
#include "cthreads.h"

enum {
    DIRWATCH_FILE_ADDED,
    DIRWATCH_FILE_REMOVED,
    DIRWATCH_FILE_RENAMED,
};

typedef struct {
    char *name;
    char *oldname;
} dirwatch_file_t;

typedef void (*dirwatch_cb_t)(const char *path, int action, dirwatch_file_t data);

typedef struct {
    const char *path;
    dirwatch_cb_t on_event;
} dirwatch_desc_t;

typedef struct {
    cthread_t handle;
    dirwatch_desc_t *desc;
} dirwatch_t;

// Creates a thread and starts watching the folder specified by desc
// if any action happend on_event will be called from this thread
dirwatch_t watchDir(dirwatch_desc_t desc);
// waits forever
void waitForWatchDir(dirwatch_t *ctx);
// stops dirwatch thread, if immediately is true, it will try to close it right away
// otherwise it might wait for one last event 
void stopWatchDir(dirwatch_t *ctx, bool immediately);

#ifdef __cplusplus
} // extern "C"
#endif
