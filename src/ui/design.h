// The design spec in code: colours, type scale and spacing for both flavors.
// Nothing here knows about a toolkit.
#pragma once

#include <cstdint>

namespace onboarding::ui {

struct Color {
  uint8_t r, g, b, a = 255;
};

// The page is designed at 1280x720 and letterboxed to whatever the screen is.
constexpr float kCanvasWidth = 1280;
constexpr float kCanvasHeight = 720;
constexpr float kMargin = 72;

namespace color {

// Flat-neutral: neutral greys, one accent, no gradients and no brand colour.
constexpr Color kBackground{16, 16, 18};
constexpr Color kSurface{23, 23, 26};
constexpr Color kSurfaceRaised{31, 31, 35};
constexpr Color kBorder{255, 255, 255, 20};
constexpr Color kText{242, 242, 244};
constexpr Color kTextMuted{154, 154, 162};
// The system accent: focus rings, progress fills and the primary action.
constexpr Color kAccent{76, 194, 255};
// Semantic, not decorative: a verified file and a rejected one.
constexpr Color kSuccess{108, 203, 95};
constexpr Color kError{255, 153, 164};

}  // namespace color

enum class TextSize {
  kTitle,
  kBody,
  kSmall,
};

constexpr float kTitlePx = 44;
constexpr float kBodyPx = 26;
constexpr float kSmallPx = 22;

constexpr float TextPx(TextSize size) {
  switch (size) {
    case TextSize::kTitle: return kTitlePx;
    case TextSize::kBody: return kBodyPx;
    case TextSize::kSmall: return kSmallPx;
  }
  return kSmallPx;
}

// Line box height for a size, with enough leading that two lines never touch.
constexpr float LineHeight(TextSize size) { return TextPx(size) * 1.4f; }

enum class TextAlign {
  kLeft,
  kCenter,
};

}  // namespace onboarding::ui
