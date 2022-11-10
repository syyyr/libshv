cmake_minimum_required(VERSION 3.18.4)

set(WITH_SAMPLES OFF CACHE BOOL "Enable build of GUI samples")

project(libshv LANGUAGES C CXX)
include(CTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_SHARED_LIBRARY_PREFIX "") # we don't want CMake to prepend "lib" to our libraries, we prefer adding that ourselves
set(CMAKE_STATIC_LIBRARY_PREFIX "")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(SHV_WARNING_FLAGS "-Wall -Wextra -pedantic -Wshadow -Wcast-align -Wunused -Wpedantic -Wconversion -Wmisleading-indentation -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough")
	set(SHV_WARNING_FLAGS "${SHV_WARNING_FLAGS} -Wno-sign-conversion")
	set(SHV_WARNING_FLAGS_CXX "-Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(SHV_WARNING_FLAGS "-Wduplicated-cond -Wduplicated-branches -Wlogical-op ${SHV_WARNING_FLAGS}")
		set(SHV_WARNING_FLAGS_CXX "-Wuseless-cast ${SHV_WARNING_FLAGS_CXX}")

	endif()

	set(SHV_WARNING_FLAGS_CXX "${SHV_WARNING_FLAGS} ${SHV_WARNING_FLAGS_CXX}")

	set(CMAKE_C_FLAGS "${SHV_WARNING_FLAGS} ${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${SHV_WARNING_FLAGS_CXX} ${CMAKE_CXX_FLAGS}")
endif()

if(BUILD_TESTING)
	find_package(doctest QUIET)
	if(NOT doctest_FOUND)
		message(STATUS "doctest library NOT found, disabling tests")
		set(BUILD_TESTING OFF)
	endif()
endif()

if (NOT TARGET libnecrolog)
	add_subdirectory(3rdparty/necrolog)
endif()

add_subdirectory(libshvchainpack/c)
add_subdirectory(libshvchainpack)
add_subdirectory(libshvcore)

if(USE_QT6)
	message(STATUS "Enabling Qt6 support")
	if(EMSCRIPTEN)
		find_package(Qt6 COMPONENTS Core Network OPTIONAL_COMPONENTS WebSockets Widgets)
	else()
		find_package(Qt6 COMPONENTS Core Network Sql SerialPort OPTIONAL_COMPONENTS WebSockets Widgets)
	endif()
	set(QtWebSockets_FOUND ${Qt6WebSockets_FOUND})
	set(Qt_FOUND ${Qt6_FOUND})
	set(QtWidgets_FOUND ${Qt6Widgets_FOUND})
else()
	find_package(Qt5 COMPONENTS Core Network Sql SerialPort OPTIONAL_COMPONENTS WebSockets Widgets)
	set(QtWebSockets_FOUND ${Qt5WebSockets_FOUND})
	set(Qt_FOUND ${Qt5_FOUND})
	set(QtWidgets_FOUND ${Qt5Widgets_FOUND})

	function(qt_add_library)
		add_library(${ARGV})
	endfunction()

	function(qt_add_executable)
		add_executable(${ARGV})
	endfunction()
endif()

# We'll error out if the user has explicitly enabled websocket support, but Qt WebSockets aren't available.
if(WITH_SHV_WEBSOCKETS AND NOT QtWebSockets_FOUND)
	message(FATAL_ERROR "SHV websocket support explicitly enabled, but Qt WebSockets weren't found!")
endif()

# Otherwise enable websocket support based on whether we found Qt6 WebSockets.
if (NOT DEFINED WITH_SHV_WEBSOCKETS)
	set(WITH_SHV_WEBSOCKETS ${QtWebSockets_FOUND})
endif()

if (WITH_SHV_WEBSOCKETS)
	message(STATUS "SHV websocket support enabled")
else()
	message(STATUS "SHV websocket support disabled")
endif()

if(Qt_FOUND)
	set(CMAKE_AUTOMOC ON)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(CMAKE_AUTOMOC_MOC_OPTIONS "-b${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt_moc_disable_warnings.h")
	endif()

	add_subdirectory(libshvcoreqt)
	add_subdirectory(libshviotqt)
	if(NOT EMSCRIPTEN)
		add_subdirectory(libshvbroker)
	endif()

	if(QtWidgets_FOUND)
		set(CMAKE_AUTORCC ON)
		set(CMAKE_AUTOUIC ON)
		add_subdirectory(libshvvisu)
		if(WITH_SAMPLES)
			message(STATUS "samplegraph application will be built")
			add_subdirectory(samples/gui/samplegraph)
		endif()
	else()
		message(STATUS "Qt Widgets not found, libshvvisu won't be built")
	endif()

else()
	message(STATUS "Qt not found, Qt-based libs won't be built")
endif()

option(BUILD_SHV_UTILS "Whether to build ccp2cp, cp2cp, and cpmerge" ON)

# Utils
if(BUILD_SHV_UTILS)
	message(STATUS "SHV utils will be built")
	# ccp2cp
	add_executable(ccp2cp utils/ccp2cp/main.c)
	target_link_libraries(ccp2cp libshvchainpack-c)

	# cp2cp
	add_executable(cp2cp utils/cp2cp/main.cpp)
	target_link_libraries(cp2cp libshvchainpack-cpp)

	# cpmerge
	add_executable(cpmerge utils/cpmerge/main.cpp)
	target_link_libraries(cpmerge libshvchainpack-cpp)

	install(TARGETS ccp2cp cp2cp cpmerge)
else()
	message(STATUS "SHV utils WON'T be built")
endif()
