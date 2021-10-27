add_library(colla STATIC 
    socket.c 
    tracelog.c 
    http.c 
    strstream.c 
    str.c 
    coroutine.c 
    os.c 
    fs.c 
    file.c 
    dir.c 
    dirwatch.c
    cthreads.c
)

if(MSVC)
    target_link_libraries(colla ws2_32.lib)
    target_compile_options(colla PRIVATE /W4)
else()
    target_link_libraries(colla pthread)
    target_compile_options(colla PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_definitions(colla PUBLIC _DEFAULT_SOURCE)
endif()