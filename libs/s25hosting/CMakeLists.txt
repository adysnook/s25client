# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Add Icon to Application
if(WIN32)
    set(rcDir ${PROJECT_SOURCE_DIR}/data/win32)
    if(CMAKE_HOST_WIN32)
        set(s25hosting_RC ${rcDir}/s25client.rc)
    else()
        add_definitions(-DAFX_TARG_DEU)
        set(s25hosting_RC ${CMAKE_CURRENT_BINARY_DIR}/s25hosting.res.o)
        add_custom_command(OUTPUT ${s25hosting_RC}
                           COMMAND ${CMAKE_RC_COMPILER} -I${rcDir}/ -i${rcDir}/s25client.rc -o ${s25hosting_RC})

    endif()
else()
    set(s25hosting_RC )
endif()

add_executable(s25hosting s25hosting.cpp commands.cpp ${s25hosting_RC})
target_link_libraries(s25hosting PRIVATE s25Main Boost::program_options Boost::nowide rttr::vld)
add_dependencies(s25hosting drivers)

if(WIN32)
    target_include_directories(s25hosting PRIVATE ${rcDir})
    target_link_libraries(s25hosting PRIVATE ole32 ws2_32 shlwapi imagehlp)
    if(MSVC)
        set_target_properties(s25hosting PROPERTIES LINK_FLAGS /MANIFEST:NO)
    endif()
    if(CMAKE_COMPILER_IS_GNUCXX)
        set_target_properties(s25hosting PROPERTIES LINK_FLAGS -Wl,--stack,8388608)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(s25hosting PRIVATE pthread)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_link_libraries(s25hosting PRIVATE execinfo)
ENDif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT s25hosting)
endif()

if(WIN32)
    include(GatherDll)
    gather_dll_copy(s25hosting)
endif()

INSTALL(TARGETS s25hosting RUNTIME DESTINATION ${RTTR_BINDIR})
