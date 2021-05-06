# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Add Icon to Application
if(WIN32)
    set(rcDir ${PROJECT_SOURCE_DIR}/data/win32)
    if(CMAKE_HOST_WIN32)
        set(s25server_RC ${rcDir}/s25client.rc)
    else()
        add_definitions(-DAFX_TARG_DEU)
        set(s25server_RC ${CMAKE_CURRENT_BINARY_DIR}/s25server.res.o)
        add_custom_command(OUTPUT ${s25server_RC}
                           COMMAND ${CMAKE_RC_COMPILER} -I${rcDir}/ -i${rcDir}/s25client.rc -o ${s25server_RC})

    endif()
else()
    set(s25server_RC )
endif()

add_executable(s25server s25server.cpp commands.cpp ${s25server_RC})
target_link_libraries(s25server PRIVATE s25Main Boost::program_options Boost::nowide rttr::vld)
add_dependencies(s25server drivers)

if(WIN32)
    target_include_directories(s25server PRIVATE ${rcDir})
    target_link_libraries(s25server PRIVATE ole32 ws2_32 shlwapi imagehlp)
    if(MSVC)
        set_target_properties(s25server PROPERTIES LINK_FLAGS /MANIFEST:NO)
    endif()
    if(CMAKE_COMPILER_IS_GNUCXX)
        set_target_properties(s25server PROPERTIES LINK_FLAGS -Wl,--stack,8388608)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(s25server PRIVATE pthread)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_link_libraries(s25server PRIVATE execinfo)
ENDif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT s25server)
endif()

if(WIN32)
    include(GatherDll)
    gather_dll_copy(s25server)
endif()

INSTALL(TARGETS s25server RUNTIME DESTINATION ${RTTR_BINDIR})
