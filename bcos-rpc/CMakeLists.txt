
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

aux_source_directory(. SRC_LIST)
aux_source_directory(./event SRC_LIST)
aux_source_directory(./jsonrpc SRC_LIST)
aux_source_directory(./amop SRC_LIST)

add_library(${RPC_TARGET} ${SRC_LIST} ${HEADERS})
target_link_libraries(${RPC_TARGET} PUBLIC jsoncpp_lib_static bcos-framework::utilities tarscpp::tarsservant bcos-boostssl::boostssl-websocket bcos-boostssl::boostssl-httpserver bcos-boostssl::boostssl-context OpenSSL::SSL OpenSSL::Crypto)