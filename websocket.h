#pragma once

#include "arena.h"
#include "str.h"

#define WEBSOCKET_MAGIC    "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WEBSOCKET_HTTP_KEY "Sec-WebSocket-Key"

typedef uintptr_t socket_t;

bool wsInitialiseSocket(arena_t scratch, socket_t websocket, strview_t key);
buffer_t wsEncodeMessage(arena_t *arena, strview_t message);
str_t wsDecodeMessage(arena_t *arena, buffer_t message);