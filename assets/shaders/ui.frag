#version 330 core

in vec2 v_uv;
in vec2 v_local;
flat in vec2 v_half;
flat in vec4 v_radius;
flat in vec2 v_params;
flat in vec4 v_color;
flat in vec4 v_border_color;

uniform sampler2D u_tex;

out vec4 o_color;

/* Signed distance to a rounded box with one radius per corner (y down) */
float sd_round_box(vec2 p, vec2 b, vec4 r)
{
        float rad = p.x > 0.0 ? (p.y > 0.0 ? r.z : r.y)
                              : (p.y > 0.0 ? r.w : r.x);
        vec2 q = abs(p) - b + rad;
        return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

void main()
{
        float mode = v_params.y;

        if(mode < 0.5) {
                /* Rect: fill + border, antialiased edges */
                float d = sd_round_box(v_local, v_half, v_radius);
                float outer = clamp(0.5 - d, 0.0, 1.0);
                vec4 fill = vec4(v_color.rgb * v_color.a, v_color.a);
                vec4 border = vec4(v_border_color.rgb * v_border_color.a,
                                   v_border_color.a);

                if(v_params.x > 0.0) {
                        float inner = clamp(0.5 - (d + v_params.x), 0.0, 1.0);
                        o_color = fill * inner + border * (outer - inner);
                } else {
                        o_color = fill * outer;
                }
        } else if(mode < 1.5) {
                /* Glyph or icon mask, tinted; slight boost for thin text */
                float a = pow(texture(u_tex, v_uv).r, 0.85) * v_color.a;
                o_color = vec4(v_color.rgb * a, a);
        } else {
                /* Full texture (premultiplied), e.g. a viewport */
                o_color = texture(u_tex, v_uv) * v_color;
        }
}
