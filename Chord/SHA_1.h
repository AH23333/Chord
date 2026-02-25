#ifndef SHA_1_H
#define SHA_1_H

#include <string>
#include <cstdint>
#include <cstddef>

constexpr uint32_t SHA1_ROL(uint32_t value, uint32_t bits)
{
    return ((value << bits) | (value >> (32 - bits)));
}
constexpr uint32_t SHA1_BLOCK_SIZE = 64;

typedef struct
{
    uint32_t state[5];
    uint64_t count;
    uint8_t buffer[SHA1_BLOCK_SIZE];
} SHA1_CTX;

void sha1_init(SHA1_CTX *context);
void sha1_transform(uint32_t state[5], const uint8_t buffer[SHA1_BLOCK_SIZE]);
void sha1_update(SHA1_CTX *context, const uint8_t *data, size_t len);
void sha1_final(uint8_t digest[20], SHA1_CTX *context);
uint32_t sha1_hash_to_uint32(const std::string &input);

#endif