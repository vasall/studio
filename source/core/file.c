#include <core/file.h>

#include <devkit/log/log.h>

#include <stdio.h>
#include <stdlib.h>

PUB char *file_read(const char *path, s64 *size_out)
{
        FILE *fp;
        long size;
        char *buf;

        if(!(fp = fopen(path, "rb"))) {
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

        if((buf = malloc((size_t)size + 1))) {
                size = (long)fread(buf, 1, (size_t)size, fp);
                buf[size] = '\0';
                if(size_out) *size_out = size;
        }

        fclose(fp);
        return buf;
}
