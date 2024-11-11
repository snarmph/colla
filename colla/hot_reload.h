#pragma once

// api functions:
// int hr_init(hr_t *ctx)
// int hr_loop(hr_t *ctx)
// int hr_close(hr_t *ctx)

// you can turn off hot reloading and run the program 
// "as normal" by defining
// HR_DISABLE

#include "str.h"

#if COLLA_WIN
#define HR_EXPORT __declspec(dllexport)
#else
// todo linux support?
#define HR_EXPORT
#endif

typedef struct hr_t {
    void *p;
    void *userdata;
    int version;
    int last_working_version;
} hr_t;

bool hrOpen(hr_t *ctx, strview_t path);
void hrClose(hr_t *ctx, bool clean_temp_files);
int hrStep(hr_t *ctx);
bool hrReload(hr_t *ctx);
