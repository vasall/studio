#version 330 core

/* All positions in framebuffer pixels, y down */
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_local;     /* Relative to the rect center */
layout(location = 3) in vec2 a_half;      /* Half size of the rect */
layout(location = 4) in vec4 a_radius;    /* tl, tr, br, bl */
layout(location = 5) in vec2 a_params;    /* border width, mode */
layout(location = 6) in vec4 a_color;
layout(location = 7) in vec4 a_border_color;

uniform vec2 u_screen;

out vec2 v_uv;
out vec2 v_local;
flat out vec2 v_half;
flat out vec4 v_radius;
flat out vec2 v_params;
flat out vec4 v_color;
flat out vec4 v_border_color;

void main()
{
        v_uv = a_uv;
        v_local = a_local;
        v_half = a_half;
        v_radius = a_radius;
        v_params = a_params;
        v_color = a_color;
        v_border_color = a_border_color;

        gl_Position = vec4(a_pos.x / u_screen.x * 2.0 - 1.0,
                           1.0 - a_pos.y / u_screen.y * 2.0, 0.0, 1.0);
}
