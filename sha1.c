#include "sha1.h"

sha1_t sha1_init(void) {
    return (sha1_t) {
        .digest = {
			0x67452301,
			0xEFCDAB89,
			0x98BADCFE,
			0x10325476,
			0xC3D2E1F0,
        },
    };
}

uint32 sha1_left_rotate(uint32 value, uint32 count) {
    return (value << count) ^ (value >> (32 - count));
}

void sha1_process_block(sha1_t *ctx) {
    uint32 w [80];
    for (usize i = 0; i < 16; ++i) {
        w[i]  = ctx->block[i * 4 + 0] << 24;
        w[i] |= ctx->block[i * 4 + 1] << 16;
        w[i] |= ctx->block[i * 4 + 2] << 8;
        w[i] |= ctx->block[i * 4 + 3] << 0;
    }

    for (usize i = 16; i < 80; ++i) {
        w[i] = sha1_left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    uint32 a = ctx->digest[0];
    uint32 b = ctx->digest[1];
    uint32 c = ctx->digest[2];
    uint32 d = ctx->digest[3];
    uint32 e = ctx->digest[4];

    for (usize i = 0; i < 80; ++i) {
        uint32 f = 0;
        uint32 k = 0;

        if (i<20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i<40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i<60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32 temp = sha1_left_rotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = sha1_left_rotate(b, 30);
        b = a;
        a = temp;
    }
	
    ctx->digest[0] += a;
    ctx->digest[1] += b;
    ctx->digest[2] += c;
    ctx->digest[3] += d;
    ctx->digest[4] += e;
}

void sha1_process_byte(sha1_t *ctx, uint8 b) {
    ctx->block[ctx->block_index++] = b;
    ++ctx->byte_count;
    if (ctx->block_index == 64) {
        ctx->block_index = 0;
        sha1_process_block(ctx);
    }
}

sha1_result_t sha1(sha1_t *ctx, const void *buf, usize len) {
    const uint8 *block = buf;

    for (usize i = 0; i < len; ++i) {
        sha1_process_byte(ctx, block[i]);
    }

    usize bitcount = ctx->byte_count * 8;
    sha1_process_byte(ctx, 0x80);
    
    if (ctx->block_index > 56) {
        while (ctx->block_index != 0) {
            sha1_process_byte(ctx, 0);
        }
        while (ctx->block_index < 56) {
            sha1_process_byte(ctx, 0);
        }
    } else {
        while (ctx->block_index < 56) {
            sha1_process_byte(ctx, 0);
        }
    }
    sha1_process_byte(ctx, 0);
    sha1_process_byte(ctx, 0);
    sha1_process_byte(ctx, 0);
    sha1_process_byte(ctx, 0);
    sha1_process_byte(ctx, (uchar)((bitcount >> 24) & 0xFF));
    sha1_process_byte(ctx, (uchar)((bitcount >> 16) & 0xFF));
    sha1_process_byte(ctx, (uchar)((bitcount >> 8 ) & 0xFF));
    sha1_process_byte(ctx, (uchar)((bitcount >> 0 ) & 0xFF));

    // memcpy(digest, m_digest, 5 * sizeof(uint32_t));#

    sha1_result_t result = {0};
    memcpy(result.digest, ctx->digest, sizeof(result.digest));
    return result;
}

str_t sha1Str(arena_t *arena, sha1_t *ctx, const void *buf, usize len) {
    sha1_result_t result = sha1(ctx, buf, len);
    return strFmt(arena, "%08x%08x%08x%08x%08x", result.digest[0], result.digest[1], result.digest[2], result.digest[3], result.digest[4]);
}