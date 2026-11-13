#ifndef VASALL_STUDIO_ARENA_H
#define VASALL_STUDIO_ARENA_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct ArenaBlock {
        struct ArenaBlock *next;
        s64 size;               /* Usable bytes after the header */
        s64 used;
        s64 pad;                /* Keeps the data after the header 16-aligned */
} ArenaBlock;

/**
 * Linear allocator. Allocations are freed all at once with arena_reset, which
 * keeps the blocks for reuse, so a per-frame arena stops calling malloc once
 * it has grown to the size of a frame.
 */
typedef struct Arena {
        ArenaBlock *first;
        ArenaBlock *current;
        s64 block_size;         /* Size of new blocks */
} Arena;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Initialize an arena. No memory is allocated until the first allocation.
 *
 * @returns 0 on success, -1 on error
 */
PUB s32 arena_init(Arena *arena, s64 block_size);

/**
 * Free all blocks.
 */
PUB void arena_deinit(Arena *arena);

/**
 * Free all allocations, keep the blocks.
 */
PUB void arena_reset(Arena *arena);

/**
 * Allocate zeroed memory, aligned to 16 bytes.
 *
 * @returns The memory, or NULL on error
 */
PUB void *arena_alloc(Arena *arena, s64 size);

/**
 * Copy len bytes of str (or all of it if len < 0) and add a '\0'.
 */
PUB char *arena_strdup(Arena *arena, const char *str, s32 len);

/**
 * Format a string into the arena, like snprintf.
 */
PUB char *arena_printf(Arena *arena, const char *fmt, ...);

#endif /* VASALL_STUDIO_ARENA_H */
