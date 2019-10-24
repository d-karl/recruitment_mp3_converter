cmake_minimum_required(VERSION 3.10)

project(mp3_converter)

############################################
#  Acquire and build lame static library.  #
############################################
include(ExternalProject)
include(cmake/acquire_lame.cmake)
# cmake target libmp3lame exists now with all necessary information to compile

# during development, allows me to build on gnu with Address sanitizer, to
# catch undefined behavior and memory leaks.
option(BUILD_WITH_ASAN "Whether to build application with ASAN" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/WavefileConversionTask.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        libmp3lame
        $<$<C_COMPILER_ID:GNU>:stdc++fs>
        $<$<C_COMPILER_ID:GNU>:pthread>
        $<$<BOOL:${BUILD_WITH_ASAN}>:-fno-omit-frame-pointer>
        $<$<BOOL:${BUILD_WITH_ASAN}>:-fsanitize=address>
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
)

target_compile_features(${PROJECT_NAME}
    PUBLIC
        cxx_std_17
)

# wait until lame has been acquired to build project
add_dependencies(${PROJECT_NAME} lame)
