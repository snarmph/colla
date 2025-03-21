#include "os.h"

#if COLLA_WIN
	#include "win/os_win32.c"
#else
	#error "platform not supported yet"
#endif

// == HANDLE ====================================

oshandle_t os_handle_zero(void) {
	return (oshandle_t){0};
}

bool os_handle_match(oshandle_t a, oshandle_t b) {
	return a.data == b.data;
}

bool os_handle_valid(oshandle_t handle) {
	return !os_handle_match(handle, os_handle_zero());
}

// == LOGGING ===================================

os_log_colour_e log__level_to_colour(os_log_level_e level) {
    os_log_colour_e colour = LOG_COL_RESET;
    switch (level) {
        case LOG_DEBUG: colour = LOG_COL_BLUE;    break;
        case LOG_INFO:  colour = LOG_COL_GREEN;   break;
        case LOG_WARN:  colour = LOG_COL_YELLOW;  break;
        case LOG_ERR:   colour = LOG_COL_MAGENTA; break;
        case LOG_FATAL: colour = LOG_COL_RED;     break;    
        default: break;
    }
	return colour;
}

void os_log_print(os_log_level_e level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    os_log_printv(level, fmt, args);
    va_end(args);
}

void os_log_printv(os_log_level_e level, const char *fmt, va_list args) {
    const char *level_str = "";
    switch (level) {
        case LOG_DEBUG: level_str = "DEBUG"; break;
        case LOG_INFO:  level_str = "INFO";  break;
        case LOG_WARN:  level_str = "WARN";  break;
        case LOG_ERR:   level_str = "ERR";   break;
        case LOG_FATAL: level_str = "FATAL"; break;  
		default: break;  
    }

	os_log_set_colour(log__level_to_colour(level));
    if (level != LOG_BASIC) {
        fmt_print("[%s]: ", level_str);
    }
    os_log_set_colour(LOG_COL_RESET);

    fmt_printv(fmt, args);
    fmt_print("\n");

    if (level == LOG_FATAL) {
        os_abort(1);
    }
}

// == FILE ======================================

void os_file_split_path(strview_t path, strview_t *dir, strview_t *name, strview_t *ext) {
	usize dir_lin = strv_rfind(path, '/', 0);
	usize dir_win = strv_rfind(path, '\\', 0);
	dir_lin = dir_lin != STR_NONE ? dir_lin : 0;
	dir_win = dir_win != STR_NONE ? dir_win : 0;
	usize dir_pos = MAX(dir_lin, dir_win);

	usize ext_pos = strv_rfind(path, '.', 0);

	if (dir) {
		*dir = strv_sub(path, 0, dir_pos);
	}
	if (name) {
		*name = strv_sub(path, dir_pos ? dir_pos + 1 : 0, ext_pos);
	}
	if (ext) {
		*ext = strv_sub(path, ext_pos, SIZE_MAX);
	}
}

bool os_file_putc(oshandle_t handle, char c) {
	return os_file_write(handle, &c, sizeof(c)) == sizeof(c);
}

bool os_file_puts(oshandle_t handle, strview_t str) {
	return os_file_write(handle, str.buf, str.len) == str.len;
}

bool os_file_print(arena_t scratch, oshandle_t handle, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	bool result = os_file_printv(scratch, handle, fmt, args);
	va_end(args);
	return result;
}	

bool os_file_printv(arena_t scratch, oshandle_t handle, const char *fmt, va_list args) {
	str_t s = str_fmtv(&scratch, fmt, args);
	return os_file_puts(handle, strv(s));
}

usize os_file_read_buf(oshandle_t handle, buffer_t *buf) {
	return os_file_read(handle, buf->data, buf->len);
}

usize os_file_write_buf(oshandle_t handle, buffer_t buf) {
	return os_file_write(handle, buf.data, buf.len);
}

buffer_t os_file_read_all(arena_t *arena, strview_t path) {
	oshandle_t fp = os_file_open(path, FILEMODE_READ);
	if (!os_handle_valid(fp)) {
		err("could not open file: %v", path);
		return (buffer_t){0};
	}
	buffer_t out = os_file_read_all_fp(arena, fp);
	os_file_close(fp);
	return out;
}

buffer_t os_file_read_all_fp(arena_t *arena, oshandle_t handle) {
	if (!os_handle_valid(handle)) return (buffer_t){0};
	buffer_t out = {0};
	
	out.len = os_file_size(handle);
	out.data = alloc(arena, u8, out.len);
	usize read = os_file_read_buf(handle, &out);
	
	if (read != out.len) {
		err("os_file_read_all_fp: read failed, should be %zu but is %zu", out.len, read);
		arena_pop(arena, out.len);
		return (buffer_t){0};
	}
	
	return out;
}

str_t os_file_read_all_str(arena_t *arena, strview_t path) {
	oshandle_t fp = os_file_open(path, FILEMODE_READ);
	if (!os_handle_valid(fp)) {
		err("could not open file %v", path);
		return STR_EMPTY;
	}
	str_t out = os_file_read_all_str_fp(arena, fp);
	os_file_close(fp);
	return out;
}

str_t os_file_read_all_str_fp(arena_t *arena, oshandle_t handle) {
	if (!os_handle_valid(handle)) {
		return STR_EMPTY;	
	}
	
	str_t out = STR_EMPTY;
	
	out.len = os_file_size(handle);
	out.buf = alloc(arena, u8, out.len + 1);
	
	usize read = os_file_read(handle, out.buf, out.len);
	if (read != out.len) {
		err("os_file_read_all_str_fp: read failed, should be %zu but is %zu", out.len, read);
		arena_pop(arena, out.len + 1);	
		return STR_EMPTY;
	}
	
	return out;
}

bool os_file_write_all(strview_t name, buffer_t buffer) {
	oshandle_t fp = os_file_open(name, FILEMODE_WRITE);
	bool result = os_file_write_all_fp(fp, buffer);
	os_file_close(fp);
	return result;
}

bool os_file_write_all_fp(oshandle_t handle, buffer_t buffer) {
	return os_file_write(handle, buffer.data, buffer.len) == buffer.len;
}

bool os_file_write_all_str(strview_t name, strview_t data) {
	oshandle_t fp = os_file_open(name, FILEMODE_WRITE);
	bool result = os_file_write_all_str_fp(fp, data);
	os_file_close(fp);
	return result;
}

bool os_file_write_all_str_fp(oshandle_t handle, strview_t data) {
	return os_file_write(handle, data.buf, data.len) == data.len;
}

u64 os_file_time(strview_t path) {
	oshandle_t fp = os_file_open(path, FILEMODE_READ);
	u64 result = os_file_time_fp(fp);
	os_file_close(fp);
	return result;
}

bool os_file_has_changed(strview_t path, u64 last_change) {
	u64 timestamp = os_file_time(path);
	return timestamp > last_change;
}

// == PROCESS ===================================

bool os_run_cmd(arena_t scratch, os_cmd_t *cmd, os_env_t *optional_env) {
	oshandle_t proc = os_run_cmd_async(scratch, cmd, optional_env);
	return os_handle_valid(proc) ? os_process_wait(proc, OS_WAIT_INFINITE, NULL) : false;
}

// == VMEM ======================================

usize os_pad_to_page(usize byte_count) {
	usize page_size = os_get_system_info().page_size;

    if (byte_count == 0) {
        return page_size;
    }

    usize padding = page_size - (byte_count & (page_size - 1));
    if (padding == page_size) {
        padding = 0;
    }
    return byte_count + padding;
}
