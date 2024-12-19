cmake_minimum_required(VERSION 3.20)
project(mexc_api)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_definitions(-D_WIN32_WINNT=0x0A00 /bigobj)
endif ()

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif ()

find_package(Boost 1.78 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(magic_enum REQUIRED)

include_directories(include vk_cpp_tools/include SYSTEM ${Boost_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})
add_subdirectory(vk_cpp_tools)

set(HEADERS
        include/vk/mexc/mexc.h
        include/vk/mexc/mexc_enums.h
        include/vk/mexc/mexc_http_session.h
        include/vk/mexc/mexc_models.h
        include/vk/mexc/mexc_spot_rest_client.h
)

set(SOURCES
        src/mexc.cpp
        src/mexc_http_session.cpp
        src/mexc_models.cpp
        src/mexc_spot_rest_client.cpp
)

add_library(mexc_api OBJECT ${SOURCES} ${HEADERS})
target_link_libraries(mexc_api PRIVATE OpenSSL::Crypto OpenSSL::SSL vk_tools nlohmann_json::nlohmann_json)

add_executable(test tests/main.cpp)
target_link_libraries(test PRIVATE spdlog::spdlog_header_only mexc_api)
