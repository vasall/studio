#include <render/gl_util.h>

#include <devkit/log/log.h>

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

PUB char *gl_read_file(const char *path, s64 *size_out)
{
        FILE *fp;
        long size;
        char *buf;

        fp = fopen(path, "rb");
        if(!fp) {
                LOG_S(LOG_ERROR, "can't open %s", (char *)path);
                return NULL;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if(size < 0) {
                fclose(fp);
                return NULL;
        }

        buf = malloc((size_t)size + 1);
        if(buf) {
                size = (long)fread(buf, 1, (size_t)size, fp);
                buf[size] = '\0';
                if(size_out) *size_out = size;
        }
        fclose(fp);
        return buf;
}

PUB GLuint gl_program_from_files(const char *vert_path, const char *frag_path)
{
        char *vs_src;
        char *fs_src;
        GLuint vs = 0;
        GLuint fs = 0;
        GLuint prog = 0;
        GLint ok;
        char log[1024];

        vs_src = gl_read_file(vert_path, NULL);
        fs_src = gl_read_file(frag_path, NULL);
        if(!vs_src || !fs_src) goto out;

        vs = s_compile(GL_VERTEX_SHADER, vs_src, vert_path);
        fs = s_compile(GL_FRAGMENT_SHADER, fs_src, frag_path);
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
        free(vs_src);
        free(fs_src);
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
