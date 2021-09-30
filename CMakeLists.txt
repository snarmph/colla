add_library(Colla STATIC 
    socket.c 
    tracelog.c 
    http.c 
    strstream.c 
    strview.c 
    strutils.c 
)

IF (WIN32)
    target_link_libraries(Common ws2_32.lib)
ELSE()
    # posix
ENDIF()