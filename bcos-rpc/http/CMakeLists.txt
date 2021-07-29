set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

add_subdirectory(ws)

file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

add_library(${HTTP_TARGET} ${SRC_LIST})
target_link_libraries(${HTTP_TARGET} PUBLIC bcos-framework::utilities)