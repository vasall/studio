#include <core/arena.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_ALIGN 16

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC ArenaBlock *s_block_new(s64 size);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 arena_init(Arena *arena, s64 block_size)
{
        if(!arena || block_size <= 0) return -1;

        arena->first = NULL;
        arena->current = NULL;
        arena->block_size = block_size;
        return 0;
}

PUB void arena_deinit(Arena *arena)
{
        ArenaBlock *blk;
        ArenaBlock *next;

        if(!arena) return;

        for(blk = arena->first; blk; blk = next) {
                next = blk->next;
                free(blk);
        }
        arena->first = NULL;
        arena->current = NULL;
}

PUB void arena_reset(Arena *arena)
{
        ArenaBlock *blk;

        if(!arena) return;

        for(blk = arena->first; blk; blk = blk->next)
                blk->used = 0;
        arena->current = arena->first;
}

PUB void *arena_alloc(Arena *arena, s64 size)
{
        ArenaBlock *blk;
        u8 *mem;

        if(!arena || size < 0) return NULL;

        size = (size + ARENA_ALIGN - 1) & ~((s64)ARENA_ALIGN - 1);

        /* Find a block with enough room, starting at the current one */
        blk = arena->current;
        while(blk && blk->size - blk->used < size)
                blk = blk->next;

        if(!blk) {
                blk = s_block_new(size > arena->block_size ?
                                size : arena->block_size);
                if(!blk) return NULL;

                /* Append at the end of the chain */
                if(!arena->first) {
                        arena->first = blk;
                } else {
                        ArenaBlock *last = arena->current ? arena->current :
                                arena->first;
                        while(last->next) last = last->next;
                        last->next = blk;
                }
        }
        arena->current = blk;

        mem = (u8 *)(blk + 1) + blk->used;
        blk->used += size;
        memset(mem, 0, (size_t)size);
        return mem;
}

PUB char *arena_strdup(Arena *arena, const char *str, s32 len)
{
        char *out;

        if(!str) return NULL;
        if(len < 0) len = (s32)strlen(str);

        out = arena_alloc(arena, len + 1);
        if(!out) return NULL;
        memcpy(out, str, (size_t)len);
        out[len] = '\0';
        return out;
}

PUB char *arena_printf(Arena *arena, const char *fmt, ...)
{
        va_list args;
        s32 len;
        char *out;

        va_start(args, fmt);
        len = vsnprintf(NULL, 0, fmt, args);
        va_end(args);
        if(len < 0) return NULL;

        out = arena_alloc(arena, len + 1);
        if(!out) return NULL;

        va_start(args, fmt);
        vsnprintf(out, (size_t)len + 1, fmt, args);
        va_end(args);
        return out;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC ArenaBlock *s_block_new(s64 size)
{
        ArenaBlock *blk;

        blk = malloc(sizeof(ArenaBlock) + (size_t)size);
        if(!blk) return NULL;

        blk->next = NULL;
        blk->size = size;
        blk->used = 0;
        return blk;
}
