#include "hashmap.h"

#include <string.h>

static uint64 hash_seed = 0;

hashmap_t hmInit(usize initial_cap) {
    hashmap_t map = {0};
    if (!initial_cap) initial_cap = 512;
    vecReserve(map.nodes, initial_cap);
    memset(map.nodes, 0, sizeof(hashnode_t) * initial_cap);
    return map;
}

void hmFree(hashmap_t map) {
    vecFree(map.nodes);
}

void hmSet(hashmap_t *map, uint64 hash, uint64 index) {
    uint32 hm_index = hash % vecCap(map->nodes);

    while (map->nodes[hm_index].hash) {
        hashnode_t *node = &map->nodes[hm_index];
        if (node->hash == hash) {
            node->index = index;
            return;
        }
        hm_index = (hm_index + 1) % vecCap(map->nodes);
    }

    map->nodes[hm_index].hash = hash;
    map->nodes[hm_index].index = index;
    _veclen(map->nodes)++;

    float load_factor = (float)vecLen(map->nodes) / (float)vecCap(map->nodes);
    if (load_factor > 0.75f) {
        uint32 old_cap = vecCap(map->nodes);
        vecReserve(map->nodes, old_cap);
        for (usize i = old_cap; i < vecCap(map->nodes); ++i) {
            map->nodes[i].hash = 0;
            map->nodes[i].index = 0;
        }
    }
}

uint64 hmGet(hashmap_t map, uint64 hash) {
    uint32 hm_index = hash % vecCap(map.nodes);

    do {
        hashnode_t *node = &map.nodes[hm_index];
        if (node->hash == hash) {
            return node->index;
        }
        hm_index = (hm_index + 1) % vecCap(map.nodes);
    } while (map.nodes[hm_index].hash);

    return 0;
}

void hmDelete(hashmap_t *map, uint64 hash) {
    uint32 hm_index = hash % vecCap(map->nodes);

    do {
        hashnode_t *node = &map->nodes[hm_index];
        if (node->hash == hash) {
            node->hash = 0;
            node->index = 0;
            break;
        }
        hm_index = (hm_index + 1) % vecCap(map->nodes);
    } while (map->nodes[hm_index].hash);

    if(vecLen(map->nodes)) _veclen(map->nodes)--;
}

void hashSetSeed(uint64 new_seed) {
    hash_seed = new_seed;
}

uint64 hash(const void *ptr, usize len) {
    const uint64 m = 0xC6A4A7935BD1E995LLU;
    const int r = 47;

    uint64 h = hash_seed ^ (len * m);

    const uint64 *data = (const uint64 *)ptr;
    const uint64 *end = (len >> 3) + data;

    while (data != end) {
        uint64 k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char * data2 = (const unsigned char *)data;

    switch(len & 7) {
    case 7: h ^= (uint64_t)(data2[6]) << 48;
    case 6: h ^= (uint64_t)(data2[5]) << 40;
    case 5: h ^= (uint64_t)(data2[4]) << 32;
    case 4: h ^= (uint64_t)(data2[3]) << 24;
    case 3: h ^= (uint64_t)(data2[2]) << 16;
    case 2: h ^= (uint64_t)(data2[1]) << 8;
    case 1: h ^= (uint64_t)(data2[0]);
            h *= m;
    };
    
    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

uint64 hashStr(str_t str) {
    return hash(str.buf, str.len);
}

uint64 hashView(strview_t view) {
    return hash(view.buf, view.len);
}

uint64 hashCStr(const char *cstr) {
    return hash(cstr, strlen(cstr));
}
