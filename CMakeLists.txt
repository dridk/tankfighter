cmake_minimum_required (VERSION 2.4)
project (tankfighter)
########## GENERAL PARAMETERS ##########
set (TF_NAME "tankfighter")
set (TF_VERSION 0.9.0)

# define SFML requirement
include(FindPkgConfig)

set (CMAKE_CXX_FLAGS_DEBUG "-Wall -g -O1 -std=c++0x")
set (CMAKE_C_FLAGS_DEBUG "-Wall -g -O1 -pedantic -std=c99")
set (CMAKE_CXX_FLAGS_RELEASE "-O2")
set (CMAKE_C_FLAGS_RELEASE "-O2")

############ SEARCH SFML2 ##############
if(PKG_CONFIG_FOUND)
	pkg_search_module(SFML2 sfml2-static >= 2.00)
endif(PKG_CONFIG_FOUND)

if(SFML2_FOUND)
else(SFML2_FOUND)
	pkg_search_module(SFML2 sfml-all >= 2.00)
endif(SFML2_FOUND)

if(SFML2_FOUND)
else(SFML2_FOUND)
	set(SFML_STATIC_LIBRARIES TRUE)
	find_package(SFML 2 COMPONENTS audio network graphics window system REQUIRED)
	set(SFML2_INCLUDE_DIRS ${SFML_INCLUDE_DIRS})
	set(SFML2_LIBRARIES ${SFML_LIBRARIES})
	set(SFML2_LDFLAGS_OTHER "")
	set(SFML2_CFLAGS_OTHER "")
	set(SFML2_LIBRARY_DIRS "")
endif(SFML2_FOUND)
########### SEARCH Fog SVG library #########
if(PKG_CONFIG_FOUND)
	pkg_search_module(FOG fog)
endif(PKG_CONFIG_FOUND)
if(FOG_FOUND)
	set(FOG_CFLAGS_OTHER "${FOG_CFLAGS_OTHER} -DWITH_FOG")
else(FOG_FOUND)
	set(FOG_INCLUDE_DIRS "")
	set(FOG_LIBRARY_DIRS "")
	set(FOG_LIBRARIES "")
	set(FOG_CFLAGS_OTHER "")
	set(FOG_LDFLAGS_OTHER "")
endif(FOG_FOUND)
########### SEARCH fontconfig2 #############
if(${CMAKE_SYSTEM_NAME} MATCHES "Win32")
else(${CMAKE_SYSTEM_NAME} MATCHES "Win32")
	pkg_search_module(FC2 REQUIRED fontconfig >= 2.00)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Win32")

########## SEARCH OpenGL #####################
if(PKG_CONFIG_FOUND)
	pkg_search_module(OPENGL gl)
endif(PKG_CONFIG_FOUND)

if (OPENGL_FOUND)
else(OPENGL_FOUND)
	find_package(OpenGL REQUIRED)
	set(OPENGL_LDFLAGS_OTHER "")
	set(OPENGL_CFLAGS_OTHER "")
	set(OPENGL_LIBRARY_DIRS "")
endif(OPENGL_FOUND)


include_directories (${SFML2_INCLUDE_DIRS} ${FC2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ${FOG_INCLUDE_DIRS})
link_directories (${SFML2_LIBRARY_DIRS} ${FC2_LIBRARY_DIRS} ${OPENGL_LIBRARY_DIRS} ${FOG_LIBRARY_DIRS})
message(STATUS "link_directories ${LINK_DIRECTORIES}")
# define main executable target
add_executable (tankfighter color.cpp image_helper.c parse_json.cpp fc.cpp input.cpp messages.cpp menu.cpp parameters.cpp network_controller.cpp ${CMAKE_BINARY_DIR}/keys.cpp commands.cpp joystick_controller.cpp missile.cpp engine_event.cpp controller.cpp keybmouse_controller.cpp player.cpp texture_cache.cpp entity.cpp wall.cpp load_map.cpp engine.cpp misc.cpp main.cpp json.c geometry.cpp)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/keys.cpp COMMAND ${CMAKE_SOURCE_DIR}/generate_keys.cpp.sh)

target_link_libraries (tankfighter ${SFML2_LIBRARIES} ${FC2_LIBRARIES} ${OPENGL_LIBRARIES} ${FOG_LIBRARIES})
string (REPLACE ";" " " X_LDFLAGS_OTHER "${SFML2_LDFLAGS_OTHER} ${FC2_LDFLAGS_OTHER} ${OPENGL_LDFLAGS_OTHER} ${FOG_LDFLAGS_OTHER}") # fix bug in cmake that make LDFLAGS semicolon separated on command line
set_property(TARGET tankfighter PROPERTY LINK_FLAGS "${LINK_FLAGS} ${X_LDFLAGS_OTHER}")
string (REPLACE ";" " " X_CFLAGS_OTHER "-DTF_NAME=\"\\\"${TF_NAME}\\\"\" -DTF_VERSION=\"\\\"${TF_VERSION}\\\"\" -DTF_PREFIX=\"\\\"${CMAKE_INSTALL_PREFIX}\"\\\" ${SFML2_CFLAGS_OTHER} ${FC2_CFLAGS_OTHER} ${OPENGL_CFLAGS_OTHER} ${FOG_CFLAGS_OTHER}")
set_property(TARGET tankfighter PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS} ${X_CFLAGS_OTHER}")

message(STATUS "cmake_system_name is ${CMAKE_SYSTEM_NAME}")

set (sprites sprites/crosshair.png sprites/car.png sprites/bloc.png sprites/bloc2.png sprites/canon.png sprites/bullet.png sprites/dirt.jpg)
set (datafiles keymap.json map.json map2.json tankfighter.cfg)
set (ddir /share/tankfighter-${TF_VERSION})
if(${CMAKE_SYSTEM_NAME} MATCHES "Win32")
	install (TARGETS tankfighter DESTINATION .)
else(${CMAKE_SYSTEM_NAME} MATCHES "Win32")
	install (TARGETS tankfighter DESTINATION bin)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Win32")
INSTALL_FILES(${ddir}/sprites FILES ${sprites})
INSTALL_FILES(${ddir} FILES ${datafiles})
