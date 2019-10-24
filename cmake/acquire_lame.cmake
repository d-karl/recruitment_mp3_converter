set(Lame_URL http://downloads.sourceforge.net/project/lame/lame/3.100/lame-3.100.tar.gz)

if(UNIX)
    ExternalProject_Add(
        lame
        URL ${Lame_URL}
        CONFIGURE_COMMAND ./configure --prefix=${CMAKE_BINARY_DIR}/lame --disable-shared --disable-gtktest --disable-decoder --disable-frontend
        BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -f Makefile.unix libmp3lame
        BUILD_IN_SOURCE 1
    )
    add_library(libmp3lame STATIC IMPORTED)
    set_target_properties(libmp3lame
        PROPERTIES
            IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lame/lib/libmp3lame.a
            INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/lame-prefix/src/lame/include
    )
endif()
if(WIN32)
    if(CMAKE_CL_64)
        set(lame64_switches MSVCVER=Win64 MACHINE=/machine:X64)
    endif()

    ExternalProject_Add(
        lame
        URL ${Lame_URL}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND nmake ASM=NO ${lame64_switches} -f Makefile.MSVC
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE 1
    )

    add_library(libmp3lame STATIC IMPORTED)
    set_target_properties(libmp3lame
        PROPERTIES
            IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lame-prefix/src/lame/output/libmp3lame-static.lib
            INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/lame-prefix/src/lame/include
    )
endif()

if(NOT TARGET lame)
    message(FATAL_ERROR "You are on a weird system that is neither \"UNIX\" nor \"WIN32\" to cmake. Can not get lame for you.")
endif()

execute_process(COMMAND
    ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lame-prefix/src/lame/include)


