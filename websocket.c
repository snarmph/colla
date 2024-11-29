#include "websocket.h"

#include "arena.h"
#include "str.h"
#include "server.h"
#include "socket.h"
#include "base64.h"
#include "strstream.h"

#include "sha1.h"

#if !COLLA_MSVC
extern uint16_t ntohs(uint16_t hostshort);
extern uint16_t htons(uint16_t hostshort);
extern uint32_t htonl(uint32_t hostlong);

uint64_t ntohll(uint64_t input) {
    uint64_t rval = 0;
    uint8_t *data = (uint8_t *)&rval;

    data[0] = input >> 56;
    data[1] = input >> 48;
    data[2] = input >> 40;
    data[3] = input >> 32;
    data[4] = input >> 24;
    data[5] = input >> 16;
    data[6] = input >> 8;
    data[7] = input >> 0;

    return rval;
}

uint64_t htonll(uint64_t input) {
    return ntohll(input);
}

#endif

bool wsInitialiseSocket(arena_t scratch, socket_t websocket, strview_t key) {
    str_t full_key = strFmt(&scratch, "%v" WEBSOCKET_MAGIC, key, WEBSOCKET_MAGIC);
    
    sha1_t sha1_ctx = sha1_init();
    sha1_result_t sha1_data = sha1(&sha1_ctx, full_key.buf, full_key.len);

    // convert to big endian for network communication
    for (int i = 0; i < 5; ++i) {
        sha1_data.digest[i] = htonl(sha1_data.digest[i]);
    }

    buffer_t encoded_key = base64Encode(&scratch, (buffer_t){ (uint8 *)sha1_data.digest, sizeof(sha1_data.digest) });
    
    str_t response = strFmt(
        &scratch,
        
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Sec-WebSocket-Accept: %v\r\n"
        "\r\n",
        encoded_key
    );

    bool success = skSend(websocket, response.buf, response.len);
    return success;
}

buffer_t wsEncodeMessage(arena_t *arena, strview_t message) {
    int extra = 6;
    if (message.len > UINT16_MAX)     extra += sizeof(uint64);
    else if (message.len > UINT8_MAX) extra += sizeof(uint16);
    uint8 *bytes = alloc(arena, uint8, message.len + extra);
    bytes[0] = 0b10000001;
    bytes[1] = 0b10000000;
    int offset = 2;
    if (message.len > UINT16_MAX) {
        bytes[1] |= 0b01111111;
        uint64 len = htonll(message.len);
        memcpy(bytes + 2, &len, sizeof(len));
        offset += sizeof(uint64);
    }
    else if (message.len > UINT8_MAX) {
        bytes[1] |= 0b01111110;
        uint16 len = htons((uint16)message.len);
        memcpy(bytes + 2, &len, sizeof(len));
        offset += sizeof(uint16);
    }
    else {
        bytes[1] |= (uint8)message.len;
    }

    uint32 mask = 0;
    memcpy(bytes + offset, &mask, sizeof(mask));
    offset += sizeof(mask);
    memcpy(bytes + offset, message.buf, message.len);

    return (buffer_t){ bytes, message.len + extra };
}

str_t wsDecodeMessage(arena_t *arena, buffer_t message) {
    str_t out = STR_EMPTY;
    uint8 *bytes = message.data;

    bool mask  = bytes[1] & 0b10000000;
    int offset = 2;
    uint64 msglen = bytes[1] & 0b01111111;

    // 16bit msg len
    if (msglen == 126) {
        uint64 be_len = 0;
        memcpy(&be_len, bytes + 2, sizeof(be_len));
        msglen = ntohs(be_len);
        offset += sizeof(uint16);
    }
    // 64bit msg len
    else if (msglen == 127) {
        uint64 be_len = 0;
        memcpy(&be_len, bytes + 2, sizeof(be_len));
        msglen = ntohll(be_len);
        offset += sizeof(uint64);
    }

    if (msglen == 0) {
        warn("message length = 0");
    }
    else if (mask) {
        uint8 *decoded = alloc(arena, uint8, msglen + 1);
        uint8 masks[4] = {0};
        memcpy(masks, bytes + offset, sizeof(masks));
        offset += 4;

        for (uint64 i = 0; i < msglen; ++i) {
            decoded[i] = bytes[offset + i] ^ masks[i % 4];
        }

        out = (str_t){ (char *)decoded, msglen };
    }
    else {
        warn("mask bit not set!");
    }

    return out;
}
