// The flavor seam. One implementation of Renderer is compiled per build:
// render_sdl3.cpp paints the scene with SDL primitives and an atlas built from
// the embedded font, render_imgui.cpp paints it with ImGui's draw lists.
#pragma once

#include <memory>
#include <string>

#include <SDL3/SDL.h>

#include "ui/scene.h"

namespace onboarding::ui {

class Renderer {
 public:
  static std::unique_ptr<Renderer> Create(SDL_Window* window, SDL_Renderer* renderer,
                                          std::string* error);
  virtual ~Renderer() = default;
  virtual void Draw(const Scene& scene) = 0;
};

}  // namespace onboarding::ui
