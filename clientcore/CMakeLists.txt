cmake_minimum_required(VERSION 3.20)

project(Fusion32Protocol772Core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(protocol772_transport
    src/framed_connection.cpp
    src/framing.cpp
    src/tcp_transport.cpp
)

target_include_directories(protocol772_transport
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(protocol772_transport PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(protocol772_transport PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(WIN32)
    target_link_libraries(protocol772_transport PUBLIC ws2_32)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Threads REQUIRED)

    add_executable(protocol772_transport_tests
        tests/transport_tests.cpp
    )
    target_link_libraries(protocol772_transport_tests
        PRIVATE
            protocol772_transport
            Threads::Threads
    )

    if(MSVC)
        target_compile_options(protocol772_transport_tests PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(protocol772_transport_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_test(NAME protocol772_transport_tests COMMAND protocol772_transport_tests)
    set_tests_properties(protocol772_transport_tests PROPERTIES TIMEOUT 30)
endif()
