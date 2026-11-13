#include <render/gl_util.h>

#include <devkit/log/log.h>

#include <core/file.h>

#include <stdio.h>
#include <stdlib.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC GLuint s_compile(GLenum type, const char *src, const char *name);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB GLuint gl_program_from_files(const char *vert_path, const char *frag_path)
{
        char *vs_src;
        char *fs_src;
        GLuint prog = 0;

        vs_src = file_read(vert_path, NULL);
        fs_src = file_read(frag_path, NULL);
        if(vs_src && fs_src) {
                prog = gl_program_from_source(vs_src, fs_src, vert_path);
        }

        free(vs_src);
        free(fs_src);
        return prog;
}

PUB GLuint gl_program_from_source(const char *vert_src, const char *frag_src,
                const char *name)
{
        GLuint vs = 0;
        GLuint fs = 0;
        GLuint prog = 0;
        GLint ok;
        char log[1024];

        vs = s_compile(GL_VERTEX_SHADER, vert_src, name);
        fs = s_compile(GL_FRAGMENT_SHADER, frag_src, name);
        if(!vs || !fs) goto out;

        prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if(!ok) {
                glGetProgramInfoLog(prog, sizeof(log), NULL, log);
                LOG_S(LOG_ERROR, "shader link failed: %s", log);
                glDeleteProgram(prog);
                prog = 0;
        }

out:
        if(vs) glDeleteShader(vs);
        if(fs) glDeleteShader(fs);
        return prog;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC GLuint s_compile(GLenum type, const char *src, const char *name)
{
        GLuint sh;
        GLint ok;
        char log[1024];

        sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, NULL);
        glCompileShader(sh);
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if(!ok) {
                glGetShaderInfoLog(sh, sizeof(log), NULL, log);
                LOG_SS(LOG_ERROR, "shader %s: %s", (char *)name, log);
                glDeleteShader(sh);
                return 0;
        }
        return sh;
}
