#include "sha1.h"
#include "log.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

struct sha1_ctx {
    uint32_t state[5];

    // number of bits processed
    uint32_t count[2];

    // for processing data in 512-bit blocks
    uint8_t buffer[64];
};

#define LEFT_ROTATE(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void sha1_transform(uint32_t state[5], const uint8_t block[64]) {
    uint32_t w[80];

    // Break chunk into sixteen 32-bit words
    for (int i = 0; i < 16; i++) {
        w[i] = (block[i * 4] << 24)
            | (block[i * 4 + 1] << 16)
            | (block[i * 4 + 2] << 8)
            | (block[i * 4 + 3]);
    }

    // Extend the sixteen 32-bit words into eighty 32-bit words
    for (int i = 16; i < 32; i++) {
        w[i] = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
        w[i] = LEFT_ROTATE(w[i], 1);
    }

    for (int i = 32; i < 80; i++) {
        w[i] = w[i - 6] ^ w[i - 16] ^ w[i - 28] ^ w[i - 32];
        w[i] = LEFT_ROTATE(w[i], 2);
    }

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];

    // Main loop
    uint32_t temp;
    for (int i = 0; i < 80; i++) {
        if (i < 20) {
            temp = ((b & c) | ((~b) & d)) + 0x5A827999;
        } else if (i < 40) {
            temp = (b ^ c ^ d) + 0x6ED9EBA1;
        } else if (i < 60) {
            temp = ((b & c) | (b & d) | (c & d)) + 0x8F1BBCDC;
        } else {
            temp = (b ^ c ^ d) + 0xCA62C1D6;
        }
        temp += LEFT_ROTATE(a, 5) + e + w[i];
        e = d;
        d = c;
        c = LEFT_ROTATE(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

sha1_ctx_t *sha1_create(void) {
    sha1_ctx_t *ctx = malloc(sizeof(sha1_ctx_t));
    if (ctx == NULL) {
        LOG_ERROR("Failed to allocate memory for SHA-1 context");
        return NULL;
    }

    // see https://en.wikipedia.org/wiki/SHA-1#SHA-1_pseudocode
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;

    ctx->count[0] = 0;
    ctx->count[1] = 0;

    return ctx;
}

void sha1_free(sha1_ctx_t *ctx) {
    free(ctx);
}

void sha1_update(sha1_ctx_t *ctx, const uint8_t *data, size_t size) {
    if (ctx == NULL || data == NULL) {
        LOG_WARN("Invalid SHA-1 context or data");
        return;
    }

    uint32_t i = 0;
    uint32_t j = ctx->count[0];

    // update the number of bits processed and check for overflow
    if ((ctx->count[0] += size << 3) < j) {
        ctx->count[1]++;
    }

    // ensures the total number of bits processed is correctly stored
    // even if it surpasses 2^32
    ctx->count[1] += size >> 29;

    // byte offset into the buffer
    j = (j >> 3) & 63;

    // if the buffer would overflow, process it
    if (j + size > 63) {
        // copy the remaining data to the buffer
        memcpy(&ctx->buffer[j], data, 64 - j);

        // process the buffer
        sha1_transform(ctx->state, ctx->buffer);

        // process the remaining 512-bit blocks of data
        for (i = 64 - j; i + 63 < size; i += 64) {
            sha1_transform(ctx->state, &data[i]);
        }

        j = 0;
    }

    // copy the remaining data to the buffer
    memcpy(&ctx->buffer[j], &data[i], size - i);
}

void sha1_final(sha1_ctx_t *ctx, uint8_t digest[SHA1_DIGEST_SIZE]) {
    if (ctx == NULL || digest == NULL) {
        LOG_WARN("Invalid SHA-1 context or digest");
        return;
    }

    uint8_t bits_processed[8];
    for (int i = 0; i < 8; i++) {
        // i > 3 ? 0 : 1 chooses the correct count
        //
        // ((3 - (i & 3)) << 3) gets the correct byte in a 32-bit integer
        // i & 3 chooses the byte (3 -> 0b11) so the only values are 0, 1, 2, 3
        // 3 - (i & 3) reverses the order so the bytes are in the correct order
        // 
        // & 0xFF masks the byte so it is only 8 bits
        bits_processed[i] =
            (uint8_t)((ctx->count[(i > 3 ? 0 : 1)] >> ((3 - (i & 3)) << 3)) & 0xFF); // Endian independent
    }

    uint8_t byte = 0x80;
    sha1_update(ctx, &byte, 1);

    // pad the data with zeros until the length is congruent to 448 mod 512 (-64 mod 512)
    // after padding, the message should be 512 bits - 64 bits (for the length)
    while ((ctx->count[0] & 511) != 448) {
        byte = 0x00;
        sha1_update(ctx, &byte, 1);
    }

    // append the length of the data in bits as a 64-bit big-endian integer
    sha1_update(ctx, bits_processed, 8);

    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        // i >> 2 gets the correct 32-bit integer (increments by 4)
        //
        // (3 - (i & 3)) << 3 gets the correct byte in a 32-bit integer
        // i & 3 chooses the byte (3 -> 0b11) so the only values are 0, 1, 2, 3
        // 3 - (i & 3) reverses the order so the bytes are in the correct order
        //
        // & 0xFF masks the byte so it is only 8 bits
        digest[i] =
            (uint8_t)((ctx->state[i >> 2] >> ((3 - (i & 3)) << 3)) & 0xFF);
    }
}

void sha1(const uint8_t *data, size_t size, uint8_t digest[SHA1_DIGEST_SIZE]) {
    sha1_ctx_t *ctx = sha1_create();
    if (ctx == NULL) {
        return;
    }

    sha1_update(ctx, data, size);
    sha1_final(ctx, digest);

    sha1_free(ctx);
}
