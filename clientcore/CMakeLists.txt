cmake_minimum_required(VERSION 3.20)

project(Fusion32Protocol772Core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED COMPONENTS Crypto)

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

add_library(protocol772_crypto
    src/crypto.cpp
)

target_include_directories(protocol772_crypto
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(protocol772_crypto
    PRIVATE
        OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(protocol772_crypto PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(protocol772_crypto PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(WIN32)
    target_link_libraries(protocol772_crypto PRIVATE bcrypt)
endif()

add_library(protocol772_login
    src/login.cpp
)

target_include_directories(protocol772_login
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(protocol772_login
    PUBLIC
        protocol772_crypto
        protocol772_transport
)

if(MSVC)
    target_compile_options(protocol772_login PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(protocol772_login PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_library(protocol772_gamelogin
    src/gamelogin.cpp
)

target_include_directories(protocol772_gamelogin
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(protocol772_gamelogin
    PUBLIC
        protocol772_login
        protocol772_crypto
        protocol772_transport
)

if(MSVC)
    target_compile_options(protocol772_gamelogin PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(protocol772_gamelogin PRIVATE -Wall -Wextra -Wpedantic -Werror)
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

    add_executable(protocol772_crypto_tests
        tests/crypto_tests.cpp
    )
    target_link_libraries(protocol772_crypto_tests
        PRIVATE
            protocol772_crypto
            protocol772_transport
    )

    if(MSVC)
        target_compile_options(protocol772_crypto_tests PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(protocol772_crypto_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_test(NAME protocol772_crypto_tests COMMAND protocol772_crypto_tests)
    set_tests_properties(protocol772_crypto_tests PROPERTIES TIMEOUT 30)

    add_executable(protocol772_login_tests
        tests/login_tests.cpp
    )
    target_link_libraries(protocol772_login_tests
        PRIVATE
            protocol772_login
            protocol772_crypto
            protocol772_transport
    )

    if(MSVC)
        target_compile_options(protocol772_login_tests PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(protocol772_login_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_test(NAME protocol772_login_tests COMMAND protocol772_login_tests)
    set_tests_properties(protocol772_login_tests PROPERTIES TIMEOUT 30)

    add_executable(protocol772_gamelogin_tests
        tests/gamelogin_tests.cpp
    )
    target_link_libraries(protocol772_gamelogin_tests
        PRIVATE
            protocol772_gamelogin
            protocol772_login
            protocol772_crypto
            protocol772_transport
    )

    if(MSVC)
        target_compile_options(protocol772_gamelogin_tests PRIVATE /W4 /WX /permissive-)
    else()
        target_compile_options(protocol772_gamelogin_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_test(NAME protocol772_gamelogin_tests COMMAND protocol772_gamelogin_tests)
    set_tests_properties(protocol772_gamelogin_tests PROPERTIES TIMEOUT 30)
endif()
