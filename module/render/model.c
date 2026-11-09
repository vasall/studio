#include <render/model.h>

#include <devkit/log/log.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Check that every face only uses vertices that exist. GL doesn't check
 * this, and indices out of range can crash the driver.
 */
LOC s32 m_check_faces(Mesh *mesh);

/*
 * Upload the vertices and indices into the bound buffers.
 */
LOC void m_upload(Model *mdl, Mesh *mesh);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 model_init(Model *mdl, Mesh *mesh)
{
        if(!mdl || !mesh) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(m_check_faces(mesh) < 0) {
                return -1;
        }

        glGenVertexArrays(1, &mdl->vao);
        glGenBuffers(1, &mdl->vbo);
        glGenBuffers(1, &mdl->ibo);

        if(!mdl->vao || !mdl->vbo || !mdl->ibo) {
                LOG(LOG_ERROR, "Failed to create model buffers");
                model_deinit(mdl);
                return -1;
        }

        /* The VAO remembers the layout and the index buffer bound here */
        glBindVertexArray(mdl->vao);
        glBindBuffer(GL_ARRAY_BUFFER, mdl->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdl->ibo);

        glEnableVertexAttribArray(MODEL_ATTR_POSITION);
        glVertexAttribPointer(MODEL_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex),
                              (void *)offsetof(Vertex, position));

        glEnableVertexAttribArray(MODEL_ATTR_NORMAL);
        glVertexAttribPointer(MODEL_ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex),
                              (void *)offsetof(Vertex, normal));

        glEnableVertexAttribArray(MODEL_ATTR_UV);
        glVertexAttribPointer(MODEL_ATTR_UV, 2, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex),
                              (void *)offsetof(Vertex, uv));

        /* 4 bytes, mapped from 0..255 to 0..1 */
        glEnableVertexAttribArray(MODEL_ATTR_COLOR);
        glVertexAttribPointer(MODEL_ATTR_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                              sizeof(Vertex),
                              (void *)offsetof(Vertex, color));

        m_upload(mdl, mesh);

        /* Unbind the VAO first, so it keeps the index buffer */
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        return 0;
}

PUB void model_deinit(Model *mdl)
{
        if(!mdl) {
                return;
        }

        /* Deleting 0 is ignored by GL, so this also cleans up a half init */
        glDeleteVertexArrays(1, &mdl->vao);
        glDeleteBuffers(1, &mdl->vbo);
        glDeleteBuffers(1, &mdl->ibo);

        mdl->vao = 0;
        mdl->vbo = 0;
        mdl->ibo = 0;
        mdl->index_count = 0;
}

PUB s32 model_update(Model *mdl, Mesh *mesh)
{
        if(!mdl || !mesh) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(m_check_faces(mesh) < 0) {
                return -1;
        }

        glBindVertexArray(mdl->vao);
        glBindBuffer(GL_ARRAY_BUFFER, mdl->vbo);

        m_upload(mdl, mesh);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        return 0;
}

PUB void model_draw(Model *mdl)
{
        if(!mdl || mdl->index_count < 1) {
                return;
        }

        glBindVertexArray(mdl->vao);
        glDrawElements(GL_TRIANGLES, mdl->index_count, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC s32 m_check_faces(Mesh *mesh)
{
        s32 vtx_count = mesh_vertex_count(mesh);
        s32 i;
        s32 j;

        for(i = 0; i < mesh_face_count(mesh); i++) {
                Face *f = list_get(&mesh->faces, i);

                for(j = 0; j < 3; j++) {
                        if(f->indices[j] < 0 || f->indices[j] >= vtx_count) {
                                LOG_I(LOG_ERROR, "Face %d uses a vertex "
                                      "that doesn't exist", i);
                                return -1;
                        }
                }
        }

        return 0;
}

LOC void m_upload(Model *mdl, Mesh *mesh)
{
        s32 vtx_count = mesh_vertex_count(mesh);
        s32 face_count = mesh_face_count(mesh);

        /*
         * The lists store their items one after another, so they can be
         * uploaded directly. Face holds 3 s32, which matches GL_UNSIGNED_INT
         * because m_check_faces made sure no index is negative.
         */
        glBufferData(GL_ARRAY_BUFFER, vtx_count * sizeof(Vertex),
                     vtx_count ? list_get(&mesh->vertices, 0) : NULL,
                     GL_STATIC_DRAW);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, face_count * sizeof(Face),
                     face_count ? list_get(&mesh->faces, 0) : NULL,
                     GL_STATIC_DRAW);

        mdl->index_count = face_count * 3;
}
