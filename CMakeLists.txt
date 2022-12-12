cmake_minimum_required(VERSION 3.13)

project(hg_cpp_tutorial VERSION 0.0.1 LANGUAGES CXX C)

include(GetPrerequisites)

set(CMAKE_CXX_STANDARD 14)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(HG_CPPSDK_PATH CACHE PATH "Path to the Harfang C++ SDK" )
get_filename_component(HG_CPPSDK_PATH ${HG_CPPSDK_PATH} ABSOLUTE)

set(HG_ASSETC_PATH CACHE PATH "Path to the Harfang Assets Compiler" )
get_filename_component(HG_ASSETC_PATH ${HG_ASSETC_PATH} ABSOLUTE)

set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
if(NOT APPLE)
        set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
	set(CMAKE_INSTALL_RPATH $ORIGIN)
else()
	set(MACOSX_RPATH TRUE)
	set(CMAKE_INSTALL_RPATH @loader_path)
endif()

find_package(harfang REQUIRED
    COMPONENTS cppsdk
    PATHS ${HG_CPPSDK_PATH}
    NO_DEFAULT_PATH
)
	
# Resources compilation
add_custom_target(resources ALL
	COMMAND ${HG_ASSETC_PATH}/assetc ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources_compiled -api ${HG_GRAPHIC_API} -t ${HG_ASSETC_PATH}/toolchains/host-${HG_HOST_PREFIX}-target-${HG_TARGET_PREFIX}
	WORKING_DIRECTORY ${HG_ASSETC_PATH}
	COMMENT "Build assets"
)

# Basic loop 
add_executable(basic_loop basic_loop.cpp)
target_link_libraries(basic_loop hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(basic_loop PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(basic_loop pthread)
endif()

# Controllers
add_executable(game_mouse_flight game_mouse_flight.cpp)
target_link_libraries(game_mouse_flight hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(game_mouse_flight PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(game_mouse_flight pthread)
endif()

# Nodes
add_executable(material_update_value material_update_value.cpp)
target_link_libraries(material_update_value hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(material_update_value PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(material_update_value pthread)
endif()

# Nodes
add_executable(scene_many_nodes scene_many_nodes.cpp)
target_link_libraries(scene_many_nodes hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(scene_many_nodes PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(scene_many_nodes pthread)
endif()

# Scene instances
add_executable(scene_instances scene_instances.cpp)
target_link_libraries(scene_instances hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(scene_instances PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(scene_instances pthread)
endif()

# Scene draw to texture
add_executable(scene_draw_to_texture scene_draw_to_texture.cpp)
target_link_libraries(scene_draw_to_texture hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(scene_draw_to_texture PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(scene_draw_to_texture pthread)
endif()
	
# Scene "AAA" render
add_executable(scene_aaa scene_aaa.cpp)
target_link_libraries(scene_aaa hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(scene_aaa PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(scene_aaa pthread)
endif()

# VR scene
add_executable(scene_vr scene_vr.cpp)
target_link_libraries(scene_vr hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(scene_vr PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(scene_vr pthread)
endif()

# Physics
add_executable(physics_pool_of_objects physics_pool_of_objects.cpp)
target_link_libraries(physics_pool_of_objects hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(physics_pool_of_objects PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(physics_pool_of_objects pthread)
endif()

# ImGUI basic
add_executable(imgui_basic imgui_basic.cpp)
target_link_libraries(imgui_basic hg::engine hg::foundation hg::platform)
if(WIN32)
	set_target_properties(imgui_basic PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
elseif(UNIX)
	target_link_libraries(imgui_basic pthread)
endif()

# install binary, runtime dependencies and data dependencies
install(TARGETS basic_loop game_mouse_flight scene_many_nodes scene_instances physics_pool_of_objects imgui_basic scene_aaa material_update_value scene_vr DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources_compiled/ DESTINATION bin/resources_compiled)

install_cppsdk_dependencies(bin dep)

