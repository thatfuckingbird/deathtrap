cmake_minimum_required(VERSION 3.14)

project(deathtrap LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core WebSockets Test Sql REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core WebSockets Test Sql REQUIRED)
find_package(OpenSSL)

include_directories(src)
include_directories(3rdparty/mongoose)

add_definitions(-DMG_ENABLE_SSL -DMG_SSL_IF=MG_SSL_IF_OPENSSL)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_library(mongoose
    3rdparty/mongoose/mongoose.h
    3rdparty/mongoose/mongoose.c
)

add_library(deathtrap
    src/database.cpp
    src/database.h
    src/wsmessage.h
    src/wsclient.cpp
    src/wsclient.h
)

add_executable(deathtrap-api-server
    src/wsserver.cpp
    src/wsserver.h
    src/apiserver_main.cpp
)

add_executable(deathtrap-file-server
    src/fileserver_main.cpp
)

add_executable(deathtrap-tests
    test/main.h
    test/main.cpp
)

target_link_libraries(deathtrap PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(deathtrap-api-server PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql deathtrap)
target_link_libraries(deathtrap-file-server PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql deathtrap mongoose OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(deathtrap-tests PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql deathtrap)
