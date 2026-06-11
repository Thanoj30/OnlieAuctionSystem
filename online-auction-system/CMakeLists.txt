cmake_minimum_required(VERSION 3.15)
project(OnlineAuctionSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(online_auction main.cpp)

find_package(Threads REQUIRED)

# Crow is header-only. This first supports package-manager installs that provide
# CrowConfig.cmake, then falls back to looking for crow.h in common include paths.
find_package(Crow CONFIG QUIET)

if (Crow_FOUND)
    target_link_libraries(online_auction PRIVATE Crow::Crow Threads::Threads)
else()
    find_path(CROW_INCLUDE_DIR
        NAMES crow.h
        PATHS
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            /opt/homebrew/include
            /usr/local/include
            /usr/include
    )

    if (NOT CROW_INCLUDE_DIR)
        message(FATAL_ERROR
            "Crow was not found.\n"
            "Install it on macOS with:\n"
            "  brew install crow\n"
            "Then run CMake again from the project folder:\n"
            "  cmake -S . -B build\n"
        )
    endif()

    find_path(ASIO_INCLUDE_DIR
        NAMES asio.hpp
        PATHS
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            /opt/homebrew/include
            /usr/local/include
            /usr/include
    )

    if (ASIO_INCLUDE_DIR)
        target_include_directories(online_auction PRIVATE ${CROW_INCLUDE_DIR} ${ASIO_INCLUDE_DIR})
    else()
        target_include_directories(online_auction PRIVATE ${CROW_INCLUDE_DIR})
        message(WARNING
            "asio.hpp was not found in common include paths. If compilation fails, install Asio with:\n"
            "  brew install asio\n"
        )
    endif()

    target_link_libraries(online_auction PRIVATE Threads::Threads)
endif()

target_compile_definitions(online_auction PRIVATE PROJECT_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}\")

message(STATUS "Online Auction server will run at http://localhost:18080")
