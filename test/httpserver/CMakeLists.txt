set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

set(HTTP_SERVER_EXEC_TARGET "http-server")
add_executable(${HTTP_SERVER_EXEC_TARGET} ${SRC_LIST})
target_link_libraries(${HTTP_SERVER_EXEC_TARGET} PUBLIC ${HTTP_TARGET} ${JSONRPC_TARGET} ${RPC_TARGET} ${WS_SERVICE_TARGET} bcos-framework::utilities Boost::unit_test_framework)
