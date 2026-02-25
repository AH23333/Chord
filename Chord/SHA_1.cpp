#include "SHA_1.h"
#include "config.h"

void sha1_init(SHA1_CTX *context)
{
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count = 0;
}

void sha1_transform(uint32_t state[5], const uint8_t buffer[SHA1_BLOCK_SIZE])
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
    {
        w[i] = (uint32_t)buffer[i * 4] << 24 | (uint32_t)buffer[i * 4 + 1] << 16 |
               (uint32_t)buffer[i * 4 + 2] << 8 | (uint32_t)buffer[i * 4 + 3];
    }
    for (int i = 16; i < 80; i++)
    {
        w[i] = SHA1_ROL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }
    for (int i = 0; i < 80; i++)
    {
        uint32_t f, k;
        if (i < 20)
        {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        }
        else if (i < 40)
        {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        }
        else if (i < 60)
        {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        }
        else
        {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t temp = SHA1_ROL(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = SHA1_ROL(b, 30);
        b = a;
        a = temp;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void sha1_update(SHA1_CTX *context, const uint8_t *data, size_t len)
{
    uint32_t i = 0, j = (uint32_t)(context->count & 0x3F);
    context->count += len;
    for (; i < len; i++)
    {
        context->buffer[j++] = data[i];
        if (j == SHA1_BLOCK_SIZE)
        {
            sha1_transform(context->state, context->buffer);
            j = 0;
        }
    }
}

void sha1_final(uint8_t digest[20], SHA1_CTX *context)
{
    uint64_t bit_count = context->count * 8;
    uint8_t pad0x80 = 0x80;
    uint8_t pad0x00 = 0x00;
    uint8_t length[8];
    for (int i = 0; i < 8; i++)
    {
        length[i] = (uint8_t)((bit_count >> (56 - 8 * i)) & 0xFF);
    }
    sha1_update(context, &pad0x80, 1);
    while ((context->count & 0x3F) != 56)
    {
        sha1_update(context, &pad0x00, 1);
    }
    sha1_update(context, length, 8);
    for (int i = 0; i < 5; i++)
    {
        digest[i * 4] = (uint8_t)((context->state[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = (uint8_t)((context->state[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = (uint8_t)((context->state[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = (uint8_t)(context->state[i] & 0xFF);
    }
}

uint32_t sha1_hash_to_uint32(const std::string &input)
{
    SHA1_CTX ctx;
    uint8_t digest[20];
    sha1_init(&ctx);
    sha1_update(&ctx, (const uint8_t *)input.c_str(), input.length());
    sha1_final(digest, &ctx);
    uint32_t hash = (uint32_t)digest[0] << 24 | (uint32_t)digest[1] << 16 |
                    (uint32_t)digest[2] << 8 | (uint32_t)digest[3];
    return hash % (1 << m); // 限制在m位范围内
}