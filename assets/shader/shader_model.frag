#version 330 core

in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;

/* White 1x1 for objects without a texture, so only the color shows */
uniform sampler2D u_texture;

out vec4 frag_color;

void main(void)
{
        /* Light comes from the camera, so faces turned to it are brightest */
        float light = max(dot(normalize(v_normal), vec3(0.0, 0.0, 1.0)), 0.0);
        float shade = 0.25 + 0.75 * light;

        vec4 base = texture(u_texture, v_uv) * v_color;

        frag_color = vec4(base.rgb * shade, base.a);
}
