cmake_minimum_required(VERSION 3.26)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
project(akoc
        LANGUAGES C
        VERSION 0.1.0
        DESCRIPTION "A C implementation of the Ako config language"
        HOMEPAGE_URL "https://github.com/Tuyuji/AkoC"
)

set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_library(akoc
        src/lex/tokenizer.c
        src/mem/dyn_array.c
        src/elem.c
        src/ako.c
        src/mem/dyn_string.c
        src/lex/parser.c)
target_include_directories(akoc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#Test
project(akotest C)
add_executable(akotest test/main.c)
target_include_directories(akotest PUBLIC akoc)
target_link_libraries(akotest PUBLIC akoc)

enable_testing()

add_test(NAME akotest
        COMMAND $<TARGET_FILE:akotest>
)

# A bunch of small cli tools
add_subdirectory(utils)

include(GNUInstallDirs)

install(TARGETS akoc
    EXPORT akocTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT akocTargets
    FILE akocTargets.cmake
    NAMESPACE akoc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/akoc
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/akocConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/akocConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/akoc
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/akocConfigVersion.cmake"
    VERSION ${akoc_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/akocConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/akocConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/akoc
)
