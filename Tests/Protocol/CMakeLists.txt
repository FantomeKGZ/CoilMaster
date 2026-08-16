cmake_minimum_required(VERSION 3.16)
project(CMPProtocolTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cmp_protocol_tests
    test_main.cpp
    ../../Shared/Protocol/CMP_Buffer.cpp
    ../../Shared/Protocol/CMP_CRC.cpp
    ../../Shared/Protocol/CMP_Dispatcher.cpp
    ../../Shared/Protocol/CMP_Parser.cpp
    ../../Shared/Protocol/CMP_Protocol.cpp
)

add_executable(cmp1_text_tests
    ../CMP1Text/test_main.cpp
)

target_include_directories(cmp_protocol_tests PRIVATE
    ../../Shared/Protocol
)

target_include_directories(cmp1_text_tests PRIVATE
    ../../Shared/CMP1Text
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cmp_protocol_tests PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
    target_compile_options(cmp1_text_tests PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
endif()

enable_testing()
add_test(NAME cmp_protocol_tests COMMAND cmp_protocol_tests)
add_test(NAME cmp1_text_tests COMMAND cmp1_text_tests)
