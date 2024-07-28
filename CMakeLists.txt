cmake_minimum_required(VERSION 3.5.0)
project(qrview VERSION 0.1.0 LANGUAGES C CXX)

set(SDL3_SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/libs/sdl3)
set(IMGUI_SRC_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs)

set(MAIN_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/qrcodegen.cpp
  ${IMGUI_SRC_DIR}/imgui.cpp
  ${IMGUI_SRC_DIR}/imgui_demo.cpp
  ${IMGUI_SRC_DIR}/imgui_draw.cpp
  ${IMGUI_SRC_DIR}/imgui_tables.cpp
  ${IMGUI_SRC_DIR}/imgui_widgets.cpp
  ${IMGUI_SRC_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_SRC_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

add_executable(qrview ${MAIN_SRC})
target_link_libraries(qrview SDL3-static)
target_include_directories(qrview PRIVATE 
  ${IMGUI_SRC_DIR}
  ${IMGUI_SRC_DIR}/backends
  ${SDL3_SRC_DIR}/include
  ${SDL3_SRC_DIR}/include/SDL3
  ${SDL3_SRC_DIR}/build_config
)

if(EMSCRIPTEN)
  SET(GCC_COVERAGE_LINK_FLAGS "--emrun -s USE_SDL=2 -s DISABLE_EXCEPTION_CATCHING=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
  set(USE_FLAGS "-s USE_SDL=2 -s DISABLE_EXCEPTION_CATCHING=1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()
