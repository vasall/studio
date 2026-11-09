#include <render/shader.h>

#include <devkit/log/log.h>

#include <stddef.h>

#define SHADER_LOG_LENGTH       1024    /* Max. length of a GL error log */

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Compile one shader stage.
 *
 * @returns The shader object, or 0 if compiling failed
 */
LOC GLuint sh_compile(GLenum type, const char *src);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 shader_init(Shader *sh, const char *vert, const char *frag)
{
        char log[SHADER_LOG_LENGTH];
        GLuint vs;
        GLuint fs;
        GLint ok;

        if(!sh || !vert || !frag) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(!(vs = sh_compile(GL_VERTEX_SHADER, vert))) {
                return -1;
        }

        if(!(fs = sh_compile(GL_FRAGMENT_SHADER, frag))) {
                glDeleteShader(vs);
                return -1;
        }

        sh->program = glCreateProgram();
        glAttachShader(sh->program, vs);
        glAttachShader(sh->program, fs);
        glLinkProgram(sh->program);

        /* The program keeps the compiled code, the stages aren't needed */
        glDetachShader(sh->program, vs);
        glDetachShader(sh->program, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        glGetProgramiv(sh->program, GL_LINK_STATUS, &ok);
        if(!ok) {
                glGetProgramInfoLog(sh->program, sizeof(log), NULL, log);
                LOG_S(LOG_ERROR, "Failed to link shader:\n%s", log);
                goto err_delete_program;
        }

        if(uniform_cache_init(&sh->uniforms, sh->program) < 0) {
                goto err_delete_program;
        }

        return 0;

err_delete_program:
        glDeleteProgram(sh->program);
        sh->program = 0;

        return -1;
}

PUB void shader_deinit(Shader *sh)
{
        if(!sh) {
                return;
        }

        uniform_cache_deinit(&sh->uniforms);
        glDeleteProgram(sh->program);
        sh->program = 0;
}

PUB GLint shader_uniform(Shader *sh, const char *name)
{
        if(!sh) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        return uniform_cache_get(&sh->uniforms, name);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC GLuint sh_compile(GLenum type, const char *src)
{
        char log[SHADER_LOG_LENGTH];
        GLuint shader;
        GLint ok;

        shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);

        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if(!ok) {
                glGetShaderInfoLog(shader, sizeof(log), NULL, log);
                LOG_S(LOG_ERROR, type == GL_VERTEX_SHADER ?
                      "Failed to compile vertex shader:\n%s" :
                      "Failed to compile fragment shader:\n%s", log);
                glDeleteShader(shader);
                return 0;
        }

        return shader;
}
