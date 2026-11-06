// The sdl3 flavor: SDL renderer primitives plus a glyph atlas. No third-party
// UI library is linked.
#include "ui/render.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

#include "font_data.h"
#include "ui/design.h"
#include "ui/text_atlas.h"

namespace onboarding::ui {
namespace {

void SetColor(SDL_Renderer* renderer, Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void Fill(SDL_Renderer* renderer, const Rect& rect, Color color) {
  SetColor(renderer, color);
  const SDL_FRect area{rect.x, rect.y, rect.w, rect.h};
  SDL_RenderFillRect(renderer, &area);
}

void Stroke(SDL_Renderer* renderer, const Rect& rect, Color color) {
  SetColor(renderer, color);
  const SDL_FRect area{rect.x, rect.y, rect.w, rect.h};
  SDL_RenderRect(renderer, &area);
}

void StrokeThick(SDL_Renderer* renderer, const Rect& rect, Color color, float thickness) {
  Fill(renderer, {rect.x, rect.y, rect.w, thickness}, color);
  Fill(renderer, {rect.x, rect.bottom() - thickness, rect.w, thickness}, color);
  Fill(renderer, {rect.x, rect.y, thickness, rect.h}, color);
  Fill(renderer, {rect.right() - thickness, rect.y, thickness, rect.h}, color);
}

}  // namespace

class Sdl3Renderer final : public Renderer {
 public:
  explicit Sdl3Renderer(SDL_Renderer* renderer) : renderer_(renderer) {}

  bool Init(std::string* error) {
    return atlas_.Load(kFontData, sizeof(kFontData), error) &&
           atlas_.CreateTexture(renderer_, error);
  }

  void Draw(const Scene& scene) override {
    SetColor(renderer_, scene.background);
    SDL_RenderClear(renderer_);

    for (const Panel& panel : scene.panels) {
      Fill(renderer_, panel.rect, panel.fill);
      if (panel.border) Stroke(renderer_, panel.rect, color::kBorder);
    }
    if (scene.qr) DrawQr(*scene.qr);
    for (size_t i = 0; i < scene.buttons.size(); ++i) {
      DrawButton(scene.buttons[i], static_cast<int>(i) == scene.focused);
    }
    for (const Bar& bar : scene.bars) DrawBar(bar);
    for (const TextRun& run : scene.texts) {
      atlas_.Draw(renderer_, run.size, run.color, run.box, run.align, run.text);
    }
  }

 private:
  void DrawQr(const QrBlock& qr) {
    // Four modules of quiet zone on each side, as the standard asks.
    const int span = qr.modules + 8;
    const float cell = std::floor(std::min(qr.box.w, qr.box.h) / span);
    const float offset_x = qr.box.x + (qr.box.w - cell * span) / 2;
    const float offset_y = qr.box.y + (qr.box.h - cell * span) / 2;
    Fill(renderer_, {offset_x, offset_y, cell * span, cell * span}, Color{255, 255, 255});
    for (int row = 0; row < qr.modules; ++row) {
      for (int col = 0; col < qr.modules; ++col) {
        if (!qr.bits[static_cast<size_t>(row) * qr.modules + col]) continue;
        Fill(renderer_,
             {offset_x + (col + 4) * cell, offset_y + (row + 4) * cell, cell, cell},
             Color{0, 0, 0});
      }
    }
  }

  void DrawButton(const Button& button, bool focused) {
    Fill(renderer_, button.rect, button.enabled ? color::kSurfaceRaised : color::kSurface);
    if (focused && button.enabled) {
      StrokeThick(renderer_, button.rect, color::kAccent, 2);
    } else {
      Stroke(renderer_, button.rect, color::kBorder);
    }
    atlas_.Draw(renderer_, TextSize::kBody, button.enabled ? color::kText : color::kTextMuted,
                button.rect, TextAlign::kCenter, button.label);
  }

  void DrawBar(const Bar& bar) {
    Fill(renderer_, bar.rect, bar.track);
    const float fraction = std::clamp(bar.fraction, 0.0f, 1.0f);
    if (fraction <= 0) return;
    Fill(renderer_, {bar.rect.x, bar.rect.y, bar.rect.w * fraction, bar.rect.h}, bar.fill);
  }

  SDL_Renderer* renderer_;
  TextAtlas atlas_;
};

std::unique_ptr<Renderer> Renderer::Create(SDL_Window* window, SDL_Renderer* renderer,
                                           std::string* error) {
  (void)window;
  auto flavor = std::make_unique<Sdl3Renderer>(renderer);
  if (!flavor->Init(error)) return nullptr;
  return flavor;
}

}  // namespace onboarding::ui
