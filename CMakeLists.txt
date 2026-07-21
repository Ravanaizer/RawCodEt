cmake_minimum_required(VERSION 3.16)
project(RawCodEt VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

find_package(Qt6 COMPONENTS Core Network Sql Widgets WebEngineWidgets QUIET)
if(NOT Qt6_FOUND)
  find_package(Qt5 REQUIRED COMPONENTS Core Network Sql Widgets WebEngineWidgets)
endif()

add_subdirectory(protocol)
add_subdirectory(client)
add_subdirectory(server)
