/* vertex shader: no vertex buffer, the corners come from gl_VertexID */
#version 330 core
out vec2 v_uv;

void main()
{
        /* One triangle that covers the whole window: (−1,−1) (3,−1) (−1,3) */
        vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
        v_uv = vec2(pos.x, 1.0 - pos.y);         /* image row 0 is the top */
        gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
