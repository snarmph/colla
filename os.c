#include "os.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#define _BUFSZ 128

// modified from netbsd source http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/pkgtools/libnbcompat/files/getdelim.c?only_with_tag=MAIN
ssize_t getdelim(char **buf, size_t *bufsz, int delimiter, FILE *fp) {
    char *ptr, *eptr;

    if(*buf == NULL || *bufsz == 0) {
        *bufsz = _BUFSZ;
        if((*buf = malloc(*bufsz)) == NULL) {
            return -1;
        }
    }

    ssize_t result = -1;
    // usually fgetc locks every read, using windows-specific 
    // _lock_file and _unlock_file will be faster
    _lock_file(fp);

    for(ptr = *buf, eptr = *buf + *bufsz;;) {
        int c = _getc_nolock(fp);
        if(c == -1) {
            if(feof(fp)) {
                ssize_t diff = (ssize_t)(ptr - *buf);
                if(diff != 0) {
                    *ptr = '\0';
                    result = diff;
                    break;
                }
            }
            break;
        }
        *ptr++ = (char)c;
        if(c == delimiter) {
            *ptr = '\0';
            result = ptr - *buf;
            break;
        }
        if((ptr + 2) >= eptr) {
            char *nbuf;
            size_t nbufsz = *bufsz * 2;
            ssize_t d = ptr - *buf;
            if((nbuf = realloc(*buf, nbufsz)) == NULL) {
                break;
            }
            *buf = nbuf;
            *bufsz = nbufsz;
            eptr = nbuf + nbufsz;
            ptr = nbuf + d;
        }
    }

    _unlock_file(fp);
    return result;
}

// taken from netbsd source http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/pkgtools/libnbcompat/files/getline.c?only_with_tag=MAIN
ssize_t getline(char **line_ptr, size_t *n, FILE *stream) {
    return getdelim(line_ptr, n, '\n', stream);
}
#endif