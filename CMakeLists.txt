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
# CI passes LIBOBS_INCLUDE_DIR and (on Windows) LIBOBS_LIB explicitly.
# Local builds can rely on find_package if OBS is installed system-wide.
if(DEFINED LIBOBS_INCLUDE_DIR)
    target_include_directories(obs-output-screenshot PRIVATE "${LIBOBS_INCLUDE_DIR}")
    if(WIN32 AND DEFINED LIBOBS_LIB)
        target_link_libraries(obs-output-screenshot PRIVATE "${LIBOBS_LIB}")
    elseif(APPLE)
        # Symbols are provided by OBS at load time — no link needed.
        target_link_options(obs-output-screenshot PRIVATE -undefined dynamic_lookup)
    endif()
else()
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
