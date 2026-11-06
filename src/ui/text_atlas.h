// Glyph atlas for the sdl3 flavor, rasterized with the stb_truetype copy that
// ships inside ImGui. The imgui flavor uses ImGui's own atlas, which is the
// same rasterizer over the same font.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>

#include "ui/design.h"
#include "ui/scene.h"

namespace onboarding::ui {

class TextAtlas {
 public:
  TextAtlas();
  ~TextAtlas();
  TextAtlas(const TextAtlas&) = delete;
  TextAtlas& operator=(const TextAtlas&) = delete;

  bool Load(const unsigned char* ttf, size_t size, std::string* error);
  bool CreateTexture(SDL_Renderer* renderer, std::string* error);

  float Width(TextSize size, std::string_view text) const;
  // Draws `text` inside `box`, using the line height of its size.
  void Draw(SDL_Renderer* renderer, TextSize size, Color color, const Rect& box, TextAlign align,
            std::string_view text) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace onboarding::ui
