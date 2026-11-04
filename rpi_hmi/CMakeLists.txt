cmake_minimum_required(VERSION 3.16)

project(rpi_esp_hmi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Qml Quick Network QuickControls2)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Qml Quick Network QuickControls2)

set(SOURCES
    src/main.cpp
    src/mqttclient.h
    src/mqttclient.cpp
    resources.qrc
)

add_executable(rpi_esp_hmi ${SOURCES})

target_link_libraries(rpi_esp_hmi PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::QuickControls2
)

install(TARGETS rpi_esp_hmi DESTINATION bin)
