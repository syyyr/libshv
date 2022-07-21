add_library(libshvchainpack-cpp SHARED
    src/chainpack/abstractstreamreader.cpp
    src/chainpack/abstractstreamwriter.cpp
    src/chainpack/accessgrant.cpp
    src/chainpack/chainpack.cpp
    src/chainpack/chainpackreader.cpp
    src/chainpack/chainpackreader1.cpp
    src/chainpack/chainpackwriter.cpp
    src/chainpack/cpon.cpp
    src/chainpack/cponreader.cpp
    src/chainpack/cponwriter.cpp
    src/chainpack/datachange.cpp
    src/chainpack/exception.cpp
    src/chainpack/irpcconnection.cpp
    src/chainpack/metamethod.cpp
    src/chainpack/metatypes.cpp
    src/chainpack/rpc.cpp
    src/chainpack/rpcdriver.cpp
    src/chainpack/rpcmessage.cpp
    src/chainpack/rpcvalue.cpp
    src/chainpack/socketrpcdriver.cpp
    src/chainpack/tunnelctl.cpp
    src/chainpack/utils.cpp
    )

target_link_libraries(libshvchainpack-cpp libnecrolog libshvchainpack-c)
target_include_directories(libshvchainpack-cpp PUBLIC include)

install(TARGETS libshvchainpack-cpp)
