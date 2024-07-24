include(FetchContent)

set(SDL_SHARED             OFF CACHE BOOL "" FORCE)
set(SDL_STATIC             ON  CACHE BOOL "" FORCE)
set(SDL_TEST               OFF CACHE BOOL "" FORCE)
set(SDL_TESTS              OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS      OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG 27f86cb  # main 07-24-24
  SOURCE_DIR ${SDL3_SRC_DIR}
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG 237165a  # main 07-24-24
  SOURCE_DIR ${IMGUI_SRC_DIR}
)

FetchContent_MakeAvailable(SDL3 imgui)