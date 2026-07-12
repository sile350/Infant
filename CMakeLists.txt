cmake_minimum_required(VERSION 3.16)

project(infant VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network PrintSupport Sql Concurrent)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        src/infantwindow.cpp
        src/infantwindow.h
        src/models.h
        src/apiclient.h
        src/apiclient.cpp
        src/repository.h
        src/repository.cpp
        src/localdatabase.h
        src/localdatabase.cpp
        src/fieldcrypto.h
        src/fieldcrypto.cpp
        src/licenseservice.h
        src/licenseservice.cpp
        src/singleinstance.h
        src/singleinstance.cpp
        src/licenseactivationdialog.h
        src/licenseactivationdialog.cpp
        src/custommessagebox.h
        src/custommessagebox.cpp
        src/imagebutton.h
        src/imagebutton.cpp
        src/appsettings.h
        src/appsettings.cpp
        src/exerciseassets.h
        src/exerciseassets.cpp
        src/exerciseprotocol.h
        src/exerciseprotocol.cpp
        src/onlypexercise.h
        src/onlypexercise.cpp
        src/patientdisplay.h
        src/patientdisplay.cpp
        src/exercisehost.h
        src/exercisehost.cpp
        src/protocoleditguard.h
        src/protocoleditguard.cpp
)

if(UNIX AND NOT APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND PROJECT_SOURCES
        src/smbiosreader.h
        src/smbiosreader.cpp
    )
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(infant
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET infant APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(infant SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(infant
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(infant PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Concurrent
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.infant)
endif()
set_target_properties(infant PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS infant
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(infant)
endif()

if(UNIX AND NOT APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_custom_command(TARGET infant POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:infant>/assets"
        COMMENT "Copying infant assets next to executable"
    )
elseif(UNIX AND NOT APPLE)
    add_custom_command(TARGET infant POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:infant>/assets"
        COMMENT "Copying infant assets next to executable"
    )
endif()

if(WIN32)
    find_path(INFANT_OPENSSL_DLL_DIR
        NAMES libcrypto-1_1-x64.dll libssl-1_1-x64.dll
        PATHS
            "${Qt5_DIR}/../../../Tools/mingw1120_64/opt/bin"
            "${Qt5_DIR}/../../../Tools/mingw1310_64/opt/bin"
            "${Qt5_DIR}/../../../Tools/mingw810_64/opt/bin"
            "D:/Qt/Tools/mingw1120_64/opt/bin"
            "D:/Qt/Tools/mingw1310_64/opt/bin"
        NO_DEFAULT_PATH
    )
    if(INFANT_OPENSSL_DLL_DIR)
        add_custom_command(TARGET infant POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${INFANT_OPENSSL_DLL_DIR}/libcrypto-1_1-x64.dll"
                "${INFANT_OPENSSL_DLL_DIR}/libssl-1_1-x64.dll"
                $<TARGET_FILE_DIR:infant>
            COMMENT "Copying OpenSSL DLLs for HTTPS"
        )
    else()
        message(WARNING "OpenSSL 1.1 DLLs not found. HTTPS to dokitlab.ru will fail until libcrypto/libssl are copied next to infant.exe")
    endif()

    add_custom_command(TARGET infant POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:infant>/assets"
        COMMENT "Copying infant assets next to executable"
    )
endif()
