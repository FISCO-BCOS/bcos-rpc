set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_library(${JSONRPC_TARGET} ${SRC_LIST})
target_link_libraries(${JSONRPC_TARGET} PUBLIC bcos-framework::utilities)