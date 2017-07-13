#==============================================================================#
#   Copyright (c) 2017, Stanislav Gromov                                       #
#                                                                              #
#   Permission to use, copy, modify, and/or distribute this software for any   #
#   purpose with or without fee is hereby granted, provided that the above     #
#   copyright notice and this permission notice appear in all copies.          #
#                                                                              #
#   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES   #
#   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF           #
#   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR    #
#   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES     #
#   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN      #
#   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF    #
#   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.             #
#==============================================================================#

cmake_minimum_required(VERSION 3.0)


#==============================================================================#
# Settings                                                                     #
#==============================================================================#
set(PLUGIN_NAME "HelloWorld")
set(PLUGIN_VERSION_MAJOR 0)
set(PLUGIN_VERSION_MINOR 0)
set(PLUGIN_VERSION_BUILD 1)

set(PLUGIN_SUPPORTS_PROCESSTICK FALSE)
set(PLUGIN_SRC
	"main.cpp"
)
set(PLUGIN_LINK_DEPENDENCIES "")
#==============================================================================#


string(TOLOWER ${PLUGIN_NAME} PLUGIN_NAME_LOWERCASE)
configure_file("plugin.inc.in" "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME_LOWERCASE}.inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(PLUGIN_SRC
	"${PLUGIN_SRC}"
	"SDK/amx/amx.h"
	"SDK/plugincommon.h"
	"SDK/amxplugin.cpp"
	"${CMAKE_CURRENT_BINARY_DIR}/pluginconfig.h"
	"pluginutils.h"
)
if(UNIX)
	set(PLUGIN_SRC
		"${PLUGIN_SRC}"
		"SDK/amx/sclinux.h"
	)
	add_definitions("/DLINUX")
endif()

set(PLUGIN_SUPPORTS_FLAGS "SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES")
if(PLUGIN_SUPPORTS_PROCESSTICK)
	set(PLUGIN_SUPPORTS_FLAGS "${PLUGIN_SUPPORTS_FLAGS} | SUPPORTS_PROCESS_TICK")
endif()
configure_file("pluginconfig.h.in" "${CMAKE_CURRENT_BINARY_DIR}/pluginconfig.h")

project(${PLUGIN_NAME}
	VERSION ${PLUGIN_VERSION_MAJOR}.${PLUGIN_VERSION_MINOR}.${PLUGIN_VERSION_BUILD}
)
add_library(${PLUGIN_NAME_LOWERCASE} SHARED ${PLUGIN_SRC})
target_link_libraries(${PLUGIN_NAME_LOWERCASE} ${PLUGIN_LINK_DEPENDENCIES})
set_target_properties(${PLUGIN_NAME_LOWERCASE} PROPERTIES PREFIX "")
if(MSVC)
	set(STR_PLUGIN_DEF_PROCESSTICK "")
	if(PLUGIN_SUPPORTS_PROCESSTICK)
		set(STR_PLUGIN_DEF_PROCESSTICK "ProcessTick")
	endif()
	configure_file("plugin.def.in" "${CMAKE_CURRENT_BINARY_DIR}/plugin.def")
	set_target_properties(${PLUGIN_NAME_LOWERCASE}
		PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_BINARY_DIR}/plugin.def\""
	)
endif()
add_custom_command(
	TARGET "${PLUGIN_NAME_LOWERCASE}" POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME_LOWERCASE}.inc
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${PLUGIN_NAME_LOWERCASE}.inc
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME_LOWERCASE}.inc" DESTINATION "include")
install(TARGETS ${PLUGIN_NAME_LOWERCASE}
	RUNTIME DESTINATION "plugin"
	ARCHIVE DESTINATION "plugin"
)
if (WIN32)
	set(CPACK_GENERATOR "ZIP")
elseif (UNIX)
	set(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_PACKAGE_FILE_NAME
	"${PLUGIN_NAME_LOWERCASE}-${PLUGIN_VERSION_MAJOR}.${PLUGIN_VERSION_MINOR}.${PLUGIN_VERSION_BUILD}"
)
include(CPack)
