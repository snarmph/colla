#include <windows.h>
#include <assert.h>

#include "../os.h"
#include "../net.h"

#if COLLA_TCC
#include "../tcc/colla_tcc.h"
#endif

typedef enum os_entity_kind_e {
    OS_KIND_NULL,
    OS_KIND_THREAD,
    OS_KIND_MUTEX,
    OS_KIND_CONDITION_VARIABLE,
} os_entity_kind_e;

typedef struct os_entity_t os_entity_t;
struct os_entity_t {
    os_entity_t *next;
    os_entity_kind_e kind;
    union {
        struct {
            HANDLE handle;
            thread_func_t *func;
            void *userdata;
            DWORD id;
        } thread;
        CRITICAL_SECTION mutex;
        CONDITION_VARIABLE cv;
    };
};

struct {
    arena_t arena;
    os_system_info_t info;
    os_entity_t *entity_free;
    oshandle_t hstdout;
    oshandle_t hstdin;
} w32_data = {0};

os_entity_t *os__win_alloc_entity(os_entity_kind_e kind) {
    os_entity_t *entity = w32_data.entity_free;
    if (entity) {
        list_pop(w32_data.entity_free);
    }
    else {
        entity = alloc(&w32_data.arena, os_entity_t);
    }
    entity->kind = kind;
    return entity;
}

void os__win_free_entity(os_entity_t *entity) {
    entity->kind = OS_KIND_NULL;
    list_push(w32_data.entity_free, entity);
}

void os_init(void) {
    SetConsoleOutputCP(CP_UTF8);

    SYSTEM_INFO sysinfo = {0};
    GetSystemInfo(&sysinfo);

    os_system_info_t *info = &w32_data.info;
    info->processor_count = (u64)sysinfo.dwNumberOfProcessors;
    info->page_size = sysinfo.dwPageSize;

    w32_data.arena = arena_make(ARENA_VIRTUAL, OS_ARENA_SIZE);

    TCHAR namebuf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD namebuflen = sizeof(namebuf);
    BOOL result = GetComputerName(namebuf, &namebuflen);
    if (!result) {
        err("failed to get computer name: %v", os_get_error_string(os_get_last_error()));
    }

    info->machine_name = str_from_tstr(&w32_data.arena, (tstr_t){ namebuf, namebuflen});

    HANDLE hstdout = CreateFile(TEXT("CONOUT$"), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE hstdin = CreateFile(TEXT("CONIN$"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (hstdout == INVALID_HANDLE_VALUE) err("couldn't open CONOUT$");
    else w32_data.hstdout.data = (uptr)hstdout;

    if (hstdin == INVALID_HANDLE_VALUE) err("couldn't open CONIN$");
    else w32_data.hstdin.data = (uptr)hstdin;
}

void os_cleanup(void) {
    os_file_close(w32_data.hstdout);
    os_file_close(w32_data.hstdin);

    arena_cleanup(&w32_data.arena);
}

void os_abort(int code) {
    ExitProcess(code);
}

iptr os_get_last_error(void) {
    return (iptr)GetLastError();
}

str_t os_get_error_string(iptr error) {
    static u8 tmpbuf[1024] = {0};
    arena_t arena = arena_make(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);
    DWORD code = LOWORD(error);

    WCHAR msgbuf[512];
    DWORD chars;
    chars = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0, msgbuf, arrlen(msgbuf), NULL);
    if (chars == 0) {
        err("FormatMessageW: %ld", os_get_last_error());
        return STR_EMPTY;
    }

    debug("error: %zi 0x%zX", error, error);

    // remove \r\n at the end
    return str_from_str16(&arena, str16_init(msgbuf, chars - 2));
}

os_wait_t os_wait_on_handles(oshandle_t *handles, int count, bool wait_all, u32 milliseconds) {
    HANDLE win_handles[OS_MAX_WAITABLE_HANDLES] = {0};
    assert(count < MAXIMUM_WAIT_OBJECTS);

    for (int i = 0; i < count; ++i) {
        win_handles[i] = (HANDLE)(handles[i].data);
    }

    DWORD result = WaitForMultipleObjects(count, win_handles, wait_all, milliseconds);

    os_wait_t out = {0};

    if (result == WAIT_FAILED) {
        out.result = OS_WAIT_FAILED;
    }
    else if (result == WAIT_TIMEOUT) {
        out.result = OS_WAIT_TIMEOUT;
    }
    else if (result >= WAIT_ABANDONED_0) {
        out.result = OS_WAIT_ABANDONED;
        out.index = result - WAIT_ABANDONED_0;
    }
    else {
        out.result = OS_WAIT_FINISHED;
        out.index = result - WAIT_OBJECT_0;
    }

    return out;
}

os_system_info_t os_get_system_info(void) {
	return w32_data.info;
}

void os_log_set_colour(os_log_colour_e colour) {
    WORD attribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    switch (colour) {
        case LOG_COL_BLACK:   attribute = 0;                                   break;
        case LOG_COL_RED:     attribute = FOREGROUND_RED;                      break;
        case LOG_COL_GREEN:   attribute = FOREGROUND_GREEN;                    break;
        case LOG_COL_BLUE:    attribute = FOREGROUND_BLUE;                     break;
        case LOG_COL_MAGENTA: attribute = FOREGROUND_RED   | FOREGROUND_BLUE;  break;
        case LOG_COL_YELLOW:  attribute = FOREGROUND_RED   | FOREGROUND_GREEN; break;
        case LOG_COL_CYAN:    attribute = FOREGROUND_GREEN | FOREGROUND_BLUE;  break;
        default: break;
    }

    HANDLE hc = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hc, attribute | FOREGROUND_INTENSITY);
}

oshandle_t os_stdout(void) {
    return w32_data.hstdout;
}

oshandle_t os_stdin(void) {
    return w32_data.hstdin;
}

// == FILE ======================================

#define OS_SMALL_SCRATCH() \
u8 tmpbuf[KB(1)]; \
arena_t scratch = arena_make(ARENA_STATIC, sizeof(tmpbuf), tmpbuf) 

DWORD os__win_mode_to_access(filemode_e mode) {
    DWORD out = 0;
    if (mode & FILEMODE_READ)  out |= GENERIC_READ;
    if (mode & FILEMODE_WRITE) out |= GENERIC_WRITE;
    return out;
}

DWORD os__win_mode_to_creation(filemode_e mode) {
    if (mode == FILEMODE_READ)  return OPEN_EXISTING;
    if (mode == FILEMODE_WRITE) return CREATE_ALWAYS;
    return OPEN_ALWAYS;
}

bool os_file_exists(strview_t path) {
    OS_SMALL_SCRATCH();
    tstr_t name = strv_to_tstr(&scratch, path);
    DWORD attributes = GetFileAttributes(name.buf);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

tstr_t os_file_fullpath(arena_t *arena, strview_t filename) {
    OS_SMALL_SCRATCH();

    TCHAR long_path_prefix[] = TEXT("\\\\?\\");
    const usize prefix_len = arrlen(long_path_prefix) - 1;

    tstr_t rel_path = strv_to_tstr(&scratch, filename);
    DWORD pathlen = GetFullPathName(rel_path.buf, 0, NULL, NULL);

    tstr_t full_path = {
        .buf = alloc(arena, TCHAR, pathlen + prefix_len + 1),
        .len = pathlen + prefix_len,
    };
    memcpy(full_path.buf, long_path_prefix, prefix_len * sizeof(TCHAR));

    GetFullPathName(rel_path.buf, pathlen + 1, full_path.buf + prefix_len, NULL);

    return full_path;
}

bool os_file_delete(strview_t path) {
    OS_SMALL_SCRATCH();
    tstr_t fname = strv_to_tstr(&scratch, path);
    return DeleteFile(fname.buf);
}

oshandle_t os_file_open(strview_t path, filemode_e mode) {
    OS_SMALL_SCRATCH();

    tstr_t full_path = os_file_fullpath(&scratch, path);

    HANDLE handle = CreateFile(
        full_path.buf,
        os__win_mode_to_access(mode),
        FILE_SHARE_READ,
        NULL,
        os__win_mode_to_creation(mode),
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return (oshandle_t){
        .data = (uptr)handle
    };
}

void os_file_close(oshandle_t handle) {
    if (!os_handle_valid(handle)) return;
    CloseHandle((HANDLE)handle.data);    
}

usize os_file_read(oshandle_t handle, void *buf, usize len) {
    if (!os_handle_valid(handle)) return 0;
    DWORD read = 0;
    ReadFile((HANDLE)handle.data, buf, (DWORD)len, &read, NULL);
    return (usize)read;
}

usize os_file_write(oshandle_t handle, const void *buf, usize len) {
    if (!os_handle_valid(handle)) return 0;
    DWORD written = 0;
    WriteFile((HANDLE)handle.data, buf, (DWORD)len, &written, NULL);
    return (usize)written;
}

bool os_file_seek(oshandle_t handle, usize offset) {
    if (!os_handle_valid(handle)) return false;
    LARGE_INTEGER offset_large = {
        .QuadPart = offset,
    };
    DWORD result = SetFilePointer((HANDLE)handle.data, offset_large.LowPart, &offset_large.HighPart, FILE_BEGIN);
    return result != INVALID_SET_FILE_POINTER;
}

bool os_file_seek_end(oshandle_t handle) {
    if (!os_handle_valid(handle)) return false;
    DWORD result = SetFilePointer((HANDLE)handle.data, 0, NULL, FILE_END);
    return result != INVALID_SET_FILE_POINTER;
}

void os_file_rewind(oshandle_t handle) {
    if (!os_handle_valid(handle)) return;
    SetFilePointer((HANDLE)handle.data, 0, NULL, FILE_BEGIN);
}

usize os_file_tell(oshandle_t handle) {
    if (!os_handle_valid(handle)) return 0;
    LARGE_INTEGER tell = {0};
    BOOL result = SetFilePointerEx((HANDLE)handle.data, (LARGE_INTEGER){0}, &tell, FILE_CURRENT);
    return result == TRUE ? (usize)tell.QuadPart : 0;
}

usize os_file_size(oshandle_t handle) {
    if (!os_handle_valid(handle)) return 0;
    LARGE_INTEGER size = {0};
    BOOL result = GetFileSizeEx((HANDLE)handle.data, &size);
    return result == TRUE ? (usize)size.QuadPart : 0;
}

bool os_file_is_finished(oshandle_t handle) {
    if (!os_handle_valid(handle)) return 0;

    char tmp = 0;
    DWORD read = 0;
    BOOL success = ReadFile((HANDLE)handle.data, &tmp, sizeof(tmp), &read, NULL);
    bool is_finished = success && read == 0;
    
    if (!is_finished) {
        SetFilePointer((HANDLE)handle.data, -1, NULL, FILE_CURRENT);
    }

    return is_finished;
}

u64 os_file_time_fp(oshandle_t handle) {
    if (!os_handle_valid(handle)) return 0;
    FILETIME time = {0};
    GetFileTime((HANDLE)handle.data, NULL, NULL, &time);
    ULARGE_INTEGER utime = {
        .HighPart = time.dwHighDateTime,
        .LowPart = time.dwLowDateTime,
    };
    return (u64)utime.QuadPart;
}

// == DIR WALKER ================================

typedef struct dir_t {
    WIN32_FIND_DATA find_data;
    HANDLE handle;
    dir_entry_t cur_entry;
    dir_entry_t next_entry;
} dir_t;

dir_entry_t os__dir_entry_from_find_data(arena_t *arena, WIN32_FIND_DATA *fd) {
    dir_entry_t out = {0};

    out.name = str_from_tstr(arena, tstr_init(fd->cFileName, 0));

    if (fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        out.type = DIRTYPE_DIR;
    }
    else {
        LARGE_INTEGER filesize = {
            .LowPart  = fd->nFileSizeLow,
            .HighPart = fd->nFileSizeHigh,
        };
        out.file_size = filesize.QuadPart;
    }

    return out;
}

dir_t *os_dir_open(arena_t *arena, strview_t path) {
    u8 tmpbuf[KB(1)] = {0};
    arena_t scratch = arena_make(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);
    
    tstr_t winpath = strv_to_tstr(&scratch, path);
    // get a little extra leeway
    TCHAR fullpath[MAX_PATH + 16] = {0};
    DWORD pathlen = GetFullPathName(winpath.buf, MAX_PATH, fullpath, NULL);
    // add asterisk at the end of the path
    if (fullpath[pathlen] != '\\' && fullpath[pathlen] != '/') {
        fullpath[pathlen++] = '\\';
    }
    fullpath[pathlen++] = '*';
    fullpath[pathlen++] = '\0';

    dir_t *ctx = alloc(arena, dir_t);
    ctx->handle = FindFirstFile(fullpath, &ctx->find_data);

    if (ctx->handle == INVALID_HANDLE_VALUE) {
        arena_pop(arena, sizeof(dir_t));
        return NULL;
    }

    ctx->next_entry = os__dir_entry_from_find_data(arena, &ctx->find_data);

    return ctx;
}

void os_dir_close(dir_t *dir) {
    FindClose(dir->handle);
    dir->handle = INVALID_HANDLE_VALUE;
}

bool os_dir_is_valid(dir_t *dir) {
    return dir && dir->handle != INVALID_HANDLE_VALUE;
}

dir_entry_t *os_dir_next(arena_t *arena, dir_t *dir) {
    if (!os_dir_is_valid(dir)) {
        return NULL;
    }

    dir->cur_entry = dir->next_entry;

    dir->next_entry = (dir_entry_t){0};

    if (FindNextFile(dir->handle, &dir->find_data)) {
        dir->next_entry = os__dir_entry_from_find_data(arena, &dir->find_data);
    }
    else {
        os_dir_close(dir);
    }
    
    return &dir->cur_entry;
}

// == PROCESS ===================================

struct os_env_t {
    void *data;
};

void os_set_env_var(arena_t scratch, strview_t key, strview_t value) {
    str16_t k = strv_to_str16(&scratch, key);
    str16_t v = strv_to_str16(&scratch, value);
    SetEnvironmentVariableW(k.buf, v.buf);
}

str_t os_get_env_var(arena_t *arena, strview_t key) {
    u8 tmpbuf[KB(1)];
    arena_t scratch = arena_make(ARENA_STATIC, sizeof(tmpbuf), tmpbuf);

    wchar_t static_buf[1024] = {0};
    wchar_t *buf = static_buf;

    str16_t k = strv_to_str16(&scratch, key);
    DWORD len = GetEnvironmentVariableW(k.buf, static_buf, arrlen(static_buf));

    if (len > arrlen(static_buf)) {
        buf = alloc(&scratch, wchar_t, len);
        len = GetEnvironmentVariableW(k.buf, buf, len);
    }

    return str_from_str16(arena, str16_init(buf, len));
}

os_env_t *os_get_env(arena_t *arena) {
    os_env_t *out = alloc(arena, os_env_t);
    out->data = GetEnvironmentStringsW();
    return out;
}

oshandle_t os_run_cmd_async(arena_t scratch, os_cmd_t *cmd, os_env_t *optional_env) {
    STARTUPINFOW start_info = {
        .cb = sizeof(STARTUPINFO),
        .hStdError = GetStdHandle(STD_ERROR_HANDLE),
        .hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE),
        .hStdInput = GetStdHandle(STD_INPUT_HANDLE),
        .dwFlags = STARTF_USESTDHANDLES,
    };
    
    PROCESS_INFORMATION proc_info = {0};

    outstream_t cmdline = ostr_init(&scratch);

    for_each (cur, cmd->head ? cmd->head : cmd) {
        for (int i = 0; i < cur->count; ++i) {
            strview_t arg = cur->items[i];
            if (strv_contains(arg, ' ')) {
                ostr_print(&cmdline, "\"%v\"", arg);
            }
            else {
                ostr_puts(&cmdline, arg);
            }
            ostr_putc(&cmdline, ' ');
        }
    }

    ostr_pop(&cmdline, 1);
    ostr_putc(&cmdline, '\0');

    strview_t cmd_view = ostr_as_view(&cmdline);
    str16_t command = strv_to_str16(&scratch, cmd_view);

    WCHAR *env = optional_env ? optional_env->data : NULL;

    BOOL success = CreateProcessW(
        NULL,
        command.buf,
        NULL,
        NULL,
        TRUE,
        0,
        env,
        NULL,
        &start_info, 
        &proc_info
    );

    if (env) {
        FreeEnvironmentStringsW(env);
        optional_env->data = NULL;
    }

    if (!success) {
        err("couldn't create process (%v): %v", cmd_view, os_get_error_string(os_get_last_error()));
        return os_handle_zero();
    }

    CloseHandle(proc_info.hThread);

    return (oshandle_t) {
        .data = (uptr)proc_info.hProcess
    };
}

bool os_process_wait(oshandle_t proc, uint time, int *out_exit) {
    if (!os_handle_valid(proc)) {
        err("waiting on invalid handle");
        return false;
    }

    DWORD result = WaitForSingleObject((HANDLE)proc.data, (DWORD)time);

    if (result == WAIT_TIMEOUT) {
        return false;
    }

    if (result != WAIT_OBJECT_0) {
        err("could not wait for proces: %v", os_get_error_string(os_get_last_error()));
        return false;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess((HANDLE)proc.data, &exit_status)) {
        err("could not get exit status from process: %v", os_get_error_string(os_get_last_error()));
        return false;
    }

    CloseHandle((HANDLE)proc.data);

    if (out_exit) {
        *out_exit = exit_status;
    }

    return exit_status == 0;
}


// == VMEM ======================================

void *os_reserve(usize size, usize *out_padded_size) {
    usize alloc_size = os_pad_to_page(size);
    void *ptr = VirtualAlloc(NULL, alloc_size, MEM_RESERVE, PAGE_NOACCESS);
    if (out_padded_size) {
        *out_padded_size = alloc_size;
    }
    return ptr;
}

bool os_commit(void *ptr, usize num_of_pages) {
    usize page_size = os_get_system_info().page_size;
    void *new_ptr = VirtualAlloc(ptr, num_of_pages * page_size, MEM_COMMIT, PAGE_READWRITE);
    return new_ptr != NULL;
}

bool os_release(void *ptr, usize size) {
    COLLA_UNUSED(size);
    return VirtualFree(ptr, 0, MEM_RELEASE);
}

// == THREAD ====================================

DWORD os__win_thread_entry_point(void *ptr) {
    os_entity_t *entity = (os_entity_t *)ptr;
    thread_func_t *func = entity->thread.func;
    void *userdata = entity->thread.userdata;
    u64 id = entity->thread.id;
    return func(id, userdata);
}

oshandle_t os_thread_launch(thread_func_t func, void *userdata) {
    os_entity_t *entity = os__win_alloc_entity(OS_KIND_THREAD);

    entity->thread.func = func;
    entity->thread.userdata = userdata;
    entity->thread.handle = CreateThread(NULL, 0, os__win_thread_entry_point, entity, 0, &entity->thread.id);

    return (oshandle_t){ (uptr)entity };
}

bool os_thread_detach(oshandle_t thread) {
    if (!os_handle_valid(thread)) return false;
    os_entity_t *entity = (os_entity_t *)thread.data;
    BOOL result = CloseHandle(entity->thread.handle);
    os__win_free_entity(entity);
    return result;
}

bool os_thread_join(oshandle_t thread, int *code) {
    if (!os_handle_valid(thread)) return false;
    os_entity_t *entity = (os_entity_t *)thread.data;
    int return_code = WaitForSingleObject(entity->thread.handle, INFINITE);
    if (code) *code = return_code;
    BOOL result = CloseHandle(entity->thread.handle);
    os__win_free_entity(entity);
    return return_code != WAIT_FAILED && result;
}   

u64 os_thread_get_id(oshandle_t thread) {
    if (!os_handle_valid(thread)) return 0;
    os_entity_t *entity = (os_entity_t *)thread.data;
    return entity->thread.id;
}

// == MUTEX =====================================

oshandle_t os_mutex_create(void) {
    os_entity_t *entity = os__win_alloc_entity(OS_KIND_MUTEX);

    InitializeCriticalSection(&entity->mutex);

    return (oshandle_t){ (uptr)entity };
}

void os_mutex_free(oshandle_t mutex) {
    if (!os_handle_valid(mutex)) return;
    os_entity_t *entity = (os_entity_t *)mutex.data;
    DeleteCriticalSection(&entity->mutex);
    os__win_free_entity(entity);
}

void os_mutex_lock(oshandle_t mutex) {
    if (!os_handle_valid(mutex)) return;
    os_entity_t *entity = (os_entity_t *)mutex.data;
    EnterCriticalSection(&entity->mutex);
}

void os_mutex_unlock(oshandle_t mutex) {
    if (!os_handle_valid(mutex)) return;
    os_entity_t *entity = (os_entity_t *)mutex.data;
    LeaveCriticalSection(&entity->mutex);
}

bool os_mutex_try_lock(oshandle_t mutex) {
    if (!os_handle_valid(mutex)) return false;
    os_entity_t *entity = (os_entity_t *)mutex.data;
    return TryEnterCriticalSection(&entity->mutex);
}

#if !COLLA_NO_CONDITION_VARIABLE

// == CONDITION VARIABLE ========================

oshandle_t os_cond_create(void) {
    os_entity_t *entity = os__win_alloc_entity(OS_KIND_CONDITION_VARIABLE);

    InitializeConditionVariable(&entity->cv);

    return (oshandle_t){ (uptr)entity };
}

void os_cond_free(oshandle_t cond) {
    if (!os_handle_valid(cond)) return;
    os_entity_t *entity = (os_entity_t *)cond.data;
    os__win_free_entity(entity);
}

void os_cond_signal(oshandle_t cond) {
    if (!os_handle_valid(cond)) return;
    os_entity_t *entity = (os_entity_t *)cond.data;
    WakeConditionVariable(&entity->cv);
}

void os_cond_broadcast(oshandle_t cond) {
    if (!os_handle_valid(cond)) return;
    os_entity_t *entity = (os_entity_t *)cond.data;
    WakeAllConditionVariable(&entity->cv);
}

void os_cond_wait(oshandle_t cond, oshandle_t mutex, int milliseconds) {
    if (!os_handle_valid(cond)) return;
    os_entity_t *entity_cv  = (os_entity_t *)cond.data;
    os_entity_t *entity_mtx = (os_entity_t *)mutex.data;
    SleepConditionVariableCS(&entity_cv->cv, &entity_mtx->mutex, milliseconds);
}

#endif
