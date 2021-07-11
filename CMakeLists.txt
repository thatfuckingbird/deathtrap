cmake_minimum_required(VERSION 3.20)

project(hyfileaccess LANGUAGES CXX C)

find_package(Qt5 COMPONENTS Core Sql REQUIRED)
find_package(OpenSSL)

add_definitions(-DMG_ENABLE_SSL -DMG_SSL_IF=MG_SSL_IF_OPENSSL)

set(CMAKE_AUTOMOC ON)

add_library(mongoose
    src/3rdparty/mongoose/mongoose.h
    src/3rdparty/mongoose/mongoose.c
)
target_compile_definitions(mongoose PUBLIC MG_ENABLE_SSL MG_SSL_IF=MG_SSL_IF_OPENSSL)
target_link_libraries(mongoose OpenSSL::SSL OpenSSL::Crypto)

add_library(hyfileaccess
    src/database.cpp
    include/hyfileaccess/database.h
)
target_include_directories(hyfileaccess PUBLIC include)
set_target_properties(hyfileaccess PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_link_libraries(hyfileaccess PRIVATE Qt5::Core Qt5::Sql)
add_library(hyfileaccess::hyfileaccess ALIAS hyfileaccess)

add_executable(hyfileaccess-server
    src/fileserver_main.cpp
)
set_target_properties(hyfileaccess-server PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_link_libraries(hyfileaccess-server PRIVATE Qt5::Core Qt5::Sql hyfileaccess::hyfileaccess mongoose)
