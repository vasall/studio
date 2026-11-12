#include <render/canvas_demo.h>

#include <render/gl_util.h>

/* Four corners from gl_VertexID, so no vertex buffer is needed */
LOC const char *s_vert =
        "#version 330 core\n"
        "uniform vec4 u_box; /* x0, y0, x1, y1 in NDC */\n"
        "void main()\n"
        "{\n"
        "        vec2 c = vec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
        "        gl_Position = vec4(mix(u_box.xy, u_box.zw, c), 0.0, 1.0);\n"
        "}\n";

LOC const char *s_frag =
        "#version 330 core\n"
        "uniform vec4 u_color;\n"
        "out vec4 frag;\n"
        "void main()\n"
        "{\n"
        "        /* The UI blends premultiplied colors */\n"
        "        frag = vec4(u_color.rgb * u_color.a, u_color.a);\n"
        "}\n";

LOC GLuint s_program;
LOC GLuint s_vao;               /* Empty, core profile needs one bound */
LOC GLint s_u_box;
LOC GLint s_u_color;
LOC s32 s_failed;

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void canvas_demo_draw(const UIDrawCallbackInfo *info, void *data)
{
        const CanvasDemoBox *b = data;

        (void)info;
        if(!b || s_failed) return;

        if(!s_program) {
                s_program = gl_program_from_source(s_vert, s_frag,
                                "canvas_demo");
                if(!s_program) {
                        s_failed = 1;
                        return;
                }
                s_u_box = glGetUniformLocation(s_program, "u_box");
                s_u_color = glGetUniformLocation(s_program, "u_color");
                glGenVertexArrays(1, &s_vao);
        }

        /* The viewport is the callback's area: 0..1 from the top left
           becomes -1..1 with y up */
        glUseProgram(s_program);
        glUniform4f(s_u_box, b->x * 2.0f - 1.0f, 1.0f - (b->y + b->h) * 2.0f,
                        (b->x + b->w) * 2.0f - 1.0f, 1.0f - b->y * 2.0f);
        glUniform4f(s_u_color, b->color.r / 255.0f, b->color.g / 255.0f,
                        b->color.b / 255.0f, b->color.a / 255.0f);
        glBindVertexArray(s_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
