cmake_minimum_required(VERSION 3.21)

project(ProsToggleUploader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(ProsToggleUploader
    WIN32
    MACOSX_BUNDLE
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.hpp
)

target_link_libraries(ProsToggleUploader PRIVATE Qt6::Widgets)

target_compile_definitions(ProsToggleUploader PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

set_target_properties(ProsToggleUploader PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "PROS Toggle Uploader"
    MACOSX_BUNDLE_GUI_IDENTIFIER "localtools.prostoggleuploader"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ProsToggleUploader
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
