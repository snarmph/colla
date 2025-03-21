#ifndef COLLA_OS_H
#define COLLA_OS_H

#include "core.h"
#include "str.h"
#include "arena.h"

#define OS_ARENA_SIZE    (MB(1))
#define OS_WAIT_INFINITE (0xFFFFFFFF)

typedef struct oshandle_t oshandle_t;
struct oshandle_t {
    uptr data;
};

typedef struct os_system_info_t os_system_info_t;
struct os_system_info_t {
    u32 processor_count;
    u64 page_size;
    str_t machine_name;
};

void os_init(void);
void os_cleanup(void);
os_system_info_t os_get_system_info(void);
void os_abort(int code);

iptr os_get_last_error(void);
// NOT thread safe
str_t os_get_error_string(iptr error);

// == HANDLE ====================================

oshandle_t os_handle_zero(void);
bool os_handle_match(oshandle_t a, oshandle_t b);
bool os_handle_valid(oshandle_t handle);

#define OS_MAX_WAITABLE_HANDLES 256

typedef enum {
    OS_WAIT_FINISHED,
    OS_WAIT_ABANDONED,
    OS_WAIT_TIMEOUT,
    OS_WAIT_FAILED
} os_wait_result_e;

typedef struct {
    os_wait_result_e result;
    u32 index;
} os_wait_t;

os_wait_t os_wait_on_handles(oshandle_t *handles, int count, bool wait_all, u32 milliseconds);

// == LOGGING ===================================

typedef enum os_log_level_e {
    LOG_BASIC,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERR,
    LOG_FATAL,
} os_log_level_e;

typedef enum os_log_colour_e {
    LOG_COL_RESET,
    LOG_COL_BLACK,
    LOG_COL_BLUE,
    LOG_COL_GREEN,
    LOG_COL_CYAN,
    LOG_COL_RED,
    LOG_COL_MAGENTA,
    LOG_COL_YELLOW,
    LOG_COL_WHITE,
} os_log_colour_e;

void os_log_print(os_log_level_e level, const char *fmt, ...);
void os_log_printv(os_log_level_e level, const char *fmt, va_list args);
void os_log_set_colour(os_log_colour_e colour);

oshandle_t os_stdout(void);
oshandle_t os_stdin(void);

#define print(...)   fmt_print(__VA_ARGS__)
#define println(...) os_log_print(LOG_BASIC, __VA_ARGS__)
#define debug(...)   os_log_print(LOG_DEBUG, __VA_ARGS__)
#define info(...)    os_log_print(LOG_INFO,  __VA_ARGS__)
#define warn(...)    os_log_print(LOG_WARN,  __VA_ARGS__)
#define err(...)     os_log_print(LOG_ERR,   __VA_ARGS__)
#define fatal(...)   os_log_print(LOG_FATAL, __VA_ARGS__)

// == FILE ======================================

typedef enum filemode_e {
    FILEMODE_READ  = 1 << 0,
    FILEMODE_WRITE = 1 << 1,
} filemode_e;

bool os_file_exists(strview_t path);
tstr_t os_file_fullpath(arena_t *arena, strview_t filename);
void os_file_split_path(strview_t path, strview_t *dir, strview_t *name, strview_t *ext);
bool os_file_delete(strview_t path);

oshandle_t os_file_open(strview_t path, filemode_e mode);
void os_file_close(oshandle_t handle);

bool os_file_putc(oshandle_t handle, char c);
bool os_file_puts(oshandle_t handle, strview_t str);
bool os_file_print(arena_t scratch, oshandle_t handle, const char *fmt, ...);
bool os_file_printv(arena_t scratch, oshandle_t handle, const char *fmt, va_list args);

usize os_file_read(oshandle_t handle, void *buf, usize len);
usize os_file_write(oshandle_t handle, const void *buf, usize len);

usize os_file_read_buf(oshandle_t handle, buffer_t *buf);
usize os_file_write_buf(oshandle_t handle, buffer_t buf);

bool os_file_seek(oshandle_t handle, usize offset);
bool os_file_seek_end(oshandle_t handle);
void os_file_rewind(oshandle_t handle);
usize os_file_tell(oshandle_t handle);
usize os_file_size(oshandle_t handle);
bool os_file_is_finished(oshandle_t handle);

buffer_t os_file_read_all(arena_t *arena, strview_t path);
buffer_t os_file_read_all_fp(arena_t *arena, oshandle_t handle);

str_t os_file_read_all_str(arena_t *arena, strview_t path);
str_t os_file_read_all_str_fp(arena_t *arena, oshandle_t handle);

bool os_file_write_all(strview_t name, buffer_t buffer);
bool os_file_write_all_fp(oshandle_t handle, buffer_t buffer);

bool os_file_write_all_str(strview_t name, strview_t data);
bool os_file_write_all_str_fp(oshandle_t handle, strview_t data);

u64 os_file_time(strview_t path);
u64 os_file_time_fp(oshandle_t handle);
bool os_file_has_changed(strview_t path, u64 last_change);

// == DIR WALKER ================================

typedef enum dir_type_e {
    DIRTYPE_FILE,
    DIRTYPE_DIR,
} dir_type_e;

typedef struct dir_entry_t dir_entry_t;
struct dir_entry_t {
    str_t name;
    dir_type_e type;
    usize file_size;
};

#define dir_foreach(arena, it, dir) for (dir_entry_t *it = os_dir_next(arena, dir); it; it = os_dir_next(arena, dir))

typedef struct dir_t dir_t;
dir_t *os_dir_open(arena_t *arena, strview_t path);
bool os_dir_is_valid(dir_t *dir);
// optional, only call this if you want to return before os_dir_next returns NULL
void os_dir_close(dir_t *dir);

dir_entry_t *os_dir_next(arena_t *arena, dir_t *dir);

// == PROCESS ===================================

typedef struct os_env_t os_env_t;
typedef strv_list_t os_cmd_t;
#define os_make_cmd(...) &(os_cmd_t){ .items = (strview_t[]){ __VA_ARGS__ }, .count = arrlen(((strview_t[]){ __VA_ARGS__ })) }

void os_set_env_var(arena_t scratch, strview_t key, strview_t value);
str_t os_get_env_var(arena_t *arena, strview_t key);
os_env_t *os_get_env(arena_t *arena);
bool os_run_cmd(arena_t scratch, os_cmd_t *cmd, os_env_t *optional_env);
oshandle_t os_run_cmd_async(arena_t scratch, os_cmd_t *cmd, os_env_t *optional_env);
bool os_process_wait(oshandle_t proc, uint time, int *out_exit);

// == VMEM ======================================

void *os_reserve(usize size, usize *out_padded_size);
bool os_commit(void *ptr, usize num_of_pages);
bool os_release(void *ptr, usize size);
usize os_pad_to_page(usize byte_count);

// == THREAD ====================================

typedef int (thread_func_t)(u64 thread_id, void *userdata);

oshandle_t os_thread_launch(thread_func_t func, void *userdata);
bool os_thread_detach(oshandle_t thread);
bool os_thread_join(oshandle_t thread, int *code);

u64 os_thread_get_id(oshandle_t thread);

// == MUTEX =====================================

oshandle_t os_mutex_create(void);
void os_mutex_free(oshandle_t mutex);
void os_mutex_lock(oshandle_t mutex);
void os_mutex_unlock(oshandle_t mutex);
bool os_mutex_try_lock(oshandle_t mutex);

#if !COLLA_NO_CONDITION_VARIABLE
// == CONDITION VARIABLE ========================

oshandle_t os_cond_create(void);
void os_cond_free(oshandle_t cond);

void os_cond_signal(oshandle_t cond);
void os_cond_broadcast(oshandle_t cond);

void os_cond_wait(oshandle_t cond, oshandle_t mutex, int milliseconds);

#endif

#endif