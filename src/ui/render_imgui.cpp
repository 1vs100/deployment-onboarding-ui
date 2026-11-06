// The imgui flavor: the same scene painted with ImGui draw lists, using ImGui's
// SDL3 and SDL renderer backends.
#include "ui/render.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "font_data.h"
#include "ui/design.h"

namespace onboarding::ui {
namespace {

ImU32 ToIm(Color color) { return IM_COL32(color.r, color.g, color.b, color.a); }

ImVec2 Point(float x, float y) { return ImVec2(x, y); }

}  // namespace

class ImGuiRenderer final : public Renderer {
 public:
  explicit ImGuiRenderer(SDL_Renderer* renderer) : renderer_(renderer) {}

  ~ImGuiRenderer() override {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }

  bool Init(SDL_Window* window, std::string* error) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    // The view drives focus itself, so ImGui handles no navigation.
    io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad);

    auto* data = const_cast<unsigned char*>(kFontData);
    const int size = static_cast<int>(sizeof(kFontData));
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    fonts_[0] = io.Fonts->AddFontFromMemoryTTF(data, size, kTitlePx, &config);
    fonts_[1] = io.Fonts->AddFontFromMemoryTTF(data, size, kBodyPx, &config);
    fonts_[2] = io.Fonts->AddFontFromMemoryTTF(data, size, kSmallPx, &config);

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer_)) {
      if (error) *error = "imgui SDL3 backend failed";
      return false;
    }
    if (!ImGui_ImplSDLRenderer3_Init(renderer_)) {
      if (error) *error = "imgui SDL renderer backend failed";
      return false;
    }
    return true;
  }

  void Draw(const Scene& scene) override {
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    draw->AddRectFilled(Point(0, 0), Point(kCanvasWidth, kCanvasHeight), ToIm(scene.background));

    for (const Panel& panel : scene.panels) {
      draw->AddRectFilled(Point(panel.rect.x, panel.rect.y),
                          Point(panel.rect.right(), panel.rect.bottom()), ToIm(panel.fill));
      if (panel.border) {
        draw->AddRect(Point(panel.rect.x, panel.rect.y),
                      Point(panel.rect.right(), panel.rect.bottom()), ToIm(color::kBorder));
      }
    }
    if (scene.qr) DrawQr(draw, *scene.qr);
    for (size_t i = 0; i < scene.buttons.size(); ++i) {
      DrawButton(draw, scene.buttons[i], static_cast<int>(i) == scene.focused);
    }
    for (const Bar& bar : scene.bars) {
      draw->AddRectFilled(Point(bar.rect.x, bar.rect.y), Point(bar.rect.right(), bar.rect.bottom()),
                          ToIm(bar.track));
      const float fraction = std::clamp(bar.fraction, 0.0f, 1.0f);
      if (fraction > 0) {
        draw->AddRectFilled(Point(bar.rect.x, bar.rect.y),
                            Point(bar.rect.x + bar.rect.w * fraction, bar.rect.bottom()),
                            ToIm(bar.fill));
      }
    }
    for (const TextRun& run : scene.texts) DrawText(draw, run);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
  }

 private:
  ImFont* Font(TextSize size) const {
    switch (size) {
      case TextSize::kTitle: return fonts_[0];
      case TextSize::kBody: return fonts_[1];
      case TextSize::kSmall: return fonts_[2];
    }
    return fonts_[2];
  }

  void DrawText(ImDrawList* draw, const TextRun& run) const {
    ImFont* font = Font(run.size);
    const float px = TextPx(run.size);
    const float y = run.box.y + (run.box.h - px) / 2;
    float x = run.box.x;
    if (run.align == TextAlign::kCenter) {
      x = run.box.center_x() - font->CalcTextSizeA(px, kCanvasWidth, 0, run.text.c_str()).x / 2;
    }
    draw->AddText(font, px, Point(x, y), ToIm(run.color), run.text.c_str());
  }

  void DrawQr(ImDrawList* draw, const QrBlock& qr) {
    const int span = qr.modules + 8;
    const float cell = std::floor(std::min(qr.box.w, qr.box.h) / span);
    const float offset_x = qr.box.x + (qr.box.w - cell * span) / 2;
    const float offset_y = qr.box.y + (qr.box.h - cell * span) / 2;
    draw->AddRectFilled(Point(offset_x, offset_y), Point(offset_x + cell * span, offset_y + cell * span),
                        IM_COL32(255, 255, 255, 255));
    for (int row = 0; row < qr.modules; ++row) {
      for (int col = 0; col < qr.modules; ++col) {
        if (!qr.bits[static_cast<size_t>(row) * qr.modules + col]) continue;
        const float x = offset_x + (col + 4) * cell;
        const float y = offset_y + (row + 4) * cell;
        draw->AddRectFilled(Point(x, y), Point(x + cell, y + cell), IM_COL32(0, 0, 0, 255));
      }
    }
  }

  void DrawButton(ImDrawList* draw, const Button& button, bool focused) const {
    const bool highlight = focused && button.enabled;
    draw->AddRectFilled(Point(button.rect.x, button.rect.y),
                        Point(button.rect.right(), button.rect.bottom()),
                        ToIm(button.enabled ? color::kSurfaceRaised : color::kSurface));
    if (highlight) {
      draw->AddRect(Point(button.rect.x, button.rect.y),
                    Point(button.rect.right(), button.rect.bottom()), ToIm(color::kAccent), 0, 0, 2);
    } else {
      draw->AddRect(Point(button.rect.x, button.rect.y),
                    Point(button.rect.right(), button.rect.bottom()), ToIm(color::kBorder));
    }
    ImFont* font = Font(TextSize::kBody);
    const float width = font->CalcTextSizeA(kBodyPx, kCanvasWidth, 0, button.label.c_str()).x;
    const float x = button.rect.center_x() - width / 2;
    const float y = button.rect.y + (button.rect.h - kBodyPx) / 2;
    draw->AddText(font, kBodyPx, Point(x, y), ToIm(button.enabled ? color::kText : color::kTextMuted),
                  button.label.c_str());
  }

  SDL_Renderer* renderer_;
  ImFont* fonts_[3] = {nullptr, nullptr, nullptr};
};

std::unique_ptr<Renderer> Renderer::Create(SDL_Window* window, SDL_Renderer* renderer,
                                           std::string* error) {
  auto flavor = std::make_unique<ImGuiRenderer>(renderer);
  if (!flavor->Init(window, error)) return nullptr;
  return flavor;
}

}  // namespace onboarding::ui
