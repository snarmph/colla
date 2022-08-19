add_library(colla STATIC 
    src/collatypes.h
    src/vec.h
    src/win32_slim.h 
    src/tracelog.h      src/tracelog.c
    src/str.h           src/str.c
    src/hashmap.h       src/hashmap.c
    src/utf8.h          src/utf8.c
    src/ini.h           src/ini.c
    src/strstream.h     src/strstream.c
    src/os.h            src/os.c
    src/file.h          src/file.c
    src/dir.h           src/dir.c
    src/socket.h        src/socket.c
    src/http.h          src/http.c
    src/cthreads.h      src/cthreads.c
)

if(MSVC)
    target_link_libraries(colla ws2_32.lib)
    target_compile_options(colla PRIVATE /W4)
else()
    target_link_libraries(colla pthread)
    target_compile_options(colla PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_definitions(colla PUBLIC _DEFAULT_SOURCE)
endif()

target_include_directories(colla PUBLIC src)