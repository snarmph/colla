#pragma once

#include "collatypes.h"
#include "vec.h"
#include "str.h"

/*
Example usage:
hashSetSeed(time(NULL));
vec(const char *) strings = NULL;
hashmap_t map = hmInit(32);

// mapGet returns 0 in case it doesn't find anything, this way we don't need
// to check its return value
vecAppend(strings, "nil");

hmSet(&map, hashCStr("english"), vecAppend(strings, "hello"));
hmSet(&map, hashCStr("french"),  vecAppend(strings, "bonjour"));
hmSet(&map, hashCStr("italian"), vecAppend(strings, "ciao"));

printf("english: %s\n", strings[hmGet(map, hashCStr("english"))]);
printf("french: %s\n",  strings[hmGet(map, hashCStr("french"))]);
printf("italian: %s\n", strings[hmGet(map, hashCStr("italian"))]);

mapFree(map);
vecFree(strings);
*/

typedef struct {
    uint64 hash;
    uint64 index;
} hashnode_t;

typedef struct {
    vec(hashnode_t) nodes;
} hashmap_t;

hashmap_t hmInit(usize initial_cap);
void hmFree(hashmap_t map);

void hmSet(hashmap_t *map, uint64 hash, uint64 index);
uint64 hmGet(hashmap_t map, uint64 hash);
void hmDelete(hashmap_t *map, uint64 hash);

void hashSetSeed(uint64 new_seed);
uint64 hash(const void *data, usize len);
uint64 hashStr(str_t str);
uint64 hashView(strview_t view);
uint64 hashCStr(const char *cstr);
