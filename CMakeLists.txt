cmake_minimum_required(VERSION 3.16) # Need newer CMake for Qt6/better C++17 support

project(IYSSearcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer standard features

# --- Find Qt ---
# Find necessary Qt components (adjust for Qt5 if needed)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Concurrent) # Concurrent might be implicitly included but good to be explicit
# For Qt5 use: find_package(Qt5 REQUIRED COMPONENTS Widgets Core Concurrent)

# --- Automatic Qt Setup ---
set(CMAKE_AUTOMOC ON) # Automatically run Meta-Object Compiler
set(CMAKE_AUTORCC ON) # Automatically handle Qt resources (if you add a .qrc file)
set(CMAKE_AUTOUIC ON) # Automatically run UI compiler for .ui files

# --- Add Source Files ---
set(SOURCES
    main.cpp
    mainwindow.cpp
    searchworker.cpp
    searchlogic.cpp
)

# --- Add Header Files ---
set(HEADERS
    mainwindow.h
    searchworker.h
    searchlogic.h
)

# --- Add UI Files ---
set(FORMS
    mainwindow.ui
)

# --- Add Resources ---
set(RESOURCES
    resources.qrc
)

# --- Create Executable ---
qt_add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    ${RESOURCES}
)

# --- Link Libraries ---
# Link against the found Qt modules
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Concurrent # For QStorageInfo / QThread etc.
    # Use Qt5::Widgets etc. for Qt5
)

# --- Platform Specific ---
if(WIN32)
    # <filesystem> might need explicit linking on some older MinGW setups,
    # but usually not needed with MSVC or modern GCC/Clang and Qt's setup.
    # target_link_libraries(${PROJECT_NAME} PRIVATE -lstdc++fs) # Uncomment if needed for MinGW

    # If directly using GetLogicalDrives (though we replaced it with QStorageInfo)
    # target_link_libraries(${PROJECT_NAME} PRIVATE user32 shell32)
endif()

if(APPLE)
    # Specific settings for macOS if needed
endif()

if(UNIX AND NOT APPLE)
    # Specific settings for Linux if needed
    # Often <filesystem> links correctly with modern GCC/Clang
    # target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs) # For older GCC/libstdc++
endif()

# --- Installation (Optional) ---
# install(TARGETS ${PROJECT_NAME} DESTINATION bin)