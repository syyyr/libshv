add_library(libshvcore SHARED
    src/exception.cpp
    src/string.cpp
    src/stringview.cpp
    src/utils.cpp
    src/utils/abstractshvjournal.cpp
    src/utils/clioptions.cpp
    src/utils/crypt.cpp
    src/utils/patternmatcher.cpp
    src/utils/shvalarm.cpp
    src/utils/shvfilejournal.cpp
    src/utils/shvgetlogparams.cpp
    src/utils/shvjournalcommon.cpp
    src/utils/shvjournalentry.cpp
    src/utils/shvjournalfilereader.cpp
    src/utils/shvjournalfilewriter.cpp
    src/utils/shvlogfilereader.cpp
    src/utils/shvlogfilter.cpp
    src/utils/shvlogheader.cpp
    src/utils/shvlogrpcvaluereader.cpp
    src/utils/shvmemoryjournal.cpp
    src/utils/shvpath.cpp
    src/utils/shvtypeinfo.cpp
    src/utils/shvurl.cpp
    src/utils/versioninfo.cpp
    )

target_link_libraries(libshvcore libshvchainpack-cpp)
target_include_directories(libshvcore PUBLIC include)
target_compile_definitions(libshvcore PRIVATE SHVCORE_BUILD_DLL)

function(add_shv_test test_name)
	add_compile_definitions(DEF_FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/files")
	add_executable(test_${test_name}
		tests/test_${test_name}.cpp
		#tests/${test_name}.qrc
		)
	target_link_libraries(test_${test_name} libshvcore)
	add_test(NAME test_${test_name} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_${test_name})
endfunction(add_shv_test)

if(BUILD_TESTING)
	add_shv_test(shvtypeinfo)
	add_shv_test(stringview)
	add_shv_test(shvpath)
endif()

install(TARGETS libshvcore)
