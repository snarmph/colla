add_library(Colla STATIC 
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
)

if(MSVC)
    target_link_libraries(Colla ws2_32.lib)
    target_compile_options(Colla PRIVATE /W4)
else()
    target_link_libraries(Colla pthread)
    target_compile_options(Colla PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_definitions(Colla PUBLIC _DEFAULT_SOURCE)
endif()