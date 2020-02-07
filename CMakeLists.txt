cmake_minimum_required(VERSION 3.15)
project(cppSRTFramingExample)

set(CMAKE_CXX_STANDARD 17)

include(ExternalProject)
ExternalProject_Add(project_efp
        GIT_REPOSITORY https://bitbucket.org/unitxtra/efp.git
        GIT_SUBMODULES ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/efp
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/efp
        GIT_PROGRESS 1
        BUILD_COMMAND make efp
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        INSTALL_COMMAND ""
        )
add_library(efp STATIC IMPORTED)
set_property(TARGET efp PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/efp/libefp.a)

ExternalProject_Add(project_cppsrtwrapp
        GIT_REPOSITORY https://github.com/andersc/cppSRTWrapper.git
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/srtwrap
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/srtwrap
        GIT_PROGRESS 1
        BUILD_COMMAND make srtnet
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        INSTALL_COMMAND ""
        )
add_library(srtnet STATIC IMPORTED)
set_property(TARGET srtnet PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/srtwrap/libsrtnet.a)

find_package (Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/srtwrap/srt/)

add_executable(cppSRTFramingClient client.cpp)
add_dependencies(cppSRTFramingClient project_efp)
add_dependencies(cppSRTFramingClient project_cppsrtwrapp)
target_link_libraries(cppSRTFramingClient efp srtnet ${CMAKE_CURRENT_SOURCE_DIR}/srtwrap/srt/libsrt.a ${OPENSSL_LIBRARIES} Threads::Threads)

add_executable(cppSRTFramingServer server.cpp)
add_dependencies(cppSRTFramingClient project_efp)
add_dependencies(cppSRTFramingClient project_cppsrtwrapp)
target_link_libraries(cppSRTFramingServer efp srtnet ${CMAKE_CURRENT_SOURCE_DIR}/srtwrap/srt/libsrt.a ${OPENSSL_LIBRARIES} Threads::Threads)

