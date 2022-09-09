#pragma once

#include <collatypes.h>
#include <cthreads.h>

#include "minicoro.h"

typedef void *coropool_t;
typedef void (*copo_func_f)(mco_coro *co);

coropool_t copoInit(uint32 num);
void copoFree(coropool_t copo);

bool copoAdd(coropool_t copo, copo_func_f fn);
bool copoAddDesc(coropool_t copo, mco_desc *desc);
bool copoAddCo(coropool_t copo, mco_coro *co);
void copoWait(coropool_t copo);
