cmake_minimum_required(VERSION 3.19)
project(harbour-asphaltcam CXX)

# i486 puts Qt CMake files in /usr/lib/cmake; aarch64 uses /usr/lib64/cmake.
# An in-source cache from the other architecture otherwise hides Qt5Config.cmake.
foreach(_qt_cmake_dir /usr/lib64/cmake /usr/lib/cmake)
    if(EXISTS "${_qt_cmake_dir}")
        list(APPEND CMAKE_PREFIX_PATH "${_qt_cmake_dir}")
    endif()
endforeach()
list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

find_package(Qt5Core REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Positioning REQUIRED)
find_package(Qt5Multimedia REQUIRED)

include(FindPkgConfig)
pkg_check_modules(SAILFISH REQUIRED IMPORTED_TARGET sailfishapp)
pkg_check_modules(MLITE REQUIRED IMPORTED_TARGET mlite5)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)

set(SRC
    src/asphaltcam.cpp
    src/appsettings.cpp
    src/telemetry.cpp
    src/clipstorage.cpp
    src/clipmodel.cpp
    src/devicestatus.cpp
    src/recordingengine.cpp
    src/subtitlelog.cpp
)

add_executable(harbour-asphaltcam ${SRC})
target_compile_definitions(harbour-asphaltcam PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)
target_link_libraries(harbour-asphaltcam
    Qt5::Quick
    Qt5::Positioning
    Qt5::Multimedia
    PkgConfig::SAILFISH
    PkgConfig::MLITE
)

install(TARGETS harbour-asphaltcam
    RUNTIME DESTINATION bin
)
install(DIRECTORY qml
    DESTINATION share/harbour-asphaltcam
)
install(DIRECTORY translations
    DESTINATION share/harbour-asphaltcam
    FILES_MATCHING PATTERN "*.qm"
)
install(FILES LICENSE
    DESTINATION share/harbour-asphaltcam
)
install(FILES harbour-asphaltcam.desktop
    DESTINATION share/applications
)
install(FILES icons/86x86/harbour-asphaltcam.png
    DESTINATION share/icons/hicolor/86x86/apps
)
install(FILES icons/108x108/harbour-asphaltcam.png
    DESTINATION share/icons/hicolor/108x108/apps
)
install(FILES icons/128x128/harbour-asphaltcam.png
    DESTINATION share/icons/hicolor/128x128/apps
)
install(FILES icons/172x172/harbour-asphaltcam.png
    DESTINATION share/icons/hicolor/172x172/apps
)

FILE(GLOB TsFiles "translations/*.ts")
add_custom_target(distfiles
    SOURCES
        harbour-asphaltcam.desktop
        qml/harbour-asphaltcam.qml
        qml/CameraDevices.qml
        qml/NgfClient.qml
        qml/cover/CoverPage.qml
        qml/pages/KeepAliveHint.qml
        qml/pages/RecordingNoticeDialog.qml
        qml/pages/RecordPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/GalleryPage.qml
        qml/pages/PlayerPage.qml
        rpm/harbour-asphaltcam.changes
        rpm/harbour-asphaltcam.changes.in
        rpm/harbour-asphaltcam.changes.run.in
        rpm/harbour-asphaltcam.spec
        LICENSE
        icons/harbour-asphaltcam.svg
        ${TsFiles}
        ${SRC}
)

file(WRITE "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
    "${CMAKE_INSTALL_PREFIX}\n${CMAKE_BINARY_DIR}/harbour-asphaltcam:bin\n")
