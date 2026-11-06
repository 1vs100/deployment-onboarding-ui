// The window and frame loop, shared by the gate and by the demo that captures
// screenshots. Which toolkit paints a frame is the only thing a flavor decides.
#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <SDL3/SDL.h>

#include "ui/render.h"
#include "ui/scene.h"

namespace onboarding {
class App;
}

namespace onboarding::ui {

struct ViewOptions {
  const char* window_title = "Setup";
};

struct Session {
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  std::unique_ptr<Renderer> flavor;
};

// Starts SDL, opens the window and creates the flavor renderer.
bool OpenSession(const ViewOptions& options, Session* session, std::string* error);
void CloseSession(Session* session);

// Paints one frame. The caller presents.
void DrawFrame(Session& session, const Scene& scene);
// Writes the current backbuffer to `png`.
bool CaptureFrame(Session& session, const Scene& scene, const std::filesystem::path& png,
                  std::string* error);

// Runs the flow until the player continues or closes the window. Returns true
// when they continued.
bool Run(App& app, Session& session);

}  // namespace onboarding::ui
