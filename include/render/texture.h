#ifndef VASALL_TEXTURE_H
#define VASALL_TEXTURE_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <SDL3/SDL.h>

typedef struct Texture {
        GLuint id;
        s32 width;
        s32 height;
} Texture;



#endif /* VASALL_TEXTURE_H */