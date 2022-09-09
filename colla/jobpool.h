#pragma once

#include <collatypes.h>
#include <cthreads.h>

typedef void *jobpool_t;

jobpool_t poolInit(uint32 num);
void poolFree(jobpool_t pool);

bool poolAdd(jobpool_t pool, cthread_func_t func, void *arg);
void poolWait(jobpool_t pool);
