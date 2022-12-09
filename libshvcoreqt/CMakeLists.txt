qt_add_library(libshvcoreqt SHARED
    src/data/valuechange.cpp
    src/exception.cpp
    src/log.cpp
	src/utils.cpp
	src/rpc.cpp
	src/utils/clioptions.cpp
    src/utils/versioninfo.cpp
    )
target_link_libraries(libshvcoreqt PUBLIC Qt::Core libshvcore)
target_include_directories(libshvcoreqt PUBLIC include)
target_compile_definitions(libshvcoreqt PRIVATE SHVCOREQT_BUILD_DLL)

if(WIN32)
    # Need these for some WinAPI calls
    target_compile_definitions(libshvcoreqt PRIVATE UNICODE)
    target_compile_definitions(libshvcoreqt PRIVATE _UNICODE)
endif()

function(add_shvcoreqt_test test_name)
	add_executable(test_${test_name}
		tests/test_${test_name}.cpp
		)
	target_compile_definitions(test_${test_name} PRIVATE DEF_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/files")
	target_link_libraries(test_${test_name} libshvcoreqt)
	add_test(NAME test_${test_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_${test_name})
endfunction(add_shvcoreqt_test)

if(BUILD_TESTING)
	add_shvcoreqt_test(rpc_variant)
endif()

install(TARGETS libshvcoreqt)
