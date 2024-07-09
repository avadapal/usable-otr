cmake_minimum_required(VERSION 3.28)
project(denim_project)

set(CMAKE_CXX_STANDARD 20)


find_package(Boost 1.85.0 REQUIRED COMPONENTS filesystem system serialization)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost not found")
endif()

find_package(SQLite3 REQUIRED)
if(SQLite3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "SQLite3 not found")
endif()

find_package(Botan 3.4.0 REQUIRED)
if(Botan_FOUND)
    include_directories(${Botan_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Botan not found")
endif()


add_executable(denim src/denim.cpp)

target_link_libraries(denim Boost::system Boost::filesystem Boost::serialization SQLite::SQLite3 Botan::Botan)
target_include_directories(denim PRIVATE ${CMAKE_SOURCE_DIR})
