add_library(Colla STATIC 
    socket.c 
    tracelog.c 
    http.c 
    strstream.c 
    strview.c 
    str.c 
    coroutine.c 
    os.c 
)

IF (WIN32)
    target_link_libraries(Colla ws2_32.lib)
ELSE()
    # posix
ENDIF()