#version 330 core

in vec3 v_normal;
in vec4 v_color;

out vec4 frag_color;

void main(void)
{
        /* Light comes from the camera, so faces turned to it are brightest */
        float light = max(dot(normalize(v_normal), vec3(0.0, 0.0, 1.0)), 0.0);
        float shade = 0.25 + 0.75 * light;

        frag_color = vec4(v_color.rgb * shade, v_color.a);
}
