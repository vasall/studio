#include <core/hash.h>

/* FNV-1a 64-bit prime, 0x100000001b3 */
#define FNV_PRIME ((((u64)0x00000100UL) << 32) | (u64)0x000001b3UL)

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB u64 hash_bytes(const void *data, s32 len, u64 seed)
{
        const u8 *p = (const u8 *)data;
        u64 h = seed;
        s32 i;

        if(!p) return h;

        for(i = 0; i < len; i++) {
                h ^= p[i];
                h *= FNV_PRIME;
        }
        return h;
}

PUB u64 hash_str(const char *str, u64 seed)
{
        u64 h = seed;

        if(!str) return h;

        while(*str) {
                h ^= (u8)*str++;
                h *= FNV_PRIME;
        }
        return h;
}
