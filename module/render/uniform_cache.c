#include <render/uniform_cache.h>

#include <devkit/log/log.h>

#include <string.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 uniform_cache_init(UniformCache *cache, GLuint program)
{
        if(!cache) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(list_init(&cache->entries, sizeof(UniformEntry), 8) < 0) {
                LOG(LOG_ERROR, "Failed to create the uniform list");
                return -1;
        }

        cache->program = program;

        return 0;
}

PUB void uniform_cache_deinit(UniformCache *cache)
{
        if(!cache) {
                return;
        }

        list_deinit(&cache->entries);
        cache->program = 0;
}

PUB GLint uniform_cache_get(UniformCache *cache, const char *name)
{
        UniformEntry entry;
        s32 i;

        if(!cache || !name) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        /* A shader has few uniforms, so a linear search is fast enough */
        for(i = 0; i < list_count(&cache->entries); i++) {
                UniformEntry *e = list_get(&cache->entries, i);
                if(strcmp(e->name, name) == 0) {
                        return e->location;
                }
        }

        if(strlen(name) >= UNIFORM_NAME_LENGTH) {
                LOG_S(LOG_ERROR, "Uniform name too long: %s", (char *)name);
                return -1;
        }

        strcpy(entry.name, name);
        entry.location = glGetUniformLocation(cache->program, name);

        /* Also unknown names are stored, so the warning only shows once */
        if(entry.location < 0) {
                LOG_S(LOG_WARN, "Shader has no uniform: %s", (char *)name);
        }

        if(list_push(&cache->entries, 1, &entry) < 0) {
                LOG(LOG_ERROR, "Failed to cache uniform location");
        }

        return entry.location;
}
