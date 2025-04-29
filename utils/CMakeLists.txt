# cli tools

if(NOT WIN32)
    add_executable(akocli
            akocli.c)
    target_link_libraries(akocli PUBLIC akoc)
endif ()