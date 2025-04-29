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
target_include_directories(akoc PUBLIC include)

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