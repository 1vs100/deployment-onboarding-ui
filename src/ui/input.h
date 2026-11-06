// One focused button at a time. Controller, keyboard and mouse all drive the
// same focus, and a flavor never sees an input event.
#pragma once

#include <functional>
#include <vector>

#include <SDL3/SDL.h>

#include "ui/scene.h"

namespace onboarding::ui {

class Focus {
 public:
  // Replaces the buttons, keeping the focus index when it still exists.
  void SetButtons(const std::vector<Button>& buttons);
  // Returns true when the event changed or used the focus.
  bool HandleEvent(const SDL_Event& event);

  int index() const { return index_; }

  // Called with the index of the activated button.
  std::function<void(int)> on_activate;

 private:
  void Move(int dx, int dy);
  void Activate(int index);
  int HitTest(float x, float y) const;

  std::vector<Button> buttons_;
  int index_ = 0;
  // Last left-stick direction, so a held stick moves focus once.
  int stick_x_ = 0;
  int stick_y_ = 0;
};

}  // namespace onboarding::ui
