if(WIN32)
    # Windows additional options, libraries and default path (Picoscope SDK 64bits)
    # Use MinGW formatting (C *printf Format "%zu"...) rather than MSVCRT for C99 support.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__USE_MINGW_ANSI_STDIO=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__USE_MINGW_ANSI_STDIO=1")
    set(WIN_LIBS shlwapi)
else()
endif()



###############################################################################
#C++ compilation
add_executable(owon_firmware_extractor
        main.cpp
        NSIS.cpp
)


###############################################################################
#Linker settings
if(WIN32)
    # Windows specific linker
    target_link_libraries(owon_firmware_extractor
            log
    )
else()
    # Linux specific linker
    target_link_libraries(owon_firmware_extractor
            log
    )
endif()

target_compile_options(owon_firmware_extractor PRIVATE -Wno-shadow)