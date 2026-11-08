#include <graphic/mesh.h>

#include <devkit/log/log.h>

#include <stdlib.h>
#include <string.h>

PUB s32 mesh_init(Mesh *mesh, s32 num, Vector3 *v)
{
        s32 size;

        size = num * sizeof(Vector3);
        if(!(mesh->vertices = malloc(size))) {
                LOG(LOG_ERROR, "Failed to allocate memory for vertices");
                return -1;
        }

        memcpy(mesh->vertices, v, size);
        mesh->number = num;

        return 0;
}

PUB void mesh_deinit(Mesh *mesh)
{
        free(mesh->vertices);
}
