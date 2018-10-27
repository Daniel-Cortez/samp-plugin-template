#==============================================================================#
#   SA-MP plugin template v1.1                                                 #
#                                                                              #
#   Copyright (c) 2017-2018, Stanislav Gromov                                  #
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
include(CheckIncludeFiles)

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


project(${PLUGIN_NAME}
	LANGUAGES CXX
	VERSION ${PLUGIN_VERSION_MAJOR}.${PLUGIN_VERSION_MINOR}.${PLUGIN_VERSION_BUILD}
)

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
	"pluginutils.cpp"
)
if(UNIX)
	set(PLUGIN_SRC
		"${PLUGIN_SRC}"
		"SDK/amx/sclinux.h"
	)
	# check compiler bits
	if(CMAKE_COMPILER_IS_GNUCXX) 
		if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4) 
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32") 
		endif() 
	endif()
	add_definitions("/DLINUX")
endif()
if(MINGW)
	add_definitions("/DHAVE_STDINT_H" "/D__need_size_t")
endif()

set(PLUGIN_SUPPORTS_FLAGS "SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES")
if(PLUGIN_SUPPORTS_PROCESSTICK)
	set(PLUGIN_SUPPORTS_FLAGS "${PLUGIN_SUPPORTS_FLAGS} | SUPPORTS_PROCESS_TICK")
endif()
configure_file("pluginconfig.h.in" "${CMAKE_CURRENT_BINARY_DIR}/pluginconfig.h")

add_library(${PLUGIN_NAME_LOWERCASE} SHARED ${PLUGIN_SRC})
target_link_libraries(${PLUGIN_NAME_LOWERCASE} ${PLUGIN_LINK_DEPENDENCIES})
set_property(TARGET ${PLUGIN_NAME_LOWERCASE} PROPERTY PREFIX "")
if(MSVC)
	set(STR_PLUGIN_DEF_PROCESSTICK "")
	if(PLUGIN_SUPPORTS_PROCESSTICK)
		set(STR_PLUGIN_DEF_PROCESSTICK "ProcessTick")
	endif()
	configure_file("plugin.def.in" "${CMAKE_CURRENT_BINARY_DIR}/plugin.def")
	# check exists stddef.h and stdint.h
	check_include_files (stddef.h HAVE_STDDEF_H)
	check_include_files (stdint.h HAVE_STDINT_H)
	if(HAVE_STDINT_H)
		# add to preprocessor "HAVE_STDINT_H"
		add_definitions(-DHAVE_STDINT_H)
	endif(HAVE_STDINT_H)
	set_property(TARGET ${PLUGIN_NAME_LOWERCASE}
		APPEND_STRING PROPERTY LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_BINARY_DIR}/plugin.def\""
	)
endif()
if(MINGW)
	set_property(TARGET ${PLUGIN_NAME_LOWERCASE}
		APPEND_STRING PROPERTY LINK_FLAGS "-Wl,-k"
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
		LIBRARY DESTINATION "plugins"
		RUNTIME DESTINATION "plugins"
)
if(WIN32)
	set(CPACK_GENERATOR "ZIP")
elseif(UNIX)
	set(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_PACKAGE_NAME "${PLUGIN_NAME_LOWERCASE}")
set(CPACK_PACKAGE_VERSION_MAJOR ${PLUGIN_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PLUGIN_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PLUGIN_VERSION_BUILD})
include(CPack)