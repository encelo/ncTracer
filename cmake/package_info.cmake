set(PACKAGE_NAME "ncTracer")
set(PACKAGE_EXE_NAME "nctracer")
set(PACKAGE_VENDOR "Angelo Theodorou")
set(PACKAGE_COPYRIGHT "Copyright ©2019-2020 ${PACKAGE_VENDOR}")
set(PACKAGE_DESCRIPTION "pmTracer front-end")
set(PACKAGE_HOMEPAGE "https://ncine.github.io")
set(PACKAGE_REVERSE_DNS "io.github.ncine.nctracer")

set(PACKAGE_INCLUDE_DIRS include)

set(PACKAGE_SOURCES
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
			file(COPY ${PMTRACER_ROOT}/cmake DESTINATION ${CMAKE_BINARY_DIR}/android/src/main/pmTracer)
			file(COPY ${PMTRACER_ROOT}/include DESTINATION ${CMAKE_BINARY_DIR}/android/src/main/pmTracer)
			file(COPY ${PMTRACER_ROOT}/src DESTINATION ${CMAKE_BINARY_DIR}/android/src/main/pmTracer)
		endif()
	else()
		set(PMTRACER_ROOT "${PARENT_DIRECTORY}/pmTracer")
	endif()

	include(${PMTRACER_ROOT}/cmake/pmtracer_sources.cmake)
	foreach(SOURCE ${PMTRACER_SOURCES})
		list(APPEND PACKAGE_SOURCES "${PMTRACER_ROOT}/${SOURCE}")
	endforeach()

	include(${PMTRACER_ROOT}/cmake/pmtracer_headers.cmake)
	foreach(HEADER ${PMTRACER_HEADERS})
		list(APPEND PACKAGE_SOURCES "${PMTRACER_ROOT}/${HEADER}")
	endforeach()

	set(PACKAGE_SOURCES ${PACKAGE_SOURCES} PARENT_SCOPE)

	list(APPEND PACKAGE_INCLUDE_DIRS "${PMTRACER_ROOT}/include")
	set(PACKAGE_INCLUDE_DIRS ${PACKAGE_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()

function(callback_after_target)
	target_compile_features(${PACKAGE_EXE_NAME} PUBLIC cxx_std_14)

	if(MSVC)
		target_compile_definitions(${PACKAGE_EXE_NAME} PRIVATE "WITH_GLEW")
		target_include_directories(${PACKAGE_EXE_NAME} PRIVATE ${EXTERNAL_MSVC_DIR}/include)
	endif()
endfunction()
