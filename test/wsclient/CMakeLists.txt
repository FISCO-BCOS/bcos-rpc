set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_executable(broadcast-sub-client broadcast_sub_client.cpp)
target_link_libraries(broadcast-sub-client PUBLIC ${HTTP_TARGET} ${JSONRPC_TARGET} ${RPC_TARGET} ${WS_SERVICE_TARGET} bcos-framework::utilities)

add_executable(broadcast-pub-client broadcast_pub_client.cpp)
target_link_libraries(broadcast-pub-client PUBLIC ${HTTP_TARGET} ${JSONRPC_TARGET} ${RPC_TARGET} ${WS_SERVICE_TARGET} bcos-framework::utilities)
