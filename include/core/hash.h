#ifndef VASALL_STUDIO_HASH_H
#define VASALL_STUDIO_HASH_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

/**
 * Start value of a fresh hash (the FNV-1a 64-bit offset basis). Built from
 * two halves, since C89 has no 64-bit integer constants.
 */
#define HASH_SEED ((((u64)0xcbf29ce4UL) << 32) | (u64)0x84222325UL)

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * FNV-1a 64-bit hash of len bytes. Pass HASH_SEED for a fresh hash, or a
 * previous result to continue it, so hashes can be chained, e.g. an id
 * hashed with its parent's hash.
 */
PUB u64 hash_bytes(const void *data, s32 len, u64 seed);

/**
 * Hash a '\0'-terminated string, like hash_bytes. NULL returns seed.
 */
PUB u64 hash_str(const char *str, u64 seed);

#endif /* VASALL_STUDIO_HASH_H */
