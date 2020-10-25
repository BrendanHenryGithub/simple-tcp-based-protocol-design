cmake_minimum_required(VERSION 3.16.0)
project(interview)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(glog REQUIRED)

add_executable(test
    src/protocol.cpp
    test/test.cpp
)
target_link_libraries(test
    PRIVATE ${GTEST_LIBRARIES}
    PRIVATE Threads::Threads
    PRIVATE glog::glog
)
target_include_directories(test
    PRIVATE inc/
)

add_executable(client
    src/protocol.cpp
    src/client.cpp
    src/client_program.cpp
)
target_link_libraries(client
    PRIVATE Threads::Threads
    PRIVATE glog::glog
)
target_include_directories(client
    PRIVATE inc/
)

add_executable(server
    src/protocol.cpp
    src/server.cpp
    src/server_program.cpp
)
target_link_libraries(server
    PRIVATE Threads::Threads
    PRIVATE glog::glog
)
target_include_directories(server
    PRIVATE inc/
)