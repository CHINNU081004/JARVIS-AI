cmake_minimum_required(VERSION 3.20)
project(JARVIS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Sql
    Test
)

# Core Library
set(JARVIS_CORE_SOURCES
    src/core/Types.h
    src/core/ModelListModel.h
    src/core/ModelListModel.cpp
    src/core/JarvisController.h
    src/core/JarvisController.cpp
    src/core/MetricsCollector.h
    src/core/MetricsCollector.cpp
    src/core/RoutingEngine.h
    src/core/RoutingEngine.cpp
    src/providers/IProvider.h
    src/providers/OllamaAdapter.h
    src/providers/OllamaAdapter.cpp
    src/providers/GenericApiAdapter.h
    src/providers/GenericApiAdapter.cpp
    src/storage/DatabaseManager.h
    src/storage/DatabaseManager.cpp
    src/workspace/WorkspaceManager.h
    src/workspace/WorkspaceManager.cpp
)

add_library(jarvis_core STATIC ${JARVIS_CORE_SOURCES})
target_include_directories(jarvis_core PUBLIC src src/core src/providers src/storage src/workspace)
target_link_libraries(jarvis_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    Qt6::Sql
)

# Main Executable
add_executable(jarvis
    src/main.cpp
    src/resources.qrc
)

target_link_libraries(jarvis PRIVATE
    jarvis_core
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# Test Suite
enable_testing()
add_executable(test_jarvis
    tests/TestJarvisPhase1.cpp
)
target_link_libraries(test_jarvis PRIVATE
    jarvis_core
    Qt6::Test
)
add_test(NAME TestJarvisPhase1 COMMAND test_jarvis)

# Installation specification for packaging
include(GNUInstallDirs)
install(TARGETS jarvis
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Desktop entry & Icon
install(FILES debian/jarvis.desktop DESTINATION share/applications)
install(FILES src/ui/assets/icon.png DESTINATION share/icons/hicolor/256x256/apps RENAME jarvis.png)

# CPack configuration for Debian package (.deb)
set(CPACK_PACKAGE_NAME "jarvis")
set(CPACK_PACKAGE_VENDOR "JarvisAI")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JARVIS - Native Futuristic AI Desktop Assistant")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jarvis Engineering <support@jarvis-ai.local>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6gui6, libqt6qml6, libqt6quick6, libqt6network6, libqt6sql6, libqt6sql6-sqlite")

include(CPack)
