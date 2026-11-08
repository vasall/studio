#ifndef VASALL_MESH_H
#define VASALL_MESH_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/math/vector.h>

typedef struct Mesh {
        s32 number;
        Vector3 *vertices;
} Mesh;


/*
 * 
 */
PUB s32 mesh_init(Mesh *mesh, s32 num, Vector3 *v);

/*
 * 
 */
PUB void mesh_deinit(Mesh *mesh);

#endif /* VASALL_MESH_H */

