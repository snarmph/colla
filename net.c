#include "net.h" 
#include "arena.h" 

#if COLLA_WIN
#include "win/net_win32.c"
#else
#error "platform not supported"
#endif

const char *http_get_method_string(http_method_e method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_HEAD: return "HEAD";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE"; 
    }
    return "GET";
}

const char *http_get_status_string(int status) {
    switch (status) {
        case 200: return "OK";              
        case 201: return "CREATED";         
        case 202: return "ACCEPTED";        
        case 204: return "NO CONTENT";      
        case 205: return "RESET CONTENT";   
        case 206: return "PARTIAL CONTENT"; 

        case 300: return "MULTIPLE CHOICES";    
        case 301: return "MOVED PERMANENTLY";   
        case 302: return "MOVED TEMPORARILY";   
        case 304: return "NOT MODIFIED";        

        case 400: return "BAD REQUEST";             
        case 401: return "UNAUTHORIZED";            
        case 403: return "FORBIDDEN";               
        case 404: return "NOT FOUND";               
        case 407: return "RANGE NOT SATISFIABLE";   

        case 500: return "INTERNAL SERVER_ERROR";   
        case 501: return "NOT IMPLEMENTED";         
        case 502: return "BAD GATEWAY";             
        case 503: return "SERVICE NOT AVAILABLE";   
        case 504: return "GATEWAY TIMEOUT";         
        case 505: return "VERSION NOT SUPPORTED";   
    }
    
    return "UNKNOWN";
}

http_header_t *http__parse_headers_instream(arena_t *arena, instream_t *in) {
    http_header_t *head = NULL;

    while (!istr_is_finished(in)) {
        strview_t line = istr_get_line(in);

        usize pos = strv_find(line, ':', 0);
        if (pos != STR_NONE) {
            http_header_t *new_head = alloc(arena, http_header_t);

            new_head->key = strv_sub(line, 0, pos);
            new_head->value = strv_sub(line, pos + 2, SIZE_MAX);

            list_push(head, new_head);
        }
    }

    return head;
}

http_header_t *http_parse_headers(arena_t *arena, strview_t header_string) {
    instream_t in = istr_init(header_string);
    return http__parse_headers_instream(arena, &in);
}

http_req_t http_parse_req(arena_t *arena, strview_t request) {
    http_req_t req = {0};
    instream_t in = istr_init(request);

    strview_t method = strv_trim(istr_get_view(&in, '/'));
    istr_skip(&in, 1); // skip /
    req.url          = strv_trim(istr_get_view(&in, ' '));
    strview_t http   = strv_trim(istr_get_view(&in, '\n'));

    istr_skip(&in, 1); // skip \n

    req.headers = http__parse_headers_instream(arena, &in);

    req.body = strv_trim(istr_get_view_len(&in, SIZE_MAX));

    strview_t methods[5] = { strv("GET"), strv("POST"), strv("HEAD"), strv("PUT"), strv("DELETE") };
    usize methods_count = arrlen(methods);

    for (usize i = 0; i < methods_count; ++i) {
        if (strv_equals(method, methods[i])) {
            req.method = (http_method_e)i;
            break;
        }
    }

    in = istr_init(http);
    istr_ignore_and_skip(&in, '/'); // skip HTTP/
    istr_get_u8(&in, &req.version.major);
    istr_skip(&in, 1); // skip .
    istr_get_u8(&in, &req.version.minor);

    return req;
}

http_res_t http_parse_res(arena_t *arena, strview_t response) {
    http_res_t res = {0};
    instream_t in = istr_init(response);

    strview_t http = istr_get_view_len(&in, 5);
    if (!strv_equals(http, strv("HTTP"))) {
        err("response doesn't start with 'HTTP', instead with %v", http);
        return (http_res_t){0};
    }
    istr_skip(&in, 1); // skip /
    istr_get_u8(&in, &res.version.major);
    istr_skip(&in, 1); // skip .
    istr_get_u8(&in, &res.version.minor);
    istr_get_i32(&in, (i32*)&res.status_code);

    istr_ignore(&in, '\n');
    istr_skip(&in, 1); // skip \n

    res.headers = http__parse_headers_instream(arena, &in);

    strview_t encoding = http_get_header(res.headers, strv("transfer-encoding"));
    if (!strv_equals(encoding, strv("chunked"))) {
        res.body = istr_get_view_len(&in, SIZE_MAX);
    }
    else {
        err("chunked encoding not implemented yet! body ignored");
    }

    return res;
}

str_t http_req_to_str(arena_t *arena, http_req_t *req) {
    outstream_t out = ostr_init(arena);

    const char *method = NULL;
    switch (req->method) {
        case HTTP_GET:    method = "GET";       break;
        case HTTP_POST:   method = "POST";      break;
        case HTTP_HEAD:   method = "HEAD";      break;
        case HTTP_PUT:    method = "PUT";       break;
        case HTTP_DELETE: method = "DELETE";    break;
        default: err("unrecognised method: %d", method); return STR_EMPTY;
    }

    ostr_print(
        &out, 
        "%s /%v HTTP/%hhu.%hhu\r\n",
        method, req->url, req->version.major, req->version.minor
    );

    http_header_t *h = req->headers;
    while (h) {
        ostr_print(&out, "%v: %v\r\n", h->key, h->value);
        h = h->next;
    }

    ostr_puts(&out, strv("\r\n"));
    ostr_puts(&out, req->body);

    return ostr_to_str(&out);
}

str_t http_res_to_str(arena_t *arena, http_res_t *res) {
    outstream_t out = ostr_init(arena);

    ostr_print(
        &out,
        "HTTP/%hhu.%hhu %d %s\r\n",
        res->version.major, 
        res->version.minor,
        res->status_code, 
        http_get_status_string(res->status_code)
    );
    ostr_puts(&out, strv("\r\n"));
    ostr_puts(&out, res->body);

    return ostr_to_str(&out);
}

bool http_has_header(http_header_t *headers, strview_t key) {
    for_each(h, headers) {
        if (strv_equals(h->key, key)) {
            return true;
        }
    }
    return false;
}

void http_set_header(http_header_t *headers, strview_t key, strview_t value) {
    http_header_t *h = headers;
    while (h) {
        if (strv_equals(h->key, key)) {
            h->value = value;
            break;
        }
        h = h->next;
    }
}

strview_t http_get_header(http_header_t *headers, strview_t key) {
    http_header_t *h = headers;
    while (h) {
        if (strv_equals(h->key, key)) {
            return h->value;
        }
        h = h->next;
    }
    return STRV_EMPTY;
}

str_t http_make_url_safe(arena_t *arena, strview_t string) {
    strview_t chars = strv(" !\"#$%%&'()*+,/:;=?@[]");
    usize final_len = string.len;

    // find final string length first
    for (usize i = 0; i < string.len; ++i) {
        if (strv_contains(chars, string.buf[i])) {
            final_len += 2;
        }
    }
    
    str_t out = {
        .buf = alloc(arena, char, final_len + 1),
        .len = final_len
    };
    usize cur = 0;
    // substitute characters
    for (usize i = 0; i < string.len; ++i) {
        if (strv_contains(chars, string.buf[i])) {
            fmt_buffer(out.buf + cur, 4, "%%%X", string.buf[i]);
            cur += 3;
        }
        else {
            out.buf[cur++] = string.buf[i];
        }
    }

    return out;
}

str_t http_decode_url_safe(arena_t *arena, strview_t string) {
    usize final_len = string.len;

    for (usize i = 0; i < string.len; ++i) {
        if (string.buf[i] == '%') {
            final_len -= 2;
            i += 2;
        }
    }

    assert(final_len <= string.len);

    str_t out = {
        .buf = alloc(arena, char, final_len + 1),
        .len = final_len
    };

    usize k = 0;

    for (usize i = 0; i < string.len; ++i) {
        if (string.buf[i] == '%') {
            // skip %
            ++i;

            unsigned int ch = 0;
            int result = sscanf(string.buf + i, "%02X", &ch);
            if (result != 1 || ch > UINT8_MAX) {
                err("malformed url at %zu (%s)", i, string.buf + i);
                return STR_EMPTY;
            }
            out.buf[k++] = (char)ch;
            
            // skip first char of hex
            ++i;
        }
        else {
            out.buf[k++] = string.buf[i];
        }
    }

    return out;
}

http_url_t http_split_url(strview_t url) {
    http_url_t out = {0};

    if (strv_starts_with_view(url, strv("https://"))) {
        url = strv_remove_prefix(url, 8);
    }
    else if (strv_starts_with_view(url, strv("http://"))) {
        url = strv_remove_prefix(url, 7);
    }

    out.host = strv_sub(url, 0, strv_find(url, '/', 0));
    out.uri = strv_sub(url, out.host.len, SIZE_MAX);

    return out;
}

// WEBSOCKETS ///////////////////////

#define WEBSOCKET_MAGIC    "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WEBSOCKET_HTTP_KEY "Sec-WebSocket-Key"

bool websocket_init(arena_t scratch, socket_t socket, strview_t key) {
    str_t full_key = str_fmt(&scratch, "%v" WEBSOCKET_MAGIC, key);
    
    sha1_t sha1_ctx = sha1_init();
    sha1_result_t sha1_data = sha1(&sha1_ctx, full_key.buf, full_key.len);

    // convert to big endian for network communication
    for (int i = 0; i < 5; ++i) {
        sha1_data.digest[i] = htonl(sha1_data.digest[i]);
    }

    buffer_t encoded_key = base64_encode(&scratch, (buffer_t){ (u8 *)sha1_data.digest, sizeof(sha1_data.digest) });
    
    str_t response = str_fmt(
        &scratch,
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Sec-WebSocket-Accept: %v\r\n"
        "\r\n",
        encoded_key
    );

    int result = sk_send(socket, response.buf, (int)response.len);
    return result != SOCKET_ERROR;
}

buffer_t websocket_encode(arena_t *arena, strview_t message) {
    int extra = 6;
    if (message.len > UINT16_MAX)     extra += sizeof(u64);
    else if (message.len > UINT8_MAX) extra += sizeof(u16);
    u8 *bytes = alloc(arena, u8, message.len + extra);
    bytes[0] = 0b10000001;
    bytes[1] = 0b10000000;
    int offset = 2;
    if (message.len > UINT16_MAX) {
        bytes[1] |= 0b01111111;
        u64 len = htonll(message.len);
        memmove(bytes + 2, &len, sizeof(len));
        offset += sizeof(u64);
    }
    else if (message.len > UINT8_MAX) {
        bytes[1] |= 0b01111110;
        u16 len = htons((u16)message.len);
        memmove(bytes + 2, &len, sizeof(len));
        offset += sizeof(u16);
    }
    else {
        bytes[1] |= (u8)message.len;
    }

    u32 mask = 0;
    memmove(bytes + offset, &mask, sizeof(mask));
    offset += sizeof(mask);
    memmove(bytes + offset, message.buf, message.len);

    return (buffer_t){ bytes, message.len + extra };
}

str_t websocket_decode(arena_t *arena, buffer_t message) {
    str_t out = STR_EMPTY;
    u8 *bytes = message.data;

    bool mask  = bytes[1] & 0b10000000;
    int offset = 2;
    u64 msglen = bytes[1] & 0b01111111;

    // 16bit msg len
    if (msglen == 126) {
        u16 be_len = 0;
        memmove(&be_len, bytes + 2, sizeof(be_len));
        msglen = ntohs(be_len);
        offset += sizeof(u16);
    }
    // 64bit msg len
    else if (msglen == 127) {
        u64 be_len = 0;
        memmove(&be_len, bytes + 2, sizeof(be_len));
        msglen = ntohll(be_len);
        offset += sizeof(u64);
    }

    if (msglen == 0) {
        warn("message length = 0");
    }
    else if (mask) {
        u8 *decoded = alloc(arena, u8, msglen + 1);
        u8 masks[4] = {0};
        memmove(masks, bytes + offset, sizeof(masks));
        offset += 4;

        for (u64 i = 0; i < msglen; ++i) {
            decoded[i] = bytes[offset + i] ^ masks[i % 4];
        }

        out = (str_t){ (char *)decoded, msglen };
    }
    else {
        warn("mask bit not set!");
    }

    return out;
}


// SHA 1 ////////////////////////////

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

u32 sha1__left_rotate(u32 value, u32 count) {
    return (value << count) ^ (value >> (32 - count));
}

void sha1__process_block(sha1_t *ctx) {
    u32 w [80];
    for (usize i = 0; i < 16; ++i) {
        w[i]  = ctx->block[i * 4 + 0] << 24;
        w[i] |= ctx->block[i * 4 + 1] << 16;
        w[i] |= ctx->block[i * 4 + 2] << 8;
        w[i] |= ctx->block[i * 4 + 3] << 0;
    }

    for (usize i = 16; i < 80; ++i) {
        w[i] = sha1__left_rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    u32 a = ctx->digest[0];
    u32 b = ctx->digest[1];
    u32 c = ctx->digest[2];
    u32 d = ctx->digest[3];
    u32 e = ctx->digest[4];

    for (usize i = 0; i < 80; ++i) {
        u32 f = 0;
        u32 k = 0;

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
        u32 temp = sha1__left_rotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = sha1__left_rotate(b, 30);
        b = a;
        a = temp;
    }
	
    ctx->digest[0] += a;
    ctx->digest[1] += b;
    ctx->digest[2] += c;
    ctx->digest[3] += d;
    ctx->digest[4] += e;
}

void sha1__process_byte(sha1_t *ctx, u8 b) {
    ctx->block[ctx->block_index++] = b;
    ++ctx->byte_count;
    if (ctx->block_index == 64) {
        ctx->block_index = 0;
        sha1__process_block(ctx);
    }
}

sha1_result_t sha1(sha1_t *ctx, const void *buf, usize len) {
    const u8 *block = buf;

    for (usize i = 0; i < len; ++i) {
        sha1__process_byte(ctx, block[i]);
    }

    usize bitcount = ctx->byte_count * 8;
    sha1__process_byte(ctx, 0x80);
    
    if (ctx->block_index > 56) {
        while (ctx->block_index != 0) {
            sha1__process_byte(ctx, 0);
        }
        while (ctx->block_index < 56) {
            sha1__process_byte(ctx, 0);
        }
    } else {
        while (ctx->block_index < 56) {
            sha1__process_byte(ctx, 0);
        }
    }
    sha1__process_byte(ctx, 0);
    sha1__process_byte(ctx, 0);
    sha1__process_byte(ctx, 0);
    sha1__process_byte(ctx, 0);
    sha1__process_byte(ctx, (uchar)((bitcount >> 24) & 0xFF));
    sha1__process_byte(ctx, (uchar)((bitcount >> 16) & 0xFF));
    sha1__process_byte(ctx, (uchar)((bitcount >> 8 ) & 0xFF));
    sha1__process_byte(ctx, (uchar)((bitcount >> 0 ) & 0xFF));

    sha1_result_t result = {0};
    memcpy(result.digest, ctx->digest, sizeof(result.digest));
    return result;
}

str_t sha1Str(arena_t *arena, sha1_t *ctx, const void *buf, usize len) {
    sha1_result_t result = sha1(ctx, buf, len);
    return str_fmt(arena, "%08x%08x%08x%08x%08x", result.digest[0], result.digest[1], result.digest[2], result.digest[3], result.digest[4]);
}

// BASE 64 //////////////////////////

unsigned char b64__encoding_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',           
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

u8 b64__decoding_table[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63, 
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 
    0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 
    24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 
    44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

buffer_t base64_encode(arena_t *arena, buffer_t buffer) {
    usize outlen = ((buffer.len + 2) / 3) * 4;
    u8 *out = alloc(arena, u8, outlen);

    for (usize i = 0, j = 0; i < buffer.len;) {
        u32 a = i < buffer.len ? buffer.data[i++] : 0;
        u32 b = i < buffer.len ? buffer.data[i++] : 0;
        u32 c = i < buffer.len ? buffer.data[i++] : 0;

        u32 triple = (a << 16) | (b << 8) | c;

        out[j++] = b64__encoding_table[(triple >> 18) & 0x3F];
        out[j++] = b64__encoding_table[(triple >> 12) & 0x3F];
        out[j++] = b64__encoding_table[(triple >>  6) & 0x3F];
        out[j++] = b64__encoding_table[(triple >>  0) & 0x3F];
    }

    usize mod = buffer.len % 3;
    if (mod) {
        mod = 3 - mod;
        for (usize i = 0; i < mod; ++i) {
            out[outlen - 1 - i] = '=';
        }
    }

    return (buffer_t){
        .data = out,
        .len = outlen
    };
}

buffer_t base64_decode(arena_t *arena, buffer_t buffer) {
    u8 *out = arena->cur;
    usize start = arena_tell(arena);

    for (usize i = 0; i < buffer.len; i += 4) {
        u8 a = b64__decoding_table[buffer.data[i + 0]];
        u8 b = b64__decoding_table[buffer.data[i + 1]];
        u8 c = b64__decoding_table[buffer.data[i + 2]];
        u8 d = b64__decoding_table[buffer.data[i + 3]];

        u32 triple =
            ((u32)a << 18) |
            ((u32)b << 12) |
            ((u32)c << 6)  |
            ((u32)d);

        u8 *bytes = alloc(arena, u8, 3);

        bytes[0] = (triple >> 16) & 0xFF;
        bytes[1] = (triple >>  8) & 0xFF;
        bytes[2] = (triple >>  0) & 0xFF;
    }

    usize outlen = arena_tell(arena) - start;

    return (buffer_t){
        .data = out,
        .len = outlen,
    };
}