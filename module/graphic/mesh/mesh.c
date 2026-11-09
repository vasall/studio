#include <graphic/mesh/mesh.h>

#include <devkit/log/log.h>

#include <stdlib.h>
#include <string.h>

PUB s32 mesh_init(Mesh *mesh)
{
        if(list_init(&mesh->vertices, sizeof(Vertex), 128) < 0) {
                LOG(LOG_ERROR, "Failed to create vertex list for mesh");
                return -1;
        }

        if(list_init(&mesh->faces, sizeof(Face), 128) < 0) {
                LOG(LOG_ERROR, "Failed to create face list for mesh");
                list_deinit(&mesh->vertices);
                return -1;
        }

        return 0;
}

PUB void mesh_deinit(Mesh *mesh)
{
        list_deinit(&mesh->faces);
        list_deinit(&mesh->vertices);
}

PUB void mesh_clear(Mesh *mesh)
{
        list_clear(&mesh->vertices);
        list_clear(&mesh->faces);
}

PUB s32 mesh_vertex_count(Mesh *mesh)
{
        return list_count(&mesh->vertices);
}

PUB s32 mesh_face_count(Mesh *mesh)
{
        return list_count(&mesh->faces);
}

PUB s32 mesh_add_vertex(Mesh *mesh, Vertex vtx)
{
        /* The new vertex goes to the end, so its index is the old count */
        s32 index = list_count(&mesh->vertices);
        if(list_push(&mesh->vertices, 1, &vtx) < 0) {
                LOG(LOG_ERROR, "Failed to add vertex to mesh");
                return -1;
        }
        return index;
}

PUB s32 mesh_add_face(Mesh *mesh, Face fc)
{
        s32 index = list_count(&mesh->faces);
        if(list_push(&mesh->faces, 1, &fc) < 0) {
                LOG(LOG_ERROR, "Failed to add face to mesh");
                return -1;
        }
        return index;
}
