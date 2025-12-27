#include <iostream>
#include <string>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#include <SDL3/SDL.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_bgfx.h>
#include <imgui/imgui_impl_sdl3.h>

#include "file-ops.h"

struct PosColorVertex {
  float x;
  float y;
  float z;
};

static PosColorVertex cube_vertices[] = {
    {-1.0f, 1.0f, 1.0f},   {1.0f, 1.0f, 1.0f},   {-1.0f, -1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f},   {-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, -1.0f},
    {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f},
};

static const uint16_t cube_tri_list[] = {
    0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 2, 4, 4, 2, 6,
    1, 5, 3, 5, 7, 3, 0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
};

static bgfx::ShaderHandle create_shader(const std::string &shader,
                                        const char *name) {
  const bgfx::Memory *mem = bgfx::copy(shader.data(), shader.size());
  const bgfx::ShaderHandle handle = bgfx::createShader(mem);
  bgfx::setName(handle, name);
  return handle;
}

struct context_t {
  SDL_Window *window = nullptr;
  bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
  bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle u_color = BGFX_INVALID_HANDLE;

  float cube_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};

  float cam_pitch = 0.0f;
  float cam_yaw = 0.0f;
  float rot_scale = 0.01f;

  int prev_mouse_x = 0;
  int prev_mouse_y = 0;

  int width = 0;
  int height = 0;
};

static context_t context;

SDL_AppResult SDL_AppInit([[maybe_unused]] void **appstate,
                          [[maybe_unused]] int argc,
                          [[maybe_unused]] char *argv[]) {

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL could not initialize. SDL_Error: %s\n"
              << SDL_GetError() << std::endl;
    return SDL_APP_FAILURE;
  }

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

  const int width = 800;
  const int height = 600;

  SDL_WindowFlags window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

  SDL_Window *window =
      SDL_CreateWindow("sdl3_bgfx_imgui_example", (int)(width * main_scale),
                       (int)(height * main_scale), window_flags);

  if (window == nullptr) {
    std::cerr << "Window could not be created. SDL_Error: %s\n"
              << SDL_GetError() << std::endl;
    return SDL_APP_FAILURE;
  }

  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

#if defined(SDL_PLATFORM_WIN32)
  void *hwnd =
      SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                             SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

  if (!hwnd) {
    std::cerr << "hwnd null" << std::endl;
    return SDL_APP_FAILURE;
  }
#else
#error "Only support Windows"
#endif

  bgfx::renderFrame();

  bgfx::Init init;
  init.platformData.nwh = hwnd;
  init.type = bgfx::RendererType::Direct3D12;
  init.resolution.width = width;
  init.resolution.height = height;
  init.resolution.reset = BGFX_RESET_VSYNC;

  if (!bgfx::init(init)) {
    std::cerr << "bgfx init failed" << std::endl;
    return SDL_APP_FAILURE;
  }

  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f,
                     0);
  bgfx::setViewRect(0, 0, 0, width, height);

  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  ImGui_Implbgfx_Init(255);
#if BX_PLATFORM_WINDOWS
  ImGui_ImplSDL3_InitForD3D(window);
#else
#error "Only support Windows"
#endif

  bgfx::VertexLayout pos_col_vert_layout;
  pos_col_vert_layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .end();
  bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
      bgfx::makeRef(cube_vertices, sizeof(cube_vertices)), pos_col_vert_layout);
  bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
      bgfx::makeRef(cube_tri_list, sizeof(cube_tri_list)));

  const std::string shader_root = "shader/build/";

  std::string vshader;
  if (!fileops::read_file(shader_root + "v_simple.bin", vshader)) {
    std::cerr << "Could not find shader vertex shader (ensure shaders have "
                 "been compiled).\nRun compile-shaders-<platform>.sh/bat"
              << std::endl;
    return SDL_APP_FAILURE;
  }

  std::string fshader;
  if (!fileops::read_file(shader_root + "f_simple.bin", fshader)) {
    std::cerr << "Could not find shader fragment shader (ensure shaders have "
                 "been compiled).\nRun compile-shaders-<platform>.sh/bat"
              << std::endl;
    return SDL_APP_FAILURE;
  }

  bgfx::ShaderHandle vsh = create_shader(vshader, "vshader");
  bgfx::ShaderHandle fsh = create_shader(fshader, "fshader");
  bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);
  bgfx::UniformHandle u_color =
      bgfx::createUniform("u_color", bgfx::UniformType::Vec4);

  context.width = width;
  context.height = height;
  context.program = program;
  context.window = window;
  context.vbh = vbh;
  context.ibh = ibh;
  context.u_color = u_color;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *appstate) {
  ImGui_Implbgfx_NewFrame();
  ImGui_ImplSDL3_NewFrame();

  ImGui::NewFrame();

  ImGui::SetNextWindowPos({10, 10}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({250, 150}, ImGuiCond_FirstUseEver);

  ImGui::Begin("Cube Controls");
  ImGui::ColorEdit4("Cube Color", context.cube_color);
  ImGui::End();

  ImGui::Render();

  ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

  if (!ImGui::GetIO().WantCaptureMouse) {
    // simple input code for orbit camera
    float mouse_x, mouse_y;
    const int buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    if ((buttons & SDL_BUTTON_LEFT) != 0) {
      int delta_x = mouse_x - context.prev_mouse_x;
      int delta_y = mouse_y - context.prev_mouse_y;
      context.cam_yaw += float(-delta_x) * context.rot_scale;
      context.cam_pitch += float(-delta_y) * context.rot_scale;
    }
    context.prev_mouse_x = mouse_x;
    context.prev_mouse_y = mouse_y;
  }

  float cam_rotation[16];
  bx::mtxRotateXYZ(cam_rotation, context.cam_pitch, context.cam_yaw, 0.0f);

  float cam_translation[16];
  bx::mtxTranslate(cam_translation, 0.0f, 0.0f, -5.0f);

  float cam_transform[16];
  bx::mtxMul(cam_transform, cam_translation, cam_rotation);

  float view[16];
  bx::mtxInverse(view, cam_transform);

  float proj[16];
  bx::mtxProj(proj, 60.0f, float(context.width) / float(context.height), 0.1f,
              100.0f, bgfx::getCaps()->homogeneousDepth);

  bgfx::setViewTransform(0, view, proj);

  float model[16];
  bx::mtxIdentity(model);
  bgfx::setTransform(model);

  bgfx::setUniform(context.u_color, context.cube_color);

  bgfx::setVertexBuffer(0, context.vbh);
  bgfx::setIndexBuffer(context.ibh);

  bgfx::submit(0, context.program);

  bgfx::frame();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event) {
  ImGui_ImplSDL3_ProcessEvent(event);

  if (event->type == SDL_EVENT_QUIT)
    return SDL_APP_SUCCESS;

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
      event->type == SDL_EVENT_MOUSE_BUTTON_UP ||
      event->type == SDL_EVENT_MOUSE_MOTION)
    if (ImGui::GetIO().WantCaptureMouse)
      return SDL_APP_CONTINUE;

  if (event->type == SDL_EVENT_WINDOW_RESIZED ||
      event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
    int new_width = event->window.data1;
    int new_height = event->window.data2;

    context.width = new_width;
    context.height = new_height;

    bgfx::reset(new_width, new_height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(0, 0, 0, new_width, new_height);
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit([[maybe_unused]] void *appstate,
                 [[maybe_unused]] SDL_AppResult result) {
  bgfx::destroy(context.vbh);
  bgfx::destroy(context.ibh);
  bgfx::destroy(context.program);
  bgfx::destroy(context.u_color);

  ImGui_ImplSDL3_Shutdown();
  ImGui_Implbgfx_Shutdown();

  ImGui::DestroyContext();
  bgfx::shutdown();

  SDL_DestroyWindow(context.window);
  SDL_Quit();
}