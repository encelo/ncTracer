set(NCPROJECT_NAME "ncTracer")
set(NCPROJECT_EXE_NAME "nctracer")
set(NCPROJECT_VENDOR "Angelo Theodorou")
set(NCPROJECT_COPYRIGHT "Copyright ©2019-2021 ${NCPROJECT_VENDOR}")
set(NCPROJECT_DESCRIPTION "pmTracer front-end")
set(NCPROJECT_HOMEPAGE "https://ncine.github.io")
set(NCPROJECT_REVERSE_DNS "io.github.ncine.nctracer")
set(NCPROJECT_XDGDESKTOP_CATEGORIES "Education")
set(NCPROJECT_MACOS_CATEGORY "public.app-category.education")

set(NCPROJECT_INCLUDE_DIRS include)

set(NCPROJECT_SOURCES
	include/main.h
	include/shader_strings.h
	include/SceneContext.h
	include/VisualFeedback.h
	include/UserInterface.h
	include/ThreadManager.h
	include/ObjectsPool.h
	include/LuaSerializer.h

	src/main.cpp
	src/build_world.cpp
	src/shader_strings.cpp
	src/SceneContext.cpp
	src/VisualFeedback.cpp
	src/UserInterface.cpp
	src/ThreadManager.cpp
	src/ObjectsPool.cpp
	src/LuaSerializer.cpp
)

function(callback_before_target)
	get_filename_component(PARENT_DIRECTORY ${CMAKE_SOURCE_DIR} DIRECTORY)
	if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
		set(PMTRACER_ROOT "${PARENT_DIRECTORY}/pmTracer" CACHE PATH "The path to the pmTracer root directory")

		if(IS_DIRECTORY ${PMTRACER_ROOT}/cmake AND IS_DIRECTORY ${PMTRACER_ROOT}/include AND IS_DIRECTORY ${PMTRACER_ROOT}/src)
			file(COPY ${PMTRACER_ROOT}/cmake DESTINATION ${CMAKE_BINARY_DIR}/android/app/src/main/pmTracer)
			file(COPY ${PMTRACER_ROOT}/include DESTINATION ${CMAKE_BINARY_DIR}/android/app/src/main/pmTracer)
			file(COPY ${PMTRACER_ROOT}/src DESTINATION ${CMAKE_BINARY_DIR}/android/app/src/main/pmTracer)
		endif()
	else()
		set(PMTRACER_ROOT "${PARENT_DIRECTORY}/pmTracer")
	endif()

	include(${PMTRACER_ROOT}/cmake/pmtracer_sources.cmake)
	foreach(SOURCE ${PMTRACER_SOURCES})
		list(APPEND NCPROJECT_SOURCES "${PMTRACER_ROOT}/${SOURCE}")
	endforeach()

	include(${PMTRACER_ROOT}/cmake/pmtracer_headers.cmake)
	foreach(HEADER ${PMTRACER_HEADERS})
		list(APPEND NCPROJECT_SOURCES "${PMTRACER_ROOT}/${HEADER}")
	endforeach()

	set(NCPROJECT_SOURCES ${NCPROJECT_SOURCES} PARENT_SCOPE)

	list(APPEND NCPROJECT_INCLUDE_DIRS "${PMTRACER_ROOT}/include")
	set(NCPROJECT_INCLUDE_DIRS ${NCPROJECT_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()

function(callback_after_target)
	target_compile_features(${NCPROJECT_EXE_NAME} PUBLIC cxx_std_14)

	if(WIN32)
		target_compile_definitions(${NCPROJECT_EXE_NAME} PRIVATE "WITH_GLEW")
		if(MSVC)
			target_include_directories(${NCPROJECT_EXE_NAME} PRIVATE ${EXTERNAL_MSVC_DIR}/include)
		endif()
	endif()
endfunction()

# Don't edit beyond this line
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	return()
endif()

cmake_minimum_required(VERSION 3.13)
project(${NCPROJECT_NAME})

find_path(NCPROJECT_DIR cmake/project_main.cmake
	PATHS ${nCine_DIR} $ENV{NCINE_DIR}
	PATH_SUFFIXES project ../project ../../../project ../../../share/ncine/project
	NO_CMAKE_FIND_ROOT_PATH # For Emscripten
	DOC "Path to the nCine template project scripts")

if(IS_DIRECTORY ${NCPROJECT_DIR})
	include(${NCPROJECT_DIR}/cmake/project_main.cmake)
else()
	if(NOT DEFINED nCine_DIR AND "$ENV{NCINE_DIR}" STREQUAL "")
		message(FATAL_ERROR "Set the nCine_DIR CMake variable or the NCINE_DIR environment variable to a valid nCine directory.")
	elseif(DEFINED nCine_DIR)
		message(FATAL_ERROR "The nCine_DIR CMake variable \"${nCine_DIR}\" is not a valid nCine directory.")
	elseif(NOT "$ENV{NCINE_DIR}" STREQUAL "")
		message(FATAL_ERROR "The NCINE_DIR environment variable \"$ENV{NCINE_DIR}\" is not a valid nCine directory.")
	endif()
endif()
