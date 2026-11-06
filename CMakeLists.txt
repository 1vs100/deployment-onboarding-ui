cmake_minimum_required(VERSION 3.25)
project(deployment_onboarding_ui LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ONBOARDING_UI_FLAVOR "sdl3" CACHE STRING
    "Toolkit that paints the onboarding view: imgui or sdl3")
set_property(CACHE ONBOARDING_UI_FLAVOR PROPERTY STRINGS imgui sdl3)
set(ONBOARDING_UI_MANIFEST "" CACHE FILEPATH
    "File list the game needs, embedded into the library")
set(ONBOARDING_UI_SDL_INCLUDE_DIR "" CACHE PATH
    "SDL3 include directory for hosts that provide SDL themselves")
option(ONBOARDING_UI_WINRT "Build for a store app, where the app container needs its own file calls" OFF)
option(ONBOARDING_UI_BUILD_DEMO "Build the capture demo" ${PROJECT_IS_TOP_LEVEL})
option(ONBOARDING_UI_BUILD_TESTS "Build the layout tests" ${PROJECT_IS_TOP_LEVEL})

if(NOT ONBOARDING_UI_FLAVOR MATCHES "^(imgui|sdl3)$")
  message(FATAL_ERROR "ONBOARDING_UI_FLAVOR is '${ONBOARDING_UI_FLAVOR}', expected imgui or sdl3")
endif()

# Standalone builds get the fixture manifest; a host passes its own file.
if(NOT ONBOARDING_UI_MANIFEST)
  set(ONBOARDING_UI_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/demo/fixture-manifest.txt")
  message(STATUS "deployment-onboarding-ui: using the fixture manifest")
endif()
if(NOT EXISTS "${ONBOARDING_UI_MANIFEST}")
  message(FATAL_ERROR "ONBOARDING_UI_MANIFEST does not exist: ${ONBOARDING_UI_MANIFEST}")
endif()

set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED}")

# Every embedded asset goes through one script, so a rebuild only happens when
# the bytes change.
function(onboarding_embed input output name type)
  add_custom_command(
    OUTPUT "${output}"
    COMMAND ${CMAKE_COMMAND} -DINPUT=${input} -DOUTPUT=${output} -DNAME=${name} -DTYPE=${type}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed.cmake
    DEPENDS "${input}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed.cmake"
    COMMENT "Embedding ${input}")
endfunction()

onboarding_embed("${ONBOARDING_UI_MANIFEST}" "${GENERATED}/embedded_manifest.h" kEmbeddedManifest char)
onboarding_embed("${CMAKE_CURRENT_SOURCE_DIR}/src/web/index.html" "${GENERATED}/web_page.h" kWebPage "unsigned char")
onboarding_embed("${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/Roboto-Medium.ttf" "${GENERATED}/font_data.h" kFontData "unsigned char")

set(SOURCES
  src/core/app.cpp
  src/core/catalog.cpp
  src/core/file_store.cpp
  src/core/manifest.cpp
  src/gate.cpp
  src/ui/input.cpp
  src/ui/layout.cpp
  src/ui/view.cpp
  src/web/status_json.cpp
  src/thirdparty/qrcodegen/qrcodegen.cpp
  "${GENERATED}/embedded_manifest.h"
  "${GENERATED}/web_page.h"
  "${GENERATED}/font_data.h")

if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" OR ONBOARDING_UI_WINRT)
  # The AppContainer entry points only link for a store package.
  list(APPEND SOURCES src/platform/file_io_winrt.cpp src/platform/sha256_win32.cpp)
elseif(WIN32)
  list(APPEND SOURCES src/platform/file_io_win32.cpp src/platform/sha256_win32.cpp)
else()
  list(APPEND SOURCES src/platform/file_io_posix.cpp src/platform/sha256_posix.cpp)
endif()

set(FLAVOR_SOURCES src/ui/render_${ONBOARDING_UI_FLAVOR}.cpp)
if(ONBOARDING_UI_FLAVOR STREQUAL "sdl3")
  list(APPEND FLAVOR_SOURCES src/ui/text_atlas.cpp)
else()
  list(APPEND FLAVOR_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_sdl3.cpp
    third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp)
endif()

add_library(deployment_onboarding_ui STATIC ${SOURCES} ${FLAVOR_SOURCES})
add_library(onboarding_ui::onboarding_ui ALIAS deployment_onboarding_ui)

target_include_directories(deployment_onboarding_ui
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends"
    "${GENERATED}")

target_compile_definitions(deployment_onboarding_ui PRIVATE IMGUI_DISABLE_DEMO_WINDOWS)
if(ONBOARDING_UI_WINRT)
  target_compile_definitions(deployment_onboarding_ui PRIVATE REX_PLATFORM_WINRT=1)
endif()
if(ONBOARDING_UI_SDL_INCLUDE_DIR)
  target_include_directories(deployment_onboarding_ui PRIVATE "${ONBOARDING_UI_SDL_INCLUDE_DIR}")
endif()
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
  target_link_libraries(deployment_onboarding_ui PRIVATE bcrypt)
endif()

# A host that provides SDL itself (a console prefix, a vendored copy) does not
# need this; the standalone build does.
find_package(SDL3 CONFIG QUIET)
if(TARGET SDL3::SDL3)
  target_link_libraries(deployment_onboarding_ui PUBLIC SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
  target_link_libraries(deployment_onboarding_ui PUBLIC SDL3::SDL3-static)
endif()

if(MSVC)
  # C++/WinRT coroutines in the host's transports need unwind semantics.
  target_compile_options(deployment_onboarding_ui PRIVATE /EHsc)
else()
  target_compile_options(deployment_onboarding_ui PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

if(ONBOARDING_UI_BUILD_DEMO)
  add_executable(deployment_onboarding_demo demo/main.cpp)
  target_link_libraries(deployment_onboarding_demo PRIVATE deployment_onboarding_ui)
  target_include_directories(deployment_onboarding_demo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src" "${GENERATED}")
  target_compile_definitions(deployment_onboarding_demo PRIVATE
    ONBOARDING_UI_FLAVOR_NAME="${ONBOARDING_UI_FLAVOR}")
endif()

if(ONBOARDING_UI_BUILD_TESTS)
  enable_testing()
  add_executable(onboarding_layout_test tests/layout_test.cpp)
  target_link_libraries(onboarding_layout_test PRIVATE deployment_onboarding_ui)
  target_include_directories(onboarding_layout_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src" "${GENERATED}")
  add_test(NAME onboarding_layout_test COMMAND onboarding_layout_test)
endif()
