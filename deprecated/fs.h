#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "collatypes.h"

#include "file.h"

typedef enum {
    FS_MODE_FILE,
    FS_MODE_DIR,
    FS_MODE_CHARACTER_DEVICE,
    FS_MODE_FIFO,
    FS_MODE_UKNOWN,
} fsmode_t;

typedef struct {
    fsmode_t type;
    uint64_t size;
    int64_t last_access;
    int64_t last_modif;
} fs_stat_t;

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minutes;
    int seconds;
    bool daylight_saving;
} fs_time_t;

fs_stat_t fsStat(file_t fp);
fs_time_t fsAsTime(int64_t time);

bool fsIsDir(const char *path);

#ifdef __cplusplus
} // extern "C"
#endif
