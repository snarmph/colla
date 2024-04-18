#include "os.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if COLLA_WIN
#define _BUFSZ 128

#include <lmcons.h>

// modified from netbsd source http://cvsweb.netbsd.org/bsdweb.cgi/pkgsrc/pkgtools/libnbcompat/files/getdelim.c?only_with_tag=MAIN
isize getdelim(char **buf, size_t *bufsz, int delimiter, FILE *fp) {
    char *ptr, *eptr;

    if(*buf == NULL || *bufsz == 0) {
        *bufsz = _BUFSZ;
        if((*buf = malloc(*bufsz)) == NULL) {
            return -1;
        }
    }

    isize result = -1;
    // usually fgetc locks every read, using windows-specific 
    // _lock_file and _unlock_file will be faster
    _lock_file(fp);

    for(ptr = *buf, eptr = *buf + *bufsz;;) {
        int c = _getc_nolock(fp);
        if(c == -1) {
            if(feof(fp)) {
                isize diff = (isize)(ptr - *buf);
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
            isize d = ptr - *buf;
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
isize getline(char **line_ptr, size_t *n, FILE *stream) {
    return getdelim(line_ptr, n, '\n', stream);
}

str_t getUserName() {    
    char buf[UNLEN + 1];
    DWORD sz = sizeof(buf);
    BOOL res = GetUserNameA(buf, &sz);
    if(!res) {
        return strInit();
    }
    return strFromBuf(buf, sz);
}

#else

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

int stricmp(const char *a, const char *b) {
    int result;
    
    if (a == b) {
        return 0;
    }

    while ((result = tolower(*a) - tolower(*b++)) == 0) {
        if (*a++ == '\0') {
            break;
        }
    }

    return result;
}

str_t getUserName() {
    return strFromStr(getlogin());
}

#endif
