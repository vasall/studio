#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;

uniform mat4 u_mvp;
uniform mat4 u_model_view;

out vec3 v_normal;      /* in camera space */
out vec4 v_color;

void main(void)
{
        /* Correct for rotation and uniform scale, not for uneven scale */
        v_normal = mat3(u_model_view) * a_normal;
        v_color = a_color;
        gl_Position = u_mvp * vec4(a_position, 1.0);
}
