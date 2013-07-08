cmake_minimum_required (VERSION 2.6)
project (tankfighter)


# define SFML requirement
include(FindPkgConfig)
if(PKG_CONFIG_FOUND)
 message(STATUS "PkgConfig Found.")
else(PKG_CONFIG_FOUND)
 message(STATUS "PkgConfig Not Found.")
endif(PKG_CONFIG_FOUND)

pkg_search_module(SFML2 REQUIRED sfml-all >= 2.00)

include_directories (${SFML2_INCLUDE_DIRS})
link_directories (${SFML2_LIBRARY_DIRS})
message(STATUS "link_directories ${LINK_DIRECTORIES}")

# define main executable target
add_executable (tankfighter ${CMAKE_BINARY_DIR}/keys.cpp joystick_controller.cpp missile.cpp engine_event.cpp controller.cpp keybmouse_controller.cpp player.cpp texture_cache.cpp entity.cpp wall.cpp load_map.cpp engine.cpp misc.cpp main.cpp json.c geometry.cpp)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/keys.cpp COMMAND ${CMAKE_SOURCE_DIR}/generate_keys.cpp.sh)
target_link_libraries (tankfighter ${SFML2_LIBRARIES})
string (REPLACE ";" " " SFML2_LDFLAGS_OTHER "${SFML2_LDFLAGS_OTHER}") # fix bug in cmake that make LDFLAGS semicolon separated on command line
set_property(TARGET tankfighter PROPERTY LINK_FLAGS "${LINK_FLAGS} ${SFML2_LDFLAGS_OTHER}")
install (TARGETS tankfighter DESTINATION bin)
