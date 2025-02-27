#include "SDL3/SDL.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "qrcodegen.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef __EMSCRIPTEN__
  #include <emscripten/emscripten.h>
#endif

#include <iostream>
#include <memory>

#define INITAL_WINDOW_WIDTH  1280
#define INITAL_WINDOW_HEIGHT 720
#define QR_TEXT_LIMIT        1024

#define SAVE_FILE_EVENT (SDL_EVENT_USER + 1)

int app_init();
void app_deinit();
void app_loop(void* data);

void app_handle_event(const SDL_Event& event);
void app_imgui_render();
void app_main_render();

struct app_t {
  bool quit       = false;
  bool imgui_open = true;
  std::shared_ptr<SDL_Window> window;
  std::shared_ptr<SDL_Renderer> renderer;

  char qr_text[QR_TEXT_LIMIT] = "Hello, World!";
  std::shared_ptr<SDL_Surface> qr_surface;
  std::shared_ptr<SDL_Texture> qr_texture;
  float qr_color1[4] = {0, 0, 0, 1};
  float qr_color2[4] = {1, 1, 1, 1};
  int qr_min_ver     = 1;
  int qr_max_ver     = 40;
  int qr_mask        = -1;
  int qr_ecc         = 0;
  bool qr_boost_ecc  = false;

  // layout params
  SDL_FRect imgui_rect;
  SDL_FRect main_rect;
  SDL_FRect qr_rect;
} app;

void recompute_layout();
bool recompute_qr();

void app_imgui_render() {
  if (app.imgui_open) {
    ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::SetWindowPos({app.imgui_rect.x, app.imgui_rect.y});
    ImGui::SetWindowSize({app.imgui_rect.w, app.imgui_rect.h});

    bool recompute = false;

    if (ImGui::Button("Text")) {
      strncpy(app.qr_text, "Hello, World!", QR_TEXT_LIMIT);
      recompute_qr();
    }
    ImGui::SameLine();
    if (ImGui::Button("URL")) {
      strncpy(app.qr_text, "https://github.com/REDl3east/qrview", QR_TEXT_LIMIT);
      recompute_qr();
    }
    ImGui::SameLine();
    if (ImGui::Button("Email")) {
      strncpy(app.qr_text, "mailto:someone@yoursite.com?subject=Mail%20from%20Our%20Site", QR_TEXT_LIMIT);
      recompute_qr();
    }
    ImGui::SameLine();
    if (ImGui::Button("Phone")) {
      strncpy(app.qr_text, "tel:+12125551212", QR_TEXT_LIMIT);
      recompute_qr();
    }
    ImGui::SameLine();
    if (ImGui::Button("Wifi")) {
      strncpy(app.qr_text, "WIFI:T:WPA;S:mynetwork;P:mypass;;", QR_TEXT_LIMIT);
      recompute_qr();
    }

    if (ImGui::InputTextMultiline("Input", app.qr_text, QR_TEXT_LIMIT)) {
      recompute = true;
    }

    const char* ecc_combo[] = {
        "Low",
        "Medium",
        "Quartile",
        "High",
    };
    if (ImGui::Combo("ECC", &app.qr_ecc, ecc_combo, IM_ARRAYSIZE(ecc_combo))) {
      recompute = true;
    }
    if (ImGui::Checkbox("Boost ECC", &app.qr_boost_ecc)) {
      recompute = true;
    }

    if (ImGui::InputInt("Version Min", &app.qr_min_ver, 1, 1)) {
      recompute = true;
      if (app.qr_min_ver > app.qr_max_ver) {
        app.qr_min_ver--;
      }
    }
    if (ImGui::InputInt("Version Max", &app.qr_max_ver, 1, 1)) {
      recompute = true;
      if (app.qr_max_ver < app.qr_min_ver) {
        app.qr_max_ver++;
      }
    }
    if (app.qr_min_ver < 1) app.qr_min_ver = 1;
    if (app.qr_min_ver > 40) app.qr_min_ver = 40;
    if (app.qr_max_ver < 1) app.qr_max_ver = 1;
    if (app.qr_max_ver > 40) app.qr_max_ver = 40;

    if (ImGui::InputInt("Mask", &app.qr_mask, 1, 1)) {
      recompute = true;
      if (app.qr_mask < -1) app.qr_mask = -1;
      if (app.qr_mask > 7) app.qr_mask = 7;
    }

    ImGui::NewLine();

    if (ImGui::ColorPicker4("Color 1", app.qr_color1, ImGuiColorEditFlags_AlphaBar)) {
      recompute = true;
    }
    if (ImGui::ColorPicker4("Color 2", app.qr_color2, ImGuiColorEditFlags_AlphaBar)) {
      recompute = true;
    }
    ImGui::End();

    if (recompute) {
      recompute_qr();
    }
  }
}

void app_main_render() {
  SDL_SetRenderDrawColor(app.renderer.get(), 0x18, 0x18, 0x18, 255);
  SDL_RenderClear(app.renderer.get());

  SDL_RenderTexture(app.renderer.get(), app.qr_texture.get(), NULL, &app.qr_rect);
  SDL_SetRenderDrawColor(app.renderer.get(), 255.0f * app.qr_color1[0], 255.0f * app.qr_color1[1], 255.0f * app.qr_color1[2], 255.0f * app.qr_color1[3]);
  SDL_RenderRect(app.renderer.get(), &app.qr_rect);
}

void app_handle_event(const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_QUIT: {
      app.quit = true;
      break;
    }
    case SDL_EVENT_KEY_DOWN: {
      SDL_Keycode code = event.key.key;
      SDL_Keymod mod   = event.key.mod;
      if (code == SDLK_ESCAPE) {
        app.imgui_open = !app.imgui_open;
        recompute_layout();
      }
      if ((mod & SDL_KMOD_LCTRL) && (code == SDLK_S)) {
        static const SDL_DialogFileFilter filters[] = {
            {"PNG images", "png"},
        };

        SDL_ShowSaveFileDialog([](void* userdata, const char* const* filelist, int filter) {
          if (filelist) {
            if (!*filelist) return;

            SDL_Event event;
            SDL_memset(&event, 0, sizeof(event));
            event.type       = SAVE_FILE_EVENT;
            event.user.data1 = strdup(*filelist);
            SDL_PushEvent(&event);
          }
        },
                               nullptr, app.window.get(), filters, 1, nullptr);
      }

      break;
    }
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
      recompute_layout();
      break;
    }
    case SAVE_FILE_EVENT: {
      char* file = (char*)event.user.data1;
      stbi_write_png(file, app.qr_surface.get()->w, app.qr_surface.get()->h, 4, app.qr_surface.get()->pixels, app.qr_surface.get()->w * 4);
      free(file);
      break;
    }
    default: {
      break;
    }
  }
}

int app_init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << '\n';
    return 1;
  }

  app.window = std::shared_ptr<SDL_Window>(SDL_CreateWindow("qrview", INITAL_WINDOW_WIDTH, INITAL_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE), SDL_DestroyWindow);
  if (app.window == nullptr) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << '\n';
    return 1;
  }

  app.renderer = std::shared_ptr<SDL_Renderer>(SDL_CreateRenderer(app.window.get(), NULL), SDL_DestroyRenderer);
  if (app.renderer == nullptr) {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << '\n';
    return 1;
  }

  SDL_SetRenderDrawBlendMode(app.renderer.get(), SDL_BLENDMODE_BLEND);

  recompute_qr();
  recompute_layout();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui_ImplSDL3_InitForSDLRenderer(app.window.get(), app.renderer.get());
  ImGui_ImplSDLRenderer3_Init(app.renderer.get());
  ImGui::StyleColorsDark();

  ImGui::GetIO().IniFilename = NULL;
  ImGui::GetIO().LogFilename = NULL;

  return 1;
}

void app_deinit() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  SDL_Quit();
}

void app_loop(void* data) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    app_handle_event(event);
  }

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  app_imgui_render();

  ImGui::Render();

  app_main_render();

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), app.renderer.get());
  SDL_RenderPresent(app.renderer.get());
}

int main(int argc, char** argv) {
  if (!app_init()) {
    std::cerr << "App failed to initialized\n";
    return 1;
  }

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(app_loop, NULL, 0, 1);
#else
  while (!app.quit) {
    app_loop(NULL);
  }
#endif

  app_deinit();

  return 0;
}

void recompute_layout() {
  int window_w, window_h;
  SDL_GetWindowSize(app.window.get(), &window_w, &window_h);
  if (app.imgui_open) {
    app.imgui_rect = {
        0,
        0,
        (float)window_w * 0.25f,
        (float)window_h,
    };
  } else {
    app.imgui_rect = {
        0,
        0,
        0,
        0,
    };
  }
  app.main_rect = {
      app.imgui_rect.w,
      0,
      (float)window_w - app.imgui_rect.w,
      (float)window_h,
  };

  float qr_size = app.main_rect.h * (app.imgui_open ? 0.75f : 0.90f);
  app.qr_rect   = {
      app.main_rect.x + (app.main_rect.w - qr_size) * 0.5f,
      app.main_rect.y + (app.main_rect.h - qr_size) * 0.5f,
      qr_size,
      qr_size,
  };
}

bool recompute_qr() {
  uint8_t qr0[qrcodegen_BUFFER_LEN_MAX];
  uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
  bool ok = qrcodegen_encodeText(app.qr_text, tempBuffer, qr0, (qrcodegen_Ecc)app.qr_ecc, app.qr_min_ver, app.qr_max_ver, (qrcodegen_Mask)app.qr_mask, app.qr_boost_ecc);

  if (!ok) {
    std::cerr << "Failed to encode QR code" << '\n';
    return false;
  }

  app.qr_surface = std::shared_ptr<SDL_Surface>(SDL_CreateSurface(qrcodegen_getSize(qr0), qrcodegen_getSize(qr0), SDL_PIXELFORMAT_ABGR8888), SDL_DestroySurface);
  if (app.qr_surface == nullptr) {
    std::cerr << "QR surface could not be created! SDL_Error: " << SDL_GetError() << '\n';
    return false;
  }

  auto convert_rgb = [](float color[4]) -> Uint32 {
    Uint8 r1 = static_cast<Uint8>(color[0] * 255.0f);
    Uint8 g1 = static_cast<Uint8>(color[1] * 255.0f);
    Uint8 b1 = static_cast<Uint8>(color[2] * 255.0f);
    Uint8 a1 = static_cast<Uint8>(color[3] * 255.0f);
    return (a1 << 24) | (b1 << 16) | (g1 << 8) | r1;
  };
  Uint32 rgba1 = convert_rgb(app.qr_color1);
  Uint32 rgba2 = convert_rgb(app.qr_color2);

  for (int y = 0; y < qrcodegen_getSize(qr0); ++y) {
    for (int x = 0; x < qrcodegen_getSize(qr0); ++x) {
      Uint32 color                             = qrcodegen_getModule(qr0, x, y) ? rgba1 : rgba2;
      Uint32* pixels                           = (Uint32*)app.qr_surface.get()->pixels;
      pixels[(y * qrcodegen_getSize(qr0)) + x] = color;
    }
  }

  app.qr_texture = std::shared_ptr<SDL_Texture>(SDL_CreateTextureFromSurface(app.renderer.get(), app.qr_surface.get()), SDL_DestroyTexture);
  if (app.qr_texture == nullptr) {
    std::cerr << "QR texture could not be created! SDL_Error: " << SDL_GetError() << '\n';
    return false;
  }

  SDL_SetTextureScaleMode(app.qr_texture.get(), SDL_SCALEMODE_NEAREST);

  return true;
}