#include "ui/view.h"

#include <utility>

#include "core/app.h"
#include "ui/design.h"
#include "ui/input.h"

namespace onboarding::ui {
namespace {

bool Present(Session& session) {
  SDL_RenderPresent(session.renderer);
  return true;
}

}  // namespace

bool OpenSession(const ViewOptions& options, Session* session, std::string* error) {
  // XInput is absent in an AppContainer; WGI is the only gamepad backend there
  // and SDL disables it by default.
  SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "1");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    if (error) *error = SDL_GetError();
    return false;
  }
#if defined(REX_PLATFORM_WINRT)
  // The GLES fallback faults inside SDL on an Xbox One S; D3D11 is the
  // console's renderer.
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#endif
  if (!SDL_CreateWindowAndRenderer(options.window_title, static_cast<int>(kCanvasWidth),
                                   static_cast<int>(kCanvasHeight), SDL_WINDOW_RESIZABLE,
                                   &session->window, &session->renderer)) {
    if (error) *error = SDL_GetError();
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    return false;
  }
  SDL_SetRenderLogicalPresentation(session->renderer, static_cast<int>(kCanvasWidth),
                                   static_cast<int>(kCanvasHeight),
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetRenderVSync(session->renderer, 1);
  session->flavor = Renderer::Create(session->window, session->renderer, error);
  if (!session->flavor) {
    SDL_DestroyRenderer(session->renderer);
    SDL_DestroyWindow(session->window);
    session->renderer = nullptr;
    session->window = nullptr;
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    return false;
  }
  return true;
}

void CloseSession(Session* session) {
  session->flavor.reset();
  if (session->renderer) SDL_DestroyRenderer(session->renderer);
  if (session->window) SDL_DestroyWindow(session->window);
  session->renderer = nullptr;
  session->window = nullptr;
  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
}

void DrawFrame(Session& session, const Scene& scene) { session.flavor->Draw(scene); }

bool CaptureFrame(Session& session, const Scene& scene, const std::filesystem::path& png,
                  std::string* error) {
  session.flavor->Draw(scene);
  SDL_Surface* surface = SDL_RenderReadPixels(session.renderer, nullptr);
  if (!surface) {
    if (error) *error = SDL_GetError();
    return false;
  }
  const bool saved = SDL_SavePNG(surface, png.string().c_str());
  if (!saved && error) *error = SDL_GetError();
  SDL_DestroySurface(surface);
  return saved;
}

bool Run(App& app, Session& session) {
  Focus focus;
  focus.on_activate = [&app](int index) { app.Activate(index); };
  bool running = true;
  while (running) {
    app.Advance();
    Scene scene = app.BuildScene(focus.index());
    focus.SetButtons(scene.buttons);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
        break;
      }
      if (event.type == SDL_EVENT_GAMEPAD_ADDED) SDL_OpenGamepad(event.gdevice.which);
      SDL_ConvertEventToRenderCoordinates(session.renderer, &event);
      focus.HandleEvent(event);
    }
    if (!running) break;

    scene.focused = focus.index() < static_cast<int>(scene.buttons.size()) ? focus.index() : -1;
    DrawFrame(session, scene);
    Present(session);
    if (app.done()) return true;
  }
  return false;
}

}  // namespace onboarding::ui
