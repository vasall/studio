#ifndef VASALL_MESH_H
#define VASALL_MESH_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/list.h>

#include <graphic/mesh/vertex.h>
#include <graphic/mesh/face.h>


typedef struct Mesh {
        List vertices;
        List faces;
} Mesh;


/*
 *
 */
PUB s32 mesh_init(Mesh *mesh);

/*
 *
 */
PUB void mesh_deinit(Mesh *mesh);

/*
 *
*/
PUB void mesh_clear(Mesh *mesh);

/*
 *
*/
PUB s32 mesh_vertex_count(Mesh *mesh);

/*
 *
*/
PUB s32 mesh_face_count(Mesh *mesh);

/*
 * Add a new vertex to the mesh.
 * This function either returns the index of the vertex or -1 if an error
 * occured.
*/
PUB s32 mesh_add_vertex(Mesh *mesh, Vertex vtx);

/*
 *
*/
PUB s32 mesh_add_face(Mesh *mesh, Face fc);

#endif /* VASALL_MESH_H */
