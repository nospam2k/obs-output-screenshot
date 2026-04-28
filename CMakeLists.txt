cmake_minimum_required(VERSION 3.22)

project(obs-output-screenshot VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Plugin target ─────────────────────────────────────────────────────────────
add_library(obs-output-screenshot MODULE src/plugin-main.cpp)

target_compile_definitions(obs-output-screenshot PRIVATE
    PLUGIN_VERSION="${CMAKE_PROJECT_VERSION}"
)

# ── Find libobs ───────────────────────────────────────────────────────────────
if(DEFINED LIBOBS_INCLUDE_DIR)
    target_include_directories(obs-output-screenshot PRIVATE "${LIBOBS_INCLUDE_DIR}")
    if(APPLE)
        # Symbols provided by OBS at load time; no link target needed.
        target_link_options(obs-output-screenshot PRIVATE -undefined dynamic_lookup)
    elseif(WIN32 AND DEFINED LIBOBS_LIB)
        target_link_libraries(obs-output-screenshot PRIVATE "${LIBOBS_LIB}")
    endif()
else()
    # Windows CI and local builds: use the installed libobs cmake package.
    find_package(libobs REQUIRED)
    target_link_libraries(obs-output-screenshot PRIVATE OBS::libobs)
endif()

# ── obs-websocket-api (header-only, bundled fallback) ─────────────────────────
find_package(obs-websocket-api QUIET)
if(obs-websocket-api_FOUND)
    target_link_libraries(obs-output-screenshot PRIVATE OBS::obs-websocket-api)
else()
    target_include_directories(obs-output-screenshot PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/deps")
endif()

# ── Platform output name ──────────────────────────────────────────────────────
set_target_properties(obs-output-screenshot PROPERTIES PREFIX "")
if(WIN32)
    set_target_properties(obs-output-screenshot PROPERTIES SUFFIX ".dll")
elseif(APPLE)
    set_target_properties(obs-output-screenshot PROPERTIES SUFFIX ".dylib")
endif()

# ── Install ───────────────────────────────────────────────────────────────────
if(WIN32)
    install(TARGETS obs-output-screenshot LIBRARY DESTINATION obs-plugins/64bit)
elseif(APPLE)
    install(TARGETS obs-output-screenshot LIBRARY DESTINATION obs-plugins/mac)
endif()
