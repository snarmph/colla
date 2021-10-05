#pragma once

#include <stddef.h> // size_t

#define slice_t(type) struct { type buf; size_t len; }
