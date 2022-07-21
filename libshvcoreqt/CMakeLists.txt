add_library(libshvcoreqt SHARED
    src/data/valuechange.cpp
    src/exception.cpp
    src/log.cpp
    src/utils.cpp
    src/utils/clioptions.cpp
    src/utils/versioninfo.cpp
    )
target_link_libraries(libshvcoreqt PUBLIC Qt::Core libshvcore)
target_include_directories(libshvcoreqt PUBLIC include)

install(TARGETS libshvcoreqt)
