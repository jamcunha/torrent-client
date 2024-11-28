#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>
#include <stdlib.h>

#define SHA1_DIGEST_SIZE 20

typedef struct sha1_ctx sha1_ctx_t;

/**
 * @brief Create a new SHA1 context
 *
 * @return sha_ctx_t* The SHA1 context
 */
sha1_ctx_t* sha1_create(void);

/**
 * @brief Free the SHA1 context
 *
 * @param ctx The SHA1 context
 */
void sha1_free(sha1_ctx_t* ctx);

/**
 * @brief Update the SHA1 context with data
 *
 * This function breaks the data into 512-bit blocks and processes each block
 *
 * @param ctx The SHA1 context
 * @param data The data
 * @param size The size of the data
 */
void sha1_update(sha1_ctx_t* ctx, const uint8_t* data, size_t size);

/**
 * @brief Finalize the SHA1 context and get the digest
 *
 * This function appends a 1 bit to the data, then pads the data with zeros
 * until the length is congruent to 448 mod 512. Finally, it appends the
 * length of the data in bits as a 64-bit big-endian integer.
 *
 * @param ctx The SHA1 context
 * @param digest The digest
 */
void sha1_final(sha1_ctx_t* ctx, uint8_t digest[SHA1_DIGEST_SIZE]);

/**
 * @brief Compute the SHA1 digest of the data
 *
 * This function creates a new SHA1 context, updates it with the data, finalizes
 * it and returns the digest.
 *
 * @param data The data
 * @param size The size of the data
 * @param digest The digest
 */
void sha1(const uint8_t* data, size_t size, uint8_t digest[SHA1_DIGEST_SIZE]);

#endif // !SHA1_H
