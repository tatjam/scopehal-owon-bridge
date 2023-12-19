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
add_executable(vds1022
        OWONSCPIServer.cpp
        main.cpp
        Driver.cpp
)


###############################################################################
#Linker settings
if(WIN32)
    # Windows specific linker
    target_link_libraries(vds1022
            xptools
            log
            scpi-server-tools
            ${libusb_LIBRARIES}
    )
else()
    # Linux specific linker
    target_link_libraries(vds1022
            xptools
            log
            scpi-server-tools
            usb-1.0
            ${libusb_LIBRARIES}
    )
endif()