#include "ui/input.h"

#include <cmath>
#include <limits>

namespace onboarding::ui {
namespace {

constexpr Sint16 kStickThreshold = 16000;

}  // namespace

void Focus::SetButtons(const std::vector<Button>& buttons) {
  buttons_ = buttons;
  if (index_ >= static_cast<int>(buttons_.size())) index_ = 0;
}

int Focus::HitTest(float x, float y) const {
  const SDL_FPoint point{x, y};
  for (size_t i = 0; i < buttons_.size(); ++i) {
    const Rect& rect = buttons_[i].rect;
    const SDL_FRect box{rect.x, rect.y, rect.w, rect.h};
    if (SDL_PointInRectFloat(&point, &box)) return static_cast<int>(i);
  }
  return -1;
}

void Focus::Activate(int index) {
  if (index < 0 || index >= static_cast<int>(buttons_.size())) return;
  if (!buttons_[index].enabled || !on_activate) return;
  on_activate(index);
}

void Focus::Move(int dx, int dy) {
  if (buttons_.empty()) return;
  // Pick the nearest button in the pressed direction.
  const Rect& from = buttons_[index_].rect;
  const float fx = from.center_x();
  const float fy = from.y + from.h / 2;
  int best = -1;
  float best_score = std::numeric_limits<float>::max();
  for (size_t i = 0; i < buttons_.size(); ++i) {
    if (static_cast<int>(i) == index_ || !buttons_[i].enabled) continue;
    const Rect& to = buttons_[i].rect;
    const float tx = to.center_x() - fx;
    const float ty = to.y + to.h / 2 - fy;
    const float along = dx ? tx * dx : ty * dy;
    const float across = std::fabs(dx ? ty : tx);
    if (along <= 0) continue;
    const float score = along + across * 2;
    if (score < best_score) {
      best_score = score;
      best = static_cast<int>(i);
    }
  }
  if (best >= 0) index_ = best;
}

bool Focus::HandleEvent(const SDL_Event& event) {
  const int count = static_cast<int>(buttons_.size());
  switch (event.type) {
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      switch (event.gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP: Move(0, -1); return true;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: Move(0, 1); return true;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: Move(-1, 0); return true;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: Move(1, 0); return true;
        case SDL_GAMEPAD_BUTTON_SOUTH: Activate(index_); return true;
        default: return false;
      }
    case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
      int* last = nullptr;
      if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) last = &stick_x_;
      if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) last = &stick_y_;
      if (!last) return false;
      const int direction = event.gaxis.value > kStickThreshold    ? 1
                           : event.gaxis.value < -kStickThreshold ? -1
                                                                  : 0;
      if (direction != *last && direction != 0) {
        if (last == &stick_x_) {
          Move(direction, 0);
        } else {
          Move(0, direction);
        }
      }
      *last = direction;
      return true;
    }
    case SDL_EVENT_KEY_DOWN:
      switch (event.key.key) {
        case SDLK_UP: Move(0, -1); return true;
        case SDLK_DOWN: Move(0, 1); return true;
        case SDLK_LEFT: Move(-1, 0); return true;
        case SDLK_RIGHT: Move(1, 0); return true;
        case SDLK_TAB:
          if (count > 0) {
            const bool back = (event.key.mod & SDL_KMOD_SHIFT) != 0;
            for (int step = 0; step < count; ++step) {
              index_ = (index_ + (back ? count - 1 : 1)) % count;
              if (buttons_[index_].enabled) break;
            }
          }
          return true;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
          Activate(index_);
          return true;
        default:
          return false;
      }
    case SDL_EVENT_MOUSE_MOTION: {
      const int hit = HitTest(event.motion.x, event.motion.y);
      if (hit >= 0 && buttons_[hit].enabled) index_ = hit;
      return false;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
      if (event.button.button == SDL_BUTTON_LEFT) {
        const int hit = HitTest(event.button.x, event.button.y);
        if (hit >= 0 && buttons_[hit].enabled) {
          index_ = hit;
          Activate(hit);
          return true;
        }
      }
      return false;
    default:
      return false;
  }
}

}  // namespace onboarding::ui
