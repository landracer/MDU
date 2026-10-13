cmake_minimum_required(VERSION 3.16)

project(multidisplay-unified VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui SerialPort Widgets)
option(ENABLE_QWT "Enable Qwt plotting library" ON)

# Collect all source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

file(GLOB_RECURSE HEADERS_MOC
    "include/*.h"
    "src/*.h"
)

file(GLOB RESOURCES
    "resources/*"
)

message(STATUS "Found source files: ${SOURCES}")
message(STATUS "Found header files: ${HEADERS_MOC}")

# Main executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS_MOC}
    ${RESOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    src
)

# Single source of truth for the version string (used by main.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

# ─── Unit tests (CTest aggregates the built-in TestHarness runner) ───
enable_testing()
add_test(NAME unit_tests COMMAND ${PROJECT_NAME} --unit-test)
set_tests_properties(unit_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    PASS_REGULAR_EXPRESSION "ALL TESTS PASSED"
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::SerialPort
    Qt6::Widgets
)

# Qwt linking (must come after target creation)
if(ENABLE_QWT)
    set(QWT_FOUND FALSE)
    # Try CMake config first
    find_package(Qwt 6.2 CONFIG QUIET)
    if(Qwt_FOUND)
        message(STATUS "Qwt found via CMake config - enabling plotting")
        target_link_libraries(${PROJECT_NAME} PRIVATE Qwt)
        target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_QWT_PLOTTING)
    else()
        # Direct library path for Qwt 6.3 installed without CMake config
        find_library(QWT_LIBRARY NAMES qwt-qt6 Qwt PATHS /usr/lib64 /usr/lib /usr/local/lib)
        if(QWT_LIBRARY)
            message(STATUS "Qwt found at ${QWT_LIBRARY} - enabling plotting")
            target_link_libraries(${PROJECT_NAME} PRIVATE ${QWT_LIBRARY})
            target_include_directories(${PROJECT_NAME} PRIVATE /usr/include/qt6)
            target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_QWT_PLOTTING)
            set(QWT_FOUND TRUE)
        else()
            message(WARNING "Qwt not found - plotting features disabled. Install Qwt 6.2+ or set ENABLE_QWT=OFF")
        endif()
    endif()
endif()

# Platform-specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    # macOS support (future)
endif()

# Android settings
if(ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        ANDROID_BUILD_GRADLE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle"
    )
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
