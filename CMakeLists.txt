cmake_minimum_required(VERSION 3.10)

project(RTSPServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_BUILD_TYPE DEBUG)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

file(GLOB SRC_NETWORK src/Network/*.cpp)
file(GLOB SRC_THREAD src/Thread/*.cpp)
file(GLOB SRC_EVENT src/Event/*.cpp)

add_executable(${PROJECT_NAME}
    ${SRC_NETWORK}
    ${SRC_THREAD}
    ${SRC_EVENT}
    src/main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    src/Network
    src/Thread
    src/Event
)

target_link_libraries(${PROJECT_NAME}
    ws2_32
)

# target_link_libraries(${PROJECT_NAME}
#     pthread
# )