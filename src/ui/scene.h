// What a page looks like, independent of the toolkit that paints it. Layout
// produces a Scene; a flavor turns a Scene into pixels. Neither knows the
// other, which is what keeps the two flavors showing the same design.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "ui/design.h"

namespace onboarding::ui {

struct Rect {
  float x = 0, y = 0, w = 0, h = 0;
  float right() const { return x + w; }
  float bottom() const { return y + h; }
  float center_x() const { return x + w / 2; }
};

struct Panel {
  Rect rect;
  Color fill = color::kSurface;
  bool border = true;
};

struct TextRun {
  Rect box;
  std::string text;
  TextSize size = TextSize::kBody;
  Color color = color::kText;
  TextAlign align = TextAlign::kLeft;
};

struct Bar {
  Rect rect;
  Color track = color::kSurfaceRaised;
  Color fill = color::kAccent;
  // Clamped to [0, 1] by the renderer.
  float fraction = 0;
};

struct QrBlock {
  Rect box;
  // Modules run row-major, 1 is dark. The renderer adds the quiet zone.
  int modules = 0;
  std::vector<uint8_t> bits;
};

struct Button {
  Rect rect;
  std::string label;
  bool enabled = true;
};

struct Scene {
  Color background = color::kBackground;
  std::vector<Panel> panels;
  std::vector<TextRun> texts;
  std::vector<Bar> bars;
  std::optional<QrBlock> qr;
  std::vector<Button> buttons;
  // Index into `buttons`, or -1.
  int focused = -1;
};

}  // namespace onboarding::ui
