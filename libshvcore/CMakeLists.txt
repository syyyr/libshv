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
