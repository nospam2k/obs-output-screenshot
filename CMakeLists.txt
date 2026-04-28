cmake_minimum_required(VERSION 3.22)

project(obs-output-screenshot VERSION 1.0.0)

set(PLUGIN_AUTHOR "obs-output-screenshot")
set(MACOS_BUNDLEID "com.obsoutputscreenshot.obs-output-screenshot")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Find OBS ──────────────────────────────────────────────────────────────────
find_package(libobs REQUIRED)
find_package(obs-websocket-api QUIET)

# ── Plugin target ─────────────────────────────────────────────────────────────
add_library(obs-output-screenshot MODULE src/plugin-main.cpp)

target_compile_definitions(obs-output-screenshot PRIVATE
    PLUGIN_VERSION="${CMAKE_PROJECT_VERSION}"
)

target_link_libraries(obs-output-screenshot PRIVATE OBS::libobs)

# obs-websocket-api is header-only; we ship a local copy as fallback
if(obs-websocket-api_FOUND)
    target_link_libraries(obs-output-screenshot PRIVATE OBS::obs-websocket-api)
else()
    message(STATUS "obs-websocket-api not found via CMake, using bundled header")
    target_include_directories(obs-output-screenshot PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/deps")
endif()

# ── Platform setup ────────────────────────────────────────────────────────────
if(OS_WINDOWS)
    set_target_properties(obs-output-screenshot PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(OS_MACOS)
    set_target_properties(obs-output-screenshot PROPERTIES
        PREFIX ""
        SUFFIX ".dylib"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${MACOS_BUNDLEID}"
    )
endif()

# ── Install via obs-plugintemplate helper ──────────────────────────────────────
include(cmake/ObsPluginHelpers.cmake OPTIONAL RESULT_VARIABLE _helpers_found)
if(_helpers_found)
    setup_plugin_target(obs-output-screenshot)
else()
    # Fallback install for manual builds
    install(TARGETS obs-output-screenshot
        LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/obs-plugins/64bit"
    )
endif()
