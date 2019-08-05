set(PACKAGE_NAME "ncTracer")
set(PACKAGE_EXE_NAME "nctracer")
set(PACKAGE_VENDOR "Angelo Theodorou")
set(PACKAGE_DESCRIPTION "pmTracer front-end")
set(PACKAGE_HOMEPAGE "https://ncine.github.io")
set(PACKAGE_REVERSE_DNS "io.github.ncine.nctracer")

set(PACKAGE_INCLUDE_DIR include)

set(PACKAGE_SOURCES
	include/main.h
	include/shader_strings.h
	include/SceneContext.h
	include/VisualFeedback.h
	include/UserInterface.h
	include/ThreadManager.h

	src/main.cpp
	src/shader_strings.cpp
	src/SceneContext.cpp
	src/VisualFeedback.cpp
	src/UserInterface.cpp
	src/ThreadManager.cpp
)

function(callback_after_target)
	set(PMTRACER_INCLUDE_DIR "/home/encelo/pmTracer/include" CACHE PATH "Set the path to the pmTracer include directory")
	set(PMTRACER_LIBRARY "/home/encelo/pmTracer-release/libpmTracer.a" CACHE PATH "Set the path to the pmTracer library")
	target_include_directories(${PACKAGE_EXE_NAME} PRIVATE ${PMTRACER_INCLUDE_DIR})
	target_link_libraries(${PACKAGE_EXE_NAME} PRIVATE ${PMTRACER_LIBRARY})
endfunction()
