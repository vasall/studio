/* fragment shader */
#version 330 core
in vec2 v_uv;
out vec4 out_color;
uniform sampler2D u_canvas;

void main()
{
        out_color = texture(u_canvas, v_uv);     /* already premultiplied */
}
